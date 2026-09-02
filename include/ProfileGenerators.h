#pragma once

#include "IProfileGenerator.h"

/**
 * @class StepProfileGenerator
 * @brief Generates a "step function" profile.
 * Frames that were selected (score > 0) get a high value (1.0),
 * others get a low value (0.0).
 */
class StepProfileGenerator : public IProfileGenerator {
public:
    std::vector<double> generateProfile(
        const HeuristicResult& result) const override;
};


/**
 * @class SmoothProfileGenerator
 * @brief Generates a "smooth" profile.
 * Normalizes the raw scores and applies a simple
 * moving average (smoothing) filter.
 */
class SmoothProfileGenerator : public IProfileGenerator {
public:
    std::vector<double> generateProfile(
        const HeuristicResult& result) const override;
};

class NormalizedProfileGenerator : public IProfileGenerator {
    public:
    std::vector<double> generateProfile(const HeuristicResult& result) const override;
};