#!/bin/bash
set -e

# Ensure we are in the project root
cd "$(dirname "$0")/.."

# Build the project
cmake --build build --config Release

# Create/Clear AppDir
rm -rf AppDir
mkdir -p AppDir

# Install to AppDir
cmake --install build --prefix AppDir/usr

# Download linuxdeploy and plugins if not present
DOWNLOAD_DIR="build_tools"
mkdir -p "$DOWNLOAD_DIR"
cd "$DOWNLOAD_DIR"

if [ ! -f linuxdeploy-x86_64.AppImage ]; then
    wget -c https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
    chmod +x linuxdeploy-x86_64.AppImage
fi

if [ ! -f linuxdeploy-plugin-qt-x86_64.AppImage ]; then
    wget -c https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
    chmod +x linuxdeploy-plugin-qt-x86_64.AppImage
fi

cd ..

# Set up environment for linuxdeploy
export QMAKE="$CONDA_PREFIX/bin/qmake"
export LD_LIBRARY_PATH="$CONDA_PREFIX/lib:$LD_LIBRARY_PATH"
export EXTRA_QT_PLUGINS="svg" # If needed

# Run linuxdeploy
# --appimage-extract-and-run is used to avoid FUSE issues in some environments
./build_tools/linuxdeploy-x86_64.AppImage --appimage-extract-and-run \
    --appdir AppDir \
    --plugin qt \
    --output appimage \
    --desktop-file AppDir/usr/share/applications/mkpass.desktop \
    --icon-file AppDir/usr/share/icons/hicolor/256x256/apps/mkpass.png

echo "AppImage created successfully!"
