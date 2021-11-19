LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_DEVICE),mcv5a)
include $(call all-subdir-makefiles,$(LOCAL_PATH))
endif