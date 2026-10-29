#include "docx.h"
#include "i18n.h"
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPdfWriter>
#include <QSaveFile>
#include <QImageReader>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextImageFormat>
#include <QTextList>
#include <QTextTable>
#include <QXmlStreamReader>
#include <QtEndian>
#include <zlib.h>
#include <stdexcept>
#include <cmath>

namespace {
[[noreturn]] void fail(const char *key) { throw std::runtime_error(I18n::text(key).toStdString()); }
constexpr qsizetype MaxPackage=64*1024*1024, MaxPart=16*1024*1024;
class Package {
    QByteArray bytes_;
    struct Part { quint32 offset,size,packed,crc; quint16 method; };
    QMap<QString,Part> parts_;
    quint16 u16(qsizetype p) const {
        if (p<0 || p+2>bytes_.size()) fail("Invalid or unsupported DOCX package.");
        return qFromLittleEndian<quint16>(bytes_.constData()+p);
    }
    quint32 u32(qsizetype p) const {
        if (p<0 || p+4>bytes_.size()) fail("Invalid or unsupported DOCX package.");
        return qFromLittleEndian<quint32>(bytes_.constData()+p);
    }
public:
    explicit Package(const QString &path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) fail("Unable to read the file.");
        if (file.size()>MaxPackage) fail("This DOCX exceeds the viewer's size limits.");
        bytes_=file.readAll();
        qsizetype end=-1;
        for (qsizetype p=bytes_.size()-22;p>=qMax(qsizetype(0),bytes_.size()-65557);--p)
            if (u32(p)==0x06054b50 && p+22+u16(p+20)==bytes_.size()) { end=p; break; }
        if (end<0 || u16(end+4) || u16(end+6) || u16(end+8)!=u16(end+10)) fail("Invalid or unsupported DOCX package.");
        const quint32 start=u32(end+16), size=u32(end+12);
        if (quint64(start)+size>quint64(end) || u16(end+10)>8192) fail("Invalid or unsupported DOCX package.");
        qsizetype p=start;
        quint64 total=0;
        for (int i=0;i<u16(end+10);++i) {
            if (u32(p)!=0x02014b50 || p+46>end) fail("Invalid or unsupported DOCX package.");
            const quint16 flags=u16(p+8), method=u16(p+10), length=u16(p+28);
            const quint32 packed=u32(p+20), unpacked=u32(p+24), local=u32(p+42);
            const qsizetype next=p+46+length+u16(p+30)+u16(p+32);
            if (next>qsizetype(start)+size || u16(p+34) || (flags&0x41) || (method!=0 && method!=8)) fail("Invalid or unsupported DOCX package.");
            if (unpacked>MaxPart || (total+=unpacked)>128*1024*1024) fail("This DOCX exceeds the viewer's size limits.");
            const QString name=QString::fromUtf8(bytes_.mid(p+46,length));
            if (name.isEmpty() || name.contains(QChar::Null) || name.contains('\\') || name.startsWith('/') || QDir::cleanPath(name)!=name || name.startsWith("../") || name.contains(':') || parts_.contains(name)) {
                if (!name.endsWith('/')) fail("Invalid or unsupported DOCX package.");
            } else {
                if (u32(local)!=0x04034b50 || u16(local+8)!=method || (u16(local+6)&0x41)) fail("Invalid or unsupported DOCX package.");
                const quint64 data=quint64(local)+30+u16(local+26)+u16(local+28);
                if (data+packed>start || bytes_.mid(local+30,u16(local+26))!=bytes_.mid(p+46,length)) fail("Invalid or unsupported DOCX package.");
                parts_.insert(name,{quint32(data),unpacked,packed,u32(p+16),method});
            }
            p=next;
        }
        if (!parts_.contains("[Content_Types].xml") || !parts_.contains("word/document.xml")) fail("Invalid or unsupported DOCX package.");
    }
    QByteArray part(const QString &name) const {
        if (!parts_.contains(name)) return {};
        const auto e=parts_.value(name);
        QByteArray out;
        if (e.method==0) out=bytes_.mid(e.offset,e.packed);
        else {
            out.resize(qMax(quint32(1),e.size));
            z_stream stream{};
            stream.next_in=reinterpret_cast<Bytef *>(const_cast<char *>(bytes_.constData()+e.offset));
            stream.avail_in=e.packed;
            stream.next_out=reinterpret_cast<Bytef *>(out.data()); stream.avail_out=out.size();
            if (inflateInit2(&stream,-MAX_WBITS)!=Z_OK) fail("Invalid or unsupported DOCX package.");
            const int result=inflate(&stream,Z_FINISH);
            const bool valid=result==Z_STREAM_END && stream.total_out==e.size && stream.total_in==e.packed;
            inflateEnd(&stream);
            if (!valid) fail("Invalid or unsupported DOCX package.");
            out.resize(e.size);
        }
        if (quint32(out.size())!=e.size || quint32(crc32(0,reinterpret_cast<const Bytef *>(out.constData()),out.size()))!=e.crc) fail("Invalid or unsupported DOCX package.");
        return out;
    }
};
struct Node {
    QString name,content;
    QMap<QString,QString> attrs;
    QList<Node> children;
    const Node &child(const QString &key) const {
        for (const auto &n:children) if (n.name==key) return n;
        static const Node empty; return empty;
    }
    const Node &descendant(const QString &key) const {
        if (name==key) return *this;
        for (const auto &n:children) { const auto &result=n.descendant(key); if (!result.name.isEmpty()) return result; }
        static const Node empty; return empty;
    }
    QString val() const { return attrs.value("val"); }
};
Node parseNode(QXmlStreamReader &xml,int depth,int &count) {
    if (depth>64 || ++count>200000) fail("This DOCX exceeds the viewer's size limits.");
    Node node; node.name=xml.name().toString();
    for (const auto &attr:xml.attributes()) node.attrs.insert(attr.name().toString(),attr.value().toString());
    while (!xml.atEnd()) {
        auto token=xml.readNext();
        if (token==QXmlStreamReader::DTD || token==QXmlStreamReader::EntityReference) fail("Invalid or unsupported DOCX package.");
        if (token==QXmlStreamReader::StartElement) node.children.append(parseNode(xml,depth+1,count));
        else if (token==QXmlStreamReader::Characters) node.content+=xml.text();
        else if (token==QXmlStreamReader::EndElement) break;
    }
    return node;
}
Node xmlPart(const QByteArray &data) {
    if (data.isEmpty()) return {};
    QXmlStreamReader xml(data); Node root; int count=0;
    while (!xml.atEnd()) {
        auto token=xml.readNext();
        if (token==QXmlStreamReader::DTD || token==QXmlStreamReader::EntityReference) fail("Invalid or unsupported DOCX package.");
        if (token==QXmlStreamReader::StartElement) {
            if (!root.name.isEmpty()) fail("Invalid or unsupported DOCX package.");
            root=parseNode(xml,0,count);
        }
    }
    if (xml.hasError()) fail("Invalid or unsupported DOCX package.");
    return root;
}
class LocalDocument final : public QTextDocument {
    QVariant loadResource(int,const QUrl &) override { return {}; }
};
int cellSpan(const Node &cell) {
    const QString value=cell.child("tcPr").child("gridSpan").val();
    if (value.isEmpty()) return 1;
    bool ok=false; const int span=value.toInt(&ok);
    if (!ok || span<1 || span>64) fail("This DOCX exceeds the viewer's size limits.");
    return span;
}
bool enabled(const Node &node) { return !node.name.isEmpty() && node.val()!="0" && node.val()!="false" && node.val()!="off"; }
void runProperties(const Node &node,QTextCharFormat &format) {
    for (const auto &n:node.children) {
        if (n.name=="b") format.setFontWeight(enabled(n)?QFont::Bold:QFont::Normal);
        else if (n.name=="i") format.setFontItalic(enabled(n));
        else if (n.name=="u") format.setFontUnderline(n.val()!="none" && n.val()!="0");
        else if (n.name=="strike") format.setFontStrikeOut(enabled(n));
        else if (n.name=="sz") { bool ok=false; double size=n.val().toDouble(&ok)/2; if (ok && size>=4 && size<=144) format.setFontPointSize(size); }
        else if (n.name=="color" && QColor("#"+n.val()).isValid()) format.setForeground(QColor("#"+n.val()));
        else if (n.name=="highlight" && QColor(n.val()).isValid()) format.setBackground(QColor(n.val()));
        else if (n.name=="rFonts") { auto family=n.attrs.value("ascii",n.attrs.value("eastAsia")); if (!family.isEmpty()) format.setFontFamilies({family}); }
        else if (n.name=="vertAlign") format.setVerticalAlignment(n.val()=="superscript"?QTextCharFormat::AlignSuperScript:n.val()=="subscript"?QTextCharFormat::AlignSubScript:QTextCharFormat::AlignNormal);
    }
}
void paragraphProperties(const Node &node,QTextBlockFormat &format) {
    const QString align=node.child("jc").val();
    if (!align.isEmpty()) format.setAlignment(align=="center"?Qt::AlignHCenter:align=="right" || align=="end"?Qt::AlignRight:align=="both"?Qt::AlignJustify:Qt::AlignLeft);
    const auto spacing=node.child("spacing");
    if (spacing.attrs.contains("before")) format.setTopMargin(qBound(0.,spacing.attrs.value("before").toDouble()/15,200.));
    if (spacing.attrs.contains("after")) format.setBottomMargin(qBound(0.,spacing.attrs.value("after").toDouble()/15,200.));
    const auto indent=node.child("ind");
    if (indent.attrs.contains("left")) format.setLeftMargin(qBound(0.,indent.attrs.value("left").toDouble()/15,500.));
    if (indent.attrs.contains("firstLine")) format.setTextIndent(qBound(0.,indent.attrs.value("firstLine").toDouble()/15,200.));
}
class Reader {
    Package package_;
    std::unique_ptr<QTextDocument> doc_=std::make_unique<LocalDocument>();
    QMap<QString,Node> styles_,numbering_;
    QMap<QString,QString> relationships_,abstractIds_;
    QTextCharFormat defaultChar_;
    QTextBlockFormat defaultBlock_;
    qint64 pixels_=0;
    int imageCount_=0;
    void style(const QString &id,QTextBlockFormat &block,QTextCharFormat &chars,int depth=0) {
        if (depth>16 || !styles_.contains(id)) return;
        const auto &node=styles_[id];
        style(node.child("basedOn").val(),block,chars,depth+1);
        paragraphProperties(node.child("pPr"),block); runProperties(node.child("rPr"),chars);
    }
    void image(const Node &node,QTextCursor &cursor) {
        const QString id=node.descendant("blip").attrs.value("embed");
        const QString part=relationships_.value(id);
        if (part.isEmpty()) { cursor.insertText(I18n::text("[Image unavailable]")); return; }
        const QUrl url("docx:/"+part);
        QImage image;
        auto cached=doc_->resource(QTextDocument::ImageResource,url);
        if (cached.canConvert<QImage>()) image=qvariant_cast<QImage>(cached);
        if (image.isNull()) {
            auto data=package_.part(part); QBuffer buffer(&data); buffer.open(QIODevice::ReadOnly); QImageReader reader(&buffer);
            const auto format=reader.format().toLower();
            if (!QList<QByteArray>{"png","jpeg","bmp","gif","webp"}.contains(format)) { cursor.insertText(I18n::text("[Image unavailable]")); return; }
            const auto size=reader.size(); const qint64 area=qint64(size.width())*size.height();
            if (!size.isValid() || area>16000000 || ++imageCount_>128 || (pixels_+=area)>32000000) {
                cursor.insertText(I18n::text("[Image unavailable]")); return;
            }
            image=reader.read();
            if (image.isNull()) { cursor.insertText(I18n::text("[Image unavailable]")); return; }
            doc_->addResource(QTextDocument::ImageResource,url,image);
        }
        const auto extent=node.descendant("extent");
        double width=extent.attrs.value("cx").toDouble()/9525, height=extent.attrs.value("cy").toDouble()/9525;
        QSizeF size=std::isfinite(width) && std::isfinite(height) && width>0 && height>0?QSizeF(width,height):QSizeF(image.size());
        if (size.width()>650 || size.height()>900) size.scale(QSizeF(650,900),Qt::KeepAspectRatio);
        QTextImageFormat format; format.setName(url.toString()); format.setWidth(size.width()); format.setHeight(size.height()); cursor.insertImage(format);
    }
    void inlineContent(const Node &node,QTextCursor &cursor,QTextCharFormat format) {
        if (node.name=="del" || node.name=="instrText" || node.name=="rPr" || node.name=="pPr") return;
        if (node.name=="AlternateContent") {
            const auto &choice=node.child("Choice").name.isEmpty()?node.child("Fallback"):node.child("Choice");
            inlineContent(choice,cursor,format); return;
        }
        if (node.name=="r") { QTextBlockFormat unused; style(node.child("rPr").child("rStyle").val(),unused,format); runProperties(node.child("rPr"),format); }
        if (node.name=="t") cursor.insertText(node.content,format);
        else if (node.name=="tab") cursor.insertText("\t",format);
        else if (node.name=="br" || node.name=="cr") cursor.insertText(QString(QChar::LineSeparator),format);
        else if (node.name=="drawing" || node.name=="pict") image(node,cursor);
        else if (node.name=="noBreakHyphen") cursor.insertText(QString(QChar(0x2011)),format);
        else for (const auto &child:node.children) inlineContent(child,cursor,format);
    }
    void paragraph(const Node &node,QTextCursor &cursor,bool &first,QMap<QString,QTextList *> &lists) {
        QTextBlockFormat block=defaultBlock_; QTextCharFormat chars=defaultChar_;
        const auto &props=node.child("pPr");
        const QString id=props.child("pStyle").val();
        style(id,block,chars);
        if (id.startsWith("Heading",Qt::CaseInsensitive) && !chars.hasProperty(QTextFormat::FontPointSize)) {
            bool ok=false; int level=id.mid(7).toInt(&ok); if (ok && level>=1 && level<=6) { chars.setFontPointSize(23-level*2); chars.setFontWeight(QFont::Bold); }
        }
        paragraphProperties(props,block); runProperties(props.child("rPr"),chars);
        if (!first) cursor.insertBlock(block,chars); else { cursor.setBlockFormat(block); cursor.setCharFormat(chars); }
        first=false;
        const auto num=props.child("numPr"); const QString numId=num.child("numId").val();
        if (!numId.isEmpty() && numId!="0") {
            int level=qBound(0,num.child("ilvl").val().toInt(),8); QString key=numId+":"+QString::number(level);
            const auto number=numbering_.value(abstractIds_.value(numId)).children;
            QString type="bullet";
            for (const auto &n:number) if (n.name=="lvl" && n.attrs.value("ilvl").toInt()==level) type=n.child("numFmt").val();
            if (lists.contains(key)) lists[key]->add(cursor.block());
            else { QTextListFormat f; f.setIndent(level+1); f.setStyle(type=="bullet"?QTextListFormat::ListDisc:QTextListFormat::ListDecimal); lists.insert(key,cursor.createList(f)); }
        }
        inlineContent(node,cursor,chars);
    }
    void flow(const Node &container,QTextCursor &cursor,int depth=0) {
        if (depth>8) fail("This DOCX exceeds the viewer's size limits.");
        bool first=cursor.block().text().isEmpty(); QMap<QString,QTextList *> lists;
        for (const auto &node:container.children) {
            if (node.name=="p") paragraph(node,cursor,first,lists);
            else if (node.name=="sdt") { flow(node.child("sdtContent"),cursor,depth+1); first=false; }
            else if (node.name=="tbl") {
                QList<Node> rows; int columns=0;
                for (const auto &row:node.children) if (row.name=="tr") {
                    rows.append(row); int cols=0;
                    for (const auto &cell:row.children) if (cell.name=="tc") cols+=cellSpan(cell);
                    columns=qMax(columns,cols);
                }
                if (rows.isEmpty() || columns==0) continue;
                if (rows.size()>512 || columns>64 || rows.size()*columns>4096) fail("This DOCX exceeds the viewer's size limits.");
                QTextTableFormat format; format.setBorder(1); format.setBorderStyle(QTextFrameFormat::BorderStyle_Solid); format.setCellPadding(6); format.setCellSpacing(0); format.setWidth(QTextLength(QTextLength::PercentageLength,100));
                auto *table=cursor.insertTable(rows.size(),columns,format);
                for (int r=0;r<rows.size();++r) {
                    int c=0;
                    for (const auto &cell:rows[r].children) if (cell.name=="tc") {
                        int span=cellSpan(cell);
                        if (span>1) table->mergeCells(r,c,1,span);
                        auto inside=table->cellAt(r,c).firstCursorPosition(); flow(cell,inside,depth+1); c+=span;
                    }
                }
                cursor.setPosition(table->lastPosition()+1); first=true; lists.clear();
            }
        }
    }
public:
    explicit Reader(const QString &path) : package_(path) {}
    std::unique_ptr<QTextDocument> read() {
        const auto types=xmlPart(package_.part("[Content_Types].xml"));
        bool wordType=false;
        for (const auto &node:types.children)
            if (node.attrs.value("PartName")=="/word/document.xml" && node.attrs.value("ContentType")=="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml") wordType=true;
        if (!wordType) fail("Invalid or unsupported DOCX package.");
        const auto root=xmlPart(package_.part("word/document.xml"));
        if (root.name!="document" || root.child("body").name.isEmpty()) fail("Invalid or unsupported DOCX package.");
        const auto styleRoot=xmlPart(package_.part("word/styles.xml"));
        const auto defaults=styleRoot.child("docDefaults");
        runProperties(defaults.child("rPrDefault").child("rPr"),defaultChar_);
        paragraphProperties(defaults.child("pPrDefault").child("pPr"),defaultBlock_);
        for (const auto &styleNode:styleRoot.children) if (styleNode.name=="style") styles_.insert(styleNode.attrs.value("styleId"),styleNode);
        QString defaultStyle="Normal";
        for (auto it=styles_.begin();it!=styles_.end();++it)
            if (it.value().attrs.value("type")=="paragraph" && it.value().attrs.value("default")=="1") defaultStyle=it.key();
        style(defaultStyle,defaultBlock_,defaultChar_);
        const auto rels=xmlPart(package_.part("word/_rels/document.xml.rels"));
        for (const auto &rel:rels.children) if (rel.name=="Relationship" && rel.attrs.value("TargetMode")!="External" && rel.attrs.value("Type").endsWith("/image")) {
            QString target=rel.attrs.value("Target"); target=QDir::cleanPath(target.startsWith('/')?target.mid(1):"word/"+target);
            if (!target.startsWith("../") && !target.contains(':') && !target.contains('\\')) relationships_.insert(rel.attrs.value("Id"),target);
        }
        const auto numbers=xmlPart(package_.part("word/numbering.xml"));
        for (const auto &n:numbers.children) {
            if (n.name=="abstractNum") numbering_.insert(n.attrs.value("abstractNumId"),n);
            if (n.name=="num") abstractIds_.insert(n.attrs.value("numId"),n.child("abstractNumId").val());
        }
        doc_->setDefaultFont(QFont("Sans Serif",11)); doc_->setDocumentMargin(32); doc_->setUndoRedoEnabled(false);
        QTextCursor cursor(doc_.get()); flow(root.child("body"),cursor);
        doc_->setModified(false); return std::move(doc_);
    }
};
}
std::unique_ptr<QTextDocument> Docx::read(const QString &path) { return Reader(path).read(); }

void Docx::savePdf(const QTextDocument &document,const QString &destination,const QString &sourcePath) {
    if (!destination.endsWith(".pdf",Qt::CaseInsensitive)) fail("Choose a filename ending in .pdf.");
    const QFileInfo target(destination), source(sourcePath);
    if (!sourcePath.isEmpty() && ((!target.canonicalFilePath().isEmpty() && target.canonicalFilePath()==source.canonicalFilePath()) || target.absoluteFilePath()==source.absoluteFilePath()))
        fail("Choose a different filename to preserve the original Word document.");
    QSaveFile output(destination);
    if (!output.open(QIODevice::WriteOnly)) fail("Unable to open the destination file.");
    {
        QPdfWriter writer(&output);
        writer.setTitle(QFileInfo(sourcePath).completeBaseName()); writer.setCreator("MyPdf");
        writer.setResolution(96); writer.setPageSize(QPageSize(QPageSize::A4));
        writer.setPageMargins(QMarginsF(20,20,20,20),QPageLayout::Millimeter);
        std::unique_ptr<QTextDocument> printable(document.clone());
        printable->setDocumentMargin(0);
        printable->setPageSize(QSizeF(writer.width(),writer.height()));
        printable->print(&writer);
    }
    if (output.error()!=QFileDevice::NoError || output.pos()==0 || !output.commit()) fail("Failed to save the file.");
}
