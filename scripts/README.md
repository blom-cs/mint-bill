# Scripts Directory

The `scripts/` directory contains the automation layer for the Mint-Bill project.  
These scripts provide a unified interface for building, testing, installing, and analyzing the system across various environments.  
Each script is modular, allowing internal reuse and clean separation of responsibilities.

---

## Directory Structure

```
scripts/
├── build.sh
├── install.sh
├── mint-bill.sh
├── package-rpm.sh
├── README.md
├── statistics.sh
└── test.sh
```

---

## Script Overview

### **build.sh**
Handles all CMake-based project builds. Supports:
- Host builds for local development (source-tree UI/CSS/DB paths)
- RPM / deploy builds for offline Fedora packaging

Called indirectly by `mint-bill.sh` or directly from command line.

### **package-rpm.sh**
Creates a Fedora RPM for tenant `tme` or `tce`, then packs `build/rpm/mint-bill-v<version>/`. The version is `PROJECT_VERSION_NUM` in `scripts/build.sh`.
with those RPMs, the runtime dependency RPMs, and `install.sh`. The office PC
runs that `install.sh` with no internet. `./install.sh` with no arguments prints usage help. `./install.sh --install` removes the previous program files and services, installs the new user-local application, and starts both programs. A live Turso database is left in place. `./install.sh --uninstall` removes the program files, user services, data directories, and backup directories. The live database is not inside the RPM.

---

### **install.sh**
Installs system-level dependencies required for development, GUI design, and system analysis.

Responsibilities include:
- Installing compilers, GTKmm, Poppler, and other dev tooling
- The Turso static library is not a dnf package. Build it as described in the root README.
- Installing CppUTest from source if not already present
- Installing Flatpak-based design tools (Cambalache)
- Installing analysis tools (Gaphor)

---

### **mint-bill.sh**
Main entrypoint script that orchestrates build, test, statistics, and installation operations.

Provides:
- Argument parsing (`--build`, `--test`, `--install`, `--stats`)
- Environment validation
- User-friendly help system
- Unified command interface for the whole project

This is the script developers will run most frequently.

---

### **statistics.sh**
Provides project metrics, currently:
- Line-of-code (LOC) reporting via `cloc`.

Additional statistical operations can be added here over time.

---

### **test.sh**
Runs all testing and analysis operations, including:
- Static code analysis (Cppcheck)
- Unit testing (CppUTest)
- Code coverage generation (gcov, gcovr)
- Launching coverage reports in the browser
- Virtual X framebuffer (Xvfb) use for headless testing

Also includes coverage threshold enforcement.

---

## Usage Summary

All scripts are designed to be executed through the master script:

```
./scripts/mint-bill.sh --help
```

Examples:

```
# Build for development
./scripts/mint-bill.sh --build host

# Build an offline RPM (default tenant: tme)
./scripts/mint-bill.sh --build rpm
TENANT=tce ./scripts/mint-bill.sh --build rpm

# Run unit tests
./scripts/mint-bill.sh --test unit

# Perform static analysis
./scripts/mint-bill.sh --test sca

# Install development toolchain
./scripts/mint-bill.sh --install develop

# Get project line count
./scripts/mint-bill.sh --stats lc
```

---

## Notes

- All scripts assume a Linux environment with bash available.
- Scripts are written to fail fast and provide clear error feedback.
- The system relies on consistent directory layout; calling scripts from the wrong location will trigger warnings.

---

## Maintenance

To extend the system:
- Add new build profiles in `build.sh`
- Add new test modes in `test.sh`
- Add new statistical tools in `statistics.sh`
- Modify or extend installation targets in `install.sh`

Each script is internally modular to support future growth.
