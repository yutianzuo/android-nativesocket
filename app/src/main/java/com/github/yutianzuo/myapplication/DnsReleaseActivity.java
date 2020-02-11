package com.github.yutianzuo.myapplication;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.text.TextUtils;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;

import com.github.yutianzuo.nativesock.Dns;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

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

        final EditText etHost = findViewById(R.id.host_input);

        ///custom parameters
//        Dns.getInstance().addServerIP("1.1.1.1");
//        Dns.getInstance().setRetryTimesForPublicDNSServer(20);

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


                        final String msg = Dns.getInstance().getHostIPFirstByAPI(host);
                        DnsReleaseActivity.this.runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                refresh(msg);
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


                        final String msg = Dns.getInstance().getHostIPFirstByPublicDNSServer(host);
                        DnsReleaseActivity.this.runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                refresh(msg);
                            }
                        });
                    }
                }).start();
            }
        });
    }

    void refresh(String ips) {
        mData.clear();
        String[] ipsArray = ips.split("#");
        mData.addAll(Arrays.asList(ipsArray));
        mAdpter.notifyDataSetChanged();
    }
}
