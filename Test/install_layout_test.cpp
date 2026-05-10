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
    expect_contains(install_script, "PREFIX=\"${HOME}/.local/iiXml\"",
        "install.sh must install to ~/.local/iiXml.");
    expect_contains(install_script, "BUILD_DIR=\"${ROOT_DIR}/build\"",
        "install.sh must use build/ as the build directory.");
    expect_contains(install_script, "CMAKE_HOME_DIRECTORY",
        "install.sh must inspect the cached source directory.");
    expect_contains(install_script, "CMAKE_CACHEFILE_DIR",
        "install.sh must inspect the cached build directory.");
    expect_contains(install_script, "rm -rf \"${BUILD_DIR}\"",
        "install.sh must remove build/ when the CMake cache belongs to another tree.");
    expect_contains(install_script, "--fresh",
        "install.sh must configure with a fresh CMake cache.");
    expect_contains(install_script, "cmake --install \"${BUILD_DIR}\" --prefix \"${PREFIX}\"",
        "install.sh must run cmake install with the ~/.local/iiXml prefix.");
    expect_contains(install_script, "LEGACY_INCLUDE_DIR=\"${PREFIX}/include/iiXml\"",
        "install.sh must detect the legacy include/iiXml directory.");
    expect_contains(install_script, "rm -rf \"${LEGACY_INCLUDE_DIR}\"",
        "install.sh must remove the legacy include/iiXml directory before installing the <iiXml> header.");

    expect_contains(cmake_lists, "install(TARGETS iiXml",
        "CMakeLists.txt must install the iiXml library target.");
    expect_contains(cmake_lists, "EXPORT iiXmlTargets",
        "CMakeLists.txt must export iiXmlTargets.");
    expect_contains(cmake_lists, "NAMESPACE iiXml::",
        "CMakeLists.txt must install the iiXml:: imported target namespace.");
    expect_contains(cmake_lists, "configure_package_config_file",
        "CMakeLists.txt must generate iiXmlConfig.cmake.");
    expect_contains(cmake_lists, "install(FILES iiXml iiXml.h",
        "CMakeLists.txt must install the extensionless <iiXml> umbrella header.");
    expect_contains(cmake_lists, "install(DIRECTORY Src/",
        "CMakeLists.txt must install public module headers.");
    expect_contains(cmake_lists, "DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Src",
        "CMakeLists.txt must install module headers without occupying include/iiXml.");

    expect_contains(config_template, "find_dependency(Qt6 6.8.3 EXACT COMPONENTS Core)",
        "iiXmlConfig.cmake.in must declare the Qt 6.8.3 dependency.");
    expect_contains(config_template, "iiXmlTargets.cmake",
        "iiXmlConfig.cmake.in must include exported targets.");

    expect_contains(docs, "./install.sh", "Docs/install.md must document the install script.");
    expect_contains(docs, "~/.local/iiXml",
        "Docs/install.md must document the fixed ~/.local/iiXml install prefix.");
    expect_contains(docs, "find_package(iiXml CONFIG REQUIRED)",
        "Docs/install.md must document CMake package loading.");
    expect_contains(docs, "iiXml::iiXml", "Docs/install.md must document the imported target.");
    expect_contains(docs, "#include <iiXml>",
        "Docs/install.md must document extensionless umbrella include usage.");

    return failures == 0 ? 0 : 1;
}
