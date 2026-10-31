#include "formwidgets.h"
#include "i18n.h"
#include <QWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QListWidget>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QSignalBlocker>
#include <QTimer>
#include <QHash>
#include <algorithm>
#include <stdexcept>

namespace {
template<class Button> class FieldButton : public Button {
public:
    explicit FieldButton(QWidget *parent):Button(parent) {}
protected:
    bool hitButton(const QPoint &point) const override { return this->rect().contains(point); }
};
}

Forms::InlineEditor::InlineEditor(Document &document, QWidget *page):QObject(page),document_(document),page_(page) {
    page_->installEventFilter(this);
}
void Forms::InlineEditor::clear() {
    busy_=true;
    for (auto &entry:entries_) delete entry.widget.data();
    entries_.clear();
    for (auto *group:findChildren<QButtonGroup *>(QString(),Qt::FindDirectChildrenOnly)) delete group;
    pageNumber_=-1; busy_=false;
}
void Forms::InlineEditor::place() {
    for (auto &entry:entries_) {
        const QRectF rect=entry.field.rect;
        const QRect box(qRound(rect.x()*page_->width()),qRound(rect.y()*page_->height()),
                        qMax(1,qRound(rect.width()*page_->width())),qMax(1,qRound(rect.height()*page_->height())));
        entry.widget->setGeometry(box);
        QFont font=entry.widget->font();
        int pixels=qMax(1,qRound((entry.field.fontSize>0?entry.field.fontSize:12)*page_->width()/pageSize_.width()));
        if (!entry.field.multiline) pixels=qMin(pixels,qMax(1,box.height()-4));
        font.setPixelSize(pixels);
        entry.widget->setFont(font);
        if (entry.field.kind==PdfFormField::Radio || entry.field.kind==PdfFormField::CheckBox) {
            const int side=qMax(1,qMin(box.width(),box.height())-4);
            entry.widget->setStyleSheet(QString("QAbstractButton { background: white; color: black; padding: 0; } QCheckBox::indicator, QRadioButton::indicator { width: %1px; height: %1px; subcontrol-origin: content; subcontrol-position: center; }").arg(side));
        }
    }
}
void Forms::InlineEditor::reset(Entry &entry) {
    QSignalBlocker block(entry.widget);
    if (auto *input=qobject_cast<QLineEdit *>(entry.widget.data())) input->setText(entry.field.text);
    else if (auto *input=qobject_cast<QTextEdit *>(entry.widget.data())) input->setPlainText(entry.field.text);
    else if (auto *input=qobject_cast<QAbstractButton *>(entry.widget.data())) input->setChecked(entry.field.checked);
    else if (auto *input=qobject_cast<QComboBox *>(entry.widget.data())) {
        input->setCurrentIndex(entry.field.selected.value(0,-1));
        if (entry.field.editable && entry.field.selected.isEmpty()) input->setEditText(entry.field.text);
    } else if (auto *input=qobject_cast<QListWidget *>(entry.widget.data())) {
        for (int i=0;i<input->count();++i) input->item(i)->setSelected(entry.field.selected.contains(i));
    }
}
void Forms::InlineEditor::showPage(int page) {
    clear(); if (page<0 || !document_.isOpen()) return;
    pageNumber_=page; pageSize_=document_.size(page);
    QHash<QString,QButtonGroup *> groups;
    for (const auto &field:document_.formFields(page)) {
        Entry entry; entry.field=field;
        PdfFormValue initial{field.id,field.text,field.checked,field.selected};
        if (field.kind==PdfFormField::CheckBox || field.kind==PdfFormField::Radio) {
            QAbstractButton *input=nullptr;
            if (field.kind==PdfFormField::Radio) {
                input=new FieldButton<QRadioButton>(page_);
                if (!groups.contains(field.group)) groups[field.group]=new QButtonGroup(this);
                groups[field.group]->addButton(input);
            } else input=new FieldButton<QCheckBox>(page_);
            input->setChecked(field.checked); entry.widget=input;
            entry.read=[input,initial] { auto value=initial; value.checked=input->isChecked(); return value; };
            connect(input,&QAbstractButton::clicked,this,[this] { submit(); });
        } else if (field.kind==PdfFormField::Text && field.multiline && !field.password) {
            auto *input=new QTextEdit(page_); input->setAcceptRichText(false); input->setPlainText(field.text);
            auto option=input->document()->defaultTextOption(); option.setAlignment(field.alignment); input->document()->setDefaultTextOption(option);
            input->setTabChangesFocus(true); input->setReadOnly(field.readOnly); entry.widget=input;
            entry.read=[input,initial] { auto value=initial; value.text=input->toPlainText(); return value; };
        } else if (field.kind==PdfFormField::Text) {
            auto *input=new QLineEdit(field.text,page_); input->setMaxLength(field.maximumLength>0?field.maximumLength:1000000);
            input->setAlignment(field.alignment|Qt::AlignVCenter);
            input->setReadOnly(field.readOnly); if (field.password) input->setEchoMode(QLineEdit::Password); entry.widget=input;
            entry.read=[input,initial] { auto value=initial; value.text=input->text(); return value; };
            connect(input,&QLineEdit::editingFinished,this,[this] { submit(); });
        } else if (field.multiSelect) {
            auto *input=new QListWidget(page_); input->setSelectionMode(QAbstractItemView::MultiSelection); input->addItems(field.choices);
            for (int index:field.selected) if (index>=0 && index<input->count()) input->item(index)->setSelected(true);
            entry.widget=input;
            entry.read=[input,initial] { auto value=initial; value.selected.clear(); for (int i=0;i<input->count();++i) if (input->item(i)->isSelected()) value.selected.append(i); return value; };
            connect(input,&QListWidget::itemSelectionChanged,this,[this] { submit(); });
        } else {
            auto *input=new QComboBox(page_); input->addItems(field.choices); input->setEditable(field.editable);
            input->setCurrentIndex(field.selected.value(0,-1));
            if (field.editable && field.selected.isEmpty()) input->setEditText(field.text);
            entry.widget=input;
            entry.read=[input,initial,editable=field.editable] {
                auto value=initial; value.selected.clear(); value.text.clear();
                if (input->currentIndex()>=0 && (!editable || input->currentText()==input->itemText(input->currentIndex()))) value.selected.append(input->currentIndex());
                else if (editable) value.text=input->currentText();
                return value;
            };
            connect(input,&QComboBox::activated,this,[this] { submit(); });
            if (input->lineEdit()) connect(input->lineEdit(),&QLineEdit::editingFinished,this,[this] { submit(); });
        }
        auto *widget=entry.widget.data(); widget->setObjectName("formField_"+QString::number(field.id));
        widget->setAccessibleName(field.label); widget->setToolTip(field.label+(field.readOnly?" ("+I18n::text("Read-only")+")":""));
        if (field.kind!=PdfFormField::Text) widget->setEnabled(!field.readOnly);
        if (field.kind==PdfFormField::Text || field.kind==PdfFormField::Choice)
            widget->setStyleSheet("QLineEdit, QTextEdit, QComboBox, QListWidget { background: #ffffff; color: #172b4d; border: 1px solid #7fa7e3; border-radius: 0; padding: 0; selection-background-color: #c2dcff; selection-color: #172b4d; } QLineEdit:focus, QTextEdit:focus, QComboBox:focus { border: 2px solid #2979ff; }");
        widget->installEventFilter(this);
        if (auto *input=qobject_cast<QComboBox *>(widget)) if (input->lineEdit()) input->lineEdit()->installEventFilter(this);
        entries_.append(entry); widget->setVisible(interactive_);
    }
    place();
}
void Forms::InlineEditor::commit() {
    if (busy_ || pageNumber_<0 || !document_.isOpen()) return;
    QList<PdfFormValue> changes;
    for (const auto &entry:entries_) {
        if (entry.field.readOnly) continue;
        auto value=entry.read();
        if (entry.field.kind==PdfFormField::Radio && !value.checked) continue;
        const bool different=entry.field.kind==PdfFormField::CheckBox || entry.field.kind==PdfFormField::Radio
            ?value.checked!=entry.field.checked:entry.field.kind==PdfFormField::Text
            ?value.text!=entry.field.text:value.selected!=entry.field.selected || (value.selected.isEmpty() && value.text!=entry.field.text);
        if (different) changes.append(value);
    }
    if (changes.isEmpty()) return;
    busy_=true;
    try {
        document_.fillForm(pageNumber_,changes);
        const auto updated=document_.formFields(pageNumber_);
        for (auto &entry:entries_) for (const auto &field:updated) if (entry.field.id==field.id) {
            entry.field=field;
            // A radio selection may change another widget in the same group.
            if (field.kind==PdfFormField::Radio || field.kind==PdfFormField::CheckBox) reset(entry);
            break;
        }
        if (changed) changed(pageNumber_);
        busy_=false;
    } catch (...) { busy_=false; throw; }
}
void Forms::InlineEditor::submit() {
    if (busy_) return;
    try { commit(); } catch (const std::exception &error) {
        busy_=true; if (failed) failed(QString::fromUtf8(error.what())); busy_=false;
    }
}
void Forms::InlineEditor::setInteractive(bool enabled) {
    interactive_=enabled;
    for (auto &entry:entries_) entry.widget->setVisible(enabled);
}
void Forms::InlineEditor::focusFirst() {
    for (auto &entry:entries_) if (!entry.field.readOnly) { entry.widget->setFocus(Qt::ShortcutFocusReason); return; }
}
bool Forms::InlineEditor::eventFilter(QObject *object, QEvent *event) {
    if (object==page_ && event->type()==QEvent::Resize && pageNumber_>=0) place();
    if (busy_) return false;
    if (event->type()==QEvent::FocusOut) {
        // Combo popups temporarily take focus while the user is choosing an item.
        auto *focus=static_cast<QFocusEvent *>(event);
        if (focus->reason()!=Qt::PopupFocusReason) submit();
    }
    if (event->type()==QEvent::KeyPress) {
        auto *key=static_cast<QKeyEvent *>(event);
        if (key->key()==Qt::Key_Escape) {
            for (auto &entry:entries_) if (entry.widget==object || entry.widget->isAncestorOf(qobject_cast<QWidget *>(object))) {
                busy_=true; reset(entry); busy_=false; page_->setFocus(); return true;
            }
        }
        if ((key->key()==Qt::Key_Return || key->key()==Qt::Key_Enter) && key->modifiers().testFlag(Qt::ControlModifier)) { submit(); return true; }
    }
    return QObject::eventFilter(object,event);
}
