#ifndef BUNKERBUILDER_GAME_BASE_H
#define BUNKERBUILDER_GAME_BASE_H

#include "utils.h"

namespace bb {

    constexpr i64 W = 100;
    constexpr i64 H = 200;

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

    enum Command {
        COMMAND_SELECT = 0, COMMAND_STAIRCASE, COMMAND_CORRIDOR, COMMAND_WORKSHOP
    };

    ///////////////
    // GLOBALS
    ///////////////


}

#endif //BUNKERBUILDER_GAME_BASE_H
