#!/bin/bash
set -e

# Ensure we are in the project root
cd "$(dirname "$0")/.."
PROJECT_ROOT="$(pwd)"

mkdir -p dist package/mkpass

# Resolve default version from CMakeLists.txt
DEFAULT_VERSION=$(grep -m1 -E 'project\s*\([^)]*VERSION' "$PROJECT_ROOT/CMakeLists.txt" 2>/dev/null | sed -E 's/.*VERSION\s+([0-9.]+).*/\1/' || true)
DEFAULT_VERSION="${DEFAULT_VERSION:-0.1.0}"

# Resolve release tag and version
TAG_NAME="${TAG_NAME:-${GITHUB_REF_NAME:-v$DEFAULT_VERSION}}"
TAG_NAME="${TAG_NAME#refs/tags/}"
TAG_NAME="${TAG_NAME#refs/heads/}"
TAG_NAME="${TAG_NAME//\//-}"

# Extract version starting with digit if available, else fallback to CMakeLists version
EXTRACTED_VER=$(echo "$TAG_NAME" | grep -oE '[0-9]+\.[0-9]+(\.[0-9]+)?' | head -n1 || true)
VERSION="${VERSION:-$EXTRACTED_VER}"
VERSION="${VERSION:-$DEFAULT_VERSION}"
if [[ ! "$VERSION" =~ ^[0-9] ]]; then
  VERSION="$DEFAULT_VERSION"
fi
VERSION="${VERSION%%-*}"

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

  if command -v cygpath >/dev/null 2>&1; then
    ISS_FILE_WIN="$(cygpath -w "$PROJECT_ROOT/packaging/windows/mkpass.iss")"
  else
    ISS_FILE_WIN="$PROJECT_ROOT\\packaging\\windows\\mkpass.iss"
  fi

  MSYS2_ARG_CONV_EXCL="*" MSYS_NO_PATHCONV=1 "$ISCC_BIN" \
    -DMyAppVersion="$VERSION" \
    -DOutputBaseFilename="mkpass-${TAG_NAME}-windows-x64-setup" \
    "$ISS_FILE_WIN"

  echo "Windows setup installer created: dist/mkpass-${TAG_NAME}-windows-x64-setup.exe"
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

  if command -v cygpath >/dev/null 2>&1; then
    SOURCE_DIR_WIN="$(cygpath -w "$PROJECT_ROOT/package/mkpass")"
    ICON_PATH_WIN="$(cygpath -w "$PROJECT_ROOT/icons/mkpass.ico")"
    LICENSE_RTF_WIN="$(cygpath -w "$PROJECT_ROOT/packaging/windows/License.rtf")"
    WXS_MKPASS_WIN="$(cygpath -w "$PROJECT_ROOT/packaging/windows/mkpass.wxs")"
    WXS_FILES_WIN="$(cygpath -w "$PROJECT_ROOT/packaging/windows/files.wxs")"
    OUT_DIR_WIN="$(cygpath -w "$PROJECT_ROOT/packaging/windows/")\\"
    MSI_OUT_WIN="$(cygpath -w "$PROJECT_ROOT/dist/mkpass-${TAG_NAME}-windows-x64.msi")"
    OBJ_MKPASS_WIN="$(cygpath -w "$PROJECT_ROOT/packaging/windows/mkpass.wixobj")"
    OBJ_FILES_WIN="$(cygpath -w "$PROJECT_ROOT/packaging/windows/files.wixobj")"
  else
    SOURCE_DIR_WIN="$PROJECT_ROOT\\package\\mkpass"
    ICON_PATH_WIN="$PROJECT_ROOT\\icons\\mkpass.ico"
    LICENSE_RTF_WIN="$PROJECT_ROOT\\packaging\\windows\\License.rtf"
    WXS_MKPASS_WIN="$PROJECT_ROOT\\packaging\\windows\\mkpass.wxs"
    WXS_FILES_WIN="$PROJECT_ROOT\\packaging\\windows\\files.wxs"
    OUT_DIR_WIN="$PROJECT_ROOT\\packaging\\windows\\"
    MSI_OUT_WIN="$PROJECT_ROOT\\dist\\mkpass-${TAG_NAME}-windows-x64.msi"
    OBJ_MKPASS_WIN="$PROJECT_ROOT\\packaging\\windows\\mkpass.wixobj"
    OBJ_FILES_WIN="$PROJECT_ROOT\\packaging\\windows\\files.wixobj"
  fi

  # Remove empty directories to avoid empty directory components
  find package/mkpass -type d -empty -delete 2>/dev/null || true

  MSYS2_ARG_CONV_EXCL="*" MSYS_NO_PATHCONV=1 "$WIX_HEAT" dir "$SOURCE_DIR_WIN" \
    -cg AppFiles -dr INSTALLFOLDER -scom -sreg -sfrag -srd -ag -sw5150 \
    -var var.SourceDir -out "$WXS_FILES_WIN"

  # Replace any remaining PUT-GUID-HERE with valid GUIDs
  python3 -c "import re, uuid, os; p='${PROJECT_ROOT}/packaging/windows/files.wxs'; f=open(p,'r',encoding='utf-8',errors='ignore'); c=f.read(); f.close(); open(p,'w',encoding='utf-8').write(re.sub(r'PUT-GUID-HERE', lambda m: str(uuid.uuid4()).upper(), c))" 2>/dev/null || \
  python -c "import re, uuid, os; p='${PROJECT_ROOT}/packaging/windows/files.wxs'; f=open(p,'r',encoding='utf-8',errors='ignore'); c=f.read(); f.close(); open(p,'w',encoding='utf-8').write(re.sub(r'PUT-GUID-HERE', lambda m: str(uuid.uuid4()).upper(), c))" 2>/dev/null || \
  sed -i 's/PUT-GUID-HERE/*/g' "$PROJECT_ROOT/packaging/windows/files.wxs" 2>/dev/null || true

  MSYS2_ARG_CONV_EXCL="*" MSYS_NO_PATHCONV=1 "$WIX_CANDLE" \
    -dVersion="$VERSION" -dSourceDir="$SOURCE_DIR_WIN" -dIconPath="$ICON_PATH_WIN" -dLicenseRtf="$LICENSE_RTF_WIN" \
    -arch x64 "$WXS_MKPASS_WIN" "$WXS_FILES_WIN" -out "$OUT_DIR_WIN"

  MSYS2_ARG_CONV_EXCL="*" MSYS_NO_PATHCONV=1 "$WIX_LIGHT" \
    -sval -ext WixUIExtension "$OBJ_MKPASS_WIN" "$OBJ_FILES_WIN" -out "$MSI_OUT_WIN"

  rm -f packaging/windows/*.wixobj packaging/windows/files.wxs packaging/windows/*.wixpdb
  echo "Windows MSI installer created: dist/mkpass-${TAG_NAME}-windows-x64.msi"
else
  echo "WiX Toolset (candle/light/heat) not found; skipping Windows MSI installer."
fi

echo ""
echo "=================================================="
echo " Windows Packaging Completed"
shopt -s nullglob
PACKAGES=(dist/*.zip dist/*.exe dist/*.msi)
shopt -u nullglob
if [ ${#PACKAGES[@]} -gt 0 ]; then
  ls -lh "${PACKAGES[@]}"
else
  echo "No packages generated."
fi
echo "=================================================="
