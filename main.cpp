/* TODO: frame sync menu system working with mouse */

#include <cstdio>
#include <vector>
#include <map>
#include <deque>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <set>
#include <functional>
#include "namegen.h"
#include "utils.h"
#include "game_base.h"

using namespace std;
using namespace bb;

Point camera;
double scale = 1;

map < Cell, Structure * >cells;
map < Cell, StructureType > plans;
SDL_Texture *selection_texture;
map < StructureType, SDL_Texture * >textures;
SDL_Texture *sky;
SDL_Texture *dwarf;
SDL_Window *window;
SDL_Renderer *renderer;
SDL_Rect windowRect = { 900, 300, 800, 1000 };

struct Button {
  SDL_Texture *texture;
   function < void (Button * self) > action;
  bool pressed = false;
};
vector < Button * >buttons;
Button *active_button = nullptr;

Command active_command = COMMAND_SELECT;

bool IsStructureType(Cell cell, StructureType structure_type) {
  auto it = cells.find(cell);
  if (it == cells.end())
    return false;
  else
    return it->second->type == structure_type;
}

bool CanTravelVertically(Cell cell) {
  return (cell.row == 0) || IsStructureType(cell, STAIRCASE);
}

bool CanTravel(Cell cell) {
  if (cell.row == 0)
    return true;
  return cells.find(cell) != cells.end();
}

struct Dwarf {
  string name;
  Point pos;
  void GetEffectiveSDL_Rect(SDL_Rect* rect) {
    int w = 82;
    int h = 100;
    rect->x = (int) ((pos.x + (W - w) / 2 - camera.x) * scale);
    rect->y = (int) ((pos.y + H - h - camera.y) * scale);
    rect->w = int (w * scale);
    rect->h = int (h * scale);
  }
  static Dwarf *make_random() {
    Dwarf *d = new Dwarf();
    d->name = namegen::gen();
    d->say("Hello!");
    return d;
  }

  void say(string text) {
    printf("%s: %s\n", name.c_str(), text.c_str());
  }

  void move() {
    Cell start = Cell(pos);
    map < Cell, Cell > visited;
    deque < pair < Cell, Cell >> Q;
    Q.push_back(make_pair(start, start));
    int search_counter = 0;
    while (!Q.empty()) {
      ++search_counter;
      if (search_counter > 100) {       // searched 100 cells and didn't found
        // anything - cancel
        // say("I have no idea where to go...");
        break;
      }
      pair < Cell, Cell > pair = Q.front();
      Cell current = pair.first;
      Cell source = pair.second;
      Q.pop_front();
      if (visited.find(current) != visited.end())
        continue;
      // say(format("looking at %llu %llu", current.first,
      // current.second));
      visited[current] = source;
      if (IsStructureType(current, WORKSHOP)) {
        // backtrack through bfs tree
        while (source != start) {
          pair = *visited.find(source);
          current = pair.first;
          source = pair.second;
        }
        // say(format("I'm going towards %llu, %llu", current.first,
        // current.second));
        Point dest_pos = Point(current);
        pos.y += limit_abs < i64 > (dest_pos.y - pos.y, 3);
        pos.x += limit_abs < i64 > (dest_pos.x - pos.x, 5);
        // say(format("My current pos: %lld %lld", pos.first,
        // pos.second));
        break;
      }
      Cell right = { current.row, current.col + 1 };
      Cell left = { current.row, current.col - 1 };
      if ((current.col > 0) && CanTravel(left))
        Q.push_back(make_pair(left, current));
      if (CanTravel(right))
        Q.push_back(make_pair(right, current));
      Cell above = { current.row - 1, current.col };
      Cell below = { current.row + 1, current.col };
      if (CanTravelVertically(current)) {
        if ((current.row > 0) && (CanTravelVertically(above)))
          Q.push_back(make_pair(above, current));
        if (CanTravelVertically(below))
          Q.push_back(make_pair(below, current));
      }
    }
  }
};

bool Init();

SDL_Texture *LoadTexture(const string & filename);

void Draw();

void input_SDL();

void add_structure(int level, int row, Structure * structure) {
  Cell coord = { level, row };
  auto it = cells.find(coord);
  if (it == cells.end()) {
    cells.insert(make_pair(coord, structure));
  } else {
    delete it->second;
    cells[coord] = structure;
  }
}

bool loop = true;
 set < Dwarf * >dwarves;

int main() {
  if (!Init())
    return 1;
  dwarves.insert(Dwarf::make_random());
  add_structure(1, 5, new Staircase());
  add_structure(2, 5, new Staircase());
  add_structure(3, 5, new Staircase());
  add_structure(3, 4, new Corridor());
  add_structure(3, 3, new Corridor());
  add_structure(3, 2, new Workshop());

  while (loop) {
    input_SDL();
    for (Dwarf * d : dwarves) {
      d->move();
    }
    Draw();
  }
  SDL_Quit();
  return 0;
}

bool middle_down = false;
i64 middle_down_x, middle_down_y;
double last_scale = .5;
int middle_down_time;

void set_scale(double new_scale) {
  int mx, my;
  SDL_GetMouseState(&mx, &my);
  i64 cx = camera.x + i64(mx / scale);
  i64 cy = camera.y + i64(my / scale);
  scale = clamp(new_scale, 0.1, 10.);
  camera.y = cy - i64(my / scale);
  camera.x = cx - i64(mx / scale);
}

void GetMouseCell(Cell * out) {
  int mx, my;
  SDL_GetMouseState(&mx, &my);
  *out = Cell(Point(camera.y + my / scale, camera.x + mx / scale));
}

void TogglePlan(const Cell & c, StructureType structure_type) {
  if (plans[c] == structure_type) {
    plans.erase(c);
  } else {
    plans[c] = structure_type;
  }
}

StructureType fill_structure;
set < Cell > toggled_cells;

void input_SDL() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT)
      loop = false;
    else if (event.type == SDL_KEYDOWN) {
      switch (event.key.keysym.sym) {
        default:
          loop = false;
          break;
      }
    } else if (event.type == SDL_MOUSEBUTTONDOWN) {
      if (event.button.button == SDL_BUTTON_MIDDLE) {
        middle_down = true;
        middle_down_time = SDL_GetTicks();
        middle_down_x = camera.x + i64(event.button.x / scale);
        middle_down_y = camera.y + i64(event.button.y / scale);
      } else if (event.button.button == SDL_BUTTON_LEFT) {
        if (event.button.x < 100) {
          int ind = event.button.y / 100;
          if (ind < buttons.size()) {
            Button *b = buttons[ind];
            b->action(b);
          }
        } else {
          Cell c;
          GetMouseCell(&c);
          switch (active_command) {
            case COMMAND_STAIRCASE:
              fill_structure = STAIRCASE;
              break;
            case COMMAND_CORRIDOR:
              fill_structure = CORRIDOR;
              break;
            case COMMAND_WORKSHOP:
              fill_structure = WORKSHOP;
              break;
            default:
              break;
          }
          toggled_cells.insert(c);
          TogglePlan(c, fill_structure);
        }
      }
    } else if (event.type == SDL_MOUSEBUTTONUP) {
      if (event.button.button == SDL_BUTTON_MIDDLE) {
        middle_down = false;
        int time = SDL_GetTicks();
        if (time - middle_down_time < 200) {
          if (scale == 1) {
            set_scale(last_scale);
          } else {
            last_scale = scale;
            set_scale(1);
          }
        }
      } else if (event.button.button == SDL_BUTTON_LEFT) {
        fill_structure = NONE;
        toggled_cells.clear();
      }
    } else if (event.type == SDL_MOUSEMOTION) {
      if (middle_down) {
        camera.y = middle_down_y - i64(event.motion.y / scale);
        camera.x = middle_down_x - i64(event.motion.x / scale);
      }
      if (fill_structure != NONE) {
        Cell c;
        GetMouseCell(&c);
        if (toggled_cells.find(c) == toggled_cells.end()) {
          toggled_cells.insert(c);
          TogglePlan(c, fill_structure);
        }
      }
    } else if (event.type == SDL_MOUSEWHEEL) {
      set_scale(scale * exp2(event.wheel.y / 4.));
    }
  }
}

SDL_Texture *GetTextureForStructureType(StructureType structure_type) {
  return textures[structure_type];
}

SDL_Texture *GetTextureForCell(const Cell &cell) {
  auto it = cells.find(cell);
  if (it == cells.end()) {
    return cell.row <= 0 ? sky : textures[NONE];
  }
  return GetTextureForStructureType(it->second->type);
}

void GetTileRect(int row, int col, SDL_Rect * out) {
  out->x = int ((col * W - camera.x) * scale);
  out->y = int ((row * H - camera.y) * scale);
  out->w = int (((col + 1) * W - camera.x) * scale) - out->x;
  out->h = int (((row + 1) * H - camera.y) * scale) - out->y;
}

void Draw() {
  SDL_RenderClear(renderer);

  SDL_Rect tile_rect;

  i64 left = i64(camera.x + 100 / scale);
  i64 right = left + i64(windowRect.w / scale);
  i64 top = camera.y;
  i64 bottom = top + i64(windowRect.h / scale);
  Cell top_left = Cell(Point(top, left));
  Cell bottom_right = Cell(Point(bottom, right));

  for (i64 row = top_left.row; row <= bottom_right.row; ++row) {
    for (i64 col = top_left.col; col <= bottom_right.col; ++col) {
      Cell cell = { row, col };
      SDL_Texture *texture = GetTextureForCell(cell);
      GetTileRect(row, col, &tile_rect);
      SDL_RenderCopy(renderer, texture, nullptr, &tile_rect);
      auto it = plans.find(cell);
      if (it != plans.end()) {
        SDL_Texture *structure_texture = GetTextureForStructureType(it->second);
        SDL_SetTextureAlphaMod(structure_texture, 64);
        SDL_SetTextureBlendMode(structure_texture, SDL_BLENDMODE_BLEND);
        SDL_RenderCopy(renderer, structure_texture, nullptr, &tile_rect);
        SDL_SetTextureBlendMode(structure_texture, SDL_BLENDMODE_NONE);
        // SDL_SetTextureAlphaMod(structure_texture, 255);
      }
    }
  }

for (Dwarf * d:dwarves) {
    SDL_Rect r;
    d->GetEffectiveSDL_Rect(&r);
    SDL_RenderCopy(renderer, dwarf, nullptr, &r);
  }

  if (active_command != COMMAND_SELECT) {
    Cell c;
    GetMouseCell(&c);
    GetTileRect(c.row, c.col, &tile_rect);
    SDL_RenderCopy(renderer, selection_texture, nullptr, &tile_rect);
  }

  SDL_Rect button_rect {
  0, 0, 100, 100};
  for (int i = 0; i < buttons.size(); ++i) {
    button_rect.y = i * 100;
    if (buttons[i] == active_button) {
      SDL_SetTextureColorMod(buttons[i]->texture, 128, 128, 128);
    } else {
      SDL_SetTextureColorMod(buttons[i]->texture, 255, 255, 255);
    }
    SDL_RenderCopy(renderer, buttons[i]->texture, nullptr, &button_rect);
  }
  SDL_RenderPresent(renderer);
}

bool Init() {
  if (SDL_Init(SDL_INIT_EVERYTHING) == -1) {
    fprintf(stderr, "Failed to initialize SDL : %s\n", SDL_GetError());
    return false;
  }
  window =
    SDL_CreateWindow("Server", windowRect.x, windowRect.y, windowRect.w,
                     windowRect.h, 0);
  if (window == nullptr) {
    fprintf(stderr, "Failed to create window : %s\n", SDL_GetError());
    return false;
  }
  renderer =
    SDL_CreateRenderer(window, -1,
                       SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (renderer == nullptr) {
    fprintf(stderr, "Failed to create renderer : %s\n", SDL_GetError());
    return false;
  }
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
  // Set size of renderer to the same as window
  SDL_RenderSetLogicalSize(renderer, windowRect.w, windowRect.h);
  // Set color of renderer to red
  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
  sky = LoadTexture("sky.png");
  dwarf = LoadTexture("dwarf.gif");
  textures[NONE] = LoadTexture("ground.png");
  textures[STAIRCASE] = LoadTexture("staircase.png");
  textures[CORRIDOR] = LoadTexture("corridor.png");
  textures[WORKSHOP] = LoadTexture("corridor.png");

  selection_texture = LoadTexture("block_selection.png");

for (auto p:initializer_list < pair < string, Command >> {
       {
       "btn_corridor.png", COMMAND_CORRIDOR}
       , {
       "btn_staircase.png", COMMAND_STAIRCASE}
       , {
       "btn_workshop.png", COMMAND_WORKSHOP}
       }
  ) {
    Button *b = new Button();
    b->texture = LoadTexture(p.first);
    Command c = p.second;
    b->action =[c] (Button * self) {
      if (active_button == self) {
        active_button = nullptr;
        active_command = COMMAND_SELECT;
      } else {
        active_button = self;
        active_command = c;
      }
    };
    buttons.push_back(b);
  }
  return true;
}

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