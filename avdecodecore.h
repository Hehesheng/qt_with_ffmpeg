#ifndef AVDECODECORE_H
#define AVDECODECORE_H

#include <QLabel>
#include <QObject>
#include <vector>

#define __STDC_CONSTANT_MACROS
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <stdlib.h>
}

class AVDecodeCore : public QObject {
    Q_OBJECT
public:
    QString fileName;
    explicit AVDecodeCore(QLabel *parent = nullptr, QString _filename = "");
    ~AVDecodeCore();

    bool isRunable();
    AVPacket *_getPacket();
    AVFrame *_getFrame();
    void setOutputLabel(QLabel *label);

private:
    AVFormatContext *pFormatContext = NULL;
    AVFrame *frame = NULL;
    AVFrame *srcFrame = NULL;
    AVFrame *dstFrame = NULL;
    AVPacket *pkt = NULL;
    QLabel *outputLabel;
    std::vector<const AVCodec *> codecList;
    std::vector<AVCodecContext *> codecContextList;
    std::vector<SwsContext *> swsContextList;

signals:

public slots:
    void slotsLabelSizeChanged();
};

#endif // AVDECODECORE_H
