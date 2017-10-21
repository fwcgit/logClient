package com.fwc.log;

import android.os.Handler;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.io.Writer;

public class MainActivity extends AppCompatActivity {


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        final TelLog telLog = new TelLog();
        telLog.nativeConnectServer("139.196.104.98",38888);
        telLog.connect();

        findViewById(R.id.button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                try{
                    String str = null;
                    str.length();

                }catch(Exception e){
                    Writer writer = new StringWriter();
                    PrintWriter printWriter = new PrintWriter(writer);
                    e.printStackTrace(printWriter);
                    Throwable cause = e.getCause();
                    while (cause != null) {
                        cause.printStackTrace(printWriter);
                        cause = cause.getCause();
                    }
                    printWriter.close();
                    String result = writer.toString();

                    telLog.sendLog(result);
                }
            }
        });
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

    }
}
