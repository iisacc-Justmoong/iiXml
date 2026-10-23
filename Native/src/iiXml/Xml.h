#pragma once
#include <cstddef>
#include <expected>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ii::xml {
enum class ErrorCode { syntax, forbidden_dtd, limit_exceeded, invalid_state };
struct Error final { ErrorCode code; int line=0; std::string message; };
template<class T> using Result=std::expected<T,Error>;
struct Limits final {
    std::size_t input_bytes=8*1024*1024, output_bytes=8*1024*1024;
    std::size_t text_bytes=1024*1024, attributes=128, depth=64, events=100000;
};
struct Attribute final { std::string name, value, local_name, namespace_uri; };
enum class Kind { start_element, end_element, text, comment, processing_instruction, doctype };
struct Event final {
    Kind kind;
    std::string name, value, local_name, namespace_uri;
    std::vector<Attribute> attributes;
    std::size_t depth=0;
    bool empty=false;
};
class Reader final {
public:
    ~Reader();
    Reader(Reader&&) noexcept;
    Reader& operator=(Reader&&) noexcept;
    Reader(const Reader&)=delete;
    Reader& operator=(const Reader&)=delete;
    [[nodiscard]] static Result<Reader> open(std::string utf8,Limits={});
    [[nodiscard]] Result<std::optional<Event>> next();
private:
    struct State;
    std::unique_ptr<State> state_;
    explicit Reader(std::unique_ptr<State>);
};
class Writer final {
public:
    explicit Writer(Limits={});
    ~Writer();
    Writer(Writer&&) noexcept;
    Writer& operator=(Writer&&) noexcept;
    Writer(const Writer&)=delete;
    Writer& operator=(const Writer&)=delete;
    [[nodiscard]] Result<void> doctype(std::string_view root_name);
    [[nodiscard]] Result<void> start(std::string_view name,std::span<const Attribute> attributes={});
    [[nodiscard]] Result<void> text(std::string_view);
    [[nodiscard]] Result<void> end();
    [[nodiscard]] Result<std::string> finish();
private:
    struct State;
    std::unique_ptr<State> state_;
};
}
