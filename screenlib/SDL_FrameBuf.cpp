/*
  screenlib:  A simple window and UI library based on the SDL library
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "../utils/files.h"
#include "SDL_FrameBuf.h"


#define LOWER_PREC(X)	((X)/16)	/* Lower the precision of a value */
#define RAISE_PREC(X)	((X)/16)	/* Raise the precision of a value */

#define MIN(A, B)	((A < B) ? A : B)
#define MAX(A, B)	((A > B) ? A : B)

/* Constructors cannot fail. :-/ */
FrameBuf::FrameBuf() : ErrorBase()
{
}

int
FrameBuf::Init(int width, int height, Uint32 window_flags, SDL_Surface *icon)
{
	int window_width = width;
	int window_height = height;

	// Enlarge the window so it's easy to see
	if (!(window_flags & SDL_WINDOW_FULLSCREEN)) {
		const SDL_DisplayMode *mode = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
		if (mode) {
			while ((window_width * 3) < mode->w && (window_height * 3) < mode->h) {
				window_width *= 2;
				window_height *= 2;
			}
		}
	}

	/* Create the window */
	m_window = SDL_CreateWindow(NULL, window_width, window_height, window_flags);
	if (!m_window) {
		SetError("Couldn't create %dx%d window: %s", 
					width, height, SDL_GetError());
		return(-1);
	}

	/* Set the icon, if any */
	if ( icon ) {
		SDL_SetWindowIcon(m_window, icon);
	}

	/* Create the renderer */
	m_renderer = SDL_CreateRenderer(m_window, NULL);
	if (!m_renderer) {
		SetError("Couldn't create renderer: %s", SDL_GetError());
		return(-1);
	}

	/* Set the output area */
	SDL_SetRenderLogicalPresentation(m_renderer, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX);

	m_width = width;
	m_height = height;
	m_clip.x = 0.0f;
	m_clip.y = 0.0f;
	m_clip.w = (float)width;
	m_clip.h = (float)height;

	/* Create the render target */
	m_target = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_UNKNOWN, SDL_TEXTUREACCESS_TARGET, width, height);
	if (!m_target) {
		SetError("Couldn't create target: %s", SDL_GetError());
		return(-1);
	}
	//SDL_SetTextureScaleMode(m_target, SDL_SCALEMODE_PIXELART);

	SDL_SetRenderTarget(m_renderer, m_target);

	/* Create bloom render targets at half and quarter resolution */
	m_bloomHalf = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, width / 2, height / 2);
	m_bloomQuarter = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, width / 4, height / 4);
	if (m_bloomHalf) {
		SDL_SetTextureBlendMode(m_bloomHalf, SDL_BLENDMODE_BLEND);
		SDL_SetTextureScaleMode(m_bloomHalf, SDL_SCALEMODE_LINEAR);
	}
	if (m_bloomQuarter) {
		SDL_SetTextureBlendMode(m_bloomQuarter, SDL_BLENDMODE_BLEND);
		SDL_SetTextureScaleMode(m_bloomQuarter, SDL_SCALEMODE_LINEAR);
	}

	/* Create scanline and vignette overlay textures */
	CreatePostProcessTextures();

	return(0);
}

FrameBuf::~FrameBuf()
{
	if (m_vignetteTex) {
		SDL_DestroyTexture(m_vignetteTex);
	}
	if (m_scanlineTex) {
		SDL_DestroyTexture(m_scanlineTex);
	}
	if (m_bloomQuarter) {
		SDL_DestroyTexture(m_bloomQuarter);
	}
	if (m_bloomHalf) {
		SDL_DestroyTexture(m_bloomHalf);
	}
	if (m_target) {
		SDL_DestroyTexture(m_target);
	}
	if (m_renderer) {
		SDL_DestroyRenderer(m_renderer);
	}
	if (m_window) {
		SDL_DestroyWindow(m_window);
	}
}

void
FrameBuf::ProcessEvent(SDL_Event *event)
{
	SDL_ConvertEventToRenderCoordinates(m_renderer, event);
}

bool
FrameBuf::ConvertTouchCoordinates(const SDL_TouchFingerEvent &finger, int *x, int *y)
{
	int w, h;

	SDL_GetRenderOutputSize(m_renderer, &w, &h);
	*x = (int)(finger.x * w);
	*y = (int)(finger.y * h);
	return true;
}

#ifdef __IPHONEOS__
extern "C" {
	extern int SDL_iPhoneKeyboardHide(SDL_Window * window);
	extern int SDL_iPhoneKeyboardShow(SDL_Window * window);
}
#endif

void
FrameBuf::GetCursorPosition(int *x, int *y)
{
	float mouse_x, mouse_y;

	SDL_GetMouseState(&mouse_x, &mouse_y);
	SDL_RenderCoordinatesFromWindow(m_renderer, mouse_x, mouse_y, &mouse_x, &mouse_y);

	*x = (int)mouse_x;
	*y = (int)mouse_y;
}

void
FrameBuf::EnableTextInput()
{
	SDL_StartTextInput(m_window);
}

void
FrameBuf::DisableTextInput()
{
	SDL_StopTextInput(m_window);
}

void
FrameBuf::QueueBlit(SDL_Texture *src,
			int srcx, int srcy, int srcw, int srch,
			int dstx, int dsty, int dstw, int dsth, clipval do_clip,
			float angle)
{
	SDL_FRect srcrect;
	SDL_FRect dstrect;

	srcrect.x = (float)srcx;
	srcrect.y = (float)srcy;
	srcrect.w = (float)srcw;
	srcrect.h = (float)srch;
	dstrect.x = (float)dstx;
	dstrect.y = (float)dsty;
	dstrect.w = (float)dstw;
	dstrect.h = (float)dsth;
	if (do_clip == DOCLIP) {
		float scaleX = srcrect.w / dstrect.w;
		float scaleY = srcrect.h / dstrect.h;

		if (!SDL_GetRectIntersectionFloat(&m_clip, &dstrect, &dstrect)) {
			return;
		}

		/* Adjust the source rectangle to match */
		srcrect.x += ((dstrect.x - dstx) * scaleX);
		srcrect.y += ((dstrect.y - dsty) * scaleY);
		srcrect.w = (dstrect.w * scaleX);
		srcrect.h = (dstrect.h * scaleY);
	}
	if (angle) {
		SDL_RenderTextureRotated(m_renderer, src, &srcrect, &dstrect, angle, NULL, SDL_FLIP_NONE);
	} else {
		SDL_RenderTexture(m_renderer, src, &srcrect, &dstrect);
	}
}

void
FrameBuf::StretchBlit(const SDL_Rect *_dstrect, SDL_Texture *src, const SDL_Rect *_srcrect)
{
	SDL_FRect *srcrect = NULL, cvtsrcrect;
	SDL_FRect *dstrect = NULL, cvtdstrect;

	if (_srcrect) {
		SDL_RectToFRect(_srcrect, &cvtsrcrect);
		srcrect = &cvtsrcrect;
	}

	if (_dstrect) {
		SDL_RectToFRect(_dstrect, &cvtdstrect);
		dstrect = &cvtdstrect;
	}

	SDL_RenderTexture(m_renderer, src, srcrect, dstrect);
}

void
FrameBuf::CreatePostProcessTextures()
{
	/* CRT scanline overlay: alternating transparent / semi-dark rows */
	{
		int w = m_width;
		int h = m_height;
		Uint32 *pixels = (Uint32 *)SDL_calloc(w * h, sizeof(Uint32));
		if (pixels) {
			for (int y = 0; y < h; ++y) {
				/* Every other row gets a dark tint; every 3rd row even darker */
				Uint8 alpha = 0;
				if (y % 2 == 0) {
					alpha = 60;  /* Main scanline */
				}
				if (y % 6 == 0) {
					alpha = 30;  /* Lighter sub-line for CRT phosphor feel */
				}
				Uint32 pixel = ((Uint32)alpha << 24); /* Black with alpha */
				for (int x = 0; x < w; ++x) {
					pixels[y * w + x] = pixel;
				}
			}
			m_scanlineTex = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, w, h);
			if (m_scanlineTex) {
				SDL_UpdateTexture(m_scanlineTex, NULL, pixels, w * sizeof(Uint32));
				SDL_SetTextureBlendMode(m_scanlineTex, SDL_BLENDMODE_BLEND);
			}
			SDL_free(pixels);
		}
	}

	/* Vignette overlay: radial darkening from center to edges */
	{
		int w = m_width;
		int h = m_height;
		Uint32 *pixels = (Uint32 *)SDL_calloc(w * h, sizeof(Uint32));
		if (pixels) {
			float cx = w * 0.5f;
			float cy = h * 0.5f;
			float maxDist = sqrtf(cx * cx + cy * cy);
			for (int y = 0; y < h; ++y) {
				for (int x = 0; x < w; ++x) {
					float dx = (x - cx) / cx;
					float dy = (y - cy) / cy;
					float dist = sqrtf(dx * dx + dy * dy);
					/* Smooth falloff: starts dim at 0.4 radius, full dark at edges */
					float v = (dist - 0.4f) / 0.6f;
					if (v < 0.0f) v = 0.0f;
					if (v > 1.0f) v = 1.0f;
					v = v * v; /* Quadratic falloff for smoother transition */
					Uint8 alpha = (Uint8)(v * 255.0f);
					pixels[y * w + x] = ((Uint32)alpha << 24); /* Black with alpha */
				}
			}
			m_vignetteTex = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, w, h);
			if (m_vignetteTex) {
				SDL_UpdateTexture(m_vignetteTex, NULL, pixels, w * sizeof(Uint32));
				SDL_SetTextureBlendMode(m_vignetteTex, SDL_BLENDMODE_BLEND);
			}
			SDL_free(pixels);
		}
	}
}

void
FrameBuf::Shake(float intensity)
{
	if (intensity > m_shakeIntensity) {
		m_shakeIntensity = intensity;
	}
}

void
FrameBuf::Update(void)
{
	if (m_target) {
		/* Make sure resize events are seen before drawing to the screen */
		SDL_PumpEvents();

		SDL_SetRenderTarget(m_renderer, NULL);

		SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(m_renderer);

		/* Apply screen shake offset */
		float shakeX = 0.0f, shakeY = 0.0f;
		if (m_shakeIntensity > 0.5f) {
			shakeX = ((float)(rand() % 200 - 100) / 100.0f) * m_shakeIntensity;
			shakeY = ((float)(rand() % 200 - 100) / 100.0f) * m_shakeIntensity;
			m_shakeIntensity *= m_shakeDecay;
			if (m_shakeIntensity < 0.5f) {
				m_shakeIntensity = 0.0f;
			}
		}

		/* Base destination rect (with shake offset) */
		SDL_FRect sceneDst;
		sceneDst.x = shakeX;
		sceneDst.y = shakeY;
		sceneDst.w = (float)m_width;
		sceneDst.h = (float)m_height;

		/* Render the main scene — chromatic aberration or normal */
		if (m_chromaEnabled && m_chromaOffset > 0.0f) {
			/* Chromatic aberration: split RGB channels with slight offsets */
			float off = m_chromaOffset;
			SDL_SetTextureBlendMode(m_target, SDL_BLENDMODE_ADD);

			/* Red channel — shifted left */
			SDL_SetTextureColorMod(m_target, 255, 0, 0);
			SDL_FRect rDst = sceneDst;
			rDst.x -= off;
			SDL_RenderTexture(m_renderer, m_target, NULL, &rDst);

			/* Green channel — centered */
			SDL_SetTextureColorMod(m_target, 0, 255, 0);
			SDL_RenderTexture(m_renderer, m_target, NULL, &sceneDst);

			/* Blue channel — shifted right */
			SDL_SetTextureColorMod(m_target, 0, 0, 255);
			SDL_FRect bDst = sceneDst;
			bDst.x += off;
			SDL_RenderTexture(m_renderer, m_target, NULL, &bDst);

			/* Restore color mod */
			SDL_SetTextureColorMod(m_target, 255, 255, 255);
			SDL_SetTextureBlendMode(m_target, SDL_BLENDMODE_NONE);
		} else {
			SDL_RenderTexture(m_renderer, m_target, NULL, &sceneDst);
		}

		/* Bloom post-process: downsample then composite back with additive blending */
		if (m_bloomEnabled && m_bloomHalf && m_bloomQuarter) {
			/* Step 1: Downsample m_target -> m_bloomHalf (bilinear filtering blurs) */
			SDL_SetRenderTarget(m_renderer, m_bloomHalf);
			SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
			SDL_RenderClear(m_renderer);
			SDL_SetTextureBlendMode(m_target, SDL_BLENDMODE_NONE);
			SDL_RenderTexture(m_renderer, m_target, NULL, NULL);

			/* Step 2: Downsample m_bloomHalf -> m_bloomQuarter (more blur) */
			SDL_SetRenderTarget(m_renderer, m_bloomQuarter);
			SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
			SDL_RenderClear(m_renderer);
			SDL_SetTextureBlendMode(m_bloomHalf, SDL_BLENDMODE_NONE);
			SDL_RenderTexture(m_renderer, m_bloomHalf, NULL, NULL);

			/* Step 3: Composite bloom layers back to screen with additive blending */
			SDL_SetRenderTarget(m_renderer, NULL);

			/* Quarter-res bloom (widest spread, softest glow) */
			Uint8 quarterAlpha = (Uint8)(m_bloomIntensity * 180.0f);
			SDL_SetTextureBlendMode(m_bloomQuarter, SDL_BLENDMODE_ADD);
			SDL_SetTextureAlphaMod(m_bloomQuarter, quarterAlpha);
			SDL_RenderTexture(m_renderer, m_bloomQuarter, NULL, &sceneDst);

			/* Half-res bloom (tighter glow) */
			Uint8 halfAlpha = (Uint8)(m_bloomIntensity * 100.0f);
			SDL_SetTextureBlendMode(m_bloomHalf, SDL_BLENDMODE_ADD);
			SDL_SetTextureAlphaMod(m_bloomHalf, halfAlpha);
			SDL_RenderTexture(m_renderer, m_bloomHalf, NULL, &sceneDst);

			/* Restore blend modes */
			SDL_SetTextureBlendMode(m_target, SDL_BLENDMODE_NONE);
			SDL_SetTextureAlphaMod(m_bloomQuarter, 0xFF);
			SDL_SetTextureAlphaMod(m_bloomHalf, 0xFF);
		}

		/* CRT scanline overlay */
		if (m_scanlinesEnabled && m_scanlineTex) {
			SDL_SetTextureAlphaMod(m_scanlineTex, (Uint8)(m_scanlineOpacity * 255.0f));
			SDL_RenderTexture(m_renderer, m_scanlineTex, NULL, &sceneDst);
			SDL_SetTextureAlphaMod(m_scanlineTex, 0xFF);
		}

		/* Vignette overlay */
		if (m_vignetteEnabled && m_vignetteTex) {
			SDL_SetTextureAlphaMod(m_vignetteTex, (Uint8)(m_vignetteStrength * 255.0f));
			SDL_RenderTexture(m_renderer, m_vignetteTex, NULL, &sceneDst);
			SDL_SetTextureAlphaMod(m_vignetteTex, 0xFF);
		}

		SDL_RenderPresent(m_renderer);

		SDL_SetRenderTarget(m_renderer, m_target);
	} else {
		SDL_RenderPresent(m_renderer);
	}
}

void
FrameBuf::Fade(void)
{
	const int max = 32;
	Uint8 value;

	for ( int i = 1; i <= max; ++i ) {
		int v = m_faded ? i : max - i;
		value = (Uint8)(255 * v / max);
		SDL_SetTextureColorMod(m_target, value, value, value);
		Update();
		SDL_Delay(10);
	}
	m_faded = !m_faded;
} 

int
FrameBuf::ScreenDump(const char *prefix, int x, int y, int w, int h)
{
	SDL_Rect rect;
	SDL_Surface *dump;
	char file[1024];
	int retval = -1;

	if (!w) {
		w = Width();
	}
	if (!h) {
		h = Height();
	}

	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
	dump = SDL_RenderReadPixels(m_renderer, &rect);
	if (!dump) {
		SetError("%s", SDL_GetError());
		return -1;
	}

	/* Get a suitable new filename */
	SDL_Storage *storage = OpenUserStorage();
	if (storage) {
		bool available = false;
		for ( int which = 0; !available; ++which ) {
			SDL_snprintf(file, sizeof(file), "%s%d.png", prefix, which);
			if (!SDL_GetStorageFileSize(storage, file, NULL)) {
				available = true;
			}
		}
		SDL_CloseStorage(storage);
	}

	SDL_IOStream *fp = SDL_IOFromDynamicMem();
	if (SDL_SavePNG_IO(dump, fp, false)) {
		if (SaveUserFile(file, fp)) {
			retval = 0;
		}
	} else {
		/* Couldn't save the screenshot */
		SDL_CloseIO(fp);
	}
	if (retval < 0) {
		SetError("%s", SDL_GetError());
	}
	SDL_DestroySurface(dump);

	return(retval);
}

SDL_Texture *
FrameBuf::LoadImage(const char *file)
{
	SDL_Surface *surface;
	SDL_Texture *texture;
	
	texture = NULL;
	surface = SDL_LoadBMP_IO(OpenRead(file), 1);
	if (surface) {
		texture = LoadImage(surface);
		SDL_DestroySurface(surface);
	}
	return texture;
}

SDL_Texture *
FrameBuf::LoadImage(SDL_Surface *surface)
{
	return SDL_CreateTextureFromSurface(m_renderer, surface);
}

SDL_Texture *
FrameBuf::LoadImage(int w, int h, Uint32 *pixels)
{
	SDL_Texture *texture;

	texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, w, h);
	if (!texture) {
		SetError("%s", SDL_GetError());
		return NULL;
	}

	if (!SDL_UpdateTexture(texture, NULL, pixels, w*sizeof(Uint32))) {
		SetError("%s", SDL_GetError());
		SDL_DestroyTexture(texture);
		return NULL;
	}
	return(texture);
}

void
FrameBuf::FreeImage(SDL_Texture *image)
{
	SDL_DestroyTexture(image);
}

