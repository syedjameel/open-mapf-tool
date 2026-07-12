#include "GridInteractionHandler.h"
#include "MapFormat.h"

GridInteractionHandler::GridInteractionHandler(Environment& env) : m_env(env) {}

void GridInteractionHandler::handleInteraction(const ImVec2& mousePos, const ImVec2& origin,
                                               float zoom, bool isHovered, bool isLeftClicked,
                                               const std::vector<Agent>& agents)
{
    if (!isHovered || !isLeftClicked)
        return;

    float tileSize = m_env.m_square ? m_env.m_square_tile_size : m_env.m_hex_tile_size;
    float scaledTile = tileSize * zoom;

    size_t col = 0, row = 0;

    if (m_env.m_square) {
        float mouseX = (mousePos.x - origin.x + (scaledTile * 0.5f)) / scaledTile;
        float mouseY = (mousePos.y - origin.y + (scaledTile * 0.5f)) / scaledTile;
        // Clicks left of / above the grid produce negative values; casting
        // a negative float to size_t is undefined behavior, so reject first.
        if (mouseX < 0.0f || mouseY < 0.0f)
            return;
        col = static_cast<size_t>(mouseX);
        row = static_cast<size_t>(mouseY);
    } else {
        // Convert ImGui mouse to grid-space in pixels (before hex conversion)
        ImVec2 worldMousePos = {(mousePos.x - origin.x) / zoom, (mousePos.y - origin.y) / zoom};

        Coord hexCoord = GridRenderer::pixelToHex(worldMousePos, tileSize, m_env.m_hex_grid_type);
        if (CoordUtils::x(hexCoord) < 0 || CoordUtils::y(hexCoord) < 0)
            return;
        col = static_cast<size_t>(CoordUtils::x(hexCoord));
        row = static_cast<size_t>(CoordUtils::y(hexCoord));
    }

    if (col < m_env.m_cols && row < m_env.m_rows) // Check for valid coordinates
    {
        Coord clickedPos = CoordUtils::make_coord(static_cast<int>(col), static_cast<int>(row));

        // Check if there's an agent at this position - if so, don't place/remove obstacle
        if (hasAgentAt(clickedPos, agents)) {
            return; // Don't modify grid if agent is present
        }

        int val = m_env.m_grid.get(col, row);
        // Toggle: any blocked terrain becomes free, free becomes an obstacle
        m_env.m_grid.set(col, row,
                         MapFormat::isBlocked(val) ? MapFormat::Free : MapFormat::Obstacle);
    }
}

bool GridInteractionHandler::hasAgentAt(const Coord& pos, const std::vector<Agent>& agents) const
{
    for (const auto& agent : agents) {
        // Check if position matches agent's start position
        if (CoordUtils::x(agent.start) == CoordUtils::x(pos) &&
            CoordUtils::y(agent.start) == CoordUtils::y(pos)) {
            return true;
        }

        // Check if position matches agent's goal position (if it has one)
        if (agent.hasGoal && CoordUtils::x(agent.goal) == CoordUtils::x(pos) &&
            CoordUtils::y(agent.goal) == CoordUtils::y(pos)) {
            return true;
        }
    }

    return false;
}