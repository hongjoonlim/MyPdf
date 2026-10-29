#pragma once
#include <QTextDocument>
#include <memory>
namespace Docx {
std::unique_ptr<QTextDocument> read(const QString &path);
void savePdf(const QTextDocument &document,const QString &destination,const QString &sourcePath = {});
}
