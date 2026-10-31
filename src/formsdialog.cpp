#include "formsdialog.h"
#include "document.h"
#include "i18n.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QListWidget>
#include <QMessageBox>
#include <QHash>
#include <functional>
using I18n::text;
bool Forms::edit(Document &document, int page, QWidget *parent, int focusId) {
    const auto fields=document.formFields(page);
    if (fields.isEmpty()) {
        QMessageBox::information(parent,text("Fill Form…"),text(document.hasXfaForm()?"XFA forms are not supported.":"This page has no supported form fields."));
        return false;
    }
    QDialog dialog(parent); dialog.setObjectName("formDialog");
    dialog.setWindowTitle(text("Fill Form — Page %1").arg(page+1)); dialog.resize(560,500);
    auto *layout=new QVBoxLayout(&dialog);
    auto *tip=new QLabel(text("Edit the fields on this page, then apply. Use Save to write the PDF.")); tip->setWordWrap(true); layout->addWidget(tip);
    auto *scroll=new QScrollArea; scroll->setWidgetResizable(true); layout->addWidget(scroll);
    auto *contents=new QWidget; auto *form=new QFormLayout(contents); form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow); scroll->setWidget(contents);
    QHash<QString,QButtonGroup *> groups;
    QHash<QString,int> radioNumbers;
    QList<std::function<PdfFormValue()>> readers;
    QWidget *focus=nullptr;
    for (const auto &field:fields) {
        QWidget *editor=nullptr;
        std::function<PdfFormValue()> reader;
        PdfFormValue initial; initial.id=field.id; initial.text=field.text; initial.checked=field.checked; initial.selected=field.selected;
        QString label=field.label;
        if (field.kind==PdfFormField::CheckBox || field.kind==PdfFormField::Radio) {
            QAbstractButton *button=nullptr;
            if (field.kind==PdfFormField::Radio) {
                button=new QRadioButton(text("Option %1").arg(++radioNumbers[field.group]));
                if (!groups.contains(field.group)) groups.insert(field.group,new QButtonGroup(&dialog));
                groups[field.group]->addButton(button);
            } else button=new QCheckBox;
            button->setChecked(field.checked); editor=button;
            reader=[button,initial] { auto value=initial; value.checked=button->isChecked(); return value; };
        } else if (field.kind==PdfFormField::Text) {
            if (field.multiline && !field.password) {
                auto *input=new QTextEdit; input->setAcceptRichText(false); input->setPlainText(field.text); input->setMaximumHeight(130); editor=input;
                reader=[input,initial] { auto value=initial; value.text=input->toPlainText(); return value; };
            } else {
                auto *input=new QLineEdit(field.text); input->setMaxLength(field.maximumLength>0?field.maximumLength:1000000);
                if (field.password) input->setEchoMode(QLineEdit::Password);
                editor=input; reader=[input,initial] { auto value=initial; value.text=input->text(); return value; };
            }
        } else if (field.multiSelect) {
            auto *input=new QListWidget; input->setSelectionMode(QAbstractItemView::MultiSelection); input->addItems(field.choices);
            for (int index:field.selected) if (index>=0 && index<input->count()) input->item(index)->setSelected(true);
            input->setMaximumHeight(130); editor=input;
            reader=[input,initial] { auto value=initial; value.selected.clear(); for (int i=0;i<input->count();++i) if (input->item(i)->isSelected()) value.selected.append(i); return value; };
        } else {
            auto *input=new QComboBox; input->addItems(field.choices); input->setEditable(field.editable); input->setCurrentIndex(field.selected.value(0,-1));
            if (field.editable && field.selected.isEmpty()) input->setEditText(field.text);
            editor=input; reader=[input,initial,editable=field.editable] {
                auto value=initial; value.selected.clear(); value.text.clear();
                if (input->currentIndex()>=0 && (!editable || input->currentText()==input->itemText(input->currentIndex()))) value.selected.append(input->currentIndex());
                else if (editable) value.text=input->currentText();
                return value;
            };
        }
        editor->setObjectName("formField_"+QString::number(field.id)); editor->setEnabled(!field.readOnly);
        if (field.readOnly) label+=" ("+text("Read-only")+")";
        auto *caption=new QLabel(label); caption->setWordWrap(true); caption->setBuddy(editor); form->addRow(caption,editor);
        readers.append(reader); if (field.id==focusId) focus=editor;
    }
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel); layout->addWidget(buttons);
    QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
    QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if (focus) { focus->setFocus(); scroll->ensureWidgetVisible(focus); }
    if (dialog.exec()!=QDialog::Accepted) return false;
    QList<PdfFormValue> changes;
    for (int i=0;i<fields.size();++i) {
        const auto &field=fields[i]; if (field.readOnly) continue;
        const auto value=readers[i]();
        if (field.kind==PdfFormField::Radio && !value.checked) continue;
        if (value.text!=field.text || value.checked!=field.checked || value.selected!=field.selected) changes.append(value);
    }
    if (changes.isEmpty()) return false;
    document.fillForm(page,changes); return true;
}
