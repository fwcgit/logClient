package com.fwc.log;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.View;

public class MainActivity extends AppCompatActivity {


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        final TelLog telLog = new TelLog();
        telLog.nativeConnectServer("10.40.252.237",38888);
        telLog.connect();

        Thread.setDefaultUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread thread, Throwable throwable) {
                telLog.sendLog(throwable);
                telLog.nativeFree();
            }
        });
        findViewById(R.id.button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {

                String str = null;
                str.length();


            }
        });
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

    }
}
