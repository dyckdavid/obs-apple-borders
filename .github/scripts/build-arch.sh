#!/usr/bin/env bash
# Build obs-apple-borders inside Arch Linux and produce a .pkg.tar.zst
# Install layout matches Arch OBS: /usr/lib/obs-plugins + /usr/share/obs/obs-plugins
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

if [[ "${RUNNER_DEBUG:-}" ]]; then set -x; fi

CONFIG="${1:-RelWithDebInfo}"
ARCH="$(uname -m)"
PKGREL="${PKGREL:-1}"
OUT_DIR="${ROOT}/release"
PKG_ROOT="${OUT_DIR}/arch-pkgroot"
BUILD_DIR="${ROOT}/build_arch_${ARCH}"

echo "::group::Install Arch build dependencies"
# Pin jack2 so pacman never prompts for a jack provider in CI.
pacman -Syu --noconfirm --needed \
  base-devel \
  cmake \
  ninja \
  pkgconf \
  git \
  jq \
  jack2 \
  libx11 \
  libglvnd \
  mesa \
  obs-studio
echo "::endgroup::"

PRODUCT_NAME="$(jq -r '.name' buildspec.json)"
PRODUCT_VERSION="$(jq -r '.version' buildspec.json)"
if [[ -z "${PRODUCT_NAME}" || -z "${PRODUCT_VERSION}" || "${PRODUCT_NAME}" == null ]]; then
  echo "Failed to read name/version from buildspec.json" >&2
  exit 1
fi

echo "Building ${PRODUCT_NAME} ${PRODUCT_VERSION} for ${ARCH} config=${CONFIG}"

echo "::group::Configure + build"
rm -rf "${BUILD_DIR}" "${PKG_ROOT}"
cmake -S "${ROOT}" -B "${BUILD_DIR}" -G Ninja \
  -DCMAKE_BUILD_TYPE="${CONFIG}" \
  -DCMAKE_INSTALL_PREFIX=/usr \
  -DCMAKE_INSTALL_LIBDIR=lib \
  -DENABLE_FRONTEND_API=OFF \
  -DENABLE_QT=OFF \
  -DCMAKE_COMPILE_WARNING_AS_ERROR=ON
cmake --build "${BUILD_DIR}" --parallel
echo "::endgroup::"

echo "::group::Stage install tree"
DESTDIR="${PKG_ROOT}" cmake --install "${BUILD_DIR}"
install -Dm644 "${ROOT}/LICENSE" "${PKG_ROOT}/usr/share/licenses/${PRODUCT_NAME}/LICENSE"

# Sanity: Arch OBS load paths
if ! find "${PKG_ROOT}/usr/lib/obs-plugins" -maxdepth 1 -type f \( -name "${PRODUCT_NAME}.so" -o -name "${PRODUCT_NAME}.so.*" \) | grep -q .; then
  echo "Missing plugin .so under /usr/lib/obs-plugins" >&2
  find "${PKG_ROOT}" -type f | sort >&2
  exit 1
fi
if [[ ! -d "${PKG_ROOT}/usr/share/obs/obs-plugins/${PRODUCT_NAME}" ]]; then
  echo "Missing plugin data under /usr/share/obs/obs-plugins/${PRODUCT_NAME}" >&2
  find "${PKG_ROOT}" -type f | sort >&2
  exit 1
fi

# Some CMake MODULE installs keep a versioned SONAME; OBS loads the unversioned name
if [[ ! -e "${PKG_ROOT}/usr/lib/obs-plugins/${PRODUCT_NAME}.so" ]]; then
  so="$(find "${PKG_ROOT}/usr/lib/obs-plugins" -maxdepth 1 -type f -name "${PRODUCT_NAME}.so.*" | head -n1)"
  ln -s "$(basename "${so}")" "${PKG_ROOT}/usr/lib/obs-plugins/${PRODUCT_NAME}.so"
fi
echo "::endgroup::"

echo "::group::Assemble .pkg.tar.zst"
mkdir -p "${OUT_DIR}"
PKGINFO="${PKG_ROOT}/.PKGINFO"
SIZE="$(du -sk "${PKG_ROOT}/usr" | awk '{print $1 * 1024}')"
# Approximate installed size for pacman metadata
cat >"${PKGINFO}" <<EOF
pkgname = ${PRODUCT_NAME}
pkgbase = ${PRODUCT_NAME}
pkgver = ${PRODUCT_VERSION}-${PKGREL}
pkgdesc = Apple-styled borders, PiP, backdrops, and polish filters for OBS Studio
url = https://github.com/dyckdavid/obs-apple-borders
builddate = $(date +%s)
packager = GitHub Actions <ci@github>
size = ${SIZE}
arch = ${ARCH}
license = MIT
depend = obs-studio
EOF

PKG_FILE="${OUT_DIR}/${PRODUCT_NAME}-${PRODUCT_VERSION}-${ARCH}.pkg.tar.zst"
(
  cd "${PKG_ROOT}"
  # pacman expects .PKGINFO at archive root; exclude build leftovers
  bsdtar -c --zstd -f "${PKG_FILE}" .PKGINFO usr
)

echo "Created ${PKG_FILE}"
bsdtar -tf "${PKG_FILE}" | head -n 40
echo "::endgroup::"

# Also keep a simple path listing for CI summaries
{
  echo "### Arch package"
  echo ""
  echo "- File: \`$(basename "${PKG_FILE}")\`"
  echo "- Plugin: \`/usr/lib/obs-plugins/${PRODUCT_NAME}.so\`"
  echo "- Data: \`/usr/share/obs/obs-plugins/${PRODUCT_NAME}/\`"
} >>"${GITHUB_STEP_SUMMARY:-/dev/null}"
