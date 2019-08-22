package com.github.yutianzuo.nativesock;

public class JniDef {
    static {
        System.loadLibrary("native_sock");
    }

    public static native String dnsTest(String dnsServer, String hostToRequests);

    public static native String sendFile(String file, String ip);

    public static native void recvFile(String dir, int pieces, Object callback);

    public static native void recvDone();

    public static native void quitListeningOrRecvingFile();
}
