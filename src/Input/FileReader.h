#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>

namespace iiXml::IO {
// The caller owns the storage policy and the lifetime captured by this callable.
// A missing value means that the file could not be read.
using FileReader = std::function<std::optional<std::string>(const std::filesystem::path&)>;
}
