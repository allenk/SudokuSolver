#include "solver_backtrack.hpp"
#include <algorithm>
#include <limits>

#ifdef USE_OPENMP
#include <omp.h>
#endif

namespace sudoku {

	BacktrackingSolver::BacktrackingSolver()
		: size_(0), box_rows_(0), box_cols_(0), iterations_(0), backtracks_(0) {
	}

	void BacktrackingSolver::reset() {
		iterations_ = 0;
		backtracks_ = 0;
		candidates_.clear();
		row_used_.clear();
		col_used_.clear();
		box_used_.clear();
	}

	void BacktrackingSolver::initialize(const Board& board) {
		size_ = board.size();
		box_rows_ = board.boxRows();
		box_cols_ = board.boxCols();

		// Initialize candidate sets
		candidates_.assign(size_, std::vector<std::bitset<32>>(size_));
		row_used_.assign(size_, std::bitset<32>());
		col_used_.assign(size_, std::bitset<32>());
		int numBoxes = (size_ / box_rows_) * (size_ / box_cols_);
		box_used_.assign(numBoxes, std::bitset<32>());

		// Mark used values
		for (int i = 0; i < size_; ++i) {
			for (int j = 0; j < size_; ++j) {
				Cell val = board.get(i, j);
				if (val != 0) {
					row_used_[i].set(val - 1);
					col_used_[j].set(val - 1);
					box_used_[getBoxIndex(i, j)].set(val - 1);
				}
			}
		}

		// Compute initial candidates
		for (int i = 0; i < size_; ++i) {
			for (int j = 0; j < size_; ++j) {
				if (board.get(i, j) == 0) {
					std::bitset<32> used = row_used_[i] | col_used_[j] | box_used_[getBoxIndex(i, j)];
					for (int v = 0; v < size_; ++v) {
						if (!used.test(v)) {
							candidates_[i][j].set(v);
						}
					}
				}
			}
		}
	}

	int BacktrackingSolver::getBoxIndex(int row, int col) const {
		return (row / box_rows_) * (size_ / box_cols_) + (col / box_cols_);
	}

	SolveResult BacktrackingSolver::solve(const Board& board) {
		SolveResult result;
		result.algorithm = name();

		Timer timer;
		timer.start();

		reset();
		initialize(board);

		Board workBoard = board;

		// Initial constraint propagation
		if (use_constraint_prop_) {
			if (!propagate(workBoard)) {
				timer.stop();
				result.time_ms = timer.elapsed_ms();
				result.error_message = "Puzzle is unsolvable (constraint propagation failed)";
				return result;
			}
		}

		// Solve
		bool solved = solveRecursive(workBoard);

		timer.stop();

		result.solved = solved;
		result.solution = workBoard.grid();
		result.iterations = iterations_;
		result.backtracks = backtracks_;
		result.time_ms = timer.elapsed_ms();

		if (!solved) {
			result.error_message = "No solution found";
		}

		return result;
	}

	bool BacktrackingSolver::solveRecursive(Board& board) {
		++iterations_;

		// Apply constraint propagation
		if (use_constraint_prop_ && !propagate(board)) {
			return false;
		}

		// Find best cell to fill (MRV heuristic)
		auto [row, col] = selectBestCell(board);

		// No empty cells - solved!
		if (row == -1) {
			return board.isValid();
		}

		// Check for impossible state
		if (candidates_[row][col].none()) {
			return false;
		}

		// Try each candidate
		auto& cellCandidates = candidates_[row][col];

		for (int v = 0; v < size_; ++v) {
			if (!cellCandidates.test(v)) continue;

			Cell value = v + 1;

			// Save FULL state for backtracking (board + candidates)
			Grid savedBoard = board.grid();
			auto savedCandidates = candidates_;
			auto savedRowUsed = row_used_;
			auto savedColUsed = col_used_;
			auto savedBoxUsed = box_used_;

			// Make move
			board.set(row, col, value);
			updateCandidates(row, col, value);

			// Recurse
			if (solveRecursive(board)) {
				return true;
			}

			// Backtrack - restore FULL state
			++backtracks_;
			board.grid() = savedBoard;
			candidates_ = savedCandidates;
			row_used_ = savedRowUsed;
			col_used_ = savedColUsed;
			box_used_ = savedBoxUsed;
		}

		return false;
	}

	std::vector<Board> BacktrackingSolver::findAllSolutions(const Board& board, int maxSolutions) {
		std::vector<Board> solutions;

		reset();
		initialize(board);

		Board workBoard = board;

		if (use_constraint_prop_) {
			if (!propagate(workBoard)) {
				return solutions;
			}
		}

		solveAll(workBoard, solutions, maxSolutions);

		return solutions;
	}

	bool BacktrackingSolver::solveAll(Board& board, std::vector<Board>& solutions, int maxSolutions) {
		++iterations_;

		if (use_constraint_prop_ && !propagate(board)) {
			return false;
		}

		auto [row, col] = selectBestCell(board);

		if (row == -1) {
			if (board.isValid()) {
				solutions.push_back(board);
				return solutions.size() >= static_cast<size_t>(maxSolutions);
			}
			return false;
		}

		if (candidates_[row][col].none()) {
			return false;
		}

		auto& cellCandidates = candidates_[row][col];

		for (int v = 0; v < size_; ++v) {
			if (!cellCandidates.test(v)) continue;

			Cell value = v + 1;

			// Save FULL state for backtracking
			Grid savedBoard = board.grid();
			auto savedCandidates = candidates_;
			auto savedRowUsed = row_used_;
			auto savedColUsed = col_used_;
			auto savedBoxUsed = box_used_;

			board.set(row, col, value);
			updateCandidates(row, col, value);

			if (solveAll(board, solutions, maxSolutions)) {
				return true;  // Found enough solutions
			}

			// Backtrack - restore FULL state
			++backtracks_;
			board.grid() = savedBoard;
			candidates_ = savedCandidates;
			row_used_ = savedRowUsed;
			col_used_ = savedColUsed;
			box_used_ = savedBoxUsed;
		}

		return false;
	}

	bool BacktrackingSolver::hasUniqueSolution(const Board& board) {
		auto solutions = findAllSolutions(board, 2);
		return solutions.size() == 1;
	}

	bool BacktrackingSolver::propagate(Board& board) {
		bool changed = true;
		while (changed) {
			changed = false;

			// Naked singles
			if (propagateNakedSingles(board)) {
				changed = true;
			}

			// Hidden singles
			if (propagateHiddenSingles(board)) {
				changed = true;
			}

			// Check for contradictions
			for (int i = 0; i < size_; ++i) {
				for (int j = 0; j < size_; ++j) {
					if (board.get(i, j) == 0 && candidates_[i][j].none()) {
						return false;  // No valid candidates - contradiction
					}
				}
			}
		}
		return true;
	}

	bool BacktrackingSolver::propagateNakedSingles(Board& board) {
		bool changed = false;

		for (int i = 0; i < size_; ++i) {
			for (int j = 0; j < size_; ++j) {
				if (board.get(i, j) == 0 && candidates_[i][j].count() == 1) {
					// Find the single candidate
					for (int v = 0; v < size_; ++v) {
						if (candidates_[i][j].test(v)) {
							Cell value = v + 1;
							board.set(i, j, value);
							updateCandidates(i, j, value);
							changed = true;
							break;
						}
					}
				}
			}
		}

		return changed;
	}

	bool BacktrackingSolver::propagateHiddenSingles(Board& board) {
		bool changed = false;

		// Check rows
		for (int i = 0; i < size_; ++i) {
			for (int v = 0; v < size_; ++v) {
				if (row_used_[i].test(v)) continue;

				int count = 0;
				int lastCol = -1;

				for (int j = 0; j < size_; ++j) {
					if (board.get(i, j) == 0 && candidates_[i][j].test(v)) {
						++count;
						lastCol = j;
					}
				}

				if (count == 1 && lastCol != -1) {
					Cell value = v + 1;
					board.set(i, lastCol, value);
					updateCandidates(i, lastCol, value);
					changed = true;
				}
			}
		}

		// Check columns
		for (int j = 0; j < size_; ++j) {
			for (int v = 0; v < size_; ++v) {
				if (col_used_[j].test(v)) continue;

				int count = 0;
				int lastRow = -1;

				for (int i = 0; i < size_; ++i) {
					if (board.get(i, j) == 0 && candidates_[i][j].test(v)) {
						++count;
						lastRow = i;
					}
				}

				if (count == 1 && lastRow != -1) {
					Cell value = v + 1;
					board.set(lastRow, j, value);
					updateCandidates(lastRow, j, value);
					changed = true;
				}
			}
		}

		// Check boxes
		for (int boxIdx = 0; boxIdx < static_cast<int>(box_used_.size()); ++boxIdx) {
			int startRow = (boxIdx / (size_ / box_cols_)) * box_rows_;
			int startCol = (boxIdx % (size_ / box_cols_)) * box_cols_;

			for (int v = 0; v < size_; ++v) {
				if (box_used_[boxIdx].test(v)) continue;

				int count = 0;
				int lastR = -1, lastC = -1;

				for (int di = 0; di < box_rows_; ++di) {
					for (int dj = 0; dj < box_cols_; ++dj) {
						int r = startRow + di;
						int c = startCol + dj;

						if (board.get(r, c) == 0 && candidates_[r][c].test(v)) {
							++count;
							lastR = r;
							lastC = c;
						}
					}
				}

				if (count == 1 && lastR != -1) {
					Cell value = v + 1;
					board.set(lastR, lastC, value);
					updateCandidates(lastR, lastC, value);
					changed = true;
				}
			}
		}

		return changed;
	}

	void BacktrackingSolver::updateCandidates(int row, int col, Cell value) {
		int v = value - 1;

		// Mark as used
		row_used_[row].set(v);
		col_used_[col].set(v);
		box_used_[getBoxIndex(row, col)].set(v);

		// Clear this cell's candidates
		candidates_[row][col].reset();

		// Remove from row
		for (int j = 0; j < size_; ++j) {
			candidates_[row][j].reset(v);
		}

		// Remove from column
		for (int i = 0; i < size_; ++i) {
			candidates_[i][col].reset(v);
		}

		// Remove from box
		int startRow = (row / box_rows_) * box_rows_;
		int startCol = (col / box_cols_) * box_cols_;

		for (int di = 0; di < box_rows_; ++di) {
			for (int dj = 0; dj < box_cols_; ++dj) {
				candidates_[startRow + di][startCol + dj].reset(v);
			}
		}
	}

	void BacktrackingSolver::restoreCandidates(int row, int col, Cell value) {
		int v = value - 1;

		// Unmark as used
		row_used_[row].reset(v);
		col_used_[col].reset(v);
		box_used_[getBoxIndex(row, col)].reset(v);
	}

	int BacktrackingSolver::candidateCount(int row, int col) const {
		return static_cast<int>(candidates_[row][col].count());
	}

	std::pair<int, int> BacktrackingSolver::selectBestCell(const Board& board) const {
		if (!use_mrv_) {
			// Simple: find first empty
			return board.findFirstEmpty();
		}

		// MRV: find cell with minimum remaining values
		int minCount = std::numeric_limits<int>::max();
		int bestRow = -1, bestCol = -1;

		for (int i = 0; i < size_; ++i) {
			for (int j = 0; j < size_; ++j) {
				if (board.get(i, j) == 0) {
					int count = candidateCount(i, j);
					if (count < minCount) {
						minCount = count;
						bestRow = i;
						bestCol = j;

						// Can't do better than 1
						if (minCount == 1) {
							return { bestRow, bestCol };
						}
					}
				}
			}
		}

		return { bestRow, bestCol };
	}

} // namespace sudoku
