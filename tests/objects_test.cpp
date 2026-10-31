#define MYPDF_NO_MAIN
#include "../src/main.cpp"
#include <QTemporaryDir>
#include <QPdfWriter>
#include <QProcess>
#include <poppler-qt6.h>
#include <iostream>
#include <cstdlib>
namespace {
void check(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
void mouse(PageView *view,QEvent::Type type,QPointF point,Qt::MouseButton button,Qt::MouseButtons buttons) {
    QMouseEvent event(type,point,view->mapToGlobal(point.toPoint()),button,buttons,Qt::NoModifier); QApplication::sendEvent(view,&event);
}
void key(PageView *view,int code,Qt::KeyboardModifiers modifiers=Qt::NoModifier) { QKeyEvent event(QEvent::KeyPress,code,modifiers); QApplication::sendEvent(view,&event); }
}
int main(int argc,char **argv) {
    QApplication app(argc,argv); QTemporaryDir dir;
    app.setOrganizationName("MyPdf"); app.setApplicationName("MyPdf");
    QSettings::setDefaultFormat(QSettings::IniFormat); QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,dir.path());
    // No message boxes are expected in this test. Report GUI failures instead of
    // waiting for the CTest timeout inside a modal event loop.
    QTimer messageGuard;
    QObject::connect(&messageGuard,&QTimer::timeout,[] {
        if (auto *message=qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
            std::cerr << "FAIL: unexpected message box: " << message->windowTitle().toStdString()
                      << " | " << message->text().toStdString()
                      << " | " << message->informativeText().toStdString()
                      << " | " << message->detailedText().toStdString() << std::endl;
            std::_Exit(EXIT_FAILURE);
        }
    });
    messageGuard.start(50);
    try {
        const QString source=dir.filePath("original.pdf");
        { QPdfWriter writer(source); writer.setResolution(72); QPainter p(&writer); p.drawText(50,60,"Original searchable text"); writer.newPage(); }
        Document doc; doc.open(source); const QImage original=doc.render(0,72);
        const QFont font("Sans Serif",18);
        const QString words=QString::fromUtf16(u"Editable text \ud55c\uae00");
        doc.addText(0,words,font,Qt::blue,QPointF(.1,.2));
        const QString textId=doc.objects(0).last().id;
        QImage ink(60,20,QImage::Format_ARGB32); ink.fill(Qt::black);
        doc.sign(0,ink,QRectF(.1,.4,.2,.1)); const QString signatureId=doc.objects(0).last().id;
        doc.moveObject(textId,QPointF(.3,.3)); doc.moveObject(signatureId,QPointF(.5,.5));
        check(doc.objects(0).first().rect.topLeft()==QPointF(.3,.3),"text moves independently");
        auto image=doc.render(0,72); check(image.pixelColor(image.width()*.6,image.height()*.55).value()<30,"signature appears at moved position");
        check(image.pixelColor(image.width()*.2,image.height()*.45).value()>240,"signature leaves no old mark");
        const QString saved=dir.filePath("editable.pdf"); doc.save(saved);
        const QString statePath=dir.filePath("legacy-state.json"), legacyPath=dir.filePath("legacy.pdf");
        QByteArray state;
        // Direct Poppler readers retain Windows file handles. Destroy each reader
        // after verification, before any later atomic replacement of the PDF.
        {
            auto modern=Poppler::Document::load(saved); check(bool(modern),"open saved editing attachment");
            for (auto *file:modern->embeddedFiles()) if (file->name()=="org.MyPdf.edit-state.v1.json") state=file->data();
        }
        check(!state.isEmpty(),"new branding uses the renamed editing attachment");
        { QFile file(statePath); check(file.open(QIODevice::WriteOnly) && file.write(state)==state.size(),"write legacy state fixture"); }
        QProcess legacyTool;
        legacyTool.start(qEnvironmentVariable("MYPDF_QPDF","qpdf"),{saved,"--remove-attachment=org.MyPdf.edit-state.v1.json","--add-attachment",statePath,"--key=org.mypdf.edit-state.v1.json","--filename=org.mypdf.edit-state.v1.json","--",legacyPath});
        check(legacyTool.waitForFinished() && legacyTool.exitCode()==0,"create legacy branding fixture");
        Document legacy; legacy.open(legacyPath); check(legacy.objects(0).size()==2,"legacy PDF editing data remains readable");
        const QString upgradedPath=dir.filePath("upgraded.pdf"); legacy.save(upgradedPath);
        {
            auto upgraded=Poppler::Document::load(upgradedPath); check(bool(upgraded),"open migrated editing attachment");
            for (auto *file:upgraded->embeddedFiles()) check(file->name()!="org.mypdf.edit-state.v1.json","saving migrates legacy editing metadata");
        }
        Document migrated; migrated.open(upgradedPath); check(migrated.objects(0).size()==2,"migrated objects remain editable");
        Document reopened; reopened.open(saved); check(reopened.objects(0).size()==2 && !reopened.dirty(),"saved objects restored cleanly");
        {
            auto pdf=Poppler::Document::load(saved); check(bool(pdf),"open saved text verification");
            const auto extractedText=pdf->page(0)->text(QRectF());
            check(extractedText.contains("Editable text") && extractedText.contains("Original searchable text"),"flattened appearance preserves searchable text");
        }
        reopened.moveObject(textId,QPointF(2,2));
        check(reopened.objects(0).first().rect.right()<=1.000001 && reopened.objects(0).first().rect.bottom()<=1.000001,"movement clamps to page");
        const auto beforeFailure=reopened.render(0,72); const auto previousTool=qgetenv("MYPDF_QPDF");
        qputenv("MYPDF_QPDF",dir.filePath("missing-qpdf").toUtf8()); bool rejected=false;
        try { reopened.moveObject(textId,QPointF(.1,.1)); } catch (...) { rejected=true; }
        qputenv("MYPDF_QPDF",previousTool);
        check(rejected && reopened.render(0,72)==beforeFailure,"failed move preserves appearance and model");
        reopened.move(0,1); check(reopened.objects(0).isEmpty() && reopened.objects(1).size()==2,"objects follow reordered page");
        reopened.insertBlank(0); check(reopened.objects(2).size()==2,"objects follow insertion");
        const QString split=dir.filePath("extracted.pdf"); reopened.extract({2},split);
        Document extracted; extracted.open(split); check(extracted.objects(0).size()==2,"extracted objects remain editable");
        const auto copies=extracted.objects(0); for (const auto &object:copies) extracted.removeObject(object.id);
        check(extracted.render(0,72)==original,"delete all objects restores original content exactly");
        // Extraction gives independent copies new IDs.

        const QString removed=dir.filePath("removed.pdf"); extracted.save(removed); Document empty; empty.open(removed);
        check(empty.objects(0).isEmpty(),"deleted objects stay deleted after reopening");
        Document merged; merged.open(source); merged.insertPdf(saved,1); check(merged.objects(1).size()==2,"inserted editable PDF retains its objects");
        Window window; window.show(); window.openPath(saved); app.processEvents();
        auto *view=dynamic_cast<PageView *>(window.findChild<QWidget *>("pageView")); check(view!=nullptr,"page view available");
        auto *editTool=window.findChild<QToolButton *>("editObjectTool");
        auto *deleteTool=window.findChild<QToolButton *>("deleteObjectTool");
        check(editTool && deleteTool && !editTool->isVisible() && !deleteTool->isVisible(),"selection tools are hidden until an object is selected");
        QPointF center(.6*view->width(),.55*view->height());
        mouse(view,QEvent::MouseButtonPress,center,Qt::LeftButton,Qt::LeftButton);
        mouse(view,QEvent::MouseButtonRelease,center,Qt::LeftButton,Qt::NoButton);
        check(view->selectedObject()==signatureId,"click selects saved signature");
        app.processEvents();
        check(editTool->isVisible() && deleteTool->isVisible() && editTool->isEnabled() && deleteTool->isEnabled(),"selection reveals edit and delete tools");
        QTimer::singleShot(0,[&] {
            auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());
            check(menu && menu->actions().size()==2 && menu->actions().first()->isEnabled(),"right-click offers only relevant object actions"); menu->close();
        });
        mouse(view,QEvent::MouseButtonPress,center,Qt::RightButton,Qt::RightButton);
        mouse(view,QEvent::MouseButtonRelease,center,Qt::RightButton,Qt::NoButton);
        key(view,Qt::Key_Right); check(view->selectedObject()==signatureId && window.windowTitle().contains('*'),"arrow moves and retains selection");
        auto saveWindow=[&] { for (auto *action:window.findChildren<QAction *>()) if (action->text()==I18n::text("Save")) { action->trigger(); return; } throw std::runtime_error("save action missing"); };
        saveWindow(); Document keyboardMoved; keyboardMoved.open(saved);
        check(qAbs(keyboardMoved.objects(0).last().rect.x()-(.5+1/keyboardMoved.size(0).width()))<.00001,"arrow movement is one PDF point");
        const QPointF destination=center+QPointF(25,30);
        mouse(view,QEvent::MouseButtonPress,center,Qt::LeftButton,Qt::LeftButton);
        mouse(view,QEvent::MouseMove,destination,Qt::NoButton,Qt::LeftButton);
        const auto live=view->grab().toImage();
        const auto signatureRect=keyboardMoved.objects(0).last().rect;
        const qreal dpr=view->devicePixelRatioF();
        const QPointF cleared(signatureRect.center().x()*view->width(),signatureRect.top()*view->height()+8);
        const QPointF movingInk(signatureRect.center().x()*view->width()+25,signatureRect.bottom()*view->height()+15);
        check(live.pixelColor((cleared*dpr).toPoint()).value()>240,"drag preview clears the previous signature position before release");
        check(live.pixelColor((movingInk*dpr).toPoint()).value()<30,"signature content follows pointer before release");
        check(!window.windowTitle().contains('*'),"live movement does not commit before release");
        mouse(view,QEvent::MouseButtonRelease,destination,Qt::LeftButton,Qt::NoButton);
        check(view->selectedObject()==signatureId,"drag retains object selection");
        saveWindow(); Document dragged; dragged.open(saved);
        check(qAbs(dragged.objects(0).last().rect.x()-(keyboardMoved.objects(0).last().rect.x()+25./view->width()))<.00001,"drag commits mouse displacement");

        const QRectF dragRect=dragged.objects(0).last().rect;
        const QPointF handle(dragRect.right()*view->width(),dragRect.bottom()*view->height());
        const QPointF larger=handle+QPointF(40,20);
        mouse(view,QEvent::MouseButtonPress,handle,Qt::LeftButton,Qt::LeftButton);
        mouse(view,QEvent::MouseMove,larger,Qt::NoButton,Qt::LeftButton);
        const auto resizePreview=view->grab().toImage();
        const QPointF expandedInk(dragRect.right()*view->width()+8,dragRect.center().y()*view->height());
        check(resizePreview.pixelColor((expandedInk*dpr).toPoint()).value()<30,"signature expands live before release");
        mouse(view,QEvent::MouseButtonRelease,larger,Qt::LeftButton,Qt::NoButton);
        saveWindow(); Document resized; resized.open(saved);
        const QRectF resizedRect=resized.objects(0).last().rect;
        check(resizedRect.width()>dragRect.width() && qAbs(resizedRect.width()/resizedRect.height()-dragRect.width()/dragRect.height())<.00001,"corner drag enlarges signature proportionally and persists");
        const auto stableResize=resized.render(0,72); bool invalidResize=false;
        try { resized.resizeObject(signatureId,QRectF(-.1,0,.2,.2)); } catch (...) { invalidResize=true; }
        check(invalidResize && stableResize==resized.render(0,72) && !resized.dirty(),"invalid resize is atomic");
        const QPointF topLeft(resizedRect.left()*view->width(),resizedRect.top()*view->height());
        mouse(view,QEvent::MouseButtonPress,topLeft,Qt::LeftButton,Qt::LeftButton);
        mouse(view,QEvent::MouseMove,topLeft+QPointF(15,10),Qt::NoButton,Qt::LeftButton);
        key(view,Qt::Key_Escape);
        mouse(view,QEvent::MouseButtonRelease,topLeft+QPointF(15,10),Qt::LeftButton,Qt::NoButton);
        check(!window.windowTitle().contains('*') && view->selectedObject().isEmpty(),"Escape cancels resize preview without committing");
        const QPointF resizedCenter(resizedRect.center().x()*view->width(),resizedRect.center().y()*view->height());
        mouse(view,QEvent::MouseButtonPress,resizedCenter,Qt::LeftButton,Qt::LeftButton);
        mouse(view,QEvent::MouseButtonRelease,resizedCenter,Qt::LeftButton,Qt::NoButton);
        key(view,Qt::Key_Delete); check(view->selectedObject().isEmpty(),"Delete removes selection");
        app.processEvents();
        check(!editTool->isVisible() && !deleteTool->isVisible(),"selection tools hide again after deletion");
        // Select the text from the saved document and delete it via Backspace.
        const QPointF textCenter(.31*view->width(),.31*view->height());
        mouse(view,QEvent::MouseButtonPress,textCenter,Qt::LeftButton,Qt::LeftButton);
        mouse(view,QEvent::MouseButtonRelease,textCenter,Qt::LeftButton,Qt::NoButton);
        check(view->selectedObject()==textId,"click selects saved text");
        const auto oldText=dragged.objects(0).first();
        const QPointF textHandle(oldText.rect.right()*view->width(),oldText.rect.bottom()*view->height());
        const QPointF smaller=textHandle-QPointF(25,5);
        mouse(view,QEvent::MouseButtonPress,textHandle,Qt::LeftButton,Qt::LeftButton);
        mouse(view,QEvent::MouseMove,smaller,Qt::NoButton,Qt::LeftButton);
        mouse(view,QEvent::MouseButtonRelease,smaller,Qt::LeftButton,Qt::NoButton);
        saveWindow(); Document resizedText; resizedText.open(saved);
        check(resizedText.objects(0).first().rect.width()<oldText.rect.width(),"text can be shrunk with corner handle");
        check(resizedText.objects(0).first().textSize==oldText.textSize,"text scaling preserves original line layout");
        {
            auto resizedPdf=Poppler::Document::load(saved); check(bool(resizedPdf),"open resized text verification");
            check(resizedPdf->page(0)->text(QRectF()).contains("Editable text"),"resized text remains searchable after save");
        }
        key(view,Qt::Key_Down,Qt::ShiftModifier); key(view,Qt::Key_Backspace);
        check(view->selectedObject().isEmpty(),"Backspace removes text");
        saveWindow(); Document deleted; deleted.open(saved);
        check(deleted.objects(0).isEmpty() && deleted.render(0,72)==original,"GUI deletion persists without damaging original page");
        std::cout << "PASS: editable text/signatures, persistence, independent movement/deletion, page operations, failure atomicity and mouse/keyboard interaction\n";
        return 0;
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
