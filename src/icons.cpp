#include "icons.h"
#include <QIconEngine>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <utility>
#include <initializer_list>
#include <QDir>
#include <QApplication>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QFileDialog>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QPalette>

namespace {
class LineIconEngine final : public QIconEngine {
    Icons::Symbol symbol_;
    QColor color_;
    bool followsPalette_;
public:
    LineIconEngine(Icons::Symbol symbol, QColor color)
        : symbol_(symbol), color_(std::move(color)), followsPalette_(!color_.isValid()) {}
    QIconEngine *clone() const override { return new LineIconEngine(symbol_, followsPalette_ ? QColor() : color_); }
    QString key() const override { return QStringLiteral("MyPdf.LineIcon"); }
    void paint(QPainter *p, const QRect &rect, QIcon::Mode mode, QIcon::State) override {
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        const qreal side=qMin(rect.width(),rect.height());
        p->translate(rect.x()+(rect.width()-side)/2.,rect.y()+(rect.height()-side)/2.);
        p->scale(side/24.,side/24.);
        const auto color = mode==QIcon::Disabled
            ? QApplication::palette().color(QPalette::Disabled,QPalette::ButtonText)
            : followsPalette_ ? QApplication::palette().color(QPalette::ButtonText) : color_;
        p->setPen(QPen(color,1.75,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
        p->setBrush(Qt::NoBrush);
        auto path=[&](std::initializer_list<QPointF> points) {
            QPainterPath shape; bool first=true;
            for (auto point:points) { if (first) { shape.moveTo(point); first=false; } else shape.lineTo(point); }
            p->drawPath(shape);
        };
        auto page=[&] {
            path({{14,3},{5,3},{5,21},{19,21},{19,8},{14,3},{14,8},{19,8}});
        };
        auto magnifier=[&] {
            p->drawEllipse(QRectF(3,3,13,13)); p->drawLine(QPointF(14,14),QPointF(21,21));
            p->drawLine(QPointF(6.5,9.5),QPointF(12.5,9.5));
        };
        using S=Icons::Symbol;
        switch (symbol_) {
        case S::Select:
            path({{5,3},{6,20},{10,15},{14,22},{18,20},{14,13},{21,12},{5,3}}); break;
        case S::Open:
            path({{3,10},{3,5},{9,5},{11,7},{20,7},{20,10}});
            path({{3,10},{22,10},{19,20},{2,20},{3,10}}); break;
        case S::Save: case S::SaveAs:
            path({{20,13},{20,7},{16,3},{4,3},{4,21},{13,21}});
            path({{8,3},{8,9},{15,9},{15,3}});
            if (symbol_==S::Save) {
                path({{13,21},{20,21},{20,13}}); p->drawRect(QRectF(8,14,8,7));
            } else {
                path({{15,17},{19,13},{22,16},{18,20},{14,21},{15,17},{18,20}});
            } break;
        case S::Close: case S::Cancel:
            p->drawLine(6,6,18,18); p->drawLine(18,6,6,18); break;
        case S::Exit:
            path({{10,4},{4,4},{4,20},{10,20}}); path({{14,7},{19,12},{14,17}}); p->drawLine(9,12,20,12); break;
        case S::Previous: path({{15,5},{8,12},{15,19}}); break;
        case S::Next: path({{9,5},{16,12},{9,19}}); break;
        case S::ZoomOut: case S::ZoomIn:
            magnifier(); if (symbol_==S::ZoomIn) p->drawLine(QPointF(9.5,6.5),QPointF(9.5,12.5)); break;
        case S::ActualSize:
            path({{3,8},{5,6},{5,17}}); path({{16,8},{18,6},{18,17}});
            p->drawPoint(11,9); p->drawPoint(11,15); break;
        case S::FitPage:
            p->drawRect(QRectF(8,6,8,12));
            path({{3,8},{3,3},{8,3}}); path({{16,3},{21,3},{21,8}});
            path({{21,16},{21,21},{16,21}}); path({{8,21},{3,21},{3,16}}); break;
        case S::FitWidth:
            p->drawLine(3,4,3,20); p->drawLine(21,4,21,20);
            path({{8,8},{4,12},{8,16}}); path({{16,8},{20,12},{16,16}}); p->drawLine(4,12,20,12); break;
        case S::MoveUp: path({{5,11},{12,4},{19,11}}); p->drawLine(12,4,12,20); break;
        case S::MoveDown: path({{5,13},{12,20},{19,13}}); p->drawLine(12,4,12,20); break;
        case S::NewPage:
            page(); p->drawLine(9,14,15,14); p->drawLine(12,11,12,17); break;
        case S::InsertPdf:
            path({{14,3},{5,3},{5,8}}); path({{5,17},{5,21},{19,21},{19,8},{14,3},{14,8},{19,8}});
            p->drawLine(2,12,13,12); path({{9,8},{13,12},{9,16}}); break;
        case S::Extract:
            path({{14,3},{5,3},{5,21},{19,21},{19,17}}); path({{14,3},{14,8},{19,8}});
            p->drawLine(10,13,22,13); path({{18,9},{22,13},{18,17}}); break;
        case S::Text:
            p->drawLine(5,5,19,5); p->drawLine(12,5,12,20); p->drawLine(8,20,16,20); break;
        case S::Signature:
            path({{4,16},{5,11},{15,2},{20,7},{10,17},{4,18},{4,16}}); p->drawLine(12,5,17,10);
            { QPainterPath stroke; stroke.moveTo(3,22); stroke.cubicTo(7,18,10,24,14,21); stroke.cubicTo(16,19,18,23,22,20); p->drawPath(stroke); } break;
        case S::Image:
            p->drawRoundedRect(QRectF(3,3,18,18),2,2); p->drawEllipse(QRectF(7,6,3,3));
            path({{3,17},{9,11},{14,16},{17,13},{21,17}}); break;
        case S::Info:
            p->drawEllipse(QRectF(3,3,18,18)); p->drawPoint(12,7); p->drawLine(12,11,12,17); break;
        case S::Document:
            page(); p->drawLine(9,12,15,12); p->drawLine(9,16,15,16); break;
        case S::Folder: case S::NewFolder:
            path({{3,20},{3,5},{9,5},{11,7},{21,7},{21,20},{3,20}});
            if (symbol_==S::NewFolder) { p->drawLine(9,14,15,14); p->drawLine(12,11,12,17); } break;
        case S::Home:
            path({{2,11},{12,3},{22,11}}); path({{5,9},{5,21},{19,21},{19,9}});
            path({{9,21},{9,14},{15,14},{15,21}}); break;
        case S::Computer:
            p->drawRoundedRect(QRectF(3,3,18,13),1,1); p->drawLine(12,16,12,21); p->drawLine(7,21,17,21); break;
        case S::Drive:
            path({{3,15},{6,5},{18,5},{21,15},{3,15},{3,21},{21,21},{21,15}}); p->drawPoint(17,18); break;
        case S::List:
            for (int y:{6,12,18}) { p->drawPoint(4,y); p->drawLine(8,y,20,y); } break;
        case S::Grid:
            for (int x:{4,14}) {
                for (int y:{4,14}) p->drawRoundedRect(QRectF(x,y,6,6),1,1);
            }
            break;
        case S::Table:
            p->drawRoundedRect(QRectF(3,4,18,16),1,1); p->drawLine(3,9,21,9);
            p->drawLine(3,14,21,14); p->drawLine(11,4,11,20); break;
        case S::Check: path({{4,12},{9,17},{20,6}}); break;
        case S::Warning:
            path({{12,3},{22,21},{2,21},{12,3}}); p->drawLine(12,9,12,14); p->drawPoint(12,18); break;
        case S::Question:
            p->drawEllipse(QRectF(3,3,18,18));
            { QPainterPath q; q.moveTo(9,9); q.cubicTo(9,5,17,6,15,11); q.cubicTo(14,12,12,12,12,14); p->drawPath(q); }
            p->drawPoint(12,17); break;
        case S::Trash:
            p->drawLine(3,6,21,6); path({{8,6},{8,3},{16,3},{16,6}});
            path({{5,6},{6,21},{18,21},{19,6}}); p->drawLine(10,10,10,17); p->drawLine(14,10,14,17); break;
        case S::Refresh:
            p->drawArc(QRectF(4,4,16,16),45*16,290*16); path({{20,3},{20,9},{14,9}}); break;
        case S::Sidebar:
            p->drawRoundedRect(QRectF(3,4,18,16),2,2); p->drawLine(9,4,9,20); break;
        }
        p->restore();
    }
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override {
        return scaledPixmap(size,mode,state,1.);
    }
    QPixmap scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale) override {
        QPixmap image(qRound(size.width()*scale),qRound(size.height()*scale));
        image.setDevicePixelRatio(scale); image.fill(Qt::transparent);
        QPainter painter(&image); paint(&painter,QRect(QPoint(),size),mode,state);
        return image;
    }
};
}
QIcon Icons::line(Symbol symbol, QColor color) { return QIcon(new LineIconEngine(symbol,std::move(color))); }

namespace {
class MonochromeStyle final : public QProxyStyle {
public:
    MonochromeStyle() : QProxyStyle(QStyleFactory::create("Fusion")) {}
    int styleHint(StyleHint hint,const QStyleOption *option=nullptr,const QWidget *widget=nullptr,QStyleHintReturn *result=nullptr) const override {
        if (hint==SH_DialogButtonBox_ButtonsHaveIcons) return 1;
        return QProxyStyle::styleHint(hint,option,widget,result);
    }
    QIcon standardIcon(StandardPixmap icon,const QStyleOption *option=nullptr,const QWidget *widget=nullptr) const override {
        using S=Icons::Symbol;
        switch (icon) {
        case SP_DirIcon: case SP_DirClosedIcon: case SP_DirLinkIcon: case SP_DirLinkOpenIcon: return Icons::line(S::Folder);
        case SP_DirOpenIcon: case SP_DialogOpenButton: return Icons::line(S::Open);
        case SP_FileIcon: case SP_FileLinkIcon: return Icons::line(S::Document);
        case SP_ComputerIcon: case SP_DesktopIcon: return Icons::line(S::Computer);
        case SP_DriveFDIcon: case SP_DriveHDIcon: case SP_DriveCDIcon: case SP_DriveDVDIcon: case SP_DriveNetIcon: return Icons::line(S::Drive);
        case SP_DirHomeIcon: return Icons::line(S::Home);
        case SP_FileDialogNewFolder: return Icons::line(S::NewFolder);
        case SP_FileDialogDetailedView: case SP_FileDialogInfoView: return Icons::line(S::Table);
        case SP_FileDialogListView: return Icons::line(S::List);
        case SP_FileDialogContentsView: return Icons::line(S::Grid);
        case SP_FileDialogToParent: case SP_ArrowUp: return Icons::line(S::MoveUp);
        case SP_ArrowDown: return Icons::line(S::MoveDown);
        case SP_FileDialogStart: case SP_FileDialogBack: case SP_ArrowLeft: case SP_ArrowBack: return Icons::line(S::Previous);
        case SP_FileDialogEnd: case SP_ArrowRight: case SP_ArrowForward: case SP_CommandLink: return Icons::line(S::Next);
        case SP_DialogSaveButton: case SP_DialogSaveAllButton: return Icons::line(S::Save);
        case SP_DialogOkButton: case SP_DialogApplyButton: case SP_DialogYesButton: case SP_DialogYesToAllButton: return Icons::line(S::Check);
        case SP_DialogDiscardButton: case SP_TrashIcon: return Icons::line(S::Trash);
        case SP_DialogResetButton: case SP_DialogRetryButton: case SP_RestoreDefaultsButton: case SP_BrowserReload: return Icons::line(S::Refresh);
        case SP_DialogHelpButton: case SP_TitleBarContextHelpButton: case SP_MessageBoxQuestion: return Icons::line(S::Question);
        case SP_MessageBoxWarning: return Icons::line(S::Warning);
        case SP_MessageBoxInformation: return Icons::line(S::Info);
        case SP_MessageBoxCritical: case SP_DialogCancelButton: case SP_DialogCloseButton:
        case SP_DialogNoButton: case SP_DialogNoToAllButton: case SP_DialogAbortButton:
        case SP_BrowserStop: case SP_LineEditClearButton: case SP_TabCloseButton:
        case SP_TitleBarCloseButton: case SP_DockWidgetCloseButton: return Icons::line(S::Cancel);
        default: return QProxyStyle::standardIcon(icon,option,widget);
        }
    }
    QPixmap standardPixmap(StandardPixmap icon,const QStyleOption *option=nullptr,const QWidget *widget=nullptr) const override {
        const bool message=icon==SP_MessageBoxInformation || icon==SP_MessageBoxWarning || icon==SP_MessageBoxCritical || icon==SP_MessageBoxQuestion;
        const int size=pixelMetric(message ? PM_MessageBoxIconSize : PM_SmallIconSize,option,widget);
        return standardIcon(icon,option,widget).pixmap(size,size);
    }
};
class MonochromeFileIcons final : public QFileIconProvider {
public:
    QIcon icon(IconType type) const override {
        using S=Icons::Symbol;
        switch (type) {
        case Computer: case Desktop: return Icons::line(S::Computer);
        case Drive: case Network: return Icons::line(S::Drive);
        case Folder: return Icons::line(S::Folder);
        case Trashcan: return Icons::line(S::Trash);
        default: return Icons::line(S::Document);
        }
    }
    QIcon icon(const QFileInfo &file) const override {
        if (file.absoluteFilePath()==QDir::homePath()) return Icons::line(Icons::Symbol::Home);
        return icon(file.isDir() ? Folder : File);
    }
};
}
void Icons::installStyle() { QApplication::setStyle(new MonochromeStyle); }
void Icons::configureFileDialog(QFileDialog &dialog) {
    static MonochromeFileIcons provider;
    dialog.setOption(QFileDialog::DontUseNativeDialog);
    dialog.setIconProvider(&provider);
}
