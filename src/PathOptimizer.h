#pragma once

#include "Types.h"
#include <vector>
#include <unordered_set>
#include <algorithm>

// Optimizations for agent path processing and rendering
class PathOptimizer {
public:
    // Remove duplicate consecutive coordinates in paths
    static void removeDuplicates(AgentPaths& paths) {
        for (auto& path : paths) {
            if (path.empty()) continue;
            
            auto new_end = std::unique(path.begin(), path.end(),
                [](const Coord& a, const Coord& b) {
                    return CoordUtils::x(a) == CoordUtils::x(b) && 
                           CoordUtils::y(a) == CoordUtils::y(b);
                });
            path.erase(new_end, path.end());
        }
    }
    
    // Pre-compute path bounds for culling
    struct PathBounds {
        int min_x, min_y, max_x, max_y;
        
        bool intersects(int view_min_x, int view_min_y, int view_max_x, int view_max_y) const {
            return !(max_x < view_min_x || min_x > view_max_x || 
                     max_y < view_min_y || min_y > view_max_y);
        }
    };
    
    static std::vector<PathBounds> computePathBounds(const AgentPaths& paths) {
        std::vector<PathBounds> bounds;
        bounds.reserve(paths.size());
        
        for (const auto& path : paths) {
            if (path.empty()) {
                bounds.push_back({0, 0, 0, 0});
                continue;
            }
            
            PathBounds bound;
            bound.min_x = bound.max_x = CoordUtils::x(path[0]);
            bound.min_y = bound.max_y = CoordUtils::y(path[0]);
            
            for (const auto& coord : path) {
                int x = CoordUtils::x(coord);
                int y = CoordUtils::y(coord);
                bound.min_x = std::min(bound.min_x, x);
                bound.max_x = std::max(bound.max_x, x);
                bound.min_y = std::min(bound.min_y, y);
                bound.max_y = std::max(bound.max_y, y);
            }
            
            bounds.push_back(bound);
        }
        
        return bounds;
    }
    
    // Spatial indexing for collision detection
    class SpatialIndex {
    public:
        void buildIndex(const AgentPaths& paths, size_t time_step) {
            m_positions.clear();
            
            for (size_t agent = 0; agent < paths.size(); ++agent) {
                if (time_step < paths[agent].size()) {
                    const auto& coord = paths[agent][time_step];
                    uint64_t key = spatialKey(CoordUtils::x(coord), CoordUtils::y(coord));
                    m_positions[key].push_back(agent);
                }
            }
        }
        
        std::vector<size_t> getAgentsAt(int x, int y) const {
            uint64_t key = spatialKey(x, y);
            auto it = m_positions.find(key);
            return it != m_positions.end() ? it->second : std::vector<size_t>();
        }
        
    private:
        std::unordered_map<uint64_t, std::vector<size_t>> m_positions;
        
        static uint64_t spatialKey(int x, int y) {
            return (static_cast<uint64_t>(x) << 32) | static_cast<uint64_t>(y);
        }
    };
    
    // Compress paths by removing intermediate waypoints on straight lines
    static void compressPaths(AgentPaths& paths) {
        for (auto& path : paths) {
            if (path.size() <= 2) continue;
            
            Path compressed;
            compressed.reserve(path.size());
            compressed.push_back(path[0]);
            
            for (size_t i = 1; i < path.size() - 1; ++i) {
                int x1 = CoordUtils::x(path[i-1]);
                int y1 = CoordUtils::y(path[i-1]);
                int x2 = CoordUtils::x(path[i]);
                int y2 = CoordUtils::y(path[i]);
                int x3 = CoordUtils::x(path[i+1]);
                int y3 = CoordUtils::y(path[i+1]);
                
                // Check if points are collinear
                if ((y2 - y1) * (x3 - x2) != (y3 - y2) * (x2 - x1)) {
                    compressed.push_back(path[i]);
                }
            }
            
            compressed.push_back(path.back());
            path = std::move(compressed);
        }
    }
};