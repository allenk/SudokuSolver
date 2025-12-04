#pragma once

#include "types.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>
#include <cmath>

namespace sudoku {

class Board {
public:
    // Constructors
    Board();
    Board(int size);
    Board(const BoardDimension& dim);
    Board(const Grid& grid);
    Board(const Grid& grid, const BoardDimension& dim);

    // Copy and move
    Board(const Board&) = default;
    Board(Board&&) = default;
    Board& operator=(const Board&) = default;
    Board& operator=(Board&&) = default;

    // Accessors
    int size() const { return dim_.size; }
    int boxRows() const { return dim_.box_rows; }
    int boxCols() const { return dim_.box_cols; }
    const BoardDimension& dimension() const { return dim_; }

    Cell get(int row, int col) const;
    void set(int row, int col, Cell value);
    bool isEmpty(int row, int col) const;

    const Grid& grid() const { return grid_; }
    Grid& grid() { return grid_; }

    // Validation
    bool isValid() const;
    bool isValidPlacement(int row, int col, Cell value) const;
    bool isSolved() const;
    bool hasEmptyCell() const;

    // Find empty cells
    std::pair<int, int> findFirstEmpty() const;
    std::vector<std::pair<int, int>> findAllEmpty() const;
    int countEmpty() const;

    // Candidates management
    std::vector<Cell> getCandidates(int row, int col) const;
    std::bitset<32> getCandidateBits(int row, int col) const;

    // Box utilities
    int getBoxIndex(int row, int col) const;
    std::pair<int, int> getBoxStart(int row, int col) const;

    // Display
    std::string toString() const;
    std::string toStringWithHighlight(int highlightRow = -1, int highlightCol = -1) const;
    void print(std::ostream& os = std::cout) const;
    void printCompact(std::ostream& os = std::cout) const;

    // Comparison
    bool operator==(const Board& other) const;
    bool operator!=(const Board& other) const { return !(*this == other); }

    // Statistics
    int filledCount() const;
    double fillRatio() const;
    int difficulty() const; // Heuristic difficulty score

private:
    Grid grid_;
    BoardDimension dim_;

    void initGrid(int size);
    bool isInRange(int row, int col) const;
    bool isValidValue(Cell value) const;

    // Validation helpers
    bool isRowValid(int row) const;
    bool isColValid(int col) const;
    bool isBoxValid(int boxRow, int boxCol) const;
};

// Stream operators
std::ostream& operator<<(std::ostream& os, const Board& board);

} // namespace sudoku
