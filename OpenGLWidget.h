//
// Created by Administrator on 2024/4/1.
//

#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLShader>
#include <QOpenGLFunctions>

#include "YUVDataDefined.h"

enum {
    ATTRIBUTE_VERTEX = 0,
    ATTRIBUTE_TEXCOORD = 1
};
class OpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions{
public:
    explicit OpenGLWidget(QWidget *parent = nullptr);
    ~OpenGLWidget() override;

    void rendVideo(H264YUV_Frame *decoder);

protected:
    void initializeGL() override;
    void initializeGLSLShader();
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    void vinitShader();
    GLuint createImageTexture(QString &pathString);

private:
    bool m_bUpdateData = false;
    GLuint m_textures[3];
    QOpenGLShaderProgram *m_pShaderProgram = nullptr;

    int m_nVideoW = 0;
    int m_nVideoH = 0;
    int m_yFrameLengh = 0;
    int m_uFrameLengh = 0;
    int m_vFrameLengh = 0;
    unsigned char* m_pBufYuv420p = nullptr;

    struct Vertex {
        float x, y, z;
        float u, v;
    };

};


#endif //OPENGLWIDGET_H
