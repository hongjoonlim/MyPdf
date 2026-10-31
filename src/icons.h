#pragma once
#include <QColor>
#include <QIcon>

class QFileDialog;

namespace Icons {
enum class Symbol {
    Select, Open, Save, SaveAs, Close, Exit, Previous, Next, ZoomOut, ZoomIn,
    ActualSize, FitPage, FitWidth, MoveUp, MoveDown, NewPage, InsertPdf,
    Extract, Text, Highlight, Signature, Image, Cancel, Info, Sidebar, Document, Folder, Home,
    Computer, Drive, NewFolder, List, Grid, Table, Check, Warning, Question, Trash, Refresh
};
// Original vector icons: 24-unit grid, 1.75-unit rounded monochrome strokes.
QIcon line(Symbol symbol, QColor color = QColor());
void installStyle();
void configureFileDialog(QFileDialog &dialog);
}
