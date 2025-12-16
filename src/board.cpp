#include "board.hpp"
#include <algorithm>
#include <numeric>

namespace sudoku {

// Constructors
Board::Board() : Board(9) {}

Board::Board(int size) : Board(BoardDimension::FromSize(size)) {}

Board::Board(const BoardDimension& dim) : dim_(dim) {
    if (!dim_.isValid()) {
        throw std::invalid_argument("Invalid board dimension");
    }
    initGrid(dim_.size);
}

Board::Board(const Grid& grid) {
    if (grid.empty() || grid[0].empty()) {
        throw std::invalid_argument("Empty grid");
    }
    auto size = grid.size();
    if (grid[0].size() != size) {
        throw std::invalid_argument("Grid must be square");
    }
    dim_ = BoardDimension::FromSize(static_cast<int>(size));
    grid_ = grid;
}

Board::Board(const Grid& grid, const BoardDimension& dim) : dim_(dim) {
    if (!dim_.isValid()) {
        throw std::invalid_argument("Invalid board dimension");
    }
    if (grid.size() != static_cast<std::size_t>(dim_.size) ||
        grid[0].size() != static_cast<std::size_t>(dim_.size)) {
        throw std::invalid_argument("Grid size doesn't match dimension");
    }
    grid_ = grid;
}

void Board::initGrid(int size) {
    grid_.assign(size, std::vector<Cell>(size, 0));
}

// Accessors
Cell Board::get(int row, int col) const {
    if (!isInRange(row, col)) {
        throw std::out_of_range("Cell position out of range");
    }
    return grid_[row][col];
}

void Board::set(int row, int col, Cell value) {
    if (!isInRange(row, col)) {
        throw std::out_of_range("Cell position out of range");
    }
    if (value != 0 && !isValidValue(value)) {
        throw std::invalid_argument("Invalid cell value");
    }
    grid_[row][col] = value;
}

bool Board::isEmpty(int row, int col) const {
    return get(row, col) == 0;
}

// Range checking
bool Board::isInRange(int row, int col) const {
    return row >= 0 && row < dim_.size && col >= 0 && col < dim_.size;
}

bool Board::isValidValue(Cell value) const {
    return value >= 1 && value <= dim_.size;
}

// Validation
bool Board::isValid() const {
    // Check all rows
    for (int i = 0; i < dim_.size; ++i) {
        if (!isRowValid(i)) return false;
    }

    // Check all columns
    for (int j = 0; j < dim_.size; ++j) {
        if (!isColValid(j)) return false;
    }

    // Check all boxes
    // Number of boxes in each direction
    int numBoxesVertical = dim_.size / dim_.box_rows;
    int numBoxesHorizontal = dim_.size / dim_.box_cols;

    for (int bi = 0; bi < numBoxesVertical; ++bi) {
        for (int bj = 0; bj < numBoxesHorizontal; ++bj) {
            int startRow = bi * dim_.box_rows;
            int startCol = bj * dim_.box_cols;
            if (!isBoxValid(startRow, startCol)) return false;
        }
    }

    return true;
}

bool Board::isRowValid(int row) const {
    std::vector<bool> seen(dim_.size + 1, false);
    for (int col = 0; col < dim_.size; ++col) {
        Cell val = grid_[row][col];
        if (val != 0) {
            if (seen[val]) return false;
            seen[val] = true;
        }
    }
    return true;
}

bool Board::isColValid(int col) const {
    std::vector<bool> seen(dim_.size + 1, false);
    for (int row = 0; row < dim_.size; ++row) {
        Cell val = grid_[row][col];
        if (val != 0) {
            if (seen[val]) return false;
            seen[val] = true;
        }
    }
    return true;
}

bool Board::isBoxValid(int boxRow, int boxCol) const {
    auto [startRow, startCol] = getBoxStart(boxRow, boxCol);
    std::vector<bool> seen(dim_.size + 1, false);

    for (int i = 0; i < dim_.box_rows; ++i) {
        for (int j = 0; j < dim_.box_cols; ++j) {
            Cell val = grid_[startRow + i][startCol + j];
            if (val != 0) {
                if (seen[val]) return false;
                seen[val] = true;
            }
        }
    }
    return true;
}

bool Board::isValidPlacement(int row, int col, Cell value) const {
    if (value == 0) return true;
    if (!isValidValue(value)) return false;

    // Check row
    for (int c = 0; c < dim_.size; ++c) {
        if (c != col && grid_[row][c] == value) return false;
    }

    // Check column
    for (int r = 0; r < dim_.size; ++r) {
        if (r != row && grid_[r][col] == value) return false;
    }

    // Check box
    auto [startRow, startCol] = getBoxStart(row, col);
    for (int i = 0; i < dim_.box_rows; ++i) {
        for (int j = 0; j < dim_.box_cols; ++j) {
            int r = startRow + i;
            int c = startCol + j;
            if ((r != row || c != col) && grid_[r][c] == value) return false;
        }
    }

    return true;
}

bool Board::isSolved() const {
    return !hasEmptyCell() && isValid();
}

bool Board::hasEmptyCell() const {
    for (int i = 0; i < dim_.size; ++i) {
        for (int j = 0; j < dim_.size; ++j) {
            if (grid_[i][j] == 0) return true;
        }
    }
    return false;
}

// Find empty cells
std::pair<int, int> Board::findFirstEmpty() const {
    for (int i = 0; i < dim_.size; ++i) {
        for (int j = 0; j < dim_.size; ++j) {
            if (grid_[i][j] == 0) return {i, j};
        }
    }
    return {-1, -1};
}

std::vector<std::pair<int, int>> Board::findAllEmpty() const {
    std::vector<std::pair<int, int>> empty;
    for (int i = 0; i < dim_.size; ++i) {
        for (int j = 0; j < dim_.size; ++j) {
            if (grid_[i][j] == 0) {
                empty.emplace_back(i, j);
            }
        }
    }
    return empty;
}

int Board::countEmpty() const {
    int count = 0;
    for (int i = 0; i < dim_.size; ++i) {
        for (int j = 0; j < dim_.size; ++j) {
            if (grid_[i][j] == 0) ++count;
        }
    }
    return count;
}

// Candidates
std::vector<Cell> Board::getCandidates(int row, int col) const {
    std::vector<Cell> candidates;
    if (grid_[row][col] != 0) return candidates;

    for (Cell v = 1; v <= dim_.size; ++v) {
        if (isValidPlacement(row, col, v)) {
            candidates.push_back(v);
        }
    }
    return candidates;
}

std::bitset<32> Board::getCandidateBits(int row, int col) const {
    std::bitset<32> bits;
    if (grid_[row][col] != 0) return bits;

    for (Cell v = 1; v <= dim_.size; ++v) {
        if (isValidPlacement(row, col, v)) {
            bits.set(v - 1);
        }
    }
    return bits;
}

// Box utilities
int Board::getBoxIndex(int row, int col) const {
    return (row / dim_.box_rows) * (dim_.size / dim_.box_cols) + (col / dim_.box_cols);
}

std::pair<int, int> Board::getBoxStart(int row, int col) const {
    return {
        (row / dim_.box_rows) * dim_.box_rows,
        (col / dim_.box_cols) * dim_.box_cols
    };
}

// Display
std::string Board::toString() const {
    return toStringWithHighlight(-1, -1);
}

std::string Board::toStringWithHighlight(int highlightRow, int highlightCol) const {
    std::ostringstream oss;
    int cellWidth = (dim_.size >= 10) ? 3 : 2;

    // Calculate separator widths
    int boxWidth = dim_.box_cols * cellWidth + 1;
    int totalWidth = (dim_.size / dim_.box_cols) * (boxWidth + 1) + 1;
    std::string hLine(totalWidth, '-');

    for (int i = 0; i < dim_.size; ++i) {
        // Horizontal separator before each box row
        if (i % dim_.box_rows == 0) {
            oss << hLine << "\n";
        }

        for (int j = 0; j < dim_.size; ++j) {
            // Vertical separator before each box column
            if (j == 0) {
                oss << "|";
            } else if (j % dim_.box_cols == 0) {
                oss << " |";
            }

            Cell val = grid_[i][j];
            bool highlight = (i == highlightRow && j == highlightCol);

            if (highlight) oss << "[";
            else oss << " ";

            if (val == 0) {
                oss << std::setw(cellWidth - 1) << ".";
            } else {
                if (dim_.size > 9 && val < 10) {
                    oss << std::setw(cellWidth - 1) << val;
                } else {
                    oss << std::setw(cellWidth - 1) << val;
                }
            }

            if (highlight) oss << "]";
        }
        oss << " |\n";
    }
    oss << hLine << "\n";

    return oss.str();
}

void Board::print(std::ostream& os) const {
    os << toString();
}

void Board::printCompact(std::ostream& os) const {
    for (int i = 0; i < dim_.size; ++i) {
        for (int j = 0; j < dim_.size; ++j) {
            if (dim_.size > 9) {
                os << std::setw(3) << (grid_[i][j] == 0 ? 0 : grid_[i][j]);
            } else {
                os << grid_[i][j];
            }
        }
        os << "\n";
    }
}

// Comparison
bool Board::operator==(const Board& other) const {
    return dim_.size == other.dim_.size && grid_ == other.grid_;
}

// Statistics
int Board::filledCount() const {
    int count = 0;
    for (int i = 0; i < dim_.size; ++i) {
        for (int j = 0; j < dim_.size; ++j) {
            if (grid_[i][j] != 0) ++count;
        }
    }
    return count;
}

double Board::fillRatio() const {
    int total = dim_.size * dim_.size;
    return total > 0 ? static_cast<double>(filledCount()) / total : 0.0;
}

int Board::difficulty() const {
    // Heuristic: count empty cells and cells with few candidates
    int empty = countEmpty();
    int difficultCells = 0;

    for (int i = 0; i < dim_.size; ++i) {
        for (int j = 0; j < dim_.size; ++j) {
            if (grid_[i][j] == 0) {
                auto candidates = getCandidates(i, j);
                if (candidates.size() <= 2) {
                    difficultCells += 3 - static_cast<int>(candidates.size());
                }
            }
        }
    }

    return empty * 10 + difficultCells * 5;
}

// Stream operator
std::ostream& operator<<(std::ostream& os, const Board& board) {
    board.print(os);
    return os;
}

} // namespace sudoku
