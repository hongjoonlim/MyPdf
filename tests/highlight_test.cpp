#define MYPDF_NO_MAIN
#include "../src/main.cpp"
#include <QPdfWriter>
#include <QTemporaryDir>
#include <poppler-qt6.h>
#include <limits>
#include <iostream>
namespace {
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
void mouse(QWidget *view,QEvent::Type type,QPointF point,Qt::MouseButton button,Qt::MouseButtons buttons,Qt::KeyboardModifiers modifiers=Qt::NoModifier) {
    QMouseEvent event(type,point,view->mapToGlobal(point.toPoint()),button,buttons,modifiers); QApplication::sendEvent(view,&event);
}
}
int main(int argc,char **argv) {
    QApplication app(argc,argv); QTemporaryDir dir;
    app.setOrganizationName("MyPdf"); app.setApplicationName("MyPdf");
    QSettings::setDefaultFormat(QSettings::IniFormat); QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,dir.path());
    try {
        const QString source=dir.filePath("source.pdf"),saved=dir.filePath("highlighted.pdf");
        { QPdfWriter writer(source); writer.setResolution(72); writer.setPageMargins(QMarginsF(0,0,0,0),QPageLayout::Point); QPainter painter(&writer); painter.drawText(120,254,"Original searchable sentence"); writer.newPage(); }
        Document doc; doc.open(source);
        const QPolygonF points{QPointF(.2,.3),QPointF(.5,.3),QPointF(.7,.3)};
        doc.addHighlight(0,points,QColor(255,235,59,90),14);
        const auto object=doc.objects(0).first();
        auto image=doc.render(0,72); const QColor mark=image.pixelColor(image.width()*.65,image.height()*.3);
        check(mark.red()>230 && mark.blue()<230,"transparent yellow stroke rendered in composed PDF");
        const QImage layer=doc.renderObject(object,144);
        check(layer.pixelColor(layer.width()/2,layer.height()/2).alpha()>0,"stroke appears in preview layer");
        doc.save(saved); Document reopened; reopened.open(saved);
        check(reopened.objects(0).size()==1 && reopened.objects(0).first().isHighlight(),"highlight restores after save/reopen");
        check(reopened.objects(0).first().highlight==object.highlight && reopened.objects(0).first().color==object.color,"stroke geometry and opacity preserved");
        auto pdf=Poppler::Document::load(saved);
        check(pdf->page(0)->text(QRectF()).contains("Original searchable sentence"),"highlight preserves searchable original text");
        const QImage stable=reopened.render(0,72);
        bool rejected=false; try { reopened.addHighlight(0,points,Qt::yellow,0); } catch (...) { rejected=true; }
        check(rejected && !reopened.dirty() && stable==reopened.render(0,72),"invalid stroke preserves document");
        rejected=false; try { reopened.addHighlight(0,{QPointF(-.1,.2),QPointF(.4,.2)},Qt::yellow,12); } catch (...) { rejected=true; }
        check(rejected && !reopened.dirty(),"out-of-page stroke rejected");
        reopened.updateHighlight(object.id,QColor(100,240,100,100),24);
        reopened.moveObject(object.id,QPointF(.2,.5));
        auto resized=reopened.objects(0).first().rect; resized.setSize(resized.size()*.8);
        reopened.resizeObject(object.id,resized);
        check(reopened.objects(0).first().strokeWidth<24,"highlight thickness follows resize");
        reopened.move(0,1); check(reopened.objects(1).first().isHighlight(),"highlight follows page reordering");
        const QString extracted=dir.filePath("extracted.pdf"); reopened.extract({1},extracted);
        Document split; split.open(extracted); check(split.objects(0).first().isHighlight(),"extracted highlight stays editable");
        split.removeObject(split.objects(0).first().id); check(split.objects(0).isEmpty(),"highlight can be deleted");

        Window window; window.openPath(source); window.resize(850,650); window.show(); app.processEvents();
        auto *bar=window.findChild<QToolBar *>("mainTools");
        auto *zoom=window.findChild<QWidget *>("zoomTools");
        check(zoom && bar->isAncestorOf(zoom) && !window.statusBar()->isAncestorOf(zoom),"zoom controls belong to main toolbar");
        for (auto *button:window.findChildren<QToolButton *>()) if (button->objectName()=="highlightTool") {
            check(button->isVisibleTo(bar),"highlight tool visible at minimum window width");
            check(button->popupMode()==QToolButton::InstantPopup,"highlight opens the same popup as other menu tools");
            auto *highlightAction=window.findChild<QAction *>("highlightAction");
            check(highlightAction!=nullptr,"highlight action exists");
            auto dismissPopup=[&] {
                bool opened=false;
                QTimer::singleShot(0,button->menu(),[&] {
                    opened=button->menu()->isVisible();
                    button->menu()->close();
                });
                // Let Qt handle the popup lifecycle without a synthetic release after its modal loop.
                button->showMenu();
                check(opened,"highlight popup opens before dismissal");
                check(!button->menu()->isVisible(),"highlight popup closes");
            };
            dismissPopup();
            check(!highlightAction->isChecked(),"dismissing the popup does not activate the pen action");
            check(!button->isChecked(),"dismissing the popup keeps the pen button inactive");
            check(button->menu()->actions().contains(window.findChild<QAction *>("highlightAction")),"highlight action is available in its popup");
            button->menu()->actions().first()->trigger();
            check(highlightAction->isChecked() && button->isChecked(),"choosing the pen synchronizes action and button");
            dismissPopup();
            check(highlightAction->isChecked() && button->isChecked(),"dismissing the popup preserves an active pen");
        }
        auto *pen=window.findChild<QAction *>("highlightAction"); check(pen && pen->isChecked(),"highlight tool activates");
        auto *view=dynamic_cast<PageView *>(window.findChild<QWidget *>("pageView"));
        int committed=0; const auto place=view->highlightPlaced;
        view->highlightPlaced=[&](QPolygonF stroke) { ++committed; place(stroke); };
        const QPointF start(view->width()*.2,view->height()*.3),finish(view->width()*.7,view->height()*.32);
        mouse(view,QEvent::MouseButtonPress,start,Qt::LeftButton,Qt::LeftButton);
        mouse(view,QEvent::MouseMove,finish,Qt::NoButton,Qt::LeftButton,Qt::ShiftModifier);
        mouse(view,QEvent::MouseButtonRelease,finish,Qt::LeftButton,Qt::NoButton,Qt::ShiftModifier);
        check(pen->isChecked(),"pen remains active for another stroke");
        check(committed==1,"one stroke committed on release");
        if (const auto screenshot=qEnvironmentVariable("MYPDF_HIGHLIGHT_SCREENSHOT"); !screenshot.isEmpty()) window.grab().save(screenshot);
        mouse(view,QEvent::MouseButtonPress,start,Qt::LeftButton,Qt::LeftButton);
        mouse(view,QEvent::MouseMove,finish,Qt::NoButton,Qt::LeftButton);
        QKeyEvent escape(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier); QApplication::sendEvent(view,&escape);
        check(!pen->isChecked(),"Esc exits highlighter mode");
        mouse(view,QEvent::MouseButtonRelease,finish,Qt::LeftButton,Qt::NoButton);
        check(committed==1,"Esc discards the in-progress stroke");
        // Switch to selection and double-click the stroke to edit its style.
        QTimer::singleShot(0,[&] {
            auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); check(dialog!=nullptr,"highlight style editor opens");
            dialog->findChild<QDoubleSpinBox *>("highlightWidth")->setValue(20);
            dialog->findChild<QSpinBox *>("highlightOpacity")->setValue(45);
            dialog->accept();
        });
        mouse(view,QEvent::MouseButtonDblClick,(start+QPointF(finish.x(),start.y()))/2,Qt::LeftButton,Qt::LeftButton);
        const QString selected=view->selectedObject(); check(!selected.isEmpty(),"highlight remains selected after editing");
        QKeyEvent remove(QEvent::KeyPress,Qt::Key_Delete,Qt::NoModifier); QApplication::sendEvent(view,&remove);
        check(view->selectedObject().isEmpty(),"Delete removes selected highlight");
        for (const auto &language:QStringList{"en","de","ko"}) {
            window.findChild<QAction *>("language_"+language)->trigger(); app.processEvents();
            check(zoom->isVisibleTo(bar) && bar->rect().contains(zoom->geometry()),"top zoom controls visible in every language");
        }
        std::cout << "PASS: highlight rendering, persistence, editing, page operations, pen interaction and top zoom toolbar\n";
        return 0;
    } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
