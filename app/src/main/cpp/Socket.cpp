//
// Created by fwc on 2017/7/13.
//
#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include "Echo.cpp"
#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include "jnih/com_fwc_log_TelLog.h"

#define logD(...)  __android_log_print(ANDROID_LOG_DEBUG,"smart jni log", __VA_ARGS__)

static int socket_id = 0;
static volatile bool is_connect = false;
static volatile bool is_run = true;
static  char* server_ip = NULL;
static  int server_port = 0;
static volatile bool is_try_connect = false;
static volatile bool connect_backup = false;
static volatile bool is_init_connect = false;

static pthread_t  pthread_ID = NULL;
static  JavaVM *gVm = NULL;
static  jobject gObj = NULL;

static volatile int disconnect_count = 0;

static volatile bool history_connect = false;

void sendHearData();
void callConnectStatus();
void connect_server();
void connect_server_try();

static int CreateTcpSocket(JNIEnv *env,jobject obj)
{
    logD("create new Tcp socket");

    int tcpSocket = socket(PF_INET,SOCK_STREAM,0);

    if(-1 == tcpSocket)
    {
        logD("create new Tcp socket fail");
    }

    return tcpSocket;
}

static void ConnectToAddress(JNIEnv *env,jobject obj,int socketID, const char *ip, unsigned short port)
{

    logD("Connecting to server");

    struct sockaddr_in address;
    memset(&address,0, sizeof(address));

    address.sin_family = PF_INET;

    if(0 == inet_aton(ip,&(address.sin_addr)))
    {
        is_connect = false;
        logD(" address connect fail");
    }
    else
    {
        address.sin_port = htons(port);

        if(-1 == connect(socketID,(struct sockaddr *)&address, sizeof(address)))
        {
            is_connect = false;

//            shutdown(socketID,2);
//            close(socketID);

            logD("connect fail");
        }
        else
        {

            history_connect = true;
            is_connect = true;
            callConnectStatus();
            logD("Connected....");
        }

    }

}

void* read_socket_data_thread(void*)
{

    logD("read_socket_data_thread");

    JNIEnv *env = NULL;
    int res  = gVm->AttachCurrentThread(&env,NULL);

    if(0 == res )
    {
       char buffer[MAX_BUFFER_SIZE];

       while(is_run)
      {

        if(is_try_connect && !connect_backup)
        {
            if(NULL != server_ip){

                if(!is_connect){

                    connect_server_try();

                }
            }
        }

        if(is_connect){

            ssize_t  recvSize = recv(socket_id,buffer,MAX_BUFFER_SIZE,0);

            if(-1 == recvSize){

                is_connect = false;
                LogMessage(env,gObj,"no recv data");

            }else{

                if(recvSize > 0)
                {
                    jclass clz = env->GetObjectClass(gObj);

                    if(NULL != clz)
                    {
                        jmethodID method_ID = env->GetMethodID(clz,"recvData","([B)V");
                        env->DeleteLocalRef(clz);

                        if(NULL != method_ID)
                        {

                            jbyteArray ba = env->NewByteArray(recvSize);

                            jbyte *jBytes = (jbyte*)malloc(recvSize* sizeof(jbyte));
                            memset(jBytes,0,recvSize* sizeof(jbyte));

                            for(int i = 0;i < recvSize;i++)
                            {
                                *(jBytes+i) = buffer[i];
                            }

                            env->SetByteArrayRegion(ba,0,recvSize,jBytes);

                            env->CallVoidMethod(gObj,method_ID,ba);

                            free(jBytes);
                            jBytes = NULL;

                            env->DeleteLocalRef(ba);

                        }

                    }

                    logD("received %d bytes:%s",recvSize,buffer);
                }
                else{
                    is_connect = false;
                    logD("socket disconnected.");
                }
            }
        }

      }

        gVm->DetachCurrentThread();

        pthread_ID = NULL;

        logD("read_socket_data_thread exit");

        pthread_exit(0);

    }else{
        logD("read_socket_data_thread %d",res);
    }
    return (void *) 1;
}

static void sendToSocket(JNIEnv *env,jobject obj,const char *buffer,size_t buffersize)
{
    if(is_connect)
    {
        logD("sending to the socket....");

        size_t  sendSize = send(socket_id,buffer,buffersize,0);

        if(-1 == sendSize){

            is_connect = false;
            logD("sending fail ,socket the close....");
        }
        else
        {
            if(sendSize > 0) {
                logD("send %d bytes", sendSize);
            }
        }
    }
    else
    {
        logD("sending fail ,socket the close....");
    }
}

static void timer_task(int sig)
{
    if(SIGALRM == sig)
    {
        logD("check socket is_connect");

        if(gVm != NULL)
        {
            JNIEnv *jniEnv = NULL;

            gVm->GetEnv((void **)&jniEnv,JNI_VERSION_1_6);

            if(jniEnv == NULL)
                return;


            if(!is_connect)
            {
                if(disconnect_count++ > 3)
                {
                    is_try_connect = true;
                    disconnect_count = 0;
                }

            }else{
                sendHearData();
            }

        }

        callConnectStatus();

        alarm(10);

    }

    return ;
}

void sendHearData()
{

    JNIEnv *jniEnv = NULL;

    gVm->GetEnv((void **)&jniEnv,JNI_VERSION_1_6);

    if(jniEnv == NULL)
        return;

    char *buffer = (char *) malloc(sizeof(char));
    memset(buffer,0,sizeof(char));
    *(buffer) = 0;
    sendToSocket(jniEnv,gObj,buffer, 1);
    free(buffer);
    buffer = NULL;
}

/*
 * Class:     com_fu_smart_jni_Socket
 * Method:    nativeConnectServer
 * Signature: (Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL Java_com_fwc_log_TelLog_nativeConnectServer
        (JNIEnv *env, jobject obj, jstring ip, jint port)
{

    const char* ipAddr = env->GetStringUTFChars(ip,NULL);

    if(gObj == NULL)
    {
        gObj = env->NewGlobalRef(obj);

        if(gObj == NULL)
        {
            logD("create public obj fail");
        }
    }

    server_ip = (char *) malloc(strlen(ipAddr) * sizeof(char));
    strcpy(server_ip,ipAddr);
    server_port = port;

    env->ReleaseStringUTFChars(ip,ipAddr);

    if(NULL == pthread_ID){

        is_run = true;

        if(!pthread_create(&pthread_ID,NULL,read_socket_data_thread,(void *)NULL))
        {
            logD("pthread create success....");
        }else{
            logD("pthread create fail....");
        }
    }

}


JNIEXPORT void JNICALL Java_com_fwc_log_TelLog_sendData
        (JNIEnv *env, jobject obj, jstring msg)
{
    const char *msgBuffer = env->GetStringUTFChars(msg,NULL);
    jsize msgSize = env->GetStringUTFLength(msg);
    sendToSocket(env,obj,msgBuffer,msgSize);
    env->ReleaseStringUTFChars(msg,msgBuffer);
}

/*
 * Class:     com_fu_smart_jni_Socket
 * Method:    sendData
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL Java_com_fwc_log_TelLog_sendDataByte
        (JNIEnv *env, jobject obj, jbyteArray data)
{

    jsize size = env->GetArrayLength(data);

    char *buff = (char*)malloc(size* sizeof(char));

    jbyte *jbyteList = env->GetByteArrayElements(data,NULL);

    for(int i = 0 ;i < size;i++)
    {
        *(buff+i) = *(jbyteList+i);
    }

    sendToSocket(env,obj,buff,size);

    env->ReleaseByteArrayElements(data,jbyteList,0);

    free(buff);
    buff = NULL;


}

JNIEXPORT jboolean JNICALL Java_com_fwc_log_TelLog_isConnect
        (JNIEnv *, jobject)
{
    return (jboolean)is_connect;
}


jint JNI_OnLoad(JavaVM *vm,void *reserved)
{
    gVm = vm;

    return JNI_VERSION_1_4;
}

/*
 * Class:     com_fu_smart_jni_Socket
 * Method:    nativeFree
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_fwc_log_TelLog_nativeFree
        (JNIEnv *env, jobject obj)
{


    if(NULL != gObj)
    {
        env->DeleteGlobalRef(gObj);
        gObj = NULL;
    }

    logD("nativeFree");

    exit(0);

}

JNIEXPORT void JNICALL Java_com_fwc_log_TelLog_disConnect
        (JNIEnv *env, jobject obj)
{

    is_run = false;

    alarm(0);
    shutdown(socket_id,2);
    close(socket_id);

    is_connect = false;

    socket_id = NULL;
    server_ip = NULL;
}

JNIEXPORT void JNICALL Java_com_fwc_log_TelLog_connect
        (JNIEnv *env, jobject obj)
{

    if(NULL != server_ip && !is_init_connect){

        is_init_connect = true;

        connect_server();

        signal(SIGALRM,timer_task);
        alarm(10);
    }else{

        signal(SIGALRM,timer_task);
        alarm(10);

    }

}

void callConnectStatus()
{

    JNIEnv *jniEnv = NULL;

    gVm->GetEnv((void **)&jniEnv,JNI_VERSION_1_6);

    if(jniEnv == NULL)
        return;

    jclass clz = jniEnv->GetObjectClass(gObj);
    if(NULL != clz)
    {
        jmethodID methodID = jniEnv->GetMethodID(clz,"status","(I)V");
        jniEnv->DeleteLocalRef(clz);

        if(NULL != methodID)
        {
            jniEnv->CallVoidMethod(gObj,methodID,is_connect ?  1 : 0);
        }
    }
}

void connect_server(){

    JNIEnv *jniEnv = NULL;

    gVm->GetEnv((void **)&jniEnv,JNI_VERSION_1_6);

    if(NULL != jniEnv){

        connect_backup = true;

        socket_id = CreateTcpSocket(jniEnv,gObj);

        ConnectToAddress(jniEnv,gObj,socket_id,server_ip,(unsigned short)server_port);

        connect_backup = false;

    }


}

void connect_server_try(){

    is_try_connect = false;

    if(connect_backup)
        return;


    logD("try socket connect");

    JNIEnv *jniEnv = NULL;

    gVm->GetEnv((void **)&jniEnv,JNI_VERSION_1_6);

    if(NULL != jniEnv){

        connect_backup = true;

        if(history_connect){

            history_connect = false;

            if(NULL != socket_id){

                shutdown(socket_id,2);
                close(socket_id);
            }

            socket_id = CreateTcpSocket(jniEnv,gObj);
        }

        ConnectToAddress(jniEnv,gObj,socket_id,server_ip,(unsigned short)server_port);

        connect_backup = false;

    }
}




