#ifndef HEURISTICS_H
#define HEURISTICS_H

#include "IHeuristicCalculator.h" 

class ClassicGreedyHeuristic : public IHeuristicCalculator {
public:
    HeuristicResult calculateScores(
        const PixelSet& universe, const std::vector<PixelSet>& subsets,
        const std::vector<double>& cam_centers_x, const std::vector<double>& cam_centers_y,
        const std::vector<size_t>& frame_areas,
        const std::unordered_map<Pixel, int, PixelHash>& pixel_to_comp_id) const override;
};

class SemanticAccelerationGT : public IHeuristicCalculator {
public:
    HeuristicResult calculateScores(
        const PixelSet& universe, const std::vector<PixelSet>& subsets,
        const std::vector<double>& cam_centers_x, const std::vector<double>& cam_centers_y,
        const std::vector<size_t>& frame_areas,
        const std::unordered_map<Pixel, int, PixelHash>& pixel_to_comp_id) const override;
};

class ContinuousUpsampling : public IHeuristicCalculator {
public:
    HeuristicResult calculateScores(
        const PixelSet& universe, const std::vector<PixelSet>& subsets,
        const std::vector<double>& cam_centers_x, const std::vector<double>& cam_centers_y,
        const std::vector<size_t>& frame_areas,
        const std::unordered_map<Pixel, int, PixelHash>& pixel_to_comp_id) const override;
};

class DiscreteUpsampling : public IHeuristicCalculator {
public:
    HeuristicResult calculateScores(
        const PixelSet& universe, const std::vector<PixelSet>& subsets,
        const std::vector<double>& cam_centers_x, const std::vector<double>& cam_centers_y,
        const std::vector<size_t>& frame_areas,
        const std::unordered_map<Pixel, int, PixelHash>& pixel_to_comp_id) const override;
};

#endif 