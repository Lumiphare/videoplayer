//
// Created by Administrator on 2024/3/29.
//

#include "AVCodecHandler.h"
#include "AudioPlayer.h"
#include <qdebug>

#include <mutex>
#include <thread>
#include <utility>
#include <vector>
#include <iostream>
#include <atomic>
#include <QThread>

#include "YUVDataDefined.h"

#if !defined(MIN)
#define MIN(A,B) ((A) < (B) ? (A) : (B))
#endif


std::atomic<bool> m_bThreadRunning(false);
std::atomic<bool> m_bFileThreadRunning(false);
std::atomic<bool> m_bVideoThreadRunning(false);
std::atomic<bool> m_bAudioThreadRunning(false);

AVCodecHandler::AVCodecHandler():m_videoFilePath(""),
    m_mediaWidthHeight(0, 0)
{
}

AVCodecHandler::~AVCodecHandler()
{
}

QString AVCodecHandler::getVideoFilePath() const
{
    return m_videoFilePath;
}

void AVCodecHandler::setVideoFilePath(const QString &path)
{
    m_videoFilePath = path;
}

int AVCodecHandler::initVideoCodec()
{
    if (m_videoFilePath.isEmpty()) {
        qDebug() << "video file path is Empty";
        return -1;
    }
    // 这里用qPrintable()，需要在参数内直接用，不能提前定义，否则调试时会报错
    if (avformat_open_input(&m_pFormatCtx, qPrintable(m_videoFilePath), nullptr, nullptr) != 0) {   // 输入
        qDebug() << "Open_input Stream failed";
        return -1;
    }
    if (avformat_find_stream_info(m_pFormatCtx, NULL) < 0) {   // 获取信息
        qDebug() << "find_stream_info failed";
        return -1;
    }
    // av_dump_format(m_pFormatCtx, 0, filePath, 0);   // 打印获取到的信息
    fflush(stderr);

    for (int i = 0; i < (int)m_pFormatCtx->nb_streams; i++) {   // 查找视频和音频流
        AVCodecParameters* codecParameters = m_pFormatCtx->streams[i]->codecpar;
        if (codecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_videoStreamIdx = i;
            // ffmepg6之后使用avcodec_alloc_context3封装
            m_pVideoCodecCtx = avcodec_alloc_context3(NULL);
            if (!m_pVideoCodecCtx) {
                qDebug() << "Could not allocate video codec context";
                return -1;
            }
            // 参数变为上下文
            if (avcodec_parameters_to_context(m_pVideoCodecCtx, m_pFormatCtx->streams[i]->codecpar) < 0) {
                qDebug() << "avcodec_parameters_to_context error";
                avcodec_free_context(&m_pVideoCodecCtx);
                return -1;
            }
            // 解码器
            const AVCodec* decoder = avcodec_find_decoder(m_pVideoCodecCtx->codec_id);
            if (!decoder) {
                qDebug() << "Decoder not found";
                avcodec_free_context(&m_pVideoCodecCtx);
                return -1;
            }
            // 打开
            if (avcodec_open2(m_pVideoCodecCtx, decoder, NULL) < 0) {
                qDebug() << "Failed to open codec";
                avcodec_free_context(&m_pVideoCodecCtx);
                return -1;
            }
        }
        if (codecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            m_audioStreamIdx = i;
            m_pAudioCodecCtx = avcodec_alloc_context3(NULL);
            if (!m_pAudioCodecCtx) {
                qDebug() << "Could not allocate Audio codec context";
                return -1;
            }
            if (avcodec_parameters_to_context(m_pAudioCodecCtx, m_pFormatCtx->streams[i]->codecpar) < 0) {
                qDebug() << "avcodec_parameters_to_context error";
                avcodec_free_context(&m_pAudioCodecCtx);
                return -1;
            }
            const AVCodec* decoder = avcodec_find_decoder(m_pAudioCodecCtx->codec_id);
            if (!decoder) {
                qDebug() << "Decoder not found";
                avcodec_free_context(&m_pAudioCodecCtx);
                return -1;
            }
            if (avcodec_open2(m_pAudioCodecCtx, decoder, NULL) < 0) {
                qDebug() << "Failed to open codec";
                avcodec_free_context(&m_pAudioCodecCtx);
                return -1;
            }
            m_sampleRate = m_pAudioCodecCtx->sample_rate;
            m_channel = m_pAudioCodecCtx->ch_layout.nb_channels;
            switch (m_pAudioCodecCtx->sample_fmt) {
                case AV_SAMPLE_FMT_U8:
                    m_sampleSize = 8;
                    break;
                case AV_SAMPLE_FMT_S16:
                    m_sampleSize = 16;
                    break;
                case AV_SAMPLE_FMT_S32:
                    m_sampleSize = 3;
                    break;
                default:
                    break;
            }
        }
    }
    AudioPlayer::getInstance()->setSampleRate(m_sampleRate);
    AudioPlayer::getInstance()->setChannelCount(m_channel);

    if (m_pYUVFrame == nullptr) {
        m_pYUVFrame = av_frame_alloc();
    }
    if (m_pYUV420Buffer == nullptr) {
        // av_image_get_buffer_size用于获取存储图像所需的数据缓冲区大小，这个图像具有指定的像素格式和尺寸。
        m_pYUV420Buffer = new uint8_t[av_image_get_buffer_size(AV_PIX_FMT_YUV420P, m_pVideoCodecCtx->width, m_pVideoCodecCtx->height, 1)];
    }
    // av_image_fill_arrays用于设置数据指针和行大小以便容纳图像。
    av_image_fill_arrays(m_pYUVFrame->data, m_pYUVFrame->linesize, m_pYUV420Buffer, AV_PIX_FMT_YUV420P, m_pVideoCodecCtx->width, m_pVideoCodecCtx->height, 1);

    m_mediaWidthHeight = QSize(m_pVideoCodecCtx->width, m_pVideoCodecCtx->height);
    AVStream *audioStream = m_pFormatCtx->streams[m_audioStreamIdx];
    AVStream *videoStream = m_pFormatCtx->streams[m_videoStreamIdx];
    m_aStreamTimeRational = audioStream->time_base;
    m_vStreamTimeRational = videoStream->time_base;
    m_nAudioFrameDuration = audioStream->duration * av_q2d(m_aStreamTimeRational);
    qDebug() << "Video Size:" << m_mediaWidthHeight;
    qDebug() << "V" << videoStream->time_base.den << "A" << audioStream->time_base.den;
    return 0;
}

void AVCodecHandler::startPlayVideo() {
    if (m_videoFilePath.isEmpty()) {
        qDebug() << "video file path is Empty";
        return;
    }
    m_bThreadRunning = true;
    startMediaProcessThread();
}

void AVCodecHandler::stopPlayVideo() {
    // AudioPlayer::getInstance()->stopAudioPlay();
    m_bThreadRunning = false;
    m_bFileThreadRunning = false;
    m_bVideoThreadRunning = false;
    m_bAudioThreadRunning = false;
}

// 寻找特定位置
void AVCodecHandler::seekMedia(float fPos) {
    if (fPos < 0) return;
    if (m_pFormatCtx == nullptr) return;
    stopPlayVideo();
    // m_nSeekingPos = (qint64) fPos / av_q2d(m_vStreamTimeRational);
    m_nSeekingPos = fPos / 100 * m_nAudioFrameDuration /  av_q2d(m_vStreamTimeRational);

    if (m_audioStreamIdx >= 0 && m_videoStreamIdx >= 0) {
        av_seek_frame(m_pFormatCtx, m_videoStreamIdx, m_nSeekingPos, AVSEEK_FLAG_BACKWARD);  // 找到一个最近的I帧去播放
    }
    // 清空队列
    while (!m_videoQueue.empty()) {
        AVPacket* p = m_videoQueue.takeFirst();
        av_packet_free(&p);
    }
    while (!m_audioQueue.empty()) {
        AVPacket* p = m_audioQueue.takeFirst();
        av_packet_free(&p);
    }
    // 重新启动线程
    startPlayVideo();
}

void AVCodecHandler::setUpdateVideoCallback(UpdateVideo2GUI_Callback callback, uintptr_t userData) {
        m_updateVideoCallback = callback;
        m_userDataVideo = userData;
}

void AVCodecHandler::setUpdateProgressCallback(std::function<void(int)> callback) {
        m_updateProgressCallback = std::move(callback);
}

QSize AVCodecHandler::getMediaWidthHeight() const
{
    return m_mediaWidthHeight;
}

void AVCodecHandler::setVolume(double volume) {
    m_volumeRation = volume;
}

void AVCodecHandler::sleep(unsigned long msecs) {
    QThread::msleep(msecs);
}

void AVCodecHandler::convertAndRenderVideo(AVFrame *videoFrame, long long ppts) {
    if (videoFrame == nullptr) return;
    if (m_pVideoSwsCtx == nullptr) {
        // sws_getContext 它用于获取一个用于图像缩放和格式转换的上下文。
        // videoFrame是原本，m_pYUVFrame是转换YUV420P后的
        m_pVideoSwsCtx = sws_getContext(m_pVideoCodecCtx->width, m_pVideoCodecCtx->height,
            m_pVideoCodecCtx->pix_fmt, m_pVideoCodecCtx->width, m_pVideoCodecCtx->height,
            AV_PIX_FMT_YUV420P, SWS_BICUBIC, nullptr,nullptr,nullptr);
    }
    // sws_scale是FFmpeg用于缩放或转换图像的尺寸和像素格式。  linesize因为字节对齐的原因，需要32的整数倍，会变成1664，可能产生绿边问题
    sws_scale(m_pVideoSwsCtx, videoFrame->data, videoFrame->linesize, 0,
        m_pVideoCodecCtx->height, m_pYUVFrame->data, m_pYUVFrame->linesize);
    unsigned int lumaLength = m_pVideoCodecCtx->height *(MIN(m_pVideoFrame->linesize[0], m_pVideoFrame->width));
    unsigned int chromBLength = m_pVideoCodecCtx->height * (MIN(m_pVideoFrame->linesize[1], m_pVideoFrame->width)) / 2;
    unsigned int chromRLength = m_pVideoCodecCtx->height * (MIN(m_pVideoFrame->linesize[2], m_pVideoFrame->width)) / 2;

    // 用于OPENGL处理
    H264YUV_Frame* updateYUVFrame = new H264YUV_Frame();

    updateYUVFrame->width = m_pVideoCodecCtx->width;
    updateYUVFrame->height = m_pVideoCodecCtx->height;
    updateYUVFrame->pts = ppts;

    updateYUVFrame->luma.length = lumaLength;
    updateYUVFrame->chromaB.length = chromBLength;
    updateYUVFrame->chromaR.length = chromRLength;

    updateYUVFrame->luma.dataBuffer = new unsigned char[lumaLength];
    updateYUVFrame->chromaB.dataBuffer = new unsigned char[chromBLength];
    updateYUVFrame->chromaR.dataBuffer = new unsigned char[chromRLength];

    copyDecodedFrame420(m_pYUVFrame->data[0], updateYUVFrame->luma.dataBuffer,
        m_pYUVFrame->linesize[0], m_pVideoCodecCtx->width, m_pVideoCodecCtx->height);
    copyDecodedFrame420(m_pYUVFrame->data[1], updateYUVFrame->chromaB.dataBuffer,
        m_pYUVFrame->linesize[1], m_pVideoCodecCtx->width / 2, m_pVideoCodecCtx->height / 2);
    copyDecodedFrame420(m_pYUVFrame->data[2], updateYUVFrame->chromaR.dataBuffer,
    m_pYUVFrame->linesize[2], m_pVideoCodecCtx->width / 2, m_pVideoCodecCtx->height / 2);

    double fps = av_q2d(m_vStreamTimeRational);
    tickVideoFrameTimeDelay(ppts);
    m_updateVideoCallback(updateYUVFrame, m_userDataVideo);

    if (updateYUVFrame->luma.dataBuffer) {
        delete updateYUVFrame->luma.dataBuffer;
        updateYUVFrame->luma.dataBuffer = nullptr;
    }
    if (updateYUVFrame->chromaB.dataBuffer) {
        delete updateYUVFrame->chromaB.dataBuffer;
        updateYUVFrame->chromaB.dataBuffer = nullptr;
    }
    if (updateYUVFrame->chromaR.dataBuffer) {
        delete updateYUVFrame->chromaR.dataBuffer;
        updateYUVFrame->chromaR.dataBuffer = nullptr;
    }
    delete updateYUVFrame;
    updateYUVFrame = nullptr;
}

// 重采样

void AVCodecHandler::copyDecodedFrame420(uint8_t *src, uint8_t *dst, int linesize, int width, int height) {
    width = MIN(linesize, width);
    // 一行行拷贝， 因为可能存在字节YUV字节对齐的问题，所以width需要MIN，dst和src一次处理的数据不一样
    for (int i = 0; i < height; ++i) {
        memcpy(dst, src, width);
        dst += width;
        src += linesize;
    }
}

void AVCodecHandler::convertAndPlayAudio(AVFrame *audioFrame) {
    if (!m_pFormatCtx || !m_pAudioFrame || !audioFrame) return;
    if (!m_pAudioSwrCtx) {
        /*
        swr_alloc_set_opts(m_pAudioSwrCtx, av_get_default_channel_layout(m_channel),
                           AV_SAMPLE_FMT_S16, m_sampleRate,
                           av_get_default_channel_layout(m_pAudioCodecCtx->ch_layout.nb_channels),
                           m_pAudioCodecCtx->sample_fmt,
                           m_pAudioCodecCtx->sample_rate,0, nullptr
        );
        */
        m_pAudioSwrCtx = swr_alloc();
        // 设置输出参数
        av_opt_set_int(m_pAudioSwrCtx, "out_channel_layout", av_get_default_channel_layout(m_channel), 0);
        av_opt_set_int(m_pAudioSwrCtx, "out_sample_rate", m_sampleRate, 0);
        av_opt_set_sample_fmt(m_pAudioSwrCtx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
        // 设置输入参数
        av_opt_set_int(m_pAudioSwrCtx, "in_channel_layout", av_get_default_channel_layout(m_pAudioCodecCtx->ch_layout.nb_channels), 0);
        av_opt_set_int(m_pAudioSwrCtx, "in_sample_rate", m_pAudioCodecCtx->sample_rate, 0);
        av_opt_set_sample_fmt(m_pAudioSwrCtx, "in_sample_fmt", m_pAudioCodecCtx->sample_fmt, 0);
    }
    int ret = swr_init(m_pAudioSwrCtx);
    if (ret < 0) {
        qDebug() << "swr_init error";
        return;
    }
    int buffsize = av_samples_get_buffer_size(nullptr, m_channel, audioFrame->nb_samples, AV_SAMPLE_FMT_S16, 0);
    if (!m_pSwrBuffer || m_swrBuffSize <  buffsize) {
        m_swrBuffSize = buffsize;
        m_pSwrBuffer = (uint8_t*)realloc(m_pSwrBuffer, m_swrBuffSize);
    }
    uint8_t *outBuff[2] = {m_pSwrBuffer, 0};
    int len = swr_convert(m_pAudioSwrCtx, outBuff, audioFrame->nb_samples, (const uint8_t**)audioFrame->data, audioFrame->nb_samples);
    if (len < 0) {
        return;
    }
    int freeSpace = 0;
    // 等待缓冲区有足够的空间
    while (m_bThreadRunning) {
        freeSpace = AudioPlayer::getInstance()->getFreeSpace();
        if (freeSpace > buffsize) {
            break;
        } else {
            sleep(1);
        }
    }
    if (!m_bThreadRunning) {
        return;
    }
    AudioPlayer::getInstance()->setVolume(m_volumeRation);
    AudioPlayer::getInstance()->writeAudioData((const char *)m_pSwrBuffer, buffsize);
}


void AVCodecHandler::tickAudioFrameTimeDelay(int64_t pts) {
    if (m_aStreamTimeRational.den <= 0) {
        return;
    }
    // float currVideoTimeStamp = getVideoTimeStampFromPts(pts);
    // m_nCurrAudioTimeStamp = getAudioTimeStampFromPts(pts);
    // double diffTime = (currVideoTimeStamp - m_nCurrAudioTimeStamp) * 1000;  // 乘以1000是为了转换为毫秒
    // int sleepTime = (int)diffTime;

    m_nCurrAudioTimeStamp = getAudioTimeStampFromPts(pts);
}

// 音视频同步和视频播放进度条更新

void AVCodecHandler::tickVideoFrameTimeDelay(int64_t pts) {
    if (m_aStreamTimeRational.den <= 0) {
        return;
    }
    static float last = 0;
    // m_nCurrAudioTimeStamp = getAudioTimeStampFromPts(pts);
    // int diffTime = (int)(m_nCurrAudioTimeStamp - m_nLastAudioTimeStamp);
    // if (diffTime >= 1) {
    //     // 更新播放器进度条
    //     m_nLastAudioTimeStamp = m_nCurrAudioTimeStamp;
    // }
    static float lastPts = 0;
    float currVideoTimeStamp = getVideoTimeStampFromPts(pts);
    double diffTime = (currVideoTimeStamp - m_nCurrAudioTimeStamp) * 1000;
    if (diffTime > 0) {
        sleep(diffTime);
        qDebug() << "diffTime:" << diffTime;
    } else {qDebug() << "123";}
    lastPts = currVideoTimeStamp;
    m_updateProgressCallback(static_cast<int>(currVideoTimeStamp / m_nAudioFrameDuration * 100));
}

void AVCodecHandler::startMediaProcessThread() {
    if (!m_bThreadRunning) return;
    // this原来作为参数传入readMediaFrameThread中
    std::thread readThread(&AVCodecHandler::readMediaFrameThread, this);
    readThread.detach();
    // std::thread audioThread(&AVCodecHandler::audioDecodeThread, this);
    // audioThread.detach();
    QThread *audioThread = QThread::create(audioDecodeThread, this);
    audioThread->start();
    std::thread videoThread(&AVCodecHandler::videoDecodeThread, this);
    videoThread.detach();
}
//endregion
void AVCodecHandler::readMediaFrameThread() {
    while (m_bThreadRunning) {
        m_bFileThreadRunning = true;
        if (m_eMediaPlayStatus == PAUSE) {
            sleep(10);
            continue;
        }
        if (m_videoQueue.size() > 600 && m_audioQueue.size() > 1200) {
            sleep(10);
            continue;
        }
        if (!m_bReadFileEOF) {
            AVPacket* packet = av_packet_alloc();
            if (!packet) continue;
            m_eMediaPlayStatus = PLAYING;
            int retValue = av_read_frame(m_pFormatCtx, packet);
            if (retValue == 0) {
                if (packet->stream_index == m_videoStreamIdx) {
                    m_videoQueue.enqueue(packet);
                }
                if (packet->stream_index == m_audioStreamIdx) {
                    m_audioQueue.enqueue(packet);
                }
            } else if (retValue < 0) {
                if (retValue == AVERROR_EOF) {
                    m_bReadFileEOF = true;
                }
                av_packet_free(&packet);
            }
        } else {
            sleep(10);
            continue;
        }
    }
    qDebug() << "read media frame thread exit";
    m_bFileThreadRunning = false;
}

void AVCodecHandler::audioDecodeThread() {
    if (m_pFormatCtx == nullptr) return;
    if (m_pAudioFrame == nullptr) {
        m_pAudioFrame = av_frame_alloc();
    }
    AudioPlayer::getInstance()->startAudioPlay();

    while (m_bThreadRunning) {
        m_bAudioThreadRunning = true;
        if (m_eMediaPlayStatus == PAUSE) {
            sleep(10);
            continue;
        }
        if (m_audioQueue.isEmpty()) {
            sleep(10);
            continue;
        }
        AVPacket* pkt = (AVPacket*)m_audioQueue.dequeue();
        tickAudioFrameTimeDelay(pkt->pts);
        if (pkt == nullptr) break;
        int retValue = avcodec_send_packet(m_pAudioCodecCtx, pkt);
        if (retValue != 0) {
            sleep(10);
            continue;
        }
        int decodeRet = avcodec_receive_frame(m_pAudioCodecCtx, m_pAudioFrame);
        if (decodeRet == 0) {
            convertAndPlayAudio(m_pAudioFrame);
        }
        // sleep(10));
    }
    m_bAudioThreadRunning = false;
    qDebug() << "audio decode thread exit....";
}

void AVCodecHandler::videoDecodeThread() {
    if (m_pFormatCtx == nullptr) return;
    if (m_pVideoFrame == nullptr) {
        m_pVideoFrame = av_frame_alloc();    // 初始化分配内存
    }
    while (m_bThreadRunning) {
        m_bVideoThreadRunning = true;
        if (m_eMediaPlayStatus == PAUSE) {
            sleep(10);
            continue;
        }
        if (m_videoQueue.isEmpty()) {
            sleep(10);
            continue;
        }
        AVPacket* pkt = (AVPacket*)m_videoQueue.dequeue();
        if (pkt == nullptr) break;
        int retValue = avcodec_send_packet(m_pVideoCodecCtx, pkt);   // 压缩编码的数据包(AVPacket)以供解码
        if (retValue != 0) {
            av_packet_free(&pkt);
        }
        int decodeRet = avcodec_receive_frame(m_pVideoCodecCtx, m_pVideoFrame);
        if (decodeRet == 0) {
            convertAndRenderVideo(m_pVideoFrame, pkt->dts);
        }
        av_packet_free(&pkt);
    }
    m_bVideoThreadRunning = false;
    qDebug() << "video decode thread exit";
}

float AVCodecHandler::getAudioTimeStampFromPts(qint64 pts) {
    // 音频的时间戳
    float aTimeStamp = pts * av_q2d(m_aStreamTimeRational);
    return aTimeStamp;
}

float AVCodecHandler::getVideoTimeStampFromPts(qint64 pts) {
    float vTimeStamp = pts * av_q2d(m_vStreamTimeRational);
    return vTimeStamp;
}


