package com.github.yutianzuo.nativesock;

import android.text.TextUtils;

import java.util.ArrayList;
import java.util.List;

public class Dns {
    private static Dns dns;

    private List<String> dnsServerIPs;
    private int retryTimes = 30;
    private Dns() {
        initDns();
    }

    private void initDns() {
        dnsServerIPs = new ArrayList<>();
        dnsServerIPs.add("119.29.29.29"); //tencent
        dnsServerIPs.add("223.5.5.5"); //ali
        dnsServerIPs.add("180.76.76.76"); //baidu
        dnsServerIPs.add("114.114.114.114"); //114
        dnsServerIPs.add("8.8.8.8"); //google
    }

    public static Dns getInstance() {
        if (dns == null) {
            synchronized (Dns.class) {
                if (dns == null) {
                    dns = new Dns();
                }
            }
        }
        return dns;
    }

    public void addServerIP(String ip) {
        if (!dnsServerIPs.contains(ip)) {
            dnsServerIPs.add(0, ip);
        }
    }

    public void addServerIPs(List<String> ips) {
        if (ips == null) {
            return;
        }
        for (String ip : ips) {
            addServerIP(ip);
        }
    }

    public void setRetryTimesForPublicDNSServer(int times) {
        if (times > 0 && times < 100) {
            retryTimes = times;
        }
    }

    public int[] getHostIPFirstByAPI(String host) {
        int[] ips = JniDef.dnsByAPI(host);
        if (ips == null || ips.length == 0) {
            ips = JniDef.dnsBySpecifiedServers(dnsServerIPs, host, retryTimes);
        }
        return ips;
    }

    public int[] getHostIPFirstByPublicDNSServer(String host) {
        int[] ips = JniDef.dnsBySpecifiedServers(dnsServerIPs, host, retryTimes);
        if (ips == null || ips.length == 0) {
            ips = JniDef.dnsByAPI(host);
        }
        return ips;
    }
}
