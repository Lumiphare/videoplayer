//
// Created by Administrator on 2024/3/29.
//

#ifndef AVCODECHANDLER_H
#define AVCODECHANDLER_H

#include <QObject>
#include <QSize>

#include "TSQueue.h"
#include "YUVDataDefined.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

enum MediaPlayStatus{
    PLAYING = 0,PAUSE,SEEK,STOP
};
typedef void (*UpdateVideo2GUI_Callback) (H264YUV_Frame* yuv, uintptr_t userData);

class AVCodecHandler {

public:
    explicit AVCodecHandler();
    virtual ~AVCodecHandler();

    QString getVideoFilePath() const;
    void setVideoFilePath(const QString& path);
    int initVideoCodec();

    void startPlayVideo();
    void stopPlayVideo();

    void seekMedia(float fPos);

    void setUpdateVideoCallback(UpdateVideo2GUI_Callback callback, uintptr_t userData);
    void setUpdateProgressCallback(std::function<void(int)> callback);

    QSize getMediaWidthHeight() const;

    void setVolume(double volume);

private:
    void convertAndRenderVideo(AVFrame* videoFrame, long long ppts);
    void convertAndPlayAudio(AVFrame* audioFrame);
    void copyDecodedFrame420(uint8_t* src, uint8_t* dst, int linesize, int width, int height);
    void tickAudioFrameTimeDelay(int64_t pts);
    void tickVideoFrameTimeDelay(int64_t pts);
    void startMediaProcessThread();
    void readMediaFrameThread();
    void audioDecodeThread();
    void videoDecodeThread();
    float getAudioTimeStampFromPts(qint64 pts);
    float getVideoTimeStampFromPts(qint64 pts);
    void sleep(unsigned long msecs);


private:
    QString m_videoFilePath;
    QSize m_mediaWidthHeight;

    QString m_videoPathString = "";
    int m_videoStreamIdx = -1;
    int m_audioStreamIdx = -1;

    bool m_bReadFileEOF = false;

    MediaPlayStatus m_eMediaPlayStatus;
    TSQueue<AVPacket*> m_audioQueue;
    TSQueue<AVPacket*> m_videoQueue;

    uintptr_t m_userDataVideo = 0;
    UpdateVideo2GUI_Callback m_updateVideoCallback = nullptr;
    std::function<void(int)> m_updateProgressCallback = nullptr;

    AVRational m_vStreamTimeRational;   // 记录video时间刻度,分子和分母的比值  num分子，den分母
    AVRational m_aStreamTimeRational;

    SwrContext* m_pAudioSwrCtx = nullptr;
    SwsContext* m_pVideoSwsCtx = nullptr;

    uint8_t*    m_pYUV420Buffer = nullptr;
    uint8_t*    m_pSwrBuffer = nullptr;
    int         m_swrBuffSize = 0;

    AVFrame* m_pYUVFrame = nullptr;    // Frame表示原始数据  AVPacket是编码后的数据
    AVFrame* m_pVideoFrame = nullptr;
    AVFrame* m_pAudioFrame = nullptr;

    AVFormatContext* m_pFormatCtx = nullptr;
    AVCodecContext* m_pVideoCodecCtx = nullptr;
    AVCodecContext* m_pAudioCodecCtx = nullptr;

    float m_nCurrAudioTimeStamp = 0.0f;
    float m_nLastAudioTimeStamp = 0.0f;  // 当前视频的时间戳
    float m_nAudioFrameDuration = 0.0f;  // 音频帧时长

    float m_nSeekingPos = 0.0f;

    int m_sampleRate = 44100;    // 采样率
    int m_sampleSize = 16;      // 比特率
    int m_channel = 2;          // 立体声通道
    double m_volumeRation = 1;      // 音量

};


#endif //AVCODECHANDLER_H
