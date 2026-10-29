#pragma once
#include <QString>
namespace I18n {
QString text(const QString &source);
QString sourceText(const QString &translated);
QString language();
QString languageName(const QString &code);
void setLanguage(const QString &language);
}
