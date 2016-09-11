#ifndef BUNKERBUILDER_GAME_H
#define BUNKERBUILDER_GAME_H

#include <cstdio>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <deque>
#include <vector>
#include <functional>
#include "utils.h"

/**
 * Each cell is able to hold arbitrary number of small items.
 * Each corridor is able to hold one structure.
 *
 * When a user is pointing at a cell, a detailed box about all therein located items appears.
 *
 * Dwarf can hold one item in hands (weapon / tool / moved furniture etc.)
 * Dwarf can equip one light and one heavy piece of clothing for each location: hands, torso, head, legs, feet.
 */

namespace bb {

using namespace std;

constexpr int W = 100;
constexpr int H = 200;

enum StructureType {
  NONE = 0,
  STAIRCASE,
  CORRIDOR,
  MUSHROOM_FARM
};

struct Dwarf;

struct Structure {
  StructureType type;
  Dwarf *assignee = nullptr;

  static Structure *New(StructureType);
};

struct Staircase : Structure {
};

struct Corridor : Structure {
};

struct MushroomFarm : Structure {
};

Structure *Structure::New(StructureType type) {
  Structure *s;
  switch (type) {
    case STAIRCASE:
      s = new Staircase();
      break;
    case CORRIDOR:
      s = new Corridor();
      break;
    case MUSHROOM_FARM:
      s = new MushroomFarm();
      break;
    default:
      fprintf(stderr, "Unknown structure type: %d\n", type);
      return nullptr;
  }
  s->type = type;
  return s;
}

struct Point;

struct Cell {
  int row, col;

  Cell(int _row, int _col) : row(_row), col(_col) {}

  Cell() : row(0), col(0) {}

  Cell(const Point &);

  bool operator!=(const Cell &other) const {
    return row != other.row || col != other.col;
  }

  bool operator==(const Cell &other) const { return !(*this != other); }

  bool operator<(const Cell &other) const {
    if (row == other.row) return col < other.col;
    return row < other.row;
  }
};


struct Point {
  int y, x;

  Point(int _y, int _x) : y(_y), x(_x) {}

  Point() : y(0), x(0) {}

  Point(const Cell &cell) : y(cell.row * H), x(cell.col * W) {}

  int MetroDist(const Point &other) {
    return abs(x - other.x) + abs(y - other.y);
  }
};

Point Waypoint(const Cell &cell) {
  return Point((cell.row + 1) * H - 1, (cell.col) * W + W / 2);
}

Cell::Cell(const Point &point) : row(div_floor(point.y, H)), col(div_floor(point.x, W)) {}

struct AABB {
  int left, right, top, bottom; // bottom >= top (y axis grows downwards)
  AABB(const Dwarf &);

  AABB(const Cell &c) : left(c.col * W), right((c.col + 1) * W - 1), top(c.row * H),
                        bottom((c.row + 1) * H - 1) {}
};

// TODO: Convert Items & Dwarves to "Objects"
// TODO: Convert Structures to similar structure as Items

enum ItemType {
  SPORE,
  NO_ITEM_TYPE,
};

struct ItemDef {
  ItemType type;
  int w, h;
  string texture_name;
};

struct Item {
  Dwarf* carrier;
  Point pos;
  ItemDef *def;
  Dwarf *assignee;
};

ItemDef item_defs[] = {
    [SPORE]={.type = SPORE, .w=27, .h=26, .texture_name = "mushroom.png"}
};

struct Plan {
  StructureType structure_type;
  double progress;
  Dwarf *assignee;

  Plan(StructureType _structure_type) : structure_type(_structure_type), progress(0), assignee(nullptr) {}
};

}
namespace std {
template<>
struct hash<bb::Cell> {
  size_t operator()(const bb::Cell &cell) const {
    return (hash<int>()(cell.col) >> 1) ^ hash<int>()(cell.row);
  }
};
}

namespace bb {

unordered_map<Cell, Structure *> cells;
unordered_map<Cell, Plan *> plans;
unordered_multimap<Cell, Item *> items;

void AddItem(Point pos, ItemType item_type) {
  Item *item = new Item();
  item->def = &item_defs[item_type];
  item->pos = pos;
  items.insert(make_pair(Cell(item->pos), item));
}

void AddStructure(int row, int col, Structure *structure) {
  Cell coord = {row, col};
  auto it = cells.find(coord);
  if (it == cells.end()) {
    cells.insert(make_pair(coord, structure));
  } else {
    delete it->second;
    cells[coord] = structure;
  }
}

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

bool HasPlan(Cell cell) {
  return plans.find(cell) != plans.end();
}

Event<Dwarf> dwarf_created;

struct Dwarf {
  string name;
  Point pos;
  Event<string> said_something;
  Item *item = nullptr;

  static Dwarf *MakeRandom(int row, int col) {
    Dwarf *d = new Dwarf();
    d->name = namegen::gen();
    d->pos = Waypoint(Cell(row, col));
    dwarf_created.run(d);
    d->Say("Hello!");
    return d;
  }

  void Say(string text) {
    said_something.run(&text);
  }

  int last_said = -5000;

  void SaySlow(const string &text) {
    int now = SDL_GetTicks();
    if (now - last_said > 5000) {
      last_said = now;
      Say(text);
    }
  }

  Plan *plan = nullptr;
  Structure *structure = nullptr;
  Item *assigned_item = nullptr;
  Cell destination = Cell(0, 0);

  void ReturnWork() {
    if (plan) {
      plan->assignee = nullptr;
      plan = nullptr;
    }
    if (structure) {
      structure->assignee = nullptr;
      structure = nullptr;
    }
    if (assigned_item) {
      assigned_item->assignee = nullptr;
      assigned_item = nullptr;
    }
  }

  void GoToWork(const Point &waypoint) {
    int dy = limit_abs<int>(waypoint.y - pos.y, 3);
    int dx = limit_abs<int>(waypoint.x - pos.x, 5);
    pos.y += dy;
    pos.x += dx;
    if (Cell(waypoint) == destination) {
      if (!CanTravel(destination)) {
        AABB dwarf_bb = AABB(*this);
        AABB dest_bb = AABB(destination);
        if ((dx > 0) && (dwarf_bb.right >= dest_bb.left)) {
          pos.x -= dwarf_bb.right - dest_bb.left + 1;
          dx = 0;
        }
        if ((dx < 0) && (dwarf_bb.left <= dest_bb.right)) {
          pos.x += dest_bb.right - dwarf_bb.left + 1;
          dx = 0;
        }

        if ((dy > 0) && (dwarf_bb.bottom >= dest_bb.top)) {
          pos.y -= dwarf_bb.bottom - dest_bb.top + 1;
          dy = 0;
        }
        if ((dy < 0) && (dwarf_bb.top <= dest_bb.bottom)) {
          pos.y += dest_bb.bottom - dwarf_bb.top + 1;
          dy = 0;
        }
      }
      if (plan && dx == 0 && dy == 0) {
        plan->progress += 0.01;
        if (plan->progress >= 1) {
          cells[destination] = Structure::New(plan->structure_type);
          plans.erase(destination);
          delete plan;
          plan = nullptr;
        }
      }
    }
  }

  static const int width = 82;
  static const int height = 100;
};

AABB::AABB(const Dwarf &dwarf)
    : left(dwarf.pos.x - Dwarf::width / 2), right(dwarf.pos.x + Dwarf::width / 2), top(dwarf.pos.y - Dwarf::height),
      bottom(dwarf.pos.y) {}

set<Dwarf *> dwarves;

// TODO: preferential weighing of distances
// TODO: item pickup

bool TakeWorkAt(Dwarf *dwarf, Item *item, Cell cell) {
  auto plan_it = plans.find(cell);
  if (plan_it != plans.end() && plan_it->second->assignee == nullptr) {
    dwarf->destination = cell;
    dwarf->plan = plan_it->second;
    dwarf->plan->assignee = dwarf;
    return true;
  }
  auto struct_it = cells.find(cell);
  if (struct_it != cells.end() && struct_it->second->type == MUSHROOM_FARM &&
      struct_it->second->assignee == nullptr && item != nullptr && item->def->type == SPORE &&
      item->assignee == nullptr) {
    dwarf->SaySlow("I'll work at this workshop...");
    dwarf->destination = cell;
    dwarf->structure = struct_it->second;
    dwarf->structure->assignee = dwarf;
    item->assignee = dwarf;
    dwarf->assigned_item = item;
    return true;
  }
  return false;
}

void Tick() {
  typedef pair<Dwarf *, Item *> DwarfItem;
  map<DwarfItem, map<Cell, Cell>> shortest_path_tree;
  multimap<double, pair<DwarfItem, pair<Cell, Cell>>> Q;
  for (Dwarf *d : dwarves) {
    auto pos = d->pos;
    if (TakeWorkAt(d, d->item, pos)) { // skip search if already "standing" on a job
      d->GoToWork(Waypoint(pos));
    } else {
      Q.insert(make_pair(0, make_pair(make_pair(d, d->item), make_pair(pos, pos))));
    }
  }
  int search_counter = 0;
  while (!Q.empty()) {
    auto p = Q.begin();
    double dist = p->first;
    auto dwarf_item = p->second.first;
    auto current_source = p->second.second;
    Dwarf *dwarf = dwarf_item.first;
    Item *item = dwarf_item.second;
    Cell current = current_source.first;
    Cell source = current_source.second;
    Q.erase(p);
    if (dwarf->plan || dwarf->structure) continue;
    auto &visited = shortest_path_tree[dwarf_item];
    if (visited.find(current) != visited.end()) continue;
    visited[current] = source;
    auto Peek = [&](Cell next) -> bool {
      if (next.row == current.row - 1) {
        if (!CanTravelVertically(current)) return false;
        dist += 2;
      }
      bool is_below = next.row == current.row + 1;
      auto plan_it = plans.find(next);
      bool is_staircase_planned = plan_it != plans.end() &&
                                  plan_it->second->structure_type == STAIRCASE;
      if ((!is_below || is_staircase_planned) && TakeWorkAt(dwarf, item, next)) {
        // add the next cell to the bfs tree regardless of reachability
        visited[next] = current;
        source = current;
        current = next;
        // backtrack through bfs tree
        Cell start = Cell(dwarf->pos);
        while (source != start) {
          auto p = visited.find(source);
          current = p->first;
          source = p->second;
          // TODO: fix this loop
          if (item != nullptr &&
              item->assignee == dwarf &&
              item->carrier == nullptr &&
              current == Cell(item->pos)) {
            visited = shortest_path_tree[make_pair(dwarf, dwarf->item)];
          }
        }
        Point first = Waypoint(source); // cell where the dwarf is standing currently
        if (item != nullptr &&
            item->assignee == dwarf &&
            item->carrier == nullptr &&
            Cell(item->pos) == source) {
          item->carrier = dwarf;
          dwarf->item = item;
        }
        Point second = Waypoint(current); // next cell in the path
        // prevent moving backwards by looking one waypoint ahead
        int block_dist = first.MetroDist(second);
        int my_dist = dwarf->pos.MetroDist(second);
        if (my_dist <= block_dist) dwarf->GoToWork(second);
        else dwarf->GoToWork(first);
        return true;
      }
      if (next.row == current.row) {
        if (!CanTravel(next)) return false;
        dist += 1;
      }
      if (next.row == current.row + 1) {
        if (!CanTravelVertically(next)) return false;
        dist += 2;
      }
      auto step = make_pair(next, current);

      if (item == nullptr) {
        auto range = items.equal_range(current);
        for (auto it = range.first; it != range.second; ++it) {
          Q.insert(make_pair(dist, make_pair(make_pair(dwarf, it->second), step)));
        }
      }
      Q.insert(make_pair(dist, make_pair(make_pair(dwarf, item), step)));
      return false;
    };
    if (++search_counter > 1000) break;
    if (Peek({current.row, current.col + 1})) continue;
    if ((current.col > 0) && Peek({current.row, current.col - 1})) continue;
    if (Peek({current.row + 1, current.col})) continue;
    if ((current.row > 0) && Peek({current.row - 1, current.col})) continue;
  }

  for (Dwarf *d : dwarves) d->ReturnWork();
}
}

#endif //BUNKERBUILDER_GAME_BASE_H