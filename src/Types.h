#pragma once

#include <array>
#include <vector>

// Performance-optimized coordinate types
using Coord = std::array<int, 2>;

// Utility functions for coordinate operations
namespace CoordUtils {
    constexpr Coord make_coord(int x, int y) {
        return {x, y};
    }
    
    constexpr int x(const Coord& coord) {
        return coord[0];
    }
    
    constexpr int y(const Coord& coord) {
        return coord[1];
    }
    
    constexpr void set_x(Coord& coord, int x) {
        coord[0] = x;
    }
    
    constexpr void set_y(Coord& coord, int y) {
        coord[1] = y;
    }
}

// Agent data structure for dynamic agent placement
struct Agent {
    size_t id;
    Coord start;
    Coord goal;
    bool hasGoal;
    
    Agent(size_t agent_id, const Coord& start_pos) 
        : id(agent_id), start(start_pos), goal{-1, -1}, hasGoal(false) {}
        
    Agent(size_t agent_id, const Coord& start_pos, const Coord& goal_pos) 
        : id(agent_id), start(start_pos), goal(goal_pos), hasGoal(true) {}
};

// Type aliases for path structures
using Path = std::vector<Coord>;
using AgentPaths = std::vector<Path>;