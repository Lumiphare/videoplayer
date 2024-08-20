//
// Created by Administrator on 2024/3/30.
//

#include "AudioPlayer.h"
#include <QAudioFormat>

static QMutex mutex;
static AudioPlayer *m_pInstance = nullptr;

AudioPlayer * AudioPlayer::getInstance() {
    QMutexLocker locker(&mutex);
    if (m_pInstance == nullptr) {
        m_pInstance = new AudioPlayer();
    }
    return m_pInstance;
}

AudioPlayer::AudioPlayer() = default;

AudioPlayer::~AudioPlayer() = default;


void AudioPlayer::startAudioPlay() {
    QMutexLocker locker(&mutex);
    QAudioFormat format;
    format.setChannelCount(m_channelCount);
    format.setSampleRate(m_sampleRate);
    format.setSampleFormat(QAudioFormat::Int16);

    m_pAudioSink = new QAudioSink(format);
    m_pAudioDevice = m_pAudioSink->start();
}

void AudioPlayer::stopAudioPlay() {
    QMutexLocker locker(&mutex);
    if (m_pAudioSink!= nullptr) {
        m_pAudioSink->stop();
        delete m_pAudioSink;
        m_pAudioSink = nullptr;
    }
}

int AudioPlayer::getFreeSpace() {
    QMutexLocker locker(&mutex);
    if (m_pAudioSink == nullptr) {
        return 0;
    }
    return m_pAudioSink->bytesFree();
}

void AudioPlayer::setVolume(double volume) {
    QMutexLocker locker(&mutex);
    if (m_pAudioSink == nullptr) {
        return;
    }
    m_pAudioSink->setVolume(volume);
}

bool AudioPlayer::writeAudioData(const char *data, int len) {
    QMutexLocker locker(&mutex);
    if (data == nullptr || len <= 0 || m_pAudioSink == nullptr) {
        return false;
    }
    m_pAudioDevice->write(data, len);
    return true;
}

void AudioPlayer::playAduio(bool bPlay) {
    QMutexLocker locker(&mutex);
    if (m_pAudioSink == nullptr) {
        return;
    }
    if (bPlay) {
        m_pAudioSink->resume();
    } else m_pAudioSink->suspend();
}



