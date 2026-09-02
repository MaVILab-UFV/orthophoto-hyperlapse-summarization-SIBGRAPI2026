#pragma once

#include <vector>
#include <string>
#include <stdexcept>

/**
 * @class RasterMask
 * @brief A 2D bitmap class (1 bit per pixel).
 * Used as an efficient intermediate format for loading data
 * from GDAL before converting it to a sparse PixelSet.
 */
class RasterMask {
public:
    int width, height;
    std::vector<bool> data;

    RasterMask(int w = 0, int h = 0, bool initialValue = false)
        : width(w), height(h), data((size_t)w * h, initialValue) {}

    bool get(int x, int y) const {
        if (x < 0 || x >= width || y < 0 || y >= height) return false;
        return data[(size_t)y * width + x];
    }

    void set(int x, int y, bool value) {
        if (x < 0 || x >= width || y < 0 || y >= height) return;
        data[(size_t)y * width + x] = value;
    }

    size_t count() const {
        size_t c = 0;
        for (bool val : data) {
            if (val) c++;
        }
        return c;
    }

    RasterMask op_and(const RasterMask& other) const {
        if (width != other.width || height != other.height) {
            throw std::runtime_error("Mask dimensions do not match for AND.");
        }
        RasterMask result(width, height);
        for (size_t i = 0; i < data.size(); ++i) {
            result.data[i] = data[i] && other.data[i];
        }
        return result;
    }

    void op_inplace_subtract(const RasterMask& other) {
        if (width != other.width || height != other.height) {
            throw std::runtime_error("Mask dimensions do not match for SUBTRACT.");
        }
        for (size_t i = 0; i < data.size(); ++i) {
            if (other.data[i]) {
                data[i] = false;
            }
        }
    }

    void op_inplace_or(const RasterMask& other) {
        if (width != other.width || height != other.height) {
            throw std::runtime_error("Mask dimensions do not match for OR.");
        }
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = data[i] || other.data[i];
        }
    }
};