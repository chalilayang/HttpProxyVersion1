#ifndef _LOCAL_HTTP_SERVER_JNI_
#define _LOCAL_HTTP_SERVER_JNI_

#define LOCAL_HTTP_SERVER_CLASS "com/p2p/SHP2PSystem/LocalHttpServer"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO, __FILE__, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, __FILE__, __VA_ARGS__)

jint startLocalServer(JNIEnv *evn, jobject self, jobject p2p_system_param);
jint stopLocalServer(JNIEnv *env, jobject this_obj);


#endif //_LOCAL_HTTP_SERVER_JNI_
