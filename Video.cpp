#include <iostream>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

#include <glm/vec2.hpp>

#include <hppv/glad.h>

#include "Video.hpp"

void Video::init()
{
    av_register_all();
}

void Video::open(const char* file)
{
    if(avformat_open_input(&formatCtx_, file, nullptr, nullptr) != 0)
    {
        std::cout << "Video, avformat_open_input failed: " << std::endl;
        return;
    }

    if(avformat_find_stream_info(formatCtx_, nullptr) != 0)
        return;

    av_dump_format(formatCtx_, 0, file, 0);

    for(std::size_t i = 0; i < formatCtx_->nb_streams; ++i)
    {
        if(formatCtx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStream_ = i;
            break;
        }
    }

    auto* codecCtxStream = formatCtx_->streams[videoStream_]->codec;

    auto* codec = avcodec_find_decoder(codecCtxStream->codec_id);

    if(codec == nullptr)
    {
        std::cout << "Video, unsupported coded" << std::endl;
        return;
    }

    codecCtx_ = avcodec_alloc_context3(codec);

    if(avcodec_copy_context(codecCtx_, codecCtxStream) != 0)
    {
        std::cout << "Video, couldn't copy codec context" << std::endl;
        return;
    }

    if(avcodec_open2(codecCtx_, codec, nullptr) < 0)
    {
        std::cout << "Video, couldn't open codec" << std::endl;
        return;
    }

    frame_ = av_frame_alloc();

    targetFrame_ = av_frame_alloc();

    unsigned char* buffer = nullptr;

    std::size_t numBytes = avpicture_get_size(AV_PIX_FMT_RGBA, codecCtx_->width, codecCtx_->height);

    buffer = static_cast<unsigned char*>(av_malloc(numBytes * sizeof(unsigned char)));

    avpicture_fill((AVPicture *)targetFrame_, buffer, AV_PIX_FMT_RGBA, codecCtx_->width, codecCtx_->height);

    swsCtx_ = sws_getContext(codecCtx_->width, codecCtx_->height, codecCtx_->pix_fmt, codecCtx_->width, codecCtx_->height,
                             AV_PIX_FMT_RGBA, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

    texture_ = hppv::Texture(GL_RGBA8, {codecCtx_->width, codecCtx_->height});
}

void Video::decodeNextFrame()
{
    AVPacket packet;

    if(av_read_frame(formatCtx_, &packet) < 0)
    {
        av_seek_frame(formatCtx_, videoStream_, 0, AVSEEK_FLAG_ANY);
        return;
    }

    if(packet.stream_index == videoStream_)
    {
        int frameFinished;

        avcodec_decode_video2(codecCtx_, frame_, &frameFinished, &packet);

        if(frameFinished)
        {
            sws_scale(swsCtx_, (uint8_t const * const *)frame_->data, frame_->linesize, 0, codecCtx_->height,
                      targetFrame_->data, targetFrame_->linesize);


        }
    }

    av_packet_unref(&packet);
}

void Video::uploadTexture()
{
    auto* data = targetFrame_->data[0];
    glm::ivec2 size = {codecCtx_->width, codecCtx_->height};
    texture_.bind();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, data);
}
