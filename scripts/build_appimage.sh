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

download() {
    local url="$1"
    local output="$2"
    if [ ! -f "$output" ]; then
        if command -v wget >/dev/null 2>&1; then
            wget --tries=3 -c "$url" -O "$output"
        elif command -v curl >/dev/null 2>&1; then
            curl --retry 3 -L -C - "$url" -o "$output"
        else
            echo "Error: Neither wget nor curl is available" >&2
            exit 1
        fi
        chmod +x "$output"
    fi
}

download "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" "linuxdeploy-x86_64.AppImage"
download "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage" "linuxdeploy-plugin-qt-x86_64.AppImage"

cd ..

# Set up environment for linuxdeploy
if [ -z "$CONDA_PREFIX" ] && [ -d ".pixi/envs/default" ]; then
    CONDA_PREFIX="$(pwd)/.pixi/envs/default"
fi

if [ -n "$CONDA_PREFIX" ]; then
    if [ -f "$CONDA_PREFIX/bin/qmake" ]; then
        export QMAKE="$CONDA_PREFIX/bin/qmake"
    elif [ -f "$CONDA_PREFIX/bin/qmake6" ]; then
        export QMAKE="$CONDA_PREFIX/bin/qmake6"
    fi
    export LD_LIBRARY_PATH="$CONDA_PREFIX/lib:$LD_LIBRARY_PATH"
fi

export EXTRA_QT_PLUGINS="svg"

# Pre-populate extra platform and SVG plugins if available
mkdir -p AppDir/usr/plugins/platforms AppDir/usr/plugins/iconengines AppDir/usr/plugins/imageformats
if [ -n "$CONDA_PREFIX" ]; then
    QT_PLUGINS_DIR=""
    if [ -d "$CONDA_PREFIX/plugins" ]; then
        QT_PLUGINS_DIR="$CONDA_PREFIX/plugins"
    elif [ -d "$CONDA_PREFIX/lib/qt5/plugins" ]; then
        QT_PLUGINS_DIR="$CONDA_PREFIX/lib/qt5/plugins"
    elif [ -d "$CONDA_PREFIX/lib/qt6/plugins" ]; then
        QT_PLUGINS_DIR="$CONDA_PREFIX/lib/qt6/plugins"
    fi

    if [ -n "$QT_PLUGINS_DIR" ]; then
        if [ -f "$QT_PLUGINS_DIR/platforms/libqoffscreen.so" ]; then
            cp -f "$QT_PLUGINS_DIR/platforms/libqoffscreen.so" AppDir/usr/plugins/platforms/
        fi
        if [ -f "$QT_PLUGINS_DIR/platforms/libqwayland.so" ]; then
            cp -f "$QT_PLUGINS_DIR/platforms/libqwayland.so" AppDir/usr/plugins/platforms/
        fi
        if [ -f "$QT_PLUGINS_DIR/iconengines/libqsvgicon.so" ]; then
            cp -f "$QT_PLUGINS_DIR/iconengines/libqsvgicon.so" AppDir/usr/plugins/iconengines/
        fi
        if [ -f "$QT_PLUGINS_DIR/imageformats/libqsvg.so" ]; then
            cp -f "$QT_PLUGINS_DIR/imageformats/libqsvg.so" AppDir/usr/plugins/imageformats/
        fi
    fi
fi

# Run linuxdeploy
# --appimage-extract-and-run is used to avoid FUSE issues in some environments
./build_tools/linuxdeploy-x86_64.AppImage --appimage-extract-and-run \
    --appdir AppDir \
    --plugin qt \
    --output appimage \
    --desktop-file AppDir/usr/share/applications/mkpass.desktop \
    --icon-file AppDir/usr/share/icons/hicolor/256x256/apps/mkpass.png

echo "AppImage created successfully!"
