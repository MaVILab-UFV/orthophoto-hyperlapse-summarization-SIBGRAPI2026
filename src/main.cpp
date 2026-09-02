#include "../include/SemanticPipeline.h"
#include "../include/Heuristics.h"
#include "../include/ProfileGenerators.h"
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <memory>
#include <string>

namespace fs = std::filesystem;

struct Config {
    std::string ortho_path = "data/Orthophotomosaic.tif";
    std::string frame_dir = "data/OrthoByFrameMasks/";
    std::string output_dir = "data/";
    bool debug_enabled = false;
    ColorRangeRGB target_range;
};

std::string getArgValue(const std::string& arg) {
    size_t pos = arg.find('=');
    if (pos == std::string::npos) throw std::runtime_error("Invalid arg format.");
    return arg.substr(pos + 1);
}

Config parseArguments(int argc, char* argv[]) {
    Config config;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--ortho=", 0) == 0) { config.ortho_path = getArgValue(arg); }
        else if (arg.rfind("--frames_dir=", 0) == 0) { config.frame_dir = getArgValue(arg); }
        else if (arg.rfind("--out_dir=", 0) == 0) {
            config.output_dir = getArgValue(arg);
            if (!config.output_dir.empty() && config.output_dir.back() != '/') config.output_dir += '/';
        } 
        else if (arg == "--debug") { config.debug_enabled = true; }
        else if (arg.rfind("--r_min=", 0) == 0) { config.target_range.r_min = std::stoul(getArgValue(arg)); }
        else if (arg.rfind("--r_max=", 0) == 0) { config.target_range.r_max = std::stoul(getArgValue(arg)); }
        else if (arg.rfind("--g_min=", 0) == 0) { config.target_range.g_min = std::stoul(getArgValue(arg)); }
        else if (arg.rfind("--g_max=", 0) == 0) { config.target_range.g_max = std::stoul(getArgValue(arg)); }
        else if (arg.rfind("--b_min=", 0) == 0) { config.target_range.b_min = std::stoul(getArgValue(arg)); }
        else if (arg.rfind("--b_max=", 0) == 0) { config.target_range.b_max = std::stoul(getArgValue(arg)); }
    }
    return config;
}

void saveReportToFile(const std::string& filepath, const HeuristicResult& result, const std::vector<std::string>& frame_paths) {
    std::ofstream outfile(filepath);
    if (!outfile.is_open()) return;
    outfile << "--- Heuristic Analysis Report ---" << "\n";
    std::vector<std::string> used_frames, unused_frames;
    for (size_t i = 0; i < result.scores.size(); ++i) {
        std::string frame_name = fs::path(frame_paths[i]).filename().string();
        if (result.scores[i] > 0.0) {
            std::stringstream ss;
            ss << frame_name << " (Score: " << std::fixed << std::setprecision(1) << result.scores[i] << ")";
            used_frames.push_back(ss.str());
        } else { unused_frames.push_back(frame_name); }
    }
    outfile << "\nUSED Frames: " << used_frames.size() << "\n";
    for (const auto& s : used_frames) outfile << "  - " << s << "\n";
    outfile << "\nUNUSED Frames: " << unused_frames.size() << "\n";
    for (const auto& s : unused_frames) outfile << "  - " << s << "\n";
    outfile.close();
}

void saveProfileToFile(const std::vector<double>& profile, const std::string& output_path) {
    std::ofstream outfile(output_path);
    if (!outfile.is_open()) return;
    outfile << std::fixed << std::setprecision(8);
    for (const double& value : profile) outfile << value << "\n";
    outfile.close();
}

int main(int argc, char* argv[]) {
    try {
        Config config = parseArguments(argc, argv);
        fs::create_directories(config.output_dir); 
        std::string debug_dir = config.debug_enabled ? config.output_dir + "debug/" : config.output_dir;
        
        ClassicGreedyHeuristic classic_h;
        ContinuousUpsampling continuous_up; 
        DiscreteUpsampling discrete_up;     
        SemanticAccelerationGT s_hyperlapse;

        std::vector<IHeuristicCalculator*> heuristics = {&classic_h, &continuous_up, &discrete_up, &s_hyperlapse};    
       std::vector<std::string> h_names = {"classic", "continuous_up", "discrete_up", "s_hyperlapse"};
        
        auto smooth_generator = std::make_unique<SmoothProfileGenerator>();
        auto step_generator   = std::make_unique<StepProfileGenerator>();
        auto normalized_generator = std::make_unique<NormalizedProfileGenerator>();

        SemanticPipeline pipeline(config.ortho_path, config.frame_dir, debug_dir);
        
        std::vector<HeuristicResult> results = pipeline.run(config.target_range, heuristics, h_names);

        std::cout << "\n--- Generating Final Profiles ---" << std::endl;
        const auto& frame_paths = pipeline.getDiscoveredFramePaths();

        // 3. Salva os resultados independentes para cada heurística
        for (size_t i = 0; i < results.size(); ++i) {
            std::string prefix = h_names[i] + "_";
            
            std::vector<double> p_smooth = smooth_generator->generateProfile(results[i]);
            std::vector<double> p_step   = step_generator->generateProfile(results[i]);
            std::vector<double> p_norm   = normalized_generator->generateProfile(results[i]);

            saveProfileToFile(p_smooth, config.output_dir + prefix + "semantic_profile_smooth.txt");
            saveProfileToFile(p_step, config.output_dir + prefix + "semantic_profile_step.txt");
            saveProfileToFile(p_norm, config.output_dir + prefix + "semantic_profile_normalized.txt");
            saveReportToFile(config.output_dir + prefix + "semantic_report.txt", results[i], frame_paths);
        }
        std::cout << "All profiles saved successfully." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Fatal pipeline error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}