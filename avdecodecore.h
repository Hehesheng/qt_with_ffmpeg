#ifndef AVDECODECORE_H
#define AVDECODECORE_H

#include <QObject>
#include <vector>

#define __STDC_CONSTANT_MACROS
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <stdlib.h>
}

class AVDecodeCore : public QObject {
    Q_OBJECT
public:
    QString fileName;
    explicit AVDecodeCore(QObject *parent = nullptr, QString _filename = "");
    ~AVDecodeCore();

    bool isRunable();
    AVFrame *getFrame();

private:
    FILE *file;
    AVFormatContext *pFormatContext = NULL;
    AVFrame *frame = NULL;
    AVPacket *pkt = NULL;
    //    QList<const AVCodec *> codecList;
    //    QList<AVCodecContext *> codecContextList;
    std::vector<const AVCodec *> codecList;
    std::vector<AVCodecContext *> codecContextList;

signals:
};

#endif // AVDECODECORE_H
