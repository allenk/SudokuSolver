#include "json_handler.hpp"
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>

namespace sudoku {

Board JSONHandler::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }

    nlohmann::json json;
    try {
        file >> json;
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("JSON parse error: " + std::string(e.what()));
    }

    return loadFromJSON(json);
}

Board JSONHandler::loadFromString(const std::string& jsonStr) {
    nlohmann::json json;
    try {
        json = nlohmann::json::parse(jsonStr);
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("JSON parse error: " + std::string(e.what()));
    }

    return loadFromJSON(json);
}

Board JSONHandler::loadFromJSON(const nlohmann::json& json) {
    Grid grid;
    int detectedSize = 0;

    // Try different formats
    if (json.contains("grid")) {
        const auto& gridData = json["grid"];

        if (gridData.is_array() && !gridData.empty()) {
            if (gridData[0].is_array()) {
                // Format 1: 2D array of numbers
                grid = parseGrid2D(gridData, detectedSize);
            } else if (gridData[0].is_string()) {
                // Format 2: Array of strings
                grid = parseGridStrings(gridData, detectedSize);
            } else if (gridData[0].is_number()) {
                // Single row? Or flat array
                // Check if it could be a flat array
                int totalSize = gridData.size();
                int possibleSize = static_cast<int>(std::sqrt(totalSize));
                if (possibleSize * possibleSize == totalSize) {
                    // Flat array
                    std::string flatStr;
                    for (const auto& val : gridData) {
                        flatStr += std::to_string(val.get<int>());
                    }
                    grid = parseSingleString(flatStr, detectedSize);
                }
            }
        }
    } else if (json.contains("puzzle")) {
        // Format 3: Single string
        std::string puzzle = json["puzzle"].get<std::string>();
        grid = parseSingleString(puzzle, detectedSize);
    } else if (json.contains("board")) {
        // Alternative naming
        const auto& boardData = json["board"];
        if (boardData.is_string()) {
            grid = parseSingleString(boardData.get<std::string>(), detectedSize);
        } else if (boardData.is_array()) {
            if (boardData[0].is_array()) {
                grid = parseGrid2D(boardData, detectedSize);
            } else {
                grid = parseGridStrings(boardData, detectedSize);
            }
        }
    } else if (json.is_array()) {
        // Root is the grid itself
        if (json[0].is_array()) {
            grid = parseGrid2D(json, detectedSize);
        } else if (json[0].is_string()) {
            grid = parseGridStrings(json, detectedSize);
        }
    } else if (json.is_string()) {
        // Root is the puzzle string
        grid = parseSingleString(json.get<std::string>(), detectedSize);
    }

    if (grid.empty()) {
        throw std::runtime_error("Could not parse grid from JSON");
    }

    // Detect or read dimensions
    BoardDimension dim = detectDimension(json, detectedSize);

    return Board(grid, dim);
}

Grid JSONHandler::parseGrid2D(const nlohmann::json& arr, int& detectedSize) {
    Grid grid;
    detectedSize = arr.size();

    for (const auto& row : arr) {
        std::vector<Cell> gridRow;
        for (const auto& cell : row) {
            int val = 0;
            if (cell.is_number()) {
                val = cell.get<int>();
            } else if (cell.is_string()) {
                std::string s = cell.get<std::string>();
                if (!s.empty() && s[0] >= '1' && s[0] <= '9') {
                    val = std::stoi(s);
                }
            }
            gridRow.push_back(val);
        }
        grid.push_back(gridRow);
    }

    return grid;
}

Grid JSONHandler::parseGridStrings(const nlohmann::json& arr, int& detectedSize) {
    Grid grid;
    detectedSize = arr.size();

    for (const auto& row : arr) {
        std::string rowStr = row.get<std::string>();
        std::vector<Cell> gridRow;

        for (char c : rowStr) {
            if (c >= '1' && c <= '9') {
                gridRow.push_back(c - '0');
            } else if (c == '.' || c == '0' || c == ' ' || c == '_') {
                gridRow.push_back(0);
            }
            // For larger boards, handle hex or multi-char
            // A=10, B=11, etc.
            else if (c >= 'A' && c <= 'Z') {
                gridRow.push_back(10 + (c - 'A'));
            } else if (c >= 'a' && c <= 'z') {
                gridRow.push_back(10 + (c - 'a'));
            }
        }

        grid.push_back(gridRow);
    }

    return grid;
}

Grid JSONHandler::parseSingleString(const std::string& str, int& detectedSize) {
    // Remove whitespace
    std::string cleaned;
    for (char c : str) {
        if (!std::isspace(c)) {
            cleaned += c;
        }
    }

    // Detect size from length
    int len = cleaned.length();
    detectedSize = static_cast<int>(std::sqrt(len));

    if (detectedSize * detectedSize != len) {
        throw std::runtime_error("Invalid puzzle string length: " + std::to_string(len));
    }

    Grid grid(detectedSize, std::vector<Cell>(detectedSize, 0));

    for (int i = 0; i < detectedSize; ++i) {
        for (int j = 0; j < detectedSize; ++j) {
            char c = cleaned[i * detectedSize + j];
            if (c >= '1' && c <= '9') {
                grid[i][j] = c - '0';
            } else if (c >= 'A' && c <= 'Z') {
                grid[i][j] = 10 + (c - 'A');
            } else if (c >= 'a' && c <= 'z') {
                grid[i][j] = 10 + (c - 'a');
            } else {
                grid[i][j] = 0;
            }
        }
    }

    return grid;
}

BoardDimension JSONHandler::detectDimension(const nlohmann::json& json, int gridSize) {
    // Check for explicit dimensions
    if (json.contains("size") && json.contains("box_rows") && json.contains("box_cols")) {
        return BoardDimension(
            json["size"].get<int>(),
            json["box_rows"].get<int>(),
            json["box_cols"].get<int>()
        );
    }

    if (json.contains("box_size")) {
        int boxSize = json["box_size"].get<int>();
        return BoardDimension(gridSize, boxSize, boxSize);
    }

    // Auto-detect from grid size
    return BoardDimension::FromSize(gridSize);
}

void JSONHandler::saveToFile(const Board& board, const std::string& filepath, bool pretty) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to create file: " + filepath);
    }

    nlohmann::json json = toJSON(board);

    if (pretty) {
        file << json.dump(2);
    } else {
        file << json.dump();
    }
}

std::string JSONHandler::toString(const Board& board, bool pretty) {
    nlohmann::json json = toJSON(board);
    return pretty ? json.dump(2) : json.dump();
}

nlohmann::json JSONHandler::toJSON(const Board& board) {
    nlohmann::json json;

    json["size"] = board.size();
    json["box_rows"] = board.boxRows();
    json["box_cols"] = board.boxCols();

    // Save as 2D array (most intuitive)
    nlohmann::json gridArr = nlohmann::json::array();
    for (int i = 0; i < board.size(); ++i) {
        nlohmann::json row = nlohmann::json::array();
        for (int j = 0; j < board.size(); ++j) {
            row.push_back(board.get(i, j));
        }
        gridArr.push_back(row);
    }
    json["grid"] = gridArr;

    // Also include string representation for easy viewing
    std::vector<std::string> rows;
    for (int i = 0; i < board.size(); ++i) {
        std::string row;
        for (int j = 0; j < board.size(); ++j) {
            Cell val = board.get(i, j);
            if (val == 0) {
                row += '.';
            } else if (val < 10) {
                row += ('0' + val);
            } else {
                row += ('A' + val - 10);
            }
        }
        rows.push_back(row);
    }
    json["grid_string"] = rows;

    return json;
}

void JSONHandler::saveSolutionToFile(const Board& original, const SolveResult& result,
                                     const std::string& filepath, bool pretty) {
    nlohmann::json json;

    // Original puzzle
    json["original"] = toJSON(original);

    // Solution info
    json["solved"] = result.solved;
    json["algorithm"] = result.algorithm;
    json["time_ms"] = result.time_ms;
    json["iterations"] = result.iterations;
    json["backtracks"] = result.backtracks;

    if (result.solved) {
        Board solutionBoard(result.solution, original.dimension());
        json["solution"] = toJSON(solutionBoard);
    }

    if (!result.error_message.empty()) {
        json["error"] = result.error_message;
    }

    std::ofstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to create file: " + filepath);
    }

    file << (pretty ? json.dump(2) : json.dump());
}

std::string JSONHandler::getFormatHelp() {
    return R"(
Supported JSON Input Formats
============================

Format 1: 2D Array (recommended)
{
  "grid": [
    [5, 3, 0, 0, 7, 0, 0, 0, 0],
    [6, 0, 0, 1, 9, 5, 0, 0, 0],
    ...
  ]
}

Format 2: String Rows (use '.' or '0' for empty cells)
{
  "grid": [
    "530070000",
    "600195000",
    ...
  ]
}

Format 3: Single String
{
  "puzzle": "530070000600195000098000060800060003400803001700020006060000280000419005000080079"
}

Format 4: With Explicit Dimensions (for non-standard sizes)
{
  "size": 16,
  "box_rows": 4,
  "box_cols": 4,
  "grid": [...]
}

Notes:
- Empty cells can be represented as 0, '.', '_', or ' '
- For boards larger than 9x9, use hex (A-Z) for values 10-35
- The grid can also be the root JSON element (without wrapper object)
)";
}

} // namespace sudoku
