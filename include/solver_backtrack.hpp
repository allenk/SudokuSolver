#pragma once

#include "solver.hpp"
#include <bitset>

namespace sudoku {

/**
 * Backtracking Solver with Constraint Propagation
 *
 * Features:
 * - Constraint propagation (naked singles, hidden singles)
 * - MRV (Minimum Remaining Values) heuristic for cell selection
 * - Bitset representation for efficient candidate tracking
 * - Arc consistency maintenance
 */
class BacktrackingSolver : public ISolver {
public:
    BacktrackingSolver();
    ~BacktrackingSolver() override = default;

    SolveResult solve(const Board& board) override;
    std::vector<Board> findAllSolutions(const Board& board, int maxSolutions = 100) override;
    bool hasUniqueSolution(const Board& board) override;
    std::string name() const override { return "Backtracking with Constraint Propagation"; }
    void reset() override;

    // Configuration
    void setUseConstraintPropagation(bool use) { use_constraint_prop_ = use; }
    void setUseMRV(bool use) { use_mrv_ = use; }

private:
    // Internal state
    int size_;
    int box_rows_;
    int box_cols_;
    std::vector<std::vector<std::bitset<32>>> candidates_;
    std::vector<std::bitset<32>> row_used_;
    std::vector<std::bitset<32>> col_used_;
    std::vector<std::bitset<32>> box_used_;

    // Statistics
    int iterations_;
    int backtracks_;

    // Configuration
    bool use_constraint_prop_ = true;
    bool use_mrv_ = true;

    // Initialize from board
    void initialize(const Board& board);

    // Core solving
    bool solveRecursive(Board& board);
    bool solveAll(Board& board, std::vector<Board>& solutions, int maxSolutions);

    // Constraint propagation
    bool propagate(Board& board);
    bool propagateNakedSingles(Board& board);
    bool propagateHiddenSingles(Board& board);

    // Candidate management
    void updateCandidates(int row, int col, Cell value);
    void restoreCandidates(int row, int col, Cell value);
    int candidateCount(int row, int col) const;

    // MRV heuristic
    std::pair<int, int> selectBestCell(const Board& board) const;

    // Utility
    int getBoxIndex(int row, int col) const;
};

} // namespace sudoku
