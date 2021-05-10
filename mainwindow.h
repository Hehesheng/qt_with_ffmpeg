#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "avdecodecore.h"
#include <QAudioOutput>
#include <QMainWindow>
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pathPushButton_pressed();

    void on_browsePushButton_pressed();

    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    std::shared_ptr<AVDecodeCore> videoOutput;
    std::shared_ptr<QAudioOutput> audioOutput;
};
#endif // MAINWINDOW_H
