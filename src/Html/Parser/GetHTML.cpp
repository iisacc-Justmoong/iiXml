#include "GetHTML.h"

#include <iiXml.h>

#include <QByteArray>
#include <QDebug>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace iiHtmlBlock {

namespace {

std::string ToUtf8String(const QString& value) {
    const QByteArray utf8 = value.toUtf8();
    return std::string(utf8.constData(), static_cast<std::size_t>(utf8.size()));
}

QString FromUtf8String(const std::string& value) {
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

std::string ResolveHtmlTagName(std::string_view source_tag_name) {
    return std::string(source_tag_name);
}

bool IsAttributeNameSafe(std::string_view name) {
    if (name.empty()) {
        return false;
    }

    for (char character : name) {
        const bool invalid =
            character == ' '
            || character == '\n'
            || character == '\r'
            || character == '\t'
            || character == '"'
            || character == '\''
            || character == '>'
            || character == '/'
            || character == '=';
        if (invalid) {
            return false;
        }
    }

    return true;
}

std::string EscapeText(std::string_view value) {
    std::string escaped;
    escaped.reserve(value.size());

    for (char character : value) {
        switch (character) {
            case '&':
                escaped += "&amp;";
                break;
            case '<':
                escaped += "&lt;";
                break;
            case '>':
                escaped += "&gt;";
                break;
            default:
                escaped.push_back(character);
                break;
        }
    }

    return escaped;
}

std::string EscapeAttribute(std::string_view value) {
    std::string escaped;
    escaped.reserve(value.size());

    for (char character : value) {
        switch (character) {
            case '&':
                escaped += "&amp;";
                break;
            case '"':
                escaped += "&quot;";
                break;
            case '<':
                escaped += "&lt;";
                break;
            case '>':
                escaped += "&gt;";
                break;
            default:
                escaped.push_back(character);
                break;
        }
    }

    return escaped;
}

std::string Slice(std::string_view source, std::size_t begin, std::size_t end) {
    if (begin > end || end > source.size()) {
        return {};
    }

    return std::string(source.substr(begin, end - begin));
}

bool IsAsciiSpace(char value) {
    return value == ' ' || value == '\n' || value == '\r' || value == '\t';
}

std::string_view TrimLeadingAscii(std::string_view source) {
    std::size_t begin = 0;
    while (begin < source.size() && IsAsciiSpace(source[begin])) {
        ++begin;
    }

    return source.substr(begin);
}

std::string NormalizeParserInput(std::string_view xml) {
    xml = TrimLeadingAscii(xml);

    bool consumed = true;
    while (consumed) {
        consumed = false;

        if (xml.rfind("<?xml", 0) == 0) {
            const std::size_t end = xml.find("?>");
            if (end != std::string_view::npos) {
                xml = TrimLeadingAscii(xml.substr(end + 2));
                consumed = true;
            }
        }

        if (xml.rfind("<!DOCTYPE", 0) == 0 || xml.rfind("<!doctype", 0) == 0) {
            const std::size_t end = xml.find('>');
            if (end != std::string_view::npos) {
                xml = TrimLeadingAscii(xml.substr(end + 1));
                consumed = true;
            }
        }
    }

    return std::string(xml);
}

std::string RenderAttributes(
    std::string_view xml,
    const iiXml::Parser::TagNode& node
) {
    std::string html;

    for (const iiXml::Parser::TagField& field : node.Fields) {
        if (!IsAttributeNameSafe(field.Name)) {
            continue;
        }

        html.push_back(' ');
        html += field.Name;
        if (field.HasValue) {
            html += "=\"";
            html += EscapeAttribute(Slice(xml, field.ValueBegin, field.ValueEnd));
            html.push_back('"');
        }
    }

    return html;
}

std::string RenderNode(std::string_view xml, const iiXml::Parser::TagNode& node) {
    const std::string html_tag_name = ResolveHtmlTagName(node.Range.TagName);
    std::string html = "<" + html_tag_name + RenderAttributes(xml, node) + ">";

    std::vector<const iiXml::Parser::TagNode*> children;
    children.reserve(node.Children.size());
    for (const iiXml::Parser::TagNode& child : node.Children) {
        children.push_back(&child);
    }

    std::sort(
        children.begin(),
        children.end(),
        [](const iiXml::Parser::TagNode* left, const iiXml::Parser::TagNode* right) {
            return left->Range.RawBegin < right->Range.RawBegin;
        }
    );

    std::size_t cursor = node.Range.ValueBegin;
    for (const iiXml::Parser::TagNode* child : children) {
        if (cursor < child->Range.RawBegin) {
            html += EscapeText(xml.substr(cursor, child->Range.RawBegin - cursor));
        }

        html += RenderNode(xml, *child);
        cursor = child->Range.RawEnd;
    }

    if (cursor < node.Range.ValueEnd) {
        html += EscapeText(xml.substr(cursor, node.Range.ValueEnd - cursor));
    }

    html += "</";
    html += html_tag_name;
    html.push_back('>');
    return html;
}

} // namespace

GetHTML::GetHTML(QObject* parent)
    : QObject(parent) {
    qDebug() << "GetHTML::GetHTML constructed";
}

bool GetHTML::Parse(const char* xml) {
    if (xml == nullptr) {
        Clear();
        error_ = "xml input is null";
        qDebug() << "GetHTML::Parse failed"
                 << "reason=" << QString::fromStdString(error_);
        return false;
    }

    return Parse(std::string_view(xml));
}

bool GetHTML::Parse(std::string_view xml) {
    qDebug() << "GetHTML::Parse begin"
             << "input_size=" << xml.size();
    try {
        Clear();
        const std::string normalized = NormalizeParserInput(xml);
        const std::string_view parse_input(normalized.data(), normalized.size());

        const iiXml::Parser::TagParser parser;
        const std::optional<std::vector<iiXml::Parser::TagNode>> parsed = parser.ParseAll(parse_input);
        if (!parsed.has_value()) {
            error_ = "iiXml tag parser rejected input";
            qDebug() << "GetHTML::Parse failed"
                     << "reason=" << QString::fromStdString(error_);
            return false;
        }

        if (parsed->empty()) {
            error_ = "iiXml tag parser returned no tag";
            qDebug() << "GetHTML::Parse failed"
                     << "reason=" << QString::fromStdString(error_);
            return false;
        }

        tag_name_ = ResolveHtmlTagName(parsed->front().Range.TagName);
        source_tag_name_ = parsed->front().Range.TagName;
        for (const iiXml::Parser::TagNode& node : *parsed) {
            html_ += RenderNode(parse_input, node);
        }

        qDebug() << "GetHTML::Parse parsed"
                 << "source_tag=" << QString::fromStdString(source_tag_name_)
                 << "html_tag=" << QString::fromStdString(tag_name_)
                 << "html_size=" << html_.size();
        return true;
    } catch (const std::exception& exception) {
        Clear();
        error_ = std::string("html parse exception: ") + exception.what();
        qDebug() << "GetHTML::Parse exception"
                 << "what=" << exception.what();
        return false;
    } catch (...) {
        Clear();
        error_ = "html parse exception: unknown";
        qDebug() << "GetHTML::Parse exception"
                 << "what=unknown";
        return false;
    }
}

bool GetHTML::Parse(const QString& xml) {
    const std::string bytes = ToUtf8String(xml);
    return Parse(std::string_view(bytes.data(), bytes.size()));
}

const std::string& GetHTML::GetHTMLText() const {
    return html_;
}

const std::string& GetHTML::GetTagName() const {
    return tag_name_;
}

const std::string& GetHTML::GetSourceTagName() const {
    return source_tag_name_;
}

const std::string& GetHTML::GetError() const {
    return error_;
}

void GetHTML::ParseXml(const QString& xml) {
    if (Parse(xml)) {
        emit Parsed(FromUtf8String(html_));
        return;
    }

    emit ParseFailed(FromUtf8String(error_));
}

void GetHTML::Clear() {
    html_.clear();
    tag_name_.clear();
    source_tag_name_.clear();
    error_.clear();
}

} // namespace iiHtmlBlock
