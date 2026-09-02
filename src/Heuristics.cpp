#include "../include/Heuristics.h"
#include "../include/Pixel.h" 
#include <numeric>   
#include <algorithm>
#include <iostream>
#include <cmath> 
#include <limits>

PixelSet get_intersection(const PixelSet& setA, const PixelSet& setB) {
    PixelSet intersection;
    const PixelSet& smaller = (setA.size() < setB.size()) ? setA : setB;
    const PixelSet& larger = (setA.size() < setB.size()) ? setB : setA;
    intersection.reserve(smaller.size()); 
    for (const auto& pixel : smaller) {
        if (larger.count(pixel)) { intersection.insert(pixel); }
    }
    return intersection;
}

void inplace_subtract(PixelSet& setA, const PixelSet& setB) {
    for (const auto& pixel : setB) { setA.erase(pixel); }
}

std::vector<double> precalculate_tie_breakers(const std::vector<PixelSet>& subsets, const std::vector<double>& cam_centers_x, const std::vector<double>& cam_centers_y) {
    std::vector<double> tie_breaker_scores(subsets.size(), 0.0);
    for (size_t i = 0; i < subsets.size(); ++i) {
        if (subsets[i].empty()) continue;
        double cx = cam_centers_x[i];
        double cy = cam_centers_y[i];
        double sum_dist_sq = 0.0;
        for (const auto& p : subsets[i]) {
            double dx = p.x - cx;
            double dy = p.y - cy;
            sum_dist_sq += (dx * dx + dy * dy);
        }
        double avg_dist_sq = sum_dist_sq / subsets[i].size();
        tie_breaker_scores[i] = -avg_dist_sq;
    }
    return tie_breaker_scores;
}

// =========================================================================
// CONTINUOUS UPSAMPLING (Apenas Gaussiana * Ratio Bruto)
// =========================================================================
HeuristicResult ContinuousUpsampling::calculateScores( 
    const PixelSet& universe, const std::vector<PixelSet>& subsets, 
    const std::vector<double>& cam_centers_x, const std::vector<double>& cam_centers_y,
    const std::vector<size_t>& frame_areas,
    const std::unordered_map<Pixel, int, PixelHash>& pixel_to_comp_id) const 
{
    HeuristicResult result;
    int num_frames = subsets.size();
    
    // Extrai os Scores Brutos (Gaussiana * Ratio) diretamente para o resultado
    std::vector<double> raw_scores(num_frames, 0.0);
    double spatial_sigma_sq = 2.0 * (300.0 * 300.0);

    for (int f = 0; f < num_frames; ++f) {
        if (subsets[f].empty() || frame_areas[f] == 0) continue;

        std::unordered_map<int, size_t> frag_area;
        std::unordered_map<int, double> frag_sum_x, frag_sum_y;
        
        for (const auto& p : subsets[f]) {
            int cid = pixel_to_comp_id.at(p);
            frag_area[cid]++;
            frag_sum_x[cid] += p.x;
            frag_sum_y[cid] += p.y;
        }

        double cx = cam_centers_x[f];
        double cy = cam_centers_y[f];
        double total_frame_score = 0.0;

        for (const auto& [cid, obj_area] : frag_area) {
            double frag_cx = frag_sum_x[cid] / obj_area;
            double frag_cy = frag_sum_y[cid] / obj_area;
            double dx = cx - frag_cx;
            double dy = cy - frag_cy;
            
            double gaussian = std::exp(-((dx * dx) + (dy * dy)) / spatial_sigma_sq);
            double ratio = (double)obj_area / (double)frame_areas[f];
            
            total_frame_score += (gaussian * ratio);
        }
        raw_scores[f] = total_frame_score;
    }

    result.scores = raw_scores;
    for (int f = 0; f < num_frames; ++f) {
        if (raw_scores[f] > 0.0) {
            result.selection_order.push_back(f);
            result.contribution_masks.push_back(subsets[f]);
        }
    }

    return result;
}

// =========================================================================
// DISCRETE UPSAMPLING (Binário) - COM THRESHOLD de 2%
// =========================================================================
HeuristicResult DiscreteUpsampling::calculateScores( 
    const PixelSet& universe, const std::vector<PixelSet>& subsets, 
    const std::vector<double>& cam_centers_x, const std::vector<double>& cam_centers_y,
    const std::vector<size_t>& frame_areas,
    const std::unordered_map<Pixel, int, PixelHash>& pixel_to_comp_id) const 
{
    HeuristicResult result;
    int num_frames = subsets.size();
    result.scores.resize(num_frames, 0.0);

    std::unordered_map<int, size_t> absolute_comp_area;
    size_t max_comp_area = 0;
    for (const auto& pair : pixel_to_comp_id) {
        int cid = pair.second;
        absolute_comp_area[cid]++;
        if (absolute_comp_area[cid] > max_comp_area) max_comp_area = absolute_comp_area[cid];
    }
    double relative_area_threshold = max_comp_area * 0.02;

    for (int f = 0; f < num_frames; ++f) {
        bool has_valid_object = false;
        
        std::unordered_map<int, size_t> frag_area;
        for (const auto& p : subsets[f]) {
            int cid = pixel_to_comp_id.at(p);
            frag_area[cid]++;
        }

        for (const auto& [cid, obj_area] : frag_area) {
            if (absolute_comp_area[cid] >= relative_area_threshold) {
                has_valid_object = true;
                break;
            }
        }

        if (has_valid_object) {
            result.scores[f] = 1.0;
            result.selection_order.push_back(f);
            result.contribution_masks.push_back(subsets[f]);
        }
    }
    return result;
}

HeuristicResult ClassicGreedyHeuristic::calculateScores( 
    const PixelSet& universe, const std::vector<PixelSet>& subsets, 
    const std::vector<double>& cam_centers_x, const std::vector<double>& cam_centers_y,
    const std::vector<size_t>& frame_areas, 
    const std::unordered_map<Pixel, int, PixelHash>& pixel_to_comp_id) const 
{
    HeuristicResult result;
    result.scores.resize(subsets.size(), 0.0);
    PixelSet uncovered_pixels = universe; 
    std::vector<size_t> available_frames(subsets.size());
    std::iota(available_frames.begin(), available_frames.end(), 0);
    std::vector<double> tie_breaker_scores = precalculate_tie_breakers(subsets, cam_centers_x, cam_centers_y);

    while (!uncovered_pixels.empty() && !available_frames.empty()) {
        size_t best_idx = -1, best_internal = -1; 
        size_t max_new = 0;
        double best_tie = -std::numeric_limits<double>::max();        
        PixelSet best_coverage; 

        for (size_t i = 0; i < available_frames.size(); ++i) {
            size_t f_idx = available_frames[i];
            PixelSet new_cov = get_intersection(subsets[f_idx], uncovered_pixels);
            size_t count = new_cov.size();
            if (count == 0) continue;

            double tie_score = tie_breaker_scores[f_idx];

            if (count > max_new || (count == max_new && tie_score > best_tie)) {
                max_new = count; best_tie = tie_score;
                best_idx = f_idx; best_internal = i; best_coverage = std::move(new_cov);
            }
        }

        if (best_idx == (size_t)-1) break;
        result.scores[best_idx] = static_cast<double>(max_new);
        result.selection_order.push_back(best_idx);
        result.contribution_masks.push_back(std::move(best_coverage)); 
        inplace_subtract(uncovered_pixels, subsets[best_idx]);
        std::swap(available_frames[best_internal], available_frames.back());
        available_frames.pop_back();
    }
    return result;
}

HeuristicResult SemanticAccelerationGT::calculateScores( 
    const PixelSet& universe, const std::vector<PixelSet>& subsets, 
    const std::vector<double>& cam_centers_x, const std::vector<double>& cam_centers_y,
    const std::vector<size_t>& frame_areas, 
    const std::unordered_map<Pixel, int, PixelHash>& pixel_to_comp_id) const 
{
    HeuristicResult result;
    int num_frames = subsets.size();
    result.scores.resize(num_frames, 0.0);

    std::unordered_map<int, size_t> absolute_comp_area;
    size_t max_comp_area = 0;
    int max_comp_id = -1;

    for (const auto& pair : pixel_to_comp_id) {
        int cid = pair.second;
        absolute_comp_area[cid]++;
        if (absolute_comp_area[cid] > max_comp_area) {
            max_comp_area = absolute_comp_area[cid];
        }
        if (cid > max_comp_id) max_comp_id = cid;
    }
    
    int comp_id_counter = max_comp_id + 1;
    double relative_area_threshold = max_comp_area * 0.02; 

    std::vector<std::vector<double>> comp_scores(comp_id_counter, std::vector<double>(num_frames, 0.0));
    double sigma = 300.0; 
    double sigma_sq = 2.0 * sigma * sigma;
    
    for (int f = 0; f < num_frames; ++f) {
        std::unordered_map<int, size_t> frag_area;
        std::unordered_map<int, double> frag_sum_x, frag_sum_y;

        for (const auto& p : subsets[f]) { 
            int cid = pixel_to_comp_id.at(p);
            frag_area[cid]++;
            frag_sum_x[cid] += p.x;
            frag_sum_y[cid] += p.y;
        }

        double cx = cam_centers_x[f];
        double cy = cam_centers_y[f];

        for (const auto& [cid, area] : frag_area) {
            if (absolute_comp_area[cid] < relative_area_threshold) continue; 

            double frag_cx = frag_sum_x[cid] / area;
            double frag_cy = frag_sum_y[cid] / area;
            double dx = cx - frag_cx;
            double dy = cy - frag_cy;
            double dist_sq = (dx * dx) + (dy * dy);

            comp_scores[cid][f] = std::exp(-dist_sq / sigma_sq); 
        }
    }

    std::vector<double> final_profile(num_frames, 0.0);

    for (int c = 0; c < comp_id_counter; ++c) {
        const auto& scores = comp_scores[c];
        int best_seq_start = -1, best_seq_end = -1;
        double max_seq_peak = -1.0; 
        int current_seq_start = -1;
        double current_seq_peak = -1.0;

        for (int f = 0; f <= num_frames; ++f) {
            bool isActive = (f < num_frames && scores[f] > 0.0);

            if (isActive) {
                if (current_seq_start == -1) current_seq_start = f;
                current_seq_peak = std::max(current_seq_peak, scores[f]);
            } else {
                if (current_seq_start != -1) {
                    if (current_seq_peak > max_seq_peak) {
                        max_seq_peak = current_seq_peak;
                        best_seq_start = current_seq_start;
                        best_seq_end = f - 1;
                    }
                    current_seq_start = -1;
                    current_seq_peak = -1.0;
                }
            }
        }
       
        if (best_seq_start != -1) {
            for (int f = best_seq_start; f <= best_seq_end; ++f) {
                final_profile[f] = 1.0; 
            }
        }
    }

    result.scores = final_profile;
    for (int f = 0; f < num_frames; ++f) {
        if (final_profile[f] > 0.0) {
            result.selection_order.push_back(f);
            result.contribution_masks.push_back(subsets[f]);
        }
    }

    return result;
}