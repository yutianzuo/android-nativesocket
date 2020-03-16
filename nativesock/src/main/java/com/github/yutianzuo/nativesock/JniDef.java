package com.github.yutianzuo.nativesock;

import android.os.Build.VERSION;

import java.util.List;

import static android.os.Build.VERSION_CODES.JELLY_BEAN_MR2;

public class JniDef {
    static {
        //bugs in linker, <= api18, we need link stl shared_library manually.
        //a better way is use relinker:https://github.com/KeepSafe/ReLinker
        if (VERSION.SDK_INT <= JELLY_BEAN_MR2) {
            System.loadLibrary("c++_shared");
        }
        System.loadLibrary("native_sock");
    }

    public static native String dnsTest(String dnsServer, String hostToRequests);

    /// 用公共dnsservers请求hostip
    public static native int[] dnsBySpecifiedServers(List<String> ips, String hostToRequest,
            int retryTimes);

    /// 用系统API请求hostip
    public static native int[] dnsByAPI(String hostToRequest);

    public static native String sendFile(String file, String ip, Object callbackObj);

    public static native void sendFileDone();

    public static native void recvFile(String dir, int pieces, Object callback);

    public static native void recvDone();

    public static native void quitListeningOrRecvingFile();
}
