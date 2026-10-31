#pragma once
#include <QApplication>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QPointer>
#include <QLineEdit>
#include <utility>
#include <QTimer>
#include <cstdio>

// File-system models load asynchronously. Retry actual QFileDialog acceptance
// until selection is ready instead of assuming one zero-delay callback is enough.
class FileDialogAutomation {
    QTimer timer_;
    QElapsedTimer elapsed_;
    QPointer<QFileDialog> dialog_;
    QString path_;
    bool accept_, completed_=false;
public:
    explicit FileDialogAutomation(QString path={}, bool accept=true)
        : path_(std::move(path)), accept_(accept) {
        elapsed_.start(); timer_.setInterval(50);
        QObject::connect(&timer_,&QTimer::timeout,&timer_,[this] {
            if (elapsed_.elapsed()>10000) {
                const auto *active=QApplication::activeModalWidget();
                qFatal("File dialog automation timed out: path=%s, active window=%s, directory=%s, selected=%s, visible=%d",qPrintable(path_),active?qPrintable(active->windowTitle()):"none",dialog_?qPrintable(dialog_->directory().path()):"none",dialog_?qPrintable(dialog_->selectedFiles().join("|")):"none",dialog_?dialog_->isVisible():false);
            }
            if (!dialog_) {
                auto *file=qobject_cast<QFileDialog *>(QApplication::activeModalWidget());
                if (!file || !file->isVisible()) return;
                dialog_=file;
                QObject::connect(file,&QDialog::accepted,&timer_,[this] { completed_=true; timer_.stop(); });
                QObject::connect(file,&QDialog::rejected,&timer_,[this] { completed_=!accept_; timer_.stop(); });
                if (accept_) file->setDirectory(QFileInfo(path_).absolutePath());
            }
            if (!dialog_->isVisible()) return;
            if (!accept_) { dialog_->reject(); return; }
            if (dialog_->acceptMode()==QFileDialog::AcceptOpen) {
                const QFileInfo source(path_);
                auto *model=dialog_->findChild<QFileSystemModel *>();
                if (!source.isFile() || (model && !model->index(source.absoluteFilePath()).isValid())) return;
            }
            dialog_->selectFile(path_);
            // selectFile intentionally ignores the filename edit when it has focus.
            // Exercise the real filename field, as a user would, in that case too.
            if (auto *edit=dialog_->findChild<QLineEdit *>("fileNameEdit")) edit->setText(QFileInfo(path_).fileName());
            QMetaObject::invokeMethod(dialog_.data(),"accept",Qt::QueuedConnection);
        });
        timer_.start();
    }
    bool completed() const { return completed_; }
};
