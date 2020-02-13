package com.github.yutianzuo.myapplication;

import com.google.common.net.InetAddresses;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
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

import java.io.IOException;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;

import okhttp3.Call;
import okhttp3.Callback;
import okhttp3.HttpUrl;
import okhttp3.Interceptor;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.Response;

@SuppressLint("Registered")
public class DnsReleaseActivity extends AppCompatActivity {
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

        final EditText etHost = findViewById(R.id.host_input);

        ///custom parameters---optional
//        Dns.getInstance().addServerIP("1.1.1.1");
//        Dns.getInstance().setRetryTimesForPublicDNSServer(20);
//        Dns.getInstance().setCacheTimeInSeconds(60);
        ///custom parameters---optional

        btn.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        String host = etHost.getText().toString();

                        if (TextUtils.isEmpty(host)) {
                            host = "baidu.com";
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
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        String host = etHost.getText().toString();

                        if (TextUtils.isEmpty(host)) {
                            host = "baidu.com";
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
            }
        });

        btn_oktest.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                httpRequest();
            }
        });
    }

    void refresh(int[] ips) {
        mData.clear();
        for (int ip : ips) {
            InetAddress addr = InetAddresses.fromInteger(ip);
            mData.add(addr.getHostAddress());
        }
        mAdpter.notifyDataSetChanged();
    }


    void httpRequest() {
        OkHttpClient client = OkhttpClientUtils.getClient();
        //创建一个Request
        Request request = new Request.Builder()
                .get()
                .url("https://example.com")
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

    public static OkHttpClient getClient() {
        OkHttpClient client = null;
        OkHttpClient.Builder builder = new OkHttpClient.Builder();
        builder.connectTimeout(TIME_OUT, TimeUnit.SECONDS);
        builder.readTimeout(TIME_OUT, TimeUnit.SECONDS);
        builder.writeTimeout(TIME_OUT, TimeUnit.SECONDS);
        builder.retryOnConnectionFailure(false);
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
