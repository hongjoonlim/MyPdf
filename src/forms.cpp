#include "document.h"
#include "i18n.h"
#include <poppler-qt6.h>
#include <poppler-form.h>
#include <stdexcept>
#include <QSet>

namespace {
[[noreturn]] void invalid() { throw std::runtime_error(I18n::text("Invalid or read-only form field.").toStdString()); }
std::unique_ptr<Poppler::Document> openBase(const QString &path) {
    auto pdf=Poppler::Document::load(path);
    if (!pdf || pdf->isLocked()) throw std::runtime_error(I18n::text("Unable to read PDF form fields.").toStdString());
    return pdf;
}
}
bool Document::hasXfaForm() const { return pdf_ && pdf_->formType()==Poppler::Document::XfaForm; }
QList<PdfFormField> Document::formFields(int page) const {
    check(page);
    auto original=openBase(base_);
    QList<PdfFormField> result;
    if (original->formType()==Poppler::Document::XfaForm) return result;
    auto source=original->page(page);
    for (const auto &field:source->formFields()) {
        if (!field->isVisible()) continue;
        PdfFormField item; item.id=field->id(); item.rect=field->rect();
        item.group=field->fullyQualifiedName();
        item.label=field->uiName(); if (item.label.isEmpty()) item.label=item.group;
        if (item.label.isEmpty()) item.label=I18n::text("Field %1").arg(item.id);
        item.readOnly=field->isReadOnly() || !original->okToFillForm();
        if (auto *button=dynamic_cast<Poppler::FormFieldButton *>(field.get())) {
            if (button->buttonType()==Poppler::FormFieldButton::Push) continue;
            item.kind=button->buttonType()==Poppler::FormFieldButton::Radio?PdfFormField::Radio:PdfFormField::CheckBox;
            item.checked=button->state();
            if (!button->caption().isEmpty()) item.label+=" — "+button->caption();
        } else if (auto *text=dynamic_cast<Poppler::FormFieldText *>(field.get())) {
            item.kind=PdfFormField::Text; item.text=text->text(); item.maximumLength=text->maximumLength();
            item.multiline=text->textType()==Poppler::FormFieldText::Multiline; item.password=text->isPassword();
        } else if (auto *choice=dynamic_cast<Poppler::FormFieldChoice *>(field.get())) {
            item.kind=PdfFormField::Choice; item.choices=choice->choices(); item.selected=choice->currentChoices();
            item.editable=choice->isEditable(); item.multiSelect=choice->multiSelect(); item.text=choice->editChoice();
        } else continue;
        result.append(item);
    }
    return result;
}
void Document::fillForm(int page, const QList<PdfFormValue> &values) {
    check(page); if (values.isEmpty()) return;
    auto original=openBase(base_);
    if (!original->okToFillForm() || original->formType()!=Poppler::Document::AcroForm) invalid();
    auto source=original->page(page); auto fields=source->formFields();
    QSet<int> seen;
    // Work on an isolated document so validation and export failures leave the editor unchanged.
    for (const auto &value:values) {
        if (seen.contains(value.id)) invalid();
        seen.insert(value.id);
        Poppler::FormField *target=nullptr;
        for (const auto &field:fields) if (field->id()==value.id) { target=field.get(); break; }
        if (!target || target->isReadOnly() || !target->isVisible()) invalid();
        if (auto *button=dynamic_cast<Poppler::FormFieldButton *>(target)) {
            if (button->buttonType()==Poppler::FormFieldButton::Push) invalid();
            button->setState(value.checked);
        } else if (auto *text=dynamic_cast<Poppler::FormFieldText *>(target)) {
            if (value.text.size()>1000000 || (text->maximumLength()>0 && value.text.size()>text->maximumLength())) invalid();
            text->setText(value.text);
        } else if (auto *choice=dynamic_cast<Poppler::FormFieldChoice *>(target)) {
            if ((!choice->multiSelect() && value.selected.size()>1) || value.text.size()>1000000) invalid();
            QSet<int> indices;
            for (int index:value.selected) {
                if (index<0 || index>=choice->choices().size() || indices.contains(index)) invalid();
                indices.insert(index);
            }
            if (!choice->isEditable() && !value.text.isEmpty()) invalid();
            choice->setCurrentChoices(value.selected);
            if (choice->isEditable() && value.selected.isEmpty()) choice->setEditChoice(value.text);
        } else invalid();
    }
    const QString out=next(); auto converter=original->pdfConverter();
    converter->setOutputFileName(out); converter->setPDFOptions(Poppler::PDFConverter::WithChanges);
    if (!converter->convert()) throw std::runtime_error(I18n::text("Unable to save PDF form values.").toStdString());
    commitObjects(out,objects_);
}
