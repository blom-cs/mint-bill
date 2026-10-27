# Database

Mint-Bill stores each business in a plain Turso file. The file is not encrypted, and the application does not use the system keyring.

Connections to the same file share one database inside the process. Writes use `BEGIN CONCURRENT` and retry when two transactions change the same row.

The application does not call a Turso command-line tool. A plain SQLite file can be opened until Turso rewrites it; after that, the file and its `-log` sidecar are the database.

# Test databases

Unit tests create temporary databases under `/tmp`. The files in `storage/tests/` are plain databases used by the model tests.
