#include "licenses.h"
#include "i18n.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QComboBox>
#include <QTextBrowser>
#include <QDialogButtonBox>
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QCoreApplication>
static void initializeLicenseResources() { Q_INIT_RESOURCE(licenses); }
void Licenses::show(QWidget *parent) {
    initializeLicenseResources();
    QDialog dialog(parent); dialog.setObjectName("licensesDialog");
    dialog.setWindowTitle(I18n::text("Licenses…")); dialog.resize(760,560);
    auto *layout=new QVBoxLayout(&dialog); auto *selector=new QComboBox; layout->addWidget(selector);
    selector->addItems({"NOTICE","LICENSE","THIRD_PARTY_NOTICES.md","DISTRIBUTING.md"});
    for (const auto &name:QDir(":/licenses/LICENSES").entryList(QDir::Files,QDir::Name)) selector->addItem("LICENSES/"+name);
    const QDir executable(QCoreApplication::applicationDirPath());
#ifdef Q_OS_MACOS
    const QString installed=executable.absoluteFilePath("../Resources/licenses");
#else
    const QString installed=executable.absoluteFilePath("../share/MyPdf/licenses");
#endif
    QDirIterator files(installed,QDir::Files,QDirIterator::Subdirectories);
    while (files.hasNext()) {
        const QString path=files.next();
        if (path.endsWith(".tar.gz")) continue;
        const QString relative=QDir(installed).relativeFilePath(path);
        selector->addItem("Installed / "+relative,path);
    }
    auto *browser=new QTextBrowser; browser->setOpenExternalLinks(true); layout->addWidget(browser);
    auto load=[selector,browser] {
        QFile file(selector->currentData().isValid()?selector->currentData().toString():":/licenses/"+selector->currentText());
        if (file.open(QIODevice::ReadOnly)) browser->setPlainText(QString::fromUtf8(file.readAll()));
    };
    QObject::connect(selector,&QComboBox::currentIndexChanged,&dialog,[load] { load(); }); load();
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Close); layout->addWidget(buttons);
    QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject); dialog.exec();
}
