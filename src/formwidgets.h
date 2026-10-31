#pragma once
#include "document.h"
#include <QObject>
#include <QPointer>
#include <functional>
class QWidget;
namespace Forms {
class InlineEditor : public QObject {
    Document &document_;
    QWidget *page_;
    int pageNumber_=-1;
    QSizeF pageSize_{1,1};
    bool busy_=false, interactive_=true;
    struct Entry {
        PdfFormField field;
        QPointer<QWidget> widget;
        std::function<PdfFormValue()> read;
    };
    QList<Entry> entries_;
    void place();
    void submit();
    void reset(Entry &entry);
    bool eventFilter(QObject *object, QEvent *event) override;
public:
    InlineEditor(Document &document, QWidget *page);
    void showPage(int page);
    void clear();
    void commit();
    void setInteractive(bool enabled);
    void focusFirst();
    std::function<void(int)> changed;
    std::function<void(QString)> failed;
};
}
