#include "benchmark.hpp"
#include "solver_backtrack.hpp"
#include "solver_dlx.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <future>
#include <thread>

namespace sudoku {

BenchmarkResult Benchmark::run(const Board& puzzle, ISolver& solver) {
    BenchmarkResult result;
    result.algorithm = solver.name();

    std::vector<double> times;
    std::vector<int> iterations;
    std::vector<int> backtracks;
    bool allSolved = true;

    // Warm-up runs
    for (int i = 0; i < config_.warmup_runs; ++i) {
        solver.reset();
        solver.solve(puzzle);
    }

    // Actual benchmark runs
    for (int i = 0; i < config_.runs; ++i) {
        solver.reset();

        auto solveResult = solver.solve(puzzle);

        times.push_back(solveResult.time_ms);
        iterations.push_back(solveResult.iterations);
        backtracks.push_back(solveResult.backtracks);

        if (!solveResult.solved) {
            allSolved = false;
        }

        if (config_.verbose) {
            std::cout << "  Run " << (i + 1) << ": "
                      << std::fixed << std::setprecision(3)
                      << solveResult.time_ms << " ms, "
                      << solveResult.iterations << " iterations, "
                      << solveResult.backtracks << " backtracks"
                      << (solveResult.solved ? "" : " [FAILED]")
                      << "\n";
        }
    }

    // Calculate statistics
    result.runs = config_.runs;
    result.all_solved = allSolved;

    if (!times.empty()) {
        result.min_time_ms = *std::min_element(times.begin(), times.end());
        result.max_time_ms = *std::max_element(times.begin(), times.end());
        result.avg_time_ms = calculateMean(times);
        result.std_dev_ms = calculateStdDev(times, result.avg_time_ms);

        result.total_iterations = std::accumulate(iterations.begin(), iterations.end(), 0);
        result.total_backtracks = std::accumulate(backtracks.begin(), backtracks.end(), 0);
    }

    return result;
}

std::map<SolverAlgorithm, BenchmarkResult> Benchmark::compare(
    const Board& puzzle,
    const std::vector<SolverAlgorithm>& algorithms) {

    std::map<SolverAlgorithm, BenchmarkResult> results;

    for (auto algo : algorithms) {
        auto solver = SolverFactory::create(algo);
        results[algo] = run(puzzle, *solver);
    }

    return results;
}

std::vector<BenchmarkResult> Benchmark::runBatch(
    const std::vector<Board>& puzzles,
    ISolver& solver) {

    std::vector<BenchmarkResult> results;

    for (size_t i = 0; i < puzzles.size(); ++i) {
        if (config_.verbose) {
            std::cout << "Puzzle " << (i + 1) << "/" << puzzles.size() << ":\n";
        }
        results.push_back(run(puzzles[i], solver));
    }

    return results;
}

double Benchmark::calculateMean(const std::vector<double>& values) const {
    if (values.empty()) return 0.0;
    return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
}

double Benchmark::calculateStdDev(const std::vector<double>& values, double mean) const {
    if (values.size() < 2) return 0.0;

    double sum = 0.0;
    for (double v : values) {
        sum += (v - mean) * (v - mean);
    }
    return std::sqrt(sum / (values.size() - 1));
}

std::string Benchmark::generateReport(const BenchmarkResult& result) const {
    std::ostringstream oss;

    oss << "=== Benchmark Report ===\n";
    oss << "Algorithm: " << result.algorithm << "\n";
    oss << "Runs: " << result.runs << "\n";
    oss << "All Solved: " << (result.all_solved ? "Yes" : "No") << "\n\n";

    oss << "Timing (ms):\n";
    oss << std::fixed << std::setprecision(6);
    oss << "  Min:     " << std::setw(12) << result.min_time_ms << "\n";
    oss << "  Max:     " << std::setw(12) << result.max_time_ms << "\n";
    oss << "  Average: " << std::setw(12) << result.avg_time_ms << "\n";
    oss << "  Std Dev: " << std::setw(12) << result.std_dev_ms << "\n\n";

    oss << "Statistics:\n";
    oss << "  Total Iterations: " << result.total_iterations << "\n";
    oss << "  Total Backtracks: " << result.total_backtracks << "\n";
    oss << "  Avg Iterations:   " << (result.total_iterations / std::max(1, result.runs)) << "\n";
    oss << "  Avg Backtracks:   " << (result.total_backtracks / std::max(1, result.runs)) << "\n";

    return oss.str();
}

std::string Benchmark::generateComparisonReport(
    const std::map<SolverAlgorithm, BenchmarkResult>& results) const {

    std::ostringstream oss;

    oss << "=== Algorithm Comparison ===\n\n";

    // Calculate max algorithm name width dynamically
    size_t maxNameWidth = std::string("Algorithm").length();
    for (const auto& [algo, result] : results) {
        maxNameWidth = std::max(maxNameWidth, result.algorithm.length());
    }
    maxNameWidth += 2;  // Add padding

    // Column widths
    const int colWidth = 12;
    const int solvedWidth = 8;
    const size_t totalWidth = maxNameWidth + colWidth * 4 + solvedWidth + 2;

    // Table header
    oss << std::left << std::setw(static_cast<int>(maxNameWidth)) << "Algorithm"
        << std::right << std::setw(colWidth) << "Min (ms)"
        << std::setw(colWidth) << "Avg (ms)"
        << std::setw(colWidth) << "Max (ms)"
        << std::setw(colWidth) << "Std Dev"
        << std::setw(solvedWidth) << "Solved"
        << "\n";
    oss << std::string(totalWidth, '-') << "\n";

    // Find best average for highlighting
    double bestAvg = std::numeric_limits<double>::max();
    for (const auto& [algo, result] : results) {
        if (result.avg_time_ms < bestAvg) {
            bestAvg = result.avg_time_ms;
        }
    }

    oss << std::fixed << std::setprecision(6);

    for (const auto& [algo, result] : results) {
        bool isBest = (result.avg_time_ms == bestAvg);

        oss << std::left << std::setw(static_cast<int>(maxNameWidth)) << result.algorithm
            << std::right << std::setw(colWidth) << result.min_time_ms
            << std::setw(colWidth) << result.avg_time_ms
            << std::setw(colWidth) << result.max_time_ms
            << std::setw(colWidth) << result.std_dev_ms
            << std::setw(solvedWidth) << (result.all_solved ? "Yes" : "No")
            << (isBest ? " *" : "")
            << "\n";
    }

    oss << "\n* = Best average time\n";

    return oss.str();
}

void Benchmark::printResult(const BenchmarkResult& result) const {
    std::cout << generateReport(result);
}

void Benchmark::printComparison(
    const std::map<SolverAlgorithm, BenchmarkResult>& results) const {
    std::cout << generateComparisonReport(results);
}

// Worker task for multi-threaded benchmark
BenchmarkResult Benchmark::workerTask(const Board& puzzle, SolverAlgorithm algo, int workerId) {
    (void)workerId;

    // Each worker creates its own solver (solvers are not thread-safe)
    auto solver = SolverFactory::create(algo);

    BenchmarkResult result;
    result.algorithm = solver->name();

    std::vector<double> times;
    std::vector<int> iterations;
    std::vector<int> backtracks;
    bool allSolved = true;

    // Run benchmark iterations
    for (int i = 0; i < config_.runs; ++i) {
        solver->reset();

        auto solveResult = solver->solve(puzzle);

        times.push_back(solveResult.time_ms);
        iterations.push_back(solveResult.iterations);
        backtracks.push_back(solveResult.backtracks);

        if (!solveResult.solved) {
            allSolved = false;
        }

        // Save last result
        result.result = solveResult;
    }

    // Calculate statistics
    result.runs = config_.runs;
    result.all_solved = allSolved;

    if (!times.empty()) {
        result.min_time_ms = *std::min_element(times.begin(), times.end());
        result.max_time_ms = *std::max_element(times.begin(), times.end());
        result.avg_time_ms = calculateMean(times);
        result.std_dev_ms = calculateStdDev(times, result.avg_time_ms);

        result.total_iterations = std::accumulate(iterations.begin(), iterations.end(), 0);
        result.total_backtracks = std::accumulate(backtracks.begin(), backtracks.end(), 0);
    }

    return result;
}

MultithreadResult Benchmark::runMultithreaded(const Board& puzzle, SolverAlgorithm algo) {
    MultithreadResult result;
    result.algorithm = algorithmToString(algo);
    result.num_workers = config_.num_workers;
    result.runs_per_worker = config_.runs;
    result.total_runs = config_.num_workers * config_.runs;

    // Fixed baseline calculation for stable Speedup measurement
    // Use 100 runs with 10 warmup runs, independent of config_.runs
    const int BASELINE_WARMUP = 10;
    const int BASELINE_RUNS = 100;
    double singleSolveTime = 0.0;
    {
        auto solver = SolverFactory::create(algo);

        // Warmup runs
        for (int i = 0; i < BASELINE_WARMUP; ++i) {
            solver->reset();
            solver->solve(puzzle);
        }

        // Measure baseline
        Timer timer;
        timer.start();
        for (int i = 0; i < BASELINE_RUNS; ++i) {
            solver->reset();
            solver->solve(puzzle);
        }
        timer.stop();
        singleSolveTime = timer.elapsed_ms() / BASELINE_RUNS;  // Average per solve
    }

    // Launch worker threads
    std::vector<std::future<BenchmarkResult>> futures;

    Timer wallTimer;
    wallTimer.start();

    for (int w = 0; w < config_.num_workers; ++w) {
        futures.push_back(std::async(std::launch::async,
            &Benchmark::workerTask, this, std::cref(puzzle), algo, w));
    }

    // Collect results
    result.worker_results.reserve(config_.num_workers);
    result.all_solved = true;
    result.total_cpu_time_ms = 0.0;

    for (auto& future : futures) {
        auto workerResult = future.get();
        result.total_cpu_time_ms += workerResult.avg_time_ms * workerResult.runs;
        if (!workerResult.all_solved) {
            result.all_solved = false;
        }
        result.worker_results.push_back(std::move(workerResult));
    }

    wallTimer.stop();
    result.wall_time_ms = wallTimer.elapsed_ms();

    // Calculate aggregated statistics
    result.avg_time_per_solve_ms = result.total_cpu_time_ms / result.total_runs;
    result.throughput = (result.total_runs / result.wall_time_ms) * 1000.0;  // solves per second

    // Speedup: compare with single-thread doing the SAME total work
    // singleThreadTime = time for 1 thread to do (runs) iterations
    // To do total_runs = (runs * num_workers), single thread needs (singleThreadTime * num_workers)
    double expectedSingleThreadTime = singleSolveTime * result.total_runs;
    result.speedup = expectedSingleThreadTime / result.wall_time_ms;
    result.efficiency = result.speedup / config_.num_workers;

    return result;
}

std::map<SolverAlgorithm, MultithreadResult> Benchmark::compareMultithreaded(
    const Board& puzzle,
    const std::vector<SolverAlgorithm>& algorithms) {

    std::map<SolverAlgorithm, MultithreadResult> results;

    for (auto algo : algorithms) {
        results[algo] = runMultithreaded(puzzle, algo);
    }

    return results;
}

std::string Benchmark::generateMultithreadReport(const MultithreadResult& result) const {
    std::ostringstream oss;

    oss << "=== Multi-threaded Benchmark Report ===\n";
    oss << "Algorithm: " << result.algorithm << "\n";
    oss << "Workers: " << result.num_workers << "\n";
    oss << "Runs per worker: " << result.runs_per_worker << "\n";
    oss << "Total runs: " << result.total_runs << "\n";
    oss << "All Solved: " << (result.all_solved ? "Yes" : "No") << "\n\n";

    oss << std::fixed << std::setprecision(3);
    oss << "Performance:\n";
    oss << "  Wall time:      " << std::setw(12) << result.wall_time_ms << " ms\n";
    oss << "  Total CPU time: " << std::setw(12) << result.total_cpu_time_ms << " ms\n";
    oss << "  Throughput:     " << std::setw(12) << result.throughput << " solves/sec\n";
    oss << "  Speedup:        " << std::setw(12) << result.speedup << "x\n";
    oss << "  Efficiency:     " << std::setw(12) << (result.efficiency * 100) << "%\n\n";

    oss << "Per-worker statistics:\n";
    oss << std::left << std::setw(10) << "Worker"
        << std::right << std::setw(12) << "Avg (ms)"
        << std::setw(12) << "Min (ms)"
        << std::setw(12) << "Max (ms)"
        << "\n";
    oss << std::string(46, '-') << "\n";

    for (size_t i = 0; i < result.worker_results.size(); ++i) {
        const auto& wr = result.worker_results[i];
        oss << std::left << std::setw(10) << ("W" + std::to_string(i))
            << std::right << std::setw(12) << wr.avg_time_ms
            << std::setw(12) << wr.min_time_ms
            << std::setw(12) << wr.max_time_ms
            << "\n";
    }

    return oss.str();
}

std::string Benchmark::generateMultithreadComparisonReport(
    const std::map<SolverAlgorithm, MultithreadResult>& results) const {

    std::ostringstream oss;

    oss << "=== Multi-threaded Algorithm Comparison ===\n";
    oss << "Workers: " << config_.num_workers << " | ";
    oss << "Runs per worker: " << config_.runs << "\n\n";

    // Calculate max algorithm name width
    size_t maxNameWidth = std::string("Algorithm").length();
    for (const auto& [algo, result] : results) {
        maxNameWidth = std::max(maxNameWidth, result.algorithm.length());
    }
    maxNameWidth += 2;

    const int colWidth = 14;

    // Header
    oss << std::left << std::setw(static_cast<int>(maxNameWidth)) << "Algorithm"
        << std::right << std::setw(colWidth) << "Wall (ms)"
        << std::setw(colWidth) << "Throughput"
        << std::setw(colWidth) << "Speedup"
        << std::setw(colWidth) << "Efficiency"
        << "\n";
    oss << std::string(maxNameWidth + colWidth * 4, '-') << "\n";

    // Find best throughput
    double bestThroughput = 0.0;
    for (const auto& [algo, result] : results) {
        if (result.throughput > bestThroughput) {
            bestThroughput = result.throughput;
        }
    }

    oss << std::fixed << std::setprecision(2);

    for (const auto& [algo, result] : results) {
        bool isBest = (result.throughput == bestThroughput);

        oss << std::left << std::setw(static_cast<int>(maxNameWidth)) << result.algorithm
            << std::right << std::setw(colWidth) << result.wall_time_ms
            << std::setw(colWidth - 2) << result.throughput << "/s"
            << std::setw(colWidth - 1) << result.speedup << "x"
            << std::setw(colWidth - 1) << (result.efficiency * 100) << "%"
            << (isBest ? " *" : "")
            << "\n";
    }

    oss << "\n* = Best throughput\n";

    return oss.str();
}

void Benchmark::printMultithreadResult(const MultithreadResult& result) const {
    std::cout << generateMultithreadReport(result);
}

void Benchmark::printMultithreadComparison(
    const std::map<SolverAlgorithm, MultithreadResult>& results) const {
    std::cout << generateMultithreadComparisonReport(results);
}

// Profiler implementation
void Profiler::beginSection(const std::string& name) {
    if (sections_.find(name) == sections_.end()) {
        sections_[name] = Section{name, 0.0, 0};
    }
    active_timers_[name].start();
}

void Profiler::endSection(const std::string& name) {
    auto it = active_timers_.find(name);
    if (it != active_timers_.end()) {
        it->second.stop();
        sections_[name].total_time_ms += it->second.elapsed_ms();
        sections_[name].call_count++;
    }
}

void Profiler::reset() {
    sections_.clear();
    active_timers_.clear();
}

std::string Profiler::getReport() const {
    std::ostringstream oss;

    oss << "=== Profile Report ===\n\n";
    oss << std::left << std::setw(30) << "Section"
        << std::right << std::setw(15) << "Total (ms)"
        << std::setw(10) << "Calls"
        << std::setw(15) << "Avg (ms)"
        << "\n";
    oss << std::string(70, '-') << "\n";

    for (const auto& [name, section] : sections_) {
        double avg = section.call_count > 0 ?
            section.total_time_ms / section.call_count : 0.0;

        oss << std::left << std::setw(30) << section.name
            << std::right << std::fixed << std::setprecision(6)
            << std::setw(15) << section.total_time_ms
            << std::setw(10) << section.call_count
            << std::setw(15) << avg
            << "\n";
    }

    return oss.str();
}

} // namespace sudoku
