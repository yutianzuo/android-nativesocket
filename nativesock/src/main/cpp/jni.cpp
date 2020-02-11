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

inline
void get_vector(std::vector<std::string>& ips, JNIEnv *env, jobject list_ips) {
    jclass cls_list = env->GetObjectClass(list_ips);
    if (cls_list) {
        jmethodID arraylist_get = env->GetMethodID(cls_list, "get",
                                                       "(I)Ljava/lang/Object;");
        jmethodID arraylist_size = env->GetMethodID(cls_list, "size", "()I");
        if (arraylist_get && arraylist_size) {
            jint size = env->CallIntMethod(list_ips, arraylist_size);
            for (int i = 0; i < size; ++i) {
                jstring jstr_ip = (jstring) env->CallObjectMethod(list_ips,
                                                                  arraylist_get, i);
                if (jstr_ip) {
                    const char *sz_ip = (char *) env->GetStringUTFChars(jstr_ip,
                                                                        nullptr);
                    if (sz_ip) {
                        ips.emplace_back(sz_ip);
                        env->ReleaseStringUTFChars(jstr_ip, sz_ip);
                    }
                }
            }
        }
    }
}


extern "C"
JNIEXPORT jstring JNICALL
Java_com_github_yutianzuo_nativesock_JniDef_dnsBySpecifiedServers(JNIEnv *env, jclass clazz,
                                                                  jobject ips,
                                                                  jstring host_to_request,
                                                                  jint retry_times) {
    if (!ips || !host_to_request || retry_times <= 0) {
        return nullptr;
    }
    const char *str_host = nullptr;
    str_host = env->GetStringUTFChars(host_to_request, 0);
    if (!str_host) {
        return nullptr;
    }
    std::vector<std::string> vec_dns_servers;
    get_vector(vec_dns_servers, env, ips);

    jstring jstr_ret = nullptr;
    for (auto& ip : vec_dns_servers) {
        std::string ret;
        DNSQuery query;
        std::vector<std::string> hostips;
        query.get_ip_by_host(ip, str_host, hostips, retry_times);

        if (!hostips.empty()) {
            jstr_ret = env->NewStringUTF(hostips.at(0).data());
            break;
        }
    }
    env->ReleaseStringUTFChars(host_to_request, str_host);
    return jstr_ret;
}


extern "C"
JNIEXPORT jstring JNICALL
Java_com_github_yutianzuo_nativesock_JniDef_dnsByAPI(JNIEnv *env, jclass clazz,
                                                     jstring host_to_request) {
    if (!host_to_request) {
        return nullptr;
    }

    const char *str_host = nullptr;
    str_host = env->GetStringUTFChars(host_to_request, 0);
    if (!str_host) {
        return nullptr;
    }

    DNSQuery query;
    std::vector<std::string> vec_ips;
    query.get_ip_by_api(str_host, vec_ips);
    env->ReleaseStringUTFChars(host_to_request, str_host);
    if (!vec_ips.empty()) {
        return env->NewStringUTF(vec_ips.at(0).data());
    }
    return nullptr;
}