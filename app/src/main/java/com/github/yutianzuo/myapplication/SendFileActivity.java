package com.github.yutianzuo.myapplication;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.FileProvider;

import android.os.Environment;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import com.github.yutianzuo.nativesock.JniDef;

import java.io.File;
import java.io.InputStream;

public class SendFileActivity extends AppCompatActivity implements View.OnClickListener {
    public static final String IP_ADDR = "ip_addr";
    TextView mTvSendFile;
    String mFileSend;
    Button mBtnSend;
    EditText mEtIp;
    static final int REQUEST_CODE = 999;
    String ip;

    private File getWechatDirFile() {
        String dir = Environment.getExternalStorageDirectory().getAbsolutePath();
        dir = dir + File.separator + "tencent" + File.separator + "MicroMsg" + File.separator +
        "Download" + File.separator;
        File fHide = new File(dir);
        return fHide;
    }

    private void browser() {
        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        intent.setType("*/*");
        intent.addCategory(Intent.CATEGORY_OPENABLE);

//        File wechat_dir = getWechatDirFile();
//        FileProviderUtils.setIntentDataAndType(this, intent, "*/*", wechat_dir, false);


        startActivityForResult(intent, REQUEST_CODE);
    }

    private String getShareFile() {
        Intent intent = getIntent();
        Bundle extras = intent.getExtras();
        String action = intent.getAction();
        // 判断Intent是否是“分享”功能(Share Via)
        if (Intent.ACTION_SEND.equals(action))
        {
            if (extras.containsKey(Intent.EXTRA_STREAM))
            {
                try
                {
                    // 获取资源路径Uri
                    Uri uri = (Uri) extras.getParcelable(Intent.EXTRA_STREAM);
                    Log.e("SnedFileActivity", "uri:" + uri.toString());

                    return PathUtils.getImageAbsolutePath(this, uri);
                }
                catch (Exception e)
                {
                    Log.e(this.getClass().getName(), e.toString());
                }
                return "";

            }
            else if (extras.containsKey(Intent.EXTRA_TEXT))
            {
                return "";
            }
        }
        return "";
    }

    String mFilePath;
    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_send);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        mFilePath = getShareFile();
        if (TextUtils.isEmpty(mFilePath)) {
            ip = getIntent().getStringExtra(IP_ADDR);
        } else {
            mFileSend = mFilePath;
        }
        if (TextUtils.isEmpty(ip)) {
            ip = "";
        }
        mBtnSend = findViewById(R.id.btn_send);
        mTvSendFile = findViewById(R.id.tv_filesend);
        mEtIp = findViewById(R.id.et_ip);
        mEtIp.setText(ip);
        mEtIp.setSelection(ip.length());
        mTvSendFile.setOnClickListener(this);
        mBtnSend.setOnClickListener(this);

        if (!TextUtils.isEmpty(mFileSend)) {
            mTvSendFile.setText(mFileSend);
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_CODE && resultCode == Activity.RESULT_OK) {
            Uri uri = data.getData();//得到uri，后面就是将uri转化成file的过程。
            String strPath = PathUtils.getImageAbsolutePath(this, uri);
            mTvSendFile.setText(strPath);
            mFileSend = strPath;
        }
    }

    void callBack(final String info) {
        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                dlg.setTitle(info);
            }
        });
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.tv_filesend:
                browser();
                break;
            case R.id.btn_send:
                send();
                break;
        }
    }

    class PromptDlg extends AlertDialog {

        public  PromptDlg(Context context) {
            super(context);
        }
    }
    Dialog dlg;

    private void send() {
        if (TextUtils.isEmpty(mFileSend) || !new File(mFileSend).isFile()) {
            Toast.makeText(this, "请选择文件", Toast.LENGTH_LONG).show();
            return;
        }

        final String ip = mEtIp.getText().toString();
//        if (TextUtils.isEmpty(ip)) {
//            Toast.makeText(this, "请输入ip地址", Toast.LENGTH_LONG).show();
//            return;
//        }

        if (dlg == null) {
            dlg = new PromptDlg(this);
        }
        dlg.setTitle("正在计算上传文件,请稍候");
        dlg.setCancelable(false);
        dlg.setCanceledOnTouchOutside(false);
        dlg.show();

        new Thread(new Runnable() {
            @Override
            public void run() {
                final String strRet = JniDef.sendFile(mFileSend, ip, SendFileActivity.this);
                JniDef.sendFileDone();
                if (SendFileActivity.this.isFinishing()) {
                    return;
                }
                SendFileActivity.this.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        dlg.dismiss();
                        mBtnSend.setEnabled(false);
                        mBtnSend.setText(strRet);
                    }
                });

            }
        }).start();
    }
}

