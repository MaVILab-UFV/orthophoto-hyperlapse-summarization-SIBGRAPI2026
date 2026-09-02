#include "../include/ProfileGenerators.h"
#include <cmath>
#include <algorithm>
#include <numeric> // For std::iota

// --- StepProfileGenerator Implementation ---
std::vector<double> StepProfileGenerator::generateProfile(
    const HeuristicResult& result) const 
{
    std::vector<double> final_profile;
    final_profile.reserve(result.scores.size());
    for (const double& score : result.scores) {
        final_profile.push_back((score > 0.0) ? 1.0 : 0.0);
    }
    return final_profile;
}

// --- NormalizedProfileGenerator Implementation ---
std::vector<double> NormalizedProfileGenerator::generateProfile(
    const HeuristicResult& result) const 
{
    if (result.scores.empty()) {
        return {};
    }
    
    std::vector<double> final_profile = result.scores;
    double max_score = *std::max_element(final_profile.begin(), final_profile.end());
    
    // Normaliza os valores para o intervalo [0.0, 1.0] dividindo pelo pico máximo
    if (max_score > 0.0) {
        for (double& score : final_profile) {
            score /= max_score;
        }
    }
    
    return final_profile;
}

// --- SmoothProfileGenerator Implementation ---
std::vector<double> SmoothProfileGenerator::generateProfile(
    const HeuristicResult& result) const
{
    if (result.scores.empty()) {
        return {};
    }
    std::vector<double> normalized_scores = result.scores;
    double max_score = *std::max_element(normalized_scores.begin(), normalized_scores.end());
    if (max_score > 0.0) {
        for (double& score : normalized_scores) {
            score /= max_score;
        }
    }
    
    // Apply a simple moving average (window size 3)
    std::vector<double> final_profile = normalized_scores;
    int n = normalized_scores.size();
    int kernel_radius = 1; // (Window size = 1 + kernel_radius*2)

    for (int i = 0; i < n; ++i) {
        double sum = 0.0;
        int count = 0;
        for (int j = -kernel_radius; j <= kernel_radius; ++j) {
            int idx = i + j;
            if (idx >= 0 && idx < n) {
                sum += normalized_scores[idx];
                count++;
            }
        }
        final_profile[i] = sum / count;
    }
    return final_profile;
}