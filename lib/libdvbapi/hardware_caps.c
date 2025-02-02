/*
 * determine the capabilities of the hardware.
 *
 * (C) 2010-2012 Stefan Seyfried
 *
 * License: GPL v2 or later
 */

#include <config.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <hardware_caps.h>

#define FP_DEV "/dev/dbox/oled0"
static int initialized = 0;
static hw_caps_t caps;

hw_caps_t *get_hwcaps(void)
{
	if (initialized)
		return &caps;

	memset(&caps, 0, sizeof(hw_caps_t));

	caps.pip_devs = 0;
	if (access("/dev/dvb/adapter0/video1", F_OK) != -1)
		caps.pip_devs = 1;
	if (access("/dev/dvb/adapter0/video2", F_OK) != -1)
		caps.pip_devs = 2;
	if (access("/dev/dvb/adapter0/video3", F_OK) != -1)
		caps.pip_devs = 3;
	if (caps.pip_devs > 0)
		caps.can_pip = 1;

#if BOXMODEL_VUSOLO4K
	initialized = 1;
	caps.has_CI = 1;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 480;
	caps.display_yres = 320;
	caps.display_type = HW_DISPLAY_GFX;
	caps.display_can_deepstandby = 0;	// 0 because we use graphlcd/lcd4linux
	caps.display_can_set_brightness = 0;	// 0 because we use graphlcd/lcd4linux
	caps.display_has_statusline = 0;	// 0 because we use graphlcd/lcd4linux
	caps.display_has_colon = 0;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxvendor, "VU+");
	strcpy(caps.boxname, "SOLO4K");
	strcpy(caps.boxarch, "BCM7376");
#endif
#if BOXMODEL_VUDUO4K
	initialized = 1;
	caps.has_CI = 2;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 480;
	caps.display_yres = 320;
	caps.display_type = HW_DISPLAY_GFX;
	caps.display_can_deepstandby = 0;	// 0 because we use graphlcd/lcd4linux
	caps.display_can_set_brightness = 0;	// 0 because we use graphlcd/lcd4linux
	caps.display_has_statusline = 0;	// 0 because we use graphlcd/lcd4linux
	caps.display_has_colon = 0;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 2;
	strcpy(caps.startup_file, "STARTUP");
	caps.has_HDMI_input = 1;
	strcpy(caps.boxvendor, "VU+");
	strcpy(caps.boxname, "DUO4K");
	strcpy(caps.boxarch, "BCM7278");
#endif
#if BOXMODEL_VUDUO4KSE
	initialized = 1;
	caps.has_CI = 2;
	caps.can_cec = 1;
	caps.can_shutdown = 1;
	caps.display_xres = 480;
	caps.display_yres = 320;
	caps.display_type = HW_DISPLAY_GFX;
	caps.display_can_deepstandby = 0;	// 0 because we use graphlcd/lcd4linux
	caps.display_can_set_brightness = 0;	// 0 because we use graphlcd/lcd4linux
	caps.display_has_statusline = 0;	// 0 because we use graphlcd/lcd4linux
	caps.display_has_colon = 0;
	caps.has_button_timer = 1;
	caps.has_HDMI = 2;
	strcpy(caps.startup_file, "STARTUP");
	caps.has_HDMI_input = 1;
	strcpy(caps.boxvendor, "VU+");
	strcpy(caps.boxname, "DUO4KSE");
	strcpy(caps.boxarch, "BCM7444S");
#endif
#if BOXMODEL_VUULTIMO4K
	initialized = 1;
	caps.has_CI = 2;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 800;
	caps.display_yres = 480;
	caps.display_type = HW_DISPLAY_GFX;
	caps.display_can_deepstandby = 0;	// 0 because we use graphlcd/lcd4linux
	caps.display_can_set_brightness = 0;	// 0 because we use graphlcd/lcd4linux
	caps.display_has_statusline = 0;	// 0 because we use graphlcd/lcd4linux
	caps.display_has_colon = 0;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 2;
	strcpy(caps.startup_file, "STARTUP");
	caps.has_HDMI_input = 1;
	strcpy(caps.boxvendor, "VU+");
	strcpy(caps.boxname, "ULTIMO4K");
	strcpy(caps.boxarch, "BCM7444S");
#endif
#if BOXMODEL_VUZERO4K
	initialized = 1;
	caps.has_CI = 1;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_type = HW_DISPLAY_LED_ONLY;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 0;
	caps.display_has_colon = 0;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxvendor, "VU+");
	strcpy(caps.boxname, "ZERO4K");
	strcpy(caps.boxarch, "BCM72604");
#endif
#if BOXMODEL_VUUNO4KSE
	initialized = 1;
	caps.has_CI = 1;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 400;
	caps.display_yres = 240;
	caps.display_type = HW_DISPLAY_GFX;
	caps.display_can_deepstandby = 0;	// 0 because we use graphlcd/lcd4linux
	caps.display_can_set_brightness = 0;	// 0 because we use graphlcd/lcd4linux
	caps.display_has_statusline = 0;	// 0 because we use graphlcd/lcd4linux
	caps.display_has_colon = 0;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 2;
	strcpy(caps.startup_file, "STARTUP");
	caps.has_HDMI_input = 1;
	strcpy(caps.boxvendor, "VU+");
	strcpy(caps.boxname, "UNO4KSE");
	strcpy(caps.boxarch, "BCM7252S");
#endif
#if BOXMODEL_VUUNO4K
	initialized = 1;
	caps.has_CI = 1;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_type = HW_DISPLAY_LED_ONLY;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 0;
	caps.display_has_colon = 0;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxvendor, "VU+");
	strcpy(caps.boxname, "UNO4K");
	strcpy(caps.boxarch, "BCM7252S");
#endif
#if BOXMODEL_HD51
	initialized = 1;
	caps.has_CI = 1;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 16;
	caps.display_type = HW_DISPLAY_LINE_TEXT;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 0;
	caps.display_has_colon = 0;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxvendor, "AX-Technologies");
	strcpy(caps.boxname, "HD51");
	strcpy(caps.boxarch, "BCM7251S");
#endif
#if BOXMODEL_MULTIBOX
	initialized = 1;
	caps.has_CI = 0;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 0;
	caps.display_type = HW_DISPLAY_NONE;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 0;
	caps.display_has_statusline = 0;
	caps.display_has_colon = 1;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP_LINUX");
	strcpy(caps.boxvendor, "Maxytec");
	strcpy(caps.boxname, "MULTIBOX");
	strcpy(caps.boxarch, "HI3798M");
#endif
#if BOXMODEL_MULTIBOXSE
	initialized = 1;
	caps.has_CI = 0;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 0;
	caps.display_type = HW_DISPLAY_NONE;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 0;
	caps.display_has_statusline = 0;
	caps.display_has_colon = 1;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP_LINUX");
	strcpy(caps.boxvendor, "Maxytec");
	strcpy(caps.boxname, "MULTIBOXSE");
	strcpy(caps.boxarch, "HI3798M");
#endif
#if BOXMODEL_HD60
	initialized = 1;
	caps.has_CI = 0;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 4;
	caps.display_type = HW_DISPLAY_LED_NUM;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 0;
	caps.display_has_colon = 1;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP_LINUX");
	strcpy(caps.boxvendor, "AX-Technologies");
	strcpy(caps.boxname, "HD60");
	strcpy(caps.boxarch, "HI3798M");
#endif
#if BOXMODEL_HD61
	initialized = 1;
	caps.has_CI = 2;
	caps.can_cec = 1;
	caps.can_shutdown = 1;
	caps.display_xres = 4;
	caps.display_type = HW_DISPLAY_LED_NUM;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 0;
	caps.display_has_colon = 1;
	caps.has_button_timer = 1;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP_LINUX");
	strcpy(caps.boxvendor, "AX-Technologies");
	strcpy(caps.boxname, "HD61");
	strcpy(caps.boxarch, "HI3798M");
#endif
#if BOXMODEL_BRE2ZE4K
	initialized = 1;
	caps.has_CI = 1;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 4;
	caps.display_type = HW_DISPLAY_LED_NUM;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 0;
	caps.display_has_colon = 0;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 1;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxvendor, "WWIO");
	strcpy(caps.boxname, "BRE2ZE4K");
	strcpy(caps.boxarch, "BCM7251S");
#endif
#if BOXMODEL_OSMINI4K
	initialized = 1;
	caps.has_CI = 0;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 4;
	caps.display_type = HW_DISPLAY_LED_NUM;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 1;
	caps.display_has_colon = 1;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 1;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxvendor, "Edision");
	strcpy(caps.boxname, "OS mini 4K");
	strcpy(caps.boxarch, "BCM72604");
#endif
#if BOXMODEL_OSMIO4K
	initialized = 1;
	caps.has_CI = 0;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 4;
	caps.display_type = HW_DISPLAY_LED_NUM;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 1;
	caps.display_has_colon = 1;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 1;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxvendor, "Edision");
	strcpy(caps.boxname, "OS mio 4K");
	strcpy(caps.boxarch, "BCM72604");
#endif
#if BOXMODEL_OSMIO4KPLUS
	initialized = 1;
	caps.has_CI = 0;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 128;
	caps.display_yres = 32;
	caps.display_type = HW_DISPLAY_GFX;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 0;
	caps.display_has_colon = 0;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 1;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxvendor, "Edision");
	strcpy(caps.boxname, "OS mio+ 4K");
	strcpy(caps.boxarch, "BCM72604");
#endif
#if BOXMODEL_H7
	initialized = 1;
	caps.has_CI = 1;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 4;
	caps.display_type = HW_DISPLAY_LED_NUM;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 0;
	caps.display_has_colon = 1;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxvendor, "AirDigital");
	strcpy(caps.boxname, "Zgemma H7C/S");
	strcpy(caps.boxarch, "BCM7251S");
#endif
#if BOXMODEL_SF8008
	initialized = 1;
	caps.has_CI = 0;
	caps.can_cec = 1;
	caps.can_shutdown = 1;
	caps.display_xres = 4;
	caps.display_type = HW_DISPLAY_LED_NUM;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 0;
	caps.display_has_colon = 1;
	caps.has_button_timer = 1;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxvendor, "Octagon");
	strcpy(caps.boxname, "SF8008");
	strcpy(caps.boxarch, "HI3798M");
#endif
#if BOXMODEL_SF8008M
	initialized = 1;
	caps.has_CI = 0;
	caps.can_cec = 1;
	caps.can_shutdown = 1;
	caps.display_xres = 4;
	caps.display_type = HW_DISPLAY_LED_NUM;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 0;
	caps.display_has_colon = 1;
	caps.has_button_timer = 1;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxvendor, "Octagon");
	strcpy(caps.boxname, "SF8008M");
	strcpy(caps.boxarch, "HI3798M");
#endif
#if BOXMODEL_USTYM4KPRO
	initialized = 1;
	caps.has_CI = 0;
	caps.can_cec = 1;
	caps.can_shutdown = 1;
	caps.display_xres = 4;
	caps.display_type = HW_DISPLAY_LED_NUM;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 0;
	caps.display_has_colon = 1;
	caps.has_button_timer = 1;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxvendor, "Uclan");
	strcpy(caps.boxname, "Ustym 4K Pro");
	strcpy(caps.boxarch, "HI3798M");
#endif
#if BOXMODEL_USTYM4KS2OTTX
	initialized = 1;
	caps.has_CI = 0;
	caps.can_cec = 1;
	caps.can_shutdown = 1;
	caps.display_xres = 4;
	caps.display_type = HW_DISPLAY_LED_NUM;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 0;
	caps.display_has_colon = 1;
	caps.has_button_timer = 1;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxvendor, "Uclan");
	strcpy(caps.boxname, "Ustym 4K S2 OTT X");
	strcpy(caps.boxarch, "Hi3798MV300H");
#endif
#if BOXMODEL_H9COMBO
	initialized = 1;
	caps.has_CI = 1;
	if (access("/dev/ci1", F_OK) != -1)
		caps.has_CI = 2;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 4;
	caps.display_type = HW_DISPLAY_LED_NUM;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 0;
	caps.display_has_colon = 1;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP_LINUX");
	strcpy(caps.boxvendor, "AirDigital");
	strcpy(caps.boxname, "Zgemma H9Combo/Twin");
	strcpy(caps.boxarch, "HI3798M");
#endif
#if BOXMODEL_H9
	initialized = 1;
	caps.has_CI = 0;
	caps.can_cec = 1;
	caps.can_cpufreq = 0;
	caps.can_shutdown = 1;
	caps.display_xres = 4;
	caps.display_type = HW_DISPLAY_NONE;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 0;
	caps.display_has_colon = 1;
	caps.has_button_timer = 1;
	caps.has_button_vformat = 0;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP_LINUX");
	strcpy(caps.boxvendor, "AirDigital");
	strcpy(caps.boxname, "Zgemma H9s/H9.2s");
	strcpy(caps.boxarch, "HI3798M");
#endif
#if BOXMODEL_E4HDULTRA
	initialized = 1;
	caps.has_CI = 1;
	caps.can_cec = 1;
	caps.can_shutdown = 1;
	caps.display_xres = 220;
	caps.display_yres = 176;
	caps.display_type = HW_DISPLAY_GFX;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 0;
	caps.display_has_colon = 0;
	caps.has_button_timer = 1;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxvendor, "AXAS");
	strcpy(caps.boxname, "E4HD 4K ULTRA");
	strcpy(caps.boxarch, "BCM7252S");
#endif
#if BOXMODEL_PROTEK4K
	initialized = 1;
	caps.has_CI = 1;
	caps.can_cec = 1;
	caps.can_shutdown = 1;
	caps.display_xres = 220;
	caps.display_yres = 176;
	caps.display_type = HW_DISPLAY_GFX;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 0;
	caps.display_has_colon = 0;
	caps.has_button_timer = 1;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxvendor, "PROTEK");
	strcpy(caps.boxname, "PROTEK 4K");
	strcpy(caps.boxarch, "BCM7252S");
#endif
#if BOXMODEL_GBUE4K
	initialized = 1;
	caps.has_CI = 1;
	caps.can_cec = 1;
	caps.can_shutdown = 1;
	caps.display_xres = 220;
	caps.display_yres = 176;
	caps.display_type = HW_DISPLAY_GFX;
	caps.display_can_deepstandby = 0;
	caps.display_can_set_brightness = 1;
	caps.display_has_statusline = 0;
	caps.display_has_colon = 0;
	caps.has_button_timer = 1;
	caps.has_HDMI = 1;
	strcpy(caps.startup_file, "STARTUP");
	strcpy(caps.boxvendor, "GIGABLUE");
	strcpy(caps.boxname, "UHD UE 4K");
	strcpy(caps.boxarch, "BCM7252S");
#endif
	return &caps;
}
