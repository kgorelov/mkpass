#!/bin/bash
set -e

# Ensure we are in the project root
cd "$(dirname "$0")/.."

mkdir -p dist package/mkpass
TAG_NAME="${TAG_NAME:-${GITHUB_REF_NAME//\//-}}"
TAG_NAME="${TAG_NAME:-v0.1.0}"

# Copy binaries
cp build/cli/mkpass.exe package/mkpass/
cp build/gui/mkpass-gui.exe package/mkpass/

# Copy Qt runtime DLLs and plugins from Conda prefix
if [ -d "$CONDA_PREFIX/Library/bin" ]; then
  PREFIX_BIN="$CONDA_PREFIX/Library/bin"
  PREFIX_PLUGINS="$CONDA_PREFIX/Library/plugins"
else
  PREFIX_BIN="$CONDA_PREFIX/bin"
  PREFIX_PLUGINS="$CONDA_PREFIX/plugins"
fi

cp "$PREFIX_BIN"/Qt5Core.dll package/mkpass/ 2>/dev/null || true
cp "$PREFIX_BIN"/Qt5Gui.dll package/mkpass/ 2>/dev/null || true
cp "$PREFIX_BIN"/Qt5Widgets.dll package/mkpass/ 2>/dev/null || true
cp "$PREFIX_BIN"/Qt5Concurrent.dll package/mkpass/ 2>/dev/null || true
cp "$PREFIX_BIN"/Qt5Svg.dll package/mkpass/ 2>/dev/null || true

# Copy any additional runtime DLLs copied during build
cp build/gui/*.dll package/mkpass/ 2>/dev/null || true

# Platform plugins
mkdir -p package/mkpass/platforms
cp "$PREFIX_PLUGINS"/platforms/qwindows.dll package/mkpass/platforms/ 2>/dev/null || true

# Icon and image format plugins
if [ -d "$PREFIX_PLUGINS/iconengines" ]; then
  mkdir -p package/mkpass/iconengines
  cp "$PREFIX_PLUGINS"/iconengines/*.dll package/mkpass/iconengines/ 2>/dev/null || true
fi
if [ -d "$PREFIX_PLUGINS/imageformats" ]; then
  mkdir -p package/mkpass/imageformats
  cp "$PREFIX_PLUGINS"/imageformats/*.dll package/mkpass/imageformats/ 2>/dev/null || true
fi

# Compress portable zip
cmake -E chdir package cmake -E tar cfv "../dist/mkpass-${TAG_NAME}-windows-x64.zip" --format=zip mkpass

echo "Windows package created: dist/mkpass-${TAG_NAME}-windows-x64.zip"
