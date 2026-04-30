#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

std::string read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return {};
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

} // namespace

int main() {
    const std::string policy = read_file(std::string(IIXML_SOURCE_DIR) + "/AGENTS.md");
    if (policy.find("Qt 6.8.3") == std::string::npos) {
        std::cerr << "AGENTS.md must declare Qt 6.8.3 as the project Qt policy.\n";
        return 1;
    }

    if (policy.find("~/Qt") == std::string::npos) {
        std::cerr << "AGENTS.md must declare ~/Qt as the project Qt path policy.\n";
        return 1;
    }

    return 0;
}
