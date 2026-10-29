#pragma once
#include <QImage>
#include <QFont>
#include <QColor>
#include <QSizeF>
#include <QStringList>
#include <QTemporaryDir>
#include <memory>
namespace Poppler { class Document; }
struct PageObject {
    QString id;
    int page=0;
    QRectF rect;
    QString text;
    QFont font;
    QSizeF textSize;
    QColor color;
    QImage image;
};
class Document {
public:
    Document();
    ~Document();
    void open(const QString &path);
    void close();
    void importDocx(const QString &path);
    bool isOpen() const { return bool(pdf_); }
    int count() const;
    QSizeF size(int page) const;
    QImage render(int page, double dpi = 110) const;
    QImage renderBase(int page, double dpi) const;
    QImage renderObject(const PageObject &object, double dpi) const;
    void move(int from, int to);
    void insertBlank(int before);
    void insertPdf(const QString &path, int before);
    void insertDocument(const QString &path, int before);
    void addText(int page, const QString &text, const QFont &font, const QColor &color, const QPointF &position);
    void sign(int page, const QImage &image, const QRectF &normalizedRect);
    QList<PageObject> objects(int page) const;
    void updateText(const QString &id, const QString &text, const QFont &font, const QColor &color);
    void updateSignature(const QString &id, const QImage &image);
    void resizeObject(const QString &id, const QRectF &rect);
    void moveObject(const QString &id, const QPointF &position);
    void removeObject(const QString &id);
    bool canCancelSignature() const { return !signatureBefore_.isEmpty(); }
    void cancelSignature();
    void extract(const QList<int> &pages, const QString &destination) const;
    void save(const QString &destination);
    bool dirty() const { return dirty_; }
    QString path() const { return path_; }
    QString suggestedPath() const { return suggestedPath_; }
    QString importedFrom() const { return importedFrom_; }
private:
    QTemporaryDir temp_;
    std::unique_ptr<Poppler::Document> pdf_;
    QString base_;
    QList<PageObject> objects_, objectsBeforeSignature_;
    QString current_, path_, suggestedPath_, importedFrom_;
    QStringList protectedWordSources_;
    bool dirty_ = false;
    QString signatureBefore_;
    bool dirtyBeforeSignature_ = false;
    mutable int serial_ = 0;
    QString next() const;
    QString convertDocx(const QString &path) const;
    void adopt(const QString &path);
    void commitObjects(const QString &base, const QList<PageObject> &objects);
    QString compose(const QString &base, const QList<PageObject> &objects) const;
    PageObject makeText(int page, const QString &text, const QFont &font, const QColor &color, const QPointF &position, double width=0) const;
    void restoreObjects();
    QString withEditState() const;
    void run(const QStringList &args) const;
    void check(int page) const;
    void validateDestination(const QString &destination) const;
    static void atomicCopy(const QString &source, const QString &destination);
};
