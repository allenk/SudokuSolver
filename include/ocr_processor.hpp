#pragma once

#include "board.hpp"
#include "types.hpp"
#include <string>
#include <opencv2/opencv.hpp>

namespace sudoku {

/**
 * OCR Processor for Sudoku Images
 *
 * Uses OpenCV for image preprocessing and Tesseract for digit recognition.
 *
 * Supports:
 * - Standard 9x9 Sudoku images
 * - Arbitrary NxN grids (auto-detection)
 * - Various image qualities and formats
 */
class OCRProcessor {
public:
    OCRProcessor();
    ~OCRProcessor();

    // Main processing
    OCRResult processImage(const std::string& imagePath);
    OCRResult processImage(const cv::Mat& image);

    // Configuration
    void setDebugMode(bool debug) { debug_mode_ = debug; }
    void setTessdataPath(const std::string& path) { tessdata_path_ = path; }
    void setExpectedSize(int size) { expected_size_ = size; }

    // Get detailed info
    std::string getLastError() const { return last_error_; }

private:
    bool debug_mode_ = false;
    std::string tessdata_path_;
    int expected_size_ = 0;  // 0 = auto-detect
    std::string last_error_;

    // Image preprocessing
    cv::Mat preprocessImage(const cv::Mat& input);
    cv::Mat binarizeImage(const cv::Mat& gray);

    // Grid detection
    struct GridInfo {
        std::vector<cv::Point> corners;
        cv::Mat warped;
        int detected_size;
        bool valid;
    };

    GridInfo detectGrid(const cv::Mat& binary, const cv::Mat& gray);
    std::vector<cv::Point> findGridCorners(const cv::Mat& binary);
    std::vector<cv::Point> orderCorners(const std::vector<cv::Point>& corners);
    cv::Mat perspectiveTransform(const cv::Mat& image, const std::vector<cv::Point>& corners);

    // Cell extraction
    std::vector<std::vector<cv::Mat>> extractCells(const cv::Mat& grid, int size);
    std::vector<int> findGridLines(const cv::Mat& grid, bool horizontal);
    cv::Mat cleanCell(const cv::Mat& cell);

    // Digit recognition
    int recognizeDigit(const cv::Mat& cell);
    bool isCellEmpty(const cv::Mat& cell);

    // Grid size detection
    int detectGridSize(const cv::Mat& grid);

    // Validation
    bool validateBoard(const Grid& grid, int size);

    // Utility
    void saveDebugImage(const std::string& name, const cv::Mat& image);
};

} // namespace sudoku
