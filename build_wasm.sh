#!/bin/bash
set -e

# mkpass WebAssembly Build Script
# Get the root directory of the project (where this script is located)
ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$ROOT_DIR/build_wasm"
WEB_PUBLIC_DIR="$ROOT_DIR/mkpass_web/public"
WASM_SOURCE_DIR="$ROOT_DIR/mkpass_webasm"

echo "Creating build directory: $BUILD_DIR"
mkdir -p "$BUILD_DIR"

cd "$BUILD_DIR"

echo "Running emcmake..."
emcmake cmake "$WASM_SOURCE_DIR"

echo "Building with emmake..."
emmake make

echo "Copying artifacts to $WEB_PUBLIC_DIR..."
cp mkpass_webasm.js mkpass_webasm.wasm "$WEB_PUBLIC_DIR/"

echo "WASM module built and copied successfully!"
