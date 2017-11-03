#include <iostream>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <glm/vec2.hpp>

#include <hppv/glad.h>
#include <hppv/Deleter.hpp>

#include "Video.hpp"

void Video::init()
{
    av_register_all();
}

Video::~Video()
{
    clearVideo();
}

void Video::clearVideo()
{
    if(formatCtx_)
        avformat_close_input(&formatCtx_);

    if(codecCtx_)
        avcodec_free_context(&codecCtx_);

    if(frame_)
        av_frame_free(&frame_);

    if(outputFrame_)
        av_frame_free(&outputFrame_);

    av_free(buffer_);
}

void Video::open(const char* file)
{
    std::cout << "opening " << file << std::endl;

    clearVideo();

    if(avformat_open_input(&formatCtx_, file, nullptr, nullptr) < 0)
    {
        std::cout << "avformat_open_input failed" << std::endl;
        return;
    }

    if(avformat_find_stream_info(formatCtx_, nullptr) < 0)
    {
        std::cout << "avformat_find_stream_info failed" << std::endl;
        return;
    }

    {
        std::size_t i;
        for(i = 0; i < formatCtx_->nb_streams; ++i)
        {
            if(formatCtx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                videoStream_ = i;
                break;
            }
        }

        if(i == formatCtx_->nb_streams)
        {
            std::cout << "could not find video stream" << std::endl;
        }
    }

    {
        auto* codecParams = formatCtx_->streams[videoStream_]->codecpar;
        auto* codec = avcodec_find_decoder(codecParams->codec_id);

        if(codec == nullptr)
        {
            std::cout << "avcodec_find_decoder failed" << std::endl;
            return;
        }

        codecCtx_ = avcodec_alloc_context3(codec);

        if(avcodec_parameters_to_context(codecCtx_, codecParams) < 0)
        {
            std::cout << "avcodec_parameters_to_context failed" << std::endl;
            return;
        }

        if(avcodec_open2(codecCtx_, codec, nullptr) < 0)
        {
            std::cout << "avcodec_open2 failed" << std::endl;
            return;
        }
    }

    frame_ = av_frame_alloc();
    outputFrame_ = av_frame_alloc();

    const auto format = AV_PIX_FMT_RGBA;
    const int align = 32;

    auto size = av_image_get_buffer_size(format, codecCtx_->width, codecCtx_->height, align);

    buffer_ = static_cast<unsigned char*>(av_malloc(size * sizeof(char)));

    av_image_fill_arrays(outputFrame_->data, outputFrame_->linesize, buffer_, format, codecCtx_->width, codecCtx_->height, align);

    swsCtx_ = sws_getContext(codecCtx_->width, codecCtx_->height, codecCtx_->pix_fmt, codecCtx_->width, codecCtx_->height,
                             format, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

    texture_ = hppv::Texture(GL_RGBA8, {codecCtx_->width, codecCtx_->height});
}

void Video::decodeNextFrame()
{
    AVPacket packet;
    hppv::Deleter delPacket;
    delPacket.set([&packet]{av_packet_unref(&packet);});

    if(av_read_frame(formatCtx_, &packet) < 0)
    {
        av_seek_frame(formatCtx_, videoStream_, 0, AVSEEK_FLAG_ANY);
        return;
    }

    if(avcodec_send_packet(codecCtx_, &packet) < 0)
        return;

    if(avcodec_receive_frame(codecCtx_, frame_) < 0)
        return;

    sws_scale(swsCtx_, reinterpret_cast<const unsigned char* const*>(frame_->data), frame_->linesize, 0, codecCtx_->height,
              outputFrame_->data, outputFrame_->linesize);
}

void Video::uploadTexture()
{
    texture_.bind();
    auto size = texture_.getSize();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, outputFrame_->data[0]);
}
