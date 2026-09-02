#pragma once

#include "IHeuristicCalculator.h" // For HeuristicResult
#include <vector>
#include <string>

/**
 * @class IProfileGenerator
 * @brief Strategy pattern interface for *formatting* the heuristic results
 * into a final 1D profile vector for export.
 */
class IProfileGenerator {
public:
    virtual ~IProfileGenerator() = default;

    /**
     * @brief Transforms the raw heuristic result into a final 1D profile.
     * @param result The raw output from the IHeuristicCalculator.
     * @return A 1D vector of scores (the final profile).
     */
    virtual std::vector<double> generateProfile(
        const HeuristicResult& result) const = 0;
};