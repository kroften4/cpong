#ifndef _RENDER_H
#define _RENDER_H

#include "cpong_logic.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

void draw_obj(struct game_obj obj, SDL_Renderer *renderer, SDL_Color color);

void draw_score(struct pong_state state, SDL_Renderer *renderer, SDL_Color color);

#endif

