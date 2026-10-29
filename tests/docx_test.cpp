#define MYPDF_NO_MAIN
#include "../src/main.cpp"
#include <QDataStream>
#include <QBuffer>
#include <poppler-qt6.h>
#include <QPdfWriter>
#include <QTemporaryDir>
#include <QTextTable>
#include <zlib.h>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
QByteArray zip(const QList<QPair<QByteArray,QByteArray>> &parts,bool compressed=false) {
    QByteArray data,central;
    QDataStream out(&data,QIODevice::WriteOnly),index(&central,QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::LittleEndian); index.setByteOrder(QDataStream::LittleEndian);
    for (const auto &part:parts) {
        QByteArray packed=part.second;
        if (compressed) {
            packed.resize(compressBound(part.second.size())); z_stream stream{};
            check(deflateInit2(&stream,Z_DEFAULT_COMPRESSION,Z_DEFLATED,-MAX_WBITS,8,Z_DEFAULT_STRATEGY)==Z_OK,"initialize compressor");
            stream.next_in=reinterpret_cast<Bytef *>(const_cast<char *>(part.second.constData())); stream.avail_in=part.second.size();
            stream.next_out=reinterpret_cast<Bytef *>(packed.data()); stream.avail_out=packed.size();
            check(deflate(&stream,Z_FINISH)==Z_STREAM_END,"deflate fixture"); packed.resize(stream.total_out); deflateEnd(&stream);
        }
        quint32 offset=data.size(),crc=crc32(0,reinterpret_cast<const Bytef *>(part.second.constData()),part.second.size());
        quint16 method=compressed?8:0;
        out<<quint32(0x04034b50)<<quint16(20)<<quint16(0)<<method<<quint32(0)<<crc<<quint32(packed.size())<<quint32(part.second.size())<<quint16(part.first.size())<<quint16(0);
        out.writeRawData(part.first.constData(),part.first.size()); out.writeRawData(packed.constData(),packed.size());
        index<<quint32(0x02014b50)<<quint16(20)<<quint16(20)<<quint16(0)<<method<<quint32(0)<<crc<<quint32(packed.size())<<quint32(part.second.size())<<quint16(part.first.size())<<quint16(0)<<quint16(0)<<quint16(0)<<quint16(0)<<quint32(0)<<offset;
        index.writeRawData(part.first.constData(),part.first.size());
    }
    quint32 offset=data.size(); out.writeRawData(central.constData(),central.size());
    out<<quint32(0x06054b50)<<quint16(0)<<quint16(0)<<quint16(parts.size())<<quint16(parts.size())<<quint32(central.size())<<offset<<quint16(0);
    return data;
}
const QByteArray types=R"(<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/></Types>)";
const QByteArray body=R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:wp="http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing"><w:body>
<w:p><w:pPr><w:pStyle w:val="Heading1"/></w:pPr><w:r><w:t>Native DOCX</w:t></w:r></w:p>
<w:p><w:r><w:t xml:space="preserve">Hello </w:t></w:r><w:r><w:rPr><w:b/><w:i/><w:color w:val="2356ab"/></w:rPr><w:t>World</w:t></w:r><w:r><w:tab/><w:t>after tab</w:t><w:br/><w:t>after break</w:t></w:r></w:p>
<w:p><w:pPr><w:numPr><w:ilvl w:val="0"/><w:numId w:val="1"/></w:numPr></w:pPr><w:r><w:t>List item</w:t></w:r></w:p>
<w:tbl><w:tr><w:tc><w:tcPr><w:gridSpan w:val="2"/></w:tcPr><w:p><w:r><w:t>Table heading</w:t></w:r></w:p></w:tc></w:tr><w:tr><w:tc><w:p><w:r><w:t>Left cell</w:t></w:r></w:p></w:tc><w:tc><w:p><w:r><w:t>Right cell</w:t></w:r></w:p></w:tc></w:tr></w:tbl>
<w:p><w:r><w:t>After table</w:t></w:r></w:p>
<w:p><w:r><w:drawing><wp:inline><wp:extent cx="914400" cy="457200"/><a:graphic><a:blip r:embed="rImg"/></a:graphic></wp:inline></w:drawing></w:r></w:p>
<w:p><w:r><w:drawing><a:blip r:embed="rExternal"/></w:drawing></w:r></w:p>
</w:body></w:document>)";
QList<QPair<QByteArray,QByteArray>> fixture() {
    QImage image(40,20,QImage::Format_ARGB32); image.fill(Qt::red); QByteArray png; QBuffer buffer(&png); buffer.open(QIODevice::WriteOnly); image.save(&buffer,"PNG");
    return {{"[Content_Types].xml",types},{"word/document.xml",body},
        {"word/styles.xml",R"(<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:docDefaults><w:rPrDefault><w:rPr><w:sz w:val="22"/></w:rPr></w:rPrDefault></w:docDefaults><w:style w:styleId="Heading1"><w:rPr><w:b/><w:sz w:val="32"/></w:rPr></w:style></w:styles>)"},
        {"word/numbering.xml",R"(<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:abstractNum w:abstractNumId="0"><w:lvl w:ilvl="0"><w:numFmt w:val="bullet"/></w:lvl></w:abstractNum><w:num w:numId="1"><w:abstractNumId w:val="0"/></w:num></w:numbering>)"},
        {"word/_rels/document.xml.rels",R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rImg" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/image" Target="media/sample.png"/><Relationship Id="rExternal" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/image" Target="https://example.invalid/image.png" TargetMode="External"/></Relationships>)"},
        {"word/media/sample.png",png}};
}
void write(const QString &path,const QByteArray &data) { QFile f(path); check(f.open(QIODevice::WriteOnly),"write fixture"); check(f.write(data)==data.size(),"write full fixture"); }
void rejected(const QString &path) { bool failed=false; try { Docx::read(path); } catch (const std::exception &) { failed=true; } check(failed,"invalid DOCX must fail"); }
QAction *action(Window &window,const QString &key) {
    for (auto *a:window.findChildren<QAction *>()) if (a->text()==I18n::text(key)) return a;
    throw std::runtime_error("missing action");
}
}
int main(int argc,char **argv) {
    QApplication app(argc,argv); app.setApplicationName("MyPdf"); app.setOrganizationName("MyPdf");
    QTemporaryDir temp; QSettings::setDefaultFormat(QSettings::IniFormat); QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,temp.path());
    try {
        const QString source=temp.filePath("native with spaces.docx"); const auto bytes=zip(fixture()); write(source,bytes);
        auto document=Docx::read(source);
        auto plain=document->toPlainText();
        check(plain.contains("Native DOCX") && plain.contains("Hello World") && plain.contains("After table"),"paragraphs and table ordering");
        check(document->begin().begin().fragment().charFormat().fontWeight()==QFont::Bold,"heading style");
        auto word=document->find("World").charFormat();
        check(word.fontWeight()==QFont::Bold && word.fontItalic() && word.foreground().color()==QColor("#2356ab"),"run styles");
        bool table=false,list=false,image=false;
        for (auto *frame:document->rootFrame()->childFrames()) if (auto *t=qobject_cast<QTextTable *>(frame)) table=t->rows()==2 && t->columns()==2 && t->cellAt(0,0).columnSpan()==2;
        for (auto block=document->begin();block.isValid();block=block.next()) {
            if (block.text()=="List item") list=block.textList()!=nullptr;
            for (auto it=block.begin();!it.atEnd();++it) if (it.fragment().charFormat().isImageFormat()) image=true;
        }
        check(table && list && image,"table, list, and embedded raster image");
        check(plain.contains("[Image unavailable]"),"external images are not fetched");
        check(!document->resource(QTextDocument::ImageResource,QUrl("file:///etc/passwd")).isValid(),"arbitrary resource loading blocked");
        const auto beforeSize=document->pageSize();
        const QString exported=temp.filePath("native export.pdf");
        Docx::savePdf(*document,exported,source);
        check(document->toPlainText()==plain && document->pageSize()==beforeSize && !document->isModified(),"export preserves viewer document");
        auto pdfDocument=Poppler::Document::load(exported);
        check(pdfDocument && pdfDocument->numPages()>=1 && pdfDocument->page(0)->text(QRectF()).contains("Hello World"),"exported text remains searchable");
        const auto raster=pdfDocument->page(0)->renderToImage(72,72);int red=0;
        for (int y=0;y<raster.height();++y) for(int x=0;x<raster.width();++x) {auto c=raster.pixelColor(x,y);if(c.red()>200 && c.green()<60 && c.blue()<60) ++red;}
        check(red>100,"embedded image survives PDF export");
        std::unique_ptr<QTextDocument> longDocument(document->clone());QTextCursor end(longDocument.get());end.movePosition(QTextCursor::End);
        for(int i=0;i<200;++i) {end.insertBlock();end.insertText("Additional paragraph "+QString::number(i));}
        const QString multipage=temp.filePath("multipage.pdf");Docx::savePdf(*longDocument,multipage,source);
        check(Poppler::Document::load(multipage)->numPages()>1,"long documents paginate");
        bool invalid=false;try {Docx::savePdf(*document,source,source);}catch(const std::exception &){invalid=true;}check(invalid,"Word original cannot be overwritten");
        const QString compressed=temp.filePath("compressed.docx");write(compressed,zip(fixture(),true));check(Docx::read(compressed)->toPlainText()==plain,"deflated ZIP support");
        auto damaged=bytes; damaged.replace(damaged.indexOf("Hello"),5,"Jello"); const QString bad=temp.filePath("bad.docx");write(bad,damaged);rejected(bad);
        auto parts=fixture();parts[1].second="<!DOCTYPE document [<!ENTITY x SYSTEM 'file:///etc/passwd'>]>"+body;write(bad,zip(parts));rejected(bad);
        parts=fixture();parts[1].second.replace("<w:body>","<w:body>"+QByteArray("<w:sdt>").repeated(70));write(bad,zip(parts));rejected(bad);
        parts=fixture();parts[1].second.replace("w:val=\"2\"","w:val=\"2147483647\"");write(bad,zip(parts));rejected(bad);
        write(bad,"not a zip");rejected(bad);
        Document imported;imported.importDocx(source);
        check(imported.isOpen() && imported.dirty() && imported.path().isEmpty(),"import creates an unsaved PDF");
        check(imported.suggestedPath().endsWith("native with spaces.pdf") && imported.importedFrom()==source,"import preserves source and suggests PDF name");
        const auto importedImage=imported.render(0,72);
        bool failedImport=false;try {imported.importDocx(bad);}catch(const std::exception &){failedImport=true;}
        check(failedImport && imported.render(0,72)==importedImage && imported.dirty(),"failed import preserves current PDF");
        bool protectedSource=false;try {imported.save(source);}catch(const std::exception &){protectedSource=true;}
        check(protectedSource && imported.dirty(),"import cannot overwrite Word source");
        bool protectedExtract=false;try {imported.extract({0},source);}catch(const std::exception &){protectedExtract=true;}
        check(protectedExtract,"extraction cannot overwrite Word source");
        const QString base=temp.filePath("base.pdf");
        {QPdfWriter writer(base);writer.setResolution(72);QPainter painter(&writer);painter.drawText(50,100,"BASE FIRST");writer.newPage();painter.drawText(50,100,"BASE LAST");}
        auto pageText=[](const QString &file,int page) {auto pdf=Poppler::Document::load(file);check(bool(pdf),"load merged PDF");return pdf->page(page)->text(QRectF());};
        for(int position:{0,1,2}) {
            Document target;target.open(base);target.insertDocument(source,position);
            const QString merged=temp.filePath("merged-"+QString::number(position)+".pdf");target.save(merged);
            check(target.count()==2+imported.count(),"DOCX insertion adds every page");
            check(pageText(merged,position).contains("Hello World"),"DOCX inserted at selected position");
            check(pageText(merged,position==0?imported.count():0).contains("BASE FIRST"),"first original page retained");
            check(pageText(merged,position<2?target.count()-1:1).contains("BASE LAST"),"last original page retained");
        }
        auto longParts=fixture();QByteArray extra;
        for(int i=0;i<180;++i) extra+="<w:p><w:r><w:t>More DOCX paragraphs for insertion</w:t></w:r></w:p>";
        extra+="<w:p><w:r><w:t>LONG LAST</w:t></w:r></w:p>";
        longParts[1].second.replace("</w:body>",extra+"</w:body>");
        const QString longSource=temp.filePath("long source.DOCX");write(longSource,zip(longParts,true));
        Document probe;probe.importDocx(longSource);check(probe.count()>1,"multi-page DOCX fixture");
        Document combined;combined.open(base);combined.insertDocument(source,1);combined.insertDocument(longSource,combined.count());
        check(combined.count()==2+imported.count()+probe.count(),"multiple and multi-page DOCX insertions");
        const QString combinedPath=temp.filePath("combined.pdf");combined.save(combinedPath);check(pageText(combinedPath,combined.count()-1).contains("LONG LAST"),"all DOCX pages included in order");
#ifndef Q_OS_WIN
        for(const auto &wordSource:QStringList{source,longSource}) {
            const QString alias=temp.filePath(QFileInfo(wordSource).completeBaseName()+"-alias.pdf");check(QFile::link(wordSource,alias),"create source alias");
            bool protectedAlias=false;try {combined.save(alias);}catch(const std::exception &){protectedAlias=true;}check(protectedAlias,"all inserted Word sources protected from save aliases");
        }
#endif
        Document unchanged;unchanged.open(base);QImage insertionInk(50,20,QImage::Format_ARGB32);insertionInk.fill(Qt::black);unchanged.sign(0,insertionInk,QRectF(.7,.7,.2,.1));
        const auto unchangedImage=unchanged.render(0,72);
        for(const auto &candidate:QStringList{bad,temp.filePath("legacy.doc"),temp.filePath("unsupported.txt")}) {
            bool rejectedInsert=false;try {unchanged.insertDocument(candidate,1);}catch(const std::exception &){rejectedInsert=true;}
            check(rejectedInsert && unchanged.count()==2 && unchanged.render(0,72)==unchangedImage && unchanged.canCancelSignature(),"failed insertion preserves PDF and signature cancellation");
        }
        const auto qpdfPath=qgetenv("MYPDF_QPDF");qputenv("MYPDF_QPDF",temp.filePath("missing-qpdf").toUtf8());
        bool mergeFailed=false;try {unchanged.insertDocument(source,1);}catch(const std::exception &){mergeFailed=true;}qputenv("MYPDF_QPDF",qpdfPath);
        check(mergeFailed && unchanged.render(0,72)==unchangedImage && unchanged.canCancelSignature(),"failed PDF merge preserves document after DOCX conversion");
        Document pdfInsertion;pdfInsertion.open(base);pdfInsertion.insertDocument(exported,1);check(pdfInsertion.count()==2+pdfDocument->numPages(),"PDF insertion remains supported");
        Window window;window.show();window.openPath(source);app.processEvents();
        auto *view=dynamic_cast<PageView *>(window.findChild<QWidget *>("pageView"));
        auto *pages=window.findChild<QListWidget *>("pageList");
        check(view && view->isVisible() && pages->count()>=1 && window.windowTitle().contains('*'),"DOCX opens directly as an unsaved PDF");
        check(action(window,"Save")->isEnabled() && action(window,"Draw Signature…")->isEnabled() && action(window,"Blank Page…")->isEnabled(),"PDF editing available immediately after import");
        QTimer::singleShot(0,[&] {for(auto *widget:app.topLevelWidgets()) if(auto *dialog=qobject_cast<QFileDialog *>(widget)) dialog->reject();});
        action(window,"Save")->trigger();check(window.windowTitle().contains('*'),"cancel first save retains unsaved PDF");
        const QString uiExport=temp.filePath("ui export.pdf");bool saveDialogShown=false;
        QTimer::singleShot(0,[&] {for(auto *widget:app.topLevelWidgets()) if(auto *dialog=qobject_cast<QFileDialog *>(widget)) {saveDialogShown=true;dialog->selectFile(uiExport);QMetaObject::invokeMethod(dialog,"accept",Qt::DirectConnection);}});
        action(window,"Save")->trigger();
        check(saveDialogShown && QFile::exists(uiExport) && !window.windowTitle().contains('*'),"first save chooses a destination and saves PDF");
        QImage ink(80,30,QImage::Format_ARGB32);ink.fill(Qt::black);
        view->placed(ink,QRectF(.7,.7,.2,.1));
        check(window.windowTitle().contains('*') && action(window,"Delete Selected Object")->isEnabled(),"imported PDF accepts signatures");
        action(window,"Delete Selected Object")->trigger();check(!action(window,"Delete Selected Object")->isEnabled(),"delete removes imported PDF signature selection");
        QTimer::singleShot(0,[&] {for(auto *widget:app.topLevelWidgets()) if(auto *dialog=qobject_cast<QInputDialog *>(widget)) {dialog->setIntValue(1);dialog->accept();}});
        const int before=pages->count();action(window,"Blank Page…")->trigger();check(pages->count()==before+1,"imported PDF accepts page insertion");
        action(window,"Save")->trigger();check(Poppler::Document::load(uiExport)->numPages()==before+1,"subsequent save retains page edits");
        window.findChild<QAction *>("language_ko")->trigger();check(pages->count()==before+1,"language change preserves imported PDF");
        const QString legacy=temp.filePath("legacy.doc");write(legacy,"legacy");bool legacyMessage=false;
        QTimer::singleShot(0,[&] {for(auto *widget:app.topLevelWidgets()) if(auto *box=qobject_cast<QMessageBox *>(widget)) {legacyMessage=box->text()==I18n::text("Legacy DOC files are not supported. Open a DOCX file instead.");box->accept();}});
        window.openPath(legacy);check(legacyMessage && pages->count()==before+1,"legacy DOC rejected without losing PDF");
        action(window,"Close Document")->trigger();check(!view->isVisible(),"close imported PDF");
        const QString pdf=temp.filePath("sample.pdf");{QPdfWriter writer(pdf);QPainter painter(&writer);painter.drawText(20,20,"PDF");}
        window.openPath(pdf);check(action(window,"Save")->isEnabled() && !window.windowTitle().contains('*'),"ordinary PDF opens normally");
        auto chooseInsert=[&](bool acceptFile) {
            QTimer::singleShot(0,[&,acceptFile] {
                for(auto *widget:app.topLevelWidgets()) if(auto *dialog=qobject_cast<QInputDialog *>(widget)) {
                    dialog->setIntValue(1);
                    QTimer::singleShot(0,[&,acceptFile] {for(auto *next:app.topLevelWidgets()) if(auto *file=qobject_cast<QFileDialog *>(next)) {
                        if(acceptFile) {file->selectFile(source);QMetaObject::invokeMethod(file,"accept",Qt::DirectConnection);} else file->reject();
                    }});
                    dialog->accept();
                }
            });
        };
        const int initialCount=pages->count();chooseInsert(true);action(window,"Insert from PDF or DOCX…")->trigger();
        check(pages->count()==initialCount+imported.count() && pages->currentRow()==0 && window.windowTitle().contains('*'),"menu inserts DOCX and selects first inserted page");
        const auto titleBeforeCancel=window.windowTitle();chooseInsert(false);action(window,"Insert from PDF or DOCX…")->trigger();
        check(pages->count()==initialCount+imported.count() && window.windowTitle()==titleBeforeCancel,"canceling document picker preserves insertion state");
        action(window,"Save")->trigger();check(pageText(pdf,0).contains("Hello World"),"inserted DOCX content persists after saving");
        QFile original(source);check(original.open(QIODevice::ReadOnly) && original.readAll()==bytes,"original Word file unchanged");
        std::cout<<"PASS: native DOCX stored/deflated ZIP, styles, tables, images, invalid input, and imported PDF editing\n";return 0;
    } catch(const std::exception &e) { std::cerr<<e.what()<<'\n';return 1; }
}
