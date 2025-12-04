#include "solver_dlx.hpp"
#include "solver_backtrack.hpp"
#include <algorithm>
#include <limits>
#include <tuple>

namespace sudoku {

DLXSolver::DLXSolver()
    : header_(nullptr), size_(0), box_rows_(0), box_cols_(0),
      iterations_(0), backtracks_(0) {}

DLXSolver::~DLXSolver() = default;

void DLXSolver::reset() {
    header_ = nullptr;
    nodes_.clear();
    column_headers_.clear();
    solution_rows_.clear();
    iterations_ = 0;
    backtracks_ = 0;
}

void DLXSolver::createColumnHeaders(int numConstraints) {
    // Create header node
    auto headerNode = std::make_unique<Node>();
    header_ = headerNode.get();
    nodes_.push_back(std::move(headerNode));

    // Create column headers
    column_headers_.resize(numConstraints);
    Node* prev = header_;

    for (int i = 0; i < numConstraints; ++i) {
        auto col = std::make_unique<Node>();
        col->col_id = i;
        col->column = col.get();

        // Link horizontally
        col->left = prev;
        col->right = header_;
        prev->right = col.get();
        header_->left = col.get();

        column_headers_[i] = col.get();
        prev = col.get();
        nodes_.push_back(std::move(col));
    }
}

void DLXSolver::addRow(int rowId, const std::vector<int>& columns) {
    if (columns.empty()) return;

    Node* first = nullptr;
    Node* prev = nullptr;

    for (int colIdx : columns) {
        auto node = std::make_unique<Node>();
        node->row_id = rowId;
        node->column = column_headers_[colIdx];
        node->col_id = colIdx;

        // Link vertically
        Node* colHeader = column_headers_[colIdx];
        node->up = colHeader->up;
        node->down = colHeader;
        colHeader->up->down = node.get();
        colHeader->up = node.get();
        colHeader->size++;

        // Link horizontally
        if (first == nullptr) {
            first = node.get();
            node->left = node.get();
            node->right = node.get();
        } else {
            node->left = prev;
            node->right = first;
            prev->right = node.get();
            first->left = node.get();
        }

        prev = node.get();
        nodes_.push_back(std::move(node));
    }
}

void DLXSolver::buildMatrix(const Board& board) {
    size_ = board.size();
    box_rows_ = board.boxRows();
    box_cols_ = board.boxCols();

    // Total constraints = 4 * size^2
    // Cell constraints: size * size (one per cell)
    // Row constraints: size * size (one per row-digit combination)
    // Column constraints: size * size (one per column-digit combination)
    // Box constraints: size * size (one per box-digit combination)
    int numConstraints = 4 * size_ * size_;

    createColumnHeaders(numConstraints);

    // For each cell
    for (int row = 0; row < size_; ++row) {
        for (int col = 0; col < size_; ++col) {
            Cell given = board.get(row, col);
            int box = (row / box_rows_) * (size_ / box_cols_) + (col / box_cols_);

            // For each possible value
            int startVal = (given != 0) ? given : 1;
            int endVal = (given != 0) ? given : size_;

            for (int val = startVal; val <= endVal; ++val) {
                // Check if valid placement
                if (given == 0 && !board.isValidPlacement(row, col, val)) {
                    continue;
                }

                int rowId = getRowId(row, col, val);

                std::vector<int> constraints = {
                    cellConstraint(row, col),
                    rowConstraint(row, val),
                    colConstraint(col, val),
                    boxConstraint(box, val)
                };

                addRow(rowId, constraints);
            }
        }
    }
}

int DLXSolver::getRowId(int row, int col, int val) const {
    return row * size_ * size_ + col * size_ + (val - 1);
}

std::tuple<int, int, int> DLXSolver::decodeRowId(int rowId) const {
    int val = (rowId % size_) + 1;
    rowId /= size_;
    int col = rowId % size_;
    int row = rowId / size_;
    return {row, col, val};
}

int DLXSolver::cellConstraint(int row, int col) const {
    return row * size_ + col;
}

int DLXSolver::rowConstraint(int row, int val) const {
    return size_ * size_ + row * size_ + (val - 1);
}

int DLXSolver::colConstraint(int col, int val) const {
    return 2 * size_ * size_ + col * size_ + (val - 1);
}

int DLXSolver::boxConstraint(int box, int val) const {
    return 3 * size_ * size_ + box * size_ + (val - 1);
}

void DLXSolver::cover(Node* col) {
    // Remove column from header list
    col->right->left = col->left;
    col->left->right = col->right;

    // Remove all rows in this column
    for (Node* row = col->down; row != col; row = row->down) {
        for (Node* node = row->right; node != row; node = node->right) {
            node->down->up = node->up;
            node->up->down = node->down;
            node->column->size--;
        }
    }
}

void DLXSolver::uncover(Node* col) {
    // Restore all rows in this column (in reverse order)
    for (Node* row = col->up; row != col; row = row->up) {
        for (Node* node = row->left; node != row; node = node->left) {
            node->column->size++;
            node->down->up = node;
            node->up->down = node;
        }
    }

    // Restore column to header list
    col->right->left = col;
    col->left->right = col;
}

DLXSolver::Node* DLXSolver::selectColumn() {
    // MRV heuristic: choose column with minimum size
    Node* best = nullptr;
    int minSize = std::numeric_limits<int>::max();

    for (Node* col = header_->right; col != header_; col = col->right) {
        if (col->size < minSize) {
            minSize = col->size;
            best = col;

            // Can't do better than 0 or 1
            if (minSize <= 1) break;
        }
    }

    return best;
}

bool DLXSolver::search(int depth) {
    ++iterations_;

    // All columns covered - solution found
    if (header_->right == header_) {
        return true;
    }

    // Choose column with minimum remaining values (MRV)
    Node* col = selectColumn();
    if (col == nullptr || col->size == 0) {
        return false;
    }

    // Cover this column
    cover(col);

    // Try each row in this column
    for (Node* row = col->down; row != col; row = row->down) {
        solution_rows_.push_back(row->row_id);

        // Cover all other columns in this row
        for (Node* node = row->right; node != row; node = node->right) {
            cover(node->column);
        }

        // Recurse
        if (search(depth + 1)) {
            return true;
        }

        // Backtrack
        ++backtracks_;
        solution_rows_.pop_back();

        // Uncover all other columns in this row (reverse order)
        for (Node* node = row->left; node != row; node = node->left) {
            uncover(node->column);
        }
    }

    // Uncover this column
    uncover(col);

    return false;
}

bool DLXSolver::searchAll(int depth, std::vector<std::vector<int>>& solutions, int maxSolutions) {
    ++iterations_;

    if (header_->right == header_) {
        solutions.push_back(solution_rows_);
        return solutions.size() >= static_cast<size_t>(maxSolutions);
    }

    Node* col = selectColumn();
    if (col == nullptr || col->size == 0) {
        return false;
    }

    cover(col);

    for (Node* row = col->down; row != col; row = row->down) {
        solution_rows_.push_back(row->row_id);

        for (Node* node = row->right; node != row; node = node->right) {
            cover(node->column);
        }

        if (searchAll(depth + 1, solutions, maxSolutions)) {
            // Don't return early - continue searching after uncover
            // Actually, if we found enough solutions, we should stop
            for (Node* node = row->left; node != row; node = node->left) {
                uncover(node->column);
            }
            solution_rows_.pop_back();
            uncover(col);
            return true;
        }

        ++backtracks_;
        solution_rows_.pop_back();

        for (Node* node = row->left; node != row; node = node->left) {
            uncover(node->column);
        }
    }

    uncover(col);

    return false;
}

Board DLXSolver::solutionToBoard(const std::vector<int>& rowIds, const Board& original) const {
    Board result = original;

    for (int rowId : rowIds) {
        auto [row, col, val] = decodeRowId(rowId);
        result.set(row, col, val);
    }

    return result;
}

SolveResult DLXSolver::solve(const Board& board) {
    SolveResult result;
    result.algorithm = name();

    Timer timer;
    timer.start();

    reset();
    buildMatrix(board);

    bool solved = search(0);

    timer.stop();

    result.solved = solved;
    result.iterations = iterations_;
    result.backtracks = backtracks_;
    result.time_ms = timer.elapsed_ms();

    if (solved) {
        result.solution = solutionToBoard(solution_rows_, board).grid();
    } else {
        result.error_message = "No solution found";
    }

    return result;
}

std::vector<Board> DLXSolver::findAllSolutions(const Board& board, int maxSolutions) {
    std::vector<Board> results;
    std::vector<std::vector<int>> solutionSets;

    reset();
    buildMatrix(board);

    searchAll(0, solutionSets, maxSolutions);

    for (const auto& rowIds : solutionSets) {
        results.push_back(solutionToBoard(rowIds, board));
    }

    return results;
}

bool DLXSolver::hasUniqueSolution(const Board& board) {
    auto solutions = findAllSolutions(board, 2);
    return solutions.size() == 1;
}

// Solver Factory Implementation
std::unique_ptr<ISolver> SolverFactory::create(SolverAlgorithm algorithm) {
    switch (algorithm) {
        case SolverAlgorithm::Backtracking:
            return createBacktracking();
        case SolverAlgorithm::DancingLinks:
        case SolverAlgorithm::Auto:
        case SolverAlgorithm::Hybrid:
        default:
            return createDLX();
    }
}

std::unique_ptr<ISolver> SolverFactory::createBacktracking() {
    return std::make_unique<BacktrackingSolver>();
}

std::unique_ptr<ISolver> SolverFactory::createDLX() {
    return std::make_unique<DLXSolver>();
}

} // namespace sudoku
