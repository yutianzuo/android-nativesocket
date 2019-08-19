//
// Created by yutianzuo on 2019-08-09.
//

#ifndef NATIVESOCKET_JNIDEF_H
#define NATIVESOCKET_JNIDEF_H

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT
jstring
JNICALL
Java_com_github_yutianzuo_nativesock_JniDef_dnsTest(JNIEnv *, jclass, jstring, jstring);

JNIEXPORT
jstring
JNICALL
Java_com_github_yutianzuo_nativesock_JniDef_sendFile(JNIEnv *, jclass, jstring, jstring);

JNIEXPORT
void
JNICALL
Java_com_github_yutianzuo_nativesock_JniDef_recvFile(JNIEnv *env, jclass jobj, jstring, jint, jobject);


JNIEXPORT
void
JNICALL
Java_com_github_yutianzuo_nativesock_JniDef_recvDone(JNIEnv *env, jclass jobj);

#ifdef __cplusplus
};
#endif


#endif //NATIVESOCKET_JNIDEF_H
