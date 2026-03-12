/*
  Maelstrom particle effects system
  Adds visual flair: explosion sparks, thrust exhaust, bullet trails
*/

#ifndef _particles_h
#define _particles_h

#include <SDL3/SDL.h>

class FrameBuf;

/* Maximum particles alive at once */
#define MAX_PARTICLES 512

/* Particle types determine behavior and appearance */
enum ParticleType {
	PARTICLE_SPARK,		/* Explosion sparks - bright, fast, short-lived */
	PARTICLE_EMBER,		/* Explosion embers - slower, fade to red */
	PARTICLE_THRUST,	/* Engine exhaust - blue/white, medium life */
	PARTICLE_DEBRIS,	/* Small debris chunks */
	PARTICLE_STAR_STREAK,	/* Star streaking during thrust */
};

struct Particle {
	bool alive;
	ParticleType type;
	float x, y;		/* Position in render coordinates */
	float vx, vy;		/* Velocity in pixels/frame */
	float life;		/* Remaining life (1.0 = full, 0.0 = dead) */
	float decay;		/* Life lost per frame */
	float size;		/* Current size in pixels */
	Uint8 r, g, b;		/* Base color */
	bool additive;		/* Use additive blending */
};

class ParticleSystem {
public:
	ParticleSystem();

	/* Spawn effects at game coordinates (pre-GetRenderCoordinates) */
	void SpawnExplosion(int gameX, int gameY, int count, bool large);
	void SpawnThrust(int gameX, int gameY, int shipPhase);
	void SpawnBulletTrail(int gameX, int gameY);
	void SpawnShieldHit(int gameX, int gameY);

	/* Called each game tick to age/move particles */
	void Update();

	/* Called during draw to render all particles */
	void Draw(FrameBuf *screen);

	/* Clear all particles (e.g. on wave transition) */
	void Clear();

private:
	Particle particles[MAX_PARTICLES];
	int nextParticle;

	/* Convert game coords to render coords (mirrors GetRenderCoordinates) */
	void GameToRender(int gameX, int gameY, float &rx, float &ry);

	/* Find a free particle slot */
	Particle *Alloc();
};

/* Global particle system instance */
extern ParticleSystem *gParticles;

/* Call once at init and cleanup */
void InitParticles();
void FreeParticles();

#endif /* _particles_h */
