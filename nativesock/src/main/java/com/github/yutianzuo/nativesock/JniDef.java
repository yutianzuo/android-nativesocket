package com.github.yutianzuo.nativesock;

import java.util.List;

public class JniDef {
    static {
        System.loadLibrary("native_sock");
    }

    public static native String dnsTest(String dnsServer, String hostToRequests);

    /// 用公共dnsservers请求hostip
    public static native int[] dnsBySpecifiedServers(List<String> ips, String hostToRequest,
            int retryTimes);

    /// 用系统API请求hostip
    public static native int[] dnsByAPI(String hostToRequest);

    public static native String sendFile(String file, String ip);

    public static native void recvFile(String dir, int pieces, Object callback);

    public static native void recvDone();

    public static native void quitListeningOrRecvingFile();
}
