#include "document.h"
#include "formsdialog.h"
#include "licenses.h"
#include "docx.h"
#include "icons.h"
#include "i18n.h"
#include <QSettings>
#include <QHash>
#include <QDoubleSpinBox>
#include <QFontComboBox>
#include <QTextEdit>
#include <QColorDialog>
#include <QFormLayout>
#include <QKeyEvent>
#include <QActionGroup>
#include <QPointer>
#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QScrollArea>
#include <QListWidget>
#include <QLabel>
#include <QSplitter>
#include <QMenuBar>
#include <QToolBar>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QStatusBar>
#include <QFileInfo>
#include <QCloseEvent>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTimer>
#include <functional>
#include <stdexcept>
#include <QStackedWidget>
#include <QToolButton>
#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QStyledItemDelegate>
#include <QHBoxLayout>
#include <QStyle>
#include <QSignalBlocker>
#include <QPalette>
#include <algorithm>

using I18n::text;

static void applyApplicationTheme(bool dark) {
    const QColor window = dark ? QColor("#1f2125") : QColor("#f5f6f8");
    const QColor text = dark ? QColor("#e7e9ed") : QColor("#263244");
    const QColor base = dark ? QColor("#272a2f") : QColor(Qt::white);
    const QColor alternate = dark ? QColor("#30343a") : QColor("#f5f6f8");
    const QColor button = dark ? QColor("#2b2e33") : QColor(Qt::white);
    const QColor highlight = dark ? QColor("#375a89") : QColor("#dce9fb");
    const QColor highlightedText = dark ? QColor(Qt::white) : QColor("#174984");
    const QColor accent = dark ? QColor("#8ab4f8") : QColor("#185abc");
    const QColor border = dark ? QColor("#3c4047") : QColor("#dde1e7");
    const QColor hover = dark ? QColor("#343942") : QColor("#edf2f9");
    const QColor selected = dark ? QColor("#334a6a") : QColor("#e3edfc");
    const QColor selectedBorder = dark ? QColor("#587aa5") : QColor("#7fa7e3");
    const QColor muted = dark ? QColor("#a0a6b0") : QColor("#64748b");
    const QColor canvas = dark ? QColor("#17191c") : QColor("#dce0e6");
    const QColor buttonHover = dark ? QColor("#194f89") : QColor("#124996");

    QPalette palette;
    palette.setColor(QPalette::Window,window);
    palette.setColor(QPalette::WindowText,text);
    palette.setColor(QPalette::Base,base);
    palette.setColor(QPalette::AlternateBase,alternate);
    palette.setColor(QPalette::Text,text);
    palette.setColor(QPalette::Button,button);
    palette.setColor(QPalette::ButtonText,text);
    palette.setColor(QPalette::Highlight,highlight);
    palette.setColor(QPalette::HighlightedText,highlightedText);
    palette.setColor(QPalette::Mid,border);
    palette.setColor(QPalette::ToolTipBase,base);
    palette.setColor(QPalette::ToolTipText,text);
    palette.setColor(QPalette::Link,accent);
    palette.setColor(QPalette::Disabled,QPalette::ButtonText,muted);
    palette.setColor(QPalette::Disabled,QPalette::Text,muted);
    qApp->setPalette(palette);

    QString styleSheet = QStringLiteral(R"(
        QMenuBar { background: %1; padding: 4px 8px; }
        QMenuBar::item:selected { background: %2; }
        QMenu { background: %1; border: 1px solid %3; }
        QToolBar { background: %1; border: 0; border-bottom: 1px solid %3; spacing: 6px; padding: 6px 12px; }
        QToolButton { padding: 7px 9px; border-radius: 5px; }
        QToolButton:hover { background: %2; }
        QToolButton:checked { color: %5; background: %6; }
        QToolButton[popupMenu="true"] { padding-right: 24px; }
        QToolButton[popupMenu="true"]::menu-indicator {
            subcontrol-origin: padding; subcontrol-position: right center;
            right: 7px; width: 10px; height: 10px;
        }
        QToolBar#mainTools { padding: 8px 12px; spacing: 5px; }
        QLabel#filename { padding-left: 12px; font-weight: 600; }
        QLabel#sideTitle { padding: 14px; font-weight: 600; background: %1; }
        QLabel#selectionTip { padding: 10px; color: %4; font-size: 11px; }
        QListWidget { background: %7; border: 0; padding: 8px; }
        QListWidget::item { padding: 8px; border: 1px solid transparent; border-radius: 6px; }
        QListWidget::item:hover { background: %2; }
        QListWidget::item:selected { background: %6; border-color: %8; color: %9; }
        QScrollArea#documentCanvas { background: %10; border: 0; }
        QStatusBar { background: %1; border-top: 1px solid %3; padding: 3px 10px; }
        QStatusBar::item { border: 0; }
        QLabel#welcomeTitle { color: %5; font-size: 34px; font-weight: 700; padding: 16px; }
        QPushButton#openButton { background: %5; color: #fff; border: 0; border-radius: 6px; margin-top: 10px; }
        QPushButton#openButton:hover { background: %11; }
    )");
    styleSheet = styleSheet.arg(base.name()).arg(hover.name()).arg(border.name())
        .arg(muted.name()).arg(accent.name()).arg(selected.name()).arg(window.name())
        .arg(selectedBorder.name()).arg(highlightedText.name()).arg(canvas.name())
        .arg(buttonHover.name());
    qApp->setStyleSheet(styleSheet);
}

class InkPad : public QWidget {
    QImage ink_{800, 260, QImage::Format_ARGB32_Premultiplied};
    QPoint last_;
    bool drawing_ = false, modified_ = false;
public:
    InkPad() { ink_.fill(Qt::transparent); setFixedSize(800,260); setCursor(Qt::CrossCursor); }
    QImage image() const { return ink_; }
    bool modified() const { return modified_; }
    void setImage(const QImage &image) {
        ink_.fill(Qt::transparent); QPainter painter(&ink_);
        const auto preview=image.scaled(ink_.size(),Qt::KeepAspectRatio,Qt::SmoothTransformation);
        painter.drawImage(QPoint((ink_.width()-preview.width())/2,(ink_.height()-preview.height())/2),preview);
        modified_=false; update();
    }
    void clear() { modified_=true; ink_.fill(Qt::transparent); update(); }
protected:
    void paintEvent(QPaintEvent *) override { QPainter p(this); p.fillRect(rect(), Qt::white); p.drawImage(0,0,ink_); }
    void mousePressEvent(QMouseEvent *e) override {
        if (e->button() != Qt::LeftButton) return;
        modified_=true; drawing_ = true; last_ = e->position().toPoint();
        QPainter p(&ink_); p.setPen(QPen(QColor("#16305b"),3,Qt::SolidLine,Qt::RoundCap)); p.drawPoint(last_); update();
    }
    void mouseMoveEvent(QMouseEvent *e) override {
        if (!drawing_) return;
        QPainter p(&ink_); p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(QColor("#16305b"),3,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
        p.drawLine(last_, e->position().toPoint()); last_ = e->position().toPoint(); update();
    }
    void mouseReleaseEvent(QMouseEvent *) override { drawing_ = false; }
};

class PageView : public QWidget {
    QImage page_, signature_;
    QPointF start_, end_;
    bool dragging_ = false;
    bool textPlacement_ = false;
    bool highlighting_=false;
    QPolygonF highlightPoints_;
    QColor highlightColor_{255,235,59,90};
    double highlightWidth_=12;
    QPointF pagePosition(QPointF point) const { return QPointF(qBound(0.,point.x()/width(),1.),qBound(0.,point.y()/height(),1.)); }
    void extendHighlight(QPointF point, Qt::KeyboardModifiers modifiers) {
        if (highlightPoints_.isEmpty()) return;
        QPointF position=pagePosition(point);
        if (modifiers & Qt::ShiftModifier) {
            position.setY(highlightPoints_.first().y());
            highlightPoints_={highlightPoints_.first(),position};
        } else if (highlightPoints_.size()<20000 && position!=highlightPoints_.last()) highlightPoints_.append(position);
    }
    QSizeF pagePoints_{1,1};
    QList<PageObject> objects_;
    QList<PdfFormField> formFields_;
    QHash<QString,QImage> layers_;
    int resizeCorner_ = -1;
    QString selected_;
    bool moving_ = false;
    QRectF originalRect_, previewRect_;
    QPointF moveStart_;
    QRectF pixels(const QRectF &rect) const { return QRectF(rect.x()*width(),rect.y()*height(),rect.width()*width(),rect.height()*height()); }
    const PageObject *selection() const { for (const auto &object:objects_) if (object.id==selected_) return &object; return nullptr; }
    QPointF bounded(const QPointF &position) const { return QPointF(qBound(0.,position.x(),1-originalRect_.width()),qBound(0.,position.y(),1-originalRect_.height())); }
    QList<QPointF> corners(const QRectF &rect) const { return {rect.topLeft(),rect.topRight(),rect.bottomRight(),rect.bottomLeft()}; }
    int handleAt(QPointF point) const {
        if (!selection()) return -1;
        const auto points=corners(pixels(selection()->rect));
        for (int i=0;i<points.size();++i) if (QRectF(points[i]-QPointF(6,6),QSizeF(12,12)).contains(point)) return i;
        return -1;
    }
    void updatePreview(QPointF point) {
        const QPointF delta=point-moveStart_;
        previewRect_=originalRect_;
        if (resizeCorner_<0) { previewRect_.moveTopLeft(bounded(originalRect_.topLeft()+QPointF(delta.x()/width(),delta.y()/height()))); return; }
        const auto points=corners(originalRect_);
        const QPointF anchor=points[(resizeCorner_+2)%4], corner=points[resizeCorner_];
        const QPointF direction=corner-anchor;
        const QPointF proposed=direction+QPointF(delta.x()/width(),delta.y()/height());
        double factor=QPointF::dotProduct(proposed,direction)/QPointF::dotProduct(direction,direction);
        const double maxX=direction.x()>0?(1-anchor.x())/direction.x():-anchor.x()/direction.x();
        const double maxY=direction.y()>0?(1-anchor.y())/direction.y():-anchor.y()/direction.y();
        const double maximum=qMin(maxX,maxY);
        const double minimum=qMin(maximum,qMax(4/(pagePoints_.width()*originalRect_.width()),4/(pagePoints_.height()*originalRect_.height())));
        factor=qBound(minimum,factor,maximum);
        previewRect_=QRectF(anchor,anchor+direction*factor).normalized().intersected(QRectF(0,0,1,1));
    }


public:
    std::function<bool(QPointF)> formClicked;
    std::function<void(QPolygonF)> highlightPlaced;
    std::function<void()> highlightCanceled;
    void setHighlight(bool enabled, const QColor &color=QColor(255,235,59,90), double width=12) {
        highlighting_=enabled; highlightPoints_.clear(); highlightColor_=color; highlightWidth_=width;
        if (enabled) { selectObject({}); setFocusPolicy(Qt::StrongFocus); setFocus(); }
        setCursor(enabled?Qt::CrossCursor:Qt::ArrowCursor); update();
    }
    std::function<void(QString,QPoint)> objectContextMenu;
    std::function<void(QString)> objectEdited;
    std::function<void(QString,QRectF)> objectResized;
    std::function<void(QString,QPointF)> objectMoved;
    std::function<void(QString)> objectRemoved;
    std::function<void()> selectionChanged;
    void setFormFields(const QList<PdfFormField> &fields) { formFields_=fields; update(); }
    void setPagePoints(QSizeF size) { setMouseTracking(true); pagePoints_=size; setFocusPolicy(Qt::StrongFocus); }
    QString selectedObject() const { return selected_; }
    void selectObject(const QString &id) { selected_=id; moving_=false; resizeCorner_=-1; update(); if (selectionChanged) selectionChanged(); }
    void setObjects(const QList<PageObject> &objects, const QHash<QString,QImage> &layers={}) { objects_=objects; layers_=layers; moving_=false; if (!selection()) selected_.clear(); update(); if (selectionChanged) selectionChanged(); }
    std::function<void(QPointF)> textPlaced;
    std::function<void()> textCanceled;
    void setTextPlacement(bool enabled) { if (enabled) selectObject({}); textPlacement_=enabled; setCursor(enabled?Qt::IBeamCursor:Qt::ArrowCursor); if (enabled) { setFocusPolicy(Qt::StrongFocus); setFocus(); } }
    std::function<void(QImage,QRectF)> placed;
    std::function<void()> signatureStateChanged;
    bool hasSignature() const { return !signature_.isNull(); }
    void setPage(QImage page) { highlightPoints_.clear(); page_ = std::move(page); setFixedSize(page_.deviceIndependentSize().toSize()); update(); }
    void setSignature(QImage image) { if (!image.isNull()) selectObject({}); dragging_ = false; signature_ = std::move(image); setCursor(textPlacement_ ? Qt::IBeamCursor : (signature_.isNull() ? Qt::ArrowCursor : Qt::CrossCursor)); update(); if (signatureStateChanged) signatureStateChanged(); }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::SmoothPixmapTransform); p.fillRect(rect(),Qt::white); p.drawImage(0,0,page_);
        for (const auto &object:objects_) {
            const QRectF target=pixels(moving_ && object.id==selected_?previewRect_:object.rect);
            const auto layer=layers_.constFind(object.id);
            if (layer!=layers_.cend()) p.drawImage(target,*layer);
        }
        if (!highlighting_ && !textPlacement_ && signature_.isNull()) {
            for (const auto &field:formFields_) {
                p.setPen(QPen(field.readOnly?QColor("#8c929b"):QColor("#4e8ccf"),1,Qt::DashLine));
                p.setBrush(Qt::NoBrush); p.drawRect(pixels(field.rect));
            }
        }
        if (!highlightPoints_.isEmpty()) {
            QPainterPath stroke;
            for (int i=0;i<highlightPoints_.size();++i) {
                const QPointF point(highlightPoints_[i].x()*width(),highlightPoints_[i].y()*height());
                if (i==0) stroke.moveTo(point); else stroke.lineTo(point);
            }
            p.setRenderHint(QPainter::Antialiasing);
            p.setPen(QPen(highlightColor_,highlightWidth_*width()/pagePoints_.width(),Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
            p.drawPath(stroke);
        }
        if (const auto *object=selection()) {
            const QRectF box=pixels(moving_?previewRect_:object->rect);
            p.setPen(QPen(QColor("#2979ff"),1,Qt::DashLine)); p.setBrush(Qt::NoBrush); p.drawRect(box);
            p.setPen(QColor("#2979ff")); p.setBrush(Qt::white);
            for (const auto &point:corners(box)) p.drawRect(QRectF(point-QPointF(4,4),QSizeF(8,8)));
        }
        if (dragging_) { auto r = QRectF(start_,end_).normalized(); p.drawImage(r, signature_); p.setPen(Qt::DashLine); p.drawRect(r); }
    }
    void keyPressEvent(QKeyEvent *e) override {
        if (e->key()==Qt::Key_Escape && highlighting_) {
            highlightPoints_.clear(); update(); if (highlightCanceled) highlightCanceled(); e->accept(); return;
        }
        if (e->key()==Qt::Key_Escape && textPlacement_) { if (textCanceled) textCanceled(); e->accept(); return; }
        if (selection() && !textPlacement_ && signature_.isNull()) {
            if (e->key()==Qt::Key_F2 || e->key()==Qt::Key_Return || e->key()==Qt::Key_Enter) { selectObject(selected_); if (objectEdited) objectEdited(selected_); e->accept(); return; }
            if (e->key()==Qt::Key_Escape) { moving_=false; selectObject({}); e->accept(); return; }
            if (e->key()==Qt::Key_Delete || e->key()==Qt::Key_Backspace) { if (objectRemoved) objectRemoved(selected_); e->accept(); return; }
            QPointF delta; const double step=(e->modifiers()&Qt::ShiftModifier)?10.:1.;
            if (e->key()==Qt::Key_Left) delta.setX(-step);
            else if (e->key()==Qt::Key_Right) delta.setX(step);
            else if (e->key()==Qt::Key_Up) delta.setY(-step);
            else if (e->key()==Qt::Key_Down) delta.setY(step);
            if (!delta.isNull()) { originalRect_=selection()->rect; if (objectMoved) objectMoved(selected_,bounded(originalRect_.topLeft()+QPointF(delta.x()/pagePoints_.width(),delta.y()/pagePoints_.height()))); e->accept(); return; }
        }
        QWidget::keyPressEvent(e);
    }
    void mousePressEvent(QMouseEvent *e) override {
        if (highlighting_) {
            if (e->button()==Qt::LeftButton) { setFocus(); highlightPoints_={pagePosition(e->position())}; update(); }
            return;
        }
        if (textPlacement_ && e->button()==Qt::LeftButton) {
            if (textPlaced) textPlaced(QPointF(e->position().x()/width(),e->position().y()/height()));
            return;
        }
        if (e->button() == Qt::LeftButton && !signature_.isNull()) { start_ = end_ = e->position(); dragging_ = true; return; }
        if (e->button()==Qt::RightButton && !textPlacement_ && signature_.isNull()) {
            QString hit;
            for (auto it=objects_.crbegin();it!=objects_.crend();++it) if (pixels(it->rect).contains(e->position())) { hit=it->id; break; }
            selectObject(hit); setFocus();
            if (!hit.isEmpty() && objectContextMenu) objectContextMenu(hit,e->position().toPoint());
            return;
        }
        if (e->button()!=Qt::LeftButton) return;
        setFocus();
        const int handle=handleAt(e->position());
        if (handle>=0) { originalRect_=previewRect_=selection()->rect; resizeCorner_=handle; moveStart_=e->position(); moving_=true; return; }
        QString hit;
        for (auto it=objects_.crbegin();it!=objects_.crend();++it) if (pixels(it->rect).adjusted(-3,-3,3,3).contains(e->position())) { hit=it->id; break; }
        selectObject(hit);
        if (hit.isEmpty() && formClicked && formClicked(pagePosition(e->position()))) return;
        if (const auto *object=selection()) { originalRect_=previewRect_=object->rect; moveStart_=e->position(); moving_=true; }

    }
    void mouseDoubleClickEvent(QMouseEvent *e) override {
        if (e->button()!=Qt::LeftButton || highlighting_ || textPlacement_ || !signature_.isNull()) { e->ignore(); return; }
        QString hit;
        for (auto it=objects_.crbegin();it!=objects_.crend();++it) if (pixels(it->rect).contains(e->position())) { hit=it->id; break; }
        selectObject(hit);
        if (!hit.isEmpty() && objectEdited) objectEdited(hit);
        e->accept();
    }
    void mouseMoveEvent(QMouseEvent *e) override {
        if (highlighting_) { if (!highlightPoints_.isEmpty()) { extendHighlight(e->position(),e->modifiers()); update(); } return; }
        if (moving_) {
            updatePreview(e->position()); update(); return;
        }
        if (!dragging_ && !textPlacement_ && signature_.isNull()) {
            const int handle=handleAt(e->position());
            setCursor(handle<0?Qt::ArrowCursor:((handle==0 || handle==2)?Qt::SizeFDiagCursor:Qt::SizeBDiagCursor));
        }
        if (dragging_) { end_ = QPointF(qBound(0.,e->position().x(),double(width())),qBound(0.,e->position().y(),double(height()))); update(); }
    }
    void mouseReleaseEvent(QMouseEvent *e) override {
        if (highlighting_ && e->button()==Qt::LeftButton) {
            extendHighlight(e->position(),e->modifiers());
            const auto points=highlightPoints_; highlightPoints_.clear(); update();
            if (points.size()>1 && (points.boundingRect().width()>0 || points.boundingRect().height()>0) && highlightPlaced) highlightPlaced(points);
            return;
        }
        if (moving_ && e->button()==Qt::LeftButton) {
            updatePreview(e->position()); moving_=false;
            const int handle=resizeCorner_; resizeCorner_=-1;
            if (previewRect_!=originalRect_) {
                if (handle>=0 && objectResized) objectResized(selected_,previewRect_);
                else if (objectMoved) objectMoved(selected_,previewRect_.topLeft());
            }
            update(); return;
        }
        if (!dragging_ || e->button() != Qt::LeftButton) return;
        dragging_ = false;
        QRectF r = QRectF(start_,end_).normalized().intersected(QRectF(rect()));
        if (r.width() >= 5 && r.height() >= 5 && placed) {
            auto img = signature_; setSignature({});
            placed(img, QRectF(r.x()/width(), r.y()/height(), r.width()/width(), r.height()/height()));
        }
        update();
    }
};

// Reserve the complete thumbnail and caption area even before lazy rendering finishes.
// The default icon delegate can cache a text-only height while the icons are absent.
class ThumbnailDelegate : public QStyledItemDelegate {
public:
    explicit ThumbnailDelegate(QObject *parent) : QStyledItemDelegate(parent) {}
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override {
        return QSize(156,184);
    }
    void paint(QPainter *p, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        const auto palette = QApplication::palette();
        const QRect card=option.rect.adjusted(5,4,-5,-4);
        if (option.state & QStyle::State_Selected) {
            p->setPen(palette.color(QPalette::Mid)); p->setBrush(palette.color(QPalette::Highlight));
            p->drawRoundedRect(card,6,6);
        } else if (option.state & QStyle::State_MouseOver) {
            p->setPen(Qt::NoPen); p->setBrush(palette.color(QPalette::AlternateBase)); p->drawRoundedRect(card,6,6);
        }
        const QRect imageArea(card.left()+10,card.top()+10,card.width()-20,138);
        const auto icon=qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        if (!icon.isNull()) {
            QPixmap image=icon.pixmap(QSize(144,144));
            QSizeF size=image.deviceIndependentSize();
            size.scale(QSizeF(imageArea.size()),Qt::KeepAspectRatio);
            const QRectF target(QPointF(imageArea.center())-QPointF(size.width()/2,size.height()/2),size);
            p->setRenderHint(QPainter::SmoothPixmapTransform);
            p->fillRect(target.adjusted(-1,-1,1,1),QColor("#cbd1da"));
            p->drawPixmap(target,image,QRectF(image.rect()));
        } else {
            p->setPen(palette.color(QPalette::Mid)); p->setBrush(Qt::white);
            p->drawRect(QRect(imageArea.center().x()-46,imageArea.top(),92,134));
        }
        p->setPen((option.state & QStyle::State_Selected)
            ? palette.color(QPalette::HighlightedText) : palette.color(QPalette::Text));
        p->setFont(option.font);
        p->drawText(QRect(card.left()+4,card.bottom()-24,card.width()-8,20),Qt::AlignCenter,index.data().toString());
        p->restore();
    }
};

class Window : public QMainWindow {
    Document doc_;
    QLabel *importNotice_ = new QLabel;
    QAction *closeDocument_ = nullptr;
    QAction *saveDocument_ = nullptr, *saveAsDocument_ = nullptr;
    QListWidget *pages_ = new QListWidget;
    PageView *view_ = new PageView;
    QScrollArea *scroll_ = new QScrollArea;
    QLabel *status_ = new QLabel;
    QList<QAction *> requiresDoc_;
    double dpi_ = 110;
    bool refreshing_ = false;
    QStackedWidget *workspace_ = new QStackedWidget;
    QWidget *sidebar_ = new QWidget;
    QLabel *filename_ = new QLabel(text("Open a document to get started"));
    QSpinBox *pageNumber_ = new QSpinBox;
    QLabel *pageTotal_ = new QLabel("/ 0");
    QComboBox *zoom_ = new QComboBox;
    QAction *previous_ = nullptr, *next_ = nullptr, *moveUp_ = nullptr, *moveDown_ = nullptr;
    QAction *cancelSignature_ = nullptr;
    QAction *highlight_ = nullptr;
    QColor highlightColor_{255,235,59,90};
    double highlightWidth_=12;
    QAction *addText_ = nullptr, *cancelText_ = nullptr, *deleteObject_ = nullptr;
    QTimer thumbnails_;
    int thumbnailRow_ = 0;
    struct TranslationBinding { QPointer<QObject> object; QByteArray property; QString source; };
    QList<TranslationBinding> translationBindings_;
    QAction *selectObjects_ = nullptr, *editObject_ = nullptr, *cancelPlacement_ = nullptr;
    QAction *editObjectToolAction_ = nullptr, *deleteObjectToolAction_ = nullptr;
    QActionGroup *languages_ = nullptr;
    QActionGroup *themes_ = nullptr;
    QString statusKey_ = "Open a PDF or DOCX file to get started.";
    QStringList statusArgs_;
public:
    Window() {
        resize(1200,850); setMinimumSize(850,600); setWindowTitle("MyPdf");
        auto *root = new QWidget;
        auto *layout = new QVBoxLayout(root); layout->setContentsMargins(0,0,0,0); layout->setSpacing(0);
        setCentralWidget(root);
        auto *file = menuBar()->addMenu(text("&File"));
        auto *edit = menuBar()->addMenu(text("&Edit"));
        auto *display = menuBar()->addMenu(text("&View"));
        auto *organize = menuBar()->addMenu(text("&Pages"));
        auto *sign = new QMenu(text("Signature"),this); sign->setIcon(Icons::line(Icons::Symbol::Signature));
        auto *settings = menuBar()->addMenu(text("Se&ttings"));
        auto *languageMenu = settings->addMenu(text("Language"));
        languages_ = new QActionGroup(this); languages_->setExclusive(true);
        const QStringList languageOptions{"en","de","ko"};
        for (const auto &code : languageOptions) {
            auto *choice=languageMenu->addAction(I18n::languageName(code)); choice->setCheckable(true);
            choice->setData(code); choice->setObjectName("language_"+code); languages_->addAction(choice);
            connect(choice,&QAction::triggered,this,[this,code] { applyLanguage(code,true); });
        }
        auto *appearanceMenu = settings->addMenu(text("Appearance"));
        themes_ = new QActionGroup(this); themes_->setExclusive(true);
        for (const auto &theme : QStringList{"light","dark"}) {
            auto *choice = appearanceMenu->addAction(text(theme=="dark" ? "Dark" : "Light"));
            choice->setCheckable(true);
            choice->setData(theme);
            choice->setObjectName("theme_"+theme);
            themes_->addAction(choice);
            connect(choice,&QAction::triggered,this,[this,theme] { applyTheme(theme,true); });
        }
        auto *help = menuBar()->addMenu(text("&Help"));
        auto action = [&](QMenu *menu, QString name, QKeySequence key, Icons::Symbol icon, std::function<void()> fn, bool needDoc = true) {
            auto *a = menu->addAction(Icons::line(icon),name);
            a->setShortcut(key); a->setToolTip(name);
            connect(a,&QAction::triggered,this,[this,fn] { attempt(fn); });
            if (needDoc) requiresDoc_.append(a);
            return a;
        };
        auto *open = action(file,text("Open…"),QKeySequence::Open,Icons::Symbol::Open,[this] { openDialog(); },false);
        saveDocument_ = action(file,text("Save"),QKeySequence::Save,Icons::Symbol::Save,[this] { save(false); });
        saveAsDocument_ = action(file,text("Save As…"),QKeySequence::SaveAs,Icons::Symbol::SaveAs,[this] { save(true); });
        file->addSeparator();
        closeDocument_=action(file,text("Close Document"),QKeySequence::Close,Icons::Symbol::Close,[this] {
            if (canClose()) { doc_.close(); refresh(); }
        });
        file->addSeparator();
        action(file,text("Exit"),QKeySequence::Quit,Icons::Symbol::Exit,[this] { close(); },false);
        const QSettings penSettings;
        const QColor savedColor(penSettings.value("highlight/color",QString("#5affeb3b")).toString());
        if (savedColor.isValid() && savedColor.alpha()>0) highlightColor_=savedColor;
        highlightWidth_=qBound(1.,penSettings.value("highlight/width",12).toDouble(),72.);
        highlight_=action(edit,text("Highlighter"),{},Icons::Symbol::Highlight,[this] {
            addText_->setChecked(false); view_->setTextPlacement(false); view_->setSignature({});
            view_->setHighlight(highlight_->isChecked(),highlightColor_,highlightWidth_); updateSignatureAction();
            if (highlight_->isChecked()) setStatus("Drag to highlight. Hold Shift for a straight line. Press Esc to finish.");
        });
        highlight_->setCheckable(true); highlight_->setObjectName("highlightAction");
        auto *highlightMenu=new QMenu(this);
        highlightMenu->addAction(highlight_); highlightMenu->addSeparator();
        auto *penSettingsAction=action(highlightMenu,text("Highlighter Settings…"),{},Icons::Symbol::Highlight,[this] { editHighlight(); },false);
        edit->addAction(penSettingsAction);
        view_->highlightCanceled=[this] { highlight_->setChecked(false); view_->setHighlight(false); updateSignatureAction(); setStatus("Ready · Pages: %1",{QString::number(doc_.count())}); };
        view_->highlightPlaced=[this](QPolygonF points) { attempt([this,points] {
            const int page=pages_->currentRow(); doc_.addHighlight(page,points,highlightColor_,highlightWidth_);
            refresh(page); highlight_->setChecked(true); view_->setHighlight(true,highlightColor_,highlightWidth_); updateSignatureAction();
        }); };
        addText_=action(edit,text("Add Text…"),QKeySequence("Ctrl+Shift+T"),Icons::Symbol::Text,[this] {
            highlight_->setChecked(false); view_->setHighlight(false);
            view_->setSignature({}); view_->setTextPlacement(addText_->isChecked());
            if (addText_->isChecked()) setStatus("Click the page to add text. Press Esc to cancel.");
        });
        addText_->setCheckable(true);
        view_->textCanceled=[this] { addText_->setChecked(false); view_->setTextPlacement(false); updateSignatureAction(); setStatus("Text placement canceled."); };
        cancelText_=action(edit,text("Cancel Text Placement"),QKeySequence(Qt::Key_Escape),Icons::Symbol::Cancel,[this] { view_->textCanceled(); });
        view_->textPlaced=[this](QPointF position) { attempt([this,position] { insertText(position); }); };
        selectObjects_=action(edit,text("Select Objects"),{},Icons::Symbol::Select,[this] {
            highlight_->setChecked(false); view_->setHighlight(false);
            addText_->setChecked(false); view_->setTextPlacement(false); view_->setSignature({}); view_->setFocus();
            setStatus("Click an added object to select it. Drag to move; drag a corner to resize.");
        });
        selectObjects_->setCheckable(true); selectObjects_->setObjectName("selectObjects");
        edit->removeAction(addText_); edit->removeAction(cancelText_);
        edit->addAction(addText_); edit->addMenu(sign); edit->addAction(cancelText_); edit->addSeparator();
        editObject_=action(edit,text("Edit Selected…"),QKeySequence(Qt::Key_F2),Icons::Symbol::Signature,[this] { if (!view_->selectedObject().isEmpty()) editObject(view_->selectedObject()); });
        deleteObject_=action(edit,text("Delete Selected Object"),{},Icons::Symbol::Trash,[this] { if (!view_->selectedObject().isEmpty()) removeSelectedObject(); });
        auto *fill=action(edit,text("Fill Form…"),QKeySequence("Ctrl+Shift+F"),Icons::Symbol::Text,[this] { editForms(); });
        fill->setObjectName("fillFormAction");
        view_->formClicked=[this](QPointF point) {
            const int page=pages_->currentRow(); if (!doc_.isOpen() || page<0) return false;
            for (const auto &field:doc_.formFields(page)) if (field.rect.contains(point)) {
                attempt([this,field] { editForms(field.id); }); return true;
            }
            return false;
        };
        previous_ = action(display,text("Previous Page"),QKeySequence("Ctrl+PgUp"),Icons::Symbol::Previous,[this] { navigate(-1); });
        next_ = action(display,text("Next Page"),QKeySequence("Ctrl+PgDown"),Icons::Symbol::Next,[this] { navigate(1); });
        display->addSeparator();
        auto *zoomOut = action(display,text("Zoom Out"),QKeySequence::ZoomOut,Icons::Symbol::ZoomOut,[this] { setDpi(dpi_-14.4); });
        auto *zoomIn = action(display,text("Zoom In"),QKeySequence::ZoomIn,Icons::Symbol::ZoomIn,[this] { setDpi(dpi_+14.4); });
        auto *actual = action(display,text("Actual Size"),QKeySequence("Ctrl+0"),Icons::Symbol::ActualSize,[this] { setDpi(72); });
        auto *fit = action(display,text("Fit Page"),QKeySequence("Ctrl+1"),Icons::Symbol::FitPage,[this] { fitPage(false); });
        auto *width = action(display,text("Fit Width"),QKeySequence("Ctrl+2"),Icons::Symbol::FitWidth,[this] { fitPage(true); });
        display->addSeparator();
        auto *toggleSidebar = display->addAction(Icons::line(Icons::Symbol::Sidebar),text("Page Thumbnails")); toggleSidebar->setCheckable(true); toggleSidebar->setChecked(true);
        connect(toggleSidebar,&QAction::toggled,sidebar_,&QWidget::setVisible);
        moveUp_ = action(organize,text("Move Earlier"),QKeySequence("Alt+Up"),Icons::Symbol::MoveUp,[this] { move(-1); });
        moveDown_ = action(organize,text("Move Later"),QKeySequence("Alt+Down"),Icons::Symbol::MoveDown,[this] { move(1); });
        organize->addSeparator();
        action(organize,text("Blank Page…"),{},Icons::Symbol::NewPage,[this] { insert(false); });
        action(organize,text("Insert from PDF or DOCX…"),{},Icons::Symbol::InsertPdf,[this] { insert(true); });
        organize->addSeparator();
        action(organize,text("Extract Selected Pages…"),{},Icons::Symbol::Extract,[this] { extract(); });
        auto *draw=action(sign,text("Draw Signature…"),{},Icons::Symbol::Signature,[this] { drawSignature(); });
        action(sign,text("Import Signature Image…"),{},Icons::Symbol::Image,[this] {
            auto path=openFileDialog(text("Signature Image"),text("Images (*.png *.jpg *.jpeg *.bmp)"));
            if (path.isEmpty()) return;
            QImage image(path); if (image.isNull()) throw std::runtime_error(text("Unable to read the signature image.").toStdString());
            armSignature(image);
        });
        sign->addSeparator();
        cancelSignature_=action(sign,text("Cancel Signature Placement"),QKeySequence(Qt::Key_Escape),Icons::Symbol::Cancel,[this] {
            if (view_->hasSignature()) view_->setSignature({});

            updateSignatureAction();
            setStatus("Signature placement canceled.");
        });
        action(help,text("About MyPdf"),{},Icons::Symbol::Info,[this] {
            QMessageBox::about(this,text("About MyPdf"),text("<h3>MyPdf %1</h3><p>A PDF editor for organizing pages and adding signatures.</p><p>Signatures are visual marks drawn by hand or imported as images.</p>").arg(QStringLiteral(MYPDF_VERSION)));
        },false);

        action(help,text("Licenses…"),{},Icons::Symbol::Info,[this] { Licenses::show(this); },false)->setObjectName("licensesAction");

        auto *quick=addToolBar(text("Main Tools")); quick->setObjectName("mainTools"); quick->setMovable(false); quick->setFloatable(false);
        quick->setIconSize(QSize(20,20)); quick->setToolButtonStyle(Qt::ToolButtonIconOnly);
        quick->addAction(open); quick->addAction(saveDocument_); quick->addSeparator();
        filename_->setObjectName("filename"); filename_->setTextInteractionFlags(Qt::TextSelectableByMouse);
        filename_->setMinimumWidth(110); filename_->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred); quick->addWidget(filename_);
        quick->addSeparator();
        auto compactTool=[&](QAction *a,const QString &label,const QString &name,QMenu *menu=nullptr) {
            auto *button=new QToolButton; button->setObjectName(name); button->setProperty("compactLabel",true);
            button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon); button->setIconSize(QSize(20,20));
            auto sync=[button,a,label] {
                button->setText(text(label)); button->setEnabled(a->isEnabled()); button->setCheckable(a->isCheckable()); button->setChecked(a->isChecked());
                button->setToolTip(a->toolTip()); button->setIcon(label=="Pages"?Icons::line(Icons::Symbol::Table):a->icon());
            };
            sync(); connect(a,&QAction::changed,button,sync);
            if (menu) {
                button->setProperty("popupMenu",true); button->setMenu(menu); button->setPopupMode(QToolButton::InstantPopup);
            } else connect(button,&QToolButton::clicked,a,&QAction::trigger);
            quick->addWidget(button); return button;
        };
        compactTool(selectObjects_,"Select","selectTool");
        compactTool(addText_,"Text","textTool");
        compactTool(draw,"Sign","signatureTool",sign);
        compactTool(highlight_,"Highlight","highlightTool",highlightMenu);
        requiresDoc_.append(organize->menuAction());
        compactTool(organize->menuAction(),"Pages","pagesTool",organize)->setIcon(Icons::line(Icons::Symbol::Table));
        // Secondary actions appear only when they apply to the current selection or tool.
        auto selectionTool=[&](QAction *a,const QString &name) {
            auto *button=new QToolButton; button->setObjectName(name); button->setIconSize(QSize(20,20));
            auto sync=[button,a] { button->setEnabled(a->isEnabled()); button->setIcon(a->icon()); button->setToolTip(a->toolTip()); };
            sync(); connect(a,&QAction::changed,button,sync); connect(button,&QToolButton::clicked,a,&QAction::trigger);
            return quick->addWidget(button);
        };
        editObjectToolAction_=selectionTool(editObject_,"editObjectTool");
        deleteObjectToolAction_=selectionTool(deleteObject_,"deleteObjectTool");
        cancelPlacement_=quick->addAction(Icons::line(Icons::Symbol::Cancel),text("Cancel")); cancelPlacement_->setObjectName("cancelPlacement");
        connect(cancelPlacement_,&QAction::triggered,this,[this] { if (highlight_->isChecked()) view_->highlightCanceled(); else if (addText_->isChecked()) view_->textCanceled(); else cancelSignature_->trigger(); });
        quick->setContextMenuPolicy(Qt::PreventContextMenu);

        auto *split=new QSplitter;
        auto *sideLayout=new QVBoxLayout(sidebar_); sideLayout->setContentsMargins(0,0,0,0); sideLayout->setSpacing(0);
        auto *sideTitle=new QLabel(text("Page Thumbnails")); sideTitle->setObjectName("sideTitle"); sideLayout->addWidget(sideTitle);
        pages_->setObjectName("pageList");
        pages_->setSelectionMode(QAbstractItemView::ExtendedSelection);
        pages_->setViewMode(QListView::ListMode); pages_->setFlow(QListView::TopToBottom);
        pages_->setWrapping(false); pages_->setUniformItemSizes(true);
        pages_->setResizeMode(QListView::Adjust); pages_->setMovement(QListView::Static);
        pages_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        pages_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        pages_->setItemDelegate(new ThumbnailDelegate(pages_));
        pages_->setContextMenuPolicy(Qt::CustomContextMenu); sideLayout->addWidget(pages_);
        auto *selectionTip=new QLabel(text("Ctrl / Shift: select multiple pages")); selectionTip->setObjectName("selectionTip"); selectionTip->setWordWrap(true); sideLayout->addWidget(selectionTip);
        sidebar_->setMinimumWidth(190); sidebar_->setMaximumWidth(270);
        scroll_->setWidget(view_); scroll_->setAlignment(Qt::AlignCenter); scroll_->setObjectName("documentCanvas");
        split->addWidget(sidebar_); split->addWidget(scroll_); split->setStretchFactor(1,1); split->setSizes({210,990});
        auto *welcome=new QWidget; auto *welcomeLayout=new QVBoxLayout(welcome); welcomeLayout->addStretch();
        if (!qApp->windowIcon().isNull()) {
            auto *mascot=new QLabel;
            mascot->setPixmap(qApp->windowIcon().pixmap(112,112));
            welcomeLayout->addWidget(mascot,0,Qt::AlignHCenter);
        }
        auto *welcomeTitle=new QLabel("MyPdf"); welcomeTitle->setAlignment(Qt::AlignCenter); welcomeTitle->setObjectName("welcomeTitle"); welcomeLayout->addWidget(welcomeTitle);
        auto *welcomeText=new QLabel(text("Open a PDF or import a DOCX to edit it as PDF.")); welcomeText->setAlignment(Qt::AlignCenter); welcomeLayout->addWidget(welcomeText);
        auto *openButton=new QPushButton(Icons::line(Icons::Symbol::Open,Qt::white),text("Open Document…")); openButton->setObjectName("openButton"); openButton->setFixedSize(200,44);
        connect(openButton,&QPushButton::clicked,open,&QAction::trigger); welcomeLayout->addWidget(openButton,0,Qt::AlignHCenter); welcomeLayout->addStretch();
        workspace_->addWidget(welcome); workspace_->addWidget(split);
        importNotice_->setText(text("Imported DOCX as PDF. Check the layout before saving; the Word original is unchanged."));
        importNotice_->setWordWrap(true); importNotice_->setContentsMargins(12,8,12,8);
        layout->addWidget(importNotice_); layout->addWidget(workspace_,1);

        status_->setMinimumWidth(0); status_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred); statusBar()->addWidget(status_,1);
        auto *navigation=new QWidget; navigation->setObjectName("pageNavigation"); auto *navLayout=new QHBoxLayout(navigation); navLayout->setContentsMargins(0,0,0,0); navLayout->setSpacing(6);
        for (auto *a:{previous_,next_}) { auto *b=new QToolButton; b->setIconSize(QSize(20,20)); b->setDefaultAction(a); navLayout->addWidget(b); }
        pageNumber_->setObjectName("pageNumber");
        pageNumber_->setKeyboardTracking(false); pageNumber_->setButtonSymbols(QAbstractSpinBox::NoButtons); pageNumber_->setFixedWidth(56); pageNumber_->setAlignment(Qt::AlignCenter);
        navLayout->addWidget(pageNumber_); navLayout->addWidget(pageTotal_); statusBar()->addPermanentWidget(navigation);
        auto *zoomTools=new QWidget; zoomTools->setObjectName("zoomTools");
        auto *zoomLayout=new QHBoxLayout(zoomTools); zoomLayout->setContentsMargins(0,0,0,0); zoomLayout->setSpacing(4);
        quick->addSeparator(); quick->addWidget(zoomTools);
        zoom_->setObjectName("zoomLevel");
        zoom_->addItems({"50%","75%","100%","125%","150%","200%","300%"}); zoom_->setEditable(true); zoom_->setInsertPolicy(QComboBox::NoInsert); zoom_->setFixedWidth(90);
        auto navigationButton=[&](QAction *a,const QString &name) { auto *button=new QToolButton; button->setObjectName(name); button->setDefaultAction(a); button->setIconSize(QSize(18,18)); zoomLayout->addWidget(button); return button; };
        navigationButton(zoomOut,"zoomOutTool");
        zoomLayout->addWidget(zoom_);
        navigationButton(zoomIn,"zoomInTool");
        auto *fitMenu=new QMenu(this); fitMenu->addAction(fit); fitMenu->addAction(width); fitMenu->addAction(actual);
        auto *fitButton=new QToolButton; fitButton->setObjectName("fitTool"); fitButton->setIcon(fit->icon()); fitButton->setIconSize(QSize(18,18));
        fitButton->setProperty("popupMenu",true); fitButton->setMenu(fitMenu); fitButton->setPopupMode(QToolButton::InstantPopup);
        auto syncFit=[fitButton,fit] { fitButton->setEnabled(fit->isEnabled()); fitButton->setToolTip(fit->toolTip()); };
        syncFit(); connect(fit,&QAction::changed,fitButton,syncFit);
        zoomLayout->addWidget(fitButton);
        connect(pageNumber_,&QSpinBox::valueChanged,this,[this](int page) { if (!refreshing_) pages_->setCurrentRow(page-1); });
        connect(zoom_,&QComboBox::activated,this,[this] { applyZoom(); });
        connect(zoom_->lineEdit(),&QLineEdit::returnPressed,this,[this] { applyZoom(); });
        connect(pages_,&QListWidget::currentRowChanged,this,[this] { if (!refreshing_) attempt([this]{ view_->setSignature({}); showPage(); }); });
        connect(pages_,&QListWidget::customContextMenuRequested,this,[this,organize](QPoint pos) { if (doc_.isOpen()) organize->exec(pages_->viewport()->mapToGlobal(pos)); });
        connect(&thumbnails_,&QTimer::timeout,this,[this] {
            if (thumbnailRow_>=doc_.count()) { thumbnails_.stop(); return; }
            try {
                const qreal scale=qMax(qreal(2),pages_->devicePixelRatioF());
                QImage image=doc_.render(thumbnailRow_,24*scale);
                image.setDevicePixelRatio(scale);
                pages_->item(thumbnailRow_)->setIcon(QPixmap::fromImage(image)); ++thumbnailRow_;
            }
            catch (...) { thumbnails_.stop(); }
        });
        view_->setObjectName("pageView");
        view_->signatureStateChanged=[this] { updateSignatureAction(); };
        view_->selectionChanged=[this] { updateObjectActions(); };
        view_->objectContextMenu=[this](QString,QPoint position) {
            QMenu menu(this); menu.addAction(editObject_); menu.addAction(deleteObject_); menu.exec(view_->mapToGlobal(position));
        };
        view_->objectMoved=[this](QString id,QPointF position) { attempt([this,id,position] {
            doc_.moveObject(id,position); refresh(pages_->currentRow()); view_->selectObject(id); view_->setFocus();
        }); };
        view_->objectEdited=[this](QString id) { attempt([this,id] { editObject(id); }); };
        view_->objectResized=[this](QString id,QRectF rect) { attempt([this,id,rect] {
            doc_.resizeObject(id,rect); refresh(pages_->currentRow()); view_->selectObject(id); view_->setFocus();
        }); };
        view_->objectRemoved=[this](QString) { attempt([this] { removeSelectedObject(); }); };
        view_->placed=[this](QImage image,QRectF r) { attempt([&] { const int page=pages_->currentRow(); doc_.sign(page,image,r); refresh(page); view_->selectObject(doc_.objects(page).last().id); view_->setFocus(); }); };
        captureTranslations();
        applyLanguage(QSettings().value("ui/language","en").toString(),false);
        applyTheme(QSettings().value("ui/theme","light").toString(),false);
        updateToolbarLayout();
        refresh();
    }
    void openPath(const QString &path) {
        attempt([&] {
            const auto extension=QFileInfo(path).suffix().toLower();
            if (extension=="doc") throw std::runtime_error(text("Legacy DOC files are not supported. Open a DOCX file instead.").toStdString());
            if (!canClose()) return;
            if (extension=="docx") doc_.importDocx(path);
            else doc_.open(path);
            refresh();
            QTimer::singleShot(0,this,[this] { if (doc_.isOpen()) attempt([this] { fitPage(false); }); });
        });
    }

protected:
    void resizeEvent(QResizeEvent *event) override { QMainWindow::resizeEvent(event); updateToolbarLayout(); }
    bool event(QEvent *event) override {
        const bool handled=QMainWindow::event(event);
        if (event->type()==QEvent::ScreenChangeInternal) {
            QTimer::singleShot(0,this,[this] {
                attempt([this] { showPage(); });
                thumbnailRow_=0;
                if (doc_.isOpen()) thumbnails_.start(15);
            });
        }
        return handled;
    }
    void closeEvent(QCloseEvent *e) override { if (qEnvironmentVariableIsSet("MYPDF_SMOKE_TEST") || canClose()) e->accept(); else e->ignore(); }
private:
    void updateToolbarLayout() {
        const bool compact=width()<1100;
        filename_->setMinimumWidth(compact?40:110); filename_->setMaximumWidth(compact?80:200);
        filename_->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
        for (auto *button:findChildren<QToolButton *>()) if (button->property("compactLabel").toBool())
            button->setToolButtonStyle(compact?Qt::ToolButtonIconOnly:Qt::ToolButtonTextBesideIcon);
    }
    void captureTranslations() {
        auto bind=[this](QObject *object,const char *property) {
            auto value=object->property(property).toString();
            if (!value.isEmpty()) translationBindings_.append({object,property,I18n::sourceText(value)});
        };
        for (auto *a:findChildren<QAction *>()) {
            if (!a->objectName().startsWith("language_")) { bind(a,"text"); bind(a,"toolTip"); }
        }
        for (auto *label:findChildren<QLabel *>())
            if (label!=filename_ && label!=status_) bind(label,"text");
        for (auto *button:findChildren<QPushButton *>()) bind(button,"text");
        for (auto *bar:findChildren<QToolBar *>()) bind(bar,"windowTitle");
        for (auto *button:findChildren<QToolButton *>()) if (button->property("compactLabel").toBool()) bind(button,"text");
    }
    void applyLanguage(const QString &code,bool persist) {
        I18n::setLanguage(code);
        if (persist) QSettings().setValue("ui/language",I18n::language());
        for (const auto &binding:translationBindings_)
            if (binding.object) binding.object->setProperty(binding.property.constData(),text(binding.source));
        for (auto *a:languages_->actions()) a->setChecked(a->data().toString()==I18n::language());
        for (int i=0;i<pages_->count();++i) pages_->item(i)->setText(text("Page %1").arg(i+1));
        setStatus(statusKey_,statusArgs_);
        title(); updateToolbarLayout();
    }
    void applyTheme(const QString &theme,bool persist) {
        const QString selected = theme=="dark" ? "dark" : "light";
        if (persist) QSettings().setValue("ui/theme",selected);
        applyApplicationTheme(selected=="dark");
        for (auto *action:themes_->actions())
            action->setChecked(action->data().toString()==selected);
    }
    void setStatus(QString source,QStringList args={}) {
        statusKey_=I18n::sourceText(source); statusArgs_=args;
        QString message=text(statusKey_);
        for (const auto &value:args) message=message.arg(value);
        status_->setText(message);
    }
    void updateObjectActions() {
        const bool selected=doc_.isOpen() && !view_->selectedObject().isEmpty();
        deleteObject_->setEnabled(selected); editObject_->setEnabled(selected);
        editObjectToolAction_->setVisible(selected); deleteObjectToolAction_->setVisible(selected);
    }
    void updateSignatureAction() {
        const bool placingText=doc_.isOpen() && addText_->isChecked();
        const bool placingSignature=doc_.isOpen() && !addText_->isChecked() && view_->hasSignature();
        cancelText_->setVisible(placingText); cancelText_->setEnabled(placingText);
        cancelSignature_->setVisible(placingSignature); cancelSignature_->setEnabled(placingSignature);
        selectObjects_->setChecked(!highlight_->isChecked() && !addText_->isChecked() && !view_->hasSignature());
        cancelPlacement_->setVisible(doc_.isOpen() && (highlight_->isChecked() || addText_->isChecked() || view_->hasSignature()));
    }
    void editForms(int focusId=-1) {
        const int page=pages_->currentRow();
        if (Forms::edit(doc_,page,this,focusId)) {
            refresh(page); setStatus("Form values updated. Save to keep your changes.");
        }
    }
    void removeSelectedObject() {
        const auto id=view_->selectedObject(); if (id.isEmpty()) return;
        doc_.removeObject(id); refresh(pages_->currentRow()); view_->setFocus();
        setStatus("Object deleted.");
    }
    void navigate(int delta) { pages_->setCurrentRow(qBound(0,pages_->currentRow()+delta,doc_.count()-1)); }
    void setDpi(double dpi) { dpi_=qBound(28.8,dpi,216.); showPage(); }
    void applyZoom() {
        attempt([this] {
            QString text=zoom_->currentText(); text.remove('%'); bool ok=false; double value=text.toDouble(&ok);
            if (ok && value>=40 && value<=300) setDpi(value*.72); else showPage();
        });
    }
    void fitPage(bool widthOnly) {
        auto size=doc_.size(pages_->currentRow()); auto available=scroll_->viewport()->size()-QSize(36,36);
        double scale=available.width()/size.width();
        if (!widthOnly) scale=qMin(scale,available.height()/size.height());
        setDpi(scale*72);
    }
    void attempt(const std::function<void()> &fn) {
        try { fn(); } catch (const std::exception &e) { QMessageBox::critical(this,"MyPdf",QString::fromUtf8(e.what())); }
    }
    void title() {
        const QString path=doc_.path().isEmpty()?doc_.suggestedPath():doc_.path();
        filename_->setText(doc_.isOpen() ? QFileInfo(path).fileName() + (doc_.dirty()?" •":"") : text("Open a document to get started"));
        filename_->setToolTip(doc_.importedFrom().isEmpty()?path:text("Imported from: %1").arg(doc_.importedFrom()));
        setWindowTitle((doc_.isOpen() ? QFileInfo(path).fileName() + (doc_.dirty()?" *":"") + " — " : "") + "MyPdf");
    }
    QString openFileDialog(const QString &caption,const QString &filter) {
        QFileDialog dialog(this,caption,{},filter);
        Icons::configureFileDialog(dialog);
        dialog.setFileMode(QFileDialog::ExistingFile);
        return dialog.exec()==QDialog::Accepted ? dialog.selectedFiles().value(0) : QString();
    }
    QString saveDialog(const QString &caption, const QString &initial = {}) {
        QFileDialog dialog(this,caption,initial,"PDF (*.pdf)");
        Icons::configureFileDialog(dialog);
        dialog.setAcceptMode(QFileDialog::AcceptSave);
        dialog.setDefaultSuffix("pdf");
        return dialog.exec()==QDialog::Accepted ? dialog.selectedFiles().value(0) : QString();
    }
    bool save(bool as) {
        QString path = doc_.path();
        if (as || path.isEmpty()) path = saveDialog(text("Save PDF"),path.isEmpty()?doc_.suggestedPath():path);
        if (path.isEmpty()) return false;
        doc_.save(path); updateSignatureAction(); title(); setStatus("Saved: %1",{path}); return true;
    }
    bool canClose() {
        if (!doc_.dirty()) return true;
        auto result = QMessageBox::question(this,text("Save Changes"),text("Do you want to save your changes?"),QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel,QMessageBox::Save);
        if (result == QMessageBox::Cancel) return false;
        if (result == QMessageBox::Discard) return true;
        try { return save(false); } catch (const std::exception &e) { QMessageBox::critical(this,text("Save Failed"),QString::fromUtf8(e.what())); return false; }
    }
    void openDialog() { auto p=openFileDialog(text("Open Document"),text("Supported documents (*.pdf *.docx);;PDF (*.pdf);;Word documents (*.docx)")); if (!p.isEmpty()) openPath(p); }
    void refresh(int page=0) {
        highlight_->setChecked(false); view_->setHighlight(false);
        addText_->setChecked(false); view_->setTextPlacement(false);
        thumbnails_.stop(); thumbnailRow_=0;
        refreshing_=true; pages_->clear();
        for (int i=0;i<doc_.count();++i) pages_->addItem(text("Page %1").arg(i+1));
        if (doc_.isOpen()) pages_->setCurrentRow(qBound(0,page,doc_.count()-1));
        pageNumber_->setRange(doc_.isOpen()?1:0,doc_.count());
        pageTotal_->setText(QString("/ %1").arg(doc_.count()));
        workspace_->setCurrentIndex(doc_.isOpen()?1:0);
        importNotice_->setVisible(!doc_.importedFrom().isEmpty());
        pageNumber_->setEnabled(doc_.isOpen()); zoom_->setEnabled(doc_.isOpen());
        refreshing_=false;
        if (doc_.isOpen()) thumbnails_.start(15);
        for (auto *a:requiresDoc_) a->setEnabled(doc_.isOpen());
        const bool opened=doc_.isOpen();
        closeDocument_->setEnabled(opened); saveDocument_->setEnabled(opened); saveAsDocument_->setEnabled(opened);
        view_->setSignature({}); title(); showPage();
    }
    void showPage() {
        int row=pages_->currentRow();
        if (!doc_.isOpen() || row<0) { view_->setFormFields({}); view_->setObjects({}); view_->setPage({}); setStatus("Open a PDF or DOCX file to get started."); return; }
        const qreal scale=qMax(qreal(2),view_->devicePixelRatioF());
        QImage image=doc_.renderBase(row,dpi_*scale);
        if (image.isNull()) throw std::runtime_error(text("Unable to display this page.").toStdString());
        image.setDevicePixelRatio(scale);
        view_->setPage(image); view_->setPagePoints(doc_.size(row)); QHash<QString,QImage> layers;
        const auto objects=doc_.objects(row);
        for (const auto &object:objects) layers.insert(object.id,doc_.renderObject(object,dpi_*scale));
        view_->setObjects(objects,layers); view_->setFormFields(doc_.formFields(row));
        { QSignalBlocker block(pageNumber_); pageNumber_->setValue(row+1); }
        { QSignalBlocker block(zoom_); zoom_->setCurrentText(QString::number(qRound(dpi_/72*100))+"%"); }
        previous_->setEnabled(row>0); next_->setEnabled(row<doc_.count()-1);
        moveUp_->setEnabled(row>0); moveDown_->setEnabled(row<doc_.count()-1);
        setStatus("Ready · Pages: %1",{QString::number(doc_.count())});
    }
    void move(int delta) {
        int from=pages_->currentRow(), to=from+delta;
        if (to<0 || to>=doc_.count()) return;
        doc_.move(from,to); refresh(to);
    }
    void insertText(QPointF position, const QString &id={}) {
        PageObject existing;
        if (!id.isEmpty()) for (const auto &object:doc_.objects(pages_->currentRow())) if (object.id==id) existing=object;
        const bool editing=!id.isEmpty();
        if (editing && (existing.id.isEmpty() || existing.isHighlight() || !existing.image.isNull())) return;
        QFont initialFont=existing.font;
        if (editing && existing.textSize.height()>0) initialFont.setPointSizeF(existing.font.pointSizeF()*existing.rect.height()*doc_.size(existing.page).height()/existing.textSize.height());
        QDialog dialog(this); dialog.setWindowTitle(text(editing?"Edit Text…":"Add Text…"));
        auto *layout=new QVBoxLayout(&dialog);
        auto *input=new QTextEdit; input->setObjectName("insertTextInput"); input->setAcceptRichText(false);
        layout->addWidget(new QLabel(text("Text"))); layout->addWidget(input);
        auto *form=new QFormLayout; layout->addLayout(form);
        auto *font=new QFontComboBox; font->setObjectName("textFont"); if (editing) font->setCurrentFont(initialFont); const QString initialFamily=font->currentFont().family(); form->addRow(text("Font"),font);
        auto *size=new QDoubleSpinBox; size->setObjectName("textSize"); size->setDecimals(2); size->setRange(editing?.1:1,editing?10000:144); size->setValue(editing?initialFont.pointSizeF():12); const double initialSize=size->value(); form->addRow(text("Size (pt)"),size);
        QColor color=editing?existing.color:QColor(Qt::black); auto *colorButton=new QPushButton(color.name()); form->addRow(text("Color"),colorButton);
        connect(colorButton,&QPushButton::clicked,&dialog,[&] {
            const auto selected=QColorDialog::getColor(color,&dialog,text("Color"));
            if (selected.isValid()) { color=selected; colorButton->setText(color.name()); }
        });
        auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel); layout->addWidget(buttons);
        buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
        connect(input,&QTextEdit::textChanged,&dialog,[=] { buttons->button(QDialogButtonBox::Ok)->setEnabled(!input->toPlainText().trimmed().isEmpty() && input->toPlainText().size()<=10000); });
        connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
        connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
        if (editing) input->setPlainText(existing.text);
        dialog.resize(420,320); input->setFocus();
        if (dialog.exec()!=QDialog::Accepted) return;
        QFont selected=editing?initialFont:font->currentFont(); selected.setFamily(editing && font->currentFont().family()==initialFamily?initialFont.family():font->currentFont().family()); selected.setPointSizeF(size->value());
        if (editing && input->toPlainText()==existing.text && selected.family()==initialFont.family() && size->value()==initialSize && color==existing.color) return;
        const int page=pages_->currentRow();
        if (editing) doc_.updateText(id,input->toPlainText(),selected,color);
        else doc_.addText(page,input->toPlainText(),selected,color,position);
        refresh(page); view_->selectObject(editing?id:doc_.objects(page).last().id); view_->setFocus();
        if (editing) setStatus("Object updated."); else setStatus("Text inserted. Drag to move; drag a corner to resize.");
    }
    void editObject(const QString &id) {
        for (const auto &object:doc_.objects(pages_->currentRow())) if (object.id==id) {
            if (object.isHighlight()) editHighlight(object.id);
            else if (object.image.isNull()) insertText(object.rect.topLeft(),id);
            else editSignature(object);
            return;
        }
    }
    void editHighlight(const QString &id={}) {
        PageObject existing;
        if (!id.isEmpty()) for (const auto &object:doc_.objects(pages_->currentRow())) if (object.id==id) existing=object;
        if (!id.isEmpty() && !existing.isHighlight()) return;
        QColor color=id.isEmpty()?highlightColor_:existing.color;
        const int initialAlpha=color.alpha(), initialOpacity=qRound(color.alphaF()*100);
        QDialog dialog(this); dialog.setObjectName("highlightSettings"); dialog.setWindowTitle(text("Highlighter Settings…"));
        auto *layout=new QVBoxLayout(&dialog); auto *form=new QFormLayout; layout->addLayout(form);
        auto *colors=new QComboBox; colors->setObjectName("highlightColor");
        const QList<QPair<QString,QColor>> presets{{"Yellow",QColor("#ffeb3b")},{"Green",QColor("#76ff03")},{"Pink",QColor("#ff80ab")},{"Blue",QColor("#80d8ff")},{"Orange",QColor("#ffab40")}};
        for (const auto &preset:presets) { QPixmap swatch(20,20); swatch.fill(preset.second); colors->addItem(QIcon(swatch),text(preset.first),preset.second); }
        colors->addItem(text("Custom Color…"));
        int current=colors->count()-1;
        for (int i=0;i<presets.size();++i) if (presets[i].second.rgb()==color.rgb()) current=i;
        colors->setCurrentIndex(current); form->addRow(text("Color"),colors);
        connect(colors,&QComboBox::activated,&dialog,[&](int index) {
            const QColor selected=index<presets.size()?presets[index].second:QColorDialog::getColor(color,&dialog,text("Color"));
            if (selected.isValid()) color=selected;
        });
        auto *width=new QDoubleSpinBox; width->setObjectName("highlightWidth"); width->setRange(1,72); width->setDecimals(1); width->setValue(id.isEmpty()?highlightWidth_:existing.strokeWidth); form->addRow(text("Thickness (pt)"),width);
        auto *opacity=new QSpinBox; opacity->setObjectName("highlightOpacity"); opacity->setRange(5,80); opacity->setSuffix("%"); opacity->setValue(qRound(color.alphaF()*100)); form->addRow(text("Opacity"),opacity);
        auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel); layout->addWidget(buttons);
        connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept); connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
        if (dialog.exec()!=QDialog::Accepted) return;
        color.setAlpha(opacity->value()==initialOpacity?initialAlpha:qRound(opacity->value()*2.55));
        if (!id.isEmpty()) { doc_.updateHighlight(id,color,width->value()); refresh(existing.page); view_->selectObject(id); view_->setFocus(); }
        else {
            highlightColor_=color; highlightWidth_=width->value();
            QSettings settings; settings.setValue("highlight/color",color.name(QColor::HexArgb)); settings.setValue("highlight/width",highlightWidth_);
            if (highlight_->isChecked()) view_->setHighlight(true,highlightColor_,highlightWidth_);
        }
    }
    void editSignature(const PageObject &object) {
        QDialog dialog(this); dialog.setWindowTitle(text("Edit Signature…"));
        auto *layout=new QVBoxLayout(&dialog);
        layout->addWidget(new QLabel(text("Draw on the existing signature, clear it to draw again, or replace the image.")));
        auto *pad=new InkPad; pad->setObjectName("signatureEditor"); pad->setImage(object.image); layout->addWidget(pad);
        auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
        auto *clear=buttons->addButton(text("Clear"),QDialogButtonBox::ResetRole); clear->setObjectName("clearSignature");
        auto *replace=buttons->addButton(text("Replace Image…"),QDialogButtonBox::ActionRole); replace->setObjectName("replaceSignature");
        QImage replacement;
        connect(clear,&QPushButton::clicked,pad,&InkPad::clear);
        connect(replace,&QPushButton::clicked,&dialog,[&] { attempt([&] {
            const auto path=openFileDialog(text("Signature Image"),text("Images (*.png *.jpg *.jpeg *.bmp)"));
            if (path.isEmpty()) return;
            QImage image(path); if (image.isNull()) throw std::runtime_error(text("Unable to read the signature image.").toStdString());
            replacement=image; pad->setImage(image);
        }); });
        connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
        connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject); layout->addWidget(buttons);
        if (dialog.exec()!=QDialog::Accepted || (!pad->modified() && replacement.isNull())) return;
        QImage image=pad->modified()?pad->image():replacement;
        if (pad->modified()) {
            QRect bounds;
            for (int y=0;y<image.height();++y) for (int x=0;x<image.width();++x) if (qAlpha(image.pixel(x,y))) bounds=bounds.united(QRect(x,y,1,1));
            if (bounds.isEmpty()) throw std::runtime_error(text("Draw a signature before confirming.").toStdString());
            image=image.copy(bounds.adjusted(-3,-3,3,3).intersected(image.rect()));
        }
        doc_.updateSignature(object.id,image); refresh(object.page); view_->selectObject(object.id); view_->setFocus(); setStatus("Object updated.");
    }
    void insert(bool fromDocument) {
        bool ok=false;
        int pos=QInputDialog::getInt(this,text("Insert Pages"),text("Insert before page (last number appends to the document):"),pages_->currentRow()+1,1,doc_.count()+1,1,&ok);
        if (!ok) return;
        bool insertedWord=false;
        if (fromDocument) {
            auto path=openFileDialog(text("Document to Insert"),text("Supported documents (*.pdf *.docx);;PDF (*.pdf);;Word documents (*.docx)")); if (path.isEmpty()) return;
            insertedWord=QFileInfo(path).suffix().compare("docx",Qt::CaseInsensitive)==0;
            doc_.insertDocument(path,pos-1);
        } else doc_.insertBlank(pos-1);
        refresh(pos-1);
        if (insertedWord) setStatus("DOCX pages inserted. Check the layout before saving.");
    }
    void extract() {
        QList<int> selected;
        for (auto *item:pages_->selectedItems()) selected.append(pages_->row(item));
        std::sort(selected.begin(),selected.end());
        if (selected.isEmpty()) return;
        auto path=saveDialog(text("Extract Selected Pages to a New PDF"));
        if (path.isEmpty()) return;
        if (QFileInfo(path).absoluteFilePath()==doc_.path()) throw std::runtime_error(text("Save the extracted pages under a different name from the original.").toStdString());
        doc_.extract(selected,path); setStatus("Selected pages extracted: %1",{path});
    }
    void armSignature(QImage image) {
        highlight_->setChecked(false); view_->setHighlight(false);
        addText_->setChecked(false); view_->setTextPlacement(false);
        view_->setSignature(std::move(image)); setStatus("Drag a rectangle on the page to place the signature. Esc: cancel");
    }
    void drawSignature() {
        QDialog dialog(this); dialog.setWindowTitle(text("Draw Signature"));
        auto *layout=new QVBoxLayout(&dialog); layout->addWidget(new QLabel(text("Draw your signature. After confirming, drag its placement area on the page.")));
        auto *pad=new InkPad; layout->addWidget(pad);
        auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
        auto *clear=buttons->addButton(text("Clear"),QDialogButtonBox::ResetRole);
        connect(clear,&QPushButton::clicked,pad,&InkPad::clear);
        connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
        connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject); layout->addWidget(buttons);
        if (dialog.exec()!=QDialog::Accepted) return;
        QImage image=pad->image(); QRect bounds;
        for (int y=0;y<image.height();++y) for (int x=0;x<image.width();++x)
            if (qAlpha(image.pixel(x,y))) bounds=bounds.united(QRect(x,y,1,1));
        if (bounds.isEmpty()) return;
        armSignature(image.copy(bounds.adjusted(-3,-3,3,3).intersected(image.rect())));
    }
};
#ifndef MYPDF_NO_MAIN
int main(int argc,char **argv) {
    QApplication app(argc,argv); app.setApplicationVersion(QStringLiteral(MYPDF_VERSION)); app.setApplicationName("MyPdf"); app.setOrganizationName("MyPdf");
    QSettings settings;
    QSettings previous(QStringLiteral("MyPDF"),QStringLiteral("MyPDF"));
    for (const auto &key:QStringList{"ui/language","ui/theme"})
        if (!settings.contains(key) && previous.contains(key)) settings.setValue(key,previous.value(key));
    app.setDesktopFileName("MyPdf");
    app.setWindowIcon(QIcon(":/icons/MyPdf.png"));
    Icons::installStyle();
    Window window; window.show();
    if (argc>1) window.openPath(QString::fromLocal8Bit(argv[1]));
    if (qEnvironmentVariableIsSet("MYPDF_SMOKE_TEST")) QTimer::singleShot(500,&app,[&] {
        auto screenshot=qEnvironmentVariable("MYPDF_SCREENSHOT");
        if (!screenshot.isEmpty()) window.grab().save(screenshot);
        app.quit();
    });
    return app.exec();
}

#endif
