//
// Created by Administrator on 2024/3/30.
//

#ifndef YUVDATADEFINED_H
#define YUVDATADEFINED_H

// typedef struct H264FrameDef {
//     unsigned int length;
//     unsigned char* dataBuffer;
// } H264Frame;

struct H264Frame{
    unsigned int length;
    unsigned char* dataBuffer;
};
struct H264YUV_Frame {
    unsigned int width;
    unsigned int height;
    H264Frame luma;
    H264Frame chromaB;
    H264Frame chromaR;
    long long pts;
};
struct CDecodedAudioData {
    unsigned char* databuff;
    unsigned int dataLength;
};
#endif //YUVDATADEFINED_H
