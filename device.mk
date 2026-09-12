#
# Copyright (C) 2011 The Android Open-Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

$(call inherit-product, $(SRC_TARGET_DIR)/product/languages_full.mk)

$(call inherit-product, vendor/pantech/ef52/ef52-vendor.mk)

DEVICE_PACKAGE_OVERLAYS += device/pantech/ef52/overlay

# Device uses high-density artwork where available
PRODUCT_AAPT_CONFIG := normal hdpi xhdpi
PRODUCT_AAPT_PREF_CONFIG := xhdpi

# Camera
#PRODUCT_PACKAGES += \
#    camera.msm8960

# Lights
PRODUCT_PACKAGES += \
	lights.msm8960

# LiveDisplay
PRODUCT_PACKAGES += \
    vendor.lineage.livedisplay@2.0-service.ef52

# audio UCM files
#PRODUCT_COPY_FILES += \
                       device/pantech/ef52/snd_soc_msm/snd_soc_msm_2x_Fusion3:system/etc/snd_soc_msm/snd_soc_msm_2x_Fusion3 \

# Checking model
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/releasetools/device_check.sh:system/bin/device_check.sh

# keylayout
PRODUCT_COPY_FILES += \
	device/pantech/ef52/keylayout/AVRCP.kl:system/usr/keylayout/AVRCP.kl \
	device/pantech/ef52/keylayout/Generic.kl:system/usr/keylayout/Generic.kl \
	device/pantech/ef52/keylayout/gpio-keys.kl:system/usr/keylayout/gpio-keys.kl \
	device/pantech/ef52/keylayout/qwerty.kl:system/usr/keylayout/qwerty.kl \
	device/pantech/ef52/keylayout/apq8064-tabla-snd-card_Button_Jack.kl:system/usr/keylayout/apq8064-tabla-snd-card_Button_Jack.kl 

# keychars
PRODUCT_COPY_FILES += \
	device/pantech/ef52/keychars/Generic.kcm:system/usr/keychars/Generic.kcm \
	device/pantech/ef52/keychars/qwerty2.kcm:system/usr/keychars/qwerty2.kcm \
	device/pantech/ef52/keychars/qwerty.kcm:system/usr/keychars/qwerty.kcm \
	device/pantech/ef52/keychars/Virtual.kcm:system/usr/keychars/Virtual.kcm

# idc
PRODUCT_COPY_FILES += \
	device/pantech/ef52/idc/qwerty2.idc:system/usr/idc/qwerty2.idc \
	device/pantech/ef52/idc/qwerty.idc:system/usr/idc/qwerty.idc \

PRODUCT_COPY_FILES += \
	device/pantech/ef52/thermald/thermald-8064_ef52.conf:$(TARGET_COPY_OUT_VENDOR)/etc/thermald.conf \
	device/pantech/ef52/thermald/thermald-8064_ef52.conf:$(TARGET_COPY_OUT_VENDOR)/etc/thermald-8064.conf \
	device/pantech/ef52/thermald/thermald-8064ab_ef52.conf:$(TARGET_COPY_OUT_VENDOR)/etc/thermald-8064ab.conf

PRODUCT_PROPERTY_OVERRIDES += \
	ro.sf.lcd_density=300

# Enable unauthenticated root ADB for bring-up builds before Android starts.
ifneq ($(filter userdebug eng,$(TARGET_BUILD_VARIANT)),)
# Product properties take precedence over platform/Lineage defaults.
PRODUCT_PRODUCT_PROPERTIES += \
    ro.secure=0 \
    ro.adb.secure=0 \
    ro.debuggable=1 \
    persist.sys.usb.config=adb

PRODUCT_COPY_FILES += \
    device/pantech/ef52/init.ef52.adb.rc:$(TARGET_COPY_OUT_SYSTEM)/etc/init/init.ef52.adb.rc
endif

# Inherit from msm8960-common
$(call inherit-product, device/pantech/msm8960-common/msm8960.mk)

# PN544 uses the legacy NXP HCI stack and the proprietary board HAL.
PRODUCT_PACKAGES += NfcPn544
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.nfc.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.nfc.xml

# Reject early modem requests until the stock QCRIL has initialized QMI.
PRODUCT_PACKAGES += libril_ef52
