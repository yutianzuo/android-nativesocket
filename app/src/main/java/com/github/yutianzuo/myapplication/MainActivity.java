package com.github.yutianzuo.myapplication;

import android.content.Intent;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {

    Button mButtonDns;
    Button mButtonDnsRelease;
    Button mButtonNetChoose;
    Button mButtonP2p;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mButtonDns = findViewById(R.id.btn_dns);
        mButtonDnsRelease = findViewById(R.id.btn_dns_release);
        mButtonNetChoose = findViewById(R.id.btn_netchoose);
        mButtonP2p = findViewById(R.id.btn_p2p);


        mButtonDns.setOnClickListener(this);
        mButtonNetChoose.setOnClickListener(this);
        mButtonP2p.setOnClickListener(this);
        mButtonDnsRelease.setOnClickListener(this);

    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.btn_dns:
                dns();
                break;
            case R.id.btn_netchoose:
                chooseNet();
                break;
            case R.id.btn_p2p:
                P2P();
                break;
            case R.id.btn_dns_release:
                dnsRelease();
                break;
        }
    }

    private void dnsRelease() {
        Intent intent = new Intent(MainActivity.this, DnsReleaseActivity.class);
        this.startActivity(intent);
    }

    private void P2P() {
        Intent intent = new Intent(MainActivity.this, WifiP2pDynamicOwnerActivity.class);
        this.startActivity(intent);
    }

    private void dns() {
        Intent intent = new Intent(MainActivity.this, DnsActivity.class);
        this.startActivity(intent);
    }

    private void chooseNet() {
        BaseDialog.newBuilder(this).setText(BaseDialog.MESSAGE_VIEW, "选择网络类型").
                setText(BaseDialog.LEFT_VIEW, "Wifi-P2p", new OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        showWifiP2pMasterChooseDlg();
                    }
                }).
                setText(BaseDialog.RIGHT_VIEW, "连接同一个路由器的局域网", new OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        showSendRecvChooseDlg();
                    }
                }).widthPercent(0.9f).
                show();
    }

    private void showSendRecvChooseDlg() {
        BaseDialog.newBuilder(this).setText(BaseDialog.MESSAGE_VIEW, "选择接受或者发送").
                setText(BaseDialog.LEFT_VIEW, "接收文件", new OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        recvFile();
                    }
                }).
                setText(BaseDialog.RIGHT_VIEW, "发送文件", new OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        sendFile();
                    }
                }).
                widthPercent(0.9f).
                show();
    }

    private void showWifiP2pMasterChooseDlg() {
        BaseDialog.newBuilder(this).setText(BaseDialog.MESSAGE_VIEW, "选择作为一个p2p网络的角色").
                setText(BaseDialog.LEFT_VIEW, "作为主节点（只能有一个）", new OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        Intent intent = new Intent(MainActivity.this, WifiP2pActivity.class);
                        intent.putExtra(WifiP2pActivity.GROUP, true);
                        MainActivity.this.startActivity(intent);
                    }
                }).
                setText(BaseDialog.RIGHT_VIEW, "作为从节点", new OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        Intent intent = new Intent(MainActivity.this, WifiP2pActivity.class);
                        intent.putExtra(WifiP2pActivity.GROUP, false);
                        MainActivity.this.startActivity(intent);
                    }
                }).
                widthPercent(0.9f).
                show();
    }

    private void sendFile() {
        Intent intent = new Intent(MainActivity.this, SendFileActivity.class);
        this.startActivity(intent);
    }

    private void recvFile() {
        Intent intent = new Intent(MainActivity.this, RecvActivity.class);
        this.startActivity(intent);
    }
}
