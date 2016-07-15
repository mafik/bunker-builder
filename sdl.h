#ifndef BUNKERBUILDER_SDL_H
#define BUNKERBUILDER_SDL_H

#include <string>
#include <functional>
#include <unordered_map>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL_ttf.h>
#include <algorithm>
#include "game.h"

namespace bb {
    using namespace std;

    struct Button {
        SDL_Texture *texture;
        function<void(Button *self)> action;
        bool pressed = false;
    };

    enum Command {
        COMMAND_SELECT = 0, COMMAND_STAIRCASE, COMMAND_CORRIDOR, COMMAND_MUSHROOM_FARM
    };

    SDL_Window *window;
    SDL_Renderer *renderer;

    SDL_Texture *selection_texture;
    unordered_map<StructureType, SDL_Texture *> textures;
    SDL_Texture *item_textures[LAST_ITEM_TYPE];
    SDL_Texture *sky;
    SDL_Texture *dwarf;
    SDL_Rect windowRect = {900, 300, 800, 1000};
    TTF_Font *font;

    vector<Button *> buttons;
    Button *active_button = nullptr;
    Command active_command = COMMAND_SELECT;

    Point camera;
    double scale = 1;

    bool middle_down = false;
    int middle_down_x, middle_down_y;
    double last_scale = .5;
    int middle_down_time;

    StructureType fill_structure;
    set<Cell> toggled_cells;

    void SetScale(double new_scale) {
      int mx, my;
      SDL_GetMouseState(&mx, &my);
      int cx = camera.x + int(mx / scale);
      int cy = camera.y + int(my / scale);
      scale = clamp(new_scale, 0.1, 10.);
      camera.y = cy - int(my / scale);
      camera.x = cx - int(mx / scale);
    }

    SDL_Texture *LoadTexture(const string &filename) {
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

    void GetEffectiveSDL_Rect(const Dwarf &d, SDL_Rect *rect) {
      int w = 82;
      int h = 100;
      rect->x = (int) ((d.pos.x - w / 2 - camera.x) * scale);
      rect->y = (int) ((d.pos.y - h - camera.y) * scale);
      rect->w = int(w * scale);
      rect->h = int(h * scale);
    }

    void GetMouseCell(Cell *out) {
      int mx, my;
      SDL_GetMouseState(&mx, &my);
      *out = Cell(Point(camera.y + my / scale, camera.x + mx / scale));
    }

    void TogglePlan(const Cell &c, StructureType structure_type) {
      auto it = plans.find(c);
      if (it != plans.end()) {
        bool the_same = it->second->structure_type == structure_type;
        delete it->second;
        plans.erase(it);
        if (the_same) return;
      }
      plans[c] = new Plan(structure_type);
    }

    bool InitTextures() {
      sky = LoadTexture("sky.png");
      dwarf = LoadTexture("dwarf.gif");
      textures[NONE] = LoadTexture("ground.png");
      textures[STAIRCASE] = LoadTexture("staircase.png");
      textures[CORRIDOR] = LoadTexture("corridor.png");
      textures[MUSHROOM_FARM] = LoadTexture("mushroom_farm.png");

      selection_texture = LoadTexture("block_selection.png");

      buttons.clear();
      for (auto p : initializer_list<pair<string, Command >> {
          {
              "btn_corridor.png",      COMMAND_CORRIDOR},
          {
              "btn_staircase.png",     COMMAND_STAIRCASE},
          {
              "btn_mushroom_farm.png", COMMAND_MUSHROOM_FARM}
      }
          ) {
        Button *b = new Button();
        b->texture = LoadTexture(p.first);
        Command c = p.second;
        b->action = [c](Button *self) {
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

      for (int i = 0; i < LAST_ITEM_TYPE; ++i) {
        item_textures[i] = LoadTexture(item_defs[i].texture_name);
      }

      TTF_Init();
      font = TTF_OpenFont("./Katibeh-Regular.ttf", 24);
      if (!font) {
        fprintf(stderr, "TTF_OpenFont: %s\n", TTF_GetError());
        return false;
      }
      return true;
    }

    bool InitRenderer() {
      if (renderer) SDL_DestroyRenderer(renderer);
      renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
      if (renderer == nullptr) {
        fprintf(stderr, "Failed to create renderer : %s\n", SDL_GetError());
        return false;
      }
      SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
      SDL_GetRendererOutputSize(renderer, &windowRect.w, &windowRect.h);
      SDL_SetRenderDrawColor(renderer, 64, 0, 0, 255);
      return InitTextures();
    }

    bool HandleInput() {
      SDL_Event event;
      while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT)
          return false;
        else if (event.type == SDL_KEYDOWN) {
          switch (event.key.keysym.sym) {
            default:
              windowRect.w += 100;
              //InitRenderer();
              return true;
          }
        } else if (event.type == SDL_MOUSEBUTTONDOWN) {
          if (event.button.button == SDL_BUTTON_MIDDLE) {
            middle_down = true;
            middle_down_time = SDL_GetTicks();
            middle_down_x = camera.x + int(event.button.x / scale);
            middle_down_y = camera.y + int(event.button.y / scale);
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
                case COMMAND_MUSHROOM_FARM:
                  fill_structure = MUSHROOM_FARM;
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
                SetScale(last_scale);
              } else {
                last_scale = scale;
                SetScale(1);
              }
            }
          } else if (event.button.button == SDL_BUTTON_LEFT) {
            fill_structure = NONE;
            toggled_cells.clear();
          }
        } else if (event.type == SDL_MOUSEMOTION) {
          if (middle_down) {
            camera.y = middle_down_y - int(event.motion.y / scale);
            camera.x = middle_down_x - int(event.motion.x / scale);
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
          SetScale(scale * exp2(event.wheel.y / 4.));
        } else if (event.type == SDL_WINDOWEVENT) {
          switch (event.window.event) {
            case SDL_WINDOWEVENT_RESIZED:
              windowRect.w = event.window.data1;
              windowRect.h = event.window.data2;
              break;
            defualt:
              windowRect.w += 100;//= event.window.data1;
              windowRect.h += 100; //= event.window.data2;
              break;
          }
        }
      }
      return true;
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

    void GetTileRect(int row, int col, SDL_Rect *out) {
      out->x = int((col * W - camera.x) * scale);
      out->y = int((row * H - camera.y) * scale);
      out->w = int(((col + 1) * W - camera.x) * scale) - out->x;
      out->h = int(((row + 1) * H - camera.y) * scale) - out->y;
    }

    struct Text {
        SDL_Texture *texture;
        SDL_Rect size;

        Text(const std::string &s, SDL_Color fg = {255, 255, 255, 0}, SDL_Color bg = {0, 0, 0, 0}) {
          int outline_width = 3;
          TTF_SetFontOutline(font, outline_width);
          SDL_Surface *surface_outer = TTF_RenderText_Blended(font, s.c_str(), bg);
          TTF_SetFontOutline(font, 0);
          SDL_Surface *surface_inner = TTF_RenderText_Blended(font, s.c_str(), fg);
          TTF_SizeText(font, s.c_str(), &size.w, &size.h);
          size.x = outline_width;
          size.y = outline_width;
          SDL_BlitSurface(surface_inner, NULL, surface_outer, &size);
          TTF_SetFontOutline(font, outline_width);
          TTF_SizeText(font, s.c_str(), &size.w, &size.h);
          TTF_SetFontOutline(font, 0);
          SDL_FreeSurface(surface_inner);
          SDL_Surface *surface = surface_outer;
          texture = SDL_CreateTextureFromSurface(renderer, surface);
          SDL_FreeSurface(surface);
        }

        virtual ~Text() { SDL_DestroyTexture(texture); }
    };

    struct SaidText : Text {
        int time_said;

        SaidText(const string &s, const SDL_Color &fg, const SDL_Color &bg) : Text(s, fg, bg),
                                                                              time_said(SDL_GetTicks()) { }
    };

    map<Dwarf *, deque<SaidText *>> said_texts;
    map<Dwarf *, Text *> name_texts;

    void Draw() {
      SDL_RenderClear(renderer);

      SDL_Rect tile_rect;

      int left = int(camera.x + 100 / scale);
      int right = left + int(windowRect.w / scale);
      int top = camera.y;
      int bottom = top + int(windowRect.h / scale);
      Cell top_left = Cell(Point(top, left));
      Cell bottom_right = Cell(Point(bottom, right));

      for (int row = top_left.row; row <= bottom_right.row; ++row) {
        for (int col = top_left.col; col <= bottom_right.col; ++col) {
          Cell cell = {row, col};
          SDL_Texture *texture = GetTextureForCell(cell);
          GetTileRect(row, col, &tile_rect);
          SDL_RenderCopy(renderer, texture, nullptr, &tile_rect);

          auto range = items.equal_range(cell);
          for (auto it = range.first; it != range.second; ++it) {
            SDL_Rect rect;
            rect.x = it->second->pos.x;
            rect.y = it->second->pos.y;
            rect.w = it->second->def->w;
            rect.h = it->second->def->h;
            SDL_RenderCopy(renderer, item_textures[it->second->def->type], nullptr, &rect);
          }

          auto it = plans.find(cell);
          if (it != plans.end()) {
            int orig_h = tile_rect.h;
            tile_rect.h *= 1 - it->second->progress;
            SDL_Rect source_rect = {0, 0, W, int(H * (1 - it->second->progress))};
            SDL_Texture *structure_texture = GetTextureForStructureType(it->second->structure_type);
            SDL_SetTextureAlphaMod(structure_texture, 64);
            SDL_SetTextureBlendMode(structure_texture, SDL_BLENDMODE_BLEND);
            SDL_RenderCopy(renderer, structure_texture, &source_rect, &tile_rect);
            SDL_SetTextureBlendMode(structure_texture, SDL_BLENDMODE_NONE);
            source_rect.y = source_rect.h;
            source_rect.h = H - source_rect.h;
            tile_rect.y += tile_rect.h;
            tile_rect.h = orig_h - tile_rect.h;
            SDL_RenderCopy(renderer, structure_texture, &source_rect, &tile_rect);
            tile_rect.h = orig_h;
            //SDL_SetTextureAlphaMod(structure_texture, 255);
          }
        }
      }

      for (Dwarf *d:dwarves) {
        SDL_Rect r;
        GetEffectiveSDL_Rect(*d, &r);
        SDL_RenderCopy(renderer, dwarf, nullptr, &r);
        Text *name_texture = name_texts[d];
        name_texture->size.x = r.x + r.w / 2 - name_texture->size.w / 2;
        name_texture->size.y = r.y - name_texture->size.h;
        SDL_RenderCopy(renderer, name_texture->texture, nullptr, &name_texture->size);
        int y = name_texture->size.y;
        auto &said = said_texts[d];
        int now = SDL_GetTicks();
        while (!said.empty() && (now - said.front()->time_said > 5000)) {
          delete said.front();
          said.pop_front();
        }
        for (SaidText *said_text : said) {
          said_text->size.x = r.x + r.w / 2 - said_text->size.w / 2;
          said_text->size.y = y - said_text->size.h;
          y -= said_text->size.h;
          SDL_RenderCopy(renderer, said_text->texture, nullptr, &said_text->size);
        }
      }

      if (active_command != COMMAND_SELECT) {
        Cell c;
        GetMouseCell(&c);
        GetTileRect(c.row, c.col, &tile_rect);
        SDL_RenderCopy(renderer, selection_texture, nullptr, &tile_rect);
      }

      SDL_Rect button_rect{
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
          SDL_CreateWindow("Server", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowRect.w,
                           windowRect.h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
      if (window == nullptr) {
        fprintf(stderr, "Failed to create window : %s\n", SDL_GetError());
        return false;
      }
      dwarf_created.handlers.push_back([](Dwarf *dwarf) {
          name_texts[dwarf] = new Text(dwarf->name, {150, 255, 150, 0}, {20, 60, 20, 0});
          dwarf->said_something.handlers.push_back([dwarf](string *s) {
              said_texts[dwarf].push_back(new SaidText(*s, {230, 230, 230, 0}, {60, 60, 60, 0}));
          });
      });
      if (!InitRenderer()) return false;
      return true;
    }

}

#endif //BUNKERBUILDER_SDL_BASE_H