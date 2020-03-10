//
// Created by yutianzuo on 2019-08-09.
//

#include <complex>
#include "jnidef.h"

#include "./netutils/dns.h"


#include "tcptransfile/server.h"
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
    std::vector<int> ips;
    std::vector<std::string> str_ips;
//        static_cast<DNSQuery*>(base)->get_ip_by_host("8.8.8.8", "yutianzuo.com", ips);
    static_cast<DNSQuery *>(base)->get_ip_by_host(str_dns_server, str_host, ips);
    if (ips.empty()) {
        std::cerr << base->get_last_err_msg() << std::endl;
        ret = base->get_last_err_msg();
    } else {
        for (const auto &ip : ips) {
            str_ips.emplace_back(NetHelper::safe_ipv4_to_string_addr(ip));
        }
        for (const auto &ip : str_ips) {
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

jobject g_obj = nullptr;
jobject g_obj_send = nullptr;
FileRecvServer* g_recv = nullptr;

/**
 * get ip string, when input is ip, return it; when input is host, get ip through dns server.
 * @param ip_or_host
 * @return
 */
static std::string get_ip_str_from_str(const std::string& ipv4_or_host)
{
    in_addr buff = {0};
    if (::inet_aton(ipv4_or_host.data(), &buff) != 0) //is a ipv4 string
    {
        return ipv4_or_host;
    }
    DNSQuery dns;
    std::vector<int> vec_ips;
    dns.get_ip_by_api(ipv4_or_host, vec_ips);
    if (!vec_ips.empty())
    {
        return NetHelper::safe_ipv4_to_string_addr(vec_ips[0]);
    }
    else
    {
        return "";
    }
}


jstring Java_com_github_yutianzuo_nativesock_JniDef_sendFile(JNIEnv *env, jclass jobj,
                                                             jstring file, jstring ip, jobject
                                                             callbackObj) {
    if (callbackObj) {
        if (g_obj_send) {
            env->DeleteGlobalRef(g_obj_send);
        }
        g_obj_send = env->NewGlobalRef(callbackObj);
    }

    LogUtils::init("/storage/emulated/0/recvfile/log.txt");

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

        if (str_filex.empty()) {
            str_ret = "wrong params";
            FUNCTION_LEAVE;
        }

        if (str_ipx.empty())
        {
            DNSQuery dns;
            std::vector<int> vec_ips;
            dns.get_ip_by_api("cn.yutianzuo.com", vec_ips);
            if (!vec_ips.empty())
            {
                str_ipx = NetHelper::safe_ipv4_to_string_addr(vec_ips[0]);
            }
        }
        else
        {
            str_ipx = get_ip_str_from_str(str_ipx);
        }

        if (!sendctrl.init(str_file)) {
            str_ret = "file wrong, file exsist?";
            FUNCTION_LEAVE;
        }
        auto func = [](std::uint64_t send, std::uint64_t toll) -> void
        {
            if (toll != 0)
            {
                double persent = (double) (send) / toll;
                std::ostringstream output_persent;
                output_persent.precision(2);
                output_persent << std::fixed << "send progress:" << persent * 100 << "%";


                JNIEnvPtr jnienv_holder;
                if (g_obj_send) {
                    jclass clazz = jnienv_holder->GetObjectClass(g_obj_send);
                    jmethodID method = jnienv_holder->GetMethodID(clazz, "callBack",
                                                                  "(Ljava/lang/String;)V");
                    jstring jout = nullptr;
                    try {
                        jout = jnienv_holder->NewStringUTF(output_persent.str().data());
                    } catch (...) {

                    }
                    jnienv_holder->CallVoidMethod(g_obj_send, method, jout);
                }
            }
        };

        sendctrl.set_callback(std::move(func));
        if (!sendctrl.send_file(str_ipx, TransRecvCtrl::LISTENING_PORT)) {
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


void Java_com_github_yutianzuo_nativesock_JniDef_recvFile(JNIEnv *env, jclass jobj, jstring dir, jint pieces, jobject
callback) {
    if (callback) {
        if (g_obj) {
            env->DeleteGlobalRef(g_obj);
        }
        g_obj = env->NewGlobalRef(callback);
    }
    LogUtils::init("/storage/emulated/0/recvfile/log.txt");
    const char *str_dir = nullptr;
    if (dir) {
        str_dir = env->GetStringUTFChars(dir, 0);
    }
    /////
    std::shared_ptr<FileRecvServer> sp_recv(new (std::nothrow) FileRecvServer(),
            [](FileRecvServer* recv)->void {
        g_recv = nullptr;
        delete recv;
    });
    g_recv = sp_recv.get();

    sp_recv->set_callback([](const char* result)->void {
        JNIEnvPtr jnienv_holder;
        if (g_obj) {
            jclass clazz = jnienv_holder->GetObjectClass(g_obj);
            jmethodID method = jnienv_holder->GetMethodID(clazz, "callBack", "(Ljava/lang/String;)V");
            jstring jout = nullptr;
            try {
                jout = jnienv_holder->NewStringUTF(result);
            } catch (...) {

            }
            jnienv_holder->CallVoidMethod(g_obj, method, jout);
        }
    });
    sp_recv->start_server(pieces, str_dir);


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
        g_recv->quit();
        g_recv = nullptr;
    }
}

void Java_com_github_yutianzuo_nativesock_JniDef_sendFileDone(JNIEnv *env, jclass clazz) {
    if (g_obj_send) {
        env->DeleteGlobalRef(g_obj_send);
        g_obj_send = nullptr;
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

void int_for_java(std::vector<int>& ips) {
    for (auto& ip : ips) {
        ip = ((ip << 24) |
                ((ip << 8) & 0xff0000) |
                ((ip >> 24) & 0xff) |
                ((ip >> 8) & 0xff00)
        );
    }
}


extern "C"
JNIEXPORT jintArray JNICALL
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

    jintArray array = nullptr;
    for (auto& ip : vec_dns_servers) {
        std::string ret;
        DNSQuery query;
        std::vector<int> hostips;
        query.get_ip_by_host(ip, str_host, hostips, retry_times);

        if (!hostips.empty()) {
            int_for_java(hostips);
            array = env->NewIntArray(hostips.size());
            env->SetIntArrayRegion(array, 0, hostips.size(), &hostips[0]);
            break;
        }
    }
    env->ReleaseStringUTFChars(host_to_request, str_host);

    return array;
}


extern "C"
JNIEXPORT jintArray JNICALL
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
    std::vector<int> vec_ips;
    query.get_ip_by_api(str_host, vec_ips);
    env->ReleaseStringUTFChars(host_to_request, str_host);
    if (!vec_ips.empty()) {
        int_for_java(vec_ips);
        jintArray array = env->NewIntArray(vec_ips.size());
        env->SetIntArrayRegion(array, 0, vec_ips.size(), &vec_ips[0]);
        return array;
    }
    return nullptr;
}