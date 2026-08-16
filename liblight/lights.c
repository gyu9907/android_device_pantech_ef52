/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (C) 2011 Diogo Ferreira <defer@LineageOS.com>
 * Copyright (C) 2014 The LineageOS Project <http://www.LineageOS.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "lights.msm8960"

#include <cutils/log.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#include <sys/ioctl.h>
#include <sys/types.h>

#include <hardware/lights.h>

/* Synchronization primities */
static pthread_once_t g_init = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

/* Mini-led state machine */
static struct light_state_t g_notification;
static struct light_state_t g_battery;

char const*const RED_LED_FILE 			= "/sys/class/leds/red/brightness";
char const*const GREEN_LED_FILE 		= "/sys/class/leds/green/brightness";
char const*const BLUE_LED_FILE                  = "/sys/class/leds/blue/brightness";
char const*const RED_LED_BLINK_FILE             = "/sys/class/leds/red/pan_led";
char const*const GREEN_LED_BLINK_FILE           = "/sys/class/leds/green/pan_led";
char const*const BLUE_LED_BLINK_FILE            = "/sys/class/leds/blue/pan_led";
char const*const LCD_BACKLIGHT_FILE	= "/sys/class/leds/lcd-backlight/brightness";

static int g_backlight = 255;

/* The leds we have */
enum {
	LED_RED,
	LED_GREEN,
        LED_BLUE,
	LED_BLANK
};

enum {
	MANUAL = 0,
	AUTOMATIC,
	MANUAL_SENSOR
};

static int write_int (const char *path, unsigned int value) {
	int fd;
	static int already_warned = 0;

	fd = open(path, O_RDWR);
	if (fd < 0) {
		if (already_warned == 0) {
			ALOGE("write_int failed to open %s\n", path);
			already_warned = 1;
		}
		return -errno;
	}

	char buffer[20];
	int bytes = snprintf(buffer, sizeof(buffer), "%u\n", value);
	int written = write (fd, buffer, bytes);
	close(fd);

	return written == -1 ? -errno : 0;
}

/* Color tools */
static int is_lit (struct light_state_t const* state) {
	return state->color & 0x00ffffff;
}

static int rgb_to_brightness (struct light_state_t const* state) {
	int color = state->color & 0x00ffffff;
	return ((77*((color>>16)&0x00ff))
			+ (150*((color>>8)&0x00ff)) + (29*(color&0x00ff))) >> 8;
}

/* The actual lights controlling section */
static int set_light_backlight (struct light_device_t *dev, struct light_state_t const *state) {
	int err = 0;
	int brightness = rgb_to_brightness(state);
	(void)dev;

	ALOGV("%s brightness=%d color=0x%08x", __func__,brightness,state->color);
	pthread_mutex_lock(&g_lock);
	g_backlight = brightness;
	err = write_int (LCD_BACKLIGHT_FILE, brightness);
	pthread_mutex_unlock(&g_lock);
	return err;
}

/*
 * pan_led is the vendor kernel interface used by EF52 for hardware blinking:
 * bit 0       enable
 * bits 3-6    brightness level (0-10)
 * bits 8-15   on time in 10 ms units
 * bits 16-31  off time in 10 ms units
 */
static unsigned int make_blink_value(int brightness, int on_ms, int off_ms) {
	unsigned int level;
	unsigned int on;
	unsigned int off;

	if (!brightness || on_ms <= 0 || off_ms <= 0)
		return 0;

	level = (brightness * 10 + 254) / 255;
	on = (on_ms + 9) / 10;
	off = (off_ms + 9) / 10;

	if (on > 0xff)
		on = 0xff;
	if (off > 0xffff)
		off = 0xffff;

	return 1 | (level << 3) | (on << 8) | (off << 16);
}

static int set_shared_light_locked (struct light_device_t *dev, struct light_state_t *state) {
	int r, g, b;
	int err = 0;
	int ret;
	(void)dev;

	r = (state->color >> 16) & 0xFF;
	g = (state->color >> 8) & 0xFF;
	b = (state->color) & 0xFF;

	if (state->flashMode == LIGHT_FLASH_TIMED &&
			state->flashOnMS > 0 && state->flashOffMS > 0) {
		ret = write_int(RED_LED_BLINK_FILE,
				make_blink_value(r, state->flashOnMS, state->flashOffMS));
		if (ret < 0)
			err = ret;
		ret = write_int(GREEN_LED_BLINK_FILE,
				make_blink_value(g, state->flashOnMS, state->flashOffMS));
		if (ret < 0)
			err = ret;
		ret = write_int(BLUE_LED_BLINK_FILE,
				make_blink_value(b, state->flashOnMS, state->flashOffMS));
		if (ret < 0)
			err = ret;
	} else {
		/* Stop a previous LPG pattern before selecting a steady color. */
		write_int(RED_LED_BLINK_FILE, 0);
		write_int(GREEN_LED_BLINK_FILE, 0);
		write_int(BLUE_LED_BLINK_FILE, 0);

		ret = write_int(RED_LED_FILE, r);
		if (ret < 0)
			err = ret;
		ret = write_int(GREEN_LED_FILE, g);
		if (ret < 0)
			err = ret;
		ret = write_int(BLUE_LED_FILE, b);
		if (ret < 0)
			err = ret;
	}

	ALOGV("LED write red=%d, green=%d, blue=%d, mode=%d, on=%d, off=%d",
			r, g, b, state->flashMode, state->flashOnMS, state->flashOffMS);
	return err;
}

static int handle_shared_battery_locked (struct light_device_t *dev) {
	if (is_lit (&g_notification)) {
		return set_shared_light_locked (dev, &g_notification);
	} else {
		return set_shared_light_locked (dev, &g_battery);
	}
}

static int set_light_battery (struct light_device_t *dev, struct light_state_t const* state) {
	int err;

	pthread_mutex_lock (&g_lock);
	g_battery = *state;
	err = handle_shared_battery_locked(dev);
	pthread_mutex_unlock (&g_lock);
	return err;
}

static int set_light_notifications (struct light_device_t *dev, struct light_state_t const* state) {
	int err;

	pthread_mutex_lock (&g_lock);
	g_notification = *state;
	err = handle_shared_battery_locked(dev);
	pthread_mutex_unlock (&g_lock);
	return err;
}

/* Initializations */
void init_globals () {
	pthread_mutex_init (&g_lock, NULL);
}

/* Glueing boilerplate */
static int close_lights (struct light_device_t *dev) {
	if (dev)
		free(dev);

	return 0;
}

static int open_lights (const struct hw_module_t* module, char const* name,
						struct hw_device_t** device) {
	int (*set_light)(struct light_device_t* dev,
					 struct light_state_t const *state);

	if (0 == strcmp(LIGHT_ID_BACKLIGHT, name)) {
		set_light = set_light_backlight;
	}
	else if (0 == strcmp(LIGHT_ID_BATTERY, name)) {
		set_light = set_light_battery;
	}
	else if (0 == strcmp(LIGHT_ID_NOTIFICATIONS, name)) {
		set_light = set_light_notifications;
	}
	else {
		return -EINVAL;
	}

	pthread_once (&g_init, init_globals);
	struct light_device_t *dev = malloc(sizeof (struct light_device_t));
	memset(dev, 0, sizeof(*dev));

	dev->common.tag 	= HARDWARE_DEVICE_TAG;
	dev->common.version = 0;
	dev->common.module 	= (struct hw_module_t*)module;
	dev->common.close 	= (int (*)(struct hw_device_t*))close_lights;
	dev->set_light 		= set_light;

	*device = (struct hw_device_t*)dev;
	return 0;
}

static struct hw_module_methods_t lights_module_methods = {
	.open = open_lights,
};


struct hw_module_t HAL_MODULE_INFO_SYM = {
	.tag = HARDWARE_MODULE_TAG,
	.version_major = 1,
	.version_minor = 0,
	.id = LIGHTS_HARDWARE_MODULE_ID,
	.name = "pantech msm8960 devices lights module",
	.author = "Diogo Ferreira <defer@LineageOS.com>",
	.methods = &lights_module_methods,
};
