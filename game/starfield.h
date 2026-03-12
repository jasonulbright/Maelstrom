/*
  Maelstrom parallax starfield system
  Replaces the flat single-layer starfield with depth layers
*/

#ifndef _starfield_h
#define _starfield_h

#include <SDL3/SDL.h>

class FrameBuf;

#define STAR_LAYERS      3
#define STARS_PER_LAYER  40
#define MAX_NEBULAE      5

struct ParallaxStar {
	float x, y;
	float speed;        /* Drift speed (pixels/frame) */
	float brightness;   /* 0.0-1.0 */
	float twinklePhase; /* For subtle brightness variation */
	float twinkleSpeed;
	int   size;         /* 1 or 2 pixels */
	Uint8 r, g, b;
};

struct Nebula {
	float x, y;
	float vx, vy;
	float radius;
	float alpha;
	Uint8 r, g, b;
};

class Starfield {
public:
	Starfield();

	/* Call each tick to drift stars and animate */
	void Update();

	/* Draw all layers behind sprites */
	void Draw(FrameBuf *screen);

private:
	ParallaxStar layers[STAR_LAYERS][STARS_PER_LAYER];
	Nebula nebulae[MAX_NEBULAE];

	void InitLayer(int layer, float minSpeed, float maxSpeed,
	               float minBright, float maxBright, int maxSize);
	void InitNebulae();
	void WrapStar(ParallaxStar &s);
};

/* Global starfield instance */
extern Starfield *gStarfield;

void InitStarfield();
void FreeStarfield();

#endif /* _starfield_h */
