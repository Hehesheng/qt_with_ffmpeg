#include "avdecodecore.h"
#include <QDebug>
#include <memory>

AVDecodeCore::AVDecodeCore(QObject *parent, QString _filename) : QObject(parent), fileName(_filename) {
    frame = av_frame_alloc();
    if (frame == NULL) {
        qDebug() << "Frame alloc Failed.";
        return;
    }

    pkt = av_packet_alloc();
    if (pkt == NULL) {
        qDebug() << "Packet alloc Failed.";
        return;
    }

    if (avformat_open_input(&pFormatContext, fileName.toLocal8Bit().data(), NULL, NULL) < 0) {
        qDebug() << "Could not open file.";
        return;
    }

    if (avformat_find_stream_info(pFormatContext, NULL) < 0) {
        qDebug() << "Could find stream(s).";
        return;
    }

    for (unsigned int i = 0; i < pFormatContext->nb_streams; i++) {
        char outputBuff[4096] = {0};
        av_dump_format(pFormatContext, i, outputBuff, 0);
        qDebug() << outputBuff;

        AVCodecParameters *pLocalCodecParameter = pFormatContext->streams[i]->codecpar;
        const AVCodec *pLocalCodec = avcodec_find_decoder(pLocalCodecParameter->codec_id);
        if (pLocalCodec == NULL) {
            qDebug() << "Stream Index:" << i << "Could not find codec.";
            continue;
        }

        AVCodecContext *pLocalCodecContext = avcodec_alloc_context3(pLocalCodec);
        if (pLocalCodecContext == NULL) {
            qDebug() << "Stream Index:" << i << "Could not alloc codec context";
            continue;
        }

        if (avcodec_parameters_to_context(pLocalCodecContext, pLocalCodecParameter) < 0) {
            qDebug() << "Stream Index:" << i << "Failed to copy codec params to codec context.";
            continue;
        }

        if (avcodec_open2(pLocalCodecContext, pLocalCodec, NULL) < 0) {
            qDebug() << "Stream Index:" << i << "Failed to open codec through avcodec_open2.";
            continue;
        }
        codecList.push_back(pLocalCodec);
        codecContextList.push_back(pLocalCodecContext);
        //        codecList.append(pLocalCodec);
        //        codecContextList.append(pLocalCodecContext);
    }
}

AVDecodeCore::~AVDecodeCore() {
    for (auto it : codecContextList) {
        avcodec_free_context(&it);
    }
    if (pFormatContext != NULL) {
        avformat_free_context(pFormatContext);
    }
    if (pkt != NULL) {
        av_packet_free(&pkt);
    }
    if (frame != NULL) {
        av_frame_free(&frame);
    }
}

bool AVDecodeCore::isRunable() {
    return (pFormatContext != NULL);
}

AVFrame *AVDecodeCore::getFrame() {
    static int cntFrame = 0;
    static int cntAudio = 0;

    if (!isRunable()) {
        return NULL;
    }

    if (av_read_frame(pFormatContext, pkt) < 0) {
        qDebug() << "Read Frame Faile.";
        qDebug() << "SUM Frame: " << cntAudio + cntFrame;
        qDebug() << "Frame Cnt: " << cntFrame;
        qDebug() << "Audio Cnt: " << cntAudio;
        return NULL;
    }

    int streamIndex = pkt->stream_index;
    qDebug() << "Stream index:" << streamIndex;
    if (pFormatContext->streams[streamIndex]->codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
        qDebug() << "Read Not Video:" << av_get_media_type_string(pFormatContext->streams[streamIndex]->codecpar->codec_type);
        cntAudio++;
        return NULL;
    }
    AVCodecContext *codecContext = codecContextList[streamIndex];
    int response = avcodec_send_packet(codecContext, pkt);

    if (response < 0) {
        char _tmp[AV_ERROR_MAX_STRING_SIZE] = {0};
        qDebug() << "Error while sending a packet to the decoder:" << av_make_error_string(_tmp, AV_ERROR_MAX_STRING_SIZE, response);
        return NULL;
    }

    while (response >= 0) {
        // Return decoded output data (into a frame) from a decoder
        response = avcodec_receive_frame(codecContext, frame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            qDebug() << "Response Break.";
            break;
        } else if (response < 0) {
            char _tmp[AV_ERROR_MAX_STRING_SIZE] = {0};
            qDebug() << "Error while receiving a frame from the decoder:", av_make_error_string(_tmp, AV_ERROR_MAX_STRING_SIZE, response);
            return NULL;
        }
        qDebug() << "Frame PIX Format:" << frame->format;
    }
    cntFrame++;
    av_packet_unref(pkt);

    return frame;
}
