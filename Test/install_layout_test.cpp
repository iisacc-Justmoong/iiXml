#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

int failures = 0;

std::string read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return {};
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void expect_contains(const std::string& content, const std::string& token, const char* message) {
    if (content.find(token) == std::string::npos) {
        std::cerr << message << '\n';
        ++failures;
    }
}

void expect_executable_script_header(const std::string& content) {
    expect_contains(content, "#!/usr/bin/env bash", "install.sh must be a bash script.");
    expect_contains(content, "set -euo pipefail", "install.sh must fail on script errors.");
}

} // namespace

int main() {
    const std::string root = IIXML_SOURCE_DIR;
    const std::string install_script = read_file(root + "/install.sh");
    const std::string cmake_lists = read_file(root + "/CMakeLists.txt");
    const std::string config_template = read_file(root + "/cmake/iiXmlConfig.cmake.in");
    const std::string docs = read_file(root + "/Docs/install.md");

    expect_executable_script_header(install_script);
    expect_contains(install_script, "PREFIX=\"${HOME}/.local/SDK/iiXml\"",
        "install.sh must install to ~/.local/SDK/iiXml.");
    expect_contains(install_script, "BUILD_DIR=\"${ROOT_DIR}/build\"",
        "install.sh must use build/ as the build directory.");
    expect_contains(install_script, "macos,ios,android,wasm",
        "install.sh must default to the full macOS host platform set.");
    expect_contains(install_script, "IIXML_INSTALL_PLATFORMS",
        "install.sh must allow explicitly constrained platform installs.");
    expect_contains(install_script, "MACOS_PLATFORM_PREFIX=\"${PREFIX}/platforms/macos\"",
        "install.sh must install a macOS platform package mirror.");
    expect_contains(install_script, "IOS_PREFIX=\"${PREFIX}/platforms/ios\"",
        "install.sh must install an iOS platform package.");
    expect_contains(install_script, "ANDROID_PREFIX=\"${PREFIX}/platforms/android\"",
        "install.sh must install an Android platform package.");
    expect_contains(install_script, "WASM_PREFIX=\"${PREFIX}/platforms/wasm\"",
        "install.sh must install a WASM platform package.");
    expect_contains(install_script, "IOS_BUILD_DIR=\"${BUILD_DIR}/platforms/ios\"",
        "install.sh must keep the iOS build under build/platforms/ios.");
    expect_contains(install_script, "ANDROID_BUILD_DIR=\"${BUILD_DIR}/platforms/android\"",
        "install.sh must keep the Android build under build/platforms/android.");
    expect_contains(install_script, "WASM_BUILD_DIR=\"${BUILD_DIR}/platforms/wasm\"",
        "install.sh must keep the WASM build under build/platforms/wasm.");
    expect_contains(install_script, "install_macos()",
        "install.sh must provide a macOS platform install path.");
    expect_contains(install_script, "install_ios()",
        "install.sh must provide an iOS platform install path.");
    expect_contains(install_script, "install_android()",
        "install.sh must provide an Android platform install path.");
    expect_contains(install_script, "/opt/homebrew/share/android-commandlinetools",
        "install.sh must detect Homebrew's Android SDK package.");
    expect_contains(install_script, "/opt/homebrew/share/android-ndk",
        "install.sh must detect Homebrew's Android NDK package.");
    expect_contains(install_script, "install_wasm()",
        "install.sh must provide a WASM platform install path.");
    expect_contains(install_script, "-DIIXML_BUILD_SHARED=OFF",
        "install.sh must build the WASM package as a static library.");
    expect_contains(install_script, "CMAKE_HOME_DIRECTORY",
        "install.sh must inspect the cached source directory.");
    expect_contains(install_script, "CMAKE_CACHEFILE_DIR",
        "install.sh must inspect the cached build directory.");
    expect_contains(install_script, "rm -rf \"${BUILD_DIR}\"",
        "install.sh must remove build/ when the CMake cache belongs to another tree.");
    expect_contains(install_script, "--fresh",
        "install.sh must configure with a fresh CMake cache.");
    expect_contains(install_script, "cmake --install \"${BUILD_DIR}\" --prefix \"${PREFIX}\"",
        "install.sh must run cmake install with the ~/.local/SDK/iiXml prefix.");
    expect_contains(install_script, "iiXmlConfigVersionRoot.cmake",
        "install.sh must publish the architecture-independent root package version.");
    expect_contains(install_script, "LEGACY_INCLUDE_DIR=\"${PREFIX}/include/iiXml\"",
        "install.sh must detect the legacy include/iiXml directory.");
    expect_contains(install_script, "rm -rf \"${LEGACY_INCLUDE_DIR}\"",
        "install.sh must remove the legacy include/iiXml directory before installing the <iiXml> header.");

    expect_contains(cmake_lists, "cmake_minimum_required(VERSION 3.24)",
        "CMakeLists.txt must require the minimum version that supports cmake --fresh.");
    expect_contains(cmake_lists, "PROJECT_IS_TOP_LEVEL AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT",
        "Default CMake installs must use the SDK prefix while respecting explicit overrides.");
    expect_contains(cmake_lists, "USERPROFILE",
        "Windows user profiles must provide the default prefix when HOME is empty.");
    expect_contains(cmake_lists, ".local/SDK/iiXml",
        "CMake must use the SDK package installation root.");
    expect_contains(cmake_lists, "install(TARGETS iiXml",
        "CMakeLists.txt must install the iiXml library target.");
    expect_contains(cmake_lists, "IIXML_BUILD_SHARED",
        "CMakeLists.txt must expose a shared/static library switch for cross-platform packages.");
    expect_contains(cmake_lists, "EXPORT iiXmlTargets",
        "CMakeLists.txt must export iiXmlTargets.");
    expect_contains(cmake_lists, "NAMESPACE iiXml::",
        "CMakeLists.txt must install the iiXml:: imported target namespace.");
    expect_contains(cmake_lists, "configure_package_config_file",
        "CMakeLists.txt must generate iiXmlConfig.cmake.");
    expect_contains(cmake_lists, "iiXmlConfigVersionRoot.cmake",
        "CMakeLists.txt must generate a separate root package version.");
    expect_contains(cmake_lists, "ARCH_INDEPENDENT",
        "The root package version must accept both native and WASM consumers.");
    expect_contains(cmake_lists, "install(FILES iiXml iiXml.h",
        "CMakeLists.txt must install the extensionless <iiXml> umbrella header.");
    expect_contains(cmake_lists, "install(DIRECTORY Src/",
        "CMakeLists.txt must install public module headers.");
    expect_contains(cmake_lists, "DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Src",
        "CMakeLists.txt must install module headers without occupying include/iiXml.");

    expect_contains(config_template, "find_dependency(Qt6 6.8.3 EXACT COMPONENTS Core)",
        "iiXmlConfig.cmake.in must declare the Qt 6.8.3 dependency.");
    expect_contains(config_template, "_iiXml_detect_target_platform",
        "iiXmlConfig.cmake.in must detect target platforms.");
    expect_contains(config_template, "platforms/${_iiXmlTargetPlatform}",
        "iiXmlConfig.cmake.in must delegate root-prefix package loads to platform packages.");
    expect_contains(config_template, "iiXmlTargets.cmake",
        "iiXmlConfig.cmake.in must include exported targets.");
    expect_contains(config_template, "INTERFACE_LINK_OPTIONS",
        "iiXmlConfig.cmake.in must propagate the macOS runtime library search path.");
    expect_contains(config_template, "CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES",
        "iiXmlConfig.cmake.in must detect when an ambient library path suppresses CMake's build RPATH.");
    expect_contains(config_template, "LINKER:-rpath,${_iiXmlPackageLibraryDirectory}",
        "iiXmlConfig.cmake.in must derive the macOS runtime search path from the relocated package prefix.");

    expect_contains(docs, "./install.sh", "Docs/install.md must document the install script.");
    expect_contains(docs, "macos,ios,android,wasm",
        "Docs/install.md must document the default all-platform install set.");
    expect_contains(docs, "IIXML_INSTALL_PLATFORMS=ios ./install.sh",
        "Docs/install.md must document constrained platform installs.");
    expect_contains(docs, "/opt/homebrew/share/android-commandlinetools",
        "Docs/install.md must document Homebrew Android SDK discovery.");
    expect_contains(docs, "~/.local/SDK/iiXml/platforms/wasm",
        "Docs/install.md must document the WASM platform package.");
    expect_contains(docs, "~/.local/SDK/iiXml",
        "Docs/install.md must document the fixed ~/.local/SDK/iiXml install prefix.");
    expect_contains(docs, "find_package(iiXml CONFIG REQUIRED)",
        "Docs/install.md must document CMake package loading.");
    expect_contains(docs, "CMake 3.24",
        "Docs/install.md must document the minimum supported CMake version.");
    expect_contains(docs, "32-bit WASM",
        "Docs/install.md must document architecture-independent root dispatch.");
    expect_contains(docs, "DYLD_LIBRARY_PATH",
        "Docs/install.md must document that installed macOS consumers need no runtime environment override.");
    expect_contains(docs, "iiXml::iiXml", "Docs/install.md must document the imported target.");
    expect_contains(docs, "#include <iiXml>",
        "Docs/install.md must document extensionless umbrella include usage.");

    return failures == 0 ? 0 : 1;
}
