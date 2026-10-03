#!/bin/bash
set -e

# Ensure we are in the project root
cd "$(dirname "$0")/.."

mkdir -p dist package/mkpass
TAG_NAME="${TAG_NAME:-${GITHUB_REF_NAME//\//-}}"
TAG_NAME="${TAG_NAME:-v0.1.0}"

# 1. Normalize and resolve CONDA_PREFIX
if [ -n "$CONDA_PREFIX" ]; then
  CONDA_PREFIX="${CONDA_PREFIX//\\//}"
fi

if [ -z "$CONDA_PREFIX" ] && [ -d ".pixi/envs/default" ]; then
  CONDA_PREFIX="$(pwd)/.pixi/envs/default"
fi

# Locate PREFIX_BIN and PREFIX_PLUGINS
PREFIX_BIN=""
PREFIX_PLUGINS=""
if [ -n "$CONDA_PREFIX" ]; then
  if [ -d "$CONDA_PREFIX/Library/bin" ]; then
    PREFIX_BIN="$CONDA_PREFIX/Library/bin"
    PREFIX_PLUGINS="$CONDA_PREFIX/Library/plugins"
  elif [ -d "$CONDA_PREFIX/bin" ]; then
    PREFIX_BIN="$CONDA_PREFIX/bin"
    PREFIX_PLUGINS="$CONDA_PREFIX/plugins"
  fi
fi

echo "Packaging Windows distribution:"
echo "  CONDA_PREFIX: $CONDA_PREFIX"
echo "  PREFIX_BIN: $PREFIX_BIN"
echo "  PREFIX_PLUGINS: $PREFIX_PLUGINS"

# 2. Copy application binaries
cp build/cli/mkpass.exe package/mkpass/
cp build/gui/mkpass-gui.exe package/mkpass/

# 3. Copy any runtime DLLs deployed to build/gui
if [ -d "build/gui" ]; then
  cp build/gui/*.dll package/mkpass/ 2>/dev/null || true
fi

# 4. Copy Qt and dependency DLLs from Conda prefix if available
if [ -n "$PREFIX_BIN" ] && [ -d "$PREFIX_BIN" ]; then
  cp "$PREFIX_BIN"/Qt5*.dll package/mkpass/ 2>/dev/null || true
  cp "$PREFIX_BIN"/icu*.dll package/mkpass/ 2>/dev/null || true
  cp "$PREFIX_BIN"/libpng*.dll "$PREFIX_BIN"/zlib*.dll "$PREFIX_BIN"/pcre2*.dll "$PREFIX_BIN"/zstd*.dll "$PREFIX_BIN"/freetype*.dll "$PREFIX_BIN"/harfbuzz*.dll "$PREFIX_BIN"/libbz2*.dll "$PREFIX_BIN"/bzip2*.dll package/mkpass/ 2>/dev/null || true
  cp "$PREFIX_BIN"/libjpeg*.dll "$PREFIX_BIN"/jpeg*.dll "$PREFIX_BIN"/libtiff*.dll "$PREFIX_BIN"/liblzma*.dll package/mkpass/ 2>/dev/null || true
  cp "$PREFIX_BIN"/sqlite*.dll package/mkpass/ 2>/dev/null || true
  cp "$PREFIX_BIN"/d3dcompiler*.dll "$PREFIX_BIN"/opengl32sw.dll "$PREFIX_BIN"/libEGL.dll "$PREFIX_BIN"/libGLESv2.dll package/mkpass/ 2>/dev/null || true
  cp "$PREFIX_BIN"/vcruntime*.dll "$PREFIX_BIN"/msvcp*.dll "$PREFIX_BIN"/vcomp*.dll package/mkpass/ 2>/dev/null || true
fi

# 5. Copy MSVC runtime from system if available
cp /c/Windows/System32/vcruntime140*.dll /c/Windows/System32/msvcp140*.dll package/mkpass/ 2>/dev/null || true

# 6. Copy Qt plugins from build/gui (deployed by windeployqt or cmake during build)
mkdir -p package/mkpass/plugins/platforms package/mkpass/platforms
for plugin_type in platforms imageformats iconengines styles; do
  if [ -d "build/gui/$plugin_type" ]; then
    echo "Copying $plugin_type from build/gui..."
    cp -r "build/gui/$plugin_type"/* package/mkpass/plugins/$plugin_type/ 2>/dev/null || true
    cp -r "build/gui/$plugin_type"/* package/mkpass/$plugin_type/ 2>/dev/null || true
  fi
done

# 7. Copy Qt plugins from PREFIX_PLUGINS
if [ -n "$PREFIX_PLUGINS" ] && [ -d "$PREFIX_PLUGINS" ]; then
  for plugin_type in platforms imageformats iconengines styles; do
    if [ -d "$PREFIX_PLUGINS/$plugin_type" ]; then
      echo "Copying $plugin_type from $PREFIX_PLUGINS..."
      mkdir -p "package/mkpass/plugins/$plugin_type" "package/mkpass/$plugin_type"
      cp "$PREFIX_PLUGINS/$plugin_type"/*.dll "package/mkpass/plugins/$plugin_type/" 2>/dev/null || true
      cp "$PREFIX_PLUGINS/$plugin_type"/*.dll "package/mkpass/$plugin_type/" 2>/dev/null || true
    fi
  done
fi

# 8. Fallback search for qwindows.dll if still missing
if [ ! -f "package/mkpass/plugins/platforms/qwindows.dll" ] && [ ! -f "package/mkpass/platforms/qwindows.dll" ]; then
  echo "Warning: qwindows.dll not yet in package, performing search..."
  FOUND_QWIN=""
  if [ -n "$CONDA_PREFIX" ] && [ -d "$CONDA_PREFIX" ]; then
    FOUND_QWIN=$(find "$CONDA_PREFIX" -name "qwindows.dll" 2>/dev/null | head -n 1)
  fi
  if [ -z "$FOUND_QWIN" ]; then
    FOUND_QWIN=$(find build -name "qwindows.dll" 2>/dev/null | head -n 1)
  fi
  if [ -n "$FOUND_QWIN" ]; then
    echo "Found qwindows.dll at: $FOUND_QWIN"
    mkdir -p package/mkpass/plugins/platforms package/mkpass/platforms
    cp "$FOUND_QWIN" package/mkpass/plugins/platforms/
    cp "$FOUND_QWIN" package/mkpass/platforms/
  fi
fi

# 9. Create qt.conf so Qt discovers its plugins relative to application directory
cat << 'EOF' > package/mkpass/qt.conf
[Paths]
Prefix = .
Plugins = plugins
EOF

# 10. Verification: Fail loudly if qwindows.dll is missing
echo "Verifying package contents:"
ls -la package/mkpass/
ls -la package/mkpass/platforms/ 2>/dev/null || true
ls -la package/mkpass/plugins/platforms/ 2>/dev/null || true

if [ ! -f "package/mkpass/plugins/platforms/qwindows.dll" ] && [ ! -f "package/mkpass/platforms/qwindows.dll" ]; then
  echo "FATAL ERROR: qwindows.dll was NOT found in package!" >&2
  exit 1
fi

echo "Verification passed: qwindows.dll is present."

# 11. Compress portable zip
cmake -E chdir package cmake -E tar cfv "../dist/mkpass-${TAG_NAME}-windows-x64.zip" --format=zip mkpass
echo "Windows portable zip created: dist/mkpass-${TAG_NAME}-windows-x64.zip"

VERSION="${TAG_NAME#v}"
VERSION="${VERSION:-0.1.0}"

# 12. Compile Inno Setup EXE Installer
echo ""
echo ">>> Checking for Inno Setup compiler..."
ISCC_BIN=""
if command -v iscc >/dev/null 2>&1; then
  ISCC_BIN="$(command -v iscc)"
elif command -v iscc.exe >/dev/null 2>&1; then
  ISCC_BIN="$(command -v iscc.exe)"
elif [ -f "/c/Program Files (x86)/Inno Setup 6/ISCC.exe" ]; then
  ISCC_BIN="/c/Program Files (x86)/Inno Setup 6/ISCC.exe"
elif [ -f "/c/Program Files/Inno Setup 6/ISCC.exe" ]; then
  ISCC_BIN="/c/Program Files/Inno Setup 6/ISCC.exe"
fi

if [ -n "$ISCC_BIN" ]; then
  echo "Compiling Inno Setup installer using $ISCC_BIN..."
  "$ISCC_BIN" "/DMyAppVersion=$VERSION" packaging/windows/mkpass.iss
  echo "Windows setup installer created: dist/mkpass-${VERSION}-windows-x64-setup.exe"
else
  echo "Inno Setup (iscc) not found; skipping Windows EXE installer."
fi

# 13. Compile WiX Toolset MSI Installer
echo ""
echo ">>> Checking for WiX Toolset..."
WIX_HEAT=""
WIX_CANDLE=""
WIX_LIGHT=""
if command -v heat >/dev/null 2>&1 && command -v candle >/dev/null 2>&1 && command -v light >/dev/null 2>&1; then
  WIX_HEAT="heat"
  WIX_CANDLE="candle"
  WIX_LIGHT="light"
elif [ -f "/c/Program Files (x86)/WiX Toolset v3.11/bin/candle.exe" ]; then
  WIX_HEAT="/c/Program Files (x86)/WiX Toolset v3.11/bin/heat.exe"
  WIX_CANDLE="/c/Program Files (x86)/WiX Toolset v3.11/bin/candle.exe"
  WIX_LIGHT="/c/Program Files (x86)/WiX Toolset v3.11/bin/light.exe"
elif [ -f "/c/Program Files (x86)/WiX Toolset v3.14/bin/candle.exe" ]; then
  WIX_HEAT="/c/Program Files (x86)/WiX Toolset v3.14/bin/heat.exe"
  WIX_CANDLE="/c/Program Files (x86)/WiX Toolset v3.14/bin/candle.exe"
  WIX_LIGHT="/c/Program Files (x86)/WiX Toolset v3.14/bin/light.exe"
fi

if [ -n "$WIX_CANDLE" ] && [ -n "$WIX_LIGHT" ] && [ -n "$WIX_HEAT" ]; then
  echo "Compiling WiX MSI installer..."
  "$WIX_HEAT" dir package/mkpass -cg AppFiles -dr INSTALLFOLDER -sfrag -srd -var var.SourceDir -out packaging/windows/files.wxs
  "$WIX_CANDLE" -dVersion="$VERSION" -dSourceDir="package/mkpass" -dProjectRoot="$(pwd)" -arch x64 packaging/windows/mkpass.wxs packaging/windows/files.wxs -out packaging/windows/
  "$WIX_LIGHT" -ext WixUIExtension packaging/windows/mkpass.wixobj packaging/windows/files.wixobj -out "dist/mkpass-${TAG_NAME}-windows-x64.msi"
  rm -f packaging/windows/*.wixobj packaging/windows/files.wxs packaging/windows/*.wixpdb
  echo "Windows MSI installer created: dist/mkpass-${TAG_NAME}-windows-x64.msi"
else
  echo "WiX Toolset (candle/light/heat) not found; skipping Windows MSI installer."
fi

echo ""
echo "=================================================="
echo " Windows Packaging Completed"
ls -lh dist/*.zip dist/*.exe dist/*.msi 2>/dev/null || true
echo "=================================================="
