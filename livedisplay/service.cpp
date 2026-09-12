/*
 * Copyright (C) 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "vendor.lineage.livedisplay@2.0-service.ef52"

#include <fcntl.h>
#include <unistd.h>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/strings.h>
#include <android-base/unique_fd.h>
#include <hidl/HidlTransportSupport.h>
#include <vendor/lineage/livedisplay/2.0/IAdaptiveBacklight.h>

using android::hardware::Return;
using vendor::lineage::livedisplay::V2_0::IAdaptiveBacklight;

namespace {

constexpr const char* kCabcPath = "/sys/devices/virtual/graphics/fb0/cabc_ctl";

bool readEnabled(bool* enabled) {
    std::string value;
    if (!android::base::ReadFileToString(kCabcPath, &value)) {
        PLOG(ERROR) << "Could not read CABC state";
        return false;
    }
    value = android::base::Trim(value);
    if (value != "0" && value != "1") {
        LOG(ERROR) << "Invalid CABC state: " << value;
        return false;
    }
    *enabled = value == "1";
    return true;
}

class AdaptiveBacklight : public IAdaptiveBacklight {
    Return<bool> isEnabled() override {
        bool enabled = false;
        return readEnabled(&enabled) && enabled;
    }

    Return<bool> setEnabled(bool enabled) override {
        android::base::unique_fd fd(open(kCabcPath, O_WRONLY | O_CLOEXEC));
        if (fd < 0) {
            PLOG(ERROR) << "Could not open CABC control";
            return false;
        }

        // The Pantech ABI reads 1 for enabled, but writes 0 to enable.
        const char command = enabled ? '0' : '1';
        // The legacy store callback returns the ASCII value of the command
        // instead of the byte count. Use one write and verify the state rather
        // than a write-all helper, which assumes a conventional byte count.
        const ssize_t result = TEMP_FAILURE_RETRY(write(fd, &command, sizeof(command)));
        if (result < 0) {
            PLOG(ERROR) << "Could not write CABC control";
            return false;
        }
        if (result != static_cast<ssize_t>(sizeof(command)) && result != command) {
            LOG(ERROR) << "Unexpected CABC write result: " << result;
            return false;
        }
        bool actual = false;
        return readEnabled(&actual) && actual == enabled;
    }
};

}  // namespace

int main() {
    android::hardware::configureRpcThreadpool(1, true);
    android::sp<IAdaptiveBacklight> service = new AdaptiveBacklight();
    if (service->registerAsService() != android::OK) {
        LOG(ERROR) << "Could not register adaptive backlight service";
        return 1;
    }
    android::hardware::joinRpcThreadpool();
    return 1;
}
