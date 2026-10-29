#define MYPDF_NO_MAIN
#include "../src/main.cpp"
#include <QPdfWriter>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
void mouse(PageView *view,QEvent::Type type,QPointF pos,Qt::MouseButton button,Qt::MouseButtons buttons) {
    QMouseEvent event(type,pos,view->mapToGlobal(pos.toPoint()),button,buttons,Qt::NoModifier);
    QApplication::sendEvent(view,&event);
}
QAction *action(Window &window,const QString &source) {
    for (auto *a:window.findChildren<QAction *>()) if (a->text()==I18n::text(source)) return a;
    throw std::runtime_error("Action not found");
}
}
int main(int argc,char **argv) {
    QApplication app(argc,argv); app.setOrganizationName("MyPdf"); app.setApplicationName("MyPdf");
    QTemporaryDir dir;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,dir.path());
    try {
        QString path=dir.filePath("blank.pdf");
        { QPdfWriter writer(path); QPainter painter(&writer); painter.fillRect(0,0,200,200,Qt::white); writer.newPage(); }
        QImage ink(100,40,QImage::Format_ARGB32); ink.fill(Qt::black);
        Document doc; doc.open(path); auto original=doc.render(0,72);
        check(!doc.canCancelSignature(),"no signature to cancel initially");
        doc.sign(0,ink,QRectF(.1,.1,.2,.1));
        check(doc.canCancelSignature() && doc.dirty(),"placed signature is cancelable");
        doc.cancelSignature();
        check(doc.render(0,72)==original && !doc.dirty(),"cancel restores page and clean state");
        doc.move(0,1); doc.sign(0,ink,QRectF(.1,.1,.2,.1)); doc.cancelSignature();
        check(doc.dirty() && doc.count()==2,"cancel preserves earlier unsaved edits");
        doc.sign(0,ink,QRectF(.1,.1,.2,.1));
        try { doc.move(-1,0); } catch (const std::exception &) {}
        check(doc.canCancelSignature(),"failed edit preserves cancellation");
        doc.save(dir.filePath("saved.pdf")); auto saved=doc.render(0,72); doc.cancelSignature();
        check(!doc.canCancelSignature() && !doc.dirty() && doc.render(0,72)==saved,"saved signature cannot be canceled");
        doc.sign(0,ink,QRectF(.4,.4,.2,.1)); doc.insertBlank(0);
        check(!doc.canCancelSignature() && doc.count()==3,"later page edit invalidates cancellation");
        doc.sign(0,ink,QRectF(.4,.4,.2,.1)); doc.open(path);
        check(!doc.canCancelSignature(),"opening another document clears cancellation");

        PageView scaledView;
        QImage highResolution(800,1000,QImage::Format_RGB32); highResolution.fill(Qt::white);
        highResolution.setDevicePixelRatio(2);
        scaledView.setPage(highResolution);
        check(scaledView.size()==QSize(400,500),"high-resolution rendering retains logical page size");
        QRectF placedRect;
        scaledView.placed=[&](const QImage &,const QRectF &rect) { placedRect=rect; };
        scaledView.setSignature(ink);
        mouse(&scaledView,QEvent::MouseButtonPress,QPointF(40,50),Qt::LeftButton,Qt::LeftButton);
        mouse(&scaledView,QEvent::MouseMove,QPointF(120,150),Qt::NoButton,Qt::LeftButton);
        mouse(&scaledView,QEvent::MouseButtonRelease,QPointF(120,150),Qt::LeftButton,Qt::NoButton);
        check(placedRect==QRectF(.1,.1,.2,.2),"signature coordinates remain independent of render resolution");

        Window window; window.show(); window.openPath(path); app.processEvents();
        auto *view=dynamic_cast<PageView *>(window.findChild<QWidget *>("pageView"));
        check(view!=nullptr,"page view exists");
        auto *cancel=action(window,"Cancel Signature Placement");
        check(!cancel->isEnabled(),"cancel disabled when there is no placement");
        QPointF from(view->width()*.1,view->height()*.1),to(view->width()*.3,view->height()*.2);
        QPoint center=((from+to)/2).toPoint();
        auto begin=[&] {
            view->setSignature(ink); check(cancel->isEnabled(),"cancel enabled while armed");
            mouse(view,QEvent::MouseButtonPress,from,Qt::LeftButton,Qt::LeftButton);
            mouse(view,QEvent::MouseMove,to,Qt::NoButton,Qt::LeftButton);
        };
        begin(); cancel->trigger(); mouse(view,QEvent::MouseButtonRelease,to,Qt::LeftButton,Qt::NoButton);
        check(!window.windowTitle().contains('*') && !cancel->isEnabled() && !view->hasSignature(),"cancel during drag does not commit on mouse release");
        begin(); mouse(view,QEvent::MouseButtonRelease,to,Qt::LeftButton,Qt::NoButton);
        check(window.windowTitle().contains('*') && !cancel->isEnabled() && !view->selectedObject().isEmpty(),"placed signature becomes a selected object");
        check(view->grab().toImage().pixelColor((QPointF(center)*view->devicePixelRatioF()).toPoint()).value()<30,"placed signature is visible");
        action(window,"Delete Selected Object")->trigger();
        check(view->grab().toImage().pixelColor((QPointF(center)*view->devicePixelRatioF()).toPoint()).value()>240,"cancel removes placed signature from canvas");
        check(window.windowTitle().contains('*') && !cancel->isEnabled(),"deletion marks document dirty and disables placement cancel");
        begin(); mouse(view,QEvent::MouseButtonRelease,to,Qt::LeftButton,Qt::NoButton);
        action(window,"Move Later")->trigger();
        check(!cancel->isEnabled(),"page editing disables cancellation without reverting edits");
        auto *textAction=action(window,"Add Text…");
        textAction->trigger();
        QKeyEvent escape(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier);
        QApplication::sendEvent(view,&escape);
        check(!textAction->isChecked(),"Escape cancels text placement");
        textAction->trigger();
        QTimer::singleShot(0,[&] {
            auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());
            check(dialog!=nullptr,"text dialog opens on page click");
            dialog->reject();
        });
        mouse(view,QEvent::MouseButtonPress,from,Qt::LeftButton,Qt::LeftButton);
        check(textAction->isChecked(),"cancel dialog permits choosing another position");
        QTimer::singleShot(0,[&] {
            auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());
            auto *input=dialog->findChild<QTextEdit *>("insertTextInput");
            check(input!=nullptr,"text input exists"); input->setPlainText("GUI inserted text"); dialog->accept();
        });
        mouse(view,QEvent::MouseButtonPress,from,Qt::LeftButton,Qt::LeftButton);
        check(!textAction->isChecked() && window.windowTitle().contains('*'),"text commit exits placement and marks dirty");
        std::cout << "PASS: signature cancellation before/during/after placement, dirty state, save and edit boundaries\n";
        return 0;
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
