#ifndef BUNKERBUILDER_GAME_BASE_H
#define BUNKERBUILDER_GAME_BASE_H
#include <cstdio>
#include <map>
#include <set>
#include <deque>
#include <vector>
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
    };

    Cell::Cell(const Point &point) : row(div_floor(point.y, H)), col(div_floor(point.x, W)) { }

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

    struct Dwarf {
        string name;
        Point pos;
        static Dwarf *MakeRandom() {
            Dwarf *d = new Dwarf();
            d->name = namegen::gen();
            d->Say("Hello!");
            return d;
        }

        void Say(string text) {
            printf("%s: %s\n", name.c_str(), text.c_str());
        }

        void Move() {
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

    set < Dwarf * >dwarves;
}

#endif //BUNKERBUILDER_GAME_BASE_H
