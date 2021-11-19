# Inherit from common product
$(call inherit-product-if-exists, $(SRC_TARGET_DIR)/product/embedded.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/languages_full.mk)

# Inherit from device specific
$(call inherit-product, device/lge/mcv5a/device.mk)

# Inherit from omni product
$(call inherit-product, vendor/omni/config/common.mk)
$(call inherit-product, vendor/omni/config/gsm.mk)

# Device identifier
PRODUCT_DEVICE := mcv5a
PRODUCT_NAME := omni_mcv5a
PRODUCT_MODEL := LM-Q610
PRODUCT_BRAND := lge
PRODUCT_MANUFACTURER := lge

# Property overrides
PRODUCT_PROPERTY_OVERRIDES += \
    ro.treble.enabled=true