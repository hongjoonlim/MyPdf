#include "document.h"
#include "docx.h"
#include "i18n.h"
#include "toolpaths.h"
#include <poppler-qt6.h>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QBuffer>
#include <QSet>
#include <QImageReader>
#include <QUuid>
#include <QFileInfo>
#include <QPainter>
#include <QPainterPath>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QPdfWriter>
#include <QProcess>
#include <QSaveFile>
#include <QStandardPaths>
#include <stdexcept>
#include <cmath>
namespace {
[[noreturn]] void fail(const QString &s) { throw std::runtime_error(s.toStdString()); }
std::unique_ptr<Poppler::Document> load(const QString &path) {
    auto p = Poppler::Document::load(path);
    if (!p || p->isLocked() || p->numPages() < 1)
        fail(I18n::text("Unable to open this PDF. It may be damaged or password-protected."));
    p->setRenderHint(Poppler::Document::Antialiasing);
    p->setRenderHint(Poppler::Document::TextAntialiasing);
    return p;
}
void writePage(const QString &path, QSizeF size, const QImage &image = {}, const QRectF &rect = {}) {
    QPdfWriter writer(path);
    writer.setResolution(72);
    writer.setPageSize(QPageSize(size, QPageSize::Point));
    writer.setPageMargins(QMarginsF(0,0,0,0), QPageLayout::Point);
    QPainter painter(&writer);
    if (!painter.isActive()) fail(I18n::text("Unable to create a PDF page."));
    if (!image.isNull()) painter.drawImage(rect, image);
    painter.end();
}
}
Document::Document() { if (!temp_.isValid()) fail(I18n::text("Unable to create a temporary directory.")); }
Document::~Document() = default;
QString Document::next() const { return temp_.filePath(QString::number(++serial_) + ".pdf"); }
void Document::run(const QStringList &args) const {
    const QString program = Tools::qpdf();
    if (program.isEmpty()) fail(I18n::text("qpdf is required. Install it or set MYPDF_QPDF to its executable path."));
    QProcess process;
    process.start(program, args);
    if (!process.waitForStarted()) fail(I18n::text("Unable to start qpdf."));
    if (!process.waitForFinished(120000)) {
        process.kill(); process.waitForFinished(); fail(I18n::text("PDF processing timed out."));
    }
    // qpdf exit 3 denotes successful processing with recoverable warnings.
    if (process.exitStatus() != QProcess::NormalExit || (process.exitCode() != 0 && process.exitCode() != 3))
        fail(QString::fromUtf8(process.readAllStandardError()));
}
void Document::adopt(const QString &path) {
    auto doc = load(path);
    pdf_ = std::move(doc); current_ = path; dirty_ = true;
    signatureBefore_.clear(); objectsBeforeSignature_.clear();
}
void Document::open(const QString &path) {
    QString snapshot = next();
    if (!QFile::copy(path, snapshot)) fail(I18n::text("Unable to read the file."));
    adopt(snapshot); base_=snapshot; objects_.clear(); restoreObjects(); path_ = QFileInfo(path).absoluteFilePath(); dirty_ = false;
    suggestedPath_.clear(); importedFrom_.clear(); protectedWordSources_.clear();
}
void Document::importDocx(const QString &path) {
    const QString pdf=convertDocx(path);
    const QFileInfo source(path);
    const QString suggested=source.dir().filePath(source.completeBaseName()+".pdf");
    adopt(pdf); base_=pdf; objects_.clear();
    path_.clear(); suggestedPath_=suggested; importedFrom_=source.absoluteFilePath();
    protectedWordSources_={importedFrom_};
}
QString Document::convertDocx(const QString &path) const {
    auto word=Docx::read(path);
    const QString pdf=next();
    Docx::savePdf(*word,pdf,path);
    return pdf;
}
void Document::close() { base_.clear(); objects_.clear(); objectsBeforeSignature_.clear(); pdf_.reset(); current_.clear(); path_.clear(); dirty_ = false; signatureBefore_.clear(); suggestedPath_.clear(); importedFrom_.clear(); protectedWordSources_.clear(); }
int Document::count() const { return pdf_ ? pdf_->numPages() : 0; }
void Document::check(int page) const { if (page < 0 || page >= count()) fail(I18n::text("Invalid page number.")); }
QSizeF Document::size(int page) const { check(page); return pdf_->page(page)->pageSizeF(); }
QImage Document::render(int page, double dpi) const { check(page); return pdf_->page(page)->renderToImage(dpi, dpi); }
void Document::move(int from, int to) {
    check(from); check(to); if (from==to) return;
    QList<int> pages; for (int i=0;i<count();++i) pages.append(i);
    pages.move(from,to);
    QStringList nums; for (int p:pages) nums << QString::number(p+1);
    auto objects=objects_;
    for (auto &object:objects) object.page=pages.indexOf(object.page);
    const QString out=next(); run({base_,"--pages",".",nums.join(','),"--",out});
    commitObjects(out,objects);
}
void Document::insertBlank(int before) {
    if (!isOpen()) fail(I18n::text("Open a PDF first."));
    if (before<0 || before>count()) fail(I18n::text("Invalid insertion position."));
    QString blank=next(); writePage(blank,size(qMin(before,count()-1))); insertPdf(blank,before);
}
void Document::insertPdf(const QString &path, int before) {
    if (!isOpen() || before<0 || before>count()) fail(I18n::text("Invalid insertion position."));
    Document inserted; inserted.open(path);
    auto objects=objects_;
    for (auto &object:objects) if (object.page>=before) object.page+=inserted.count();
    for (auto object:inserted.objects_) {
        object.id=QUuid::createUuid().toString(QUuid::WithoutBraces); object.page+=before; objects.append(object);
    }
    QStringList args{base_,"--pages"};
    if (before>0) args << "." << QString("1-%1").arg(before);
    args << inserted.base_ << "1-z";
    if (before<count()) args << "." << QString("%1-z").arg(before+1);
    QString out=next(); args << "--" << out; run(args); commitObjects(out,objects);
}
void Document::insertDocument(const QString &path, int before) {
    if (!isOpen() || before<0 || before>count()) fail(I18n::text("Invalid insertion position."));
    const QFileInfo source(path);
    const auto extension=source.suffix().toLower();
    if (extension=="doc") fail(I18n::text("Legacy DOC files are not supported. Open a DOCX file instead."));
    if (extension=="pdf") { insertPdf(path,before); return; }
    if (extension!="docx") fail(I18n::text("Only PDF and DOCX documents can be inserted."));
    const QString pdf=convertDocx(path);
    insertPdf(pdf,before);
    if (!protectedWordSources_.contains(source.absoluteFilePath())) protectedWordSources_.append(source.absoluteFilePath());
}
QList<PageObject> Document::objects(int page) const {
    check(page); QList<PageObject> result;
    for (const auto &object:objects_) if (object.page==page) result.append(object);
    return result;
}
namespace {
void paintObject(QPainter &painter, const PageObject &object, const QSizeF &pageSize) {
    QRectF rect(object.rect.x()*pageSize.width(),object.rect.y()*pageSize.height(),object.rect.width()*pageSize.width(),object.rect.height()*pageSize.height());
    if (object.isHighlight()) {
        QPainterPath path;
        for (int i=0;i<object.highlight.size();++i) {
            const QPointF point=rect.topLeft()+QPointF(object.highlight[i].x()*rect.width(),object.highlight[i].y()*rect.height());
            if (i==0) path.moveTo(point); else path.lineTo(point);
        }
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(object.color,object.strokeWidth,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
        painter.drawPath(path); return;
    }
    if (!object.image.isNull()) { painter.drawImage(rect,object.image); return; }
    QTextDocument content; content.documentLayout()->setPaintDevice(painter.device());
    content.setDefaultFont(object.font); content.setDocumentMargin(0); content.setPlainText(object.text);
    const QSizeF layout=object.textSize.isValid()?object.textSize:rect.size();
    content.setTextWidth(layout.width()); painter.translate(rect.topLeft());
    painter.scale(rect.width()/layout.width(),rect.height()/layout.height());
    QAbstractTextDocumentLayout::PaintContext context; context.palette.setColor(QPalette::Text,object.color);
    content.documentLayout()->draw(&painter,context);
}
}
QImage Document::renderBase(int page, double dpi) const {
    check(page); auto document=load(base_); return document->page(page)->renderToImage(dpi,dpi);
}
QImage Document::renderObject(const PageObject &object, double dpi) const {
    const QSizeF pageSize=size(object.page);
    const QSizeF points(object.rect.width()*pageSize.width(),object.rect.height()*pageSize.height());
    const double scale=dpi/72.;
    QImage image(QSize(qMax(1,int(std::ceil(points.width()*scale))),qMax(1,int(std::ceil(points.height()*scale)))),QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    image.setDotsPerMeterX(qRound(72/.0254)); image.setDotsPerMeterY(qRound(72/.0254));
    QPainter painter(&image); painter.setRenderHint(QPainter::Antialiasing); painter.setRenderHint(QPainter::TextAntialiasing); painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.scale(image.width()/points.width(),image.height()/points.height());
    auto local=object; local.rect.moveTopLeft(QPointF(0,0)); paintObject(painter,local,pageSize);
    return image;
}
QString Document::compose(const QString &base, const QList<PageObject> &objects) const {
    if (objects.isEmpty()) return base;
    auto original=load(base);
    QString result=next(); run({base,"--flatten-rotation",result});
    // Draw all objects for a page in one overlay, preserving their stacking order.
    for (int page=0;page<original->numPages();++page) {
        QList<PageObject> pageObjects;
        for (const auto &object:objects) if (object.page==page) pageObjects.append(object);
        if (pageObjects.isEmpty()) continue;
        const QSizeF pageSize=original->page(page)->pageSizeF();
        const QString overlay=next();
        {
            QPdfWriter writer(overlay); writer.setResolution(72);
            writer.setPageSize(QPageSize(pageSize,QPageSize::Point));
            writer.setPageMargins(QMarginsF(0,0,0,0),QPageLayout::Point);
            QPainter painter(&writer);
            if (!painter.isActive()) fail(I18n::text("Unable to create a PDF page."));
            for (const auto &object:pageObjects) { painter.save(); paintObject(painter,object,pageSize); painter.restore(); }
        }
        const QString out=next();
        run({result,"--overlay",overlay,"--to="+QString::number(page+1),"--from=1","--",out}); result=out;
    }
    return result;
}
void Document::commitObjects(const QString &base, const QList<PageObject> &objects) {
    const QString result=compose(base,objects);
    adopt(result); base_=base; objects_=objects;
}
PageObject Document::makeText(int page, const QString &text, const QFont &font, const QColor &color, const QPointF &position, double fixedWidth) const {
    check(page);
    if (text.trimmed().isEmpty() || text.size()>10000 || !color.isValid() ||
        !std::isfinite(position.x()) || !std::isfinite(position.y()) ||
        !QRectF(0,0,1,1).contains(position) || position.x()>=1 || position.y()>=1 || font.pointSizeF()<.1 || font.pointSizeF()>10000)
        fail(I18n::text("Invalid text or placement."));
    const QSizeF pageSize=size(page);
    // Measure on a 72-DPI PDF paint device, matching the saved appearance.
    QPdfWriter metrics(next()); metrics.setResolution(72);
    QTextDocument content; content.documentLayout()->setPaintDevice(&metrics);
    content.setDefaultFont(font); content.setDocumentMargin(0); content.setPlainText(text);
    const double available=fixedWidth>0?qMin(fixedWidth,pageSize.width()*(1-position.x())):pageSize.width()*(1-position.x());
    content.setTextWidth(available);
    const QSizeF measured=content.size();
    if (measured.height()>pageSize.height()*(1-position.y()))
        fail(I18n::text("The text does not fit here. Choose a smaller font or another position."));
    PageObject object; object.id=QUuid::createUuid().toString(QUuid::WithoutBraces); object.page=page;
    object.text=text; object.font=font; object.color=color;
    const double width=fixedWidth>0?available:qMin(available,qMax(1.,content.idealWidth()));
    object.rect=QRectF(position,QSizeF(width/pageSize.width(),measured.height()/pageSize.height()));
    // Keep the wrap width used for measurement, so later moves do not reflow text.
    content.setTextWidth(width);
    object.rect.setHeight(content.size().height()/pageSize.height());
    if (object.rect.bottom()>1) fail(I18n::text("The text does not fit here. Choose a smaller font or another position."));
    object.textSize=QSizeF(object.rect.width()*pageSize.width(),object.rect.height()*pageSize.height());
    return object;
}
void Document::addText(int page, const QString &text, const QFont &font, const QColor &color, const QPointF &position) {
    const auto object=makeText(page,text,font,color,position);
    auto objects=objects_; objects.append(object); commitObjects(base_,objects);
}
void Document::addHighlight(int page, const QPolygonF &points, const QColor &color, double width) {
    check(page);
    if (points.size()<2 || points.size()>20000 || !color.isValid() || color.alpha()==0 ||
        !std::isfinite(width) || width<1 || width>72) fail(I18n::text("Invalid highlight stroke."));
    const QSizeF pageSize=size(page);
    for (const auto &point:points)
        if (!std::isfinite(point.x()) || !std::isfinite(point.y()) || !QRectF(0,0,1,1).contains(point))
            fail(I18n::text("Invalid highlight stroke."));
    if (points.boundingRect().width()==0 && points.boundingRect().height()==0) fail(I18n::text("Invalid highlight stroke."));
    const QPointF padding(width/(2*pageSize.width()),width/(2*pageSize.height()));
    const QRectF bounds=points.boundingRect().adjusted(-padding.x(),-padding.y(),padding.x(),padding.y()).intersected(QRectF(0,0,1,1));
    if (!bounds.isValid()) fail(I18n::text("Invalid highlight stroke."));
    PageObject object; object.id=QUuid::createUuid().toString(QUuid::WithoutBraces);
    object.page=page; object.rect=bounds; object.color=color; object.strokeWidth=width;
    for (const auto &point:points) object.highlight.append(QPointF((point.x()-bounds.x())/bounds.width(),(point.y()-bounds.y())/bounds.height()));
    auto objects=objects_; objects.append(object); commitObjects(base_,objects);
}
void Document::updateHighlight(const QString &id, const QColor &color, double width) {
    if (!color.isValid() || color.alpha()==0 || !std::isfinite(width) || width<1 || width>72)
        fail(I18n::text("Invalid highlight stroke."));
    auto objects=objects_;
    for (auto &object:objects) if (object.id==id && object.isHighlight()) {
        if (object.color==color && object.strokeWidth==width) return;
        // Recalculate the bounds so thicker strokes are not clipped in the preview.
        QPolygonF points;
        for (const auto &point:object.highlight)
            points.append(object.rect.topLeft()+QPointF(point.x()*object.rect.width(),point.y()*object.rect.height()));
        const QSizeF pageSize=size(object.page);
        const QPointF padding(width/(2*pageSize.width()),width/(2*pageSize.height()));
        object.rect=points.boundingRect().adjusted(-padding.x(),-padding.y(),padding.x(),padding.y()).intersected(QRectF(0,0,1,1));
        object.highlight.clear();
        for (const auto &point:points) object.highlight.append(QPointF((point.x()-object.rect.x())/object.rect.width(),(point.y()-object.rect.y())/object.rect.height()));
        object.color=color; object.strokeWidth=width; commitObjects(base_,objects); return;
    }
    fail(I18n::text("Select an added highlight first."));
}
void Document::sign(int page, const QImage &image, const QRectF &rect) {
    check(page);
    if (image.isNull() || !rect.isValid() || !QRectF(0,0,1,1).contains(rect)) fail(I18n::text("The signature area is outside the page."));
    PageObject object; object.id=QUuid::createUuid().toString(QUuid::WithoutBraces); object.page=page; object.rect=rect; object.image=image;
    auto objects=objects_; objects.append(object);
    const auto before=objects_; const QString beforePath=current_; const bool wasDirty=dirty_;
    commitObjects(base_,objects);
    objectsBeforeSignature_=before; signatureBefore_=beforePath; dirtyBeforeSignature_=wasDirty;
}
void Document::updateText(const QString &id, const QString &text, const QFont &font, const QColor &color) {
    auto objects=objects_;
    for (auto &object:objects) if (object.id==id && object.image.isNull() && !object.isHighlight()) {
        auto edited=makeText(object.page,text,font,color,object.rect.topLeft(),object.rect.width()*size(object.page).width());
        edited.id=object.id; object=edited; commitObjects(base_,objects); return;
    }
    fail(I18n::text("Select an added object first."));
}
void Document::updateSignature(const QString &id, const QImage &image) {
    if (image.isNull()) fail(I18n::text("Unable to read the signature image."));
    auto objects=objects_;
    for (auto &object:objects) if (object.id==id && !object.image.isNull()) {
        object.image=image; commitObjects(base_,objects); return;
    }
    fail(I18n::text("Select an added object first."));
}
void Document::resizeObject(const QString &id, const QRectF &rect) {
    if (!std::isfinite(rect.x()) || !std::isfinite(rect.y()) || !std::isfinite(rect.width()) || !std::isfinite(rect.height()) ||
        !rect.isValid() || !QRectF(0,0,1,1).contains(rect)) fail(I18n::text("Invalid text or placement."));
    auto objects=objects_;
    for (auto &object:objects) if (object.id==id) {
        if (object.rect==rect) return;
        if (object.isHighlight()) object.strokeWidth*=rect.width()/object.rect.width();
        if (object.image.isNull() && !object.isHighlight() && !object.textSize.isValid()) object.textSize=QSizeF(object.rect.width()*size(object.page).width(),object.rect.height()*size(object.page).height());
        object.rect=rect; commitObjects(base_,objects); return;
    }
    fail(I18n::text("Select an added object first."));
}
void Document::moveObject(const QString &id, const QPointF &position) {
    if (!std::isfinite(position.x()) || !std::isfinite(position.y())) fail(I18n::text("Invalid text or placement."));
    auto objects=objects_;
    for (auto &object:objects) if (object.id==id) {
        const QPointF clamped(qBound(0.,position.x(),1-object.rect.width()),qBound(0.,position.y(),1-object.rect.height()));
        if (object.rect.topLeft()==clamped) return;
        object.rect.moveTopLeft(clamped); commitObjects(base_,objects); return;
    }
    fail(I18n::text("Select an added object first."));
}
void Document::removeObject(const QString &id) {
    auto objects=objects_;
    for (int i=0;i<objects.size();++i) if (objects[i].id==id) { objects.removeAt(i); commitObjects(base_,objects); return; }
    fail(I18n::text("Select an added object first."));
}
void Document::cancelSignature() {
    if (!canCancelSignature()) return;
    auto restored=load(signatureBefore_); pdf_=std::move(restored); current_=signatureBefore_;
    objects_=objectsBeforeSignature_; dirty_=dirtyBeforeSignature_; signatureBefore_.clear();
}
void Document::atomicCopy(const QString &source, const QString &destination) {
    QFile in(source); QSaveFile out(destination);
    if (!in.open(QIODevice::ReadOnly) || !out.open(QIODevice::WriteOnly)) fail(I18n::text("Unable to open the destination file."));
    while (!in.atEnd()) {
        auto data = in.read(1024*1024);
        if (data.isEmpty() && in.error() != QFileDevice::NoError) fail(I18n::text("Failed to read the file."));
        if (out.write(data) != data.size()) fail(I18n::text("Failed to write the file."));
    }
    if (!out.commit()) fail(I18n::text("Failed to save the file."));
}
void Document::extract(const QList<int> &pages, const QString &destination) const {
    if (pages.isEmpty()) fail(I18n::text("Select pages to extract."));
    validateDestination(destination);
    QStringList nums; QList<PageObject> objects;
    for (int i=0;i<pages.size();++i) {
        check(pages[i]); nums << QString::number(pages[i]+1);
        for (auto object:objects_) if (object.page==pages[i]) {
            object.page=i; object.id=QUuid::createUuid().toString(QUuid::WithoutBraces); objects.append(object);
        }
    }
    const QString out=next(); run({base_,"--pages",".",nums.join(','),"--",out});
    Document extracted; extracted.open(out); extracted.commitObjects(out,objects); extracted.save(destination);
}
void Document::save(const QString &destination) {
    if (!isOpen()) fail(I18n::text("Open a PDF first."));
    validateDestination(destination);
    atomicCopy(withEditState(),destination); path_=QFileInfo(destination).absoluteFilePath(); dirty_=false;
    signatureBefore_.clear();
}

void Document::validateDestination(const QString &destination) const {
    if (!protectedWordSources_.isEmpty()) {
        if (!destination.endsWith(".pdf",Qt::CaseInsensitive)) fail(I18n::text("Choose a filename ending in .pdf."));
        const QFileInfo target(destination);
        for (const auto &path:protectedWordSources_) {
            const QFileInfo source(path);
            if (target.absoluteFilePath()==source.absoluteFilePath() || (!target.canonicalFilePath().isEmpty() && target.canonicalFilePath()==source.canonicalFilePath()))
                fail(I18n::text("Choose a different filename to preserve the original Word document."));
        }
    }
}

namespace {
const QString editStateName=QStringLiteral("org.MyPdf.edit-state.v1.json");
constexpr qsizetype editStateLimit=256*1024*1024;
QByteArray readStateFile(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size()>editStateLimit) fail(I18n::text("Unable to read editing data."));
    return file.readAll();
}
void writeStateFile(const QString &path,const QByteArray &data) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(data)!=data.size()) fail(I18n::text("Unable to write editing data."));
}
}
QString Document::withEditState() const {
    if (objects_.isEmpty()) return current_;
    QJsonArray items;
    for (const auto &object:objects_) {
        QJsonObject item{{"id",object.id},{"page",object.page},{"x",object.rect.x()},{"y",object.rect.y()},
            {"w",object.rect.width()},{"h",object.rect.height()},{"text",object.text},{"layoutW",object.textSize.width()},{"layoutH",object.textSize.height()},{"font",object.font.toString()},{"color",object.color.name(QColor::HexArgb)}};
        if (object.isHighlight()) {
            QJsonArray points;
            for (const auto &point:object.highlight) points.append(QJsonArray{point.x(),point.y()});
            item.insert("highlight",points); item.insert("strokeWidth",object.strokeWidth);
        }
        if (!object.image.isNull()) {
            QByteArray png; QBuffer buffer(&png); buffer.open(QIODevice::WriteOnly);
            if (!object.image.save(&buffer,"PNG")) fail(I18n::text("Unable to write editing data."));
            item.insert("image",QString::fromLatin1(png.toBase64()));
        }
        items.append(item);
    }
    QJsonObject state{{"version",1},{"base",QString::fromLatin1(readStateFile(base_).toBase64())},{"objects",items}};
    const QByteArray data=QJsonDocument(state).toJson(QJsonDocument::Compact);
    if (data.size()>editStateLimit) fail(I18n::text("Editing data is too large to save."));
    const QString attachment=temp_.filePath(QString::number(++serial_)+"-state.json"), out=next();
    writeStateFile(attachment,data);
    QStringList args{current_};
    for (auto *file:pdf_->embeddedFiles()) if (file->name()==QStringLiteral("org.mypdf.edit-state.v1.json")) {
        args << "--remove-attachment=org.mypdf.edit-state.v1.json"; break;
    }
    args << "--add-attachment" << attachment << "--key="+editStateName << "--filename="+editStateName
         << "--description=MyPdf editable objects" << "--mimetype=application/json" << "--replace" << "--" << out;
    run(args);
    return out;
}
void Document::restoreObjects() {
    // A malformed or foreign editing attachment never prevents ordinary PDF viewing.
    try {
        for (auto *attachment:pdf_->embeddedFiles()) {
            if ((attachment->name()!=editStateName && attachment->name()!=QStringLiteral("org.mypdf.edit-state.v1.json")) || attachment->size()<0 || attachment->size()>editStateLimit) continue;
            const QByteArray data=attachment->data(); if (data.size()>editStateLimit) continue;
            const auto state=QJsonDocument::fromJson(data).object();
            if (state.value("version").toInt()!=1 || !state.value("objects").isArray()) continue;
            const auto items=state.value("objects").toArray(); if (items.size()>10000) continue;
            const QByteArray baseData=QByteArray::fromBase64(state.value("base").toString().toLatin1());
            if (!baseData.startsWith("%PDF-")) continue;
            const QString base=next(); writeStateFile(base,baseData); auto original=load(base);
            if (original->numPages()!=count()) continue;
            bool valid=true; QList<PageObject> objects; QSet<QString> ids;
            for (int page=0;page<count();++page) if (original->page(page)->pageSizeF()!=size(page)) valid=false;
            for (const auto &value:items) {
                const auto item=value.toObject(); PageObject object;
                object.id=item.value("id").toString(); object.page=item.value("page").toInt(-1);
                object.rect=QRectF(item.value("x").toDouble(-1),item.value("y").toDouble(-1),item.value("w").toDouble(-1),item.value("h").toDouble(-1));
                object.textSize=QSizeF(item.value("layoutW").toDouble(-1),item.value("layoutH").toDouble(-1));
                object.text=item.value("text").toString(); object.font.fromString(item.value("font").toString()); object.color=QColor(item.value("color").toString());
                if (item.contains("image")) {
                    const QByteArray png=QByteArray::fromBase64(item.value("image").toString().toLatin1());
                    QBuffer input; input.setData(png); input.open(QIODevice::ReadOnly); QImageReader reader(&input,"PNG");
                    const QSize dimensions=reader.size();
                    if (dimensions.width()<=0 || dimensions.height()<=0 || qint64(dimensions.width())*dimensions.height()>32000000) { valid=false; break; }
                    object.image=reader.read();
                }
                if (item.contains("highlight")) {
                    const auto points=item.value("highlight").toArray();
                    object.strokeWidth=item.value("strokeWidth").toDouble(-1);
                    if (points.size()<2 || points.size()>20000 || !object.color.isValid() || object.color.alpha()==0 ||
                        !std::isfinite(object.strokeWidth) || object.strokeWidth<=0 || object.strokeWidth>10000 || !object.image.isNull()) { valid=false; break; }
                    for (const auto &value:points) {
                        const auto pair=value.toArray();
                        if (pair.size()!=2 || !pair[0].isDouble() || !pair[1].isDouble()) { valid=false; break; }
                        const QPointF point(pair[0].toDouble(),pair[1].toDouble());
                        if (!std::isfinite(point.x()) || !std::isfinite(point.y()) || !QRectF(0,0,1,1).contains(point)) { valid=false; break; }
                        object.highlight.append(point);
                    }
                    if (!valid) break;
                }
                if (object.id.isEmpty() || ids.contains(object.id) || object.page<0 || object.page>=count() ||
                    !object.rect.isValid() || !QRectF(0,0,1,1).contains(object.rect) ||
                    (object.image.isNull() && !object.isHighlight() && (object.text.trimmed().isEmpty() || object.text.size()>10000 || !object.color.isValid() || object.font.pointSizeF()<.1 || object.font.pointSizeF()>10000))) { valid=false; break; }
                if (object.image.isNull() && !object.isHighlight() && !object.textSize.isValid()) object.textSize=QSizeF(object.rect.width()*size(object.page).width(),object.rect.height()*size(object.page).height());
                if (object.image.isNull() && !object.isHighlight() && (object.textSize.width()>100000 || object.textSize.height()>100000)) { valid=false; break; }
                ids.insert(object.id); objects.append(object);
            }
            if (valid) { base_=base; objects_=objects; }
            return;
        }
    } catch (const std::exception &) { objects_.clear(); }
}
