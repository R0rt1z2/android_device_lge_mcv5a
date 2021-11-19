LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := bootctrl.mt6755
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
LOCAL_SRC_FILES := bootctrl.cpp
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += system/core/fs_mgr/include
LOCAL_STATIC_LIBRARIES += libfs_mgr libbase liblog libcutils
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := bootctrl.mt6755
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
LOCAL_SRC_FILES := bootctrl.cpp
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += system/core/fs_mgr/include
LOCAL_STATIC_LIBRARIES += libfs_mgr libbase liblog libcutils
include $(BUILD_STATIC_LIBRARY)
