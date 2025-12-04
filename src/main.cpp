#include <iostream>
#include <iomanip>
#include <string>
#include <filesystem>

#include <CLI/CLI.hpp>

#include "board.hpp"
#include "solver.hpp"
#include "solver_backtrack.hpp"
#include "solver_dlx.hpp"
#include "json_handler.hpp"
#include "ocr_processor.hpp"
#include "benchmark.hpp"
#include "types.hpp"
#include "system_info.hpp"

#ifdef USE_OPENMP
#include <omp.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

using namespace sudoku;

// ANSI color codes for console output
namespace Color {
    const std::string Reset   = "\033[0m";
    const std::string Bold    = "\033[1m";
    const std::string Red     = "\033[31m";
    const std::string Green   = "\033[32m";
    const std::string Yellow  = "\033[33m";
    const std::string Blue    = "\033[34m";
    const std::string Cyan    = "\033[36m";
    const std::string Magenta = "\033[35m";
    const std::string White   = "\033[37m";
}

void printHeader() {
    std::cout << Color::Cyan << Color::Bold;
    std::cout << R"(
  ____            _       _            ____        _
 / ___| _   _  __| | ___ | | ___   _  / ___|  ___ | |_   _____ _ __
 \___ \| | | |/ _` |/ _ \| |/ / | | | \___ \ / _ \| \ \ / / _ \ '__|
  ___) | |_| | (_| | (_) |   <| |_| |  ___) | (_) | |\ V /  __/ |
 |____/ \__,_|\__,_|\___/|_|\_\\__,_| |____/ \___/|_| \_/ \___|_|

)" << Color::Reset;
    std::cout << "  High-Performance Sudoku Solver v1.0.0 (AllenK, Kwyshell)\n";
    std::cout << "  Using Dancing Links (DLX) & Constraint Propagation\n";

#ifdef USE_OPENMP
    std::cout << "  OpenMP: Enabled (" << omp_get_max_threads() << " threads)\n";
#else
    std::cout << "  OpenMP: Disabled\n";
#endif
    std::cout << "\n";
}

void printSystemInfo() {
    auto info = SystemInfoDetector::detect();

    // Helper to truncate long strings
    auto truncate = [](const std::string& str, size_t maxLen) -> std::string {
        if (str.length() <= maxLen) return str;
        return str.substr(0, maxLen - 3) + "...";
        };

    std::cout << Color::Magenta;
    std::cout << "+-------------------------------------------------------------+\n";
    std::cout << "|" << Color::Bold << "                    System Information                       " << Color::Reset << Color::Magenta << "|\n";
    std::cout << "+-------------------------------------------------------------+\n";
    std::cout << Color::Reset;

    // CPU
    std::cout << Color::Magenta << "|" << Color::Reset;
    std::cout << " CPU: " << Color::White << std::left << std::setw(55)
        << truncate(info.cpu_model, 55) << Color::Reset;
    std::cout << Color::Magenta << "|" << Color::Reset << "\n";

    std::ostringstream coresStr;
    coresStr << info.physical_cores << " cores / " << info.logical_cores << " threads";
    std::cout << Color::Magenta << "|" << Color::Reset;
    std::cout << " Cores: " << Color::White << std::left << std::setw(53)
        << coresStr.str() << Color::Reset;
    std::cout << Color::Magenta << "|" << Color::Reset << "\n";

    std::cout << Color::Magenta << "|" << Color::Reset;
    std::cout << " Clock: " << Color::White << std::left << std::setw(53)
        << truncate(info.cpuClockFormatted(), 53) << Color::Reset;
    std::cout << Color::Magenta << "|" << Color::Reset << "\n";

    std::cout << Color::Magenta << "+-------------------------------------------------------------+" << Color::Reset << "\n";

    // Memory
    std::ostringstream ramStr;
    ramStr << info.totalRamFormatted() << " (Available: " << info.availableRamFormatted() << ")";
    std::cout << Color::Magenta << "|" << Color::Reset;
    std::cout << " RAM: " << Color::White << std::left << std::setw(55)
        << truncate(ramStr.str(), 55) << Color::Reset;
    std::cout << Color::Magenta << "|" << Color::Reset << "\n";

    std::cout << Color::Magenta << "+-------------------------------------------------------------+" << Color::Reset << "\n";

    // OS
    std::ostringstream osStr;
    osStr << info.os_name;
    if (!info.os_version.empty() && info.os_version != "Unknown") {
        osStr << " " << info.os_version;
    }
    std::cout << Color::Magenta << "|" << Color::Reset;
    std::cout << " OS: " << Color::White << std::left << std::setw(56)
        << truncate(osStr.str(), 56) << Color::Reset;
    std::cout << Color::Magenta << "|" << Color::Reset << "\n";

    std::cout << Color::Magenta << "+-------------------------------------------------------------+" << Color::Reset << "\n";

    // Compiler
    std::cout << Color::Magenta << "|" << Color::Reset;
    std::cout << " Compiler: " << Color::White << std::left << std::setw(50)
        << truncate(info.compiler_info, 50) << Color::Reset;
    std::cout << Color::Magenta << "|" << Color::Reset << "\n";

    std::cout << Color::Magenta << "|" << Color::Reset;
    std::cout << " Build: " << Color::White << std::left << std::setw(53)
        << info.build_type << Color::Reset;
    std::cout << Color::Magenta << "|" << Color::Reset << "\n";

    std::cout << Color::Magenta << "+-------------------------------------------------------------+" << Color::Reset << "\n";
    std::cout << "\n";
}

void printBoard(const Board& board, const std::string& title) {
    std::cout << Color::Yellow << title << Color::Reset << "\n";
    std::cout << board.toString();
}

void printResult(const SolveResult& result) {
    std::cout << "\n" << Color::Bold << "=== Solution Result ===" << Color::Reset << "\n";

    if (result.solved) {
        std::cout << Color::Green << "Status: SOLVED" << Color::Reset << "\n";
    } else {
        std::cout << Color::Red << "Status: FAILED" << Color::Reset << "\n";
        if (!result.error_message.empty()) {
            std::cout << "Error: " << result.error_message << "\n";
        }
    }

    std::cout << "Algorithm: " << result.algorithm << "\n";
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Time: " << result.time_ms << " ms";

    // Also show in microseconds for fast solves
    if (result.time_ms < 1.0) {
        std::cout << " (" << (result.time_ms * 1000.0) << " μs)";
    }
    std::cout << "\n";

    std::cout << "Iterations: " << result.iterations << "\n";
    std::cout << "Backtracks: " << result.backtracks << "\n";
}

Board loadBoard(const std::string& input, bool isImage) {
    if (isImage) {
        std::cout << "Processing image: " << input << "\n";
        OCRProcessor ocr;
        ocr.setDebugMode(false);

        auto result = ocr.processImage(input);

        if (!result.success) {
            throw std::runtime_error("OCR failed: " + result.error_message);
        }

        if (!result.error_message.empty()) {
            std::cout << Color::Yellow << "Warning: " << result.error_message << Color::Reset << "\n";
        }
        std::cout << "\n";

        return Board(result.grid, result.dimension);
    } else {
        return JSONHandler::loadFromFile(input);
    }
}

bool isImageFile(const std::string& path) {
    std::string ext = std::filesystem::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" ||
           ext == ".bmp" || ext == ".tiff" || ext == ".tif";
}

int main(int argc, char* argv[]) {

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    CLI::App app{"High-Performance Sudoku Solver"};

    // Input options
    std::string inputFile;
    app.add_option("-i,--input", inputFile, "Input file (JSON or image)")
        ->check(CLI::ExistingFile);

    // Algorithm selection
    std::string algorithm = "dlx";
    app.add_option("-a,--algorithm", algorithm,
                   "Solving algorithm: dlx, backtrack, compare")
        ->default_val("dlx");

    // Benchmark options
    int benchmarkRuns = 0;
    app.add_option("-b,--benchmark", benchmarkRuns,
                   "Run benchmark with N iterations")
        ->default_val(0);

    int numWorkers = 1;
    app.add_option("-w,--workers", numWorkers,
                   "Number of parallel workers for benchmark (0 = auto)")
        ->default_val(1);

    // Output options
    std::string outputFile;
    app.add_option("-o,--output", outputFile, "Output solution to JSON file");

    bool verbose = false;
    app.add_flag("-v,--verbose", verbose, "Verbose output");

    bool quiet = false;
    app.add_flag("-q,--quiet", quiet, "Minimal output");

    // Help for JSON format
    bool showJsonHelp = false;
    app.add_flag("--json-help", showJsonHelp, "Show JSON input format help");

    // Manual input for testing
    std::string puzzleString;
    app.add_option("-p,--puzzle", puzzleString,
                   "Puzzle as a string (use . or 0 for empty)");

    // Check unique solution
    bool checkUnique = false;
    app.add_flag("-u,--unique", checkUnique, "Check if solution is unique");

    // System info option
    bool showSysInfo = true;
    app.add_flag("--no-sysinfo", [&](auto) { showSysInfo = false; }, "Disable system information");

    CLI11_PARSE(app, argc, argv);

    // Show JSON help
    if (showJsonHelp) {
        std::cout << JSONHandler::getFormatHelp();
        return 0;
    }

    if (!quiet) {
        printHeader();
        // Show system info if requested or in benchmark/compare mode
        if (showSysInfo || benchmarkRuns > 0 || algorithm == "compare") {
            printSystemInfo();
        }
    }

    try {
        Board board;

        // Load board from input
        if (!inputFile.empty()) {
            bool isImage = isImageFile(inputFile);
            board = loadBoard(inputFile, isImage);
        } else if (!puzzleString.empty()) {
            // Parse puzzle string
            nlohmann::json json;
            json["puzzle"] = puzzleString;
            board = JSONHandler::loadFromJSON(json);
        } else {
            // Default test puzzle
            std::cout << "No input provided. Using default test puzzle.\n\n";

            Grid testGrid = {
                {5, 3, 0, 0, 7, 0, 0, 0, 0},
                {6, 0, 0, 1, 9, 5, 0, 0, 0},
                {0, 9, 8, 0, 0, 0, 0, 6, 0},
                {8, 0, 0, 0, 6, 0, 0, 0, 3},
                {4, 0, 0, 8, 0, 3, 0, 0, 1},
                {7, 0, 0, 0, 2, 0, 0, 0, 6},
                {0, 6, 0, 0, 0, 0, 2, 8, 0},
                {0, 0, 0, 4, 1, 9, 0, 0, 5},
                {0, 0, 0, 0, 8, 0, 0, 7, 9}
            };

            board = Board(testGrid);
        }

        // Print input board
        if (!quiet) {
            printBoard(board, "Input Puzzle:");
            std::cout << "Size: " << board.size() << "x" << board.size() << "\n";
            std::cout << "Empty cells: " << board.countEmpty() << "\n";
            std::cout << "Fill ratio: " << std::fixed << std::setprecision(1)
                      << (board.fillRatio() * 100) << "%\n\n";
        }

        // Validate input
        if (!board.isValid()) {
            std::cerr << Color::Red << "Error: Input puzzle is invalid!" << Color::Reset << "\n";
            return 1;
        }

        // Compare algorithms
        if (algorithm == "compare") {
            // Auto-detect workers if 0
            int workers = numWorkers;
            if (workers == 0) {
                workers = Benchmark::getHardwareConcurrency();
            }

            Benchmark bench;
            Benchmark::Config config;
            config.runs = std::max(1, benchmarkRuns > 0 ? benchmarkRuns : 10);
            config.warmup_runs = 2;
            config.num_workers = workers;
            config.verbose = verbose;
            bench.setConfig(config);

            if (workers > 1) {
                // Multi-threaded comparison
                if (!quiet) {
                    std::cout << "Comparing algorithms (multi-threaded: "
                              << workers << " workers)...\n\n";
                }

                auto results = bench.compareMultithreaded(board, {
                    SolverAlgorithm::DancingLinks,
                    SolverAlgorithm::Backtracking
                });

                // Print solutions from first worker
                for (const auto& [algo, result] : results) {
                    if (!result.worker_results.empty() &&
                        result.worker_results[0].result.solved) {
                        Board solutionBoard(result.worker_results[0].result.solution,
                                          board.dimension());
                        printBoard(solutionBoard, std::string("Solution: ") + result.algorithm);
                        std::cout << "\n";
                    }
                }

                bench.printMultithreadComparison(results);
            } else {
                // Single-threaded comparison
                if (!quiet) {
                    std::cout << "Comparing algorithms...\n\n";
                }

                auto results = bench.compare(board, {
                    SolverAlgorithm::DancingLinks,
                    SolverAlgorithm::Backtracking
                });

                for (const auto& [algo, result] : results) {
                    if (result.result.solved) {
                        Board solutionBoard(result.result.solution, board.dimension());
                        printBoard(solutionBoard, std::string("Solution: ") + result.algorithm);
                        std::cout << "\n";
                    }
                }

                bench.printComparison(results);
            }
            return 0;
        }

        // Create solver
        std::unique_ptr<ISolver> solver;
        if (algorithm == "backtrack") {
            solver = SolverFactory::createBacktracking();
        } else {
            solver = SolverFactory::createDLX();
        }

        // Benchmark mode
        if (benchmarkRuns > 0) {
            // Auto-detect workers if 0
            int workers = numWorkers;
            if (workers == 0) {
                workers = Benchmark::getHardwareConcurrency();
            }

            Benchmark bench;
            Benchmark::Config config;
            config.runs = benchmarkRuns;
            config.warmup_runs = std::min(2, benchmarkRuns / 5);
            config.num_workers = workers;
            config.verbose = verbose;
            bench.setConfig(config);

            if (workers > 1) {
                // Multi-threaded benchmark
                if (!quiet) {
                    std::cout << "Running multi-threaded benchmark...\n";
                    std::cout << "  Workers: " << workers << "\n";
                    std::cout << "  Runs per worker: " << benchmarkRuns << "\n";
                    std::cout << "  Total runs: " << (workers * benchmarkRuns) << "\n\n";
                }

                SolverAlgorithm algo = (algorithm == "backtrack") ?
                    SolverAlgorithm::Backtracking : SolverAlgorithm::DancingLinks;
                auto result = bench.runMultithreaded(board, algo);
                bench.printMultithreadResult(result);
            } else {
                // Single-threaded benchmark
                if (!quiet) {
                    std::cout << "Running benchmark (" << benchmarkRuns << " iterations)...\n\n";
                }

                auto result = bench.run(board, *solver);
                bench.printResult(result);
            }

            return 0;
        }

        // Solve
        if (!quiet) {
            std::cout << "Solving with " << solver->name() << "...\n";
        }

        auto result = solver->solve(board);

        // Print result
        if (!quiet) {
            printResult(result);
        }

        if (result.solved) {
            Board solutionBoard(result.solution, board.dimension());

            if (!quiet) {
                std::cout << "\n";
                printBoard(solutionBoard, "Solution:");
            } else {
                // Quiet mode: just print the solution
                solutionBoard.printCompact(std::cout);
            }

            // Check uniqueness
            if (checkUnique) {
                std::cout << "\nChecking uniqueness...\n";
                bool unique = solver->hasUniqueSolution(board);
                if (unique) {
                    std::cout << Color::Green << "Solution is UNIQUE" << Color::Reset << "\n";
                } else {
                    std::cout << Color::Yellow << "Multiple solutions exist" << Color::Reset << "\n";
                }
            }

            // Save output
            if (!outputFile.empty()) {
                JSONHandler::saveSolutionToFile(board, result, outputFile);
                std::cout << "\nSolution saved to: " << outputFile << "\n";
            }
        }

        return result.solved ? 0 : 1;

    } catch (const std::exception& e) {
        std::cerr << Color::Red << "Error: " << e.what() << Color::Reset << "\n";
        return 1;
    }
}
