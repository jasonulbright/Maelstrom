/*
  Maelstrom parallax starfield system
*/

#include <stdlib.h>
#include <math.h>

#include "starfield.h"
#include "Maelstrom_Globals.h"
#include "../screenlib/SDL_FrameBuf.h"

Starfield *gStarfield = NULL;

void InitStarfield()
{
	gStarfield = new Starfield();
}

void FreeStarfield()
{
	delete gStarfield;
	gStarfield = NULL;
}

static float RandF(float lo, float hi)
{
	return lo + (float)rand() / (float)RAND_MAX * (hi - lo);
}

Starfield::Starfield()
{
	/* Far layer: dim, slow, small */
	InitLayer(0, 0.05f, 0.15f, 0.15f, 0.35f, 1);

	/* Mid layer: medium brightness and speed */
	InitLayer(1, 0.2f, 0.5f, 0.4f, 0.7f, 1);

	/* Near layer: bright, faster, some 2px stars */
	InitLayer(2, 0.5f, 1.2f, 0.7f, 1.0f, 2);

	InitNebulae();
}

void Starfield::InitLayer(int layer, float minSpeed, float maxSpeed,
                           float minBright, float maxBright, int maxSize)
{
	for (int i = 0; i < STARS_PER_LAYER; ++i) {
		ParallaxStar &s = layers[layer][i];
		s.x = RandF(0, (float)GAME_WIDTH);
		s.y = RandF(0, (float)GAME_HEIGHT);
		s.speed = RandF(minSpeed, maxSpeed);
		s.brightness = RandF(minBright, maxBright);
		s.twinklePhase = RandF(0.0f, 6.28f);
		s.twinkleSpeed = RandF(0.02f, 0.08f);
		s.size = (maxSize > 1 && RandF(0, 1) > 0.7f) ? 2 : 1;

		/* Color variety: white, blue-white, yellow-white */
		float colorType = RandF(0, 1);
		if (colorType < 0.5f) {
			/* White */
			s.r = 255; s.g = 255; s.b = 255;
		} else if (colorType < 0.75f) {
			/* Blue-white */
			s.r = 200; s.g = 220; s.b = 255;
		} else if (colorType < 0.9f) {
			/* Yellow-white */
			s.r = 255; s.g = 240; s.b = 200;
		} else {
			/* Pale orange */
			s.r = 255; s.g = 210; s.b = 170;
		}
	}
}

void Starfield::InitNebulae()
{
	for (int i = 0; i < MAX_NEBULAE; ++i) {
		Nebula &n = nebulae[i];
		n.x = RandF(0, (float)GAME_WIDTH);
		n.y = RandF(0, (float)GAME_HEIGHT);
		n.vx = RandF(-0.05f, 0.05f);
		n.vy = RandF(0.02f, 0.08f);
		n.radius = RandF(30.0f, 80.0f);
		n.alpha = RandF(0.03f, 0.08f);

		/* Subtle colors: deep blue, purple, dark red */
		float colorType = RandF(0, 1);
		if (colorType < 0.4f) {
			n.r = 40; n.g = 60; n.b = 120;
		} else if (colorType < 0.7f) {
			n.r = 80; n.g = 40; n.b = 100;
		} else {
			n.r = 100; n.g = 30; n.b = 50;
		}
	}
}

void Starfield::WrapStar(ParallaxStar &s)
{
	if (s.y > GAME_HEIGHT) {
		s.y -= GAME_HEIGHT;
		s.x = RandF(0, (float)GAME_WIDTH);
	}
	if (s.y < 0) s.y += GAME_HEIGHT;
	if (s.x > GAME_WIDTH) s.x -= GAME_WIDTH;
	if (s.x < 0) s.x += GAME_WIDTH;
}

void Starfield::Update()
{
	/* Drift all star layers downward at different speeds */
	for (int layer = 0; layer < STAR_LAYERS; ++layer) {
		for (int i = 0; i < STARS_PER_LAYER; ++i) {
			ParallaxStar &s = layers[layer][i];
			s.y += s.speed;
			s.twinklePhase += s.twinkleSpeed;
			WrapStar(s);
		}
	}

	/* Drift nebulae */
	for (int i = 0; i < MAX_NEBULAE; ++i) {
		Nebula &n = nebulae[i];
		n.x += n.vx;
		n.y += n.vy;
		if (n.y > GAME_HEIGHT + n.radius) {
			n.y = -n.radius;
			n.x = RandF(0, (float)GAME_WIDTH);
		}
		if (n.x < -n.radius) n.x += GAME_WIDTH + n.radius * 2;
		if (n.x > GAME_WIDTH + n.radius) n.x -= GAME_WIDTH + n.radius * 2;
	}
}

void Starfield::Draw(FrameBuf *screen)
{
	/* Draw nebulae first (behind everything) */
	for (int i = 0; i < MAX_NEBULAE; ++i) {
		Nebula &n = nebulae[i];
		/* Approximate a soft blob with concentric filled rects */
		int steps = 4;
		for (int s = steps; s >= 1; --s) {
			float frac = (float)s / (float)steps;
			float r = n.radius * frac;
			float a = n.alpha * (1.0f - frac * 0.6f);
			Uint8 cr = (Uint8)(n.r * a);
			Uint8 cg = (Uint8)(n.g * a);
			Uint8 cb = (Uint8)(n.b * a);
			Uint32 color = (0xFF000000 | ((Uint32)cr << 16) | ((Uint32)cg << 8) | cb);
			int ix = (int)(n.x - r);
			int iy = (int)(n.y - r);
			int iw = (int)(r * 2);
			int ih = (int)(r * 2);
			screen->FillRect(ix, iy, iw, ih, color);
		}
	}

	/* Draw star layers (far to near) */
	for (int layer = 0; layer < STAR_LAYERS; ++layer) {
		for (int i = 0; i < STARS_PER_LAYER; ++i) {
			ParallaxStar &s = layers[layer][i];

			/* Twinkle: subtle brightness oscillation */
			float twinkle = 0.85f + 0.15f * sinf(s.twinklePhase);
			float b = s.brightness * twinkle;

			Uint8 r = (Uint8)(s.r * b);
			Uint8 g = (Uint8)(s.g * b);
			Uint8 bl = (Uint8)(s.b * b);
			Uint32 color = (0xFF000000 | ((Uint32)r << 16) | ((Uint32)g << 8) | bl);

			int ix = (int)s.x;
			int iy = (int)s.y;

			if (s.size <= 1) {
				screen->DrawPoint(ix, iy, color);
			} else {
				screen->FillRect(ix, iy, 2, 2, color);
			}
		}
	}
}
