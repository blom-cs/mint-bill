##############################################################
# Contents: Paths to source files used by CppUTest
# Author: Dawid Blom.
# Date: September 18, 2022
#
# NOTE: This Makefile adds value to the CppUTests framework
# by specifying the folders where the code is to be tested.
##############################################################
PROJECT_DIR :=$(realpath ..)
TEST_DIR :=$(realpath .)
TEST_TARGET := mint-bill
CPPUTEST_EXE_FLAGS +=-c -v

# CPPUTEST_EXE_FLAGS +=-xg "email_test client_invoice_test"
# CPPUTEST_EXE_FLAGS +=-xg "client_invoice_test"
# CPPUTEST_EXE_FLAGS +=-sg "date_manager_test"
# CPPUTEST_EXE_FLAGS +=-sg "client_invoice_test"
# CPPUTEST_EXE_FLAGS +=-sg "business_data_test"
# CPPUTEST_EXE_FLAGS +=-sg "admin_data_test"
# CPPUTEST_EXE_FLAGS +=-sg "client_data_test"
# CPPUTEST_EXE_FLAGS +=-sg "invoice_data_test"
# CPPUTEST_EXE_FLAGS +=-sg "statement_data_test"
# CPPUTEST_EXE_FLAGS +=-sg "billing_data_test"
# CPPUTEST_EXE_FLAGS +=-sg "column_data_test"
# CPPUTEST_EXE_FLAGS +=-sg "pdf_invoice_data_test"
# CPPUTEST_EXE_FLAGS +=-sg "pdf_statement_data_test"
# CPPUTEST_EXE_FLAGS +=-sg "admin_model_test"
# CPPUTEST_EXE_FLAGS +=-sg "client_model_test"
# CPPUTEST_EXE_FLAGS +=-sg "invoice_model_test"
# CPPUTEST_EXE_FLAGS +=-sg "statement_model_test"
# CPPUTEST_EXE_FLAGS +=-sg "business_serialize_test"
# CPPUTEST_EXE_FLAGS +=-sg "admin_serialize_test"
# CPPUTEST_EXE_FLAGS +=-sg "client_serialize_test"
# CPPUTEST_EXE_FLAGS +=-sg "column_serialize_test"
# CPPUTEST_EXE_FLAGS +=-sg "invoice_serialize_test"
# CPPUTEST_EXE_FLAGS +=-sg "statement_serialize_test"
# CPPUTEST_EXE_FLAGS +=-sg "pdf_window_test"
# CPPUTEST_EXE_FLAGS +=-sg "sub_button_test"
# CPPUTEST_EXE_FLAGS +=-sg "stack_test"
# CPPUTEST_EXE_FLAGS +=-sg "gui_part_search_bar"
# CPPUTEST_EXE_FLAGS +=-sg "gui_part_button"
# CPPUTEST_EXE_FLAGS +=-sg "invoice_pdf_view_test"
# CPPUTEST_EXE_FLAGS +=-sg "statement_page_test"
# CPPUTEST_EXE_FLAGS +=-sg "gui_part_dialog"
# CPPUTEST_EXE_FLAGS +=-sg "page_range_test"
# CPPUTEST_EXE_FLAGS +=-sg "sqlite_test"
# CPPUTEST_EXE_FLAGS +=-sg "sql_operations_test"
# CPPUTEST_EXE_FLAGS +=-sg "sql_row_processing_test"
# CPPUTEST_EXE_FLAGS +=-sg "password_manager_test"
# CPPUTEST_EXE_FLAGS +=-sg "generate_invoice_pdf_test"
# CPPUTEST_EXE_FLAGS +=-sg "generate_statement_pdf_test"
# CPPUTEST_EXE_FLAGS +=-sg "statement_pdf_view_test"
# CPPUTEST_EXE_FLAGS +=-sg "statement_page_column_view_test"

CPPUTEST_CXXFLAGS += -DMINTBILL_DB_PATH=\"../storage/test/model_test.db\"
CPPUTEST_CXXFLAGS += -DMINTBILL_UI_PATH=\"../gui/mint-bill.ui\"
CPPUTEST_CXXFLAGS += -DMINTBILL_CSS_PATH=\"../gui/mint-bill.css\"
CPPUTEST_CXXFLAGS += -DMINTBILL_EMAIL_TXT_PATH=\"../email.txt\"
CPPUTEST_CXXFLAGS += -DMINTBILL_EMAIL_HTML_PATH=\"../email.html\"

ifneq ($(wildcard /tmp/turso-src/target/release/libturso_sqlite3.a),)
TURSO_LIBRARY ?= /tmp/turso-src/target/release/libturso_sqlite3.a
else
TURSO_LIBRARY ?= /opt/turso/libturso_sqlite3.a
endif

CPPUTEST_CXXFLAGS +=-std=c++23 
CPPUTEST_CXXFLAGS += $(shell pkg-config --cflags cairomm-1.16 gtkmm-4.0 poppler-cpp)
CPPUTEST_CXXFLAGS += -I$(PROJECT_DIR)/third_party/turso/include
CPPUTEST_CFLAGS += $(shell pkg-config --cflags poppler-cpp)
CPPUTEST_CFLAGS += $(shell curl-config --cflags)
CPPUTEST_LDFLAGS += $(shell pkg-config --libs cairomm-1.16 gtkmm-4.0 poppler-cpp)
CPPUTEST_LDFLAGS += $(shell curl-config --libs)
CPPUTEST_LDFLAGS += $(TURSO_LIBRARY) -ldl -lpthread -lm 


SRC_DIRS :=\
	$(PROJECT_DIR)/gui/source\
	$(PROJECT_DIR)/gui/components/source\
	$(PROJECT_DIR)/data/source\
	$(PROJECT_DIR)/utility/source\
	$(PROJECT_DIR)/storage/source\
	$(PROJECT_DIR)/features/source\
	$(PROJECT_DIR)/models/source\


INCLUDE_DIRS :=\
	$(CPPUTEST_HOME)/include\
	$(PROJECT_DIR)/app/include\
	$(PROJECT_DIR)/gui/include\
	$(PROJECT_DIR)/gui/components/include\
	$(PROJECT_DIR)/data/include\
	$(PROJECT_DIR)/data/mocks/include\
	$(PROJECT_DIR)/utility/include\
	$(PROJECT_DIR)/storage/include\
	$(PROJECT_DIR)/storage/mocks/include\
	$(PROJECT_DIR)/features/include\
	$(PROJECT_DIR)/features/mocks/include\
	$(PROJECT_DIR)/features/tests/include\
	$(PROJECT_DIR)/models/include\


TEST_SRC_DIRS :=\
	$(TEST_DIR)\
	$(PROJECT_DIR)/gui/tests\
	$(PROJECT_DIR)/gui/components/tests\
	$(PROJECT_DIR)/data/tests\
	$(PROJECT_DIR)/utility/tests\
	$(PROJECT_DIR)/storage/tests\
	$(PROJECT_DIR)/features/tests\
	$(PROJECT_DIR)/features/tests/source\
	$(PROJECT_DIR)/models/tests\


MOCKS_SRC_DIRS := \
	$(PROJECT_DIR)/gui/mocks/source\
	$(PROJECT_DIR)/gui/components/mocks/source\
	$(PROJECT_DIR)/data/mocks/source\
	$(PROJECT_DIR)/utility/mocks/source\
	$(PROJECT_DIR)/storage/mocks/source\
	$(PROJECT_DIR)/features/mocks/source\
	$(PROJECT_DIR)/models/mocks/source\


CPPUTEST_USE_EXTENSIONS := Y
CPPUTEST_USE_GCOV := Y
CPPUTEST_USE_VPATH := Y
CPPUTEST_USE_STD_CPP_LIB := Y
CPPUTEST_WARNINGFLAGS += -fno-elide-type 
CPPUTEST_USE_MEM_LEAK_DETECTION := N
-include $(CPPUTEST_HOME)/build/MakefileWorker.mk
