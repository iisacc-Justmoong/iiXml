#pragma once

#include <iiXml>
#include <iiFileProvider.h>

// Application-owned bridge: neither SDK links to the other.
inline iiXml::IO::FileReader fileProviderReader() {
    return [](const std::filesystem::path& path) -> std::optional<std::string> {
        try {
            return iiFileProvider::File::read(iiFileProvider::File::pathString(path)).toStdString();
        } catch (const iiFileProvider::FileError&) {
            return std::nullopt;
        }
    };
}
