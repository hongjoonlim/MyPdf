#include "document.h"
#include "toolpaths.h"
#include "i18n.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QBuffer>
#include <QElapsedTimer>
#include <stdexcept>
#include <cstring>

namespace {
[[noreturn]] void fail(const char *message) { throw std::runtime_error(I18n::text(message).toStdString()); }
QByteArray imageStream(const QString &source, const QString &reference, const QString &output, bool raw) {
    QProcess process;
    const auto parts=reference.split(' ',Qt::SkipEmptyParts);
    if (parts.size()!=3) fail("Unable to read PDF image data.");
    // Use a temporary output file rather than binary stdout pipes, also supported by confined qpdf installations.
    process.setStandardOutputFile(output);
    process.start(Tools::qpdf(),{source,"--show-object="+parts[0]+","+parts[1],
        raw?"--raw-stream-data":"--filtered-stream-data","--decode-level=all"});
    if (!process.waitForStarted()) fail("Unable to start qpdf.");
    QElapsedTimer elapsed; elapsed.start();
    while (process.state()!=QProcess::NotRunning) {
        process.waitForFinished(100);
        if (QFileInfo(output).size()>128*1024*1024 || elapsed.elapsed()>120000) {
            process.kill(); process.waitForFinished(); fail("PDF image compression exceeded its processing limits.");
        }
    }
    if (process.exitStatus()!=QProcess::NormalExit || (process.exitCode()!=0 && process.exitCode()!=3)) fail("Unable to read PDF image data.");
    QFile file(output);
    if (!file.open(QIODevice::ReadOnly) || file.size()>128*1024*1024) fail("Unable to read PDF image data.");
    return file.readAll();
}
QJsonObject dictionary(const QJsonObject &objects, const QString &reference) {
    const auto object=objects.value("obj:"+reference).toObject();
    if (object.contains("stream")) return object.value("stream").toObject().value("dict").toObject();
    return object.value("value").toObject();
}
int components(const QJsonValue &space, const QJsonObject &objects, int depth=0) {
    if (depth>8) return 0;
    if (space==QJsonValue("/DeviceRGB")) return 3;
    if (space==QJsonValue("/DeviceGray")) return 1;
    if (space.isString() && space.toString().endsWith(" R"))
        return components(objects.value("obj:"+space.toString()).toObject().value("value"),objects,depth+1);
    const auto array=space.toArray();
    if (array.size()==2 && array[0].toString()=="/ICCBased") {
        const int n=dictionary(objects,array[1].toString()).value("/N").toInt();
        if (n==1 || n==3) return n;
    }
    return 0;
}
}
QString Document::compressImages(const QString &source, const PdfCompressionOptions &options, int &changed) const {
    const QString expanded=next(), metadata=next()+".json", update=next()+".json", output=next();
    run({source,"--externalize-inline-images","--ii-min-bytes=1024",expanded});
    run({expanded,"--json-output","--json-stream-data=none",metadata});
    QFile file(metadata);
    if (!file.open(QIODevice::ReadOnly) || file.size()>128*1024*1024) fail("PDF image compression exceeded its processing limits.");
    QJsonParseError error;
    const auto entries=QJsonDocument::fromJson(file.readAll(),&error).object().value("qpdf").toArray();
    if (error.error!=QJsonParseError::NoError || entries.size()!=2) fail("Unable to read PDF image data.");
    const auto objects=entries[1].toObject(); QJsonObject replacements;
    for (auto item=objects.begin();item!=objects.end();++item) {
        if (!item.key().startsWith("obj:")) continue;
        auto dict=item.value().toObject().value("stream").toObject().value("dict").toObject();
        if (dict.value("/Subtype").toString()!="/Image" || dict.value("/ImageMask").toBool() ||
            dict.contains("/SMask") || dict.contains("/Mask") || dict.contains("/SMaskInData") || dict.contains("/Decode")) continue;
        const int width=dict.value("/Width").toInt(),height=dict.value("/Height").toInt();
        const int channels=components(dict.value("/ColorSpace"),objects);
        if (width<=0 || height<=0 || qint64(width)*height<4096 || qint64(width)*height>64000000 ||
            dict.value("/BitsPerComponent").toInt()!=8 || channels==0) continue;
        QStringList filters;
        const auto filter=dict.value("/Filter");
        if (filter.isArray()) for (const auto &value:filter.toArray()) filters.append(value.toString());
        else if (filter.isString()) filters.append(filter.toString());
        bool supported=true;
        for (const auto &filter:filters) if (filter!="/FlateDecode" && filter!="/LZWDecode" && filter!="/ASCII85Decode" &&
            filter!="/ASCIIHexDecode" && filter!="/RunLengthDecode" && filter!="/DCTDecode") supported=false;
        if (!supported) continue;
        const bool jpegStream=filters.size()==1 && filters.first()=="/DCTDecode";
        if (!jpegStream && qint64(width)*height*channels>128*1024*1024) continue;
        const auto data=imageStream(expanded,item.key().mid(4),next(),jpegStream); QImage image;
        if (jpegStream) image=QImage::fromData(data,"JPEG");
        if (image.isNull()) {
            if (data.size()!=qint64(width)*height*channels) continue;
            image=QImage(width,height,channels==3?QImage::Format_RGB888:QImage::Format_Grayscale8);
            if (image.isNull()) fail("Unable to compress PDF images.");
            for (int y=0;y<height;++y) std::memcpy(image.scanLine(y),data.constData()+qint64(y)*width*channels,width*channels);
        }
        if (image.isNull() || image.width()!=width || image.height()!=height) continue;
        if (channels==1) image=image.convertToFormat(QImage::Format_Grayscale8);
        if (options.maxImageDimension>0 && qMax(width,height)>options.maxImageDimension)
            image=image.scaled(options.maxImageDimension,options.maxImageDimension,Qt::KeepAspectRatio,Qt::SmoothTransformation);
        QByteArray jpeg; QBuffer buffer(&jpeg); buffer.open(QIODevice::WriteOnly);
        if (!image.save(&buffer,"JPEG",options.jpegQuality)) fail("Unable to compress PDF images.");
        // Keep the original if a new JPEG stream would not actually be smaller.
        if (dict.value("/Length").isDouble() && jpeg.size()>=dict.value("/Length").toInteger()) continue;
        dict.remove("/Length"); dict.remove("/DecodeParms");
        dict.insert("/Filter","/DCTDecode"); dict.insert("/BitsPerComponent",8);
        dict.insert("/Width",image.width()); dict.insert("/Height",image.height());
        dict.insert("/Interpolate",true);
        replacements.insert(item.key(),QJsonObject{{"stream",QJsonObject{{"dict",dict},{"data",QString::fromLatin1(jpeg.toBase64())}}}});
        ++changed;
    }
    if (replacements.isEmpty()) return source;
    QFile patch(update);
    const QByteArray bytes=QJsonDocument(QJsonObject{{"qpdf",QJsonArray{entries[0],replacements}}}).toJson(QJsonDocument::Compact);
    if (!patch.open(QIODevice::WriteOnly) || patch.write(bytes)!=bytes.size()) fail("Unable to write editing data.");
    patch.close();
    run({expanded,"--update-from-json="+update,output});
    return output;
}
