#pragma once

#include "imgui.h"

class ViewportManager
{
public:
    ViewportManager();

    void handleInput(const ImGuiIO& io, bool isHovered, const ImVec2& canvasPos);
    void update(float deltaTime);

    ImVec2 getOrigin(const ImVec2& canvasPos) const;
    float getZoom() const { return m_zoom; }

    void reset();

    // Frames the world-space box [worldMin, worldMax] (as laid out at zoom 1,
    // pan 0) centered in a canvas of canvasSize, with a small margin. The
    // move is animated by update() unless immediate is set.
    void fitTo(const ImVec2& canvasSize, const ImVec2& worldMin, const ImVec2& worldMax,
               bool immediate);

private:
    ImVec2 m_panOffset;
    ImVec2 m_targetPanOffset;
    float m_zoom;
    float m_zoomTarget;

    // Touchpad tap and drag support
    bool m_doubleTapDetected; // Now used for single-tap detection
    bool m_touchDragging;
    ImVec2 m_touchDragStart;

    static constexpr float ZOOM_MIN =
        0.05f; // must stay positive: 0 or negative zoom inverts/degenerates the view
    static constexpr float ZOOM_MAX = 10.0f;
    static constexpr float ZOOM_SPEED = 0.1f;
    static constexpr float SMOOTH_FACTOR = 0.15f;
};