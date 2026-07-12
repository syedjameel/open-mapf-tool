#pragma once

#include "Types.h"
#include "imgui.h"
#include <utility>
#include <vector>

class AnimationController
{
public:
    static constexpr float STEP_INTERVAL = 0.4f;

    AnimationController();

    void reset();
    void start(const AgentPaths& paths);
    void update(float deltaTime);

    bool isAnimating() const { return m_animate; }
    void setAnimating(bool animate) { m_animate = animate; }

    float getAnimationSpeed() const { return m_animationSpeed; }
    void setAnimationSpeed(float speed) { m_animationSpeed = speed; }

    float getScrubPercent() const { return m_scrubPercent; }
    void setScrubPercent(float percent);

    const std::vector<size_t>& getPathIndices() const { return m_pathIndices; }

    // Resize the per-agent path indices to match the number of loaded paths
    // (new entries start at 0). Replaces external const_cast mutation.
    void syncWithPaths(size_t pathCount) { m_pathIndices.resize(pathCount, 0); }

    // Set a single agent's current path index (bounds-checked).
    void setPathIndex(size_t agent, size_t index)
    {
        if (agent < m_pathIndices.size()) {
            m_pathIndices[agent] = index;
        }
    }

    float getFrameTimer() const { return m_frameTimer; }

    bool showPaths() const { return m_showPaths; }
    void setShowPaths(bool show) { m_showPaths = show; }

    size_t getCurrentTimeStep() const { return m_currentTimeStep; }
    void setCurrentTimeStep(size_t step);
    size_t getMaxTimeStep() const { return m_maxTimeStep; }

private:
    std::vector<size_t> m_pathIndices;
    std::vector<std::vector<ImVec2>> m_agentTrailPoints;

    float m_animationTime = 0.0f;
    float m_frameTimer = 0.0f;
    float m_scrubPercent = 0.0f;
    float m_animationSpeed = 1.0f;

    size_t m_currentTimeStep = 0;
    size_t m_maxTimeStep = 0;

    bool m_animate = false;
    bool m_showPaths = true;
};