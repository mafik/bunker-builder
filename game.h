#ifndef BUNKERBUILDER_GAME_H
#define BUNKERBUILDER_GAME_H

#include <cstdio>
#include <map>
#include <set>
#include <deque>
#include <vector>
#include <functional>
#include "utils.h"

namespace bb {

    using namespace std;

    constexpr i64 W = 100;
    constexpr i64 H = 200;

    enum StructureType {
        NONE = 0,
        STAIRCASE,
        CORRIDOR,
        WORKSHOP
    };

    struct Dwarf;
        
    struct Structure {
        StructureType type;
        Dwarf* occupant = nullptr;
        static Structure* New(StructureType);
    };

    struct Staircase : Structure {};

    struct Corridor : Structure {};

    struct Workshop : Structure {};

    Structure* Structure::New(StructureType type) {
      Structure* s;
      switch(type) {
        case STAIRCASE: s = new Staircase(); break;
        case CORRIDOR: s = new Corridor(); break;
        case WORKSHOP: s = new Workshop(); break;
        default: fprintf(stderr, "Unknown structure type: %d\n", type); return nullptr;
      }
      s->type = type;
      return s;
    }

    struct Point;

    struct Cell {
        i64 row, col;

        Cell(i64 _row, i64 _col) : row(_row), col(_col) { }

        Cell() : row(0), col(0) { }

        Cell(const Point &);

        bool operator!=(const Cell &other) const {
            return row != other.row || col != other.col;
        }
        
        bool operator==(const Cell& other) const { return !(*this != other); }

        bool operator<(const Cell &other) const {
            if (row == other.row) return col < other.col;
            return row < other.row;
        }
    };

    struct Point {
        i64 y, x;

        Point(i64 _y, i64 _x) : y(_y), x(_x) { }

        Point() : y(0), x(0) { }

        Point(const Cell &cell) : y(cell.row * H), x(cell.col * W) { }
        
        i64 MetroDist(const Point& other) {
          return abs(x - other.x) + abs(y - other.y);
        }
    };
    
    Point Waypoint(const Cell& cell) {
      return Point((cell.row + 1) * H - 1, (cell.col) * W + W / 2);
    }

    Cell::Cell(const Point &point) : row(div_floor(point.y, H)), col(div_floor(point.x, W)) { }
    
    struct AABB {
      i64 left, right, top, bottom; // bottom >= top (y axis grows downwards)
      AABB(const Dwarf&);
      AABB(const Cell& c) : left(c.col * W), right((c.col + 1) * W - 1), top(c.row * H), bottom((c.row + 1) * H - 1) {}
    };
    
    struct Plan {
      StructureType structure_type;
      double progress;
      Dwarf* assignee;
      Plan(StructureType _structure_type) : structure_type(_structure_type), progress(0), assignee(nullptr) {}
    };

    map < Cell, Structure * >cells;
    map < Cell, Plan * > plans;

    void AddStructure(int row, int col, Structure *structure) {
        Cell coord = { row, col };
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

    struct Dwarf {
        string name;
        Point pos;
        static Dwarf *MakeRandom(int row, int col) {
            Dwarf *d = new Dwarf();
            d->name = namegen::gen();
            d->Say("Hello!");
            d->pos = Waypoint(Cell(row, col));
            return d;
        }

        void Say(string text) {
            printf("%s: %s\n", name.c_str(), text.c_str());
        }
        
        Plan* plan = nullptr;
        Structure* structure = nullptr;
        Cell destination = Cell(0,0);
        
        bool TakeWorkAt(Cell cell) {
          auto plan_it = plans.find(cell);
          if(plan_it != plans.end() && plan_it->second->assignee == nullptr) {
            destination = cell;
            plan = plan_it->second;
            plan->assignee = this;
            return true;
          }
          auto struct_it = cells.find(cell);
          if (struct_it != cells.end() && struct_it->second->type == WORKSHOP && struct_it->second->occupant == nullptr) {
            destination = cell;
            structure = struct_it->second;
            structure->occupant = this;
            return true;
          }
          return false;
        }
        
        void ReturnWork() {
          if (plan) {
            plan->assignee = nullptr;
            plan = nullptr;
          }
          if (structure) {
            structure->occupant = nullptr;
            structure = nullptr;
          }
        }
        
        void GoToWork(const Point& waypoint) {
          int dy = limit_abs<i64>(waypoint.y - pos.y, 3);
          int dx = limit_abs<i64>(waypoint.x - pos.x, 5);
          pos.y += dy; pos.x += dx;
          if (Cell(waypoint) == destination) {
            if (!CanTravel(destination)) {
              AABB dwarf_bb = AABB(*this);
              AABB dest_bb = AABB(destination);
              if((dx > 0) && (dwarf_bb.right >= dest_bb.left)) {
                pos.x -= dwarf_bb.right - dest_bb.left + 1; dx = 0;
              }
              if((dx < 0) && (dwarf_bb.left <= dest_bb.right)) {
                pos.x += dest_bb.right - dwarf_bb.left + 1; dx = 0;
              }
              
              if((dy > 0) && (dwarf_bb.bottom >= dest_bb.top)) {
                pos.y -= dwarf_bb.bottom - dest_bb.top + 1; dy = 0;
              }
              if((dy < 0) && (dwarf_bb.top <= dest_bb.bottom)) {
                pos.y += dest_bb.bottom - dwarf_bb.top + 1; dy = 0;
              }
            }
            if(plan && dx == 0 && dy == 0) {
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
    
    AABB::AABB(const Dwarf& dwarf)
      : left(dwarf.pos.x - Dwarf::width / 2)
      , right(dwarf.pos.x + Dwarf::width / 2)
      , top(dwarf.pos.y - Dwarf::height)
      , bottom(dwarf.pos.y) {}

    set < Dwarf * >dwarves;
    
    // TODO: preferential weighing of distances
    
    void Tick() {
      map<Dwarf*, map<Cell, Cell>> shortest_path_tree;
      multimap<double, tuple<Dwarf*, Cell, Cell>> Q; 
      for (Dwarf * d : dwarves) {
        auto pos = d->pos;
        if (d->TakeWorkAt(pos)) { // skip search if already "standing" on a job
          d->GoToWork(Waypoint(pos));
        } else {
          Q.insert(make_pair(0, make_tuple(d, pos, pos)));
        }
      }
      int search_counter = 0;
      while(!Q.empty()) {
        auto p = Q.begin();
        double dist = p->first;
        Dwarf* dwarf = get<0>(p->second);
        Cell current = get<1>(p->second);
        Cell source = get<2>(p->second);
        Q.erase(p);
        if (dwarf->plan || dwarf->structure) continue;
        auto& visited = shortest_path_tree[dwarf];
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
          if ((!is_below || is_staircase_planned) && dwarf->TakeWorkAt(next)) {
            // add the next cell to the bfs tree regardless of reachability
            visited[next] = current;
            source = current; current = next; 
            // backtrack through bfs tree
            Cell start = Cell(dwarf->pos);
            while (source != start) {
                auto p = visited.find(source);
                current = p->first;
                source = p->second;
            }
            Point first = Waypoint(source); // cell where the dwarf is standing currently
            Point second = Waypoint(current); // next cell in the path
            // prevent moving backwards by looking one waypoint ahead
            i64 block_dist = first.MetroDist(second);
            i64 my_dist = dwarf->pos.MetroDist(second);
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
          Q.insert(make_pair(dist, make_tuple(dwarf, next, current)));
          return false;
        };
        if (++search_counter > 1000) break;
        if (Peek({ current.row, current.col + 1 })) continue;
        if ((current.col > 0) && Peek({ current.row, current.col - 1 })) continue;
        if (Peek({ current.row + 1, current.col })) continue;
        if ((current.row > 0) && Peek({ current.row - 1, current.col })) continue;
      }
      
      for (Dwarf * d : dwarves) d->ReturnWork();
    }
}

#endif //BUNKERBUILDER_GAME_BASE_H