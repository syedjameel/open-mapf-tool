#pragma once

#include "Environment.h"
#include "GridRenderer.h"
#include "Types.h"
#include "imgui.h"

class GridInteractionHandler
{
public:
    GridInteractionHandler(Environment& env);

    void handleInteraction(const ImVec2& mousePos, const ImVec2& origin, float zoom, bool isHovered,
                           bool isLeftClicked, const std::vector<Agent>& agents);

private:
    Environment& m_env;

    bool hasAgentAt(const Coord& pos, const std::vector<Agent>& agents) const;
};