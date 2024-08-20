//
// Created by Administrator on 2024/3/29.
//

#ifndef PLAYERWINDOW_H
#define PLAYERWINDOW_H

#include "AVCodecHandler.h"
#include <QDropEvent>
#include <QDragEnterEvent>
#include "OpenGLWidget.h"
#include <QMainWindow>
#include <QStyle>

QT_BEGIN_NAMESPACE
namespace Ui { class PlayerWindow; }
QT_END_NAMESPACE

class PlayerWindow : public QMainWindow {
Q_OBJECT

public:
    explicit PlayerWindow(QWidget *parent = nullptr);
    ~PlayerWindow() override;

private:
    static void updateVideoData(H264YUV_Frame* frame, uintptr_t data);
    void updateYUVFrameData(H264YUV_Frame* frame);
    void setPlayerCentralWidget();
    void releasePlayerCentralWidget();

protected:
    void dropEvent(QDropEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;

private slots:
    void openMediaPlayerWithPath(const QString& filePath);
    void on_pushButton_clicked();
    void on_verticalSlider_valueChanged(int value);
    void updateProgessBar(int value);
    bool eventFilter(QObject *obj, QEvent *event) override;
private:
    Ui::PlayerWindow *ui;
    AVCodecHandler* m_AVCodecHandler = nullptr;
    OpenGLWidget* m_pOpenGLWidget = nullptr;
    bool progressSliderPressed = false;
};


#endif //PLAYERWINDOW_H
