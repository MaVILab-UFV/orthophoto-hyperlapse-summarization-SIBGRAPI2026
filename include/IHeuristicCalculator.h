#ifndef IHEURISTICCALCULATOR_H
#define IHEURISTICCALCULATOR_H

#include "Pixel.h"
#include <vector>
#include <unordered_map>

struct HeuristicResult {
    std::vector<double> scores;
    std::vector<int> selection_order;
    std::vector<PixelSet> contribution_masks;
};

class IHeuristicCalculator {
public:
    virtual ~IHeuristicCalculator() = default;

    virtual HeuristicResult calculateScores(
        const PixelSet& universe, 
        const std::vector<PixelSet>& subsets,
        const std::vector<double>& cam_centers_x, 
        const std::vector<double>& cam_centers_y,
        const std::vector<size_t>& frame_areas, // NOVO PARÂMETRO!
        const std::unordered_map<Pixel, int, PixelHash>& pixel_to_comp_id) const = 0;
};

#endif // IHEURISTICCALCULATOR_H