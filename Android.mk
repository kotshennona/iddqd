LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := iddqd
LOCAL_MODULE_TAGS := eng

LOCAL_SRC_FILES := iddqd.c
LOCAL_SHARED_LIBRARIES := libcutils 
LOCAL_CFLAGS := -Wall

include $(BUILD_EXECUTABLE)

