#ifndef BUNKERBUILDER_SDL_BASE_H
#define BUNKERBUILDER_SDL_BASE_H
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

namespace bb {
    using namespace std;

    SDL_Window *window;
    SDL_Renderer *renderer;

    SDL_Texture *LoadTexture(const string & filename) {
        SDL_Surface *surface = IMG_Load(filename.c_str());
        if (surface == nullptr) {
            fprintf(stderr, "Failure while loading texture surface : %s\n",
                    SDL_GetError());
            return nullptr;
        }
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (texture == nullptr) {
            fprintf(stderr, "Failure while loading texture : %s\n", SDL_GetError());
            return nullptr;
        }
        SDL_FreeSurface(surface);
        return texture;
    }
}

#endif //BUNKERBUILDER_SDL_BASE_H
