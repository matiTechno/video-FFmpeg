#pragma once

#include <hppv/Texture.hpp>

struct AVFormatContext;
struct SwsContext;
struct AVCodecContext;
struct AVFrame;

class Video
{
public:
    static void init();

    void open(const char* file);

    void decodeNextFrame();

    void uploadTexture();

    hppv::Texture texture_;

private:
    AVFormatContext* formatCtx_ = nullptr;
    SwsContext* swsCtx_ = nullptr;
    int videoStream_;
    AVCodecContext* codecCtx_ = nullptr;
    AVFrame* frame_ = nullptr;
    AVFrame* targetFrame_ = nullptr;
};
