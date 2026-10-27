/*******************************************************************************
 * @file sqlite_test.cpp
 * @brief Tests for a plain Turso database: open, bind, and concurrent writes.
 ******************************************************************************/
#include "CppUTest/TestHarness.h"

#include <atomic>
#include <filesystem>
#include <sqlite.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
#include <errors.h>


namespace {
std::string temporary_database()
{
	static int sequence{0};
	return "/tmp/mint-bill-turso-" + std::to_string(::getpid()) + "-" + std::to_string(++sequence) + ".db";
}

void remove_database(const std::string& path)
{
	std::filesystem::remove(path);
	std::filesystem::remove(path + "-wal");
	std::filesystem::remove(path + "-shm");
	std::filesystem::remove(path + "-log");
}

long long integer_at(const storage::database::part::rows& rows, std::size_t row, std::size_t column)
{
	return static_cast<long long> (std::get<sqlite3_int64> (rows.at(row).at(column)));
}
}

TEST_GROUP(sqlite_test)
{
	std::string path;
	void setup()
	{
		path = temporary_database();
		remove_database(path);
	}

	void teardown()
	{
		remove_database(path);
	}
};

TEST(sqlite_test, empty_path_throws)
{
	CHECK_THROWS(app::errors, storage::database::sqlite(""));
}

TEST(sqlite_test, missing_directory_throws)
{
	CHECK_THROWS(app::errors, storage::database::sqlite("/no/such/mint-bill-dir/plain.db"));
}

TEST(sqlite_test, opens_in_mvcc_mode)
{
	storage::database::sqlite db{path};
	storage::database::part::rows rows{db.select("PRAGMA journal_mode;")};

	CHECK_EQUAL(1, rows.size());
	STRCMP_EQUAL("mvcc", std::get<std::string> (rows.at(0).at(0)).c_str());
}

TEST(sqlite_test, empty_statement_is_rejected)
{
	storage::database::sqlite db{path};

	CHECK_FALSE(db.transaction(""));
	CHECK_FALSE(db.usert("", {std::string{"x"}}));
	CHECK_TRUE(db.select("").empty());
	CHECK_TRUE(db.select("SELECT 1", {}).empty());
}

TEST(sqlite_test, round_trips_bound_values)
{
	storage::database::sqlite db{path};
	CHECK_TRUE(db.transaction("CREATE TABLE item (id INTEGER PRIMARY KEY, name TEXT, amount REAL, note BLOB);"));

	storage::database::blob note{std::byte{0x01}, std::byte{0x02}, std::byte{0xff}};
	CHECK_TRUE(db.usert(
		"INSERT INTO item (name, amount, note) VALUES (?, ?, ?);",
		{std::string{"bolt"}, 1.5, note}));

	storage::database::part::rows rows{db.select(
		"SELECT name, amount, note FROM item WHERE id = ?;",
		{sqlite3_int64{1}})};

	CHECK_EQUAL(1, rows.size());
	STRCMP_EQUAL("bolt", std::get<std::string> (rows.at(0).at(0)).c_str());
	DOUBLES_EQUAL(1.5, std::get<double> (rows.at(0).at(1)), 0.001);
	CHECK_EQUAL(note.size(), std::get<storage::database::blob> (rows.at(0).at(2)).size());
	CHECK_EQUAL(0, std::memcmp(note.data(), std::get<storage::database::blob> (rows.at(0).at(2)).data(), note.size()));
}

TEST(sqlite_test, wrong_parameter_count_does_not_write)
{
	storage::database::sqlite db{path};
	CHECK_TRUE(db.transaction("CREATE TABLE item (id INTEGER PRIMARY KEY, name TEXT);"));

	CHECK_FALSE(db.usert("INSERT INTO item (name) VALUES (?);", {std::string{"a"}, std::string{"b"}}));
	CHECK_TRUE(db.select("SELECT name FROM item;").empty());
}

TEST(sqlite_test, failed_work_rolls_back)
{
	storage::database::sqlite db{path};
	CHECK_TRUE(db.transaction("CREATE TABLE item (id INTEGER PRIMARY KEY, name TEXT);"));

	CHECK_FALSE(db.concurrently([&db] () {
		CHECK_TRUE(db.usert("INSERT INTO item (name) VALUES (?);", {std::string{"gone"}}));
		return false;
	}));

	CHECK_TRUE(db.select("SELECT name FROM item;").empty());
}

TEST(sqlite_test, second_connection_sees_a_committed_row)
{
	storage::database::sqlite db{path};
	CHECK_TRUE(db.transaction("CREATE TABLE item (id INTEGER PRIMARY KEY, name TEXT);"));
	CHECK_TRUE(db.usert("INSERT INTO item (name) VALUES (?);", {std::string{"ada"}}));

	storage::database::sqlite other{path};
	storage::database::part::rows rows{other.select("SELECT name FROM item;")};

	CHECK_EQUAL(1, rows.size());
	STRCMP_EQUAL("ada", std::get<std::string> (rows.at(0).at(0)).c_str());
}

TEST(sqlite_test, concurrent_increments_do_not_lose_updates)
{
	constexpr int writers{8};
	{
		storage::database::sqlite db{path};
		CHECK_TRUE(db.transaction("CREATE TABLE counter (id INTEGER PRIMARY KEY, val INTEGER);"));
		CHECK_TRUE(db.usert("INSERT INTO counter (id, val) VALUES (?, ?);", {sqlite3_int64{1}, sqlite3_int64{0}}));
	}

	std::atomic<int> arrived{0};
	std::atomic<int> wins{0};
	std::vector<std::thread> threads;
	threads.reserve(writers);
	for (int index{0}; index < writers; ++index)
	{
		threads.emplace_back([&] () {
			arrived.fetch_add(1);
			while (arrived.load() < writers)
			{
				std::this_thread::yield();
			}

			storage::database::sqlite db{path};
			bool saved{db.concurrently([&db] () {
				storage::database::part::rows rows{db.select(
					"SELECT val FROM counter WHERE id = ?;",
					{sqlite3_int64{1}})};
				if (rows.empty() == true)
				{
					return false;
				}
				sqlite3_int64 next{static_cast<sqlite3_int64> (integer_at(rows, 0, 0) + 1)};
				return db.usert(
					"UPDATE counter SET val = ? WHERE id = ?;",
					{next, sqlite3_int64{1}});
			}, 64)};
			if (saved == true)
			{
				wins.fetch_add(1);
			}
		});
	}
	for (std::thread& thread : threads)
	{
		thread.join();
	}

	storage::database::sqlite db{path};
	storage::database::part::rows rows{db.select("SELECT val FROM counter WHERE id = ?;", {sqlite3_int64{1}})};
	CHECK_EQUAL(writers, wins.load());
	CHECK_EQUAL(writers, integer_at(rows, 0, 0));
}

TEST(sqlite_test, concurrent_inserts_of_different_rows_all_commit)
{
	constexpr int writers{8};
	{
		storage::database::sqlite db{path};
		CHECK_TRUE(db.transaction("CREATE TABLE item (id INTEGER PRIMARY KEY, name TEXT);"));
	}

	std::atomic<int> wins{0};
	std::vector<std::thread> threads;
	threads.reserve(writers);
	for (int index{0}; index < writers; ++index)
	{
		threads.emplace_back([this, index, &wins] () {
			storage::database::sqlite db{path};
			if (db.concurrently([index, &db] () {
				return db.usert(
					"INSERT INTO item (id, name) VALUES (?, ?);",
					{sqlite3_int64{index + 1}, std::string{"row"}});
			}) == true)
			{
				wins.fetch_add(1);
			}
		});
	}
	for (std::thread& thread : threads)
	{
		thread.join();
	}

	storage::database::sqlite db{path};
	storage::database::part::rows rows{db.select("SELECT COUNT(*) FROM item;")};
	CHECK_EQUAL(writers, wins.load());
	CHECK_EQUAL(writers, integer_at(rows, 0, 0));
}
