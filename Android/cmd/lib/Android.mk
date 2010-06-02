LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES = \
    dir.cpp \
    file.cpp \
    fsmaker.cpp \
    fsreader.cpp \
    inode.cpp \
    inode_factory.cpp \
    parser.cpp \
    space.cpp \
    util.cpp 

LOCAL_CFLAGS =   -O2 -Wall 

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../astl/include

LOCAL_MODULE := libtarfs
LOCAL_MODULE_TAGS := eng

include $(BUILD_STATIC_LIBRARY)

