/*---------------------------------------------------------------------------
 File name:  local_http_server_jni.c
 Contents: This module is the JNI implementation to manipulate the local http server.
 Author:	ZHANG Qinghua
 Date:		 2014-08-26
 --------------------------------------------------------------------------------*/

#include <jni.h>
#include <assert.h>
#include <android/log.h>

#include "local_http_server_jni.h"
//#include "../source/api/sh_p2p_system_api.h"
#include "../../../include/sh_p2p_system_define.h"
#include "../../../include/sh_local_server_api.h"

static SHP2PSystemParam s_system_param;
static JavaVM* g_javaVM;
static jobject g_p2p_system_self;
static jobject g_ret_buffer;

jint startLocalServer(JNIEnv *evn, jobject self, jobject p2p_system_param) {

	memset(&s_system_param, 0, sizeof(s_system_param));

	evn->GetJavaVM(&g_javaVM);
	g_p2p_system_self = evn->NewGlobalRef(self);

	//g_ret_buffer = evn->NewGlobalRef(retBuffer);

	jclass P2PSystemParam_class = evn->GetObjectClass(p2p_system_param);
	//      jclass cls_notify = evn->FindClass("com/p2p/SHP2PSystem/SHP2pSystemNofity");
	//      g_cls_notify = evn->NewGlobalRef(cls_notify);

	jfieldID app_version_field = evn->GetFieldID(P2PSystemParam_class, "app_version", "Ljava/lang/String;");
	if (app_version_field == NULL)
	{
		return false;
	}
	jobject app_version = evn->GetObjectField(p2p_system_param, app_version_field);
	jboolean ret;
	char const* papp_version = evn->GetStringUTFChars((jstring)app_version, &ret);
	s_system_param.app_version =(char *)malloc(strlen(papp_version)+1);
	memset(s_system_param.app_version, 0, strlen(papp_version)+1);
	memcpy(s_system_param.app_version, papp_version,strlen(papp_version)+1);

	jfieldID log_path_field = evn->GetFieldID(P2PSystemParam_class, "log_path", "Ljava/lang/String;");
	if (log_path_field == NULL)
	{
		return false;
	}
	jobject log_path = evn->GetObjectField(p2p_system_param, log_path_field);
	char const* plog_path = evn->GetStringUTFChars((jstring)log_path, &ret);
	s_system_param.log_path =(char *)malloc(strlen(plog_path)+1);
	memset(s_system_param.log_path, 0, strlen(plog_path)+1);
	memcpy(s_system_param.log_path, plog_path,strlen(plog_path)+1);
	LOGI("log path %s\n", s_system_param.log_path);

	jfieldID allow_log_field = evn->GetFieldID(P2PSystemParam_class, "allow_log", "Z");
	if (allow_log_field == NULL)
	{
		return false;
	}
	jboolean allow_log = evn->GetBooleanField(p2p_system_param, allow_log_field);
	s_system_param.allow_log = allow_log;

	jfieldID cache_path_field = evn->GetFieldID(P2PSystemParam_class, "cache_path", "Ljava/lang/String;");
	if (cache_path_field == NULL)
	{
		return false;
	}
	jobject cache_path = evn->GetObjectField(p2p_system_param, cache_path_field);
	char const* pcache_path = evn->GetStringUTFChars((jstring)cache_path, &ret);
	s_system_param.cache_path =(char *)malloc(strlen(pcache_path)+1);
	memset(s_system_param.cache_path, 0, strlen(pcache_path)+1);
	memcpy(s_system_param.cache_path, pcache_path, strlen(pcache_path)+1);
	LOGI("cache path %s\n", s_system_param.cache_path);

	jfieldID allow_cache_field = evn->GetFieldID(P2PSystemParam_class, "allow_cache", "Z");
	if (allow_cache_field == NULL)
	{
		return false;
	}
	jboolean allow_cache = evn->GetBooleanField(p2p_system_param, allow_cache_field);
	s_system_param.allow_cache = allow_cache;

	jfieldID cache_limit_field = evn->GetFieldID(P2PSystemParam_class, "cache_limit", "I");
	if (cache_limit_field == NULL)
	{
		return false;
	}
	jint cache_limit = evn->GetIntField(p2p_system_param, cache_limit_field);
	s_system_param.cache_limit = cache_limit;

	jfieldID reg_id_field = evn->GetFieldID(P2PSystemParam_class, "register_id", "I");
	if (log_path_field == NULL)
	{
		return false;
	}
	jint register_id = evn->GetIntField(p2p_system_param, reg_id_field);
	s_system_param.register_id = register_id;

	jfieldID local_ip_field = evn->GetFieldID(P2PSystemParam_class, "local_ip", "Ljava/lang/String;");
	if (local_ip_field == NULL)
	{
		return false;
	}
	jobject local_ip = evn->GetObjectField(p2p_system_param, local_ip_field);
	char const* plocal_ip = evn->GetStringUTFChars((jstring)local_ip, &ret);
	s_system_param.local_ip =(char *)malloc(strlen(plocal_ip)+1);
	memset(s_system_param.local_ip, 0, strlen(plocal_ip)+1);
	memcpy(s_system_param.local_ip, plocal_ip, strlen(plocal_ip)+1);

	init_local_server(s_system_param);

	return 0;
}

jint stopLocalServer(JNIEnv * env, jobject this_obj) {
	uninit_local_server();
	return 0;
}

/**
 * Table of methods associated with LocalServer.
 */
static JNINativeMethod localServerMethods[] = {
/* name, signature, funcPtr */
{ "start", "(Lcom/p2p/SHP2PSystem/SHP2PSystemParam;)I", (void*) startLocalServer },
{ "stop", "()I", (void*) stopLocalServer }, };

/*
 * Register several native methods for one class.
 */
static int registerNativeMethods(JNIEnv* env, const char* className,
		JNINativeMethod* methods, int numMethods) {
	jclass clazz;

	clazz = env->FindClass(className);
	if (clazz == NULL) {
		LOGE("Native registration unable to find class '%s'\n", className);
		return JNI_FALSE;
	}
	if (env->RegisterNatives(clazz, methods, numMethods) < 0) {
		LOGE("RegisterNatives failed for '%s'\n", className);
		return JNI_FALSE;
	}

	return JNI_TRUE;
}

/*
 * Set some test stuff up.
 *
 * Returns the JNI version on success, -1 on failure.
 */
jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	JNIEnv* env = NULL;
	jint result = -1;

	if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
		LOGE("ERROR: GetEnv failed\n");
		goto fail;
	}
	assert(env != NULL);

	LOGI("JNI_OnLoad\n");

	if (!registerNativeMethods(env, LOCAL_HTTP_SERVER_CLASS, localServerMethods,
			sizeof(localServerMethods) / sizeof(localServerMethods[0]))) {
		LOGE("ERROR: WbxmlParser native registration failed\n");
		goto fail;
	}

	/* success -- return valid version number */
	result = JNI_VERSION_1_4;

	fail: return result;
}
