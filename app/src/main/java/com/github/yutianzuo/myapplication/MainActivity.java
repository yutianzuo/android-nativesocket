package com.github.yutianzuo.myapplication;

import android.content.Intent;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.Button;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {

    Button mButtonDns;
    Button mButtonTransFile;
    Button mButtonRecvFile;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mButtonDns = findViewById(R.id.btn_dns);
        mButtonTransFile = findViewById(R.id.btn_transfile);
        mButtonRecvFile = findViewById(R.id.btn_recvfile);

        mButtonDns.setOnClickListener(this);
        mButtonRecvFile.setOnClickListener(this);
        mButtonTransFile.setOnClickListener(this);

    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.btn_dns:
                dns();
                break;
            case R.id.btn_transfile:
                sendFile();
                break;
            case R.id.btn_recvfile:
                recvFile();
                break;
        }
    }

    private void dns() {
        Intent intent = new Intent(MainActivity.this, DnsActivity.class);
        this.startActivity(intent);
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
