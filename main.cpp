/* TODO: frame sync menu system working with mouse */

#include <SDL2/SDL.h>
#include "namegen.h"
#include "game_base.h"
#include "sdl_base.h"

using namespace std;
using namespace bb;

int main() {
  if (!Init())
    return 1;
  dwarves.insert(Dwarf::MakeRandom());
  AddStructure(1, 5, new Staircase());
  AddStructure(2, 5, new Staircase());
  AddStructure(3, 5, new Staircase());
  AddStructure(3, 4, new Corridor());
  AddStructure(3, 3, new Corridor());
  AddStructure(3, 2, new Workshop());

  while (HandleInput()) {
    for (Dwarf * d : dwarves) {
      d->Move();
    }
    Draw();
  }
  SDL_Quit();
  return 0;
}