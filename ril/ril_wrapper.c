/*
 * Copyright (C) 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */
#define LOG_TAG "Ef52Ril"
#include <dlfcn.h>
#include <log/log.h>
#include <telephony/ril.h>

static const struct RIL_Env *ril_env;
static const RIL_RadioFunctions *vendor;
static RIL_RadioFunctions wrapped;

static void on_request(int request, void *data, size_t length, RIL_Token token) {
    /* QCRIL publishes callbacks before its asynchronous QMI setup completes. */
    if (vendor->onStateRequest() == RADIO_STATE_UNAVAILABLE) {
        ril_env->OnRequestComplete(token, RIL_E_RADIO_NOT_AVAILABLE, NULL, 0);
        return;
    }
    vendor->onRequest(request, data, length, token);
}

const RIL_RadioFunctions *RIL_Init(const struct RIL_Env *env, int argc, char **argv) {
    void *handle = dlopen("/vendor/lib/libril-qc-qmi-1.so", RTLD_NOW | RTLD_LOCAL);
    if (handle == NULL) {
        ALOGE("Cannot load QCRIL: %s", dlerror());
        return NULL;
    }
    const RIL_RadioFunctions *(*init)(const struct RIL_Env *, int, char **) =
            dlsym(handle, "RIL_Init");
    if (init == NULL) {
        ALOGE("Cannot resolve QCRIL RIL_Init: %s", dlerror());
        dlclose(handle);
        return NULL;
    }
    ril_env = env;
    vendor = init(env, argc, argv);
    if (vendor == NULL || vendor->onRequest == NULL || vendor->onStateRequest == NULL) {
        ALOGE("QCRIL returned incomplete callbacks");
        return NULL;
    }
    wrapped = *vendor;
    wrapped.onRequest = on_request;
    return &wrapped;
}
