#pragma once

#include "solver.hpp"
#include <vector>
#include <memory>

namespace sudoku {

/**
 * Dancing Links (DLX) Solver
 *
 * Implementation of Donald Knuth's Algorithm X using Dancing Links
 * for solving Sudoku as an exact cover problem.
 *
 * This is typically the fastest algorithm for hard Sudoku puzzles.
 *
 * The exact cover matrix has:
 * - Rows: Each possible placement (cell, value) combination
 * - Columns: 4 types of constraints
 *   1. Cell constraints: Each cell has exactly one value
 *   2. Row constraints: Each row has each digit exactly once
 *   3. Column constraints: Each column has each digit exactly once
 *   4. Box constraints: Each box has each digit exactly once
 */
class DLXSolver : public ISolver {
public:
    DLXSolver();
    ~DLXSolver() override;

    SolveResult solve(const Board& board) override;
    std::vector<Board> findAllSolutions(const Board& board, int maxSolutions = 100) override;
    bool hasUniqueSolution(const Board& board) override;
    std::string name() const override { return "Dancing Links (Algorithm X)"; }
    void reset() override;

private:
    // DLX Node structure
    struct Node {
        Node* left;
        Node* right;
        Node* up;
        Node* down;
        Node* column;
        int row_id;     // Which row of the matrix
        int size;       // Only used for column headers
        int col_id;     // Column identifier

        Node() : left(this), right(this), up(this), down(this),
                 column(nullptr), row_id(-1), size(0), col_id(-1) {}
    };

    // State
    Node* header_;
    std::vector<std::unique_ptr<Node>> nodes_;
    std::vector<Node*> column_headers_;
    std::vector<int> solution_rows_;

    int size_;
    int box_rows_;
    int box_cols_;
    int iterations_;
    int backtracks_;

    // Build the exact cover matrix
    void buildMatrix(const Board& board);
    void createColumnHeaders(int numConstraints);
    void addRow(int rowId, const std::vector<int>& columns);

    // Algorithm X operations
    void cover(Node* col);
    void uncover(Node* col);
    Node* selectColumn();  // MRV heuristic

    // Solving
    bool search(int depth);
    bool searchAll(int depth, std::vector<std::vector<int>>& solutions, int maxSolutions);

    // Convert solution to board
    Board solutionToBoard(const std::vector<int>& rowIds, const Board& original) const;

    // Matrix position helpers
    int getRowId(int row, int col, int val) const;
    std::tuple<int, int, int> decodeRowId(int rowId) const;

    // Constraint column indices
    int cellConstraint(int row, int col) const;
    int rowConstraint(int row, int val) const;
    int colConstraint(int col, int val) const;
    int boxConstraint(int box, int val) const;
};

} // namespace sudoku
