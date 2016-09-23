#ifndef BUNKERBUILDER_GAME_H
#define BUNKERBUILDER_GAME_H

#include <cstdio>
#include <cmath>
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
 * Dwarf can equip a costume that will alter his stats.
 */

namespace bb {

using namespace std;

constexpr int W = 100;
constexpr int H = 200;

int money = 1000000;

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

  string ToString() {
    char buff[100];
    snprintf(buff, 100, "%d-%d", row, col);
    return string(buff);
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
    if (item) {
      item->pos.x = pos.x;
      item->pos.y = pos.y;
    }
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

struct CellItem {
  Cell cell;
  Item* item;
  
  CellItem() = default;
  CellItem(const CellItem&) = default;
  CellItem(Cell cell_, Item* item_) : cell(cell_), item(item_) {}
  
  bool operator<(const CellItem& other) const {
    if (cell == other.cell) {
      return item < other.item;
    }
    return cell < other.cell;
  }
  
  bool operator!=(const CellItem& other) const {
    return (cell != other.cell) || (item != other.item);
  }

  string ToString() {
    char arr[100];
    snprintf(arr, 100, "%s(%s)", cell.ToString().c_str(), item ? item->def->texture_name.c_str() : "null");
    return string(arr);
  }
};


    // TODO
struct SearchStep {
  double dist;
};

struct SearchVisit : SearchStep {
  Dwarf* dwarf;
  pair<CellItem, CellItem> cells;
};

struct SearchAssign : SearchStep {
  Dwarf* dwarf;
  
};
  
bool TakeWorkAt(Dwarf *dwarf, CellItem cell_item) {
  const Cell& cell = cell_item.cell;
  Item* item = cell_item.item;
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
  map<Dwarf*, map<CellItem, CellItem>> shortest_path_tree;
  multimap<double, pair<Dwarf*, pair<CellItem, CellItem>>> Q;
  auto Q_add = [&Q](double dist, Dwarf* dwarf, CellItem next, CellItem prev) {
    Q.insert( make_pair(dist, make_pair(dwarf, make_pair(next, prev))) );
  };
  for (Dwarf *d : dwarves) {
    auto pos = d->pos;
    CellItem cell_item = CellItem(pos, d->item);
    if (TakeWorkAt(d, cell_item)) { // skip search if already "standing" on a job
      d->GoToWork(Waypoint(pos));
    } else {
      Q_add(0, d, cell_item, cell_item);
    }
  }
  int search_counter = 0;
  while (!Q.empty()) {
    auto p = Q.begin();
    const double dist = p->first;
    Dwarf *dwarf = p->second.first;
    auto current_source = p->second.second;
    CellItem current = current_source.first;
    CellItem source = current_source.second;
    //printf("Search step %d: '%s' is visiting %s from %s\n", search_counter, dwarf->name.c_str(), current.ToString().c_str(), source.ToString().c_str());
    Q.erase(p);
    if (dwarf->plan || dwarf->structure) continue;
    //printf("checkpoint A\n");
    auto &visited = shortest_path_tree[dwarf];
    if (visited.find(current) != visited.end()) continue;
    //printf("checkpoint B\n");
    visited[current] = source;
    auto Peek = [&](CellItem next) -> bool {
      //printf(" - considering next step to %s\n", next.ToString().c_str());
      double next_dist = dist;
      if (next.cell.row == current.cell.row - 1) {
        if (!CanTravelVertically(current.cell)) return false;
        next_dist += 2;
      }
      bool is_below = next.cell.row == current.cell.row + 1;
      auto plan_it = plans.find(next.cell);
      bool is_staircase_planned = plan_it != plans.end() &&
                                  plan_it->second->structure_type == STAIRCASE;
      if ((!is_below || is_staircase_planned) && TakeWorkAt(dwarf, next)) {
        // add the next cell to the bfs tree regardless of reachability
        visited[next] = current;
        source = current;
        current = next;
        // backtrack through bfs tree
        CellItem start = CellItem(Cell(dwarf->pos), dwarf->item);
        /*
        printf("%s (%d-%d) takes work at %d-%d\n", dwarf->name.c_str(), start.cell.row, start.cell.col, next.cell.row, next.cell.col);
        if (dwarf->item) {
          printf("%s carries %s\n", dwarf->name.c_str(), dwarf->item->def->texture_name.c_str());
        } else {
          printf("%s has no item\n", dwarf->name.c_str());
        }
        printf("%s is assigned to %s\n", dwarf->name.c_str(), next.ToString().c_str());
         */
        while (source != start) {
          auto p = visited.find(source);
          current = p->first;
          source = p->second;
        }
        Point first = Waypoint(source.cell); // cell where the dwarf is standing currently
        Point second = Waypoint(current.cell); // next cell in the path
        // prevent moving backwards by looking one waypoint ahead
        int block_dist = first.MetroDist(second);
        int my_dist = dwarf->pos.MetroDist(second);
        if (my_dist <= block_dist) {
          if (source.item != current.item) {
            dwarf->item = current.item;
          }
          dwarf->GoToWork(second);
        }
        else dwarf->GoToWork(first);
        return true;
      }
      if (next.cell.row == current.cell.row) {
        if (!CanTravel(next.cell)) return false;
        next_dist += 1;
      }
      if (next.cell.row == current.cell.row + 1) {
        if (!CanTravelVertically(next.cell)) return false;
        next_dist += 2;
      }
      //printf(" - scheduling next step to %s\n", next.ToString().c_str());
      Q_add(next_dist, dwarf, next, current);
      return false;
    };
    if (++search_counter > 1000) break;
    auto range = items.equal_range(current.cell);
    bool found = false;
    for (auto it = range.first; it != range.second; ++it) {
      if (Peek(CellItem(current.cell, it->second))) {
        found = true;
        break;
      }
    }
    if (found) continue;
    if (Peek(CellItem(Cell(current.cell.row, current.cell.col + 1), current.item))) continue;
    if ((current.cell.col > 0) && Peek(CellItem(Cell(current.cell.row, current.cell.col - 1), current.item))) continue;
    if (Peek(CellItem(Cell(current.cell.row + 1, current.cell.col), current.item))) continue;
    if ((current.cell.row > 0) && Peek(CellItem(Cell(current.cell.row - 1, current.cell.col), current.item))) continue;
  }

  for (Dwarf *d : dwarves) d->ReturnWork();
}
}

/*
Types of work:
- a room needs an item
- spend some time to produce an item in a room
- 


Digging produces different kinds of materials: stone, iron ore, copper ore, gold ore, coal, bones

On the ground there are herbs growing which can be cut to get" herbs
A herb can also be a source of saplings without cutting it down.

Underground there are mushrooms which work in exactly the same way.

Depots lets the player:
 - buy / sell bio-chemicals
 - buy / sell meals
 - buy / sell metal bars

Biolab can convert
 - mushrooms & herbs into bio-chemicals.
 - bio-chemicals into medicine.
 
Kitchen can convert
 - muchrooms & herbs into meals.
 
Smelter can convert
 - coal + ores -> bars

Metalworks can convert
 - steel bars

*/

#endif //BUNKERBUILDER_GAME_BASE_H