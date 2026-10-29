#include "document.h"
#include <QApplication>
#include <QPainter>
#include <QPdfWriter>
#include <QFile>
#include <QTemporaryDir>
#include <QProcess>
#include <poppler-qt6.h>
#include <iostream>
#include <stdexcept>
void require(bool ok,const char *message) { if(!ok) throw std::runtime_error(message); }
void fixture(const QString &path) {
    QPdfWriter writer(path); writer.setResolution(72); writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageMargins(QMarginsF(0,0,0,0),QPageLayout::Point);
    QPainter p(&writer); p.fillRect(QRect(0,0,595,842),Qt::red);
    writer.newPage(); p.fillRect(QRect(0,0,595,842),Qt::green);
    writer.newPage(); p.fillRect(QRect(0,0,595,842),Qt::blue);
    p.setPen(Qt::black); p.drawText(100,200,"MyPdf vector text");
}
void runQpdf(const QStringList &args) {
    QProcess p; auto program=qEnvironmentVariable("MYPDF_QPDF","qpdf");
    p.start(program,args); require(p.waitForFinished(),"qpdf completion");
    require(p.exitCode()==0,"qpdf validation");
}
void cropFixture(const QString &path) {
    QList<QByteArray> objects{
        "<< /Type /Catalog /Pages 2 0 R >>",
        "<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 600 800] /CropBox [100 100 500 700] /Resources << >> /Contents 4 0 R >>"};
    QByteArray content="1 0 0 rg 0 0 600 800 re f\n";
    objects.append("<< /Length "+QByteArray::number(content.size())+" >>\nstream\n"+content+"endstream");
    QByteArray pdf="%PDF-1.4\n"; QList<int> offsets;
    for(int i=0;i<objects.size();++i) { offsets.append(pdf.size()); pdf+=QByteArray::number(i+1)+" 0 obj\n"+objects[i]+"\nendobj\n"; }
    int xref=pdf.size(); pdf+="xref\n0 5\n0000000000 65535 f \n";
    for(int offset:offsets) pdf+=QByteArray::number(offset).rightJustified(10,'0')+" 00000 n \n";
    pdf+="trailer\n<< /Size 5 /Root 1 0 R >>\nstartxref\n"+QByteArray::number(xref)+"\n%%EOF\n";
    QFile f(path); require(f.open(QIODevice::WriteOnly),"create crop fixture"); require(f.write(pdf)==pdf.size(),"write crop fixture");
}
int main(int argc,char **argv) {
    QApplication app(argc,argv);
    try {
        QTemporaryDir dir; auto source=dir.filePath(QString::fromUtf16(u"\uc6d0\ubcf8 with spaces.pdf")); fixture(source);
        QFile original(source); require(original.open(QIODevice::ReadOnly),"read fixture"); auto bytes=original.readAll(); original.close();
        Document doc; doc.open(source); require(doc.count()==3 && !doc.dirty(),"open");
        require(doc.render(0).pixelColor(20,20).red()>200,"render red");
        doc.move(0,2); require(doc.dirty(),"dirty"); require(doc.render(2).pixelColor(20,20).red()>200,"move order");
        doc.insertBlank(1); require(doc.count()==4,"blank count"); require(doc.render(1).pixelColor(20,20)==QColor(Qt::white),"blank render");
        doc.insertPdf(source,0); require(doc.count()==7,"insert PDF");
        doc.insertBlank(doc.count()); require(doc.count()==8,"append");
        QImage signature(100,40,QImage::Format_ARGB32); signature.fill(Qt::black);
        doc.sign(0,signature,QRectF(.1,.1,.2,.1));
        auto signedPage=doc.render(0,72); require(signedPage.pixelColor(80,100).value()<30,"signature rendered at placement");
        auto extracted=dir.filePath(QString::fromUtf16(u"\ubd84\ud560.pdf")); doc.extract({2,0},extracted);
        Document split; split.open(extracted); require(split.count()==2,"extract count"); require(split.render(0).pixelColor(20,20).blue()>200,"extract order");
        require(original.open(QIODevice::ReadOnly),"reopen original"); require(original.readAll()==bytes,"edits leave original untouched"); original.close();
        auto textDoc=Poppler::Document::load(extracted);
        require(textDoc->page(0)->text(QRectF()).contains("MyPdf vector text"),"text remains searchable after edits");
        doc.save(source); require(!doc.dirty(),"save dirty");
        Document saved; saved.open(source); require(saved.count()==8,"saved pages");
        require(saved.render(0,72).pixelColor(80,100).value()<30,"signature persists");
        bool rejected=false; try { doc.move(-1,0); } catch (...) { rejected=true; } require(rejected && doc.count()==8,"invalid operation atomic");
        rejected=false; try { doc.open(dir.filePath("missing.pdf")); } catch (...) { rejected=true; } require(rejected && doc.count()==8,"failed open preserves doc");
        runQpdf({"--check",source});
        QString rotated=dir.filePath("rotated.pdf"); runQpdf({source,"--rotate=+90:1",rotated});
        Document rotation; rotation.open(rotated); auto rs=rotation.size(0);
        require(rs.width()>rs.height(),"rotated dimensions");
        rotation.sign(0,signature,QRectF(.5,.5,.2,.1));
        auto ri=rotation.render(0,72); require(ri.pixelColor(ri.width()*.6,ri.height()*.55).value()<30,"rotated signature position");
        QString cropped=dir.filePath("cropped.pdf"); cropFixture(cropped);
        Document crop; crop.open(cropped); require(crop.size(0)==QSizeF(400,600),"crop dimensions");
        crop.sign(0,signature,QRectF(.1,.1,.2,.1));
        auto ci=crop.render(0,72); require(ci.pixelColor(80,90).value()<30,"cropped signature position");
        Document lettering; lettering.open(source);
        const QImage beforeText=lettering.render(1,72);
        QFont font("Sans Serif",18);
        lettering.addText(1,"Inserted searchable text\nSecond line",font,QColor(Qt::blue),QPointF(.2,.3));
        require(lettering.dirty() && lettering.count()==8,"text insertion dirty state and page count");
        require(lettering.render(1,72)!=beforeText,"text insertion changes rendered page");
        const QString textPath=dir.filePath("text.pdf"); lettering.save(textPath);
        auto insertedText=Poppler::Document::load(textPath);
        require(insertedText->page(1)->text(QRectF()).contains("Inserted searchable text"),"inserted text remains searchable after save");
        const QImage stable=lettering.render(1,72);
        bool invalid=false;
        try { lettering.addText(1,"Too low",font,Qt::black,QPointF(.2,.999)); } catch (...) { invalid=true; }
        require(invalid && !lettering.dirty() && stable==lettering.render(1,72),"overflow preserves current document");
        rotation.addText(0,"Rotated text",font,Qt::black,QPointF(.1,.1));
        rotation.save(dir.filePath("rotated-text.pdf"));
        auto rotatedText=Poppler::Document::load(dir.filePath("rotated-text.pdf"));
        require(rotatedText->page(0)->text(QRectF()).contains("Rotated text"),"text on rotated page survives save");
        runQpdf({"--check",textPath});
        doc.close(); require(!doc.isOpen() && doc.count()==0,"close");
        std::cout << "PASS: open/render/move/insert blank/insert PDF/sign/extract/save/reopen/failure/close\n";
        return 0;
    } catch(const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
