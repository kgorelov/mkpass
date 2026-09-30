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

# 1. Qt Core, GUI, Widgets, SVG, Concurrent libraries
cp "$PREFIX_BIN"/Qt5*.dll package/mkpass/ 2>/dev/null || true

# 2. ICU libraries (required by Qt5Core and platform plugins)
cp "$PREFIX_BIN"/icu*.dll package/mkpass/ 2>/dev/null || true

# 3. Image, font, and compression dependencies
cp "$PREFIX_BIN"/libpng*.dll "$PREFIX_BIN"/zlib*.dll "$PREFIX_BIN"/pcre2*.dll "$PREFIX_BIN"/zstd*.dll "$PREFIX_BIN"/freetype*.dll "$PREFIX_BIN"/harfbuzz*.dll "$PREFIX_BIN"/libbz2*.dll "$PREFIX_BIN"/bzip2*.dll package/mkpass/ 2>/dev/null || true
cp "$PREFIX_BIN"/libjpeg*.dll "$PREFIX_BIN"/jpeg*.dll "$PREFIX_BIN"/libtiff*.dll "$PREFIX_BIN"/liblzma*.dll package/mkpass/ 2>/dev/null || true
cp "$PREFIX_BIN"/sqlite*.dll package/mkpass/ 2>/dev/null || true

# 4. MSVC runtime libraries
cp "$PREFIX_BIN"/vcruntime*.dll "$PREFIX_BIN"/msvcp*.dll "$PREFIX_BIN"/vcomp*.dll package/mkpass/ 2>/dev/null || true
if [ -d "$CONDA_PREFIX/bin" ]; then
  cp "$CONDA_PREFIX"/bin/vcruntime*.dll "$CONDA_PREFIX"/bin/msvcp*.dll package/mkpass/ 2>/dev/null || true
fi
cp /c/Windows/System32/vcruntime140*.dll /c/Windows/System32/msvcp140*.dll package/mkpass/ 2>/dev/null || true

# 5. Copy any additional runtime DLLs copied during build
cp build/gui/*.dll package/mkpass/ 2>/dev/null || true

# 6. Create qt.conf so Qt discovers its plugins reliably
cat << 'EOF' > package/mkpass/qt.conf
[Paths]
Prefix = .
Plugins = plugins
EOF

# 7. Platform plugins (deploy to both plugins/platforms and platforms for compatibility)
mkdir -p package/mkpass/plugins/platforms package/mkpass/platforms
cp "$PREFIX_PLUGINS"/platforms/*.dll package/mkpass/plugins/platforms/ 2>/dev/null || true
cp "$PREFIX_PLUGINS"/platforms/*.dll package/mkpass/platforms/ 2>/dev/null || true

# 8. Icon and image format plugins
if [ -d "$PREFIX_PLUGINS/iconengines" ]; then
  mkdir -p package/mkpass/plugins/iconengines package/mkpass/iconengines
  cp "$PREFIX_PLUGINS"/iconengines/*.dll package/mkpass/plugins/iconengines/ 2>/dev/null || true
  cp "$PREFIX_PLUGINS"/iconengines/*.dll package/mkpass/iconengines/ 2>/dev/null || true
fi
if [ -d "$PREFIX_PLUGINS/imageformats" ]; then
  mkdir -p package/mkpass/plugins/imageformats package/mkpass/imageformats
  cp "$PREFIX_PLUGINS"/imageformats/*.dll package/mkpass/plugins/imageformats/ 2>/dev/null || true
  cp "$PREFIX_PLUGINS"/imageformats/*.dll package/mkpass/imageformats/ 2>/dev/null || true
fi
if [ -d "$PREFIX_PLUGINS/styles" ]; then
  mkdir -p package/mkpass/plugins/styles package/mkpass/styles
  cp "$PREFIX_PLUGINS"/styles/*.dll package/mkpass/plugins/styles/ 2>/dev/null || true
  cp "$PREFIX_PLUGINS"/styles/*.dll package/mkpass/styles/ 2>/dev/null || true
fi

# Compress portable zip
cmake -E chdir package cmake -E tar cfv "../dist/mkpass-${TAG_NAME}-windows-x64.zip" --format=zip mkpass

echo "Windows package created: dist/mkpass-${TAG_NAME}-windows-x64.zip"
