//
// Created by Administrator on 2024/3/29.
//

// You may need to build the project (run Qt uic code generator) to get "ui_PlayerWindow.h" resolved

#include "playerwindow.h"
#include "ui_PlayerWindow.h"
#include "AVCodecHandler.h"
#include <QMimeData>

PlayerWindow::PlayerWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::PlayerWindow) {
    ui->setupUi(this);

    setAcceptDrops(true);  // 设置可拖动

    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, Qt::black);

    setPlayerCentralWidget();
    ui->progressBar->installEventFilter(this);
    // setCentralWidget(m_pOpenGLWidget);
}

PlayerWindow::~PlayerWindow() {
    releasePlayerCentralWidget();
    delete ui;
}

void PlayerWindow::updateVideoData(H264YUV_Frame *frame, uintptr_t data) {
    if (data == 0 || frame == nullptr) {
        return;
    }
    auto *mainWindow = (PlayerWindow*)(data);
    if (mainWindow!= nullptr) {
        mainWindow->updateYUVFrameData(frame);
    }
}

void PlayerWindow::updateYUVFrameData(H264YUV_Frame *frame) {
    // TODO OpenGL绘制
    if (frame == nullptr) return;
    m_pOpenGLWidget->rendVideo(frame);

}

void PlayerWindow::setPlayerCentralWidget()
{
    m_AVCodecHandler = new AVCodecHandler();
    m_AVCodecHandler->setUpdateVideoCallback(updateVideoData, reinterpret_cast<uintptr_t>(this));
    m_AVCodecHandler->setUpdateProgressCallback([this](auto && PH1) { updateProgessBar(std::forward<decltype(PH1)>(PH1)); });

    // m_pOpenGLWidget = new OpenGLWidget(this);
    m_pOpenGLWidget = ui->openGLWidget;
    m_pOpenGLWidget->setAutoFillBackground(true);

    // setCentralWidget(nullptr);
}

void PlayerWindow::releasePlayerCentralWidget()
{
    if (m_AVCodecHandler != NULL) {
        delete m_AVCodecHandler;
        m_AVCodecHandler = NULL;
    }
    // setCentralWidget(NULL);

}

void PlayerWindow::dropEvent(QDropEvent *event)
{
    QUrl url = event->mimeData()->urls().first();
    if (url.isEmpty()) {
        qDebug() << "URL is Empty";
        return;
    }
//TODO 完善mac平台 视频06
#ifdef macx
#endif
    QByteArray byte = QByteArray(url.toString().toUtf8());
    QUrl encodeUrl(byte);
    encodeUrl = encodeUrl.fromEncoded(byte);
    QString realPath = encodeUrl.toLocalFile();
    openMediaPlayerWithPath(realPath);
}

void PlayerWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void PlayerWindow::openMediaPlayerWithPath(const QString &filePath)
{
    if (filePath.isEmpty()) {
        qDebug() << "filePath is Empty";
        return;
    }
    releasePlayerCentralWidget();
    setPlayerCentralWidget();

    if (m_AVCodecHandler != NULL) {
        m_AVCodecHandler->setVideoFilePath(filePath);
        m_AVCodecHandler->initVideoCodec();
    }
    QSize videoSize = m_AVCodecHandler->getMediaWidthHeight();

}

void PlayerWindow::on_pushButton_clicked() {
    if (ui->pushButton->text() == "播放") {
        m_AVCodecHandler->startPlayVideo();
        ui->pushButton->setText("暂停");
    } else {
        m_AVCodecHandler->stopPlayVideo();
        ui->pushButton->setText("播放");
    }
}

void PlayerWindow::on_verticalSlider_valueChanged(int value) {
    m_AVCodecHandler->setVolume(1.0 * value / 1000);
}

void PlayerWindow::updateProgessBar(int value) {
    if (progressSliderPressed) return;
    ui->progressBar->setValue(value);
}
bool PlayerWindow::eventFilter(QObject *obj, QEvent *event) {
    if(obj ==ui->progressBar && event->type()==QEvent::MouseButtonPress) {
        auto *mouseEvent = dynamic_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            int value = QStyle::sliderValueFromPosition(ui->progressBar->minimum(),
                ui->progressBar->maximum(), mouseEvent->pos().x(), ui->progressBar->width());
            ui->progressBar->setValue(value);
        }
        progressSliderPressed = true;
        m_AVCodecHandler->seekMedia(ui->progressBar->value());
        progressSliderPressed = false;
        return true;
    }
    return QObject::eventFilter(obj,event);
}
