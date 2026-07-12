#include "AnimationController.h"
#include <algorithm>

AnimationController::AnimationController()
    : m_animationTime(0.0f), m_frameTimer(0.0f), m_scrubPercent(0.0f), m_animationSpeed(1.0f),
      m_animate(false), m_showPaths(true)
{
}

void AnimationController::reset()
{
    m_animate = false;
    m_pathIndices.clear();
    m_agentTrailPoints.clear();
    m_frameTimer = 0.0f;
    m_scrubPercent = 0.0f;
    m_currentTimeStep = 0;
    m_maxTimeStep = 0;
}

void AnimationController::start(const AgentPaths& paths)
{
    size_t num_agents = paths.size();

    m_animate = true;
    m_animationTime = 0.0f;
    m_frameTimer = 0.0f;
    m_currentTimeStep = 0;

    // Initialize step indices and trails
    m_pathIndices.assign(num_agents, 0);
    m_agentTrailPoints.clear();
    m_agentTrailPoints.resize(num_agents);

    // Find the maximum path length
    m_maxTimeStep = 0;

    // Store path lengths for animation updates
    for (size_t i = 0; i < paths.size(); ++i) {
        // For each agent, store the number of steps in their path
        // We'll use this to determine when the animation should end
        if (!paths[i].empty()) {
            m_agentTrailPoints[i].resize(paths[i].size());
            m_maxTimeStep = std::max(m_maxTimeStep, paths[i].size() > 0 ? paths[i].size() - 1 : 0);
        }
    }
}

void AnimationController::update(float deltaTime)
{
    if (!m_animate)
        return;

    m_frameTimer += deltaTime * m_animationSpeed;

    if (m_frameTimer >= STEP_INTERVAL) {
        m_frameTimer = 0.0f;

        // Increment the current time step
        if (m_currentTimeStep < m_maxTimeStep) {
            m_currentTimeStep++;
            m_scrubPercent =
                m_maxTimeStep > 0 ? static_cast<float>(m_currentTimeStep) / m_maxTimeStep : 0.0f;

            // Update each agent's position based on the current time step
            bool anyRemaining = false;
            for (size_t i = 0; i < m_pathIndices.size(); ++i) {
                // If the agent has more steps in its path and hasn't reached the current time step
                if (i < m_agentTrailPoints.size() &&
                    m_pathIndices[i] + 1 < m_agentTrailPoints[i].size() &&
                    m_pathIndices[i] < m_currentTimeStep) {
                    m_pathIndices[i]++;
                    if (m_pathIndices[i] + 1 < m_agentTrailPoints[i].size())
                        anyRemaining = true;
                }
            }

            if (!anyRemaining && m_currentTimeStep >= m_maxTimeStep) {
                m_animate = false;
                reset();
            }
        } else {
            m_animate = false;
            reset();
        }
    }
}

void AnimationController::setScrubPercent(float percent)
{
    m_scrubPercent = std::clamp(percent, 0.0f, 1.0f);

    // Update current time step based on scrub percent
    m_currentTimeStep = static_cast<size_t>(m_scrubPercent * m_maxTimeStep);
}

void AnimationController::setCurrentTimeStep(size_t step)
{
    m_currentTimeStep = std::min(step, m_maxTimeStep);

    // Update scrub percent for compatibility with existing code
    m_scrubPercent =
        m_maxTimeStep > 0 ? static_cast<float>(m_currentTimeStep) / m_maxTimeStep : 0.0f;
}