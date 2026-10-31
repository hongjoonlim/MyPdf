#pragma once
#include <QFile>
#include <QByteArray>
#include <QList>
inline void writeFormFixture(const QString &path) {
    const QByteArray normal="<< /Type /XObject /Subtype /Form /BBox [0 0 20 20] /Length 0 >>\nstream\n\nendstream";
    const QByteArray marked="<< /Type /XObject /Subtype /Form /BBox [0 0 20 20] /Length 22 >>\nstream\n0 0 m 20 20 l S 0 20 m\nendstream";
    const QList<QByteArray> objects{
        "<< /Type /Catalog /Pages 2 0 R /AcroForm 6 0 R >>",
        "<< /Type /Pages /Kids [3 0 R 4 0 R] /Count 2 >>",
        "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Resources << /Font << /Helv 5 0 R >> >> /Annots [7 0 R 8 0 R 10 0 R 11 0 R 12 0 R 13 0 R 16 0 R 17 0 R] >>",
        "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] >>",
        "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>",
        "<< /Fields [7 0 R 8 0 R 9 0 R 12 0 R 13 0 R 16 0 R 17 0 R] /DR << /Font << /Helv 5 0 R /ZaDb 18 0 R >> >> /DA (/Helv 12 Tf 0 g) /NeedAppearances true >>",
        "<< /Type /Annot /Subtype /Widget /FT /Tx /T (name) /TU (Name) /Rect [72 680 300 710] /P 3 0 R /V (Initial) /MaxLen 40 /F 4 >>",
        "<< /Type /Annot /Subtype /Widget /FT /Btn /T (agree) /Rect [72 630 92 650] /P 3 0 R /V /Off /AS /Off /AP << /N << /Off 14 0 R /Yes 15 0 R >> >> /F 4 >>",
        "<< /FT /Btn /T (delivery) /Ff 32768 /Kids [10 0 R 11 0 R] /V /A >>",
        "<< /Type /Annot /Subtype /Widget /Parent 9 0 R /Rect [72 580 92 600] /P 3 0 R /AS /A /AP << /N << /Off 14 0 R /A 15 0 R >> >> /F 4 >>",
        "<< /Type /Annot /Subtype /Widget /Parent 9 0 R /Rect [120 580 140 600] /P 3 0 R /AS /Off /AP << /N << /Off 14 0 R /B 15 0 R >> >> /F 4 >>",
        "<< /Type /Annot /Subtype /Widget /FT /Ch /T (country) /Rect [72 520 300 550] /P 3 0 R /Ff 131072 /Opt [[(kr) (Korea)] [(de) (Germany)]] /V (kr) /I [0] /F 4 >>",
        "<< /Type /Annot /Subtype /Widget /FT /Tx /T (locked) /Rect [72 460 300 490] /P 3 0 R /Ff 1 /V (Protected) /F 4 >>",
        normal,marked,
        "<< /Type /Annot /Subtype /Widget /FT /Tx /T (notes) /Rect [72 320 300 440] /P 3 0 R /Ff 4096 /V (Notes) /F 4 >>",
        "<< /Type /Annot /Subtype /Widget /FT /Ch /T (custom) /Rect [72 260 300 290] /P 3 0 R /Ff 393216 /Opt [(Other) (Default)] /V (Default) /I [1] /F 4 >>",
        "<< /Type /Font /Subtype /Type1 /BaseFont /ZapfDingbats >>"
    };
    QByteArray bytes="%PDF-1.7\n"; QList<int> offsets{0};
    for (int i=0;i<objects.size();++i) { offsets.append(bytes.size()); bytes+=QByteArray::number(i+1)+" 0 obj\n"+objects[i]+"\nendobj\n"; }
    const int xref=bytes.size(); bytes+="xref\n0 "+QByteArray::number(offsets.size())+"\n0000000000 65535 f \n";
    for (int i=1;i<offsets.size();++i) bytes+=QByteArray::number(offsets[i]).rightJustified(10,'0')+" 00000 n \n";
    bytes+="trailer\n<< /Size "+QByteArray::number(offsets.size())+" /Root 1 0 R >>\nstartxref\n"+QByteArray::number(xref)+"\n%%EOF\n";
    QFile file(path); if (!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size()) throw std::runtime_error("Unable to write form fixture");
}
