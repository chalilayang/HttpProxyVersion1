LOCAL_PATH:=$(call my-dir)
#SYSROOT:=/Users/fogin/Documents/sohu/android-ndk-r9d/platforms/android-9/arch-arm
LOCAL_CFLAGS:=-DANDROID
#boost闂堟瑦锟戒礁绨遍崠锟�
include $(CLEAR_VARS)
LOCAL_MODULE    := boost_date_time-gcc-mt  
LOCAL_SRC_FILES := ../../../../third/prefix-android_1_49/lib/libboost_date_time-gcc-mt-s-1_49.a 
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := boost_filesystem-gcc-mt  
LOCAL_SRC_FILES := ../../../../third/prefix-android_1_49/lib/libboost_filesystem-gcc-mt-s-1_49.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := boost_iostreams-gcc-mt
LOCAL_SRC_FILES := ../../../../third/prefix-android_1_49/lib/libboost_iostreams-gcc-mt-s-1_49.a  
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := boost_program_options-gcc-mt
LOCAL_SRC_FILES := ../../../../third/prefix-android_1_49/lib/libboost_program_options-gcc-mt-s-1_49.a   
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := boost_regex-gcc-mt  
LOCAL_SRC_FILES := ../../../../third/prefix-android_1_49/lib/libboost_regex-gcc-mt-s-1_49.a  
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := boost_signals-gcc-mt  
LOCAL_SRC_FILES := ../../../../third/prefix-android_1_49/lib/libboost_signals-gcc-mt-s-1_49.a  
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := boost_system-gcc-mt
LOCAL_SRC_FILES := ../../../../third/prefix-android_1_49/lib/libboost_system-gcc-mt-s-1_49.a 
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := boost_thread-gcc-mt
LOCAL_SRC_FILES := ../../../../third/prefix-android_1_49/lib/libboost_thread_pthread-gcc-mt-s-1_49.a   
include $(PREBUILT_STATIC_LIBRARY) 

include $(CLEAR_VARS)
LOCAL_MODULE    := SHP2PSystem
LOCAL_SRC_FILES := libSHP2PSystem.so
include $(PREBUILT_SHARED_LIBRARY) 

include $(CLEAR_VARS)
LOCAL_MODULE:= StaticLocalRequest
SOURCE_PATH:= ../../..

#p2pcommon
LOCAL_SRC_FILES := \
				$(SOURCE_PATH)/p2pcommon/base/buffer_pool.cpp\
				$(SOURCE_PATH)/p2pcommon/base/file.cpp\
				$(SOURCE_PATH)/p2pcommon/base/file_api.cpp\
				$(SOURCE_PATH)/p2pcommon/base/socket_api.cpp\
				$(SOURCE_PATH)/p2pcommon/log/logger.cpp\
				$(SOURCE_PATH)/p2pcommon/http_request.cpp\
				$(SOURCE_PATH)/p2pcommon/http_response.cpp\
				$(SOURCE_PATH)/p2pcommon/http_server.cpp\
				$(SOURCE_PATH)/p2pcommon/http_service.cpp
				
#localhttpserver
LOCAL_SRC_FILES += \
				$(SOURCE_PATH)/localhttpserver/local_http_connection.cpp\
				$(SOURCE_PATH)/localhttpserver/local_http_server.cpp\
				$(SOURCE_PATH)/localhttpserver/sh_local_server_api.cpp\
				$(SOURCE_PATH)/localhttpserver/remote_host_handler.cpp\
				$(SOURCE_PATH)/localhttpserver/md5.cpp\
				$(SOURCE_PATH)/localhttpserver/Utils.cpp
				
#D:/Code/p2p/third/prefix-android_1_49/include\
#婢跺瓨鏋冩禒鍓佹窗瑜帮拷  
LOCAL_C_INCLUDES += \
				/Users/fogin/Documents/sohu/android-ndk-r9d/sources/cxx-stl/gnu-libstdc++/4.6/include\
				/Users/fogin/Documents/sohu/android-ndk-r9d/sources/cxx-stl/gnu-libstdc++/4.6/libs/armeabi/include\
				./prefix-android_1_49/include\
				$(SOURCE_PATH)\
				$(SOURCE_PATH)/include\
				$(SOURCE_PATH)/p2pcommon\
				$(SOURCE_PATH)/p2pcommon/base\
				$(SOURCE_PATH)/p2pcommon/log\
				$(SOURCE_PATH)/localhttpserver

LOCAL_CPP_FEATURES += exceptions\
					rtti			
#寮曠敤p2p搴�					
LOCAL_SHARED_LIBRARIES := SHP2PSystem

#娓氭繆绂嗛棃娆愶拷浣哥氨
LOCAL_STATIC_LIBRARIES := boost_date_time-gcc-mt\
						boost_filesystem-gcc-mt\
						boost_iostreams-gcc-mt\
						boost_program_options-gcc-mt\
						boost_regex-gcc-mt\
						boost_signals-gcc-mt\
						boost_system-gcc-mt\
						boost_thread-gcc-mt

include $(BUILD_STATIC_LIBRARY)

#閸斻劍锟戒礁绨� 閼奉亜绻佺亸浣筋棅濞村鐦悽锟�
include $(CLEAR_VARS)
LOCAL_MODULE:=SHLocalRequest

SOURCE_PATH:=../source
#search all cpp file add back
LOCAL_SRC_FILES += \
			local_http_server_jni.cpp

LOCAL_C_INCLUDES += \
				$(LOCAL_PATH)\
				$(SOURCE_PATH)/api
				
#娓氭繆绂嗛棃娆愶拷浣哥氨
LOCAL_STATIC_LIBRARIES := StaticLocalRequest

LOCAL_CPP_FEATURES += exceptions\
					rtti

LOCAL_LDFLAGS +=\
				-Wl,-v

LOCAL_LDLIBS :=-llog

include $(BUILD_SHARED_LIBRARY)