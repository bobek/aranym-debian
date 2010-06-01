/*
	Audio core

	ARAnyM (C) 2005 Patrice Mandin

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "sysdeps.h"
#include "host_audio.h"
#include "host.h"
#include "parameters.h"

#define DEBUG 0
#include "debug.h"

#include <SDL.h>

/*--- SDL callback function ---*/

extern "C" {
	static void UpdateAudio(void *unused, Uint8 *stream, int len) {
		DUNUSED(unused);

		if (!host) {
			return;
		}

		/* SDL 1.3 will require the application to clear the buffer */
		//memset(stream, 0, len);

		for (int i=0; i<MAX_AUDIO_CALLBACKS; i++) {
			if (host->audio.callbacks[i]) {
				host->audio.callbacks[i](host->audio.userdatas[i], stream, len);
			}
		}
	}
};

/*--- Constructor/destructor of HostAudio class ---*/

HostAudio::HostAudio()
{
	D(bug("HostAudio: HostAudio()"));

	for (int i=0; i<MAX_AUDIO_CALLBACKS; i++) {
		callbacks[i]=NULL;
		userdatas[i]=NULL;
	}

	desired.freq = bx_options.audio.freq;
	desired.format = (bx_options.audio.bits == 8) ? AUDIO_S8 : AUDIO_S16SYS;
	desired.channels = bx_options.audio.chans;
	desired.samples = bx_options.audio.samples;
	desired.callback = UpdateAudio;
	desired.userdata = NULL;

	if (SDL_OpenAudio(&desired, &obtained)<0) {
		fprintf(stderr,"Could not open audio: %s\n", SDL_GetError());
		return;
	}

#if DEBUG
	{
		char name[32];
		if (SDL_AudioDriverName(name, 31)) {
			D(bug("HostAudio: device %s opened", name));
		}
		D(bug("HostAudio: %d Hz, 0x%04x format, %d channels, %d samples, %d bytes",
			obtained.freq, obtained.format,
			obtained.channels, obtained.samples, obtained.size));
	}
#endif

	SDL_PauseAudio(0);
}

HostAudio::~HostAudio()
{
	D(bug("HostAudio: ~HostAudio()"));
	
	SDL_CloseAudio();
}

/*--- Public stuff ---*/

void HostAudio::reset(void)
{
/*
	SDL_LockAudio();

	for (int i=0; i<MAX_AUDIO_CALLBACKS; i++) {
		callbacks[i]=NULL;
		userdatas[i]=NULL;
	}

	SDL_UnlockAudio();
*/
}

void HostAudio::AddCallback(audio_callback_f callback, void *userdata)
{
	SDL_bool callbackAdded = SDL_FALSE;

	SDL_LockAudio();

	for (int i=0; i<MAX_AUDIO_CALLBACKS;i++) {
		if (callbacks[i]==NULL) {
			userdatas[i]=userdata;
			callbacks[i]=callback;
			callbackAdded = SDL_TRUE;
			break;
		}
	}

	SDL_UnlockAudio();

	if (!callbackAdded) {
		fprintf(stderr, "Too many audio callbacks registered\n");
		return;
	}
}

void HostAudio::RemoveCallback(audio_callback_f callback)
{
	SDL_LockAudio();

	for (int i=0; i<MAX_AUDIO_CALLBACKS;i++) {
		if (callbacks[i]==callback) {
			callbacks[i]=NULL;
			userdatas[i]=NULL;
		}
	}

	SDL_UnlockAudio();
}
