#pragma once

// Hexagonal grid layout variants (offset-coordinate conventions).
// Lives in its own header so core (non-GUI) code such as Environment and the
// solver drivers can use it without pulling in ImGui via GridRenderer.h.
enum class HexGridType {
    ODD_R,  // Pointy top, odd rows shifted
    EVEN_R, // Pointy top, even rows shifted
    ODD_Q,  // Flat top, odd columns shifted
    EVEN_Q  // Flat top, even columns shifted
};
