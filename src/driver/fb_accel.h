/*
	Copyright (C) 2007-2013,2017-2018 Stefan Seyfried

	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.

	private functions for the fbaccel class (only used in CFrameBuffer)
*/


#ifndef __fbaccel__
#define __fbaccel__
#include <config.h>
#include <OpenThreads/Mutex>
#include <OpenThreads/ScopedLock>
#include <OpenThreads/Thread>
#include <OpenThreads/Condition>
#include "fb_generic.h"

class CFbAccel
	: public CFrameBuffer
{
	public:
		CFbAccel();
		~CFbAccel();
		void paintBoxRel(const int x, const int y, const int dx, const int dy, const fb_pixel_t col, int radius, int type);
		virtual void paintRect(const int x, const int y, const int dx, const int dy, const fb_pixel_t col);
};

#if HAVE_GENERIC_HARDWARE
class GLThreadObj;
#endif
class CFbAccelGLFB
	: public OpenThreads::Thread, public CFbAccel
{
	private:
		void run(void);
		void blit(void);
		void _blit(void);
		bool blit_thread;
		bool blit_pending;
		OpenThreads::Condition blit_cond;
		OpenThreads::Mutex blit_mutex;
		fb_pixel_t *backbuffer;
	public:
		CFbAccelGLFB();
		~CFbAccelGLFB();
		void init(const char * const);
		int setMode(unsigned int xRes, unsigned int yRes, unsigned int bpp);
		int scale2Res(int size);
		bool fullHdAvailable() { return true; };
		void setOsdResolutions();
		void blit2FB(void *fbbuff, uint32_t width, uint32_t height, uint32_t xoff, uint32_t yoff, uint32_t xp, uint32_t yp, bool transp);
		fb_pixel_t * getBackBufferPointer() const;
#if HAVE_GENERIC_HARDWARE
		GLThreadObj *mpGLThreadObj; // the thread object
#endif
};

class CFbAccelARM
#if ENABLE_ARM_ACC
	: public OpenThreads::Thread, public CFbAccel
#else
	: public CFbAccel
#endif

{
	private:
#if ENABLE_ARM_ACC
		void run(void);
		void blit(void);
		void _blit(void);
		bool blit_thread;
		bool blit_pending;
		OpenThreads::Condition blit_cond;
		OpenThreads::Mutex blit_mutex;
#endif
		fb_pixel_t *backbuffer;
	public:
		CFbAccelARM();
		~CFbAccelARM();
		fb_pixel_t * getBackBufferPointer() const;
		int setMode(unsigned int xRes, unsigned int yRes, unsigned int bpp);
		int scale2Res(int size);
		bool fullHdAvailable();
		void setOsdResolutions();
		void set3DMode(Mode3D);
		Mode3D get3DMode(void);
		void setBlendMode(uint8_t mode);
		void setBlendLevel(int level);
#if ENABLE_ARM_ACC
		void paintRect(const int x, const int y, const int dx, const int dy, const fb_pixel_t col);
#endif
};

#endif
