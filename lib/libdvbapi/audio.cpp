#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <linux/dvb/audio.h>

#include <hardware_caps.h>
#include <proc_tools.h>

#include "audio.h"
#include "debug.h"
#include <config.h>

#define dvbapi_debug(args...) _dvbapi_debug(DVBAPI_DEBUG_AUDIO, this, args)
#define dvbapi_info(args...) _dvbapi_info(DVBAPI_DEBUG_AUDIO, this, args)

#define fop(cmd, args...) ({				\
		int _r;						\
		if (fd >= 0) { 					\
			if ((_r = ::cmd(fd, args)) < 0)		\
				dvbapi_info(#cmd"(fd, "#args")\n");\
			else					\
				dvbapi_debug(#cmd"(fd, "#args")\n");\
		}						\
		else { _r = fd; } 				\
		_r;						\
	})

#include <linux/soundcard.h>

enum
{
	ENCODER,
	AUX
};

cAudio *audioDecoder = NULL;
cAudio *pipAudioDecoder[3] = { NULL, NULL, NULL };

static const char *ADEV[] =
{
	"/dev/dvb/adapter0/audio0",
	"/dev/dvb/adapter0/audio1",
	"/dev/dvb/adapter0/audio2",
	"/dev/dvb/adapter0/audio3"
};

cAudio::cAudio(void *, void *, void *, unsigned int unit)
{
	hw_caps_t *hwcaps = get_hwcaps();
	if (unit > (unsigned int) hwcaps->pip_devs)
	{
		dvbapi_info("%s: unit %d out of range, setting to 0\n", __func__, unit);
		devnum = 0;
	}
	else
		devnum = unit;
	fd = -1;
	fdd = false;
	clipfd = -1;
	mixer_fd = -1;
	openDevice();
	Muted = false;
}

cAudio::~cAudio(void)
{
	closeDevice();
}

void cAudio::openDevice(void)
{
	if (fd < 0)
	{
		if ((fd = open(ADEV[devnum], O_RDWR)) < 0)
			dvbapi_info("openDevice: open failed (%m)\n");
		fcntl(fd, F_SETFD, FD_CLOEXEC);
		//do_mute(true, false);
	}
	else
		dvbapi_info("openDevice: already open (fd = %d)\n", fd);
}

void cAudio::closeDevice(void)
{
	if (fd > -1)
	{
		close(fd);
		fd = -1;
	}
	if (clipfd > -1)
	{
		close(clipfd);
		clipfd = -1;
	}
	if (mixer_fd > -1)
	{
		close(mixer_fd);
		mixer_fd = -1;
	}
}

#ifndef AUDIO_SOURCE_HDMI
#define AUDIO_SOURCE_HDMI 2
#endif

void cAudio::open_AVInput_Device(void)
{
	dvbapi_debug("%s\n", __func__);

	if (fdd) /* already open */
		return;

	fop(ioctl, AUDIO_SELECT_SOURCE, AUDIO_SOURCE_HDMI);
	fop(ioctl, AUDIO_PLAY);
	fdd = true;
}

void cAudio::close_AVInput_Device(void)
{
	dvbapi_debug("%s\n", __func__);

	if (fdd)
	{
		fop(ioctl, AUDIO_STOP);
	}
	fdd = false;
}

void cAudio::setAVInput(int val)
{
	dvbapi_info("%s - switching to: %s\n", __func__, val == AUX ? "AUX" : "ENCODER");

	if (val == AUX)
	{
		Stop();
		open_AVInput_Device();
	}
	else
	{
		if (fdd)
		{
			close_AVInput_Device();
			fop(ioctl, AUDIO_SELECT_SOURCE, AUDIO_SOURCE_DEMUX);
			Start();
		}
	}
}

int cAudio::do_mute(bool enable, bool remember)
{
	dvbapi_debug("%s(%d, %d)\n", __FUNCTION__, enable, remember);

	char str[4];

	if (remember)
		Muted = enable;

	sprintf(str, "%d", Muted);
	proc_put("/proc/stb/audio/j1_mute", str, strlen(str));

	if (fd < 1)
		openDevice();

	if (fd > 0)
	{
		if (ioctl(fd, AUDIO_SET_MUTE, enable) < 0)
			perror("AUDIO_SET_MUTE");
	}

	return 0;
}

int map_volume(const int volume)
{
	unsigned char vol = volume;
	if (vol > 100)
		vol = 100;

#if !BOXMODEL_SF8008 && !BOXMODEL_SF8008M && !BOXMODEL_USTYM4KPRO && !BOXMODEL_USTYM4KS2OTTX
	// convert to -1dB steps
	vol = 63 - vol * 63 / 100;
	// now range is 63..0, where 0 is loudest
#endif

#if BOXMODEL_VUPLUS_ALL
	if (vol == 63)
		vol = 255;
#endif

	return vol;
}

int cAudio::setVolume(unsigned int left, unsigned int right)
{
	dvbapi_info("cAudio::%s(%d, %d)\n", __func__, left, right);

	volume = (left + right) / 2;
	int v = map_volume(volume);

	left = map_volume(volume);
	right = map_volume(volume);

	audio_mixer_t mixer;

	mixer.volume_left = left;
	mixer.volume_right = right;

	if (fd < 1)
		openDevice();

	if (fd > 0)
	{
		if (ioctl(fd, AUDIO_SET_MIXER, &mixer) < 0)
			perror("AUDIO_SET_MIXER");
	}

	char str[4];
	sprintf(str, "%d", v);

	proc_put("/proc/stb/avs/0/volume", str, strlen(str));

	return 0;
}

int cAudio::Start(void)
{
	int ret;
	ret = ioctl(fd, AUDIO_PLAY);
#if BOXMODEL_HISILICON
	ioctl(fd, AUDIO_CONTINUE);
#endif
	return ret;
}

int cAudio::Stop(void)
{
	return ioctl(fd, AUDIO_STOP);
}

bool cAudio::Pause(bool Pcm)
{
	ioctl(fd, Pcm ? AUDIO_PAUSE : AUDIO_CONTINUE, 1);
	return true;
}

void cAudio::SetSyncMode(AVSYNC_TYPE Mode)
{
	dvbapi_debug("%s %d\n", __func__, Mode);
	ioctl(fd, AUDIO_SET_AV_SYNC, Mode);
}

void cAudio::SetStreamType(int bypass)
{
	StreamType = bypass;

	dvbapi_info("%s %d (0x%x)\n", __FUNCTION__, bypass, bypass);

	if (ioctl(fd, AUDIO_SET_BYPASS_MODE, bypass) < 0)
		dvbapi_info("%s: AUDIO_SET_BYPASS_MODE failed (%m)\n", __func__);
}

int cAudio::setChannel(int channel)
{
	dvbapi_debug("%s %d\n", __FUNCTION__, channel);
	return 0;
}

int cAudio::PrepareClipPlay(int ch, int srate, int bits, int little_endian)
{
	int fmt;
	unsigned int devmask, stereo, usable;
	const char *dsp_dev = getenv("DSP_DEVICE");
	const char *mix_dev = getenv("MIX_DEVICE");
	dvbapi_info("cAudio::%s ch %d srate %d bits %d le %d\n", __FUNCTION__, ch, srate, bits, little_endian);
	if (clipfd > -1)
	{
		dvbapi_info("%s: clipfd already opened (%d)\n", __func__, clipfd);
		return -1;
	}
	mixer_num = -1;
	mixer_fd = -1;
	/* a different DSP device can be given with DSP_DEVICE and MIX_DEVICE
	 * if this device cannot be opened, we fall back to the internal OSS device
	 * Example:
	 *   modprobe snd-usb-audio
	 *   export DSP_DEVICE=/dev/sound/dsp2
	 *   export MIX_DEVICE=/dev/sound/mixer2
	 *   neutrino
	 */
	if ((!dsp_dev) || (access(dsp_dev, W_OK)))
	{
		if (dsp_dev)
			dvbapi_info("%s: DSP_DEVICE is set (%s) but cannot be opened,"
				" fall back to /dev/dsp\n", __func__, dsp_dev);
		dsp_dev = "/dev/dsp";
	}
	if ((!mix_dev) || (access(mix_dev, W_OK)))
	{
		if (mix_dev)
			dvbapi_info("%s: MIX_DEVICE is set (%s) but cannot be opened,"
				" fall back to /dev/mixer\n", __func__, dsp_dev);
		mix_dev = "/dev/mixer";
	}
	dvbapi_info("cAudio::%s: dsp_dev %s mix_dev %s\n", __func__, dsp_dev, mix_dev); /* NULL mix_dev is ok */
	/* the tdoss dsp driver seems to work only on the second open(). really. */
	clipfd = open(dsp_dev, O_WRONLY);
	if (clipfd < 0)
	{
		dvbapi_info("%s open %s: %m\n", dsp_dev, __FUNCTION__);
		return -1;
	}
	fcntl(clipfd, F_SETFD, FD_CLOEXEC);
	/* no idea if we ever get little_endian == 0 */
	if (little_endian)
		fmt = AFMT_S16_BE;
	else
		fmt = AFMT_S16_LE;
	if (ioctl(clipfd, SNDCTL_DSP_SETFMT, &fmt))
		perror("SNDCTL_DSP_SETFMT");
	if (ioctl(clipfd, SNDCTL_DSP_CHANNELS, &ch))
		perror("SNDCTL_DSP_CHANNELS");
	if (ioctl(clipfd, SNDCTL_DSP_SPEED, &srate))
		perror("SNDCTL_DSP_SPEED");
#if !BOXMODEL_HD51 && !BOXMODEL_BRE2ZE4K && !BOXMODEL_H7
	if (ioctl(clipfd, SNDCTL_DSP_RESET))
		perror("SNDCTL_DSP_RESET");
#endif

	if (!mix_dev)
		return 0;

	mixer_fd = open(mix_dev, O_RDWR);
	if (mixer_fd < 0)
	{
		dvbapi_info("%s: open mixer %s failed (%m)\n", __func__, mix_dev);
		/* not a real error */
		return 0;
	}
	if (ioctl(mixer_fd, SOUND_MIXER_READ_DEVMASK, &devmask) == -1)
	{
		dvbapi_info("%s: SOUND_MIXER_READ_DEVMASK %m\n", __func__);
		devmask = 0;
	}
	if (ioctl(mixer_fd, SOUND_MIXER_READ_STEREODEVS, &stereo) == -1)
	{
		dvbapi_info("%s: SOUND_MIXER_READ_STEREODEVS %m\n", __func__);
		stereo = 0;
	}
	usable = devmask & stereo;
	if (usable == 0)
	{
		dvbapi_info("%s: devmask: %08x stereo: %08x, no usable dev :-(\n",
			__func__, devmask, stereo);
		close(mixer_fd);
		mixer_fd = -1;
		return 0; /* TODO: should we treat this as error? */
	}
	/* __builtin_popcount needs GCC, it counts the set bits... */
	if (__builtin_popcount(usable) != 1)
	{
		/* TODO: this code is not yet tested as I have only single-mixer devices... */
		dvbapi_info("%s: more than one mixer control: devmask %08x stereo %08x\n"
			"%s: querying MIX_NUMBER environment variable...\n",
			__func__, devmask, stereo, __func__);
		const char *tmp = getenv("MIX_NUMBER");
		if (tmp)
			mixer_num = atoi(tmp);
		dvbapi_info("%s: mixer_num is %d -> device %08x\n",
			__func__, mixer_num, (mixer_num >= 0) ? (1 << mixer_num) : 0);
		/* no error checking, you'd better know what you are doing... */
	}
	else
	{
		mixer_num = 0;
		while (!(usable & 0x01))
		{
			mixer_num++;
			usable >>= 1;
		}
	}
	setVolume(volume, volume);

	return 0;
}

int cAudio::WriteClip(unsigned char *buffer, int size)
{
	int ret, __attribute__((unused)) count = 1;
	// dvbapi_debug("cAudio::%s\n", __FUNCTION__);
	if (clipfd < 0)
	{
		dvbapi_info("%s: clipfd not yet opened\n", __FUNCTION__);
		return -1;
	}
#if BOXMODEL_HD51 || BOXMODEL_BRE2ZE4K || BOXMODEL_H7
again:
#endif
	ret = write(clipfd, buffer, size);
	if (ret < 0)
	{
		dvbapi_info("%s: write error (%m)\n", __FUNCTION__);
		return ret;
	}
#if BOXMODEL_HD51 || BOXMODEL_BRE2ZE4K || BOXMODEL_H7
	if (ret != size)
	{
		dvbapi_info("cAudio::%s: difference > to write (%d) != written (%d) try (%d) > reset dsp and restart write\n", __FUNCTION__, size, ret, count);
		if (ioctl(clipfd, SNDCTL_DSP_RESET))
			perror("SNDCTL_DSP_RESET");
		count++;
		if (count < 3)
			goto again;
	}
#endif
	return ret;
};

int cAudio::StopClip()
{
	dvbapi_info("cAudio::%s\n", __FUNCTION__);

	if (clipfd < 0)
	{
		dvbapi_info("%s: clipfd not yet opened\n", __FUNCTION__);
		return -1;
	}
#if BOXMODEL_VUPLUS_ARM
	ioctl(clipfd, SNDCTL_DSP_RESET);
#endif
	close(clipfd);
	clipfd = -1;
	if (mixer_fd > -1)
	{
		close(mixer_fd);
		mixer_fd = -1;
	}
	setVolume(volume, volume);
	return 0;
};

void cAudio::getAudioInfo(int &type, int &layer, int &freq, int &bitrate, int &mode)
{
	dvbapi_debug("%s\n", __FUNCTION__);
	type = 0;
	layer = 0;
	freq = 0;
	bitrate = 0;
	mode = 0;
#if 0
	unsigned int atype;
	static const int freq_mpg[] = {44100, 48000, 32000, 0};
	static const int freq_ac3[] = {48000, 44100, 32000, 0};
	scratchl2 i;
	if (ioctl(fd, MPEG_AUD_GET_DECTYP, &atype) < 0)
		perror("cAudio::getAudioInfo MPEG_AUD_GET_DECTYP");
	if (ioctl(fd, MPEG_AUD_GET_STATUS, &i) < 0)
		perror("cAudio::getAudioInfo MPEG_AUD_GET_STATUS");

	type = atype;
#if 0
	/* this does not work, some of the values are negative?? */
	AMPEGStatus A;
	memcpy(&A, &i.word00, sizeof(i.word00));
	layer   = A.audio_mpeg_layer;
	mode    = A.audio_mpeg_mode;
	bitrate = A.audio_mpeg_bitrate;
	switch (A.audio_mpeg_frequency)
#endif
		/* layer and bitrate are not used anyway... */
		layer   = 0; //(i.word00 >> 17) & 3;
	bitrate = 0; //(i.word00 >> 12) & 3;
	switch (type)
	{
		case 0:	/* MPEG */
			mode = (i.word00 >> 6) & 3;
			freq = freq_mpg[(i.word00 >> 10) & 3];
			break;
		case 1:	/* AC3 */
			mode = (i.word00 >> 28) & 7;
			freq = freq_ac3[(i.word00 >> 16) & 3];
			break;
		default:
			mode = 0;
			freq = 0;
	}
	//fprintf(stderr, "type: %d layer: %d freq: %d bitrate: %d mode: %d\n", type, layer, freq, bitrate, mode);
#endif
};

void cAudio::SetSRS(int /*iq_enable*/, int /*nmgr_enable*/, int /*iq_mode*/, int /*iq_level*/)
{
	dvbapi_debug("%s\n", __FUNCTION__);
};

void cAudio::SetHdmiDD(bool enable)
{
	const char *opt[] = {  "downmix", "passthrough" };
	dvbapi_debug("%s %d\n", __func__, enable);
	proc_put("/proc/stb/audio/ac3", opt[enable], strlen(opt[enable]));
}

void cAudio::SetSpdifDD(bool enable)
{
	//using this function for dts passthrough
	const char *opt[] = {  "downmix", "passthrough" };
	dvbapi_debug("%s %d\n", __func__, enable);
	proc_put("/proc/stb/audio/dts", opt[enable], strlen(opt[enable]));
}

void cAudio::ScheduleMute(bool On)
{
	dvbapi_debug("%s %d\n", __FUNCTION__, On);
}

void cAudio::EnableAnalogOut(bool enable)
{
	dvbapi_debug("%s %d\n", __FUNCTION__, enable);
}

#define AUDIO_BYPASS_ON  0
#define AUDIO_BYPASS_OFF 1
void cAudio::setBypassMode(bool disable)
{
	int mode = disable ? AUDIO_BYPASS_OFF : AUDIO_BYPASS_ON;
	if (ioctl(fd, AUDIO_SET_BYPASS_MODE, mode) < 0)
		dvbapi_info("%s AUDIO_SET_BYPASS_MODE %d: %m\n", __func__, mode);
}
