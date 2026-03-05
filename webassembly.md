# mkpass WebAssembly Version

This document describes the architecture, implementation, and build process for the WebAssembly (WASM) version of `mkpass`.

## Overview

The WebAssembly version of `mkpass` allows the core C++ password generation logic to run directly in a web browser. It uses the same high-security algorithms (Argon2, SHA512) as the CLI and GUI versions, providing a consistent and secure experience across platforms.

## Architecture

The solution consists of three main components:

1.  **`libmkpass` (Core C++ Library):** The shared C++ logic responsible for hashing and password composition. It is compiled as a static library.
2.  **`mkpass_webasm` (WASM Wrapper):** A C++ layer using [Embind](https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html) to expose the C++ functions and structures to JavaScript.
3.  **`mkpass_web` (React Frontend):** A React-based web application that loads the WASM module and provides a user interface for password generation.

### Module Diagram

```
[ Browser / JavaScript ] <--> [ Embind Wrapper ] <--> [ libmkpass (C++) ]
      (React UI)             (mkpass_webasm.cpp)       (Argon2, SHA512)
```

## Modules and Bindings

### Exposed Types

The following types and functions are exported from C++ to JavaScript via `EMSCRIPTEN_BINDINGS`:

*   **Enums:** `Algorithm` (Argon2, SlowSha512, Old) and `CharacterClass` (LOWERCASE, UPPERCASE, etc.).
*   **Structures:** `Context` (mapped as a `value_object` in JS) containing:
    *   `password`, `service`, `char_classes`, `algorithm`, `length`, and `custom_chars`.
*   **Containers:** `VectorCharacterClass` (a `std::vector<CharacterClass>` wrapper) for passing the list of desired character sets.
*   **Functions:** `MkPass(Context ctx)` returns the generated password as a string.

## Interactions (JS <-> C++)

1.  **Loading:** The WASM module is compiled with `-sMODULARIZE=1` and `-sEXPORT_NAME='mkpass_wasm'`. This means it is loaded as a function that returns a Promise resolving to the module instance.
2.  **Memory Management:** Since Argon2 requires significant memory (default 64MiB), the module is configured with `-sALLOW_MEMORY_GROWTH=1` and an initial memory of 128MiB.
3.  **Data Transfer:**
    *   JavaScript objects are automatically converted to C++ `Context` structs by Embind.
    *   `VectorCharacterClass` must be explicitly instantiated in JS and manually deleted (`.delete()`) to prevent memory leaks in the WASM heap.

## Build Instructions

### Prerequisites

*   [Emscripten SDK (emsdk)](https://emscripten.org/docs/getting_started/downloads.html) installed and available in your `PATH`.
*   CMake 3.13 or higher.
*   Node.js and npm.

### 1. Build the WASM Module

```bash
# Create a build directory
mkdir build_wasm
cd build_wasm

# Configure with emcmake
emcmake cmake ../mkpass_webasm

# Build with emmake
emmake make
```

This generates `mkpass_webasm.js` and `mkpass_webasm.wasm`.

### 2. Integrate with React

Copy the generated files to the React `public` folder:

```bash
cp mkpass_webasm.js ../mkpass_web/public/
cp mkpass_webasm.wasm ../mkpass_web/public/
```

### 3. Build/Run the Web App

```bash
cd ../mkpass_web
npm install
npm start # For development
# OR
npm run build # For production
```

## Technical Details & Constraints

*   **No Threads:** The WASM version is compiled with `ARGON2_NO_THREADS` to avoid the complexity of SharedArrayBuffer and cross-origin isolation requirements in browsers.
*   **Static Linking:** All dependencies (Argon2, SHA512, SQLite3, etc.) are statically linked into the final WASM binary.
*   **Security:** Password generation happens entirely client-side. The master password never leaves the browser.
