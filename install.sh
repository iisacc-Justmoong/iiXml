#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
PREFIX="${HOME}/.local/iiXml"
QT_ROOT="${HOME}/Qt/6.8.3"

MACOS_QT_PREFIX="${IIXML_MACOS_QT_PREFIX:-${QT_ROOT}/macos}"
IOS_QT_PREFIX="${IIXML_IOS_QT_PREFIX:-${QT_ROOT}/ios}"
ANDROID_QT_PREFIX="${IIXML_ANDROID_QT_PREFIX:-${QT_ROOT}/android_arm64_v8a}"
WASM_QT_PREFIX="${IIXML_WASM_QT_PREFIX:-}"

MACOS_BUILD_DIR="${BUILD_DIR}"
IOS_BUILD_DIR="${BUILD_DIR}/platforms/ios"
ANDROID_BUILD_DIR="${BUILD_DIR}/platforms/android"
WASM_BUILD_DIR="${BUILD_DIR}/platforms/wasm"

MACOS_PREFIX="${PREFIX}"
MACOS_PLATFORM_PREFIX="${PREFIX}/platforms/macos"
IOS_PREFIX="${PREFIX}/platforms/ios"
ANDROID_PREFIX="${PREFIX}/platforms/android"
WASM_PREFIX="${PREFIX}/platforms/wasm"

default_install_platforms() {
    case "$(uname -s)" in
        Darwin)
            echo "macos,ios,android,wasm"
            ;;
        *)
            echo "macos"
            ;;
    esac
}

AUTO_INSTALL_PLATFORMS=true
if [[ -n "${IIXML_INSTALL_PLATFORMS:-}" ]]; then
    AUTO_INSTALL_PLATFORMS=false
    INSTALL_PLATFORMS="${IIXML_INSTALL_PLATFORMS}"
else
    INSTALL_PLATFORMS="$(default_install_platforms)"
fi

cmake_cache_value() {
    local cache_file="$1"
    local key="$2"

    awk -F= -v key="${key}" \
        '$1 == key || $1 ~ ("^" key ":") { print substr($0, index($0, "=") + 1); exit }' \
        "${cache_file}"
}

remove_stale_build_dir() {
    local build_dir="$1"
    local cache_file="${build_dir}/CMakeCache.txt"

    if [[ ! -f "${cache_file}" ]]; then
        return
    fi

    local cached_source
    local cached_build
    local stale_cache=false

    cached_source="$(cmake_cache_value "${cache_file}" "CMAKE_HOME_DIRECTORY")"
    cached_build="$(cmake_cache_value "${cache_file}" "CMAKE_CACHEFILE_DIR")"

    if [[ -n "${cached_source}" && "${cached_source}" != "${ROOT_DIR}" ]]; then
        stale_cache=true
    fi

    if [[ -n "${cached_build}" && "${cached_build}" != "${build_dir}" ]]; then
        stale_cache=true
    fi

    if [[ "${stale_cache}" == true ]]; then
        echo "Removing stale CMake build directory: ${build_dir}"
        echo "  cached source: ${cached_source:-unknown}"
        echo "  current source: ${ROOT_DIR}"
        if [[ "${build_dir}" == "${BUILD_DIR}" ]]; then
            rm -rf "${BUILD_DIR}"
        else
            rm -rf "${build_dir}"
        fi
    fi
}

join_prefixes() {
    local IFS=';'
    echo "$*"
}

skip_or_fail() {
    local platform="$1"
    local message="$2"

    if [[ "${AUTO_INSTALL_PLATFORMS}" == true ]]; then
        echo "Skipping iiXml ${platform} package: ${message}" >&2
        return 1
    fi

    echo "Cannot install iiXml ${platform} package: ${message}" >&2
    exit 1
}

require_dir_or_skip() {
    local platform="$1"
    local path="$2"
    local message="$3"

    if [[ -d "${path}" ]]; then
        return 0
    fi

    skip_or_fail "${platform}" "${message}: ${path}"
}

require_file_or_skip() {
    local platform="$1"
    local path="$2"
    local message="$3"

    if [[ -f "${path}" ]]; then
        return 0
    fi

    skip_or_fail "${platform}" "${message}: ${path}"
}

remove_legacy_include_dir() {
    local install_prefix="$1"

    LEGACY_INCLUDE_DIR="${install_prefix}/include/iiXml"
    if [[ -d "${LEGACY_INCLUDE_DIR}" && ( -f "${LEGACY_INCLUDE_DIR}/iiXml.h" || -d "${LEGACY_INCLUDE_DIR}/Src" ) ]]; then
        echo "Removing legacy iiXml include directory: ${LEGACY_INCLUDE_DIR}"
        rm -rf "${LEGACY_INCLUDE_DIR}"
    fi
}

resolve_existing_qt_prefix() {
    local candidate

    for candidate in "$@"; do
        if [[ -n "${candidate}" && -d "${candidate}/lib/cmake/Qt6" ]]; then
            echo "${candidate}"
            return
        fi
    done
}

detect_android_sdk_root() {
    local candidate

    for candidate in \
        "${ANDROID_SDK_ROOT:-}" \
        "${ANDROID_HOME:-}" \
        "${HOME}/Library/Android/sdk" \
        "/opt/homebrew/share/android-commandlinetools" \
        "/usr/local/share/android-commandlinetools" \
        "/opt/android/sdk"; do
        if [[ -n "${candidate}" && -d "${candidate}" ]]; then
            echo "${candidate}"
            return
        fi
    done
}

latest_child_dir() {
    local root="$1"

    if [[ ! -d "${root}" ]]; then
        return
    fi

    find "${root}" -mindepth 1 -maxdepth 1 -type d | sort | tail -n 1
}

detect_android_ndk_root() {
    local sdk_root="$1"
    local candidate
    local sdk_ndk

    sdk_ndk="$(latest_child_dir "${sdk_root}/ndk")"
    for candidate in \
        "${ANDROID_NDK_ROOT:-}" \
        "${ANDROID_NDK_HOME:-}" \
        "${CMAKE_ANDROID_NDK:-}" \
        "${sdk_ndk}" \
        "/opt/homebrew/share/android-ndk" \
        "/usr/local/share/android-ndk" \
        "/opt/android/android-ndk-r26b"; do
        if [[ -n "${candidate}" && -d "${candidate}" ]]; then
            echo "${candidate}"
            return
        fi
    done
}

resolve_emscripten_toolchain_file() {
    local candidate
    local emsdk_root

    for candidate in \
        "${IIXML_EMSCRIPTEN_TOOLCHAIN_FILE:-}" \
        "${QT_CHAINLOAD_TOOLCHAIN_FILE:-}" \
        "${LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE:-}"; do
        if [[ -n "${candidate}" && -f "${candidate}" ]]; then
            echo "${candidate}"
            return
        fi
    done

    for emsdk_root in "${IIXML_EMSDK_ROOT:-}" "${EMSDK:-}" "${HOME}/emsdk" "${HOME}/.local/emsdk" "/opt/emsdk"; do
        candidate="${emsdk_root}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"
        if [[ -n "${emsdk_root}" && -f "${candidate}" ]]; then
            echo "${candidate}"
            return
        fi
    done
}

install_macos() {
    require_dir_or_skip "macos" "${MACOS_QT_PREFIX}/lib/cmake/Qt6" "Qt macOS package is required" || return 1

    remove_stale_build_dir "${MACOS_BUILD_DIR}"

    local cmake_prefix_path
    cmake_prefix_path="$(join_prefixes "${MACOS_QT_PREFIX}")"

    echo "Configuring iiXml for macOS install prefix: ${MACOS_PREFIX}"
    cmake --fresh \
        -S "${ROOT_DIR}" \
        -B "${MACOS_BUILD_DIR}" \
        -DCMAKE_INSTALL_PREFIX="${MACOS_PREFIX}" \
        -DCMAKE_PREFIX_PATH="${cmake_prefix_path}" \
        -DIIXML_BUILD_SHARED=ON

    echo "Building iiXml for macOS in ${MACOS_BUILD_DIR}"
    cmake --build "${MACOS_BUILD_DIR}"

    echo "Running iiXml macOS tests"
    ctest --test-dir "${MACOS_BUILD_DIR}" --output-on-failure

    LEGACY_INCLUDE_DIR="${PREFIX}/include/iiXml"
    remove_legacy_include_dir "${PREFIX}"
    echo "Installing iiXml macOS package into ${PREFIX}"
    cmake --install "${BUILD_DIR}" --prefix "${PREFIX}"

    remove_legacy_include_dir "${MACOS_PLATFORM_PREFIX}"
    echo "Installing iiXml macOS platform package into ${MACOS_PLATFORM_PREFIX}"
    cmake --install "${MACOS_BUILD_DIR}" --prefix "${MACOS_PLATFORM_PREFIX}"

    cmake -E copy_if_different \
        "${MACOS_BUILD_DIR}/iiXmlConfigVersionRoot.cmake" \
        "${PREFIX}/lib/cmake/iiXml/iiXmlConfigVersion.cmake"
}

install_ios() {
    local ios_toolchain_file="${IOS_QT_PREFIX}/lib/cmake/Qt6/qt.toolchain.cmake"

    require_dir_or_skip "ios" "${IOS_QT_PREFIX}/lib/cmake/Qt6" "Qt iOS package is required" || return 1
    require_file_or_skip "ios" "${ios_toolchain_file}" "Qt iOS toolchain file is required" || return 1

    remove_stale_build_dir "${IOS_BUILD_DIR}"

    local cmake_prefix_path
    cmake_prefix_path="$(join_prefixes "${IOS_QT_PREFIX}" "${MACOS_QT_PREFIX}")"

    echo "Configuring iiXml for iOS install prefix: ${IOS_PREFIX}"
    cmake --fresh \
        -S "${ROOT_DIR}" \
        -B "${IOS_BUILD_DIR}" \
        -G Xcode \
        -DCMAKE_TOOLCHAIN_FILE="${ios_toolchain_file}" \
        -DCMAKE_SYSTEM_NAME=iOS \
        -DCMAKE_OSX_SYSROOT=iphoneos \
        -DCMAKE_OSX_ARCHITECTURES=arm64 \
        -DCMAKE_INSTALL_PREFIX="${IOS_PREFIX}" \
        -DCMAKE_PREFIX_PATH="${cmake_prefix_path}" \
        -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO \
        -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED=NO \
        -DIIXML_BUILD_SHARED=ON

    echo "Building iiXml for iOS in ${IOS_BUILD_DIR}"
    cmake --build "${IOS_BUILD_DIR}" --config Release --target iiXml

    remove_legacy_include_dir "${IOS_PREFIX}"
    echo "Installing iiXml iOS platform package into ${IOS_PREFIX}"
    cmake --install "${IOS_BUILD_DIR}" --prefix "${IOS_PREFIX}" --config Release
}

install_android() {
    local android_toolchain_file="${ANDROID_QT_PREFIX}/lib/cmake/Qt6/qt.toolchain.cmake"
    local android_sdk_root
    local android_ndk_root

    require_dir_or_skip "android" "${ANDROID_QT_PREFIX}/lib/cmake/Qt6" "Qt Android package is required" || return 1
    require_file_or_skip "android" "${android_toolchain_file}" "Qt Android toolchain file is required" || return 1

    android_sdk_root="$(detect_android_sdk_root)"
    if [[ -z "${android_sdk_root}" ]]; then
        skip_or_fail "android" "Android SDK root is required; set ANDROID_SDK_ROOT or ANDROID_HOME" || return 1
    fi

    android_ndk_root="$(detect_android_ndk_root "${android_sdk_root}")"
    if [[ -z "${android_ndk_root}" ]]; then
        skip_or_fail "android" "Android NDK root is required; install it under ${android_sdk_root}/ndk or set ANDROID_NDK_ROOT" || return 1
    fi

    remove_stale_build_dir "${ANDROID_BUILD_DIR}"

    local cmake_prefix_path
    cmake_prefix_path="$(join_prefixes "${ANDROID_QT_PREFIX}" "${MACOS_QT_PREFIX}")"

    local cmake_args=(
        --fresh
        -S "${ROOT_DIR}"
        -B "${ANDROID_BUILD_DIR}"
        -DCMAKE_TOOLCHAIN_FILE="${android_toolchain_file}"
        -DQT_HOST_PATH="${MACOS_QT_PREFIX}"
        -DANDROID_SDK_ROOT="${android_sdk_root}"
        -DANDROID_HOME="${android_sdk_root}"
        -DANDROID_NDK_ROOT="${android_ndk_root}"
        -DCMAKE_ANDROID_NDK="${android_ndk_root}"
        -DCMAKE_INSTALL_PREFIX="${ANDROID_PREFIX}"
        -DCMAKE_PREFIX_PATH="${cmake_prefix_path}"
        -DCMAKE_BUILD_TYPE=Release
        -DIIXML_BUILD_SHARED=ON
    )

    if [[ -n "${IIXML_ANDROID_ABI:-}" ]]; then
        cmake_args+=(-DANDROID_ABI="${IIXML_ANDROID_ABI}")
    fi

    local android_env=(
        "ANDROID_SDK_ROOT=${android_sdk_root}"
        "ANDROID_HOME=${android_sdk_root}"
        "ANDROID_NDK_ROOT=${android_ndk_root}"
        "ANDROID_NDK_HOME=${android_ndk_root}"
        "CMAKE_ANDROID_NDK=${android_ndk_root}"
    )

    echo "Configuring iiXml for Android install prefix: ${ANDROID_PREFIX}"
    env "${android_env[@]}" cmake "${cmake_args[@]}"

    echo "Building iiXml for Android in ${ANDROID_BUILD_DIR}"
    env "${android_env[@]}" cmake --build "${ANDROID_BUILD_DIR}" --target iiXml

    remove_legacy_include_dir "${ANDROID_PREFIX}"
    echo "Installing iiXml Android platform package into ${ANDROID_PREFIX}"
    env "${android_env[@]}" cmake --install "${ANDROID_BUILD_DIR}" --prefix "${ANDROID_PREFIX}"
}

install_wasm() {
    local wasm_qt_prefix
    local wasm_toolchain_file
    local emscripten_toolchain_file

    wasm_qt_prefix="$(resolve_existing_qt_prefix \
        "${WASM_QT_PREFIX}" \
        "${QT_ROOT}/wasm_multithread" \
        "${QT_ROOT}/wasm_singlethread" \
        "${QT_ROOT}/wasm_32" \
        "${QT_ROOT}/wasm")"
    if [[ -z "${wasm_qt_prefix}" ]]; then
        skip_or_fail "wasm" "Qt WASM package is required under ${QT_ROOT}" || return 1
    fi

    wasm_toolchain_file="${wasm_qt_prefix}/lib/cmake/Qt6/qt.toolchain.cmake"
    require_file_or_skip "wasm" "${wasm_toolchain_file}" "Qt WASM toolchain file is required" || return 1

    emscripten_toolchain_file="$(resolve_emscripten_toolchain_file)"
    if [[ -z "${emscripten_toolchain_file}" ]]; then
        skip_or_fail "wasm" "Emscripten.cmake is required; set IIXML_EMSDK_ROOT, EMSDK, or IIXML_EMSCRIPTEN_TOOLCHAIN_FILE" || return 1
    fi

    remove_stale_build_dir "${WASM_BUILD_DIR}"

    local cmake_prefix_path
    cmake_prefix_path="$(join_prefixes "${wasm_qt_prefix}" "${MACOS_QT_PREFIX}")"

    echo "Configuring iiXml for WASM install prefix: ${WASM_PREFIX}"
    cmake --fresh \
        -S "${ROOT_DIR}" \
        -B "${WASM_BUILD_DIR}" \
        -DCMAKE_TOOLCHAIN_FILE="${wasm_toolchain_file}" \
        -DQT_CHAINLOAD_TOOLCHAIN_FILE="${emscripten_toolchain_file}" \
        -DCMAKE_INSTALL_PREFIX="${WASM_PREFIX}" \
        -DCMAKE_PREFIX_PATH="${cmake_prefix_path}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DIIXML_BUILD_SHARED=OFF

    echo "Building iiXml for WASM in ${WASM_BUILD_DIR}"
    cmake --build "${WASM_BUILD_DIR}" --target iiXml

    remove_legacy_include_dir "${WASM_PREFIX}"
    echo "Installing iiXml WASM platform package into ${WASM_PREFIX}"
    cmake --install "${WASM_BUILD_DIR}" --prefix "${WASM_PREFIX}"
}

run_platform_install() {
    local platform="$1"

    case "${platform}" in
        macos)
            install_macos
            ;;
        ios)
            install_ios
            ;;
        android)
            install_android
            ;;
        wasm)
            install_wasm
            ;;
        "")
            return 1
            ;;
        *)
            echo "Unsupported iiXml install platform: ${platform}" >&2
            echo "Supported platforms: macos, ios, android, wasm" >&2
            exit 1
            ;;
    esac
}

IFS=',;' read -ra requested_platforms <<< "${INSTALL_PLATFORMS}"

echo "Installing iiXml for platforms: ${INSTALL_PLATFORMS}"
installed_platforms=0
installed_platform_names=()
for platform in "${requested_platforms[@]}"; do
    platform="$(echo "${platform}" | xargs)"
    if [[ -z "${platform}" ]]; then
        continue
    fi

    if run_platform_install "${platform}"; then
        ((installed_platforms += 1))
        installed_platform_names+=("${platform}")
    fi
done

if [[ "${installed_platforms}" -eq 0 ]]; then
    echo "No iiXml platform packages were installed." >&2
    exit 1
fi

installed_platforms_text="$(IFS=,; echo "${installed_platform_names[*]}")"
echo "iiXml installed for platforms: ${installed_platforms_text}."
