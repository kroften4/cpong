#include "render.h"
#include "cpong_logic.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

void draw_obj_debug(struct game_obj obj, SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderLine(renderer, obj.pos.x, obj.pos.y, obj.pos.x + obj.velocity.x * 100,
                   obj.pos.y + obj.velocity.y * 100);
}

void draw_obj(struct game_obj obj, SDL_Renderer *renderer, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_FRect rect = {obj.pos.x - obj.size.x / 2.0f,
        obj.pos.y - obj.size.y / 2.0f, obj.size.x, obj.size.y};
    SDL_RenderFillRect(renderer, &rect);
#ifdef DEBUG
    draw_obj_debug(obj, renderer);
#endif
}

void draw_score(struct pong_state state, SDL_Renderer *renderer, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    int score_diff = state.score[1] - state.score[0];
    SDL_FRect rect = {state.box_size.x / 2, 0, score_diff * 10, state.box_size.y};
    SDL_RenderFillRect(renderer, &rect);
}

