#pragma once

#include <string>
#include <cassert>

#include <hppv/Texture.hpp>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct SwsContext;

class Video
{
public:
    // call once before creating any Video variables
    static void init();

    Video() = default;
    explicit Video(const std::string& filename);

    void decodeFrame();

    void updateTexture();

    bool isValid() const {return ffmpeg_.swsCtx;}

    hppv::Texture texture;

private:
    int videoStream_;

    class FFmpegRes
    {
    public:
        FFmpegRes() = default;
        ~FFmpegRes() {clean();}

        FFmpegRes(const FFmpegRes&) = delete;
        FFmpegRes& operator=(const FFmpegRes&) = delete;

        FFmpegRes(FFmpegRes&& rhs) {*this = std::move(rhs);}

        FFmpegRes& operator=(FFmpegRes&& rhs)
        {
            assert(this != &rhs);

            clean();

            formatCtx = rhs.formatCtx;
            codecCtx = rhs.codecCtx;
            frame = rhs.frame;
            outputFrame = rhs.outputFrame;
            buffer = rhs.buffer;
            swsCtx = rhs.swsCtx;

            rhs.formatCtx = nullptr;
            rhs.codecCtx = nullptr;
            rhs.frame = nullptr;
            rhs.outputFrame = nullptr;
            rhs.buffer = nullptr;
            rhs.swsCtx = nullptr;

            return *this;
        }

        AVFormatContext* formatCtx = nullptr;
        AVCodecContext* codecCtx = nullptr;
        AVFrame* frame = nullptr;
        AVFrame* outputFrame = nullptr;
        unsigned char* buffer = nullptr;
        SwsContext* swsCtx = nullptr;

    private:
        void clean();
    }
    ffmpeg_;
};
