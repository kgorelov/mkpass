#!/bin/bash
set -e

# Ensure execution from project root
cd "$(dirname "$0")/.."
PROJECT_ROOT="$(pwd)"

echo "=================================================="
echo " mkpass Distribution Package Verification Suite"
echo "=================================================="

FAILED=0

# Helper assertion
assert_file() {
    if [ ! -f "$1" ]; then
        echo "FAIL: Expected file not found: $1"
        FAILED=1
    else
        echo "OK: Found file: $1"
    fi
}

# -----------------------------------------------------------------------------
# 1. Verify Documentation and Man Pages Syntax
# -----------------------------------------------------------------------------
echo ""
echo ">>> 1. Verifying Man Pages and Documentation Syntax..."

if command -v man >/dev/null 2>&1; then
    if man -l docs/man/mkpass.1 >/dev/null 2>&1; then
        echo "OK: docs/man/mkpass.1 passes man page syntax check"
    else
        echo "FAIL: docs/man/mkpass.1 failed man page syntax check"
        FAILED=1
    fi

    if man -l docs/man/mkpass-gui.1 >/dev/null 2>&1; then
        echo "OK: docs/man/mkpass-gui.1 passes man page syntax check"
    else
        echo "FAIL: docs/man/mkpass-gui.1 failed man page syntax check"
        FAILED=1
    fi
else
    echo "Notice: 'man' utility not installed; skipping groff formatting check."
fi

assert_file "docs/qt_help.html"
assert_file "README.md"

# -----------------------------------------------------------------------------
# 2. Verify Debian Packages (.deb)
# -----------------------------------------------------------------------------
echo ""
echo ">>> 2. Verifying Debian Packages in dist/..."

DEB_CLI=$(find dist -maxdepth 1 -name "mkpass_*.deb" ! -name "mkpass-gui*" | head -n1 || true)
DEB_GUI=$(find dist -maxdepth 1 -name "mkpass-gui_*.deb" | head -n1 || true)

if [ -z "$DEB_CLI" ] || [ -z "$DEB_GUI" ]; then
    echo "Notice: Debian packages not found in dist/. Running scripts/package_linux.sh..."
    ./scripts/package_linux.sh
    DEB_CLI=$(find dist -maxdepth 1 -name "mkpass_*.deb" ! -name "mkpass-gui*" | head -n1 || true)
    DEB_GUI=$(find dist -maxdepth 1 -name "mkpass-gui_*.deb" | head -n1 || true)
fi

TMP_CLI_DIR=$(mktemp -d /tmp/verify_mkpass_cli_XXXXXX)
TMP_GUI_DIR=$(mktemp -d /tmp/verify_mkpass_gui_XXXXXX)
trap 'rm -rf "$TMP_CLI_DIR" "$TMP_GUI_DIR"' EXIT

if [ -n "$DEB_CLI" ] && [ -f "$DEB_CLI" ]; then
    echo "Verifying CLI Debian package: $(basename "$DEB_CLI")"
    dpkg-deb -x "$DEB_CLI" "$TMP_CLI_DIR"
    dpkg-deb -e "$DEB_CLI" "$TMP_CLI_DIR/DEBIAN"

    assert_file "$TMP_CLI_DIR/usr/bin/mkpass"
    assert_file "$TMP_CLI_DIR/usr/share/man/man1/mkpass.1.gz"
    assert_file "$TMP_CLI_DIR/DEBIAN/control"

    # Verify control metadata
    PKG_NAME=$(grep "^Package:" "$TMP_CLI_DIR/DEBIAN/control" | awk '{print $2}')
    PKG_ARCH=$(grep "^Architecture:" "$TMP_CLI_DIR/DEBIAN/control" | awk '{print $2}')
    if [ "$PKG_NAME" = "mkpass" ]; then
        echo "OK: Package name is 'mkpass'"
    else
        echo "FAIL: Expected package name 'mkpass', got '$PKG_NAME'"
        FAILED=1
    fi
    echo "Package architecture: $PKG_ARCH"

    # Test running extracted binary
    echo "Testing extracted CLI binary..."
    if "$TMP_CLI_DIR/usr/bin/mkpass" --help >/dev/null 2>&1; then
        echo "OK: mkpass --help executed successfully from extracted package"
    else
        echo "FAIL: mkpass --help execution failed from extracted package"
        FAILED=1
    fi

    if "$TMP_CLI_DIR/usr/bin/mkpass" config --help >/dev/null 2>&1; then
        echo "OK: mkpass config --help executed successfully"
    else
        echo "FAIL: mkpass config --help execution failed"
        FAILED=1
    fi

    if "$TMP_CLI_DIR/usr/bin/mkpass" update --help >/dev/null 2>&1; then
        echo "OK: mkpass update --help executed successfully"
    else
        echo "FAIL: mkpass update --help execution failed"
        FAILED=1
    fi

    # Test man page decompression and content
    if zcat "$TMP_CLI_DIR/usr/share/man/man1/mkpass.1.gz" | grep -q "mkpass config"; then
        echo "OK: Decompressed mkpass.1.gz contains subcommand documentation"
    else
        echo "FAIL: Decompressed mkpass.1.gz missing subcommand documentation"
        FAILED=1
    fi
else
    echo "FAIL: No CLI .deb package found in dist/"
    FAILED=1
fi

if [ -n "$DEB_GUI" ] && [ -f "$DEB_GUI" ]; then
    echo "Verifying GUI Debian package: $(basename "$DEB_GUI")"
    dpkg-deb -x "$DEB_GUI" "$TMP_GUI_DIR"
    dpkg-deb -e "$DEB_GUI" "$TMP_GUI_DIR/DEBIAN"

    assert_file "$TMP_GUI_DIR/usr/bin/mkpass-gui"
    assert_file "$TMP_GUI_DIR/usr/share/applications/mkpass.desktop"
    assert_file "$TMP_GUI_DIR/usr/share/icons/hicolor/256x256/apps/mkpass.png"
    assert_file "$TMP_GUI_DIR/usr/share/man/man1/mkpass-gui.1.gz"
    assert_file "$TMP_GUI_DIR/DEBIAN/control"

    GUI_PKG_NAME=$(grep "^Package:" "$TMP_GUI_DIR/DEBIAN/control" | awk '{print $2}')
    if [ "$GUI_PKG_NAME" = "mkpass-gui" ]; then
        echo "OK: GUI Package name is 'mkpass-gui'"
    else
        echo "FAIL: Expected package name 'mkpass-gui', got '$GUI_PKG_NAME'"
        FAILED=1
    fi

    if [ -x "$TMP_GUI_DIR/usr/bin/mkpass-gui" ]; then
        echo "OK: mkpass-gui executable flag set"
    else
        echo "FAIL: mkpass-gui is not executable"
        FAILED=1
    fi

    if grep -q "Exec=mkpass-gui" "$TMP_GUI_DIR/usr/share/applications/mkpass.desktop"; then
        echo "OK: Desktop entry defines Exec=mkpass-gui"
    else
        echo "FAIL: Desktop entry missing Exec=mkpass-gui"
        FAILED=1
    fi
else
    echo "FAIL: No GUI .deb package found in dist/"
    FAILED=1
fi

# -----------------------------------------------------------------------------
# 3. Optional Container Installation Test
# -----------------------------------------------------------------------------
echo ""
echo ">>> 3. Checking Container Environment for Clean Install / Uninstall..."

CONTAINER_CMD=""
if command -v docker >/dev/null 2>&1; then
    CONTAINER_CMD="docker"
elif command -v podman >/dev/null 2>&1; then
    CONTAINER_CMD="podman"
fi

if [ -n "$CONTAINER_CMD" ]; then
    echo "Container runtime detected ($CONTAINER_CMD). Running clean installation tests..."
    # Test on clean Ubuntu container
    $CONTAINER_CMD run --rm -v "$PROJECT_ROOT/dist:/packages:ro" ubuntu:24.04 bash -c "
        apt-get update -qq && \
        apt-get install -y -qq /packages/mkpass_*.deb && \
        mkpass --help >/dev/null && \
        echo 'Container: CLI installed and verified.' && \
        dpkg -r mkpass && \
        echo 'Container: CLI uninstalled cleanly.'
    " || { echo "FAIL: Container verification failed"; FAILED=1; }
else
    echo "Notice: Neither docker nor podman is available on this host."
    echo "        Extracted binary and package tree verification completed above."
fi

# -----------------------------------------------------------------------------
# Summary
# -----------------------------------------------------------------------------
echo ""
echo "=================================================="
if [ "$FAILED" -eq 0 ]; then
    echo " SUCCESS: All package verification checks passed!"
    echo "=================================================="
    exit 0
else
    echo " FAILURE: Some package verification checks failed."
    echo "=================================================="
    exit 1
fi
