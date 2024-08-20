//
// Created by Administrator on 2024/4/1.
//

#include "OpenGLWidget.h"

OpenGLWidget::OpenGLWidget(QWidget *parent): QOpenGLWidget(parent) {
    m_pBufYuv420p = nullptr;
    m_pShaderProgram = nullptr;

    m_nVideoH = 0;
    m_nVideoW = 0;
    m_yFrameLengh = 0;
    m_uFrameLengh = 0;
    m_vFrameLengh = 0;
}

OpenGLWidget::~OpenGLWidget() {
    if (m_pShaderProgram != nullptr) {
        delete m_pShaderProgram;
        m_pShaderProgram = nullptr;
    }
    if (m_pBufYuv420p != nullptr) {
        delete[] m_pBufYuv420p;
        m_pBufYuv420p = nullptr;
    }
    glDeleteTextures(3, m_textures);
}

void OpenGLWidget::rendVideo(H264YUV_Frame *decoder) {
    if (decoder == nullptr) return;
    if (m_nVideoH != decoder->height || m_nVideoW != decoder->width) {
        if (m_pBufYuv420p != nullptr) {
            delete[] m_pBufYuv420p;
            m_pBufYuv420p = nullptr;
        }
    }
    m_nVideoH = decoder->height;
    m_nVideoW = decoder->width;
    m_yFrameLengh = decoder->luma.length;
    m_uFrameLengh = decoder->chromaB.length;
    m_vFrameLengh = decoder->chromaR.length;

    // 存储YUV数据
    int nLen = m_yFrameLengh + m_uFrameLengh + m_vFrameLengh;
    if (m_pBufYuv420p == nullptr) {
        m_pBufYuv420p = new unsigned char[nLen];
    }
    memcpy(m_pBufYuv420p, decoder->luma.dataBuffer, m_yFrameLengh);
    memcpy(m_pBufYuv420p + m_yFrameLengh, decoder->chromaB.dataBuffer, m_uFrameLengh);
    memcpy(m_pBufYuv420p + m_yFrameLengh + m_uFrameLengh, decoder->chromaR.dataBuffer, m_vFrameLengh);

    m_bUpdateData = true;
    update();
}

void OpenGLWidget::initializeGL() {
    m_bUpdateData = false;
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.6f, 0.3f, 0.6f, 1.0f);
    glGenTextures(3, m_textures);
    initializeGLSLShader();
}

void OpenGLWidget::initializeGLSLShader() {
    QOpenGLShader* vertexShader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    bool bCompileSucc = vertexShader->compileSourceFile(":/res/vertex.vert");
    if (!bCompileSucc) {
        qDebug() << "Vertex shader compile failed." <<  vertexShader->log();
        return;
    }

    QOpenGLShader* fragmentShader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    bCompileSucc = fragmentShader->compileSourceFile(":/res/fragment.frag");
    if (!bCompileSucc) {
        qDebug() << "Fragment shader compile failed." << fragmentShader->log();
        return;
    }
    m_pShaderProgram = new QOpenGLShaderProgram();
    m_pShaderProgram->addShader(vertexShader);
    m_pShaderProgram->addShader(fragmentShader);
    bool linkstatus =  m_pShaderProgram->link();
    if (!linkstatus) {
         qDebug() << "Shader program link failed." << m_pShaderProgram->log();
    }
    if (vertexShader != nullptr) {
        delete vertexShader;
        vertexShader = nullptr;
    }
    if (fragmentShader != nullptr) {
        delete fragmentShader;
        fragmentShader = nullptr;
    }
}

void OpenGLWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void OpenGLWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    if (!m_bUpdateData) return;
    static Vertex trinagleVertices[] = {
        { -1, 1, 1, 0, 0 },
        { -1, -1, 1, 0, 1 },
        { 1, 1, 1, 1, 0 },
        { 1, -1, 1, 1, 1 },
    };
    QMatrix4x4 matrix;
    matrix.ortho(-1.0, 1.0, -1.0, 1.0, 0.1, 1000);
    matrix.translate(0.0, 0.0, -3);
    m_pShaderProgram->bind();
    m_pShaderProgram->setUniformValue("uni_mat", matrix);
    m_pShaderProgram->enableAttributeArray("attr_position");
    m_pShaderProgram->enableAttributeArray("attr_uv");
    m_pShaderProgram->setAttributeArray("attr_position", GL_FLOAT, trinagleVertices, 3,sizeof(Vertex));
    m_pShaderProgram->setAttributeArray("attr_uv",GL_FLOAT, &trinagleVertices[0].u,2, sizeof(Vertex));
    m_pShaderProgram->setUniformValue("uni_textureY", 0);

    // 配置2D纹理    GL_LUMINANCE:表示纹理中的像素数据将只包含亮度（灰度）信息，没有颜色信息
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textures[0]);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_nVideoW, m_nVideoH, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, m_pBufYuv420p);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    m_pShaderProgram->setUniformValue("uni_textureU", 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_textures[1]);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_nVideoW/2, m_nVideoH/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, m_pBufYuv420p + m_yFrameLengh);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    m_pShaderProgram->setUniformValue("uni_textureV", 2);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_textures[2]);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_nVideoW/2, m_nVideoH/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, m_pBufYuv420p + m_yFrameLengh + m_uFrameLengh);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    m_pShaderProgram->disableAttributeArray("attr_position");
    m_pShaderProgram->disableAttributeArray("attr_uv");
    m_pShaderProgram->release();

}
