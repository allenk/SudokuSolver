#pragma once

#include "board.hpp"
#include "types.hpp"
#include <memory>

namespace sudoku {

// Abstract solver interface
class ISolver {
public:
    virtual ~ISolver() = default;

    // Main solving method
    virtual SolveResult solve(const Board& board) = 0;

    // Find all solutions (up to maxSolutions)
    virtual std::vector<Board> findAllSolutions(const Board& board, int maxSolutions = 100) = 0;

    // Check if solution is unique
    virtual bool hasUniqueSolution(const Board& board) = 0;

    // Get algorithm name
    virtual std::string name() const = 0;

    // Reset internal state
    virtual void reset() = 0;
};

// Solver factory
class SolverFactory {
public:
    static std::unique_ptr<ISolver> create(SolverAlgorithm algorithm);
    static std::unique_ptr<ISolver> createBacktracking();
    static std::unique_ptr<ISolver> createDLX();
};

} // namespace sudoku
