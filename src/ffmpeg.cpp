#include "ffmpeg.h"

extern "C" {
#include "ffmpeg/libavformat/avformat.h"
#include "ffmpeg/libavcodec/avcodec.h"
#include "ffmpeg/libswscale/swscale.h"
#include "ffmpeg/libavutil/frame.h"
#include "ffmpeg/libavutil/imgutils.h"
#include "ffmpeg/libavutil/opt.h"
#include "ffmpeg/libavutil/error.h"
}

#include <iostream>
#include <fstream>

#include "av_log.h"
#include "av_string.h"
#include "av_async.h"

static void save_rgb_to_ppm(AVFrame* frame, int width, int height)
{
    FILE* f = fopen("out.ppm", "wb");
    fprintf(f, "P6\n%d %d\n255\n", width, height);
    fwrite(frame->data[0], 1, width * height * 3, f);
    fclose(f);
}

static enum AVPixelFormat get_format(AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts)
{
    for (const enum AVPixelFormat* p = pix_fmts;
        *p != AV_PIX_FMT_NONE; p++) {
        av_log(ctx, AV_LOG_INFO,
            "decoder offers: %s\n",
            av_get_pix_fmt_name(*p));
    }
    return pix_fmts[0];
}

bool screenshot(const std::tstring& video)
{
    //const char* input = "/home/marcello/tmp/1.mp4";
    ////const char* input = "test.mp4";     // 输入视频文件
    //const char* output = "out.png";       // 输出 PNG 文件
    //int screenshot_sec = 5;               // 截第 5 秒的帧
    // "/home/marcello/tmp/1.mp4"
    const char* filename = av::str::toA(video).c_str();

    AVFormatContext* fmt_ctx = nullptr;
    if (avformat_open_input(&fmt_ctx, filename, nullptr, nullptr) != 0) {
        loge("avformat_open_input failed!");
        return false;
    }
    av::async::Exit exit_fmt_ctx([&fmt_ctx] {
        avformat_close_input(&fmt_ctx);
    });
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        loge("avformat_find_stream_info failed");
        return false;
    }

    //
    int video_stream_index = -1;
    for (unsigned i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }
    if (video_stream_index == -1) {
        logw("no vidoe stream data");
        return false;
    }

    //
    AVCodecParameters* codec_par = fmt_ctx->streams[video_stream_index]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codec_par->codec_id);
    if (!codec) {
        logw("no video decoder");
        return false;
    }

    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codec_ctx, codec_par);
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        logw("can not open decoder");
        return false;
    }
    av::async::Exit exit_codec_ctx([&codec_ctx] {
        avcodec_free_context(&codec_ctx);
    });

    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    av::async::Exit exit_frame([&frame, &pkt] {
        if (frame != nullptr) av_frame_free(&frame);
        if (pkt != nullptr) av_packet_free(&pkt);
    });
    

    int dst_width = codec_ctx->width;
    int dst_height = codec_ctx->height;

    // png codec
    const AVCodec* png_codec = avcodec_find_encoder(AV_CODEC_ID_PNG);
    AVCodecContext* png_ctx = avcodec_alloc_context3(png_codec);
    //png_ctx->bit_rate = 400000;
    png_ctx->width = dst_width;
    png_ctx->height = dst_height;
    png_ctx->time_base = { 1, 25 };
    png_ctx->pix_fmt = AV_PIX_FMT_RGB24;
    //av_opt_set_int(png_ctx->priv_data, "compression_level", 0, 0);

    // open png codec
    if (avcodec_open2(png_ctx, png_codec, nullptr) != 0) {
        logw("can not open png codec");
        return false;
    }

    // png frame
    AVFrame* png_frame = av_frame_alloc();
    png_frame->format = png_ctx->pix_fmt;
    png_frame->width = dst_width;
    png_frame->height = dst_height;
    av_image_alloc(png_frame->data, png_frame->linesize, dst_width, dst_height, png_ctx->pix_fmt, 1);

    //
    struct SwsContext* sws_ctx = nullptr;

    // move frame to special
    std::vector<int64_t> tt = { 70 * AV_TIME_BASE, 120 * AV_TIME_BASE , 180 * AV_TIME_BASE };
    
    //int64_t ts = 200 * AV_TIME_BASE;
    //av_seek_frame(fmt_ctx, video_stream_index, tt[0], AVSEEK_FLAG_BACKWARD);
    //avcodec_flush_buffers(codec_ctx);

    for (int i = 0; i < tt.size(); i++) {
        if (tt[i] > fmt_ctx->duration) {
            continue;
        }

        avcodec_flush_buffers(codec_ctx);
        av_seek_frame(fmt_ctx, -1, tt[i], AVSEEK_FLAG_BACKWARD);

        // 读取第一帧
        while (av_read_frame(fmt_ctx, pkt) >= 0) {
            if (pkt->stream_index == video_stream_index) {
                if (avcodec_send_packet(codec_ctx, pkt) == 0) {
                    if (avcodec_receive_frame(codec_ctx, frame) == 0) {

                        if (sws_ctx == nullptr) {
                            AVPixelFormat src_fmt =
                                static_cast<AVPixelFormat>(frame->format);
                            sws_ctx = sws_getContext(codec_ctx->width, codec_ctx->height, src_fmt,
                                dst_width, dst_height, png_ctx->pix_fmt,
                                SWS_BILINEAR, nullptr, nullptr, nullptr);
                        }

                        // key frame
                        //if (frame->flags & AV_FRAME_FLAG_KEY) {
                        //    logi("skip not key frame");
                        //    continue;
                        //}

                        {
                            sws_scale(sws_ctx, frame->data, frame->linesize, 0, codec_ctx->height,
                                png_frame->data, png_frame->linesize);

                            // 
                            AVPacket* png_pkt = av_packet_alloc();
                            av_init_packet(png_pkt);
                            png_pkt->data = nullptr;
                            png_pkt->size = 0;
                            av::async::Exit exit_png_pkt([&png_pkt] {
                                av_packet_free(&png_pkt);
                                });

                            // 
                            if (auto ret = avcodec_send_frame(png_ctx, png_frame); ret != 0) {
                                logw("avcodec_send_frame failed! ret = {}", ret);
                                continue;
                            }

                            //
                            if (auto ret = avcodec_receive_packet(png_ctx, png_pkt); ret != 0) {
                                logw("avcodec_receive_packet failed! ret = {}", ret);
                                continue;
                            }

                            // save
                            char filename[128];
                            sprintf(filename, "frame%ld.png", i);
                            FILE* f = fopen(filename, "wb");
                            if (f == NULL) {
                                loge("open {} failed", av::str::toA(filename));
                                continue;
                            }
                            av::async::Exit exit_f([&f] {
                                fclose(f);
                                });
                            fwrite(png_pkt->data, 1, png_pkt->size, f);
                        }

                        /*const AVCodec* jpgCodec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
                        AVCodecContext* jpgCtx = avcodec_alloc_context3(jpgCodec);
                        AVDictionary* opts = nullptr;
                        av_dict_set(&opts, "jpeg_quality", "100", 0); // 0 ~ 100，默认 75

                        jpgCtx->bit_rate = 4000000;
                        jpgCtx->width = width;
                        jpgCtx->height = height;
                        jpgCtx->time_base = { 1, 25 };
                        jpgCtx->pix_fmt = AV_PIX_FMT_YUVJ420P; // MJPEG要求YUV
                        jpgCtx->color_range = AVCOL_RANGE_JPEG;

                        avcodec_open2(jpgCtx, jpgCodec, &opts);

                        AVFrame* jpgFrame = av_frame_alloc();
                        jpgFrame->format = jpgCtx->pix_fmt;
                        jpgFrame->width = width;
                        jpgFrame->height = height;
                        av_image_alloc(jpgFrame->data, jpgFrame->linesize, width, height, jpgCtx->pix_fmt, 1);

                        struct SwsContext* sws_ctx = sws_getContext(width, height, AV_PIX_FMT_YUVJ420P,
                            width, height, AV_PIX_FMT_YUVJ420P,
                            SWS_LANCZOS, nullptr, nullptr, nullptr);

                        sws_scale(sws_ctx, frame->data, frame->linesize, 0, height,
                            jpgFrame->data, jpgFrame->linesize);

                        AVPacket* jpgPkt = av_packet_alloc();
                        av_init_packet(jpgPkt);
                        jpgPkt->data = nullptr;
                        jpgPkt->size = 0;

                        avcodec_send_frame(jpgCtx, jpgFrame);
                        avcodec_receive_packet(jpgCtx, jpgPkt);

                        av_dict_free(&opts);

                        FILE* f = fopen("frame.jpg", "wb");
                        fwrite(jpgPkt->data, 1, jpgPkt->size, f);
                        fclose(f);*/


                        break; // 只抓第一帧
                    }
                }
            }
            av_packet_unref(pkt);
        }
    }

    return 0;
}