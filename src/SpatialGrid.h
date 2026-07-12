#pragma once

#include "Grid.hpp"
#include "imgui.h"
#include <cmath>
#include <vector>

// Spatial partitioning for efficient large grid rendering
class SpatialGrid
{
public:
    struct Viewport
    {
        int min_x, min_y;
        int max_x, max_y;

        bool contains(int x, int y) const
        {
            return x >= min_x && x <= max_x && y >= min_y && y <= max_y;
        }
    };

    // Calculate visible grid region based on viewport
    static Viewport calculateVisibleRegion(const ImVec2& canvas_pos, const ImVec2& canvas_size,
                                           const ImVec2& origin, float tile_size, float zoom,
                                           size_t grid_width, size_t grid_height)
    {
        float scaled_tile = tile_size * zoom;

        // Convert screen bounds to grid coordinates
        int min_x = static_cast<int>(std::floor((canvas_pos.x - origin.x) / scaled_tile));
        int min_y = static_cast<int>(std::floor((canvas_pos.y - origin.y) / scaled_tile));
        int max_x =
            static_cast<int>(std::ceil((canvas_pos.x + canvas_size.x - origin.x) / scaled_tile));
        int max_y =
            static_cast<int>(std::ceil((canvas_pos.y + canvas_size.y - origin.y) / scaled_tile));

        // Clamp to grid bounds
        min_x = std::max(0, std::min(min_x, static_cast<int>(grid_width) - 1));
        min_y = std::max(0, std::min(min_y, static_cast<int>(grid_height) - 1));
        max_x = std::max(0, std::min(max_x, static_cast<int>(grid_width) - 1));
        max_y = std::max(0, std::min(max_y, static_cast<int>(grid_height) - 1));

        return {min_x, min_y, max_x, max_y};
    }

    // Check if a tile should be rendered at current zoom level
    static bool shouldRenderTile(float zoom, float tile_size)
    {
        float screen_tile_size = tile_size * zoom;

        // Only skip rendering if tiles are extremely small (less than 0.5 pixels)
        if (screen_tile_size < 0.5f)
            return false;

        // Skip rendering if tiles are extremely large (off-screen likely)
        if (screen_tile_size > 2000.0f)
            return false;

        return true;
    }

    // Calculate appropriate level of detail based on zoom
    static int calculateLOD(float zoom, bool enable_lod = true)
    {
        if (!enable_lod)
            return 0; // Always full detail if LOD disabled
        if (zoom >= 0.1f)
            return 0; // Full detail for most zoom levels
        if (zoom >= 0.05f)
            return 1; // Slight optimization only at very low zoom
        return 1;     // Never go beyond level 1 to maintain visibility
    }
};