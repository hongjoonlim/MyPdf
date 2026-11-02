#define MYPDF_NO_MAIN
#include "../src/main.cpp"
#include "toolpaths.h"
#include "form_fixture.h"
#include <QPdfWriter>
#include <QProcess>
#include <QBuffer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <cstdlib>
#include <QTemporaryDir>
#include <poppler-qt6.h>
#include <iostream>
namespace {
void check(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
QByteArray bytes(const QString &path) { QFile file(path); check(file.open(QIODevice::ReadOnly),"read test file"); return file.readAll(); }
void photoFixture(const QString &path, bool useJpeg=true) {
    QImage image(2400,1600,QImage::Format_RGB32);
    for (int y=0;y<image.height();++y) for (int x=0;x<image.width();++x) {
        const quint32 noise=(quint32(x)*1664525u+quint32(y)*1013904223u)^(quint32(x*y)*2654435761u);
        image.setPixel(x,y,qRgb((x/12+(noise&63))%256,(y/8+((noise>>8)&63))%256,((x+y)/18+((noise>>16)&63))%256));
    }
    QByteArray jpeg;
    if (useJpeg) { QBuffer buffer(&jpeg); buffer.open(QIODevice::WriteOnly); check(image.save(&buffer,"JPEG",95),"create high-quality JPEG fixture"); }
    else {
        const auto rgb=image.convertToFormat(QImage::Format_RGB888); QByteArray raw;
        for (int y=0;y<rgb.height();++y) raw.append(reinterpret_cast<const char *>(rgb.constScanLine(y)),rgb.width()*3);
        jpeg=qCompress(raw,9).mid(4);
    }
    const QByteArray codec=useJpeg?"/DCTDecode":"/FlateDecode";
    const QByteArray content="q 600 0 0 400 0 0 cm /Photo Do Q\n";
    QList<QByteArray> objects{
        "<< /Type /Catalog /Pages 2 0 R >>", "<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 600 400] /Resources << /XObject << /Photo 4 0 R >> >> /Contents 5 0 R >>",
        "<< /Type /XObject /Subtype /Image /Width 2400 /Height 1600 /ColorSpace /DeviceRGB /BitsPerComponent 8 /Filter "+codec+" /Length "+QByteArray::number(jpeg.size())+" >>\nstream\n"+jpeg+"\nendstream",
        "<< /Length "+QByteArray::number(content.size())+" >>\nstream\n"+content+"endstream"};
    QByteArray data="%PDF-1.4\n"; QList<int> offsets;
    for (int i=0;i<objects.size();++i) { offsets.append(data.size()); data+=QByteArray::number(i+1)+" 0 obj\n"+objects[i]+"\nendobj\n"; }
    const int xref=data.size(); data+="xref\n0 6\n0000000000 65535 f \n";
    for (int offset:offsets) data+=QByteArray::number(offset).rightJustified(10,'0')+" 00000 n \n";
    data+="trailer\n<< /Size 6 /Root 1 0 R >>\nstartxref\n"+QByteArray::number(xref)+"\n%%EOF\n";
    QFile file(path); check(file.open(QIODevice::WriteOnly) && file.write(data)==data.size(),"write photo PDF");
}
void qpdf(const QStringList &args) {
    QProcess process; process.start(Tools::qpdf(),args);
    if (!process.waitForFinished(120000) || process.exitStatus()!=QProcess::NormalExit || (process.exitCode()!=0 && process.exitCode()!=3))
        throw std::runtime_error("qpdf fixture processing: "+process.readAllStandardError().toStdString()+process.errorString().toStdString());
}
}
int main(int argc,char **argv) {
    QApplication app(argc,argv); QTemporaryDir temp;
    app.setOrganizationName("MyPdf"); app.setApplicationName("MyPdf");
    QSettings::setDefaultFormat(QSettings::IniFormat); QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,temp.path());
    QTimer modalGuard;
    QObject::connect(&modalGuard,&QTimer::timeout,[&] {
        if (auto *message=qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
            std::cerr<<"Unexpected error dialog: "<<message->text().toStdString()<<std::endl;
            std::_Exit(EXIT_FAILURE);
        }
    });
    modalGuard.start(50);
    try {
        const auto seed=temp.filePath("seed.pdf"),large=temp.filePath("large.pdf"),output=temp.filePath("optimized.pdf");
        {
            QPdfWriter writer(seed); writer.setResolution(72); writer.setTitle("Optimization fixture");
            QPainter painter(&writer);
            QImage image(160,120,QImage::Format_RGB32);
            for (int y=0;y<image.height();++y) for (int x=0;x<image.width();++x) image.setPixelColor(x,y,QColor(x%256,y%256,(x+y)%256));
            painter.drawImage(QRectF(100,100,320,240),image);
            painter.drawText(80,390,"Searchable vector text");
            // Repeated invisible graphics operations inflate the uncompressed stream without changing appearance.
            for (int i=0;i<10000;++i) { painter.save(); painter.setPen(QColor(i%255,0,0)); painter.drawLine(-100,-100,-90,-90); painter.restore(); }
            writer.newPage(); painter.drawText(100,100,"Second page");
        }
        const auto attachment=temp.filePath("note.json");
        { QFile file(attachment); check(file.open(QIODevice::WriteOnly),"create attachment"); file.write("Keep this attachment"); }
        qpdf({seed,"--stream-data=uncompress","--object-streams=disable","--add-attachment",attachment,"--key=note","--filename=note.txt","--",large});
        const auto original=bytes(large);
        Document doc; doc.open(large); const auto page=doc.render(0,96);
        const auto result=doc.saveOptimizedCopy(output);
        check(result.originalBytes==original.size() && result.optimizedBytes==QFileInfo(output).size(),"reported sizes match files");
        check(result.optimizedBytes<result.originalBytes/2,"uncompressed PDF shrinks substantially");
        check(bytes(large)==original && doc.path()==large && !doc.dirty(),"optimized export preserves source and editor state");
        Document reopened; reopened.open(output);
        check(reopened.count()==2 && reopened.size(0)==doc.size(0) && reopened.render(0,96)==page,"page geometry and rendered pixels are unchanged");
        {
            auto pdf=Poppler::Document::load(output);
            check(pdf->page(0)->text(QRectF()).contains("Searchable vector text"),"text remains searchable");
            check(pdf->info("Title")=="Optimization fixture","document metadata remains");
            bool found=false; for (auto *file:pdf->embeddedFiles()) if (file->name()=="note.txt") found=file->data()=="Keep this attachment";
            check(found,"foreign attachment remains intact");
        }
        bool rejected=false; try { doc.saveOptimizedCopy(large); } catch (...) { rejected=true; }
        check(rejected && bytes(large)==original,"export cannot overwrite the original");
        rejected=false; try { doc.saveOptimizedCopy(temp.filePath("missing/copy.pdf")); } catch (...) { rejected=true; }
        check(rejected && !doc.dirty() && bytes(large)==original,"export failure preserves editor and source");
        const auto second=temp.filePath("optimized-again.pdf"); const auto again=reopened.saveOptimizedCopy(second);
        check(again.optimizedBytes<=again.originalBytes,"already compressed PDF never grows");
        if (again.optimizedBytes==again.originalBytes) check(bytes(second)==bytes(output),"no-gain fallback copies the exact saved document");

        const auto form=temp.filePath("forms.pdf"),editable=temp.filePath("editable-optimized.pdf"); writeFormFixture(form);
        Document editing; editing.open(form); const auto field=editing.formFields(0).first();
        editing.fillForm(0,{{field.id,"Filled form",false,{}}});
        editing.addText(0,"Editable text",QFont("Sans",12),Qt::black,QPointF(.55,.3));
        QImage signature(100,40,QImage::Format_ARGB32); signature.fill(Qt::blue);
        editing.sign(0,signature,QRectF(.55,.5,.2,.05));
        editing.addHighlight(0,{QPointF(.55,.7),QPointF(.8,.7)},QColor(255,235,59,90),12);
        const auto appearance=editing.render(0,96); const auto objects=editing.objects(0);
        editing.saveOptimizedCopy(editable);
        check(editing.dirty() && editing.path()==form,"export does not mark unsaved edits as saved");
        Document restored; restored.open(editable);
        check(restored.render(0,96)==appearance,"editable-object appearance is unchanged");
        check(restored.objects(0).size()==3 && restored.objects(0).first().id==objects.first().id,"editable text/signature/highlight data is retained");
        check(restored.formFields(0).first().text=="Filled form","AcroForm input is retained");
        restored.moveObject(objects.first().id,QPointF(.6,.4)); restored.save(temp.filePath("edited-after-optimization.pdf"));
        check(restored.objects(0).first().rect.topLeft()==QPointF(.6,.4),"objects remain editable after optimization");

        const auto photo=temp.filePath("photo.pdf"),lossy=temp.filePath("photo-compressed.pdf"); photoFixture(photo);
        Document photograph; photograph.open(photo); const auto originalPhoto=bytes(photo); const auto originalPixels=photograph.renderBase(0,72);
        photograph.addText(0,"Keep editable text",QFont("Sans",12),Qt::black,QPointF(.1,.1));
        const PdfCompressionOptions lossySettings{true,45,1000};
        const auto reduced=photograph.saveOptimizedCopy(lossy,lossySettings);
        check(reduced.imagesChanged>0 && reduced.optimizedBytes<reduced.originalBytes/2,"existing JPEG images are resized and recompressed substantially");
        Document photoCopy; photoCopy.open(lossy);
        check(photoCopy.size(0)==photograph.size(0) && photoCopy.objects(0).size()==1,"lossy export preserves page size and editable text");
        check(photoCopy.renderBase(0,72)!=originalPixels,"compressed editable base does not restore the full-quality image");
        const auto compressedBase=photoCopy.renderBase(0,72);
        photoCopy.moveObject(photoCopy.objects(0).first().id,QPointF(.2,.2));
        check(photoCopy.renderBase(0,72)==compressedBase,"editing after compression retains the compressed base");
        check(bytes(photo)==originalPhoto && photograph.renderBase(0,72)==originalPixels,"lossy export preserves the original image and source file");
        const auto imageMetadata=temp.filePath("compressed-images.json");
        qpdf({lossy,"--json-output","--json-stream-data=none",imageMetadata});
        const auto imageObjects=QJsonDocument::fromJson(bytes(imageMetadata)).object().value("qpdf").toArray()[1].toObject();
        bool sized=false;
        for (auto it=imageObjects.begin();it!=imageObjects.end();++it) {
            const auto dictionary=it.value().toObject().value("stream").toObject().value("dict").toObject();
            if (dictionary.value("/Subtype").toString()=="/Image") { check(dictionary.value("/Width").toInt()<=1000 && dictionary.value("/Height").toInt()<=1000,"image dimension limit applies"); sized=true; }
        }
        check(sized,"compressed file contains a normal image XObject");
        const auto flatePhoto=temp.filePath("flate-photo.pdf"),flateOutput=temp.filePath("flate-photo-compressed.pdf");
        photoFixture(flatePhoto,false); Document flateDocument; flateDocument.open(flatePhoto);
        const auto flateResult=flateDocument.saveOptimizedCopy(flateOutput,{true,65,0});
        check(flateResult.imagesChanged>0 && flateResult.optimizedBytes<flateResult.originalBytes/2,"Flate RGB images are converted to JPEG without requiring resampling");
        Document flateCopy; flateCopy.open(flateOutput); check(flateCopy.size(0)==flateDocument.size(0),"Flate conversion preserves page geometry");
        const auto compressedForms=temp.filePath("compressed-forms.pdf"); editing.saveOptimizedCopy(compressedForms,lossySettings);
        Document formCopy; formCopy.open(compressedForms); check(formCopy.objects(0).size()==3 && formCopy.formFields(0).first().text=="Filled form","lossy mode preserves forms and editable object metadata");
        bool invalid=false; try { photograph.saveOptimizedCopy(temp.filePath("invalid.pdf"),{true,0,1000}); } catch (...) { invalid=true; }
        check(invalid && !QFileInfo::exists(temp.filePath("invalid.pdf")),"invalid lossy settings are rejected before writing");
        Window window; auto *action=window.findChild<QAction *>("saveOptimizedCopyAction");
        check(action && !action->isEnabled(),"optimization requires an open document");
        window.openPath(large); window.show(); app.processEvents(); check(action->isEnabled(),"optimization command is available");
        QTimer::singleShot(0,[&] { if (auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget())) dialog->reject(); });
        action->trigger(); check(bytes(large)==original && !window.windowTitle().contains('*'),"canceling optimization changes nothing");
        auto exportFromMenu=[&](const QString &target) {
            QTimer answer;
            QObject::connect(&answer,&QTimer::timeout,[&] {
                if (auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget())) {
                    if (dialog->objectName()=="compressionSettings") {
                        auto *profile=dialog->findChild<QComboBox *>("compressionProfile");
                        check(profile && profile->currentIndex()==1 && dialog->findChild<QSpinBox *>("compressionQuality")->value()==65,"balanced image compression is the default");
                        if (const auto screenshot=qEnvironmentVariable("MYPDF_COMPRESSION_SCREENSHOT"); !screenshot.isEmpty()) dialog->grab().save(screenshot);
                        profile->setCurrentIndex(3); dialog->accept();
                    } else if (auto *fileDialog=qobject_cast<QFileDialog *>(dialog)) {
                        answer.stop();
                        auto *name=fileDialog->findChild<QLineEdit *>("fileNameEdit");
                        check(name!=nullptr,"Qt save dialog exposes a filename input"); name->setText(target);
                        QMetaObject::invokeMethod(fileDialog,"accept",Qt::DirectConnection);
                    }
                }
            });
            answer.start(10); action->trigger();
        };
        const auto uiOutput=temp.filePath("ui-optimized.pdf");
        exportFromMenu(uiOutput);
        check(QFileInfo::exists(uiOutput) && QFileInfo(uiOutput).size()<original.size(),"menu command saves an optimized copy");
        window.openPath(form); app.processEvents();
        auto *input=window.findChild<QLineEdit *>("formField_"+QString::number(field.id));
        check(input!=nullptr,"inline form field is available for optimized export"); input->setText("Draft in optimized copy");
        const auto formOutput=temp.filePath("ui-form-optimized.pdf");
        exportFromMenu(formOutput);
        { Document savedForm; savedForm.open(formOutput); check(savedForm.formFields(0).first().text=="Draft in optimized copy","optimized export commits the focused inline field"); }
        check(window.windowTitle().contains('*'),"copy export preserves the unsaved state of the original document");
        for (const auto &code:QStringList{"ko","de","en"}) {
            window.findChild<QAction *>("language_"+code)->trigger(); check(action->text()==I18n::text("Save Optimized Copy…"),"optimization command is translated");
        }
        std::cout<<"PASS: lossless optimization, size reporting, source protection, editable objects, forms, metadata, attachments and UI\n";
        std::cout<<"JPEG fixture size: "<<reduced.originalBytes<<" -> "<<reduced.optimizedBytes<<" bytes\n";
        std::cout<<"Fixture size: "<<result.originalBytes<<" -> "<<result.optimizedBytes<<" bytes\n";
        return 0;
    } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
