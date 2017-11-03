#pragma once

#include <hppv/Texture.hpp>

struct AVFormatContext;
struct SwsContext;
struct AVCodecContext;
struct AVFrame;

class Video
{
public:
    Video() = default;
    ~Video();
    Video(const Video&) = delete;
    Video& operator=(const Video&) = delete;
    Video(Video&&) = delete;
    Video& operator=(Video&&) = delete;

    static void init();

    void open(const char* file);

    void decodeNextFrame();

    void uploadTexture();

    hppv::Texture texture_;

private:
    AVFormatContext* formatCtx_ = nullptr;
    int videoStream_;
    AVCodecContext* codecCtx_ = nullptr;
    AVFrame* frame_ = nullptr;
    unsigned char* buffer_ = nullptr;
    AVFrame* outputFrame_ = nullptr;
    SwsContext* swsCtx_ = nullptr;

    void clearVideo();
};
