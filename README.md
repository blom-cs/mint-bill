# MintBill
MintBill is an invoice system for two small businesses, T.M Engineering and T.C Engineering.      
It's domain-specific and highly related to the two firms and their needs. The invoice system            
covers business registration, new client registration, statement generation, and invoice               
creation based on work done. The user can create, store, print, and email invoices and 
statements for specific clients. 


## Documentation
All documents related to the requriements, design, analysis, and risk assessment of the                  
system should be stored in the docs directory under their respective child directories.                      


## Project Tool
The project tool used to set up the development for the developer is called `mint-bill.sh`,
located in the **scripts** directory found in the project base directory.
- For a description of the tool and its capabilities, run `./scripts/mint-bill.sh -h`.


## Getting Ready For Development
Follow the below steps to set up your system for development for this project.
**NOTE: Currently, the project supports development on Fedora only**
- To install all required packages for code development: `./scripts/mint-bill.sh -i develop`
- To install all required tools for GUI design: `./scripts/mint-bill.sh -i design`
- To install all required tools for analysis: `./scripts/mint-bill.sh -i analysis`

The database library is Turso's sqlite3-compatible C API, crate `turso_sqlite3`. Fedora does not package it, and `install.sh` does not install it. Build the static library with Rust and leave it where CMake expects it:

```
git clone https://github.com/tursodatabase/turso.git /tmp/turso-src
cd /tmp/turso-src && cargo build -p turso_sqlite3 --release
```

CMake uses `/tmp/turso-src/target/release/libturso_sqlite3.a`. Pass `-DTURSO_LIBRARY=` to use another path. The header is already vendored at `third_party/turso/include/sqlite3.h`. The Turso command-line tool is not required. This is not the libSQL C SDK (`libsql.h`). The database file is a Turso file.


## Deployment
Production is two offline Fedora RPM payloads unpacked into the user's home
directory (no root, no `/usr`).

- Build the bundle: `./scripts/mint-bill.sh --build rpm`
- Version: `PROJECT_VERSION_NUM` in `scripts/build.sh`. The RPM, the CMake project, and the bundle use that value.
- Directory: `build/rpm/mint-bill-v<version>/` (both RPMs, runtime dependency RPMs, and `install.sh`)
- Archive: `build/rpm/mint-bill-v<version>.tar.gz`

On the office PC, unpack the archive and run `./install.sh` to print the usage help. `./install.sh --install` stops the old user services, removes the old program files, installs `mint-bill-tme` and `mint-bill-tce`, and starts both programs. A live Turso database is left in place. `./install.sh --uninstall` removes the program files, user services, data directories, and backup directories.

TME and TCE are separate processes, data directories, databases, and systemd user services.


## Rules For Development
### Branching Strategy
The branching strategy is simple. It consists of three levels known as production                
(e.g. main branch), integration (e.g. develop branch), and development               
(e.g. feature/refactor branches).

Essentially, the code in the main branch is ready for deployment, the code in the                
develop branch will need to go through integration tests before it can be merged              
to the main branch for deployment, and the code in the feature/refactor branches       
are under development.

A typical flow would be a branch of off-develop and create a feature/branch-name branch.                   
Develop your feature and merge it into the develop branch for integration testing.                  
If you want to refactor a piece of code that already exists, branch off the develop              
branch and create a refactor/branch-name branch. After the refactoring is complete, merge              
it to the develop branch and run the integration tests again to verify nothing broke.

### Branch Naming
The branches **main** and **develop** are by default already created. Therefore, the only               
branches you need to create will be feature or refactor branches. Here are the rules for               
creating such branches:                
- All branch names shall be lower case seperated by hyphens. (e.g. feature/test-branch)
- All branches shall have meaningful names that describe the purpose of the branch.                        
  (e.g. feature/invoice-processing, feature/statement-verification)

# App Password
The application requires that you create a gmail app password for the email services.    
This password will not be shared after you have entered into the application.  
The create a App password with gmail. Ensure that two-factor authentication is turned   
on in your gmail account. After that, navigate to the following link:   
- GMail documentation: https://support.google.com/mail/answer/185833?hl=en  
- Click on **Create and manage your app passwords**.  
- Enter the password for your application.  
- Enter the password into this application when registering a business.  
