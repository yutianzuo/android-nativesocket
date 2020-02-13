package com.github.yutianzuo.nativesock;

import android.util.Log;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class Dns {

    static class HostIPs {
        public int[] ips;
        public long cacheTime;
    }

    private static Dns dns;

    private List<String> dnsServerIPs;
    private int retryTimes = 30;
    private Map<String, HostIPs> hostIPCache;
    private int cacheTime = 2 * 60 * 60; //secs, default is 2 hours

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
        hostIPCache = new HashMap<>();
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

    public void setCacheTimeInSeconds(int secs) {
        if (secs >= 0) {
            cacheTime = secs;
        }
    }

    private int[] getHostIPFromCache(String host) {
        try {
            if (hostIPCache.containsKey(host)) {
                HostIPs value = hostIPCache.get(host);
                if (System.currentTimeMillis() - value.cacheTime <= cacheTime * 1000) {
                    return value.ips;
                } else {
                    hostIPCache.remove(host);
                }
            }
        } catch (Throwable e) {
            hostIPCache.clear();
        }
        return null;
    }

    private void saveToCache(String host, int[] ips) {
        if (ips != null && ips.length > 0) {
            HostIPs value = new HostIPs();
            value.cacheTime = System.currentTimeMillis();
            value.ips = ips;
            hostIPCache.put(host, value);
        }
    }

    synchronized public int[] getHostIPFirstByAPI(String host) {
        int[] ips = getHostIPFromCache(host);
        if (ips != null && ips.length > 0) {
//            Log.e("DNS", "in cache");
            return ips;
        }
//        Log.e("DNS", "not in cache");
        ips = JniDef.dnsByAPI(host);
        if (ips == null || ips.length == 0) {
            ips = JniDef.dnsBySpecifiedServers(dnsServerIPs, host, retryTimes);
        }
        saveToCache(host, ips);
        return ips;
    }

    synchronized public int[] getHostIPFirstByPublicDNSServer(String host) {
        int[] ips = getHostIPFromCache(host);
        if (ips != null && ips.length > 0) {
//            Log.e("DNS", "in cache");
            return ips;
        }
//        Log.e("DNS", "not in cache");
        ips = JniDef.dnsBySpecifiedServers(dnsServerIPs, host, retryTimes);
        if (ips == null || ips.length == 0) {
            ips = JniDef.dnsByAPI(host);
        }
        saveToCache(host, ips);
        return ips;
    }
}
