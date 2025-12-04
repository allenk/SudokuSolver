#pragma once

#include <cstdint>
#include <vector>
#include <bitset>
#include <array>
#include <string>
#include <chrono>
#include <optional>
#include <memory>
#include <functional>

namespace sudoku {

// Type aliases for clarity
using Cell = int;                          // Cell value (0 = empty, 1-N = filled)
using Grid = std::vector<std::vector<Cell>>;
using Candidates = std::vector<std::vector<std::bitset<32>>>; // Max 32x32 board

// Board dimensions
struct BoardDimension {
    int size;        // Total size (e.g., 9 for 9x9)
    int box_rows;    // Rows per box (e.g., 3 for 9x9)
    int box_cols;    // Columns per box (e.g., 3 for 9x9)

    BoardDimension() : size(9), box_rows(3), box_cols(3) {}
    BoardDimension(int s, int br, int bc) : size(s), box_rows(br), box_cols(bc) {}

    // Standard sizes
    static BoardDimension Standard4x4() { return {4, 2, 2}; }
    static BoardDimension Standard6x6() { return {6, 2, 3}; }
    static BoardDimension Standard9x9() { return {9, 3, 3}; }
    static BoardDimension Standard12x12() { return {12, 3, 4}; }
    static BoardDimension Standard16x16() { return {16, 4, 4}; }
    static BoardDimension Standard25x25() { return {25, 5, 5}; }

    // Auto-detect box dimensions from size
    static BoardDimension FromSize(int size) {
        // Try to find the most square-like box dimensions
        for (int i = static_cast<int>(std::sqrt(size)); i >= 1; --i) {
            if (size % i == 0) {
                return {size, i, size / i};
            }
        }
        return {size, 1, size}; // Fallback
    }

    bool isValid() const {
        return size > 0 && box_rows > 0 && box_cols > 0 &&
               box_rows * box_cols == size;
    }
};

// Solution result
struct SolveResult {
    bool solved = false;
    Grid solution;
    int iterations = 0;
    int backtracks = 0;
    double time_ms = 0.0;
    std::string algorithm;
    std::string error_message;

    // Multiple solutions support
    bool has_unique_solution = false;
    int solution_count = 0;
};

// Benchmark result
struct BenchmarkResult {
    std::string algorithm;
    double min_time_ms = 0.0;
    double max_time_ms = 0.0;
    double avg_time_ms = 0.0;
    double std_dev_ms = 0.0;
    int total_iterations = 0;
    int total_backtracks = 0;
    int runs = 0;
    bool all_solved = true;

    // Last solve result (for solution access)
    SolveResult result;
};

// Multi-threaded benchmark result
struct MultithreadResult {
    std::string algorithm;
    int num_workers = 0;
    int runs_per_worker = 0;
    int total_runs = 0;
    bool all_solved = true;

    // Overall timing
    double wall_time_ms = 0.0;          // Real elapsed time (wall clock)
    double total_cpu_time_ms = 0.0;     // Sum of all worker times
    double throughput = 0.0;            // Solves per second

    // Per-worker results
    std::vector<BenchmarkResult> worker_results;

    // Aggregated statistics
    double avg_time_per_solve_ms = 0.0;
    double speedup = 0.0;               // vs single-threaded baseline
    double efficiency = 0.0;            // speedup / num_workers (ideal = 1.0)
};

// Timing utilities using high-resolution clock
class Timer {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = std::chrono::duration<double, std::milli>;

    void start() {
        start_time_ = Clock::now();
        running_ = true;
    }

    void stop() {
        if (running_) {
            end_time_ = Clock::now();
            running_ = false;
        }
    }

    double elapsed_ms() const {
        TimePoint end = running_ ? Clock::now() : end_time_;
        return Duration(end - start_time_).count();
    }

    double elapsed_us() const {
        return elapsed_ms() * 1000.0;
    }

    double elapsed_ns() const {
        TimePoint end = running_ ? Clock::now() : end_time_;
        return std::chrono::duration<double, std::nano>(end - start_time_).count();
    }

private:
    TimePoint start_time_;
    TimePoint end_time_;
    bool running_ = false;
};

// Scoped timer for automatic measurement
class ScopedTimer {
public:
    explicit ScopedTimer(double& result_ms) : result_(result_ms) {
        timer_.start();
    }

    ~ScopedTimer() {
        timer_.stop();
        result_ = timer_.elapsed_ms();
    }

private:
    Timer timer_;
    double& result_;
};

// OCR confidence result
struct OCRResult {
    Grid grid;
    BoardDimension dimension;
    std::vector<std::vector<float>> confidences;  // Confidence per cell
    bool success = false;
    std::string error_message;
};

// Input source type
enum class InputSource {
    JSON,
    Image,
    Manual
};

// Solver algorithm type
enum class SolverAlgorithm {
    Backtracking,      // Simple backtracking with constraint propagation
    DancingLinks,      // Donald Knuth's Algorithm X with Dancing Links
    Hybrid,            // Use DLX for difficult puzzles, backtrack for easy
    Auto               // Automatically select best algorithm
};

inline std::string algorithmToString(SolverAlgorithm algo) {
    switch (algo) {
        case SolverAlgorithm::Backtracking: return "Backtracking";
        case SolverAlgorithm::DancingLinks: return "Dancing Links (DLX)";
        case SolverAlgorithm::Hybrid: return "Hybrid";
        case SolverAlgorithm::Auto: return "Auto";
        default: return "Unknown";
    }
}

} // namespace sudoku
