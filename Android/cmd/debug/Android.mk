LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES = tarfscheck.cpp

LOCAL_CFLAGS =   -O2 -Wall 

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../astl/include

LOCAL_STATIC_LIBRARIES := libtarfs
LOCAL_MODULE := checktarfs
LOCAL_MODULE_TAGS := eng

include $(BUILD_EXECUTABLE)

#$(call dist-for-goals,droid,$(LOCAL_BUILT_MODULE))
