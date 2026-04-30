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
    expect_contains(install_script, "PREFIX=\"${HOME}/.local\"",
        "install.sh must install to ~/.local.");
    expect_contains(install_script, "BUILD_DIR=\"${ROOT_DIR}/build\"",
        "install.sh must use build/ as the build directory.");
    expect_contains(install_script, "cmake --install \"${BUILD_DIR}\" --prefix \"${PREFIX}\"",
        "install.sh must run cmake install with the ~/.local prefix.");

    expect_contains(cmake_lists, "install(TARGETS iiXml",
        "CMakeLists.txt must install the iiXml library target.");
    expect_contains(cmake_lists, "EXPORT iiXmlTargets",
        "CMakeLists.txt must export iiXmlTargets.");
    expect_contains(cmake_lists, "NAMESPACE iiXml::",
        "CMakeLists.txt must install the iiXml:: imported target namespace.");
    expect_contains(cmake_lists, "configure_package_config_file",
        "CMakeLists.txt must generate iiXmlConfig.cmake.");
    expect_contains(cmake_lists, "install(DIRECTORY Src/",
        "CMakeLists.txt must install public module headers.");

    expect_contains(config_template, "find_dependency(Qt6 6.8.3 EXACT COMPONENTS Core)",
        "iiXmlConfig.cmake.in must declare the Qt 6.8.3 dependency.");
    expect_contains(config_template, "iiXmlTargets.cmake",
        "iiXmlConfig.cmake.in must include exported targets.");

    expect_contains(docs, "./install.sh", "Docs/install.md must document the install script.");
    expect_contains(docs, "find_package(iiXml CONFIG REQUIRED)",
        "Docs/install.md must document CMake package loading.");
    expect_contains(docs, "iiXml::iiXml", "Docs/install.md must document the imported target.");

    return failures == 0 ? 0 : 1;
}
