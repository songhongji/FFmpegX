//
// Created by 宋洪基 on 2019/3/17.
//


#include <jni.h>
#include <string>

extern "C" JNIEXPORT jstring JNICALL
Java_com_xys_ffmpegx_MainActivity_stringFromNDK(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from CPP";
    return env->NewStringUTF(hello.c_str());
}