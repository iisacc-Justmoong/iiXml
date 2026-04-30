#include "iiXml.h"

#include <QDebug>

#include <exception>

void launch() {
    qDebug() << "iiXml::launch begin";
    try {
        qDebug() << "iiXml::launch library launch";
        qDebug() << "iiXml::launch finished";
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::launch exception"
                 << "what=" << exception.what();
    } catch (...) {
        qDebug() << "iiXml::launch exception"
                 << "what=unknown";
    }
}
