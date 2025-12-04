#pragma once

#include "board.hpp"
#include "types.hpp"
#include <string>
#include <nlohmann/json.hpp>

namespace sudoku {

/**
 * JSON Handler for Sudoku boards
 *
 * Supports multiple intuitive JSON formats:
 *
 * Format 1: Simple 2D array
 * {
 *   "grid": [[5,3,0,...], [6,0,0,...], ...]
 * }
 *
 * Format 2: String rows (use '.' or '0' for empty)
 * {
 *   "grid": ["530070000", "600195000", ...]
 * }
 *
 * Format 3: Single string
 * {
 *   "puzzle": "530070000600195000..."
 * }
 *
 * Format 4: With metadata
 * {
 *   "size": 9,
 *   "box_rows": 3,
 *   "box_cols": 3,
 *   "grid": [...]
 * }
 */
class JSONHandler {
public:
    // Load from file
    static Board loadFromFile(const std::string& filepath);

    // Load from JSON string
    static Board loadFromString(const std::string& jsonStr);

    // Load from JSON object
    static Board loadFromJSON(const nlohmann::json& json);

    // Save to file
    static void saveToFile(const Board& board, const std::string& filepath, bool pretty = true);

    // Convert to JSON string
    static std::string toString(const Board& board, bool pretty = true);

    // Convert to JSON object
    static nlohmann::json toJSON(const Board& board);

    // Save solution result
    static void saveSolutionToFile(const Board& original, const SolveResult& result,
                                   const std::string& filepath, bool pretty = true);

    // Generate example JSON format help
    static std::string getFormatHelp();

private:
    // Parsing helpers
    static Grid parseGrid2D(const nlohmann::json& arr, int& detectedSize);
    static Grid parseGridStrings(const nlohmann::json& arr, int& detectedSize);
    static Grid parseSingleString(const std::string& str, int& detectedSize);

    static BoardDimension detectDimension(const nlohmann::json& json, int gridSize);
};

} // namespace sudoku
