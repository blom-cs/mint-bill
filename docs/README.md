# Documentation

# Invoice Layout

# Statement Layout

# System Requirements

# Third Party APIs
This section covers where to find the different third party documentation that we use.       
As you develop refer to these areas of documentation for reference of how a method        
or function works. This section also includes code examples of how to use the methods      
and functions.           

## GTKMM-4
- GTKMM-4 API documentation: `https://gnome.pages.gitlab.gnome.org/gtkmm/index.html`    
- GTKMM-4 Code Examples: `https://gnome.pages.gitlab.gnome.org/gtkmm-documentation/index.html`         

## CURL
- libcurl API documentation: `https://curl.se/libcurl/c/`    

## CAIROMM-1.16
- Cairo Graphics API documentation: `https://www.cairographics.org/documentation/cairomm/reference/annotated.html`     

## POPPLER
- Poppler PDF AP docuemntation: `https://poppler.freedesktop.org/api/cpp/annotated.html`

## TURSO
The application links Turso's sqlite3-compatible C library (`turso_sqlite3`), not the libSQL C SDK.
- Source and `bindings/c`: `https://github.com/tursodatabase/turso`
- Vendored header: `third_party/turso/include/sqlite3.h`
- Build the static library with `cargo build -p turso_sqlite3 --release`. See the root README.


