#pragma once

#include "Grid.hpp"
#include "HexGridType.h"
#include "SpatialGrid.h"
#include "Types.h"
#include "imgui.h"

class GridRenderer
{
public:
    static void drawSquareGrid(ImDrawList* draw_list, const ImVec2& origin,
                               const ImVec2& canvas_pos, const ImVec2& canvas_end, float tile_size,
                               float zoom, const Grid<int>& grid);

    static void drawHexGrid(ImDrawList* draw_list, const ImVec2& origin, float zoom,
                            const Grid<int>& grid, float tile_size, HexGridType gridType);

    static ImVec2 gridToWorld(size_t x, size_t y, bool isHex, float tileSize, const ImVec2& origin,
                              float zoom, HexGridType gridType = HexGridType::ODD_R);

    static Coord pixelToHex(const ImVec2& pos, float tile_size, HexGridType gridType);
};

// Color utility function
ImU32 getColorForAgent(size_t i);