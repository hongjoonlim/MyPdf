#define MYPDF_NO_MAIN
#include "../src/main.cpp"
#include "file_dialog_automation.h"
#include <QPdfWriter>
#include <QEventLoop>
#include <QTemporaryDir>
#include <poppler-qt6.h>
#include <iostream>
namespace {
void check(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
void mouse(QWidget *view,QEvent::Type type,QPointF point,Qt::MouseButton button,Qt::MouseButtons buttons) {
    QMouseEvent event(type,point,view->mapToGlobal(point.toPoint()),button,buttons,Qt::NoModifier); QApplication::sendEvent(view,&event);
}
void doubleClick(PageView *view,QPointF point,const std::function<void(QDialog *)> &edit) {
    mouse(view,QEvent::MouseButtonPress,point,Qt::LeftButton,Qt::LeftButton);
    mouse(view,QEvent::MouseButtonRelease,point,Qt::LeftButton,Qt::NoButton);
    QTimer::singleShot(0,[edit] {
        auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); check(dialog!=nullptr,"double click opens editor"); edit(dialog);
    });
    mouse(view,QEvent::MouseButtonDblClick,point,Qt::LeftButton,Qt::LeftButton);
    mouse(view,QEvent::MouseButtonRelease,point,Qt::LeftButton,Qt::NoButton);
}
void saveWindow(Window &window) { for (auto *action:window.findChildren<QAction *>()) if (action->text()==I18n::text("Save")) { action->trigger(); return; } throw std::runtime_error("save action missing"); }
}
int main(int argc,char **argv) {
    QApplication app(argc,argv); app.setOrganizationName("MyPdf"); app.setApplicationName("MyPdf"); QTemporaryDir dir;
    QSettings::setDefaultFormat(QSettings::IniFormat); QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,dir.path());
    try {
        const QString source=dir.filePath("source.pdf"),saved=dir.filePath("editable.pdf");
        { QPdfWriter writer(source); writer.setResolution(72); QPainter painter(&writer); painter.drawText(30,40,"Original text"); }
        Document document; document.open(source); document.addText(0,"Editable text",QFont("Sans Serif",18),Qt::blue,QPointF(.1,.2));
        const auto text=document.objects(0).first();
        document.resizeObject(text.id,QRectF(text.rect.topLeft(),text.rect.size()*1.5));
        QImage ink(100,30,QImage::Format_ARGB32); ink.fill(Qt::black); document.sign(0,ink,QRectF(.1,.5,.3,.1));
        const auto signature=document.objects(0).last(); document.save(saved);
        const auto before=document.render(0,72); bool rejected=false;
        try { document.updateText(text.id,"",QFont("Sans Serif",12),Qt::red); } catch (...) { rejected=true; }
        check(rejected && document.render(0,72)==before && !document.dirty(),"invalid text edit preserves document");
        rejected=false; try { document.updateSignature(signature.id,QImage()); } catch (...) { rejected=true; }
        check(rejected && document.render(0,72)==before && !document.dirty(),"invalid signature edit preserves document");
        Window window; window.show(); window.openPath(saved); app.processEvents();
        auto *view=dynamic_cast<PageView *>(window.findChild<QWidget *>("pageView")); check(view!=nullptr,"page view exists");
        const QPointF textPoint(.15*view->width(),.21*view->height());
        doubleClick(view,textPoint,[&](QDialog *dialog) {
            auto *input=dialog->findChild<QTextEdit *>("insertTextInput"); auto *size=dialog->findChild<QDoubleSpinBox *>("textSize");
            check(input && input->toPlainText()=="Editable text","existing text is prefilled");
            check(size && qAbs(size->value()-27)<.02,"editor shows resized effective font size");
            input->setPlainText("Canceled edit"); dialog->reject();
        });
        check(!window.windowTitle().contains('*') && view->selectedObject()==text.id,"cancel edit preserves clean state and selection");
        doubleClick(view,textPoint,[](QDialog *dialog) { dialog->accept(); });
        check(!window.windowTitle().contains('*'),"unchanged text confirmation is a no-op");
        doubleClick(view,textPoint,[](QDialog *dialog) {
            dialog->findChild<QTextEdit *>("insertTextInput")->setPlainText("Changed text");
            dialog->findChild<QDoubleSpinBox *>("textSize")->setValue(20); dialog->accept();
        });
        check(window.windowTitle().contains('*') && view->selectedObject()==text.id,"text edit updates selected object");
        saveWindow(window); Document edited; edited.open(saved);
        check(edited.objects(0).size()==2 && edited.objects(0).first().id==text.id && edited.objects(0).first().text=="Changed text","text edit persists without duplication");
        check(edited.objects(0).first().rect.topLeft()==text.rect.topLeft() && edited.objects(0).first().font.pointSizeF()==20,"text edit retains position and updates font size");
        const QPointF signaturePoint(.25*view->width(),.55*view->height());
        doubleClick(view,signaturePoint,[](QDialog *dialog) {
            auto *pad=dynamic_cast<InkPad *>(dialog->findChild<QWidget *>("signatureEditor")); check(pad && !pad->image().isNull(),"existing signature is shown");
            pad->clear(); dialog->reject();
        });
        check(!window.windowTitle().contains('*'),"cancel signature edit preserves clean state");
        doubleClick(view,signaturePoint,[](QDialog *dialog) {
            auto *pad=dynamic_cast<InkPad *>(dialog->findChild<QWidget *>("signatureEditor")); pad->clear();
            mouse(pad,QEvent::MouseButtonPress,QPointF(30,40),Qt::LeftButton,Qt::LeftButton);
            mouse(pad,QEvent::MouseMove,QPointF(250,80),Qt::NoButton,Qt::LeftButton);
            mouse(pad,QEvent::MouseButtonRelease,QPointF(250,80),Qt::LeftButton,Qt::NoButton); dialog->accept();
        });
        check(window.windowTitle().contains('*') && view->selectedObject()==signature.id,"signature redraw updates the selected object");
        saveWindow(window); Document redrawn; redrawn.open(saved);
        check(redrawn.objects(0).last().rect==signature.rect && redrawn.objects(0).last().image!=ink,"redrawn signature persists and retains geometry");
        QImage replacement(60,20,QImage::Format_ARGB32); replacement.fill(Qt::red); const QString png=dir.filePath("replacement.png"); replacement.save(png);
        // Exercise a chooser that appears later, not in the first event-loop tick.
        QEventLoop waitForChooser; QString chosen;
        {
            FileDialogAutomation chooser(png);
            QTimer::singleShot(200,&waitForChooser,[&] {
                QFileDialog file(&window,"Delayed signature image"); Icons::configureFileDialog(file);
                file.setFileMode(QFileDialog::ExistingFile);
                QTimer::singleShot(0,&file,[&file] { file.findChild<QLineEdit *>("fileNameEdit")->setFocus(); });
                if (file.exec()==QDialog::Accepted) chosen=file.selectedFiles().value(0);
                waitForChooser.quit();
            });
            waitForChooser.exec();
            check(chooser.completed() && chosen==png,"delayed file chooser is automatically accepted");
        }
        doubleClick(view,signaturePoint,[&](QDialog *dialog) {
            FileDialogAutomation chooser(png);
            dialog->findChild<QPushButton *>("replaceSignature")->click();
            check(chooser.completed(),"replacement image is selected after the file model becomes ready");
            dialog->accept();
        });
        saveWindow(window); Document replaced; replaced.open(saved);
        check(replaced.objects(0).size()==2 && replaced.objects(0).last().id==signature.id && replaced.objects(0).last().rect==signature.rect,"replacement retains object identity and geometry");
        check(replaced.objects(0).last().image.pixelColor(10,10)==QColor(Qt::red),"signature replacement persists");
        {
            FileDialogAutomation chooser(png);
            for (auto *action:window.findChildren<QAction *>()) if (action->text()==I18n::text("Import Signature Image…")) { action->trigger(); break; }
            check(chooser.completed() && view->hasSignature(),"signature image import does not wait for user input");
            view->setSignature({});
        }
        auto result=Poppler::Document::load(saved); const QString searchable=result->page(0)->text(QRectF());
        check(searchable.contains("Changed text") && searchable.contains("Original text") && !searchable.contains("Editable text"),"edited PDF text stays searchable without the old text");
        const auto current=replaced.render(0,72); const auto tool=qgetenv("MYPDF_QPDF"); qputenv("MYPDF_QPDF",dir.filePath("missing-qpdf").toUtf8());
        rejected=false; try { replaced.updateSignature(signature.id,ink); } catch (...) { rejected=true; } qputenv("MYPDF_QPDF",tool);
        check(rejected && !replaced.dirty() && replaced.render(0,72)==current,"failed replacement preserves document");
        std::cout << "PASS: double-click text/signature editing, cancel/no-op, resized font, redraw/image replacement, persistence and failure atomicity\n";
        return 0;
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
