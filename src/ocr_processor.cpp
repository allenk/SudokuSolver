#include "ocr_processor.hpp"
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <algorithm>
#include <cmath>
#include <filesystem>

namespace sudoku {

OCRProcessor::OCRProcessor() {
    // Try to find tessdata path
    std::vector<std::string> possiblePaths = {
        "tessdata",
        "./tessdata",
        "../tessdata",
        "C:/Program Files/Tesseract-OCR/tessdata",
        "C:/Program Files/Tesseract-OCR/share/tessdata",
        "/usr/share/tesseract-ocr/4.00/tessdata",
        "/usr/share/tessdata"
    };

    for (const auto& path : possiblePaths) {
        if (std::filesystem::exists(path)) {
            tessdata_path_ = path;
            break;
        }
    }
}

OCRProcessor::~OCRProcessor() = default;

OCRResult OCRProcessor::processImage(const std::string& imagePath) {
    OCRResult result;

    // Load image
    cv::Mat image = cv::imread(imagePath);
    if (image.empty()) {
        result.error_message = "Failed to load image: " + imagePath;
        return result;
    }

    return processImage(image);
}

OCRResult OCRProcessor::processImage(const cv::Mat& image) {
    OCRResult result;

    try {
        // Preprocess
        cv::Mat gray = preprocessImage(image);
        if (debug_mode_) saveDebugImage("01_preprocessed", gray);

        // Binarize for grid detection (NOT inverted - black lines on white)
        cv::Mat binaryForGrid;
        cv::adaptiveThreshold(gray, binaryForGrid, 255,
                              cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                              cv::THRESH_BINARY, 11, 2);

        // Invert for contour detection (white lines on black)
        cv::Mat binaryInv;
        cv::bitwise_not(binaryForGrid, binaryInv);
        if (debug_mode_) saveDebugImage("02_binary", binaryInv);

        // Detect grid using the inverted image
        GridInfo gridInfo = detectGrid(binaryInv, gray);
        if (!gridInfo.valid) {
            result.error_message = "Failed to detect Sudoku grid";
            return result;
        }

        if (debug_mode_) saveDebugImage("03_warped", gridInfo.warped);

        // Detect grid size
        int size = expected_size_ > 0 ? expected_size_ : detectGridSize(gridInfo.warped);
        if (size <= 0 || size > 25) {
            result.error_message = "Invalid grid size detected: " + std::to_string(size);
            return result;
        }

        result.dimension = BoardDimension::FromSize(size);

        // Extract cells from the warped grayscale image
        auto cells = extractCells(gridInfo.warped, size);

        // Initialize grid
        result.grid.assign(size, std::vector<Cell>(size, 0));
        result.confidences.assign(size, std::vector<float>(size, 0.0f));

        // Recognize each cell
        for (int i = 0; i < size; ++i) {
            for (int j = 0; j < size; ++j) {
                cv::Mat cellImg = cells[i][j];

                if (debug_mode_) {
                    saveDebugImage("cell_" + std::to_string(i) + "_" + std::to_string(j), cellImg);
                }

                if (isCellEmpty(cellImg)) {
                    result.grid[i][j] = 0;
                    result.confidences[i][j] = 1.0f;
                } else {
                    int digit = recognizeDigit(cellImg);
                    result.grid[i][j] = digit;
                    result.confidences[i][j] = (digit > 0) ? 0.9f : 0.5f;
                }
            }
        }

        // Validate result
        if (!validateBoard(result.grid, size)) {
            result.error_message = "Warning: Detected board may have recognition errors";
        }

        result.success = true;

    } catch (const std::exception& e) {
        result.error_message = std::string("OCR processing failed: ") + e.what();
    }

    return result;
}

cv::Mat OCRProcessor::preprocessImage(const cv::Mat& input) {
    cv::Mat result;

    // Convert to grayscale if needed
    if (input.channels() == 3) {
        cv::cvtColor(input, result, cv::COLOR_BGR2GRAY);
    } else {
        result = input.clone();
    }

    // Resize if too small or too large
    int maxDim = std::max(result.rows, result.cols);
    if (maxDim < 500) {
        double scale = 500.0 / maxDim;
        cv::resize(result, result, cv::Size(), scale, scale, cv::INTER_CUBIC);
    } else if (maxDim > 2000) {
        double scale = 2000.0 / maxDim;
        cv::resize(result, result, cv::Size(), scale, scale, cv::INTER_AREA);
    }

    // Denoise
    cv::GaussianBlur(result, result, cv::Size(5, 5), 0);

    return result;
}

cv::Mat OCRProcessor::binarizeImage(const cv::Mat& gray) {
    cv::Mat binary;

    // Adaptive threshold - do NOT invert here
    cv::adaptiveThreshold(gray, binary, 255,
                          cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                          cv::THRESH_BINARY, 11, 2);

    return binary;
}

OCRProcessor::GridInfo OCRProcessor::detectGrid(const cv::Mat& binary, const cv::Mat& gray) {
    GridInfo info;
    info.valid = false;

    // Find all contours
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(binary.clone(), contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    if (contours.empty()) {
        return info;
    }

    // Find the largest quadrilateral contour (the outer grid)
    double maxArea = 0;
    std::vector<cv::Point> bestContour;
    double imageArea = static_cast<double>(binary.rows * binary.cols);

    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);

        // Must be at least 20% of image area (the grid should be prominent)
        if (area < imageArea * 0.2) continue;

        // Approximate to polygon
        std::vector<cv::Point> approx;
        double epsilon = cv::arcLength(contour, true) * 0.02;
        cv::approxPolyDP(contour, approx, epsilon, true);

        // Check if it's a quadrilateral
        if (approx.size() == 4 && cv::isContourConvex(approx)) {
            if (area > maxArea) {
                maxArea = area;
                bestContour = approx;
            }
        }
    }

    // If no quadrilateral found, try to use image borders
    if (bestContour.empty()) {
        // Try with larger epsilon
        for (const auto& contour : contours) {
            double area = cv::contourArea(contour);
            if (area < imageArea * 0.2) continue;

            std::vector<cv::Point> approx;
            cv::approxPolyDP(contour, approx, cv::arcLength(contour, true) * 0.05, true);

            if (approx.size() == 4) {
                if (area > maxArea) {
                    maxArea = area;
                    bestContour = approx;
                }
            }
        }
    }

    if (bestContour.empty()) {
        // Fallback: use image borders with small margin
        int margin = 5;
        bestContour = {
            cv::Point(margin, margin),
            cv::Point(binary.cols - margin, margin),
            cv::Point(binary.cols - margin, binary.rows - margin),
            cv::Point(margin, binary.rows - margin)
        };
    }

    info.corners = bestContour;

    // Order corners: top-left, top-right, bottom-right, bottom-left
    info.corners = orderCorners(info.corners);

    // Perspective transform using grayscale image (not binary)
    info.warped = perspectiveTransform(gray, info.corners);
    info.valid = !info.warped.empty();

    return info;
}

std::vector<cv::Point> OCRProcessor::orderCorners(const std::vector<cv::Point>& corners) {
    if (corners.size() != 4) return corners;

    // Calculate center
    cv::Point2f center(0, 0);
    for (const auto& p : corners) {
        center.x += static_cast<float>(p.x);
        center.y += static_cast<float>(p.y);
    }
    center.x /= 4;
    center.y /= 4;

    // Classify corners by their position relative to center
    std::vector<cv::Point> ordered(4);
    for (const auto& p : corners) {
        int idx = 0;
        if (p.x > center.x) idx += 1;  // right side
        if (p.y > center.y) idx += 2;  // bottom side
        // idx: 0=TL, 1=TR, 2=BL, 3=BR
        // We want: 0=TL, 1=TR, 2=BR, 3=BL
        int mapping[] = {0, 1, 3, 2};
        ordered[mapping[idx]] = p;
    }

    return ordered;
}

std::vector<cv::Point> OCRProcessor::findGridCorners(const cv::Mat& binary) {
    // Detect lines using Hough transform
    std::vector<cv::Vec4i> lines;
    cv::HoughLinesP(binary, lines, 1, CV_PI / 180, 100, 100, 10);

    if (lines.empty()) {
        return {};
    }

    // Find outermost lines
    int minX = binary.cols, maxX = 0;
    int minY = binary.rows, maxY = 0;

    for (const auto& line : lines) {
        minX = std::min({minX, line[0], line[2]});
        maxX = std::max({maxX, line[0], line[2]});
        minY = std::min({minY, line[1], line[3]});
        maxY = std::max({maxY, line[1], line[3]});
    }

    return {
        cv::Point(minX, minY),
        cv::Point(maxX, minY),
        cv::Point(maxX, maxY),
        cv::Point(minX, maxY)
    };
}

cv::Mat OCRProcessor::perspectiveTransform(const cv::Mat& image, const std::vector<cv::Point>& corners) {
    // Target size (square) - larger for better OCR
    int size = 450;  // 50 pixels per cell for 9x9

    std::vector<cv::Point2f> src(4), dst(4);
    for (int i = 0; i < 4; ++i) {
        src[i] = cv::Point2f(static_cast<float>(corners[i].x),
                             static_cast<float>(corners[i].y));
    }

    dst[0] = cv::Point2f(0, 0);
    dst[1] = cv::Point2f(static_cast<float>(size - 1), 0);
    dst[2] = cv::Point2f(static_cast<float>(size - 1), static_cast<float>(size - 1));
    dst[3] = cv::Point2f(0, static_cast<float>(size - 1));

    cv::Mat transform = cv::getPerspectiveTransform(src, dst);
    cv::Mat result;
    cv::warpPerspective(image, result, transform, cv::Size(size, size));

    return result;
}

int OCRProcessor::detectGridSize(const cv::Mat& grid) {
    // Most sudoku puzzles are 9x9, use that as default
    // User can override with -s/--size parameter if needed

    // Try simple line detection to distinguish 9x9 from 16x16
    cv::Mat edges;
    cv::Canny(grid, edges, 30, 100);

    std::vector<cv::Vec4i> lines;
    cv::HoughLinesP(edges, lines, 1, CV_PI / 180, 30, grid.cols / 5, 10);

    // Count distinct horizontal and vertical line positions
    std::vector<int> hPos, vPos;
    for (const auto& line : lines) {
        int dx = std::abs(line[2] - line[0]);
        int dy = std::abs(line[3] - line[1]);

        if (dx > dy * 3) {
            hPos.push_back((line[1] + line[3]) / 2);
        } else if (dy > dx * 3) {
            vPos.push_back((line[0] + line[2]) / 2);
        }
    }

    // Remove duplicates
    auto countUnique = [](std::vector<int>& pos, int threshold) -> int {
        if (pos.empty()) return 0;
        std::sort(pos.begin(), pos.end());
        int count = 1;
        int last = pos[0];
        for (size_t i = 1; i < pos.size(); ++i) {
            if (pos[i] - last > threshold) {
                count++;
                last = pos[i];
            }
        }
        return count;
    };

    int threshold = grid.cols / 20;
    int hCount = countUnique(hPos, threshold);
    int vCount = countUnique(vPos, threshold);

    if (debug_mode_) {
        std::cout << "Line detection: ~" << hCount << " horizontal, ~" << vCount << " vertical" << std::endl;
    }

    // Heuristic: 16x16 should have ~17 lines each direction
    // 9x9 should have ~10 lines (or just 4 if only box lines visible)
    if (hCount >= 14 && vCount >= 14) {
        return 16;
    }

    // Default to 9x9 (most common)
    return 9;
}

std::vector<int> OCRProcessor::findGridLines(const cv::Mat& grid, bool horizontal) {
    // Detect edges - use lower thresholds to catch thin lines
    cv::Mat edges;
    cv::Canny(grid, edges, 30, 100);  // Lower thresholds for thin lines

    // Find lines - lower threshold and shorter minimum length
    std::vector<cv::Vec4i> lines;
    int minLineLength = std::max(20, (horizontal ? grid.cols : grid.rows) / 5);
    cv::HoughLinesP(edges, lines, 1, CV_PI / 180, 20, minLineLength, 5);

    std::vector<int> positions;
    int maxPos = horizontal ? grid.rows : grid.cols;

    for (const auto& line : lines) {
        int dx = std::abs(line[2] - line[0]);
        int dy = std::abs(line[3] - line[1]);

        if (horizontal && dx > dy * 3) {
            // Horizontal line - use y position
            positions.push_back((line[1] + line[3]) / 2);
        } else if (!horizontal && dy > dx * 3) {
            // Vertical line - use x position
            positions.push_back((line[0] + line[2]) / 2);
        }
    }

    if (positions.empty()) {
        return positions;
    }

    // Sort and remove duplicates
    std::sort(positions.begin(), positions.end());

    // Use smaller threshold to keep more lines (thin cell lines are close together)
    int threshold = maxPos / 25;  // Smaller threshold to keep thin lines separate
    std::vector<int> filtered;
    filtered.push_back(positions[0]);

    for (size_t i = 1; i < positions.size(); ++i) {
        if (positions[i] - filtered.back() > threshold) {
            filtered.push_back(positions[i]);
        } else {
            // Average nearby lines
            filtered.back() = (filtered.back() + positions[i]) / 2;
        }
    }

    // CRITICAL: Ensure first line is at edge (or add it)
    int edgeThreshold = maxPos / 20;
    if (filtered.front() > edgeThreshold) {
        filtered.insert(filtered.begin(), 0);
    } else {
        filtered.front() = 0;  // Snap to edge
    }

    // CRITICAL: Ensure last line is at edge (or add it)
    if (filtered.back() < maxPos - edgeThreshold) {
        filtered.push_back(maxPos - 1);
    } else {
        filtered.back() = maxPos - 1;  // Snap to edge
    }

    return filtered;
}

std::vector<std::vector<cv::Mat>> OCRProcessor::extractCells(const cv::Mat& grid, int size) {
    std::vector<std::vector<cv::Mat>> cells(size, std::vector<cv::Mat>(size));

    // Simple approach: the warped image is already cropped to grid boundary
    // Just divide uniformly with a small fixed margin to avoid line pixels

    float cellWidth = static_cast<float>(grid.cols) / size;
    float cellHeight = static_cast<float>(grid.rows) / size;

    // Fixed margin - enough to skip grid lines but not too much
    // Typical grid lines are 1-3 pixels, use 5-6 to be safe
    int margin = std::max(3, static_cast<int>(std::min(cellWidth, cellHeight) * 0.12f));

    if (debug_mode_) {
        std::cout << "Grid: " << grid.cols << "x" << grid.rows << std::endl;
        std::cout << "Cell size: " << cellWidth << "x" << cellHeight
                  << " margin=" << margin << std::endl;
    }

    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            // Cell boundaries
            int x1 = static_cast<int>(j * cellWidth) + margin;
            int y1 = static_cast<int>(i * cellHeight) + margin;
            int x2 = static_cast<int>((j + 1) * cellWidth) - margin;
            int y2 = static_cast<int>((i + 1) * cellHeight) - margin;

            // Ensure valid rectangle
            x1 = std::max(0, std::min(x1, grid.cols - 1));
            y1 = std::max(0, std::min(y1, grid.rows - 1));
            x2 = std::max(x1 + 1, std::min(x2, grid.cols));
            y2 = std::max(y1 + 1, std::min(y2, grid.rows));

            cells[i][j] = grid(cv::Rect(x1, y1, x2 - x1, y2 - y1)).clone();
        }
    }

    return cells;
}

cv::Mat OCRProcessor::cleanCell(const cv::Mat& cell) {
    cv::Mat cleaned;

    // Ensure grayscale
    if (cell.channels() > 1) {
        cv::cvtColor(cell, cleaned, cv::COLOR_BGR2GRAY);
    } else {
        cleaned = cell.clone();
    }

    // Binarize - invert so digit is white on black background
    cv::threshold(cleaned, cleaned, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    // Find contours to isolate digit
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(cleaned.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    if (!contours.empty()) {
        // Find largest contour (likely the digit)
        size_t maxIdx = 0;
        double maxArea = 0;
        for (size_t i = 0; i < contours.size(); ++i) {
            double area = cv::contourArea(contours[i]);
            if (area > maxArea) {
                maxArea = area;
                maxIdx = i;
            }
        }

        // Create mask with only the largest contour
        cv::Mat mask = cv::Mat::zeros(cleaned.size(), CV_8UC1);
        cv::drawContours(mask, contours, static_cast<int>(maxIdx), cv::Scalar(255), cv::FILLED);
        cv::bitwise_and(cleaned, mask, cleaned);
    }

    // Resize to standard size for OCR
    cv::resize(cleaned, cleaned, cv::Size(28, 28), 0, 0, cv::INTER_AREA);

    // Add padding
    cv::copyMakeBorder(cleaned, cleaned, 4, 4, 4, 4, cv::BORDER_CONSTANT, cv::Scalar(0));

    return cleaned;
}

bool OCRProcessor::isCellEmpty(const cv::Mat& cell) {
    cv::Mat gray;

    if (cell.channels() > 1) {
        cv::cvtColor(cell, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = cell.clone();
    }

    // Use Otsu threshold to binarize
    cv::Mat binary;
    cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    // Count dark pixels (potential digit pixels after inversion)
    int darkPixels = cv::countNonZero(binary);
    int totalPixels = binary.rows * binary.cols;
    double ratio = static_cast<double>(darkPixels) / totalPixels;

    // If less than 2% dark pixels, consider empty
    if (ratio < 0.02) {
        return true;
    }

    // Check standard deviation of grayscale - empty cells have low variance
    cv::Scalar mean, stddev;
    cv::meanStdDev(gray, mean, stddev);

    // Very low stddev means mostly uniform (empty cell)
    if (stddev[0] < 15) {
        return true;
    }

    return false;
}

int OCRProcessor::recognizeDigit(const cv::Mat& cell) {
    cv::Mat prepared;

    // Ensure grayscale
    if (cell.channels() > 1) {
        cv::cvtColor(cell, prepared, cv::COLOR_BGR2GRAY);
    } else {
        prepared = cell.clone();
    }

    // Resize to larger size for better OCR
    cv::resize(prepared, prepared, cv::Size(64, 64), 0, 0, cv::INTER_CUBIC);

    // Add white padding
    cv::copyMakeBorder(prepared, prepared, 10, 10, 10, 10, cv::BORDER_CONSTANT, cv::Scalar(255));

    // Binarize - black text on white background
    cv::threshold(prepared, prepared, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

    // Save debug image if enabled
    static int debugCounter = 0;
    if (debug_mode_) {
        saveDebugImage("ocr_input_" + std::to_string(debugCounter++), prepared);
    }

    // Initialize Tesseract
    tesseract::TessBaseAPI tess;

    const char* tessPath = tessdata_path_.empty() ? nullptr : tessdata_path_.c_str();

    // Try OEM_DEFAULT instead of OEM_LSTM_ONLY for better single char recognition
    if (tess.Init(tessPath, "eng", tesseract::OEM_DEFAULT) != 0) {
        if (debug_mode_) {
            std::cerr << "Failed to initialize Tesseract with OEM_DEFAULT" << std::endl;
        }
        last_error_ = "Failed to initialize Tesseract";
        return 0;
    }

    // Configure for single digit recognition
    tess.SetPageSegMode(tesseract::PSM_SINGLE_CHAR);
    tess.SetVariable("tessedit_char_whitelist", "123456789");
    tess.SetVariable("classify_bln_numeric_mode", "1");

    // Set image (grayscale, 1 byte per pixel)
    tess.SetImage(prepared.data, prepared.cols, prepared.rows, 1, prepared.cols);

    // Recognize
    char* text = tess.GetUTF8Text();
    int confidence = tess.MeanTextConf();

    if (debug_mode_) {
        std::string resultText = text ? text : "(null)";
        // Remove newlines for cleaner output
        resultText.erase(std::remove(resultText.begin(), resultText.end(), '\n'), resultText.end());
        resultText.erase(std::remove(resultText.begin(), resultText.end(), '\r'), resultText.end());
        std::cout << "OCR: '" << resultText << "' conf=" << confidence << std::endl;
    }

    int digit = 0;
    if (text) {
        for (int i = 0; text[i] != '\0'; ++i) {
            if (text[i] >= '1' && text[i] <= '9') {
                digit = text[i] - '0';
                break;
            }
        }
    }

    delete[] text;
    tess.End();

    return digit;
}

bool OCRProcessor::validateBoard(const Grid& grid, int size) {
    // Basic validation: check for invalid values
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            if (grid[i][j] < 0 || grid[i][j] > size) {
                return false;
            }
        }
    }

    // Check rows for duplicates
    for (int i = 0; i < size; ++i) {
        std::vector<bool> seen(size + 1, false);
        for (int j = 0; j < size; ++j) {
            int val = grid[i][j];
            if (val != 0) {
                if (seen[val]) return false;
                seen[val] = true;
            }
        }
    }

    // Check columns for duplicates
    for (int j = 0; j < size; ++j) {
        std::vector<bool> seen(size + 1, false);
        for (int i = 0; i < size; ++i) {
            int val = grid[i][j];
            if (val != 0) {
                if (seen[val]) return false;
                seen[val] = true;
            }
        }
    }

    return true;
}

void OCRProcessor::saveDebugImage(const std::string& name, const cv::Mat& image) {
    std::filesystem::create_directories("debug");
    cv::imwrite("debug/" + name + ".png", image);
}

} // namespace sudoku
