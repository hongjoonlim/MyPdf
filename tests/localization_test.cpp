#define MYPDF_NO_MAIN
#include "../src/main.cpp"
#include <QPdfWriter>
#include <QTemporaryDir>
#include <QLibraryInfo>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
bool containsColor(const QImage &image,const QColor &color) {
    for (int y=0;y<image.height();++y)
        for (int x=0;x<image.width();++x)
            if (image.pixelColor(x,y)==color) return true;
    return false;
}
QAction *languageAction(Window &window,const QString &code) {
    auto *a=window.findChild<QAction *>("language_"+code);
    check(a!=nullptr,"language action exists"); return a;
}
QAction *themeAction(Window &window,const QString &theme) {
    auto *a=window.findChild<QAction *>("theme_"+theme);
    check(a!=nullptr,"theme action exists"); return a;
}
}
int main(int argc,char **argv) {
    QApplication app(argc,argv);
    app.setOrganizationName("MyPdf"); app.setApplicationName("MyPdf");
    QTemporaryDir config;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,config.path());
    try {
        Window window;
        check(I18n::language()=="en","initial language is English");
        check(themeAction(window,"light")->isChecked(),"light theme is the default");
        const auto paletteIcon=Icons::line(Icons::Symbol::Open);
        themeAction(window,"dark")->trigger();
        check(themeAction(window,"dark")->isChecked(),"dark theme can be selected");
        check(QApplication::palette().color(QPalette::Window)==QColor("#1f2125"),"dark palette applied");
        check(containsColor(paletteIcon.pixmap(24,24).toImage(),QColor("#e7e9ed")),"icons follow the dark palette");
        check(QSettings().value("ui/theme").toString()=="dark","dark theme preference saved");
        themeAction(window,"light")->trigger();
        check(QApplication::palette().color(QPalette::Window)==QColor("#f5f6f8"),"light theme can be restored");
        check(containsColor(paletteIcon.pixmap(24,24).toImage(),QColor("#263244")),"icons follow the light palette");
        const QString fixture=config.filePath("sample.pdf");
        { QPdfWriter writer(fixture); QPainter painter(&writer); painter.drawText(100,100,"First"); writer.newPage(); painter.drawText(100,100,"Second"); writer.newPage(); }
        window.openPath(fixture);
        auto *pages=window.findChild<QListWidget *>("pageList");
        auto *selectTool=window.findChild<QToolButton *>("selectTool");
        auto *signatureTool=window.findChild<QToolButton *>("signatureTool");
        auto *zoom=window.findChild<QComboBox *>("zoomLevel");
        check(pages && selectTool && signatureTool && zoom,"document controls available");
        check(window.findChildren<QToolBar *>().size()==1,"single unified toolbar replaces task tabs");
        window.resize(850,650); window.show(); app.processEvents();
        pages->setCurrentRow(1);
        // Make an unsaved edit, then verify that translation never rebuilds the document.
        for (auto *a:window.findChildren<QAction *>())
            if (a->text()=="Move Earlier") { a->trigger(); break; }
        check(window.windowTitle().contains('*'),"document has an unsaved edit");
        pages->setCurrentRow(1); pages->item(0)->setSelected(true);
        const auto title=window.windowTitle(); const auto zoomText=zoom->currentText(); const auto item=pages->item(1);
        themeAction(window,"dark")->trigger();
        check(window.windowTitle()==title && pages->item(1)==item,"theme switch preserves the open document");
        check(pages->currentRow()==1 && pages->selectedItems().size()==2,"theme switch preserves page selection");
        check(selectTool->isChecked() && zoom->currentText()==zoomText,"theme switch preserves view controls");
        themeAction(window,"light")->trigger();
        for (const auto &code:QStringList{"en","de","ko","de"}) {
            languageAction(window,code)->trigger();
            check(I18n::language()==code,"language switch");
            check(languageAction(window,code)->isChecked(),"language checkmark");
            check(languageAction(window,"ko")->text()==I18n::languageName("ko"),"native language labels remain unchanged");
            check(QSettings().value("ui/language").toString()==code,"language preference saved");
            check(themeAction(window,"dark")->text()==I18n::text("Dark"),"theme labels translated");
            check(window.windowTitle()==title,"unsaved document and title preserved");
            check(pages->count()==3 && pages->currentRow()==1 && pages->item(1)==item,"page identity and current page preserved");
            check(pages->selectedItems().size()==2,"multiple selection preserved");
            check(selectTool->isChecked() && zoom->currentText()==zoomText,"active tool and zoom preserved");
            check(pages->item(1)->text()==I18n::text("Page %1").arg(2),"page labels translated");
            check(window.menuBar()->actions().first()->text()==I18n::text("&File"),"menu translated");
            check(signatureTool->text()==I18n::text("Sign") && signatureTool->menu(),"compact signature label and menu survive translation");
            check(window.findChild<QToolButton *>("fitTool")->menu(),"fit menu survives translation");
            for (const auto &name:QStringList{"signatureTool","highlightTool","pagesTool","fitTool"}) {
                const auto *button=window.findChild<QToolButton *>(name);
                check(button && button->popupMode()==QToolButton::InstantPopup && button->property("popupMenu").toBool(),"all menu tools share popup behavior and arrow styling");
            }
            check(selectTool->text()==I18n::text("Select"),"selection tool keeps compact label on state updates");
            auto *pagesTool=window.findChild<QToolButton *>("pagesTool");
            check(pagesTool && pagesTool->text()==I18n::text("Pages") && pagesTool->menu(),"pages menu label translates");
            for (auto *command:pagesTool->menu()->actions()) check(command->menu()==nullptr,"page commands are directly accessible without nested menus");
            app.processEvents();
            auto *bar=window.findChild<QToolBar *>("mainTools");
            check(pagesTool->isVisibleTo(bar) && bar->rect().contains(pagesTool->geometry()),"primary tools fit at minimum width in every language");
            check(window.findChild<QLabel *>("sideTitle")->text()==I18n::text("Page Thumbnails"),"sidebar translated");
            Document empty;
            try { empty.save(config.filePath("invalid.pdf")); check(false,"empty save must fail"); }
            catch (const std::runtime_error &e) { check(QString::fromUtf8(e.what())==I18n::text("Open a PDF first."),"backend error translated"); }
        }
        auto *textTool=window.findChild<QToolButton *>("textTool");
        auto *cancelTool=window.findChild<QAction *>("cancelPlacement");
        textTool->click();
        check(textTool->isChecked() && !selectTool->isChecked() && cancelTool->isVisible(),"compact text tool enters placement mode");
        languageAction(window,"en")->trigger();
        check(textTool->isChecked() && textTool->text()=="Text","language change preserves active placement mode");
        selectTool->click();
        check(selectTool->isChecked() && !textTool->isChecked() && !cancelTool->isVisible(),"select tool exits placement and hides cancellation");
        languageAction(window,"de")->trigger();
        auto *view=dynamic_cast<PageView *>(window.findChild<QWidget *>("pageView"));
        QImage mark(60,20,QImage::Format_ARGB32); mark.fill(Qt::black); view->placed(mark,QRectF(.2,.3,.2,.1)); app.processEvents();
        auto *bar=window.findChild<QToolBar *>("mainTools");
        for (const auto &name:QStringList{"pagesTool","editObjectTool","deleteObjectTool"}) {
            auto *button=window.findChild<QToolButton *>(name);
            check(button && button->isVisibleTo(bar) && bar->rect().contains(button->geometry()),"selection actions remain accessible at minimum width");
        }
        QSettings().sync();
        if (QFile::exists(QLibraryInfo::path(QLibraryInfo::TranslationsPath)+"/qtbase_de.qm")) {
            QDialogButtonBox buttons(QDialogButtonBox::Save|QDialogButtonBox::Cancel);
            check(buttons.button(QDialogButtonBox::Cancel)->text().contains("Abbrechen"),"Qt standard dialog buttons translated");
        }
        I18n::setLanguage("ko");
        themeAction(window,"dark")->trigger();
        Window restored;
        check(I18n::language()=="de" && languageAction(restored,"de")->isChecked(),"preference restored for a new window");
        check(themeAction(restored,"dark")->isChecked(),"theme preference restored for a new window");
        check(QApplication::palette().color(QPalette::Window)==QColor("#1f2125"),"restored dark palette applied");
        check(restored.menuBar()->actions().first()->text()=="&Datei","restored menu language");
        I18n::setLanguage("invalid"); check(I18n::language()=="en","unsupported language fallback");
        check(I18n::text("Untranslated diagnostic.")=="Untranslated diagnostic.","missing translation uses English source");
        Document empty;
        try { empty.save(config.filePath("invalid.pdf")); check(false,"empty save must fail"); }
        catch (const std::runtime_error &e) { check(QString::fromUtf8(e.what())=="Open a PDF first.","default error is English"); }
        std::cout << "PASS: three languages, light/dark themes, saved preferences, backend messages, and document state preservation\n";
        return 0;
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
