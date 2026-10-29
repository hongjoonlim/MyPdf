#include "toolpaths.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>
namespace {
void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
void executable(const QString &path) {
    QFile file(path); require(file.open(QIODevice::WriteOnly),"create tool fixture");
    file.write("tool path fixture"); file.close();
    require(file.setPermissions(QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner),"set executable permissions");
}
}
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    QTemporaryDir temp;
    try {
        const auto original=qgetenv("MYPDF_QPDF");
        qputenv("MYPDF_QPDF","custom/tool");
        require(Tools::qpdf(temp.path())=="custom/tool","explicit override has priority");
        qunsetenv("MYPDF_QPDF");
#ifdef Q_OS_WIN
        const QString name="qpdf.exe";
#else
        const QString name="qpdf";
#endif
        auto root=temp.filePath("App With Spaces/Contents");
        QDir().mkpath(root+"/MacOS"); QDir().mkpath(root+"/Helpers");
        const auto helper=root+"/Helpers/"+name; executable(helper);
        require(QDir::cleanPath(Tools::qpdf(root+"/MacOS"))==helper,"macOS helper lookup");
        const auto sibling=root+"/MacOS/"+name; executable(sibling);
        require(Tools::qpdf(root+"/MacOS")==sibling,"sibling tool takes priority over helper and PATH");
        qputenv("MYPDF_QPDF",original);
        std::cout<<"PASS: qpdf override, sibling executable, and macOS helper lookup\n";
        return 0;
    } catch (const std::exception &e) { std::cerr<<e.what()<<'\n'; return 1; }
}
