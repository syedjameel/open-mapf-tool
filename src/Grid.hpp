#pragma once

#include <vector>
#include <cstddef>
#include <stdexcept>

template <typename T>
class Grid
{
    size_t m_width;
    size_t m_height;

    std::vector<std::vector<T>> m_grid;

public:
    Grid() {}
    Grid(size_t width, size_t height, T val)
        : m_width(width), m_height(height), m_grid(width, std::vector<T>(height, val))
    {
    }

    T &get(size_t x, size_t y)
    {
        if (x >= m_width || y >= m_height) {
            throw std::out_of_range("Grid access out of bounds");
        }
        return m_grid[x][y];
    }

    const T &get(size_t x, size_t y) const
    {
        if (x >= m_width || y >= m_height) {
            throw std::out_of_range("Grid access out of bounds");
        }
        return m_grid[x][y];
    }

    void set(size_t x, size_t y, T val)
    {
        if (x >= m_width || y >= m_height) {
            throw std::out_of_range("Grid access out of bounds");
        }
        m_grid[x][y] = val;
    }

    size_t width() const { return m_width; }
    size_t height() const { return m_height; }
};
