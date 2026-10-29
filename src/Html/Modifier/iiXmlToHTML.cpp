#include "iiXmlToHTML.h"

#include "src/Html/Parser/GetHTML.h"

#include <QByteArray>
#include <QDebug>

#include <cstddef>
#include <exception>
#include <string>
#include <string_view>

namespace iiHtmlBlock {

namespace {

std::string ToUtf8String(const QString& value) {
    const QByteArray utf8 = value.toUtf8();
    return std::string(utf8.constData(), static_cast<std::size_t>(utf8.size()));
}

QString FromUtf8String(const std::string& value) {
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

} // namespace

iiXmlToHTML::iiXmlToHTML(QObject* parent)
    : QObject(parent) {
    qDebug() << "iiXmlToHTML::iiXmlToHTML constructed";
}

bool iiXmlToHTML::Convert(const char* xml) {
    if (xml == nullptr) {
        Clear();
        error_ = "xml input is null";
        qDebug() << "iiXmlToHTML::Convert failed"
                 << "reason=" << QString::fromStdString(error_);
        return false;
    }

    return Convert(std::string_view(xml));
}

bool iiXmlToHTML::Convert(std::string_view xml) {
    qDebug() << "iiXmlToHTML::Convert begin"
             << "input_size=" << xml.size();
    try {
        Clear();

        GetHTML parser;
        if (!parser.Parse(xml)) {
            error_ = parser.GetError();
            if (error_.empty()) {
                error_ = "html conversion failed";
            }
            qDebug() << "iiXmlToHTML::Convert failed"
                     << "reason=" << QString::fromStdString(error_);
            return false;
        }

        html_ = parser.GetHTMLText();
        qDebug() << "iiXmlToHTML::Convert converted"
                 << "html_size=" << html_.size();
        return true;
    } catch (const std::exception& exception) {
        Clear();
        error_ = std::string("iiXml to HTML conversion exception: ") + exception.what();
        qDebug() << "iiXmlToHTML::Convert exception"
                 << "what=" << exception.what();
        return false;
    } catch (...) {
        Clear();
        error_ = "iiXml to HTML conversion exception: unknown";
        qDebug() << "iiXmlToHTML::Convert exception"
                 << "what=unknown";
        return false;
    }
}

bool iiXmlToHTML::Convert(const QString& xml) {
    const std::string bytes = ToUtf8String(xml);
    return Convert(std::string_view(bytes.data(), bytes.size()));
}

const std::string& iiXmlToHTML::GetHTMLText() const {
    return html_;
}

const std::string& iiXmlToHTML::GetError() const {
    return error_;
}

void iiXmlToHTML::ConvertXml(const QString& xml) {
    if (Convert(xml)) {
        emit Converted(FromUtf8String(html_));
        return;
    }

    emit ConvertFailed(FromUtf8String(error_));
}

void iiXmlToHTML::Clear() {
    html_.clear();
    error_.clear();
}

} // namespace iiHtmlBlock
