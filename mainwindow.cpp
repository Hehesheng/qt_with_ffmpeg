#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>

#define __STDC_CONSTANT_MACROS
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_pathPushButton_pressed() {
    decode = std::make_shared<AVDecodeCore>(this, ui->pathLineEdit->text());
}

void MainWindow::on_browsePushButton_pressed() {
    QFileDialog fileDialog(this);

    fileDialog.setWindowTitle(tr("Select Path"));

    fileDialog.setDirectory("../");
    fileDialog.setViewMode(QFileDialog::Detail);

    auto tmp = fileDialog.getOpenFileName();
    if (tmp != tr(""))
        ui->pathLineEdit->setText(tmp);
}

void MainWindow::on_pushButton_clicked() {
    decode->getFrame();
}
