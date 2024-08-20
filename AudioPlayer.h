//
// Created by Administrator on 2024/3/30.
//

#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QAudioSink>
#include <QMutex>


class AudioPlayer {
public:
    //单例模式
    static AudioPlayer* getInstance();
    AudioPlayer();
    ~AudioPlayer();
    void startAudioPlay();
    void stopAudioPlay();

    int getFreeSpace();
    void setVolume(double volume);
    bool writeAudioData(const char* data, int len);

    void playAduio(bool bPlay);

    void setSampleRate(int rate) {m_sampleRate = rate;};
    void setChannelCount(int count) {m_channelCount = count;};
    void setSampleSize(int size) {m_sampleSize = size;};

private:
    int m_sampleRate = 44100;
    int m_channelCount = 2;
    int m_sampleSize = 16;
    QAudioSink* m_pAudioSink = nullptr;
    QIODevice* m_pAudioDevice = nullptr;
    QMutex m_mutex;

};



#endif //AUDIOPLAYER_H
