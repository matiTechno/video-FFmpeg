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

    class FFmpeg
    {
    public:
        FFmpeg() = default;
        ~FFmpeg() {clean();}

        FFmpeg(const FFmpeg&) = delete;
        FFmpeg& operator=(const FFmpeg&) = delete;

        FFmpeg(FFmpeg&& rhs) {*this = std::move(rhs);}

        FFmpeg& operator=(FFmpeg&& rhs)
        {
            assert(this != &rhs);

            clean();

            formatCtx = rhs.formatCtx;
            codecCtx = rhs.codecCtx;
            swsCtx = rhs.swsCtx;
            buffer = rhs.buffer;
            outputFrame = rhs.outputFrame;
            frame = rhs.frame;

            rhs.formatCtx = nullptr;
            rhs.codecCtx = nullptr;
            rhs.swsCtx = nullptr;
            rhs.buffer = nullptr;
            rhs.outputFrame = nullptr;
            rhs.frame = nullptr;

            return *this;
        }

        AVFormatContext* formatCtx = nullptr;
        AVCodecContext* codecCtx = nullptr;
        SwsContext* swsCtx = nullptr;
        unsigned char* buffer = nullptr;
        AVFrame* outputFrame = nullptr;
        AVFrame* frame = nullptr;

    private:
        void clean();
    }
    ffmpeg_;
};
