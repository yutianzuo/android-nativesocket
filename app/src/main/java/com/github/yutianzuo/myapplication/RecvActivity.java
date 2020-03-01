package com.github.yutianzuo.myapplication;

import android.content.Context;
import android.os.Bundle;
import android.os.Environment;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.Spinner;
import android.widget.TextView;

import com.github.yutianzuo.nativesock.JniDef;

import java.io.File;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.UnknownHostException;
import java.util.Enumeration;

public class RecvActivity extends AppCompatActivity {
    TextView mTvInfo;
    Button mBtnRecv;
    Spinner mSpinner;
    int index = 0;
    Integer[] ints;
    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_recv);

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        mSpinner = findViewById(R.id.spinner);

        mTvInfo = findViewById(R.id.tv_info);
        mBtnRecv = findViewById(R.id.btn_recv);

        mBtnRecv.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                mBtnRecv.setEnabled(false);
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        int pieces = ints[index];
//                        Log.e("java-debug", "pieces is " + pieces);
                        JniDef.recvFile(getAppDir(RecvActivity.this), pieces, RecvActivity.this);

                        JniDef.recvDone();

                        RecvActivity.this.runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                mBtnRecv.setEnabled(true);
                            }
                        });
                    }
                }).start();

            }
        });

        new Thread(new Runnable() {
            @Override
            public void run() {
                InetAddress addr = null;
                try {
                    addr = getLocalHostLANAddress();
                    final String hostip = addr.getHostAddress();
                    RecvActivity.this.runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            mTvInfo.setText(hostip);
                        }
                    });
                } catch (UnknownHostException e) {
                    e.printStackTrace();
                }
            }
        }).start();
        int[] strings = this.getResources().getIntArray(R.array.pieces);
        ints = new Integer[strings.length];

        for (int i = 0; i < strings.length; ++i) {
            ints[i] = strings[i];
        }

        mSpinner.setAdapter(new ArrayAdapter<>(this, android.R.layout.simple_spinner_item, ints));

        mSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                index = position;
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });
    }

    @Override
    public void onBackPressed() {
        if (!mBtnRecv.isEnabled()) {
            BaseDialog.newBuilder(this).setText(BaseDialog.MESSAGE_VIEW, "正在接收文件，是否退出？").
                    setText(BaseDialog.LEFT_VIEW, "是", new OnClickListener() {
                        @Override
                        public void onClick(View v) {
                            JniDef.quitListeningOrRecvingFile();
                        }
                    }).
                    setText(BaseDialog.RIGHT_VIEW, "否", new OnClickListener() {
                        @Override
                        public void onClick(View v) {
                        }
                    }).widthPercent(0.7f).
                    show();
        } else {
            super.onBackPressed();
        }
    }

    void callBack(final String info) {
        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mTvInfo.setText(info + "----received ok!");
            }
        });
    }
    /////////////////////////////////////////////////////
    private static InetAddress getLocalHostLANAddress() throws UnknownHostException {

        try {
            InetAddress candidateAddress = null;
            InetAddress p2p = null;
            // 遍历所有的网络接口
            for (Enumeration ifaces = NetworkInterface.getNetworkInterfaces(); ifaces.hasMoreElements();) {
                NetworkInterface iface = (NetworkInterface) ifaces.nextElement();
                // 在所有的接口下再遍历IP
                for (Enumeration inetAddrs = iface.getInetAddresses(); inetAddrs.hasMoreElements();) {
                    InetAddress inetAddr = (InetAddress) inetAddrs.nextElement();
                    if (!inetAddr.isLoopbackAddress()) {// 排除loopback类型地址
                        if (inetAddr.isSiteLocalAddress() && inetAddr instanceof Inet4Address) {
                            if (iface.getName().contains("p2p") && iface.isUp()) {
                                return inetAddr;
                            }
                            // 如果是site-local地址，就是它了
                            if (p2p == null) {
                                candidateAddress = inetAddr;
                                continue;
                            }
                            return inetAddr;
                        } else if (candidateAddress == null) {
                            // site-local类型的地址未被发现，先记录候选地址
                            candidateAddress = inetAddr;
                        }
                    }
                }
            }
            if (candidateAddress != null) {
                return candidateAddress;
            }
            // 如果没有发现 non-loopback地址.只能用最次选的方案
            InetAddress jdkSuppliedAddress = InetAddress.getLocalHost();
            if (jdkSuppliedAddress == null) {
                throw new UnknownHostException("The JDK InetAddress.getLocalHost() method unexpectedly returned null.");
            }
            return jdkSuppliedAddress;
        } catch (Exception e) {
            UnknownHostException unknownHostException = new UnknownHostException(
                    "Failed to determine LAN address: " + e);
            unknownHostException.initCause(e);
            throw unknownHostException;
        }
    }

    //出自这篇：http://www.cnblogs.com/zrui-xyu/p/5039551.html
//实际上的代码是不准的
    private static InetAddress getLocalHostAddress() throws UnknownHostException {
        Enumeration allNetInterfaces;
        try {
            allNetInterfaces = NetworkInterface.getNetworkInterfaces();
            InetAddress ip = null;
            while (allNetInterfaces.hasMoreElements()) {
                NetworkInterface netInterface = (NetworkInterface) allNetInterfaces.nextElement();

                Enumeration addresses = netInterface.getInetAddresses();
                while (addresses.hasMoreElements()) {
                    ip = (InetAddress) addresses.nextElement();
                    if (!ip.isSiteLocalAddress() && !ip.isLoopbackAddress() && ip.getHostAddress().indexOf(":") == -1) {
                        if (ip != null && ip instanceof Inet4Address) {
                            return ip;
                        }
                    }
                }
            }
        } catch (Throwable e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }

        InetAddress jdkSuppliedAddress = InetAddress.getLocalHost();
        if (jdkSuppliedAddress == null) {
            throw new UnknownHostException("The JDK InetAddress.getLocalHost() method unexpectedly returned null.");
        }
        return jdkSuppliedAddress;
    }
/////////////////////////////////////////////////////////////////////////////////////////
    public static String getAppDir(Context context) {
        String dir = "";
        if (checkSDCard()) {
            dir = Environment.getExternalStorageDirectory().getAbsolutePath();
        } else {
            dir = context.getDir("recvfile", Context.MODE_PRIVATE)
                    .getAbsolutePath();
        }
        dir = dir + File.separator + "recvfile" + File.separator;
        makeDir(dir);

        String str_Hide_FilePath = dir + ".nomedia";
        File fHide = new File(str_Hide_FilePath);
        if (!fHide.isFile()) {
            try {
                fHide.createNewFile();
            } catch (Exception e) {
            }

        }
        return dir;
    }

    public static boolean checkSDCard() {
        String state = Environment.getExternalStorageState();

        if (Environment.MEDIA_MOUNTED.equals(state)) {
            return true;
        }
        return false;
    }

    public static void makeDir(String dir) {
        File f = new File(dir);
        if (!f.exists()) {
            f.mkdirs();
        }
    }
}
