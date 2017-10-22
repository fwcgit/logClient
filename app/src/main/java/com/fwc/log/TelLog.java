package com.fwc.log;

import android.os.Build;
import android.util.Log;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.io.Writer;

/**
 * Created by fwc on 2017/10/21.
 */

public class TelLog {
    static {
        System.loadLibrary("Socket");
    }


    private static TelLog telLog;
    public static TelLog getInstance(){
        return telLog == null ? telLog = new TelLog() : telLog;
    }

    public void logMessage(String msg){
        Log.d("JNI_log",msg);
    }

    public native void nativeFree();

    public native void nativeConnectServer(String ip,int port);

    public native void sendData(String msg);

    public native void sendDataByte(byte[] data);

    public native boolean isConnect();

    public native void disConnect();

    public native void connect();

    public TelLog(){
        telLog = this;
    }
    public void recvData(byte[] data){

        String msg = new String(data);
        Log.d("java_JNI_log",msg);

        if(msg.startsWith("-close-")){
            disConnect();
        }else if(msg.startsWith("-update-name-")){
            sendData("-update-name-:19271392");
        }

    }

    public void status(int status){
        Log.d("java_JNI_log",status == 1 ? "日志已连接":"日志连接断开");
    }

    public void sendLog(String str){
        if(isConnect()){
            sendData(Build.BOARD+"_"+Build.MODEL+">>"+str);
        }
    }

    public void sendLog(Throwable throwable){
        if(isConnect()){
            Writer writer = new StringWriter();
            PrintWriter printWriter = new PrintWriter(writer);
            if (throwable != null) {
                throwable.printStackTrace(printWriter);
            }
            printWriter.close();

            sendData(Build.BOARD+"_"+Build.MODEL+">>"+writer.toString());
        }
    }
}
