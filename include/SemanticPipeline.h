#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include "Pixel.h"
#include "RasterMask.h"
#include "IHeuristicCalculator.h" 

struct ColorRangeRGB {
    uint8_t r_min, r_max;
    uint8_t g_min, g_max;
    uint8_t b_min, b_max;
};

struct ColorRGB {
    uint8_t r, g, b;
    bool operator==(const ColorRGB& other) const;
};

struct ImageRGB {
    int width, height;
    std::vector<uint8_t> r_band, g_band, b_band;

    ImageRGB(int w, int h);
    ColorRGB get(int x, int y) const;
    void set(int x, int y, ColorRGB color);
};

class SemanticPipeline {
public:
    SemanticPipeline(const std::string& ortho_path, const std::string& frame_dir, const std::string& debug_dir = "");

    std::vector<HeuristicResult> run( ColorRangeRGB target_range, 
        const std::vector<IHeuristicCalculator*>& heuristics,
        const std::vector<std::string>& heuristic_names
    );
    
    const std::vector<std::string>& getDiscoveredFramePaths() const;

private:
    std::string m_semantic_ortho_path;
    std::string m_frame_directory_path;
    std::string m_debug_output_directory;
    bool m_debug_mode;
    std::vector<std::string> m_discovered_frame_paths;

    void discoverFrameMasks(const std::string& extension);
    ImageRGB loadSemanticOrtho(const std::string& path);
    RasterMask convertPixelSetToRaster(const PixelSet& set, int w, int h) const;
};