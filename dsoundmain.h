// This code is copyright the M.A.M.E Team (TM)

#pragma once

#ifndef DXSOUND_H
#define DXSOUND_H


#undef  DIRECTSOUND_VERSION
#define DIRECTSOUND_VERSION  0x0700
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <mmsystem.h>

#include <dsound.h>
#include "framework.h"

#ifndef WIN32
#define WIN32
#endif
#undef  WINNT
#define NONAMELESSUNION

extern LPDIRECTSOUND  dsound;

HRESULT dsound_init(int sample_rate, int channels);
void dsound_stop();
LPDIRECTSOUND get_dsound_pointer();

#endif
