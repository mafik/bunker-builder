#ifndef BUNKERBUILDER_GAME_BASE_H
#define BUNKERBUILDER_GAME_BASE_H

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

    struct Structure {
        StructureType type;
    };

    struct Staircase : Structure {
        Staircase() {
            type = STAIRCASE;
        }
    };

    struct Corridor : Structure {
        Corridor() {
            type = CORRIDOR;
        }
    };

    struct Workshop : Structure {
        Workshop() {
            type = WORKSHOP;
        }
    };

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
    
    struct Dwarf;
    struct AABB {
      i64 left, right, top, bottom; // bottom >= top (y axis grows downwards)
      AABB(const Dwarf&);
      AABB(const Cell& c) : left(c.col * W), right((c.col + 1) * W - 1), top(c.row * H), bottom((c.row + 1) * H - 1) {}
    };

    map < Cell, Structure * >cells;
    map < Cell, StructureType > plans;

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
        static Dwarf *MakeRandom() {
            Dwarf *d = new Dwarf();
            d->name = namegen::gen();
            d->Say("Hello!");
            d->pos = Waypoint(Cell(0,2));
            return d;
        }

        void Say(string text) {
            printf("%s: %s\n", name.c_str(), text.c_str());
        }
        /** This will find the path to closest cell for which "is_destination" returns true.
            Then "next_step" will be called with the next cell as the argument. */
        bool Search(function<bool (const Cell&)> is_destination,
                    function<void (const Point&)> next_step) {
          Cell start = Cell(pos);
          map < Cell, Cell > visited;
          deque < pair < Cell, Cell >> Q;
          Q.push_back(make_pair(start, start));
          int search_counter = 0;
          while (!Q.empty()) {
              ++search_counter;
              if (search_counter > 100) {
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
              if (source.row == current.row + 1 && !CanTravelVertically(source)) continue;
              visited[current] = source;
              if (is_destination(current)) {
                  // backtrack through bfs tree
                  while (source != start) {
                      pair = *visited.find(source);
                      current = pair.first;
                      source = pair.second;
                  }
                  Point first = Waypoint(start);
                  Point second = Waypoint(current);
                  i64 block_dist = first.MetroDist(second);
                  i64 my_dist = pos.MetroDist(second);
                  if (my_dist <= block_dist) next_step(second);
                  else next_step(first);
                  return true;
              }
              if (source.row == current.row && !CanTravel(current)) continue;
              if (source.row == current.row - 1 && !CanTravelVertically(current)) continue;
              Cell right = { current.row, current.col + 1 };
              Cell left = { current.row, current.col - 1 };
              if (current.col > 0)
                  Q.push_back(make_pair(left, current));
              Q.push_back(make_pair(right, current));
              Cell above = { current.row - 1, current.col };
              Cell below = { current.row + 1, current.col };
              if (current.row > 0)
                Q.push_back(make_pair(above, current));
              Q.push_back(make_pair(below, current));
              
          }
          return false;
        }

        void Move() {
          Cell destination;
          auto MakeStep = [&](const Point& waypoint) {
            /** What do we have: current position & waypoint with a direct line of sight.
                2. Find the bounding box of the character.
                3. Check if any of the corners touches the destination.
                4. Find the bounding box of the destination.
                5. Move away just enough to touch the destination. */
            pos.y += limit_abs<i64>(waypoint.y - pos.y, 3);
            pos.x += limit_abs<i64>(waypoint.x - pos.x, 5);
          };
          bool work_found = Search([&](const Cell& cell) {
              bool ret = HasPlan(cell);
              if(ret) destination = cell;
              return ret;
            }, [&](const Point& waypoint) {
              int dy = limit_abs<i64>(waypoint.y - pos.y, 3);
              int dx = limit_abs<i64>(waypoint.x - pos.x, 5);
              pos.y += dy; pos.x += dx;
              if (Cell(waypoint) == destination) {
                if (!CanTravel(destination)) {
                  AABB dwarf_bb = AABB(*this);
                  AABB dest_bb = AABB(destination);
                  if((dx > 0) && (dwarf_bb.right >= dest_bb.left)) {
                    pos.x -= dwarf_bb.right - dest_bb.left + 1;
                  }
                  if((dx < 0) && (dwarf_bb.left <= dest_bb.right)) {
                    pos.x += dest_bb.right - dwarf_bb.left + 1;
                  }
                  
                  if((dy > 0) && (dwarf_bb.bottom >= dest_bb.top)) {
                    pos.y -= dwarf_bb.bottom - dest_bb.top + 1;
                  }
                  if((dy < 0) && (dwarf_bb.top <= dest_bb.bottom)) {
                    pos.y += dest_bb.bottom - dwarf_bb.top + 1;
                  }
                }
              }
            });
          if (!work_found) {
            Search([](const Cell& c) { return IsStructureType(c, WORKSHOP); }, MakeStep);
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
}

#endif //BUNKERBUILDER_GAME_BASE_H