package com.github.yutianzuo.myapplication;

import com.google.common.net.InetAddresses;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.os.Environment;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.github.yutianzuo.nativesock.Dns;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import okhttp3.Call;
import okhttp3.Callback;
import okhttp3.Headers;
import okhttp3.HttpUrl;
import okhttp3.Interceptor;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.Response;

@SuppressLint("Registered")
public class DnsReleaseActivity extends AppCompatActivity {
    private final static String TAG = "DnsReleaseActivity";
    List<String> mData;
    class MyListAdpter extends BaseAdapter {

        MyListAdpter() {
        }

        @Override
        public int getCount() {
            return mData.size();
        }

        @Override
        public Object getItem(int position) {
            return null;
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            final Holder holder;
            if (convertView == null) {
                convertView = DnsReleaseActivity.this.getLayoutInflater().inflate(R.layout.list_item, parent, false);
                holder = new Holder(convertView);
                convertView.setTag(holder);
            } else {
                holder = (Holder) convertView.getTag();
            }
            holder.mTv.setText(mData.get(position));
            return convertView;
        }

        class Holder {
            TextView mTv;

            public Holder(View convertView) {
                mTv = convertView.findViewById(R.id.listtv);
            }
        }
    }

    private ListView mListView;
    private MyListAdpter mAdpter;

    private EditText etHost;
    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_dns_release);

        mListView = findViewById(R.id.list);
        mData = new ArrayList<>();
        mAdpter = new MyListAdpter();
        mListView.setAdapter(mAdpter);

        final Button btn = findViewById(R.id.btn_confirm);
        final Button btn_public = findViewById(R.id.btn_confirm_public);
        final Button btn_oktest = findViewById(R.id.btn_test);
        final Button btn_back = findViewById(R.id.btn_back);

        etHost = findViewById(R.id.host_input);

        ///custom parameters---optional
//        Dns.getInstance().addServerIP("1.1.1.1");
//        Dns.getInstance().setRetryTimesForPublicDNSServer(20);
//        Dns.getInstance().setCacheTimeInSeconds(60);
        ///custom parameters---optional

        btn.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                mData.clear();
                mAdpter.notifyDataSetChanged();
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        String host = etHost.getText().toString();

                        if (TextUtils.isEmpty(host)) {
                            host = "isure6.stream.qqmusic.qq.com";
                        }


                        final int[] ips = Dns.getInstance().getHostIPFirstByAPI(host);
                        DnsReleaseActivity.this.runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                refresh(ips);
                            }
                        });
                    }
                }).start();

            }
        });

        btn_public.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                mData.clear();
                mAdpter.notifyDataSetChanged();
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        String host = etHost.getText().toString();

                        if (TextUtils.isEmpty(host)) {
                            host = "isure6.stream.qqmusic.qq.com";
                        }


                        final int[] ips = Dns.getInstance().getHostIPFirstByPublicDNSServer(host);
                        DnsReleaseActivity.this.runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                refresh(ips);
                            }
                        });
                    }
                }).start();
//                btn_public.setVisibility(View.GONE);
            }
        });

        btn_oktest.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
//                httpRequest();
                httpDownLoad();
            }
        });

        btn_back.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                DnsReleaseActivity.this.finish();
            }
        });
    }

    void refresh(int[] ips) {
        mData.clear();
        if (ips == null) {
            Toast.makeText(this, "没有解析到ip，确认域名输入是否正确", Toast.LENGTH_LONG).show();
            mAdpter.notifyDataSetChanged();
            return;
        }
        for (int ip : ips) {
            InetAddress addr = InetAddresses.fromInteger(ip);
            mData.add(addr.getHostAddress());
        }
        mAdpter.notifyDataSetChanged();
    }

    private String isExistDir(String saveDir) throws IOException {
        // 下载位置
        File downloadFile = new File(Environment.getExternalStorageDirectory(), saveDir);
        downloadFile.mkdirs();
        return downloadFile.getAbsolutePath();
    }

    void httpDownLoad() {
        String url = etHost.getText().toString();
        if (TextUtils.isEmpty(url)) {
            Toast.makeText(this, "url empty!", Toast.LENGTH_LONG).show();
//            url = "https://isure6.stream.qqmusic.qq.com/C200004QxGK535eBMZ.m4a?guid=2000000177&vkey=9874767806F82DE1AF27559782AC0EE3AAEB602198EFBE4F03E1C3312C85C2E80DA824BAB95552B98ABFA17763932CA6C4548A4BDC3C4639&uin=0&fromtag=20177";
            return;
        }
        final String urlRecord = url + "\n";
        Toast.makeText(DnsReleaseActivity.this, "download will start!",
                Toast.LENGTH_LONG).show();
        try {
            OkHttpClient client = OkhttpClientUtils.getClient(false);
            Request request = new Request.Builder()
                    .get()
                    .url(url)
                    .build();
            client.newCall(request).enqueue(new Callback() {
                @Override
                public void onFailure(Call call, IOException e) {
                    Toast.makeText(DnsReleaseActivity.this, "download failed!",
                            Toast.LENGTH_LONG).show();
                }

                @Override
                public void onResponse(Call call, Response response) throws IOException {
                    InputStream is = null;
                    byte[] buf = new byte[2048];
                    int len = 0;
                    FileOutputStream fos = null;
                    FileOutputStream fHeaderOs = null;
                    // 储存下载文件的目录
                    String savePath = isExistDir("wecarflow-qqmusic-debug");
                    try {
                        is = response.body().byteStream();
                        long total = response.body().contentLength();
                        long time = System.currentTimeMillis();
                        File file = new File(savePath,
                                "qqmusic-debug-" + time + ".m4a");
                        File headerFile = new File(savePath, "qqmusic-debug-" + time + ".header");
                        fHeaderOs = new FileOutputStream(headerFile);
                        fHeaderOs.write(urlRecord.getBytes(), 0, urlRecord.getBytes().length);
                        Headers headers = response.headers();
                        for(String headerName : headers.names()) {
                            String out = headerName + ":" + response.header(headerName) + "\n";
                            Log.d(TAG, out);
                            fHeaderOs.write(out.getBytes(), 0, out.getBytes().length);
                        }
                        fHeaderOs.flush();

                        fos = new FileOutputStream(file);
                        long sum = 0;
                        while ((len = is.read(buf)) != -1) {
                            fos.write(buf, 0, len);
                            sum += len;
                            int progress = (int) (sum * 1.0f / total * 100);
                            // 下载中
                            Log.d(TAG, "download progress: " + progress);
                        }
                        fos.flush();
                        // 下载完成
                        DnsReleaseActivity.this.runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(DnsReleaseActivity.this, "download done!",
                                        Toast.LENGTH_LONG).show();
                            }
                        });

                    } catch (Exception e) {
                        DnsReleaseActivity.this.runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(DnsReleaseActivity.this, "download error!",
                                        Toast.LENGTH_LONG).show();
                            }
                        });
                    } finally {
                        try {
                            if (is != null)
                                is.close();
                        } catch (IOException e) {
                        }
                        try {
                            if (fos != null)
                                fos.close();
                            if (fHeaderOs != null)
                                fHeaderOs.close();
                        } catch (IOException e) {
                        }
                    }
                }
            });
        } catch (Throwable e) {
            Log.d(TAG, "download exp: " + e.getMessage());
        }
    }


    void httpRequest() {
        OkHttpClient client = OkhttpClientUtils.getClient(true);
        //创建一个Request
        Request request = new Request.Builder()
                .get()
                .url("https://devgw.tai.qq.com/accountsvc3/")
                .build();
        //通过client发起请求
        client.newCall(request).enqueue(new Callback() {
            @Override
            public void onFailure(Call call, final IOException e) {
                DnsReleaseActivity.this.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Log.e("httpRequest", "onFailure:" + e.toString());
                        Toast.makeText(DnsReleaseActivity.this, "onFailure", Toast.LENGTH_SHORT).show();
                    }
                });
            }
            @Override
            public void onResponse(Call call, final Response response) throws IOException {
                DnsReleaseActivity.this.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(DnsReleaseActivity.this, "onResponse", Toast.LENGTH_SHORT).show();
                        if (response.isSuccessful()) {
                            try {
                                String str = response.body().string();
                                Log.e("httpRequest", "onResponse:" + str);
                            } catch (Throwable e){}
                        } else {
                            Log.e("httpRequest", "onResponse:failed");
                        }
                    }
                });

            }
        });
    }
}

class OkhttpClientUtils {
    public static final int TIME_OUT = 15 * 1000;

    public static OkHttpClient getClient(boolean useDns) {
        OkHttpClient client = null;
        OkHttpClient.Builder builder = new OkHttpClient.Builder();
        builder.connectTimeout(TIME_OUT, TimeUnit.SECONDS);
        builder.readTimeout(TIME_OUT, TimeUnit.SECONDS);
        builder.writeTimeout(TIME_OUT, TimeUnit.SECONDS);
        builder.retryOnConnectionFailure(false);
        if (useDns) {
            builder.dns(new okhttp3.Dns() {
                @Override
                public List<InetAddress> lookup(String hostname) throws UnknownHostException {
                    try {
                        int[] ips = Dns.getInstance().getHostIPFirstByPublicDNSServer(hostname);
                        List<InetAddress> addrs = new ArrayList<>();
                        for (int ip : ips) {
                            InetAddress addr = InetAddresses.fromInteger(ip);
                            addrs.add(addr);
                        }
                        return addrs;
                    } catch (Throwable e) {
                        Log.e("httpRequest", "custom dns exp:" + e.toString());
                        return SYSTEM.lookup(hostname);
                    }
                }
            });
        }
//        builder.dns(okhttp3.Dns.SYSTEM);
//        builder.addInterceptor(new CustomInterceptor());
        client = builder.build();
        return client;
    }

    static public class CustomInterceptor implements Interceptor {
        @Override
        public Response intercept(Chain chain) throws IOException {
            Request request = chain.request();
            HttpUrl httpUrl = request.url().newBuilder()
                    .addQueryParameter("token", "tokenValue")
                    .build();
            request = request.newBuilder()
                    .url(httpUrl)
                    .header("Cookie", "q=" + "qq")
                    .build();
            return chain.proceed(request);
        }
    }

}
