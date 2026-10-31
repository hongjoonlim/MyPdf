#define MYPDF_NO_MAIN
#include "../src/main.cpp"
#include "form_fixture.h"
#include <QTemporaryDir>
#include <poppler-qt6.h>
#include <poppler-form.h>
#include <iostream>
#include <QTextBrowser>
namespace {
void check(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
PdfFormField field(const Document &doc,const QString &name,int occurrence=0,int page=0) {
    for (const auto &item:doc.formFields(page)) if (item.group==name && occurrence--==0) return item;
    throw std::runtime_error("Missing fixture field");
}
PdfFormValue value(const PdfFormField &field) { return {field.id,field.text,field.checked,field.selected}; }
}
int main(int argc,char **argv) {
    QApplication app(argc,argv); app.setOrganizationName("MyPdf"); app.setApplicationName("MyPdf"); QTemporaryDir temp;
    QSettings::setDefaultFormat(QSettings::IniFormat); QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,temp.path());
    try {
        const auto source=temp.filePath("forms.pdf"),saved=temp.filePath("filled.pdf"); writeFormFixture(source);
        Document doc; doc.open(source); check(doc.formFields(0).size()==8,"all supported widgets discovered");
        check(doc.formFields(1).isEmpty(),"ordinary page has no widgets");
        const auto before=doc.renderBase(0,72);
        auto name=value(field(doc,"name")); name.text=QString::fromUtf8("Test 한글 ä");
        auto agree=value(field(doc,"agree")); agree.checked=true;
        auto radio=value(field(doc,"delivery",1)); radio.checked=true;
        auto country=value(field(doc,"country")); country.selected={1};
        auto notes=value(field(doc,"notes")); notes.text="First line\nSecond line";
        auto custom=value(field(doc,"custom")); custom.selected.clear(); custom.text="Custom value";
        doc.fillForm(0,{name,agree,radio,country,notes,custom});
        check(doc.dirty() && doc.renderBase(0,72)!=before,"form changes update rendering and dirty state");
        check(!field(doc,"delivery",0).checked && field(doc,"delivery",1).checked,"radio group selects exactly one option");
        doc.save(saved);
        { Document plain; plain.open(saved); check(field(plain,"name").text==name.text && field(plain,"agree").checked,"form-only PDF saves without editing metadata"); }
        QTimer::singleShot(0,[&] {
            if (auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget())) {
                if (auto *input=dialog->findChild<QLineEdit *>("formField_"+QString::number(name.id))) input->setText("Discard this input");
                dialog->reject();
            }
        });
        check(!Forms::edit(doc,0,nullptr) && !doc.dirty() && field(doc,"name").text==name.text,"Cancel discards edited form values without dirtying document");
        doc.addText(0,"Overlay survives",QFont("Sans",12),Qt::black,QPointF(.6,.8));
        doc.save(saved); Document reopened; reopened.open(saved);
        check(field(reopened,"name").text==name.text,"Unicode form value survives save and reopen");
        check(field(reopened,"agree").checked && field(reopened,"country").selected==QList<int>{1},"checkbox and dropdown persist");
        check(field(reopened,"custom").text==custom.text && field(reopened,"notes").text==notes.text,"custom choice and multiline persist");
        check(reopened.objects(0).size()==1,"form and editable overlay coexist");
        // Inspect actual PDF fields, independently of MyPdf editing metadata.
        { auto pdf=Poppler::Document::load(saved); bool found=false;
          for (const auto &item:pdf->page(0)->formFields()) if (item->fullyQualifiedName()=="name") {
              found=static_cast<Poppler::FormFieldText *>(item.get())->text()==name.text;
          } check(found,"values are stored in standard AcroForm fields"); }
        auto protectedValue=value(field(reopened,"locked")); protectedValue.text="Forbidden";
        auto changed=value(field(reopened,"name")); changed.text="Should roll back";
        bool rejected=false; try { reopened.fillForm(0,{changed,protectedValue}); } catch (...) { rejected=true; }
        check(rejected && !reopened.dirty() && field(reopened,"name").text==name.text,"read-only failure rolls back entire batch");
        auto invalidChoice=value(field(reopened,"country")); invalidChoice.selected={99};
        rejected=false; try { reopened.fillForm(0,{invalidChoice}); } catch (...) { rejected=true; }
        check(rejected && !reopened.dirty(),"invalid choices cannot change document");
        agree=value(field(reopened,"agree")); agree.checked=false; reopened.fillForm(0,{agree}); reopened.save(saved);
        Document again; again.open(saved); check(!field(again,"agree").checked,"saved forms remain editable");
        again.move(0,1); check(field(again,"name",0,1).text==name.text,"values follow reordered page");
        const auto extracted=temp.filePath("extracted.pdf"); again.extract({1},extracted);
        Document split; split.open(extracted); check(field(split,"country").selected==QList<int>{1},"extracted forms remain interactive");

        Window window; window.openPath(source); window.show(); app.processEvents();
        auto *command=window.findChild<QAction *>("fillFormAction"); check(command && command->isEnabled(),"form command is available");
        bool inspected=false;
        QTimer::singleShot(0,[&] {
            auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if (!dialog) return;
            auto *input=dialog->findChild<QLineEdit *>("formField_"+QString::number(name.id));
            auto *locked=dialog->findChild<QWidget *>("formField_"+QString::number(protectedValue.id));
            inspected=input && locked && !locked->isEnabled();
            if (input) input->setText("UI input");
            dialog->accept();
        });
        command->trigger(); check(inspected && window.windowTitle().contains('*'),"form dialog edits text and disables read-only inputs");
        bool clicked=false;
        QTimer::singleShot(0,[&] { if (auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget())) { clicked=true; dialog->reject(); } });
        auto *view=window.findChild<QWidget *>("pageView");
        const QPointF location(field(doc,"name").rect.center().x()*view->width(),field(doc,"name").rect.center().y()*view->height());
        QMouseEvent click(QEvent::MouseButtonPress,location,view->mapToGlobal(location.toPoint()),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(view,&click);
        check(clicked,"clicking a page field opens the form editor");
        bool canceled=false;
        QTimer::singleShot(0,[&] { if (auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget())) { canceled=true; dialog->reject(); } });
        command->trigger(); check(canceled,"form dialog can be canceled");
        window.findChild<QAction *>("language_ko")->trigger();
        check(command->text()==I18n::text("Fill Form…"),"form menu is translated");
        bool licensed=false;
        QTimer::singleShot(0,[&] {
            if (auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget())) {
                auto *selector=dialog->findChild<QComboBox *>(); auto *browser=dialog->findChild<QTextBrowser *>();
                if (selector && browser) {
                    selector->setCurrentText("LICENSE"); licensed=browser->toPlainText().contains("GNU GENERAL PUBLIC LICENSE");
                    selector->setCurrentText("LICENSES/AppImage-runtime.txt"); licensed=licensed && browser->toPlainText().contains("MIT License");
                }
                dialog->reject();
            }
        });
        window.findChild<QAction *>("licensesAction")->trigger(); check(licensed,"license texts are embedded and accessible without installed files");
        std::cout<<"PASS: AcroForm fields, atomic updates, persistence, overlays, page operations and UI\n";
        return 0;
    } catch (const std::exception &e) { std::cerr<<e.what()<<'\n'; return 1; }
}
