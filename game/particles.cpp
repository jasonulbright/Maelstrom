/*
  Maelstrom particle effects system
*/

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "particles.h"
#include "Maelstrom_Globals.h"
#include "../screenlib/SDL_FrameBuf.h"

ParticleSystem *gParticles = NULL;

void InitParticles()
{
	gParticles = new ParticleSystem();
}

void FreeParticles()
{
	delete gParticles;
	gParticles = NULL;
}

/* Random float in [lo, hi] */
static float RandFloat(float lo, float hi)
{
	return lo + (float)rand() / (float)RAND_MAX * (hi - lo);
}

ParticleSystem::ParticleSystem()
{
	memset(particles, 0, sizeof(particles));
	nextParticle = 0;
}

Particle *ParticleSystem::Alloc()
{
	/* Scan from nextParticle looking for a dead slot */
	for (int i = 0; i < MAX_PARTICLES; ++i) {
		int idx = (nextParticle + i) % MAX_PARTICLES;
		if (!particles[idx].alive) {
			nextParticle = (idx + 1) % MAX_PARTICLES;
			return &particles[idx];
		}
	}
	/* Pool full — recycle the oldest (nextParticle) */
	Particle *p = &particles[nextParticle];
	nextParticle = (nextParticle + 1) % MAX_PARTICLES;
	return p;
}

void ParticleSystem::GameToRender(int gameX, int gameY, float &rx, float &ry)
{
	rx = gScrnRect.x + ((float)gameX * gScrnRect.w) / (GAME_WIDTH << SPRITE_PRECISION);
	ry = gScrnRect.y + ((float)gameY * gScrnRect.h) / (GAME_HEIGHT << SPRITE_PRECISION);
}

void ParticleSystem::SpawnExplosion(int gameX, int gameY, int count, bool large)
{
	float cx, cy;
	GameToRender(gameX, gameY, cx, cy);

	/* Screen shake on explosions */
	screen->Shake(large ? 8.0f : 3.0f);

	/* Center on the sprite */
	float offset = large ? 24.0f : 16.0f;
	cx += offset;
	cy += offset;

	for (int i = 0; i < count; ++i) {
		Particle *p = Alloc();
		p->alive = true;

		/* Mix of sparks and embers */
		if (i < count / 2) {
			/* Bright sparks — fast, white/yellow */
			p->type = PARTICLE_SPARK;
			float angle = RandFloat(0.0f, 2.0f * 3.14159f);
			float speed = RandFloat(2.0f, 6.0f);
			p->vx = cosf(angle) * speed;
			p->vy = sinf(angle) * speed;
			p->life = 1.0f;
			p->decay = RandFloat(0.015f, 0.035f);
			p->size = RandFloat(3.0f, 6.0f);
			p->r = 255;
			p->g = (Uint8)RandFloat(200, 255);
			p->b = (Uint8)RandFloat(100, 200);
			p->additive = true;
		} else {
			/* Slower embers — orange/red, longer life */
			p->type = PARTICLE_EMBER;
			float angle = RandFloat(0.0f, 2.0f * 3.14159f);
			float speed = RandFloat(0.8f, 3.0f);
			p->vx = cosf(angle) * speed;
			p->vy = sinf(angle) * speed;
			p->life = 1.0f;
			p->decay = RandFloat(0.008f, 0.02f);
			p->size = RandFloat(2.0f, 5.0f);
			p->r = 255;
			p->g = (Uint8)RandFloat(80, 160);
			p->b = 0;
			p->additive = true;
		}
		p->x = cx + RandFloat(-4.0f, 4.0f);
		p->y = cy + RandFloat(-4.0f, 4.0f);
	}
}

void ParticleSystem::SpawnThrust(int gameX, int gameY, int shipPhase)
{
	float cx, cy;
	GameToRender(gameX, gameY, cx, cy);

	for (int i = 0; i < 4; ++i) {
		Particle *p = Alloc();
		p->alive = true;
		p->type = PARTICLE_THRUST;
		p->x = cx + RandFloat(-2.0f, 2.0f);
		p->y = cy + RandFloat(-2.0f, 2.0f);

		/* Exhaust drifts with some spread */
		float angle = RandFloat(0.0f, 2.0f * 3.14159f);
		float speed = RandFloat(0.8f, 2.5f);
		p->vx = cosf(angle) * speed;
		p->vy = sinf(angle) * speed;

		p->life = 1.0f;
		p->decay = RandFloat(0.03f, 0.07f);
		p->size = RandFloat(2.0f, 4.0f);
		/* Blue-white exhaust */
		p->r = (Uint8)RandFloat(150, 220);
		p->g = (Uint8)RandFloat(180, 240);
		p->b = 255;
		p->additive = true;
	}
}

void ParticleSystem::SpawnBulletTrail(int gameX, int gameY)
{
	float cx, cy;
	GameToRender(gameX, gameY, cx, cy);

	for (int i = 0; i < 2; ++i) {
		Particle *p = Alloc();
		p->alive = true;
		p->type = PARTICLE_SPARK;
		p->x = cx + RandFloat(-1.0f, 1.0f);
		p->y = cy + RandFloat(-1.0f, 1.0f);
		p->vx = RandFloat(-0.5f, 0.5f);
		p->vy = RandFloat(-0.5f, 0.5f);
		p->life = 1.0f;
		p->decay = RandFloat(0.05f, 0.10f);
		p->size = RandFloat(2.0f, 3.5f);
		p->r = 200;
		p->g = 255;
		p->b = 200;
		p->additive = true;
	}
}

void ParticleSystem::SpawnShieldHit(int gameX, int gameY)
{
	float cx, cy;
	GameToRender(gameX, gameY, cx, cy);

	for (int i = 0; i < 12; ++i) {
		Particle *p = Alloc();
		p->alive = true;
		p->type = PARTICLE_SPARK;
		float angle = RandFloat(0.0f, 2.0f * 3.14159f);
		float speed = RandFloat(1.5f, 4.0f);
		p->x = cx + cosf(angle) * 12.0f;
		p->y = cy + sinf(angle) * 12.0f;
		p->vx = cosf(angle) * speed;
		p->vy = sinf(angle) * speed;
		p->life = 1.0f;
		p->decay = RandFloat(0.03f, 0.06f);
		p->size = RandFloat(2.0f, 4.0f);
		p->r = 100;
		p->g = 200;
		p->b = 255;
		p->additive = true;
	}
}

void ParticleSystem::Update()
{
	for (int i = 0; i < MAX_PARTICLES; ++i) {
		Particle &p = particles[i];
		if (!p.alive) continue;

		p.x += p.vx;
		p.y += p.vy;
		p.life -= p.decay;

		/* Friction / drag */
		p.vx *= 0.97f;
		p.vy *= 0.97f;

		/* Embers slow down more and shrink */
		if (p.type == PARTICLE_EMBER) {
			p.vx *= 0.95f;
			p.vy *= 0.95f;
			p.size *= 0.995f;
		}

		if (p.life <= 0.0f) {
			p.alive = false;
		}
	}
}

void ParticleSystem::Draw(FrameBuf *screen)
{
	for (int i = 0; i < MAX_PARTICLES; ++i) {
		Particle &p = particles[i];
		if (!p.alive) continue;

		float alpha = p.life;
		if (alpha > 1.0f) alpha = 1.0f;

		Uint8 r = (Uint8)(p.r * alpha);
		Uint8 g = (Uint8)(p.g * alpha);
		Uint8 b = (Uint8)(p.b * alpha);

		Uint32 color = (0xFF000000 | ((Uint32)r << 16) | ((Uint32)g << 8) | b);

		int ix = (int)p.x;
		int iy = (int)p.y;

		int s = (int)(p.size + 0.5f);
		if (s < 2) s = 2;
		screen->FillRect(ix - s/2, iy - s/2, s, s, color);
	}
}

void ParticleSystem::Clear()
{
	for (int i = 0; i < MAX_PARTICLES; ++i) {
		particles[i].alive = false;
	}
}
