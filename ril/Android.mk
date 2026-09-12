LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libril_ef52
LOCAL_VENDOR_MODULE := true
LOCAL_SRC_FILES := ril_wrapper.c
LOCAL_C_INCLUDES := hardware/ril/include
LOCAL_CFLAGS := -DRIL_SHLIB -Wall -Wextra -Werror
LOCAL_SHARED_LIBRARIES := libdl liblog
include $(BUILD_SHARED_LIBRARY)
