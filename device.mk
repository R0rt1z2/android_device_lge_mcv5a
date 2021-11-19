LOCAL_PATH := device/lge/mcv5a

# A/B
AB_OTA_PARTITIONS += \
    boot \
    system \
    vendor

AB_OTA_POSTINSTALL_CONFIG += \
    RUN_POSTINSTALL_system=true \
    POSTINSTALL_PATH_system=system/bin/otapreopt_script \
    FILESYSTEM_TYPE_system=ext4 \
    POSTINSTALL_OPTIONAL_system=true

# A/B Scripts
PRODUCT_PACKAGES += \
    otapreopt_script

# A/B Update Engine
PRODUCT_PACKAGES += \
    update_engine \
    update_engine_sideload \
    update_engine_client