//
// Created by yutianzuo on 2019-08-09.
//

#include <complex>
#include "jnidef.h"

#include "./netutils/dns.h"


#include "tcptransfile/transrecvctrl.h"
#include "tcptransfile/transsendctrl.h"

#include "toolbox/string_x.h"
#include "toolbox/miscs.h"

JavaVM *g_jvm;
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    g_jvm = vm;
    return JNI_VERSION_1_6;
}

class JNIEnvPtr {
public:
    JNIEnvPtr() : env_{nullptr}, need_detach_{false} {
        if (g_jvm->GetEnv((void**) &env_, JNI_VERSION_1_6) ==
            JNI_EDETACHED) {
            g_jvm->AttachCurrentThread(&env_, nullptr);
            need_detach_ = true;
        }
    }

    ~JNIEnvPtr() {
        if (need_detach_) {
            g_jvm->DetachCurrentThread();
        }
    }

    JNIEnv* operator->() {
        return env_;
    }

private:
    JNIEnvPtr(const JNIEnvPtr&) = delete;
    JNIEnvPtr& operator=(const JNIEnvPtr&) = delete;

private:
    JNIEnv* env_;
    bool need_detach_;
};

jstring Java_com_github_yutianzuo_nativesock_JniDef_dnsTest(JNIEnv *env, jclass jobj,
                                                            jstring strDnsServer, jstring strHost) {
    const char *str_dns_server = nullptr;
    const char *str_host = nullptr;

    if (strDnsServer) {
        str_dns_server = env->GetStringUTFChars(strDnsServer, 0);
    }
    if (strHost) {
        str_host = env->GetStringUTFChars(strHost, 0);
    }

    std::string ret;
    SocketBase *base = new DNSQuery();
    std::vector<std::string> ips;
//        static_cast<DNSQuery*>(base)->get_ip_by_host("8.8.8.8", "yutianzuo.com", ips);
    static_cast<DNSQuery *>(base)->get_ip_by_host(str_dns_server, str_host, ips);
    if (ips.empty()) {
        std::cerr << base->get_last_err_msg() << std::endl;
        ret = base->get_last_err_msg();
    } else {
        for (const auto &ip : ips) {
            ret += ip;
            ret += "#";
        }
    }
    delete base;

    if (str_dns_server) {
        env->ReleaseStringUTFChars(strDnsServer, str_dns_server);
    }
    if (str_host) {
        env->ReleaseStringUTFChars(strHost, str_host);
    }

    return env->NewStringUTF(ret.data());
}


jstring Java_com_github_yutianzuo_nativesock_JniDef_sendFile(JNIEnv *env, jclass jobj,
                                                             jstring file, jstring ip) {


    const char *str_file = nullptr;
    const char *str_ip = nullptr;

    std::string str_ret = "none err";

    FUNCTION_BEGIN ;


        if (file) {
            str_file = env->GetStringUTFChars(file, 0);
        }
        if (ip) {
            str_ip = env->GetStringUTFChars(ip, 0);
        }

        stringxa str_filex(str_file);
        stringxa str_ipx(str_ip);
        SendCtrl sendctrl;

        if (str_filex.empty() || str_ipx.empty()) {
            str_ret = "wrong params";
            FUNCTION_LEAVE;
        }
        if (!sendctrl.init(str_file)) {
            str_ret = "file wrong, file exsist?";
            FUNCTION_LEAVE;
        }
        if (!sendctrl.send_file(str_ip, TransRecvCtrl::LISTENING_PORT)) {
            str_ret = "send file error";
            FUNCTION_LEAVE;
        }
        str_ret = "send file all done";

        if (file) {
            env->ReleaseStringUTFChars(file, str_file);
        }
        if (ip) {
            env->ReleaseStringUTFChars(ip, str_ip);
        }
    FUNCTION_END;

    return env->NewStringUTF(str_ret.data());
}

jobject g_obj = nullptr;
TransRecvCtrl* g_recv = nullptr;

void Java_com_github_yutianzuo_nativesock_JniDef_recvFile(JNIEnv *env, jclass jobj, jstring dir, jint pieces, jobject
callback) {
    if (callback) {
        if (g_obj) {
            env->DeleteGlobalRef(g_obj);
        }
        g_obj = env->NewGlobalRef(callback);
    }

    const char *str_dir = nullptr;
    if (dir) {
        str_dir = env->GetStringUTFChars(dir, 0);
    }
    /////
    std::shared_ptr<TransRecvCtrl> sp_recv(new (std::nothrow) TransRecvCtrl(str_dir), [](TransRecvCtrl* recv)->void {
        g_recv = nullptr;
        delete recv;
    });
    g_recv = sp_recv.get();
    sp_recv->set_callback([](const char* out)->void {
        JNIEnvPtr jnienv_holder;
        if (g_obj) {
            jclass clazz = jnienv_holder->GetObjectClass(g_obj);
            jmethodID method = jnienv_holder->GetMethodID(clazz, "callBack", "(Ljava/lang/String;)V");
            jstring jout = nullptr;
            try {
                jout = jnienv_holder->NewStringUTF(out);
            } catch (...) {

            }
            jnienv_holder->CallVoidMethod(g_obj, method, jout);
        }
    });
    sp_recv->init(pieces);
    sp_recv->start_poll();


    if (str_dir) {
        env->ReleaseStringUTFChars(dir, str_dir);
    }
}

void Java_com_github_yutianzuo_nativesock_JniDef_recvDone(JNIEnv *env, jclass jobj) {
    if (g_obj) {
        env->DeleteGlobalRef(g_obj);
        g_obj = nullptr;
    }
}


void Java_com_github_yutianzuo_nativesock_JniDef_quitListeningOrRecvingFile(JNIEnv *env, jclass jobj) {
    if (g_recv) {
        g_recv->quit_listening();
        g_recv = nullptr;
    }

}
