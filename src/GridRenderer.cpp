#include "GridRenderer.h"
#include "MapFormat.h"
#include <algorithm>
#include <cmath>

void GridRenderer::drawSquareGrid(ImDrawList* draw_list, const ImVec2& origin,
                                  const ImVec2& canvas_pos, const ImVec2& canvas_end,
                                  float tile_size, float zoom, const Grid<int>& grid)
{
    // Early exit if tiles would be too small or too large to render efficiently
    if (!SpatialGrid::shouldRenderTile(zoom, tile_size)) {
        return;
    }

    ImU32 grid_color = IM_COL32(200, 200, 200, 40);
    size_t cols = grid.width();
    size_t rows = grid.height();
    float scaled_tile = tile_size * zoom;
    float half = scaled_tile * 0.5f;

    // Calculate visible region for culling
    ImVec2 canvas_size = ImVec2(canvas_end.x - canvas_pos.x, canvas_end.y - canvas_pos.y);
    auto viewport = SpatialGrid::calculateVisibleRegion(canvas_pos, canvas_size, origin, tile_size,
                                                        zoom, cols, rows);

    // Level of detail based on zoom - now much more conservative
    // Set enable_lod = false to completely disable LOD for maximum visual quality
    bool enable_lod = true; // Changed to false for better visual experience
    int lod = SpatialGrid::calculateLOD(zoom, enable_lod);
    int step = (lod == 0) ? 1 : 2; // Only use step=1 or step=2, never higher

    // Always render all visible tiles to maintain grid visibility
    for (int x = viewport.min_x; x <= viewport.max_x; x += step) {
        for (int y = viewport.min_y; y <= viewport.max_y; y += step) {
            if (x >= static_cast<int>(cols) || y >= static_cast<int>(rows))
                continue;

            // Center of each cell
            ImVec2 center = ImVec2(origin.x + x * scaled_tile, origin.y + y * scaled_tile);

            ImVec2 top_left = ImVec2(center.x - half, center.y - half);
            ImVec2 bottom_right = ImVec2(center.x + half, center.y + half);

            // Always draw grid lines to maintain visibility
            // Only reduce opacity slightly at very low zoom levels
            ImU32 current_grid_color =
                (lod > 0 && scaled_tile < 4.0f) ? IM_COL32(200, 200, 200, 20) : grid_color;
            draw_list->AddRect(top_left, bottom_right, current_grid_color);

            int v = grid.get(x, y);
            if (v == MapFormat::Obstacle)
                draw_list->AddRectFilled(top_left, bottom_right, IM_COL32(0, 0, 0, 150));
            else if (MapFormat::isBlocked(v))
                draw_list->AddRectFilled(top_left, bottom_right,
                                         IM_COL32(34, 100, 34, 150)); // terrain (T/S/W/O)
        }
    }
}

void GridRenderer::drawHexGrid(ImDrawList* draw_list, const ImVec2& origin, float zoom,
                               const Grid<int>& grid, float tile_size, HexGridType gridType)
{
    float sqrt3 = std::sqrt(3.0f);
    float r = tile_size * zoom;
    bool isPointy = (gridType == HexGridType::ODD_R || gridType == HexGridType::EVEN_R);

    for (size_t row = 0; row < grid.height(); ++row) {
        for (size_t col = 0; col < grid.width(); ++col) {
            ImVec2 center;

            // Calculate center position based on grid type
            switch (gridType) {
            case HexGridType::ODD_R: // Pointy top, odd rows shifted
                center.x = origin.x + r * sqrt3 * (col + 0.5f * (row & 1));
                center.y = origin.y + r * 1.5f * row;
                break;

            case HexGridType::EVEN_R: // Pointy top, even rows shifted
                center.x = origin.x + r * sqrt3 * (col + 0.5f * (!(row & 1)));
                center.y = origin.y + r * 1.5f * row;
                break;

            case HexGridType::ODD_Q: // Flat top, odd columns shifted
                center.x = origin.x + r * 1.5f * col;
                center.y = origin.y + r * sqrt3 * (row + 0.5f * (col & 1));
                break;

            case HexGridType::EVEN_Q: // Flat top, even columns shifted
                center.x = origin.x + r * 1.5f * col;
                center.y = origin.y + r * sqrt3 * (row + 0.5f * (!(col & 1)));
                break;
            }

            // Calculate hex corners
            ImVec2 hex[6];
            for (int i = 0; i < 6; ++i) {
                float angle_deg;
                if (isPointy) {
                    // Pointy top hexagon (odd-r, even-r)
                    angle_deg = 60.0f * i - 30.0f;
                } else {
                    // Flat top hexagon (odd-q, even-q)
                    angle_deg = 60.0f * i;
                }

                float angle_rad = angle_deg * M_PI / 180.0f;
                hex[i] = ImVec2(center.x + r * cosf(angle_rad), center.y + r * sinf(angle_rad));
            }

            // Draw hex outline
            draw_list->AddPolyline(hex, 6, IM_COL32(200, 200, 200, 60), true, 1.0f);

            // Fill based on cell value
            int v = grid.get(col, row);
            if (v == MapFormat::Obstacle)
                draw_list->AddConvexPolyFilled(hex, 6, IM_COL32(0, 0, 0, 150));
            else if (MapFormat::isBlocked(v))
                draw_list->AddConvexPolyFilled(hex, 6,
                                               IM_COL32(34, 100, 34, 150)); // terrain (T/S/W/O)
        }
    }
}

ImVec2 GridRenderer::gridToWorld(size_t x, size_t y, bool isHex, float tileSize,
                                 const ImVec2& origin, float zoom, HexGridType gridType)
{
    if (!isHex) {
        return ImVec2(origin.x + x * tileSize * zoom, origin.y + y * tileSize * zoom);
    }

    float sqrt3 = std::sqrt(3.0f);
    float r = tileSize;

    // Calculate position based on grid type
    switch (gridType) {
    case HexGridType::ODD_R: // Pointy top, odd rows shifted
        return ImVec2(origin.x + zoom * (r * sqrt3 * (x + 0.5f * (y & 1))),
                      origin.y + zoom * (r * 1.5f * y));

    case HexGridType::EVEN_R: // Pointy top, even rows shifted
        return ImVec2(origin.x + zoom * (r * sqrt3 * (x + 0.5f * (!(y & 1)))),
                      origin.y + zoom * (r * 1.5f * y));

    case HexGridType::ODD_Q: // Flat top, odd columns shifted
        return ImVec2(origin.x + zoom * (r * 1.5f * x),
                      origin.y + zoom * (r * sqrt3 * (y + 0.5f * (x & 1))));

    case HexGridType::EVEN_Q: // Flat top, even columns shifted
        return ImVec2(origin.x + zoom * (r * 1.5f * x),
                      origin.y + zoom * (r * sqrt3 * (y + 0.5f * (!(x & 1)))));

    default:
        // Default to ODD_R if unknown type
        return ImVec2(origin.x + zoom * (r * sqrt3 * (x + 0.5f * (y & 1))),
                      origin.y + zoom * (r * 1.5f * y));
    }
}

Coord GridRenderer::pixelToHex(const ImVec2& pos, float tile_size, HexGridType gridType)
{
    const float sqrt3 = 1.7320508f;
    float q, r, s;

    // Different conversion formulas based on hex orientation
    bool isPointy = (gridType == HexGridType::ODD_R || gridType == HexGridType::EVEN_R);

    if (isPointy) {
        // Pointy top hexagon (odd-r, even-r)
        q = (sqrt3 / 3.0f * pos.x - 1.0f / 3.0f * pos.y) / tile_size;
        r = (2.0f / 3.0f * pos.y) / tile_size;
    } else {
        // Flat top hexagon (odd-q, even-q)
        q = (2.0f / 3.0f * pos.x) / tile_size;
        r = (sqrt3 / 3.0f * pos.y - 1.0f / 3.0f * pos.x) / tile_size;
    }

    s = -q - r;

    // Round cube coordinates
    int rx = static_cast<int>(std::round(q));
    int ry = static_cast<int>(std::round(r));
    int rz = static_cast<int>(std::round(s));

    float dx = std::abs(rx - q);
    float dy = std::abs(ry - r);
    float dz = std::abs(rz - s);

    if (dx > dy && dx > dz)
        rx = -ry - rz;
    else if (dy > dz)
        ry = -rx - rz;
    else
        rz = -rx - ry;

    // Convert cube to offset based on grid type
    int col, row;

    switch (gridType) {
    case HexGridType::ODD_R:
        col = rx + (ry - (ry & 1)) / 2;
        row = ry;
        break;

    case HexGridType::EVEN_R:
        col = rx + (ry + (ry & 1)) / 2;
        row = ry;
        break;

    case HexGridType::ODD_Q:
        col = rx;
        row = ry + (rx - (rx & 1)) / 2;
        break;

    case HexGridType::EVEN_Q:
        col = rx;
        row = ry + (rx + (rx & 1)) / 2;
        break;

    default:
        // Default to ODD_R
        col = rx + (ry - (ry & 1)) / 2;
        row = ry;
    }

    return CoordUtils::make_coord(col, row);
}

ImU32 getColorForAgent(size_t i)
{
    static const ImU32 colors[] = {
        IM_COL32(255, 0, 0, 255),   IM_COL32(0, 255, 0, 255),   IM_COL32(0, 0, 255, 255),
        IM_COL32(255, 255, 0, 255), IM_COL32(255, 0, 255, 255), IM_COL32(0, 255, 255, 255),
        IM_COL32(255, 128, 0, 255), IM_COL32(128, 0, 255, 255), IM_COL32(0, 128, 255, 255),
    };
    return colors[i % (sizeof(colors) / sizeof(colors[0]))];
}