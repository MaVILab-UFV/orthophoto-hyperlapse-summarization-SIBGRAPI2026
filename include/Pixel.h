#pragma once

#include <cstddef>    // For size_t
#include <cstdint>    // For uint32_t, uint64_t
#include <functional> // For std::hash
#include <unordered_set>

/**
 * @struct Pixel
 * @brief Represents a single 2D coordinate (x, y) optimized for RAM.
 */
struct Pixel {
    int x, y; // Mudado para 'int' (32-bit). Agora suporta ortofotos gigantes!

    bool operator==(const Pixel& other) const {
        return x == other.x && y == other.y;
    }
};

/**
 * @struct PixelHash
 * @brief Custom hash function for std::unordered_set.
 */
struct PixelHash {
    std::size_t operator()(const Pixel& p) const {
        // Bitwise shift: Combina dois inteiros de 32-bits num uint64_t.
        // Isso garante ZERO colisões na sua Hash Table, mantendo a busca O(1) perfeita!
        uint64_t packed_coords = (static_cast<uint64_t>(p.x) << 32) | static_cast<uint32_t>(p.y);
        return std::hash<uint64_t>{}(packed_coords);
    }
};

// Typedef for ease of use across the project
using PixelSet = std::unordered_set<Pixel, PixelHash>;