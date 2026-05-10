#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
CACHE_FILE="${BUILD_DIR}/CMakeCache.txt"
PREFIX="${HOME}/.local/iiXml"
QT_PREFIX="${HOME}/Qt/6.8.3/macos"

cmake_cache_value() {
    local key="$1"

    awk -F= -v key="${key}" \
        '$1 == key || $1 ~ ("^" key ":") { print substr($0, index($0, "=") + 1); exit }' \
        "${CACHE_FILE}"
}

if [[ -f "${CACHE_FILE}" ]]; then
    cached_source="$(cmake_cache_value "CMAKE_HOME_DIRECTORY")"
    cached_build="$(cmake_cache_value "CMAKE_CACHEFILE_DIR")"
    stale_cache=false

    if [[ -n "${cached_source}" && "${cached_source}" != "${ROOT_DIR}" ]]; then
        stale_cache=true
    fi

    if [[ -n "${cached_build}" && "${cached_build}" != "${BUILD_DIR}" ]]; then
        stale_cache=true
    fi

    if [[ "${stale_cache}" == true ]]; then
        echo "Removing stale CMake build directory: ${BUILD_DIR}"
        echo "  cached source: ${cached_source:-unknown}"
        echo "  current source: ${ROOT_DIR}"
        rm -rf "${BUILD_DIR}"
    fi
fi

cmake_args=(
    --fresh
    -S "${ROOT_DIR}"
    -B "${BUILD_DIR}"
    -DCMAKE_INSTALL_PREFIX="${PREFIX}"
)

if [[ -d "${QT_PREFIX}/lib/cmake/Qt6" ]]; then
    cmake_args+=(-DCMAKE_PREFIX_PATH="${QT_PREFIX}")
fi

echo "Configuring iiXml for install prefix: ${PREFIX}"
cmake "${cmake_args[@]}"

echo "Building iiXml in ${BUILD_DIR}"
cmake --build "${BUILD_DIR}"

echo "Running iiXml tests"
ctest --test-dir "${BUILD_DIR}" --output-on-failure

LEGACY_INCLUDE_DIR="${PREFIX}/include/iiXml"
if [[ -d "${LEGACY_INCLUDE_DIR}" && ( -f "${LEGACY_INCLUDE_DIR}/iiXml.h" || -d "${LEGACY_INCLUDE_DIR}/Src" ) ]]; then
    echo "Removing legacy iiXml include directory: ${LEGACY_INCLUDE_DIR}"
    rm -rf "${LEGACY_INCLUDE_DIR}"
fi

echo "Installing iiXml into ${PREFIX}"
cmake --install "${BUILD_DIR}" --prefix "${PREFIX}"

echo "iiXml installed."
