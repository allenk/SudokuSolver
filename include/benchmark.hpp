#pragma once

#include "board.hpp"
#include "solver.hpp"
#include "types.hpp"
#include <vector>
#include <functional>
#include <map>
#include <future>
#include <thread>

namespace sudoku {

/**
 * Benchmark class for performance measurement
 *
 * Uses high-resolution timing (chrono::steady_clock) for accurate measurements.
 * Supports:
 * - Single-threaded benchmark with statistics
 * - Multi-threaded benchmark for throughput testing
 * - Comparison between algorithms
 * - Batch processing
 */
class Benchmark {
public:
    struct Config {
        int runs = 10;           // Number of benchmark runs per worker
        int warmup_runs = 2;     // Warm-up runs (single-threaded only)
        int num_workers = 1;     // Number of parallel workers for multi-threaded mode
        bool verbose = false;    // Print each run
    };

    Benchmark() = default;
    explicit Benchmark(const Config& config) : config_(config) {}

    // Single-threaded benchmark
    BenchmarkResult run(const Board& puzzle, ISolver& solver);

    // Multi-threaded benchmark
    MultithreadResult runMultithreaded(const Board& puzzle, SolverAlgorithm algo);

    // Compare algorithms (single-threaded)
    std::map<SolverAlgorithm, BenchmarkResult> compare(
        const Board& puzzle,
        const std::vector<SolverAlgorithm>& algorithms);

    // Compare algorithms (multi-threaded)
    std::map<SolverAlgorithm, MultithreadResult> compareMultithreaded(
        const Board& puzzle,
        const std::vector<SolverAlgorithm>& algorithms);

    // Run benchmark on multiple puzzles
    std::vector<BenchmarkResult> runBatch(
        const std::vector<Board>& puzzles,
        ISolver& solver);

    // Generate report
    std::string generateReport(const BenchmarkResult& result) const;
    std::string generateMultithreadReport(const MultithreadResult& result) const;
    std::string generateComparisonReport(
        const std::map<SolverAlgorithm, BenchmarkResult>& results) const;
    std::string generateMultithreadComparisonReport(
        const std::map<SolverAlgorithm, MultithreadResult>& results) const;

    // Print results to console
    void printResult(const BenchmarkResult& result) const;
    void printMultithreadResult(const MultithreadResult& result) const;
    void printComparison(
        const std::map<SolverAlgorithm, BenchmarkResult>& results) const;
    void printMultithreadComparison(
        const std::map<SolverAlgorithm, MultithreadResult>& results) const;

    // Configuration
    void setConfig(const Config& config) { config_ = config; }
    const Config& getConfig() const { return config_; }

    // Utility: get hardware concurrency
    static int getHardwareConcurrency() {
        return static_cast<int>(std::thread::hardware_concurrency());
    }

private:
    Config config_;

    // Statistics helpers
    double calculateMean(const std::vector<double>& values) const;
    double calculateStdDev(const std::vector<double>& values, double mean) const;

    // Worker task for multi-threaded benchmark
    BenchmarkResult workerTask(const Board& puzzle, SolverAlgorithm algo, int workerId);
};

// Performance profiler for detailed analysis
class Profiler {
public:
    struct Section {
        std::string name;
        double total_time_ms = 0.0;
        int call_count = 0;
    };

    void beginSection(const std::string& name);
    void endSection(const std::string& name);

    void reset();
    std::string getReport() const;

private:
    std::map<std::string, Section> sections_;
    std::map<std::string, Timer> active_timers_;
};

// RAII profiler section
class ProfileScope {
public:
    ProfileScope(Profiler& profiler, const std::string& name)
        : profiler_(profiler), name_(name) {
        profiler_.beginSection(name_);
    }

    ~ProfileScope() {
        profiler_.endSection(name_);
    }

private:
    Profiler& profiler_;
    std::string name_;
};

} // namespace sudoku
