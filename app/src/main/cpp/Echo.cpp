//
// Created by fwc on 2017/7/13.
//
#include <jni.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>

#define  MAX_LOG_MESSAGE_LENGTH 256
#define  MAX_BUFFER_SIZE 256


/***
 * 日志
 * @param env
 * @param obj
 * @param format
 */
static void LogMessage(JNIEnv* env ,jobject obj,const char* format,...)
{
    static  jmethodID   methodID = NULL;

    if(NULL == methodID)
    {
        jclass  clazz = env->GetObjectClass(obj);
        methodID = env->GetMethodID(clazz,"logMessage","(Ljava/lang/String;)V");
        env->DeleteLocalRef(clazz);
    }

    if(NULL != methodID)
    {
        char buffer[MAX_LOG_MESSAGE_LENGTH];

        va_list  ap;
        va_start(ap,format);
        vsnprintf(buffer,MAX_LOG_MESSAGE_LENGTH,format,ap);
        va_end(ap);

        jstring  message = env->NewStringUTF(buffer);

        if(NULL != message)
        {
            env->CallVoidMethod(obj,methodID,message);
            env->DeleteLocalRef(message);
        }

    }
}

/***
 * 异常
 * @param env
 * @param className
 * @param message
 */
static void ThrowException(JNIEnv* env,const char* className,const char* message)
{
    jclass clazz = env->FindClass(className);

    if(NULL != clazz)
    {
        env->ThrowNew(clazz,message);
        env->DeleteLocalRef(clazz);
    }
}

/***
 * Errno 编号 异常
 * @param env
 * @param className
 * @param message
 * @param errnum
 */
static void ThrowErrnoExcepction(JNIEnv* env,const char* className,int errnum )
{
    char buffer[MAX_LOG_MESSAGE_LENGTH];

    if(-1 == strerror_r(errnum, buffer, MAX_LOG_MESSAGE_LENGTH))
    {
        strerror_r(errno,buffer,MAX_LOG_MESSAGE_LENGTH);
    }

    ThrowException(env,className,buffer);
}