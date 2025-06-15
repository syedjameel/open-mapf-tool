#pragma once

#include "imgui.h"
#include <unordered_map>
#include <cmath>

// Cache for frequently computed rendering values
class RenderCache {
public:
    struct CacheKey {
        float zoom;
        float tile_size;
        int grid_width;
        int grid_height;
        
        bool operator==(const CacheKey& other) const {
            return std::abs(zoom - other.zoom) < 1e-6f &&
                   std::abs(tile_size - other.tile_size) < 1e-6f &&
                   grid_width == other.grid_width &&
                   grid_height == other.grid_height;
        }
    };
    
    struct CachedValues {
        float scaled_tile;
        float half_tile;
        int lod_level;
        int step_size;
        bool should_render;
        
        // Hex-specific cached values
        float hex_width;
        float hex_height;
        float hex_vertical_spacing;
        float hex_horizontal_spacing;
    };
    
    static RenderCache& getInstance() {
        static RenderCache instance;
        return instance;
    }
    
    const CachedValues& getCachedValues(const CacheKey& key) {
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            return it->second;
        }
        
        // Compute new cached values
        CachedValues values;
        values.scaled_tile = key.tile_size * key.zoom;
        values.half_tile = values.scaled_tile * 0.5f;
        values.lod_level = calculateLOD(key.zoom);
        values.step_size = 1 << values.lod_level;
        values.should_render = shouldRenderTile(key.zoom, key.tile_size);
        
        // Hex-specific calculations
        float sqrt3 = std::sqrt(3.0f);
        values.hex_width = key.tile_size * 2.0f;
        values.hex_height = sqrt3 * key.tile_size;
        values.hex_vertical_spacing = values.hex_height * 0.75f;
        values.hex_horizontal_spacing = values.hex_width;
        
        m_cache[key] = values;
        return m_cache[key];
    }
    
    void clearCache() {
        m_cache.clear();
    }
    
    size_t getCacheSize() const {
        return m_cache.size();
    }

private:
    std::unordered_map<CacheKey, CachedValues> m_cache;
    
    static int calculateLOD(float zoom) {
        if (zoom >= 0.1f) return 0;     // Full detail for most zoom levels
        if (zoom >= 0.05f) return 1;    // Slight optimization only at very low zoom  
        return 1;                       // Never go beyond level 1 to maintain visibility
    }
    
    static bool shouldRenderTile(float zoom, float tile_size) {
        float screen_tile_size = tile_size * zoom;
        return screen_tile_size >= 0.5f && screen_tile_size <= 2000.0f;
    }
};

// Hash function for CacheKey
namespace std {
    template<>
    struct hash<RenderCache::CacheKey> {
        size_t operator()(const RenderCache::CacheKey& key) const {
            size_t h1 = std::hash<float>{}(key.zoom);
            size_t h2 = std::hash<float>{}(key.tile_size);
            size_t h3 = std::hash<int>{}(key.grid_width);
            size_t h4 = std::hash<int>{}(key.grid_height);
            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
        }
    };
}