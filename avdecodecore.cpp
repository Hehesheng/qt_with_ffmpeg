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
    {
        char outputBuff[4096] = {0};
        av_dump_format(pFormatContext, 0, outputBuff, 0);
        qDebug() << outputBuff;
    }

    for (unsigned int i = 0; i < pFormatContext->nb_streams; i++) {
        AVCodecParameters *pLocalCodecParameter = pFormatContext->streams[i]->codecpar;
        const AVCodec *pLocalCodec = avcodec_find_decoder(pLocalCodecParameter->codec_id);
        qDebug() << "Stream Index: " << i << "code: " << avcodec_get_name(pLocalCodec->id);

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

AVPacket *AVDecodeCore::_getPacket() {
    if (!isRunable()) {
        return NULL;
    }

    av_packet_unref(pkt);
    if (av_read_frame(pFormatContext, pkt) < 0) {
        qDebug() << "Read Frame Faile.";
        return NULL;
    }

    return pkt;
}

AVFrame *AVDecodeCore::_getFrame() {
    bool gotPic = false;
    AVFrame *frameRGB;
    if (!isRunable()) {
        return NULL;
    }

    int streamIndex = pkt->stream_index;
    qDebug() << "Stream index:" << streamIndex;
    if (pFormatContext->streams[streamIndex]->codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
        qDebug() << "Read Not Video:" << av_get_media_type_string(pFormatContext->streams[streamIndex]->codecpar->codec_type);
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
        gotPic = true;
        SwsContext *scale = sws_getContext(codecContext->width, codecContext->height, codecContext->pix_fmt, codecContext->width, codecContext->height, AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);

        frameRGB = av_frame_alloc();
        unsigned char *out_buf = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_RGB32, codecContext->width, codecContext->height, 1));
        av_image_fill_arrays(frameRGB->data, frameRGB->linesize, out_buf, AV_PIX_FMT_RGB32, codecContext->width, codecContext->height, 1);
        sws_scale(scale, (const unsigned char *const *)frame->data, frame->linesize, 0, codecContext->height, frameRGB->data, frameRGB->linesize);
        sws_freeContext(scale);

        qDebug() << "Frame PIX Format:" << frame->format;
    }

    if (!gotPic)
        return NULL;

    return frameRGB;
}
