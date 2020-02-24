package com.github.yutianzuo.myapplication;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import android.text.TextUtils;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import com.github.yutianzuo.nativesock.JniDef;

import java.io.File;

public class SendFileActivity extends AppCompatActivity implements View.OnClickListener {
    public static final String IP_ADDR = "ip_addr";
    TextView mTvSendFile;
    String mFileSend;
    Button mBtnSend;
    EditText mEtIp;
    static final int REQUEST_CODE = 999;
    String ip;

    private void browser() {
        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        intent.setType("*/*");
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        startActivityForResult(intent, REQUEST_CODE);
    }

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_send);
        ip = getIntent().getStringExtra(IP_ADDR);
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
        if (TextUtils.isEmpty(ip)) {
            Toast.makeText(this, "请输入ip地址", Toast.LENGTH_LONG).show();
            return;
        }

        if (dlg == null) {
            dlg = new PromptDlg(this);
        }
        dlg.setTitle("传输中，进度将显示在接收设备上...");
        dlg.setCancelable(false);
        dlg.setCanceledOnTouchOutside(false);
        dlg.show();

        new Thread(new Runnable() {
            @Override
            public void run() {
                final String strRet = JniDef.sendFile(mFileSend, ip);
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

