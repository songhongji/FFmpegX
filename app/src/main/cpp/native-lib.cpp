//
// Created by 宋洪基 on 2019/3/17.
//


#include <jni.h>
#include <string>


extern "C"
{
#include <android/native_window_jni.h>
#include <libavfilter/avfilter.h>
#include <libavcodec/avcodec.h>
//封装格式处理
#include <libavformat/avformat.h>
//像素处理
#include <libswscale/swscale.h>
#include <unistd.h>
#include<android/log.h>


#define TAG    "winter-jni" // 这个是自定义的LOG的标识
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,TAG,__VA_ARGS__) // 定义LOGD类型

JNIEXPORT jstring JNICALL
Java_com_xys_ffmpegx_MainActivity_stringFromNDK(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from CPP";
    return env->NewStringUTF(hello.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_xys_ffmpegx_MainActivity_avFormatInfo(JNIEnv *env, jobject instance) {
    char info[40000] = {0};

    av_register_all();

    AVInputFormat *inputFormat = av_iformat_next(NULL);
    AVOutputFormat *outputFormat = av_oformat_next(NULL);

    while (inputFormat != NULL) {
        sprintf(info, "%sInput: %s\n", info, inputFormat->name);
        inputFormat = inputFormat->next;
    }

    while (outputFormat != NULL) {
        sprintf(info, "%sOutput: %s\n", info, outputFormat->name);
        outputFormat = outputFormat->next;
    }

    return env->NewStringUTF(info);
}


JNIEXPORT void JNICALL
Java_com_xys_ffmpegx_media_FFVideoView_render(JNIEnv *env, jobject instance, jstring url_,
                                                 jobject surface) {
    const char *url = env->GetStringUTFChars(url_, 0);

    // 注册。
    av_register_all();
    // 打开地址并且获取里面的内容  avFormatContext是内容的一个上下文
    AVFormatContext *avFormatContext = avformat_alloc_context();
    avformat_open_input(&avFormatContext, url, NULL, NULL);
    avformat_find_stream_info(avFormatContext, NULL);

    // 找出视频流
    int video_index = -1;
    for (int i = 0; i < avFormatContext->nb_streams; ++i) {
        if (avFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_index = i;
        }
    }
    // 解码  转换  绘制
    // 获取解码器上下文
    AVCodecContext *avCodecContext = avFormatContext->streams[video_index]->codec;
    // 获取解码器
    AVCodec *avCodec = avcodec_find_decoder(avCodecContext->codec_id);
    // 打开解码器
    if (avcodec_open2(avCodecContext, avCodec, NULL) < 0) {
        // 打开失败。
        return;
    }
    // 申请AVPacket和AVFrame，
    // 其中AVPacket的作用是：保存解码之前的数据和一些附加信息，如显示时间戳（pts）、解码时间戳（dts）、数据时长，所在媒体流的索引等；
    // AVFrame的作用是：存放解码过后的数据。
    AVPacket *avPacket = (AVPacket *) av_malloc(sizeof(AVPacket));
    av_init_packet(avPacket);
    // 分配一个AVFrame结构体,AVFrame结构体一般用于存储原始数据，指向解码后的原始帧
    AVFrame *avFrame = av_frame_alloc();
    //分配一个AVFrame结构体，指向存放转换成rgb后的帧
    AVFrame *rgb_frame = av_frame_alloc();
    // rgb_frame是一个缓存区域，所以需要设置。
    // 缓存区
    uint8_t *out_buffer = (uint8_t *) av_malloc(
            avpicture_get_size(AV_PIX_FMT_RGBA, avCodecContext->width, avCodecContext->height));
    // 与缓存区相关联，设置rgb_frame缓存区
    avpicture_fill((AVPicture *) rgb_frame, out_buffer, AV_PIX_FMT_RGBA, avCodecContext->width,
                   avCodecContext->height);
    // 原生绘制，需要ANativeWindow
    ANativeWindow *pANativeWindow = ANativeWindow_fromSurface(env, surface);
    if (pANativeWindow == 0) {
        // 获取native window 失败
        return;
    }
    SwsContext *swsContext = sws_getContext(
            avCodecContext->width,
            avCodecContext->height,
            avCodecContext->pix_fmt,
            avCodecContext->width,
            avCodecContext->height,
            AV_PIX_FMT_RGBA,
            SWS_BICUBIC,
            NULL,
            NULL,
            NULL);
    // 视频缓冲区
    ANativeWindow_Buffer native_outBuffer;
    // 开始解码了。
    int frameCount;
    while (av_read_frame(avFormatContext, avPacket) >= 0) {
        if (avPacket->stream_index == video_index) {
            avcodec_decode_video2(avCodecContext, avFrame, &frameCount, avPacket);
            // 当解码一帧成功过后，我们转换成rgb格式并且绘制。
            if (frameCount) {
                ANativeWindow_setBuffersGeometry(pANativeWindow, avCodecContext->width,
                                                 avCodecContext->height, WINDOW_FORMAT_RGBA_8888);
                // 上锁
                ANativeWindow_lock(pANativeWindow, &native_outBuffer, NULL);
                // 转换为rgb格式
                sws_scale(swsContext, (const uint8_t *const *) avFrame->data, avFrame->linesize, 0,
                          avFrame->height, rgb_frame->data, rgb_frame->linesize);
                uint8_t *dst = (uint8_t *) native_outBuffer.bits;
                int destStride = native_outBuffer.stride * 4;
                uint8_t *src = rgb_frame->data[0];
                int srcStride = rgb_frame->linesize[0];
                LOGD("memcpy stride = %d height = %d", srcStride, avCodecContext->height);
                for (int i = 0; i < avCodecContext->height; ++i) {
                    memcpy(dst + i * destStride, src + i * srcStride, srcStride);
                }
                ANativeWindow_unlockAndPost(pANativeWindow);
//                usleep(1000 * 16);
            }
        }
        av_free_packet(avPacket);

    }

    ANativeWindow_release(pANativeWindow);
    av_frame_free(&avFrame);
    av_frame_free(&rgb_frame);
    avcodec_close(avCodecContext);
    avformat_free_context(avFormatContext);


    env->ReleaseStringUTFChars(url_, url);
}

//JNIEXPORT void JNICALL
//Java_com_xys_ffmpegx_media_FFVideoView_render(JNIEnv *env, jobject instance, jstring video_url,
//                                              jobject surface) {
//    LOGD("render");
//    const char *url = env->GetStringUTFChars(video_url, 0);
//
//    av_register_all();
//    // 打开地址，获取里面的内容
//    AVFormatContext *avFormatContext = avformat_alloc_context();
//    avformat_open_input(&avFormatContext, url, NULL, NULL);
//    avformat_find_stream_info(avFormatContext, NULL);
//
//    // 找出视频流
//    int video_index = -1;
//    for (int i = 0; i < avFormatContext->nb_streams; ++i) {
//        if (avFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
//            video_index = i;
//        }
//    }
//
//    LOGD("video_index");
//
//    // 获取解码上下文
//    AVCodecContext *avCodecContext = avFormatContext->streams[video_index]->codec;
//    // 获取解码器
//    AVCodec *avCodec = avcodec_find_decoder(avCodecContext->codec_id);
//    // 打开解码器
//    if (avcodec_open2(avCodecContext, avCodec, NULL) < 0) { // 打开失败
//        LOGD("codec fail");
//        return;
//    }
//
//    LOGD("codec success");
//
//    // AVPacket: 保存解码之前的数据和一些附加信息，如显示时间戳、解码时间戳、数据时长、媒体流索引等
//    AVPacket *avPacket = (AVPacket *) av_malloc(sizeof(AVPacket));
//    av_init_packet(avPacket);
//
//    // AVFrame: 存放解码后的数据
//    // 分配一个AVFrame指向解码后的原始帧
//    AVFrame *avFrame = av_frame_alloc();
//    // 分配一个AVFrame指向存放转换成rgb后的帧
//    AVFrame *rgb_frame = av_frame_alloc();
//
//    // 缓存区
//    uint8_t *out_buffer = (uint8_t *) av_malloc(
//            avpicture_get_size(AV_PIX_FMT_RGBA, avCodecContext->width, avCodecContext->height));
//    avpicture_fill((AVPicture *) rgb_frame, out_buffer, AV_PIX_FMT_RGBA, avCodecContext->width,
//                   avCodecContext->height);
//
//    // 原生绘制，需要ANativeWindow
//    ANativeWindow *pANativeWindow = ANativeWindow_fromSurface(env, surface);
//    if (pANativeWindow == 0) {
//        LOGD("get native window fail");
//        // 获取失败
//        return;
//    }
//
//    LOGD("get native window success");
//
//    SwsContext *swsContext = sws_getContext(avCodecContext->width,
//                                            avCodecContext->height,
//                                            avCodecContext->pix_fmt,
//                                            avCodecContext->width,
//                                            avCodecContext->height,
//                                            AV_PIX_FMT_RGBA,
//                                            SWS_BICUBIC,
//                                            NULL,
//                                            NULL,
//                                            NULL);
//    // 视频缓冲区
//    ANativeWindow_Buffer nativeWindow_buffer;
//    int frameCount;
//
//    LOGD("start rgb");
//    while (av_read_frame(avFormatContext, avPacket) >= 0) {
//        LOGD("streamIndex = %d videoIndex = %d", avPacket->stream_index, video_index);
//        if (avPacket->stream_index == video_index) {
//            avcodec_decode_video2(avCodecContext, avFrame, &frameCount, avPacket);
//            // 当解码一帧成功后，转换成rgb格式并且绘制
//            LOGD(" frameCount = %d", frameCount);
//            if (frameCount) {
//                ANativeWindow_setBuffersGeometry(pANativeWindow, avCodecContext->width,
//                                                 avCodecContext->height, WINDOW_FORMAT_RGBA_8888);
//
//                LOGD("lock");
//                // 上锁
//                ANativeWindow_lock(pANativeWindow, &nativeWindow_buffer, NULL);
//                LOGD("change to rgb");
//                // 转换成rgb格式
//                sws_scale(swsContext, (const uint8_t *const *) avFrame->data, avFrame->linesize, 0,
//                          avFrame->height,
//                          rgb_frame->data, rgb_frame->linesize);
//                uint8_t *dst = (uint8_t *) nativeWindow_buffer.bits;
//                int destStride = nativeWindow_buffer.stride * 4;
//                uint8_t *src = rgb_frame->data[0];
//                int srcStride = rgb_frame->linesize[0];
//                LOGD("memcpy stride = %d height = %d", srcStride, avCodecContext->height);
//                for (int i = 0; i < avCodecContext->height, ++i;) {
//                    memcpy(dst + i * destStride, src + i * srcStride, srcStride);
//                }
//                LOGD("unlock");
//                ANativeWindow_unlockAndPost(pANativeWindow);
//            }
//        }
//
//        LOGD("av free");
//        av_free_packet(avPacket);
//    }
//
//    LOGD("end rgb");
//
//    ANativeWindow_release(pANativeWindow);
//    av_frame_free(&avFrame);
//    av_frame_free(&rgb_frame);
//    avcodec_close(avCodecContext);
//    avformat_free_context(avFormatContext);
//
//    env->ReleaseStringUTFChars(video_url, url);
//
//}

}


