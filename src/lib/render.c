#include "render.h"
#include "cpong_logic.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

void draw_obj(struct game_obj obj, SDL_Renderer *renderer) {
    SDL_FRect rect = {obj.pos.x, obj.pos.y, obj.size.x, obj.size.y};
    SDL_RenderFillRect(renderer, &rect);
}

