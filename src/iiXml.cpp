#include "iiXml.h"

#include <QDebug>

#include <exception>

void Launch() {
    qDebug() << "iiXml::Launch begin";
    try {
        qDebug() << "iiXml::Launch library Launch";
        qDebug() << "iiXml::Launch finished";
    } catch (const std::exception& exception) {
        qDebug() << "iiXml::Launch exception"
                 << "what=" << exception.what();
    } catch (...) {
        qDebug() << "iiXml::Launch exception"
                 << "what=unknown";
    }
}
