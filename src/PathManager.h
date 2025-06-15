#pragma once

#include <vector>
#include <string>
#include <utility>
#include "Types.h"

class PathManager {
public:
    PathManager() = default;
    
    void loadPaths(const std::string& path, bool lacamFormat);
    void clear();
    void setPaths(const AgentPaths& paths) { m_agentGridPaths = paths; }
    
    const AgentPaths& getPaths() const { return m_agentGridPaths; }
    
private:
    AgentPaths m_agentGridPaths;
};