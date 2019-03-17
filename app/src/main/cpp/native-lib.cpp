//
// Created by 宋洪基 on 2019/3/17.
//


#include <jni.h>
#include <string>


extern "C"
{
#include <libavformat/avformat.h>


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

}


