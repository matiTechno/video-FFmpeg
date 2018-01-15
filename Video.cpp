#include <iostream>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <hppv/glad.h>

#include "Video.hpp"

void Video::init()
{
    av_register_all();
}

Video::Video(const std::string& filename)
{
    std::cout << "Video: opening " << filename << std::endl;

    if(avformat_open_input(&ffmpeg_.formatCtx, filename.c_str(), nullptr, nullptr) < 0)
    {
        std::cout << "avformat_open_input failed" << std::endl;
        return;
    }

    if(avformat_find_stream_info(ffmpeg_.formatCtx, nullptr) < 0)
    {
        std::cout << "avformat_find_stream_info failed" << std::endl;
        return;
    }

    {
        std::size_t i;
        for(i = 0; i < ffmpeg_.formatCtx->nb_streams; ++i)
        {
            if(ffmpeg_.formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                videoStream_ = i;
                break;
            }
        }

        if(i == ffmpeg_.formatCtx->nb_streams)
        {
            std::cout << "could not find any video stream" << std::endl;
            return;
        }
    }

    {
        const auto* const codecParams = ffmpeg_.formatCtx->streams[videoStream_]->codecpar;
        const auto* const codec = avcodec_find_decoder(codecParams->codec_id);

        if(codec == nullptr)
        {
            std::cout << "avcodec_find_decoder failed" << std::endl;
            return;
        }

        ffmpeg_.codecCtx = avcodec_alloc_context3(codec);

        if(avcodec_parameters_to_context(ffmpeg_.codecCtx, codecParams) < 0)
        {
            std::cout << "avcodec_parameters_to_context failed" << std::endl;
            return;
        }

        if(avcodec_open2(ffmpeg_.codecCtx, codec, nullptr) < 0)
        {
            std::cout << "avcodec_open2 failed" << std::endl;
            return;
        }
    }

    constexpr auto format = AV_PIX_FMT_RGBA;

    ffmpeg_.swsCtx = sws_getContext(ffmpeg_.codecCtx->width, ffmpeg_.codecCtx->height, ffmpeg_.codecCtx->pix_fmt,
                                    ffmpeg_.codecCtx->width, ffmpeg_.codecCtx->height, format, SWS_FAST_BILINEAR,
                                    nullptr, nullptr, nullptr);

    constexpr auto align = 32;

    {
        const auto size = av_image_get_buffer_size(format, ffmpeg_.codecCtx->width, ffmpeg_.codecCtx->height, align);
        ffmpeg_.buffer = static_cast<unsigned char*>(av_malloc(size * sizeof(char)));
    }

    ffmpeg_.outputFrame = av_frame_alloc();

    av_image_fill_arrays(ffmpeg_.outputFrame->data, ffmpeg_.outputFrame->linesize, ffmpeg_.buffer, format,
                         ffmpeg_.codecCtx->width, ffmpeg_.codecCtx->height, align);

    ffmpeg_.frame = av_frame_alloc();

    texture = hppv::Texture(GL_RGBA8, {ffmpeg_.codecCtx->width, ffmpeg_.codecCtx->height});
}

void Video::decodeFrame()
{
    class PacketDel
    {
    public:
        PacketDel(AVPacket* const packet): packet_(packet) {}
        ~PacketDel() {av_packet_unref(packet_);}
        PacketDel(const PacketDel&) = delete;
        PacketDel& operator=(const PacketDel&) = delete;
        PacketDel(PacketDel&&) = delete;
        PacketDel& operator=(PacketDel&&) = delete;

    private:
        AVPacket* const packet_;
    };

    if(!isValid())
        return;

    AVPacket packet;

    const auto readFrame = av_read_frame(ffmpeg_.formatCtx, &packet);

    PacketDel packetDel(&packet);

    if(readFrame < 0)
    {
        av_seek_frame(ffmpeg_.formatCtx, videoStream_, 0, AVSEEK_FLAG_ANY);
        return;
    }

    if(avcodec_send_packet(ffmpeg_.codecCtx, &packet) < 0)
        return;

    if(avcodec_receive_frame(ffmpeg_.codecCtx, ffmpeg_.frame) < 0)
        return;

    sws_scale(ffmpeg_.swsCtx, reinterpret_cast<const unsigned char* const*>(ffmpeg_.frame->data), ffmpeg_.frame->linesize, 0,
              ffmpeg_.codecCtx->height, ffmpeg_.outputFrame->data, ffmpeg_.outputFrame->linesize);
}

void Video::updateTexture()
{
    if(!isValid())
        return;

    texture.bind();
    const auto size = texture.getSize();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, ffmpeg_.outputFrame->data[0]);
}

void Video::FFmpeg::clean()
{
    if(formatCtx)
        avformat_close_input(&formatCtx);

    if(codecCtx)
        avcodec_free_context(&codecCtx);

    // pointer can be nullptr
    sws_freeContext(swsCtx);
    swsCtx = nullptr;

    // pointer can be nullptr
    av_freep(&buffer);

    if(outputFrame)
        av_frame_free(&outputFrame);

    if(frame)
        av_frame_free(&frame);
}
