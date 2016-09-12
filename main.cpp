//#define SDL

#include <cstdio>

#ifdef SDL
#include <SDL2/SDL.h>
#endif

#include "namegen.h"
#include "game.h"

#ifdef SDL
#include "sdl.h"
#endif

using namespace std;
using namespace bb;

int main() {
#ifdef SDL
  if (!Init())
    return 1;
#endif

  dwarves.insert(Dwarf::MakeRandom(0, 2));
//  dwarves.insert(Dwarf::MakeRandom(2, 5));
  AddStructure(1, 5, Structure::New(STAIRCASE));
  AddStructure(2, 5, Structure::New(STAIRCASE));
  AddStructure(3, 5, Structure::New(STAIRCASE));
  AddStructure(3, 4, Structure::New(CORRIDOR));
  AddStructure(3, 3, Structure::New(CORRIDOR));
  AddStructure(3, 2, Structure::New(MUSHROOM_FARM));
  AddItem(Point(100, 800), SPORE);

#ifdef SDL
  while (HandleInput()) {
    Tick();
    Draw();
  }
  SDL_Quit();
#else
  for (int i = 0; i < 30; ++i) {
    printf("Tick %d\n", i);
    Tick();
  }
#endif
  return 0;
}
