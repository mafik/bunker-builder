#include <SDL2/SDL.h>
#include "namegen.h"
#include "game_base.h"
#include "sdl_base.h"

using namespace std;
using namespace bb;

int main() {
  if (!Init())
    return 1;
  dwarves.insert(Dwarf::MakeRandom(0, 2));
  dwarves.insert(Dwarf::MakeRandom(2, 5));
  AddStructure(1, 5, Structure::New(STAIRCASE));
  AddStructure(2, 5, Structure::New(STAIRCASE));
  AddStructure(3, 5, Structure::New(STAIRCASE));
  AddStructure(3, 4, Structure::New(CORRIDOR));
  AddStructure(3, 3, Structure::New(CORRIDOR));
  AddStructure(3, 2, Structure::New(WORKSHOP));

  while (HandleInput()) {
    Tick();
    Draw();
  }
  SDL_Quit();
  return 0;
}