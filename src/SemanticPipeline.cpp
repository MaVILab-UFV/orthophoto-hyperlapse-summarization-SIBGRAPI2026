#include "../include/SemanticPipeline.h"
#include "../include/GdalUtils.h" 
#include <gdal_priv.h> 
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <cmath>
#include <fstream>
#include <regex>

namespace fs = std::filesystem;

ImageRGB::ImageRGB(int w, int h)
    : width(w), height(h), r_band((size_t)w * h, 0), g_band((size_t)w * h, 0), b_band((size_t)w * h, 0) {}

ColorRGB ImageRGB::get(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return {0, 0, 0};
    size_t idx = (size_t)y * width + x;
    return {r_band[idx], g_band[idx], b_band[idx]};
}

void ImageRGB::set(int x, int y, ColorRGB color) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    size_t idx = (size_t)y * width + x;
    r_band[idx] = color.r; g_band[idx] = color.g; b_band[idx] = color.b;
}

bool ColorRGB::operator==(const ColorRGB& other) const {
    return r == other.r && g == other.g && b == other.b;
}

SemanticPipeline::SemanticPipeline(const std::string& ortho_path, const std::string& frame_dir, const std::string& debug_dir)
    : m_semantic_ortho_path(ortho_path), m_frame_directory_path(frame_dir), m_debug_mode(false) {
    GDALAllRegister();
    if (!debug_dir.empty()) {
        m_debug_output_directory = debug_dir;
        m_debug_mode = true;
        try {
            fs::create_directories(debug_dir);
        } catch (const fs::filesystem_error& e) {
            throw std::runtime_error("Failed to create debug directory.");
        }
    }
    discoverFrameMasks(".tif"); 
}

const std::vector<std::string>& SemanticPipeline::getDiscoveredFramePaths() const {
    return m_discovered_frame_paths;
}

void SemanticPipeline::discoverFrameMasks(const std::string& extension) {
    m_discovered_frame_paths.clear();
    for (const auto& entry : fs::directory_iterator(m_frame_directory_path)) {
        if (entry.is_regular_file() && entry.path().extension() == extension) {
            m_discovered_frame_paths.push_back(entry.path().string());
        }
    }
    std::sort(m_discovered_frame_paths.begin(), m_discovered_frame_paths.end(), 
        [](const std::string& a, const std::string& b) {
            std::regex re("(\\d+)");
            std::smatch matchA, matchB;
            int numA = 0, numB = 0;
            
            std::string nameA = fs::path(a).filename().string();
            std::string nameB = fs::path(b).filename().string();
            
            if (std::regex_search(nameA, matchA, re)) numA = std::stoi(matchA.str());
            if (std::regex_search(nameB, matchB, re)) numB = std::stoi(matchB.str());
            
            return numA < numB; 
        });
        
    std::cout << "  -> Found " << m_discovered_frame_paths.size() << " frames in directory." << std::endl;
}

ImageRGB SemanticPipeline::loadSemanticOrtho(const std::string& path) {
    GDALDataset *poDataset = (GDALDataset*) GDALOpen(path.c_str(), GA_ReadOnly);
    if (poDataset == nullptr) throw std::runtime_error("Failed to open GDAL Dataset.");
    int w = poDataset->GetRasterXSize(); int h = poDataset->GetRasterYSize();
    ImageRGB ortho(w, h);
    GDALRasterBand *poBandR = poDataset->GetRasterBand(1);
    GDALRasterBand *poBandG = poDataset->GetRasterBand(2);
    GDALRasterBand *poBandB = poDataset->GetRasterBand(3);
    std::vector<unsigned char> rowR(w), rowG(w), rowB(w);
    for (int y = 0; y < h; ++y) {
        poBandR->RasterIO(GF_Read, 0, y, w, 1, rowR.data(), w, 1, GDT_Byte, 0, 0);
        poBandG->RasterIO(GF_Read, 0, y, w, 1, rowG.data(), w, 1, GDT_Byte, 0, 0);
        poBandB->RasterIO(GF_Read, 0, y, w, 1, rowB.data(), w, 1, GDT_Byte, 0, 0);
        for (int x = 0; x < w; ++x) ortho.set(x, y, {rowR[x], rowG[x], rowB[x]});
    }
    GDALClose(poDataset);
    std::cout << "  -> Orthophoto loaded (" << w << "x" << h << " pixels)." << std::endl;
    return ortho;
}

RasterMask SemanticPipeline::convertPixelSetToRaster(const PixelSet& set, int w, int h) const {
    RasterMask mask(w, h, false);
    for (const auto& pixel : set) mask.set(pixel.x, pixel.y, true);
    return mask;
}

std::vector<HeuristicResult> SemanticPipeline::run( ColorRangeRGB target_range, const std::vector<IHeuristicCalculator*>& heuristics,
    const std::vector<std::string>& heuristic_names) {
    
    auto printBox = [](const std::string& msg) {
        std::string border(msg.length() + 2, '-');
        std::cout << "\n+" << border << "+\n| " << msg << " |\n+" << border << "+\n";
    };

    auto printHeader = [](const std::string& msg) {
        std::string border(msg.length() + 2, '=');
        std::cout << "\n+" << border << "+\n| " << msg << " |\n+" << border << "+\n";
    };

    std::cout << "\nStarting optimized direct-to-PixelSet pipeline..." << std::endl;
    
    printBox("[STEP 1] Loading Ortho & Extracting Target Universe");
    ImageRGB semantic_ortho_rgb = loadSemanticOrtho(m_semantic_ortho_path);
    PixelSet universe_set;
    
    for (int y = 0; y < semantic_ortho_rgb.height; ++y) {
        for (int x = 0; x < semantic_ortho_rgb.width; ++x) {
            ColorRGB pc = semantic_ortho_rgb.get(x, y);
            if (pc.r >= target_range.r_min && pc.r <= target_range.r_max &&
                pc.g >= target_range.g_min && pc.g <= target_range.g_max &&
                pc.b >= target_range.b_min && pc.b <= target_range.b_max) 
            {
                universe_set.insert({x, y});
            }
        }
    }
    std::cout << "  -> Universe created. Total size: " << universe_set.size() << " valid pixels." << std::endl;

    printBox("[STEP 2] Mapping Connected Components (Objects)");
    std::unordered_map<Pixel, int, PixelHash> pixel_to_comp_id;
    PixelSet unvisited = universe_set;
    int comp_id_counter = 0;
    int dx[] = {-1,1,0,0,-1,-1,1,1};
    int dy[] = {0,0,-1,1,-1,1,-1,1};

    while(!unvisited.empty()) {
        Pixel start = *unvisited.begin();
        std::queue<Pixel> q;
        q.push(start);
        unvisited.erase(start);

        while(!q.empty()) {
            Pixel current = q.front(); q.pop();
            pixel_to_comp_id[current] = comp_id_counter;

            for(int k = 0; k < 8; k++) {
                Pixel neighbor{current.x + dx[k], current.y + dy[k]};
                if (unvisited.count(neighbor)) {
                    q.push(neighbor);
                    unvisited.erase(neighbor);
                }
            }
        }
        comp_id_counter++;
    }
    std::cout << "  -> Connected components mapped. Total objects: " << comp_id_counter << std::endl;

    int num_frames = m_discovered_frame_paths.size();
    
    printBox("[STEP 3] Loading " + std::to_string(num_frames) + " Frames Directly to Coordinate Sets");
    
    std::vector<double> cam_centers_x(num_frames, 0.0);
    std::vector<double> cam_centers_y(num_frames, 0.0);
    std::vector<size_t> frame_areas(num_frames, 0); // VETOR DE ÁREAS PARA O UPSAMPLING

    std::vector<PixelSet> subsets_set;
    subsets_set.reserve(num_frames);
    std::vector<unsigned char> rowBuffer; 

    for (int f = 0; f < num_frames; ++f) {
        GDALDataset *poDataset = (GDALDataset*) GDALOpen(m_discovered_frame_paths[f].c_str(), GA_ReadOnly);
        int w = poDataset->GetRasterXSize(); int h = poDataset->GetRasterYSize();
        GDALRasterBand *poBand = poDataset->GetRasterBand(1);
        
        PixelSet current_set;
        rowBuffer.resize(w);
        
        size_t area_cam = 0;
        int min_x = w, max_x = -1;
        int min_y = h, max_y = -1;
        
        for (int y = 0; y < h; ++y) {
            // Lemos o frame inteiro como solicitado (sem Janela de Universo)
            poBand->RasterIO(GF_Read, 0, y, w, 1, rowBuffer.data(), w, 1, GDT_Byte, 0, 0);
            for (int x = 0; x < w; ++x) {
                if (rowBuffer[x] > 0) {
                    area_cam++; // Incrementa a área total de projeção do frame
                    
                    // Atualiza os extremos para o cálculo do centro de visão por Bounding Box (mitiga efeito trapézio)
                    if (x < min_x) min_x = x;
                    if (x > max_x) max_x = x;
                    if (y < min_y) min_y = y;
                    if (y > max_y) max_y = y;
                    
                    Pixel p{x, y};
                    if (universe_set.count(p)) current_set.insert(p);
                }
            }
        }
        GDALClose(poDataset);

        frame_areas[f] = area_cam; // Passa a área para as heurísticas

        // Define o centro baseando-se na Bounding Box
        if (max_x >= min_x && max_y >= min_y) {
            cam_centers_x[f] = min_x + (max_x - min_x) / 2.0;
            cam_centers_y[f] = min_y + (max_y - min_y) / 2.0;
        } else {
            cam_centers_x[f] = 0.0;
            cam_centers_y[f] = 0.0;
        }
        
        subsets_set.push_back(std::move(current_set));
    }
    std::cout << "  -> All frames loaded successfully." << std::endl;

    printBox("[STEP 4] Saving Absolute Universe Mask & Running Heuristics");
    std::string base_out_dir = m_debug_output_directory.empty() ? "." : m_debug_output_directory;
    RasterMask absolute_universe = convertPixelSetToRaster(universe_set, semantic_ortho_rgb.width, semantic_ortho_rgb.height);
    saveRasterMaskAsTIF(absolute_universe, base_out_dir + "/universe.tif", m_semantic_ortho_path);
    std::cout << "  -> Saved: " << base_out_dir << "/universe.tif" << std::endl;

    std::vector<HeuristicResult> all_results;

    for (size_t h = 0; h < heuristics.size(); ++h) {
        printHeader("EXECUTING HEURISTIC: " + heuristic_names[h]);
        
        // Passando a nova variável frame_areas
        HeuristicResult result = heuristics[h]->calculateScores(universe_set, subsets_set, cam_centers_x, cam_centers_y, frame_areas, pixel_to_comp_id);
        
        std::cout << "  -> Calculation finished. Generating coverage mask..." << std::endl;
        RasterMask final_coverage(semantic_ortho_rgb.width, semantic_ortho_rgb.height, false);
        
        for (const auto& contribution_set : result.contribution_masks) {
            RasterMask contrib_raster = convertPixelSetToRaster(contribution_set, semantic_ortho_rgb.width, semantic_ortho_rgb.height);
            final_coverage.op_inplace_or(contrib_raster); 
        }
        
        std::string final_mask_path = base_out_dir + "/final_coverage_" + heuristic_names[h] + ".tif";
        saveRasterMaskAsTIF(final_coverage, final_mask_path, m_semantic_ortho_path);
        std::cout << "  -> Coverage mask saved: " << final_mask_path << std::endl;
        
        all_results.push_back(std::move(result));
    }

    std::cout << "\nPipeline execution completed successfully!\n" << std::endl;
    return all_results; 
}