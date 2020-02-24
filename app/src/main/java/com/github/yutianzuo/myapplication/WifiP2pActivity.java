package com.github.yutianzuo.myapplication;

import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.wifi.p2p.WifiP2pConfig;
import android.net.wifi.p2p.WifiP2pDevice;
import android.net.wifi.p2p.WifiP2pInfo;
import android.net.wifi.p2p.WifiP2pManager;
import android.net.wifi.p2p.WifiP2pManager.ActionListener;
import android.os.Bundle;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.Collection;

public class WifiP2pActivity extends AppCompatActivity implements WifiP2pActionListener {
    private final static String TAG = "wifi-p2p";
    public final static String GROUP = "group-character";

    public WifiP2pManager mWifiP2pManager;
    public WifiP2pManager.Channel mChannel;
    private final IntentFilter intentFilter = new IntentFilter();
    private WifiP2pReceiver recv;
    private BaseDialog mLoadingDialog;

    private Button mButtonSearch;
    private ListView mListView;

    private ArrayList<String> mListDeviceName = new ArrayList();
    private ArrayList<WifiP2pDevice> mListDevices = new ArrayList();
    ArrayAdapter<String> adapter;
    boolean bCharacter;
    boolean bConnected = false;
    String hostIp;


    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        bCharacter = getIntent().getBooleanExtra(GROUP, false);

        adapter = new ArrayAdapter(this, android.R.layout.simple_list_item_1, mListDeviceName);
        setContentView(R.layout.p2p_activity);
        mListView = findViewById(R.id.lv_device);
        mListView.setAdapter(adapter);
        mListView.setOnItemClickListener(new OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                WifiP2pDevice device = mListDevices.get(position);
                connect(device);
            }
        });



        mButtonSearch = findViewById(R.id.btn_search);
        mButtonSearch.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (bCharacter) {
                    showSendRecvAfterConnectOK();
                } else {
                    searchDevices();
                    showWaitingDlg();
                }
            }
        });

        mWifiP2pManager = (WifiP2pManager) getSystemService(Context.WIFI_P2P_SERVICE);
        mChannel = mWifiP2pManager.initialize(this, getMainLooper(), this);

        // Indicates a change in the Wi-Fi P2P status.
        intentFilter.addAction(WifiP2pManager.WIFI_P2P_STATE_CHANGED_ACTION);

        // Indicates a change in the list of available peers.
        intentFilter.addAction(WifiP2pManager.WIFI_P2P_PEERS_CHANGED_ACTION);

        // Indicates the state of Wi-Fi P2P connectivity has changed.
        intentFilter.addAction(WifiP2pManager.WIFI_P2P_CONNECTION_CHANGED_ACTION);

        // Indicates this device's details have changed.
        intentFilter.addAction(WifiP2pManager.WIFI_P2P_THIS_DEVICE_CHANGED_ACTION);

        recv = new WifiP2pReceiver(mWifiP2pManager, mChannel, this);

        if (bCharacter) { //a group master
            mButtonSearch.setText("等待设备连入...");
            mButtonSearch.setEnabled(false);
            createGroup();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        registerReceiver(recv, intentFilter);
    }

    @Override
    protected void onPause() {
        super.onPause();
        unregisterReceiver(recv);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        quitGroup();
    }

    private void showWaitingDlg() {
        if (mLoadingDialog == null) {
            mLoadingDialog = BaseDialog.newBuilder(this).
                    layout(R.layout.loading_layout).
                    build();
        }
        mLoadingDialog.show();
    }

    private void searchDevices() {
        mWifiP2pManager.discoverPeers(mChannel, new WifiP2pManager.ActionListener() {
            @Override
            public void onSuccess() {
                // WifiP2pManager.WIFI_P2P_PEERS_CHANGED_ACTION 广播，此时就可以调用 requestPeers 方法获取设备列表信息
                Log.e("wifi-p2p", "搜索设备成功");
            }

            @Override
            public void onFailure(int reasonCode) {
                Log.e("wifi-p2p", "搜索设备失败");
            }
        });
    }

    private void stopSearchingDevices() {
        mWifiP2pManager.stopPeerDiscovery(mChannel, new ActionListener() {
            @Override
            public void onSuccess() {
                Toast.makeText(WifiP2pActivity.this, "停止搜索设备ok", Toast.LENGTH_SHORT).show();
            }

            @Override
            public void onFailure(int reason) {
                Toast.makeText(WifiP2pActivity.this, "停止搜索设备failed", Toast.LENGTH_SHORT).show();
            }
        });
    }

    private void connect(final WifiP2pDevice device) {
        WifiP2pConfig config = new WifiP2pConfig();
        if (device != null) {
            config.deviceAddress = device.deviceAddress;
            mWifiP2pManager.connect(mChannel, config, new WifiP2pManager.ActionListener() {
                @Override
                public void onSuccess() {
                    Toast.makeText(WifiP2pActivity.this, "connect调用成功", Toast.LENGTH_SHORT).show();
                    bConnected = true;
                    stopSearchingDevices();
                    if (bConnected && !TextUtils.isEmpty(hostIp)) {
                        showSendRecvAfterConnectOK();
                    }
                }

                @Override
                public void onFailure(int reason) {
                    Toast.makeText(WifiP2pActivity.this, "connect调用失败" + reason, Toast.LENGTH_SHORT).show();
                }
            });
        }
    }

    private void jumpToSendFile() {
        if (bConnected && !TextUtils.isEmpty(hostIp)) {
            Intent intent = new Intent(WifiP2pActivity.this, SendFileActivity.class);
            intent.putExtra(SendFileActivity.IP_ADDR, hostIp);
            this.startActivity(intent);
            bConnected = false;
        }
    }

    private void createGroup() {
        mWifiP2pManager.createGroup(mChannel, new WifiP2pManager.ActionListener() {
            @Override
            public void onSuccess() {
                Log.e("wifi-p2p", "创建群组成功");
                Toast.makeText(WifiP2pActivity.this, "创建群组成功", Toast.LENGTH_SHORT).show();
            }

            @Override
            public void onFailure(int reason) {
                Log.e("wifi-p2p", "创建群组失败: " + reason);
                Toast.makeText(WifiP2pActivity.this, "创建群组失败,请移除已有的组群或者连接同一WIFI重试", Toast.LENGTH_SHORT).show();
            }
        });
    }

    private void quitGroup() {
        mWifiP2pManager.removeGroup(mChannel, new WifiP2pManager.ActionListener() {
            @Override
            public void onSuccess() {
                Log.e("wifi-p2p", "移除组群成功");
                Toast.makeText(WifiP2pActivity.this, "移除组群成功", Toast.LENGTH_SHORT).show();
            }

            @Override
            public void onFailure(int reason) {
                Log.e("wifi-p2p", "移除组群失败");
                Toast.makeText(WifiP2pActivity.this, "移除组群失败,请创建组群重试", Toast.LENGTH_SHORT).show();
            }
        });
    }

    private void showSendRecvAfterConnectOK() {
        BaseDialog.newBuilder(this).setText(BaseDialog.MESSAGE_VIEW, "组间p2p网络成功后选择收发文件").
                setText(BaseDialog.LEFT_VIEW, "接收文件", new OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        recvFile();
                    }
                }).
                setText(BaseDialog.RIGHT_VIEW, "发送文件", new OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        if (bCharacter) {
                            sendFile();
                        } else {
                            jumpToSendFile();
                        }
                    }
                }).
                widthPercent(0.9f).
                show();
    }

    private void sendFile() {
        Intent intent = new Intent(this, SendFileActivity.class);
        this.startActivity(intent);
    }

    private void recvFile() {
        Intent intent = new Intent(this, RecvActivity.class);
        this.startActivity(intent);
    }

/////////////////////////////////////////////////////////////////////////////////////////////////

    @Override
    public void wifiP2pEnabled(boolean enabled) {
        Log.e(TAG, "wifi is " + (enabled ? "on" : "off"));
    }

    @Override
    public void onConnection(WifiP2pInfo wifiP2pInfo) {
        Log.e(TAG, "请求连接已成功：" + wifiP2pInfo.toString());
        hostIp = wifiP2pInfo.groupOwnerAddress.getHostAddress();
        if (bConnected && !TextUtils.isEmpty(hostIp)) {
            showSendRecvAfterConnectOK();
        }
    }

    @Override
    public void onDisconnection() {
        Log.e(TAG, "onDisconnection");
    }

    @Override
    public void onDeviceInfo(WifiP2pDevice wifiP2pDevice) {
        Log.e(TAG, "设备" + wifiP2pDevice.deviceName + " 信息发生改变：" + wifiP2pDevice.toString());
        if (wifiP2pDevice.status == WifiP2pDevice.CONNECTED) {
            mButtonSearch.setEnabled(true);
            mButtonSearch.setText("准备接收文件");
        }
    }

    @Override
    public void onPeersInfo(Collection<WifiP2pDevice> wifiP2pDeviceList) {
        Log.e(TAG, "周围设备列表发生变化");
        Log.e(TAG, "devices:");
        if (bCharacter) {
            return;
        }

        for (WifiP2pDevice device : wifiP2pDeviceList) {
            Log.e(TAG, "--" + device.toString());
            if (!mListDeviceName.contains(device.deviceName)) {
                mListDeviceName.add(device.deviceName);
                mListDevices.add(device);
            }
        }

        if (wifiP2pDeviceList.size() > 0) {
            try {
                mLoadingDialog.dismiss();
            } catch (Throwable e) {

            }
            adapter.notifyDataSetChanged();
        }
    }

    @Override
    public void onChannelDisconnected() {
        Log.e(TAG, "onChannelDisconnected");
    }
}
