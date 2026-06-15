#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include <omp.h>
#include "raylib.h"
#include "utlist.h"

#define SWIDTH 1280
#define SHEIGHT 720
#define SCREENR 4
#define WIDTH SWIDTH * SCREENR
#define HEIGHT SHEIGHT * SCREENR
#define FPS 1200

#define PARTICLES 10000
#define PTYPES 8
#define FRICTION 0.99
#define ATTRACT 0.25
#define REPEL -ATTRACT
#define START_VEL 0
#define MAXVEL 128

#define INTERACTION 256
#define MIN 16
#define XCHUNKS (WIDTH + INTERACTION - 1) / INTERACTION
#define YCHUNKS (HEIGHT + INTERACTION - 1) / INTERACTION
#define NCHUNKS XCHUNKS * YCHUNKS

#define h (INTERACTION - sqrt(INTERACTION * (INTERACTION + 4))) / 2
#define k 2 / (INTERACTION - sqrt(INTERACTION * (INTERACTION + 4))) + 1

typedef struct particle {
	float x, y;
	float xvel, yvel;
	int id;
	int ptype;
	struct particle *next;
} particle;

typedef struct chunk {
	int x, y;
	int chunk_x, chunk_y;
	particle *contained_head;
} chunk;

typedef struct int_pair {
	int x, y;
} int_pair;

typedef struct float_pair {
	float x, y;
} float_pair;

int_pair get_chunk(int x, int y);
float euclidean_distance(float x1, float y1, float x2, float y2);
float euclidean_distance_diff(float x, float y);
float euclidean_distance2(float x1, float x2, float y1, float y2);
float uniform(float min, float max);
bool id_match(int id1, int id2);
int cheap_mod(int x, int y);
float min3f(float a, float b, float c);
void init(float attraction_matrix[PTYPES][PTYPES], chunk **chunks, particle *particles);

int main(void) {
	srand(time(NULL));
	InitWindow(SWIDTH, SHEIGHT, "particle life");
	SetTargetFPS(FPS);

	Color type_colours[PTYPES];

	for (int i = 0; i < PTYPES; i++) {
		type_colours[i] = ColorFromHSV(360. / PTYPES * i, 1, 1);
	}

	float attraction_matrix[PTYPES][PTYPES];
	chunk **chunks = malloc(YCHUNKS * sizeof(chunk*));
	particle *particles = malloc(PARTICLES * sizeof(particle));

	init(attraction_matrix, chunks, particles);

	char fpsbuf[5];

	while (!WindowShouldClose()) {
		BeginDrawing();

		ClearBackground(BLACK);

		#pragma omp parallel for
		for (int i = 0; i < PARTICLES; i++) {
			int_pair particle_chunk = get_chunk(particles[i].x, particles[i].y);
			chunk *neighbours[9];
			neighbours[0] = (particle_chunk.x > 0 && particle_chunk.y > 0) ? &chunks[particle_chunk.y - 1][particle_chunk.x - 1] : NULL;
			neighbours[1] = particle_chunk.y > 0 ? &chunks[particle_chunk.y - 1][particle_chunk.x] : NULL;
			neighbours[2] = (particle_chunk.x < XCHUNKS - 1 && particle_chunk.y > 0) ? &chunks[particle_chunk.y - 1][particle_chunk.x + 1] : NULL;
			neighbours[3] = particle_chunk.x < XCHUNKS - 1 ? &chunks[particle_chunk.y][particle_chunk.x + 1] : NULL;
			neighbours[4] = (particle_chunk.x < XCHUNKS - 1 && particle_chunk.y < YCHUNKS - 1) ? &chunks[particle_chunk.y + 1][particle_chunk.x + 1] : NULL;
			neighbours[5] = particle_chunk.y < YCHUNKS - 1 ? &chunks[particle_chunk.y + 1][particle_chunk.x] : NULL;
			neighbours[6] = (particle_chunk.x > 0 && particle_chunk.y < YCHUNKS - 1) ? &chunks[particle_chunk.y + 1][particle_chunk.x - 1] : NULL;
			neighbours[7] = particle_chunk.x > 0 ? &chunks[particle_chunk.y][particle_chunk.x - 1] : NULL;
			// neighbours[0] = &chunks[cheap_mod(particle_chunk.y - 1, YCHUNKS)][cheap_mod(particle_chunk.x - 1, XCHUNKS)];
			// neighbours[1] = &chunks[cheap_mod(particle_chunk.y - 1, YCHUNKS)][particle_chunk.x];
			// neighbours[2] = &chunks[cheap_mod(particle_chunk.y - 1, YCHUNKS)][cheap_mod(particle_chunk.x + 1, XCHUNKS)];
			// neighbours[3] = &chunks[particle_chunk.y][cheap_mod(particle_chunk.x + 1, XCHUNKS)];
			// neighbours[4] = &chunks[cheap_mod(particle_chunk.y + 1, YCHUNKS)][cheap_mod(particle_chunk.x + 1, XCHUNKS)];
			// neighbours[5] = &chunks[cheap_mod(particle_chunk.y + 1, YCHUNKS)][particle_chunk.x];
			// neighbours[6] = &chunks[cheap_mod(particle_chunk.y + 1, YCHUNKS)][cheap_mod(particle_chunk.x - 1, XCHUNKS)];
			// neighbours[7] = &chunks[particle_chunk.y][cheap_mod(particle_chunk.x - 1, XCHUNKS)];
			neighbours[8] = &chunks[particle_chunk.y][particle_chunk.x];

			for (int n = 0; n < 9; n++) {
				if (neighbours[n] != NULL) {
					particle *o_particle;
					LL_FOREACH(neighbours[n] -> contained_head, o_particle) {
						if (o_particle -> id != particles[i].id) {
							float_pair min_dist;
							// min_dist.x = min3f(o_particle -> x - particles[i].x, o_particle -> x - particles[i].x + WIDTH, o_particle -> x - particles[i].x - WIDTH);
							// min_dist.y = min3f(o_particle -> y - particles[i].y, o_particle -> y - particles[i].y + HEIGHT, o_particle -> y - particles[i].y - HEIGHT);
							min_dist.x = o_particle -> x - particles[i].x;
							min_dist.y = o_particle -> y - particles[i].y;
							// printf("%f\n", min_dist.x);
							float particle_dist = euclidean_distance_diff(min_dist.x, min_dist.y);

							if (particle_dist < INTERACTION && particle_dist != 0) {
								float angle_rads = atanf(min_dist.y / min_dist.x);
								if (isnan(angle_rads)) continue;
								float_pair force;
								force.x = fabsf(cosf(angle_rads));
								force.y = fabsf(sinf(angle_rads));
								force.x *= signbit(min_dist.x) ? -1 : 1;
								force.y *= signbit(min_dist.y) ? -1 : 1;
								force.x *= attraction_matrix[particles[i].ptype][o_particle -> ptype];
								force.y *= attraction_matrix[particles[i].ptype][o_particle -> ptype];
								// force.x *= -1. / INTERACTION * particle_dist + 1;
								// force.y *= -1. / INTERACTION * particle_dist + 1;
								if (particle_dist > MIN) {
									force.x *= 1 / (particle_dist - h) + k;
									force.y *= 1 / (particle_dist - h) + k;
								} else {
									force.x *= -REPEL / MIN * particle_dist + REPEL;
									force.y *= -REPEL / MIN * particle_dist + REPEL;
								}
								particles[i].xvel += force.x;
								particles[i].yvel += force.y;
							}
						}
					}
				}
			}
		}

		for (int i = 0; i < PARTICLES; i++) {
			particles[i].xvel *= FRICTION;
			particles[i].yvel *= FRICTION;

			particles[i].xvel = particles[i].xvel > MAXVEL ? MAXVEL : particles[i].xvel;
			particles[i].xvel = particles[i].xvel < -MAXVEL ? -MAXVEL : particles[i].xvel;
			particles[i].yvel = particles[i].yvel > MAXVEL ? MAXVEL : particles[i].yvel;
			particles[i].yvel = particles[i].yvel < -MAXVEL ? -MAXVEL : particles[i].yvel;
			int_pair prev_chunk = get_chunk(particles[i].x, particles[i].y);
			particles[i].x += particles[i].xvel;
			particles[i].y += particles[i].yvel;
			// particles[i].x = particles[i].x < 0 ? WIDTH + particles[i].x : (particles[i].x >= WIDTH ? particles[i].x - WIDTH : particles[i].x);
			// particles[i].y = particles[i].y < 0 ? HEIGHT + particles[i].y : (particles[i].y >= HEIGHT ? particles[i].y - HEIGHT : particles[i].y);
			particles[i].xvel = particles[i].x < 0 || particles[i].x >= WIDTH ? 0 : particles[i].xvel;
			particles[i].yvel = particles[i].y < 0 || particles[i].y >= HEIGHT ? 0 : particles[i].yvel;
			particles[i].x = particles[i].x < 0 ? 0 : (particles[i].x >= WIDTH ? WIDTH - 1 : particles[i].x);
			particles[i].y = particles[i].y < 0 ? 0 : (particles[i].y >= HEIGHT ? HEIGHT - 1 : particles[i].y);
			int_pair new_chunk = get_chunk(particles[i].x, particles[i].y);

			if (new_chunk.x != prev_chunk.x || new_chunk.y != prev_chunk.y) {
				if (new_chunk.y < 0 || new_chunk.y >= YCHUNKS || new_chunk.x < 0 || new_chunk.x >= XCHUNKS) {
					printf("moved outside: cx: %i\ncy: %i", new_chunk.x, new_chunk.y);
				}
				LL_DELETE(chunks[prev_chunk.y][prev_chunk.x].contained_head, &particles[i]);
				LL_PREPEND(chunks[new_chunk.y][new_chunk.x].contained_head, &particles[i]);
			}

			DrawPixel((int) (particles[i].x / SCREENR), (int) (particles[i].y / SCREENR), type_colours[particles[i].ptype]);
			int key = GetKeyPressed();
			while (key != 0) {
				if (key == KEY_R) {
					init(attraction_matrix, chunks, particles);
				}
				key = GetKeyPressed();
			}
		}

		snprintf(fpsbuf, 5, "%i", GetFPS());
		DrawText(fpsbuf, 15, 15, 20, WHITE);

		EndDrawing();
	}

	free(particles);
	for (int y = 0; y < YCHUNKS; y++) {
		free(chunks[y]);
	} free(chunks);

	CloseWindow();
	return 0;
}

int_pair get_chunk(int x, int y) {
	int_pair ret;
	ret.x = x / INTERACTION;
	ret.y = y / INTERACTION;
	ret.x = ret.x >= XCHUNKS ? ret.x - XCHUNKS : ret.x;
	ret.y = ret.y >= XCHUNKS ? ret.y - XCHUNKS : ret.y;
	return ret;
}

float euclidean_distance(float x1, float x2, float y1, float y2) {
	return sqrt(fabs((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)));
}

float euclidean_distance_diff(float x, float y) {
	return sqrt(x * x + y * y);
}

float euclidean_distance2(float x1, float x2, float y1, float y2) {
	return fabs((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

float uniform(float min, float max) {
	return fmodf((float) rand() / 16384, max - min) + min;
}

int cheap_mod(int x, int y) {
	return x < 0 ? y + x : (x >= y ? x - y : x);
}

float min3f(float a, float b, float c) {
	return fabs(a) < fabs(b) && fabs(a) < fabs(c) ? a : (fabs(b) < fabs(a) && fabs(b) < fabs(c) ? b : c);
}

void init(float attraction_matrix[PTYPES][PTYPES], chunk **chunks, particle *particles) {
	for (int y = 0; y < PTYPES; y++) {
		for (int x = 0; x < PTYPES; x++) {
			attraction_matrix[y][x] = uniform(-ATTRACT, ATTRACT);
		}
	}

	// attraction_matrix[0][0] = 0.75 * ATTRACT;
	// attraction_matrix[1][1] = 0.75 * ATTRACT;
	// attraction_matrix[0][1] = 0.75 * ATTRACT;
	// attraction_matrix[1][0] = -0.5 * ATTRACT;

	for (int y = 0; y < YCHUNKS; y++) {
		chunks[y] = malloc(XCHUNKS * sizeof(chunk));
	}

	for (int y = 0; y < YCHUNKS; y++) {
		for (int x = 0; x < XCHUNKS; x++) {
			chunks[y][x].chunk_x = x;
			chunks[y][x].chunk_y = y;

			chunks[y][x].x = x * INTERACTION;
			chunks[y][x].y = y * INTERACTION;

			chunks[y][x].contained_head = NULL;
		}
	}

	int_pair particle_chunk;

	for (int i = 0; i < PARTICLES; i++) {
		particles[i].x = rand() % WIDTH;
		particles[i].y = rand() % HEIGHT;
		if (START_VEL != 0) {
			particles[i].xvel = uniform(0, START_VEL) * 2 - START_VEL;
			particles[i].yvel = uniform(0, START_VEL) * 2 - START_VEL;
		} else {
			particles[i].xvel = 0;
			particles[i].yvel = 0;
		}
		particles[i].id = i;
		particles[i].ptype = rand() % PTYPES;

		particle_chunk = get_chunk(particles[i].x, particles[i].y);
		printf("%i\n", i);

		if (particle_chunk.y < 0 || particle_chunk.y >= YCHUNKS || particle_chunk.x < 0 || particle_chunk.x >= XCHUNKS) {
			printf("gen outside: cx: %i\ncy: %i", particle_chunk.x, particle_chunk.y);
		}

		// printf("%i\n", chunks[chunk.y][chunk.x].chunk_x - chunk.x + chunks[chunk.y][chunk.x].chunk_y - chunk.y);
		LL_PREPEND(chunks[particle_chunk.y][particle_chunk.x].contained_head, &particles[i]);
	}
}
