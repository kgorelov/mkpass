#!/bin/bash
set -e

# Ensure execution from project root
cd "$(dirname "$0")/.."
PROJECT_ROOT="$(pwd)"

# Resolve default version from CMakeLists.txt
DEFAULT_VERSION=$(grep -m1 -E 'project\s*\([^)]*VERSION' "$PROJECT_ROOT/CMakeLists.txt" 2>/dev/null | sed -E 's/.*VERSION\s+([0-9.]+).*/\1/' || true)
DEFAULT_VERSION="${DEFAULT_VERSION:-0.1.0}"

# Resolve release tag and version
TAG_NAME="${TAG_NAME:-${GITHUB_REF_NAME:-v$DEFAULT_VERSION}}"
TAG_NAME="${TAG_NAME#refs/tags/}"
TAG_NAME="${TAG_NAME#refs/heads/}"
TAG_CLEAN="${TAG_NAME//\//-}"

# Extract version starting with digit if available, else fallback to CMakeLists version
EXTRACTED_VER=$(echo "$TAG_NAME" | grep -oE '[0-9]+\.[0-9]+(\.[0-9]+)?' | head -n1 || true)
VERSION="${VERSION:-$EXTRACTED_VER}"
VERSION="${VERSION:-$DEFAULT_VERSION}"
if [[ ! "$VERSION" =~ ^[0-9] ]]; then
  VERSION="$DEFAULT_VERSION"
fi
VERSION="${VERSION%%-*}"

echo "=================================================="
echo " Packaging Linux Deliverables for mkpass"
echo " Tag:     $TAG_NAME"
echo " Version: $VERSION"
echo "=================================================="

mkdir -p dist

# -----------------------------------------------------------------------------
# 1. Debian Package Generation (.deb)
# -----------------------------------------------------------------------------
echo ""
echo ">>> Building Debian packages (.deb)..."

if command -v dpkg-buildpackage >/dev/null 2>&1; then
    echo "Using native dpkg-buildpackage..."

    # Create temporary changelog entry for this version
    DATE_RFC2822="$(date -R)"
    cat <<EOF > debian/changelog
mkpass (${VERSION}-1) unstable; urgency=medium

  * Release version ${VERSION}.

 -- Kirill Gorelov <kgorelov@gmail.com>  ${DATE_RFC2822}
EOF

    # Build binary packages without signing (-d overrides unsatisfied build dependency check aborts)
    dpkg-buildpackage -us -uc -b -d

    # dpkg-buildpackage deposits packages into parent directory
    PARENT_DIR="$(dirname "$PROJECT_ROOT")"
    FOUND_DEB=0
    for deb in "$PARENT_DIR"/mkpass*_${VERSION}-1_*.deb "$PARENT_DIR"/mkpass*_${VERSION}_*.deb; do
        if [ -f "$deb" ]; then
            echo "Copying $(basename "$deb") to dist/"
            cp -f "$deb" dist/
            FOUND_DEB=1
        fi
    done

    if [ "$FOUND_DEB" -eq 0 ]; then
        # Fallback search for any deb matching mkpass in parent or root
        find "$PARENT_DIR" -maxdepth 1 -name "mkpass*.deb" -exec cp -f {} dist/ \;
    fi

    # Clean debian temporary files
    fakeroot debian/rules clean 2>/dev/null || true
    echo "Debian packaging complete."
elif command -v cpack >/dev/null 2>&1; then
    echo "dpkg-buildpackage not found, falling back to CPack DEB generator..."
    cmake -B build-cpack -DWITH_GUI=ON -DWITH_TESTS=OFF -DCMAKE_BUILD_TYPE=Release
    cmake --build build-cpack
    (cd build-cpack && cpack -G DEB)
    find build-cpack -name "*.deb" -exec cp -f {} dist/ \;
    echo "CPack DEB packaging complete."
else
    echo "Warning: Neither dpkg-buildpackage nor cpack is available. Skipping .deb packaging."
fi

# -----------------------------------------------------------------------------
# 2. RPM Package Generation (.rpm)
# -----------------------------------------------------------------------------
echo ""
echo ">>> Building RPM packages (.rpm)..."

if command -v rpmbuild >/dev/null 2>&1; then
    echo "Using native rpmbuild..."
    RPMBUILD_DIR="$PROJECT_ROOT/build/rpmbuild"
    rm -rf "$RPMBUILD_DIR"
    mkdir -p "$RPMBUILD_DIR"/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}

    # Generate source tarball from current working tree
    tar -czf "$RPMBUILD_DIR/SOURCES/mkpass-${VERSION}.tar.gz" \
        --exclude="./build*" \
        --exclude="./.git" \
        --exclude="./dist" \
        --exclude="./.pixi" \
        --exclude="./obj-*" \
        --exclude="./android" \
        --exclude="./mkpass_web" \
        --transform="s,^\.,mkpass-${VERSION}," .

    # Copy spec file and build RPM
    cp packaging/rpm/mkpass.spec "$RPMBUILD_DIR/SPECS/"
    rpmbuild --nodeps \
             --define "_topdir $RPMBUILD_DIR" \
             --define "version $VERSION" \
             -bb "$RPMBUILD_DIR/SPECS/mkpass.spec"

    # Copy generated RPM packages to dist/
    find "$RPMBUILD_DIR/RPMS" -name "*.rpm" -exec cp -f {} dist/ \;
    echo "RPM packaging complete."
elif command -v cpack >/dev/null 2>&1; then
    echo "rpmbuild not found, checking CPack RPM generator..."
    if cpack --help 2>&1 | grep -q "RPM"; then
        cmake -B build-cpack -DWITH_GUI=ON -DWITH_TESTS=OFF -DCMAKE_BUILD_TYPE=Release
        cmake --build build-cpack
        (cd build-cpack && cpack -G RPM) || echo "CPack RPM generation failed (requires rpmbuild on host)."
        find build-cpack -name "*.rpm" -exec cp -f {} dist/ \;
    fi
else
    echo "Warning: rpmbuild is not available. Skipping .rpm packaging."
fi

# -----------------------------------------------------------------------------
# Summary of Artifacts
# -----------------------------------------------------------------------------
echo ""
echo "=================================================="
echo " Linux Packaging Completed"
echo " Generated distribution assets in dist/:"
shopt -s nullglob
PACKAGES=(dist/*.deb dist/*.rpm)
shopt -u nullglob
if [ ${#PACKAGES[@]} -gt 0 ]; then
    ls -lh "${PACKAGES[@]}"
else
    echo "No packages generated."
fi
echo "=================================================="
