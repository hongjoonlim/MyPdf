#include "i18n.h"
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibraryInfo>
#include <QLocale>
#include <QMap>
#include <QPointer>
#include <QTranslator>

// Explicit initialization ensures the resources are linked from the static library.
static void initializeTranslationResources() { Q_INIT_RESOURCE(translations); }

namespace {
QString currentLanguage = "en";
QPointer<QTranslator> qtTranslator;
const QMap<QString,QJsonObject> &catalogs() {
    static const auto data=[] {
        initializeTranslationResources();
        QMap<QString,QJsonObject> result;
        for (const auto &code:QStringList{"de","ko"}) {
            QFile file(":/translations/"+code+".json");
            if (!file.open(QIODevice::ReadOnly)) qFatal("Unable to open translation resource.");
            QJsonParseError error;
            auto document=QJsonDocument::fromJson(file.readAll(),&error);
            if (error.error!=QJsonParseError::NoError || !document.isObject())
                qFatal("Invalid translation resource.");
            result.insert(code,document.object());
        }
        return result;
    }();
    return data;
}
}
QString I18n::text(const QString &source) {
    const auto messages=catalogs().value(currentLanguage).value("messages").toObject();
    return messages.value(source).toString(source);
}
QString I18n::sourceText(const QString &translated) {
    for (const auto &catalog:catalogs()) {
        const auto messages=catalog.value("messages").toObject();
        for (auto it=messages.begin();it!=messages.end();++it)
            if (it.value().toString()==translated) return it.key();
    }
    return translated;
}
QString I18n::languageName(const QString &code) {
    if (code=="en") return QStringLiteral("English");
    return catalogs().value(code).value("languageName").toString(code);
}
QString I18n::language() { return currentLanguage; }
void I18n::setLanguage(const QString &language) {
    currentLanguage=(language=="de" || language=="ko") ? language : "en";
    QLocale::setDefault(QLocale(currentLanguage));
    if (qtTranslator) QCoreApplication::removeTranslator(qtTranslator);
    else qtTranslator=new QTranslator(QCoreApplication::instance());
    if (qtTranslator->load("qtbase_"+currentLanguage,QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        QCoreApplication::installTranslator(qtTranslator);
}
