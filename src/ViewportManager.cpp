#include "ViewportManager.h"
#include <algorithm>

ViewportManager::ViewportManager()
    : m_panOffset(0.0f, 0.0f), m_targetPanOffset(0.0f, 0.0f), m_zoom(1.0f), m_zoomTarget(1.0f),
      m_lastClickTime(0.0f), m_doubleTapDetected(false), m_touchDragging(false),
      m_touchDragStart(0.0f, 0.0f)
{
}

void ViewportManager::handleInput(const ImGuiIO& io, bool isHovered, const ImVec2& canvasPos)
{
    if (!isHovered) {
        // Reset touchpad state when mouse leaves the canvas
        if (m_touchDragging) {
            m_touchDragging = false;
        }
        return;
    }

    // Handle panning with right mouse button
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
        ImVec2 delta = io.MouseDelta;
        m_panOffset.x += delta.x;
        m_panOffset.y += delta.y;
        m_targetPanOffset = m_panOffset;
    }

    // Handle single-tap and drag for touchpad panning
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        // Single-tap is enough to start potential drag
        m_doubleTapDetected = true;
        m_touchDragStart = io.MousePos;
    }

    // Start touchpad dragging on tap + hold
    if (m_doubleTapDetected && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        if (!m_touchDragging && ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x > 3.0f) {
            m_touchDragging = true;
        }
    }

    // Handle touchpad dragging
    if (m_touchDragging && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        ImVec2 delta = io.MouseDelta;
        m_panOffset.x += delta.x;
        m_panOffset.y += delta.y;
        m_targetPanOffset = m_panOffset;
    }

    // Reset touchpad dragging state when mouse is released
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        m_touchDragging = false;
        if (m_doubleTapDetected) {
            m_doubleTapDetected = false;
        }
    }

    // Handle zooming with mouse wheel
    float wheel = io.MouseWheel;
    if (wheel != 0.0f) {
        ImVec2 mouse = ImGui::GetMousePos();

        // Calculate world position before zoom
        ImVec2 worldBefore = ImVec2((mouse.x - canvasPos.x - m_panOffset.x) / m_zoom,
                                    (mouse.y - canvasPos.y - m_panOffset.y) / m_zoom);

        // Calculate new zoom level
        float newZoom = std::clamp(m_zoomTarget * (1.0f + wheel * ZOOM_SPEED), ZOOM_MIN, ZOOM_MAX);

        // Calculate screen position after zoom
        ImVec2 screenAfter = ImVec2(worldBefore.x * newZoom + canvasPos.x + m_targetPanOffset.x,
                                    worldBefore.y * newZoom + canvasPos.y + m_targetPanOffset.y);

        // Adjust pan offset to keep mouse position fixed
        m_targetPanOffset.x += mouse.x - screenAfter.x;
        m_targetPanOffset.y += mouse.y - screenAfter.y;
        m_zoomTarget = newZoom;
    }
}

void ViewportManager::update(float /*deltaTime*/)
{
    // Smooth interpolation for pan and zoom
    m_panOffset.x += (m_targetPanOffset.x - m_panOffset.x) * SMOOTH_FACTOR;
    m_panOffset.y += (m_targetPanOffset.y - m_panOffset.y) * SMOOTH_FACTOR;
    m_zoom += (m_zoomTarget - m_zoom) * SMOOTH_FACTOR;
}

ImVec2 ViewportManager::getOrigin(const ImVec2& canvasPos) const
{
    return ImVec2(canvasPos.x + m_panOffset.x, canvasPos.y + m_panOffset.y);
}

void ViewportManager::reset()
{
    m_panOffset = ImVec2(0.0f, 0.0f);
    m_targetPanOffset = m_panOffset;
    m_zoom = 1.0f;
    m_zoomTarget = m_zoom;

    // Reset touchpad interaction state
    m_doubleTapDetected = false;
    m_touchDragging = false;
}

void ViewportManager::fitTo(const ImVec2& canvasSize, const ImVec2& worldMin,
                            const ImVec2& worldMax, bool immediate)
{
    constexpr float FILL = 0.94f; // leave a thin border around the map
    const float w = std::max(worldMax.x - worldMin.x, 1.0f);
    const float h = std::max(worldMax.y - worldMin.y, 1.0f);
    const float zoom =
        std::clamp(FILL * std::min(canvasSize.x / w, canvasSize.y / h), ZOOM_MIN, ZOOM_MAX);

    m_zoomTarget = zoom;
    m_targetPanOffset = ImVec2(canvasSize.x * 0.5f - zoom * (worldMin.x + w * 0.5f),
                               canvasSize.y * 0.5f - zoom * (worldMin.y + h * 0.5f));
    if (immediate) {
        m_zoom = m_zoomTarget;
        m_panOffset = m_targetPanOffset;
    }
}