#pragma once

#include "RasterMask.h"
#include <string>
#include <stdexcept>
#include <iostream>
#include "gdal_priv.h"

/**
 * @brief Saves a boolean RasterMask as a georeferenced TIF file.
 * @param mask The boolean RasterMask to save.
 * @param output_path The destination file path (e.g., "debug/universo.tif").
 * @param source_geo_tif Path to the *original* orthophoto, used to copy
 * georeferencing information (projection, transform) to the new TIF.
 */
inline void saveRasterMaskAsTIF(const RasterMask& mask, 
                                const std::string& output_path, 
                                const std::string& source_geo_tif) {

    GDALDataset *poSourceDataset = (GDALDataset*) GDALOpen(source_geo_tif.c_str(), GA_ReadOnly);
    if (poSourceDataset == nullptr) {
        throw std::runtime_error("Failed to open source TIF for metadata: " + source_geo_tif);
    }

    const char* pszProjection = poSourceDataset->GetProjectionRef();
    double adfGeoTransform[6];
    poSourceDataset->GetGeoTransform(adfGeoTransform);
    GDALClose(poSourceDataset); 

    GDALDriver *poDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
    if (poDriver == nullptr) {
        throw std::runtime_error("GTiff driver not found.");
    }

    GDALDataset *poNewDataset = poDriver->Create(
        output_path.c_str(), 
        mask.width, 
        mask.height, 
        1,           // 1 band
        GDT_Byte,    // 8-bit (0-255)
        nullptr      // Options
    );
    if (poNewDataset == nullptr) {
        throw std::runtime_error("Failed to create output file: " + output_path);
    }

    poNewDataset->SetProjection(pszProjection);
    poNewDataset->SetGeoTransform(adfGeoTransform);
    GDALRasterBand *poNewBand = poNewDataset->GetRasterBand(1);

    std::vector<unsigned char> rowBuffer(mask.width);
    for (int y = 0; y < mask.height; ++y) {
        for (int x = 0; x < mask.width; ++x) {
            rowBuffer[x] = mask.get(x, y) ? 255 : 0;
        }
        poNewBand->RasterIO(GF_Write, 0, y, mask.width, 1, 
                            rowBuffer.data(), mask.width, 1, GDT_Byte, 0, 0);
    }

    GDALClose(poNewDataset);
}