# Features Module  
**High-Level Services: Email and PDF Generation**

---

## Summary

The `features/` module provides higher-level application services that sit above the data models and below the GUI.  
These features encapsulate reusable business logic, external integrations, and complex workflows such as:

- **PDF generation** (Invoices & Statements)  
- **SMTP email delivery** (with attachments & multi-part messages)

Each feature is implemented as a standalone component with a clean public interface and minimal external coupling.  
This design allows the GUI to trigger functionality without knowing how PDF rendering, MIME assembly, or SMTP communication actually works.

---

## Directory Overview
---
features
├── CMakeLists.txt
├── include
│   ├── app_features.h
│   ├── email.h
│   ├── invoice_pdf.h
│   └── statement_pdf.h
├── mocks
│   ├── include
│   └── source
├── README.md
├── source
│   ├── email.cpp
│   ├── invoice_pdf.cpp
│   └── statement_pdf.cpp
└── tests
    ├── email_test.cpp
    ├── generate_invoice_pdf_test.cpp
    ├── generate_statement_pdf_test.cpp
    ├── include
    │   └── generate_pdf.h
    └── source
        └── generate_pdf.cpp
---


---

# Components

---

## 1. Email Service (`feature::email`)
Provides a complete SMTP email-sending implementation.

### Responsibilities
- Validates email metadata (`data::email`)
- Adds TO / CC / FROM fields
- Builds MIME messages containing:
  - Plain-text body
  - HTML body
  - PDF attachments  
- Uses `libcurl` for:
  - SMTP authentication  
  - MIME construction (curl_mime)  
  - Email transmission  

### Internal Helper Classes
- `smtp::client` → Auth, SSL, SMTP URL, MAIL FROM  
- `smtp::header` → To, From, CC, Subject header injection  
- `smtp::recipients` → Recipients list for MAIL RCPT  
- `smtp::parts` → MIME bodies + file attachments

---

## 2. Invoice PDF Generator (`feature::invoice_pdf`)
Creates professional-layout invoice PDFs using **Cairo**.

### Responsibilities
- Writes invoice header (“Invoice”)
- Renders business + client information
- Draws invoice metadata:  
  - Invoice number  
  - Date  
  - Order number  
  - Job card  
- Formats labor + material tables  
- Computes totals and renders “Grand Total”
- Adds payment method section
- Handles long descriptions using boundary slicing
- Includes automatic page-wrapping logic  

### Output
Returns a **binary PDF string**, suitable for:
- Writing to disk  
- Attaching to an email  
- Storing in a database  

---

## 3. Statement PDF Generator (`feature::statement_pdf`)
Generates multi-invoice financial statements.

### Responsibilities
- Writes “Statement” header
- Shows client + business information  
- Renders:
  - Statement number  
  - Statement date  
  - Date range (period start → end)  
- Prints a table of invoices:
  - Invoice ID  
  - Date  
  - Paid status  
  - Order number  
  - Amount  
- Computes and renders total owing  
- Adds payment method section  
- Supports slicing of long order-number strings
- Page-wrapping for large lists of invoices  

Also returns a **binary PDF string**.

---

# Architectural Purpose

The `features/` module:

- Acts as the **middleware layer** between GUI and data  
- Implements all real-world side effects:
  - PDF rendering  
  - Email transmission  
- Keeps GUI code clean, so the GUI only orchestrates high-level actions  
- Ensures that business logic is testable and decoupled  

### Example Usage

```cpp
feature::invoice_pdf pdf;
std::string bytes = pdf.generate(pdf_data);

feature::email email;
email.send(email_data);
```

