# Models Module

The **models** module provides the business-logic layer responsible for loading,
saving, transforming, and serializing domain data used throughout the
application.  
It sits between the storage layer (plain Turso), the data structures
(`data::…` types), and the PDF/email generation features.

This module follows a clean separation-of-concerns design:

- **Model classes** handle business rules, validation, and orchestration of SQL operations.
- **Serialization classes** convert database rows into strongly typed `data::…` objects.
- **Interfaces** define contracts for both models and serializers.


## ✔️ Summary

The **models** directory forms the core business-logic layer of the entire system.  
It enforces a clean separation between:

- UI → Models  
- Models → Database  
- Database → Serialization  
- Serialization → Strongly-typed Data Structures  

All tests validate correctness, and the structure is fully modular and extensible for additional data models.

---

## Directory Structure

---
models/
├── CMakeLists.txt
├── include
│   ├── admin_model.h
│   ├── admin_serialize.h
│   ├── business_serialize.h
│   ├── client_model.h
│   ├── client_serialize.h
│   ├── invoice_model.h
│   ├── invoice_serialize.h
│   ├── models.h
│   ├── serialize.h
│   ├── statement_model.h
│   └── statement_serialize.h
├── mocks
│   ├── include
│   └── source
├── source
│   ├── admin_model.cpp
│   ├── admin_serialize.cpp
│   ├── business_serialize.cpp
│   ├── client_model.cpp
│   ├── client_serialize.cpp
│   ├── invoice_model.cpp
│   ├── invoice_serialize.cpp
│   ├── statement_model.cpp
│   └── statement_serialize.cpp
└── tests
    ├── admin_model_test.cpp
    ├── client_model_test.cpp
    ├── invoice_model_test.cpp
    ├── serialize_sql_data_test.cpp
    └── statement_model_test.cpp
---


---

## 📦 Components Overview

### **Model Interfaces**
Located in:
- `include/models.h`
- `include/serialize.h`

These provide abstraction layers ensuring:
- All models support load/save semantics.
- Serialization components uniformly convert SQL result sets → strongly-typed C++ data types.

---

## 🧩 Model Implementations (`source/*.cpp`)

### **admin_model / admin_serialize**
Responsible for:
- Loading and saving *business administrative information*.
- Synchronizing admin settings with the underlying Turso tables.
- Preparing admin data for use by downstream modules (PDF generation, UI).

### **client_model / client_serialize**
Handles:
- CRUD operations for clients.
- Resolving business details associated with each client.
- Interfacing with serialization logic for safe SQL conversion.

### **invoice_model / invoice_serialize**
Provides:
- Retrieval of invoices for a client or business.
- Safe transactional saving of invoice data.
- Conversion to printable formats (PDF generation).
- Population of line-items (materials, descriptions) via labor serialization.

### **statement_model / statement_serialize**
Responsible for:
- Loading and saving financial statements.
- Aggregating invoice data under a specific billing cycle.
- Producing statement-ready PDF structures.

### **business_serialize**
A supporting serializer used by admin/client/invoice systems to:
- Convert shared business fields from SQL.
- Assist with multi-table joins involving business_details.

---

## 🧪 Tests (`tests/*.cpp`)

The following test groups verify the correctness of all model + serialization logic:

- **admin_model_test.cpp**  
  Ensures admin data loads/saves correctly and validates error cases.

- **client_model_test.cpp**  
  Covers CRUD operations, schedule updates, and SQL integrity.

- **invoice_model_test.cpp**  
  Exercises invoice saving, loading, and construction of PDF invoice structures.

- **statement_model_test.cpp**  
  Verifies statement aggregation, SQL integration, and PDF conversions.

- **serialize_sql_data_test.cpp**  
  Ensures all serializers correctly:
  - Interpret SQL rows
  - Convert them into strongly-typed structures
  - Handle empty/malformed result sets safely

---

## 🧪 Mocks Directory

The `mocks/` folder contains empty scaffolding for future model or serialization mocks used for isolated unit testing.  
This allows the model layer to be tested independently from external storage or GUI components.

---

## ⚙️ CMake Integration

`CMakeLists.txt` builds this module as an **OBJECT library**:

- Makes the compiled objects linkable across all higher-level targets.
- Ensures consistent include paths for:
  - data/
  - storage/
  - utility/
  - features/
  - GUI and app modules

---
