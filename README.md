# Sudoku Solver - High-Performance Cross-Platform Sudoku Solver

A high-performance, cross-platform Sudoku solver featuring multiple algorithms, OCR image recognition, comprehensive benchmarking tools, and multi-platform CI/CD support.

![Preview](preview.png)

## ✨ Features

- **Multiple Algorithms**
  - Dancing Links (DLX) - Donald Knuth's Algorithm X implementation
  - Backtracking with Constraint Propagation
  - Automatic algorithm comparison

- **OCR Support** (Windows/Linux/macOS)
  - Extract puzzles from images (PNG, JPG, BMP, TIFF)
  - Supports 9x9 and 16x16 grids
  - Automatic grid detection and perspective correction

- **Built-in Test Puzzles**
  - 9x9 classic puzzle for quick tests
  - 16x16 hard puzzle for standard benchmarks
  - 25x25 mega puzzle for stress testing

- **Comprehensive Benchmarking**
  - Single-threaded and multi-threaded benchmarks
  - Cross-platform system information detection
  - Performance metrics: throughput, speedup, efficiency
  - Professional benchmark reports

- **Cross-Platform Support**
  - Windows (x64)
  - Linux (x64)
  - macOS (Universal Binary - Intel & Apple Silicon)
  - Android (ARM64) - Solver & Benchmark only

## 🚀 Quick Start

### Prerequisites

- CMake 3.20+
- C++20 compatible compiler (MSVC 2022, GCC 11+, Clang 14+)
- vcpkg package manager

### Build with CMake Presets

```bash
# Clone repository
git clone https://github.com/user/sudoku-solver.git
cd sudoku-solver

# Windows
cmake --preset windows-x64-Release
cmake --build --preset windows-x64-Release

# Linux
cmake --preset linux-x64-Release
cmake --build --preset linux-x64-Release

# macOS (Universal Binary)
cmake --preset macos-universal-Release
cmake --build --preset macos-universal-Release
```

### Usage

```bash
# Solve a puzzle from JSON
SudokuSolver -i puzzle.json

# Solve from image (OCR) - Windows/Linux/macOS only
SudokuSolver -i sudoku.png

# Use built-in test puzzles
SudokuSolver -t 9                    # 9x9 puzzle
SudokuSolver -t 16                   # 16x16 puzzle  
SudokuSolver -t 25                   # 25x25 puzzle (heavy)

# Compare algorithms
SudokuSolver -i puzzle.json -a compare

# Run benchmark (1000 iterations, auto-detect threads)
SudokuSolver -t 9 -b 1000 -w 0

# Multi-threaded benchmark with built-in puzzle
SudokuSolver -t 16 -b 1000 -w 0 -a compare

# Show system info
SudokuSolver -s
```

### Command Line Options

| Option | Description |
|--------|-------------|
| `-i, --input <file>` | Input file (JSON or image) |
| `-t, --test <size>` | Use built-in test puzzle (9, 16, or 25) |
| `-a, --algorithm <algo>` | Algorithm: `dlx`, `backtrack`, `compare` |
| `-b, --benchmark <N>` | Run benchmark with N iterations |
| `-w, --workers <N>` | Number of parallel workers (0 = auto) |
| `-o, --output <file>` | Output solution to JSON file |
| `-s, --sysinfo` | Show system information |
| `-q, --quiet` | Minimal output |

---

## 📊 Benchmark Results

### Performance Metrics Explained

```
                    Throughput (T)
Efficiency (E) = ─────────────────────
                  Single-Thread x Workers

Where:
  - Throughput = Total solves per second (multi-threaded)
  - Speedup = Multi-threaded throughput / Single-threaded throughput  
  - Efficiency = Speedup / Number of workers x 100%
```

**Efficiency Interpretation:**

| Efficiency | Rating | Description |
|------------|--------|-------------|
| 90-100% | ⭐⭐⭐⭐⭐ | Near-perfect scaling |
| 70-90% | ⭐⭐⭐⭐ | Excellent |
| 50-70% | ⭐⭐⭐ | Good (typical for CPU-bound tasks) |
| 30-50% | ⭐⭐ | Moderate (synchronization overhead) |
| <30% | ⭐ | Poor (bottlenecked) |

---

### 🖥️ Cross-Platform Benchmark (GitHub CI Runners)

**Test Configuration:** ./SudokuSolver -b 100 -w 0 -a compare -t 25

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    GitHub Actions Runner Performance                        │
├────────────┬──────────────────┬─────────┬───────────┬───────────┬───────────┤
│  Platform  │       CPU        │  Cores  │   BT/s    │  DLX/s    │ Winner    │
├────────────┼──────────────────┼─────────┼───────────┼───────────┼───────────┤
│  Windows   │ AMD EPYC 7763    │  2C/4T  │  44.86/s  │  58.57/s  │ DLX +31%  │
│  Linux     │ AMD EPYC 7763    │  2C/4T  │  38.94/s  │  67.49/s  │ DLX +73%  │
│  macOS     │ Apple M1 (VM)    │  3C/3T  │  35.56/s  │  94.92/s  │ DLX+167%  │ ⭐
└────────────┴──────────────────┴─────────┴───────────┴───────────┴───────────┘

BT = Backtracking, DLX = Dancing Links
```

#### Platform Performance Analysis

```
Throughput Comparison (GitHub Runners)
══════════════════════════════════════════════════════════════════

  DLX Algorithm:
  macOS (M1)   │████████████████████████████████████████████████████│ 94.92/s ⭐
  Linux        │███████████████████████████████████                 │ 67.49/s
  Windows      │██████████████████████████████                      │ 58.57/s

  Backtracking Algorithm:
  Windows      │███████████████████████                             │ 44.86/s
  Linux        │████████████████████                                │ 38.94/s
  macOS (M1)   │██████████████████                                  │ 35.56/s

               └────────────────────────────────────────────────────┘
                0        20        40        60        80       100
```

---

### 🏠 Local Machine Benchmark

**Test Configuration:** ./SudokuSolver -b 100 -w 0 -a compare -t 25

#### High-End Desktop: AMD Ryzen 9 9950X3D

```
┌─────────────────────────────────────────────────────────────────────────────┐
│           AMD Ryzen 9 9950X3D (16C/32T) - 192GB DDR5                        │
├─────────────┬─────────────┬───────────────┬──────────┬──────────────────────┤
│  Platform   │  Compiler   │  Throughput   │ Speedup  │     Efficiency       │
├─────────────┼─────────────┼───────────────┼──────────┼──────────────────────┤
│  Windows    │  MSVC 1944  │               │          │                      │
│  ├─ BT      │             │    805.40/s   │  15.02x  │  46.93%              │
│  └─ DLX     │             │    885.59/s   │  17.68x  │  55.25%              │
├─────────────┼─────────────┼───────────────┼──────────┼──────────────────────┤
│  WSL2       │  GCC 11.4   │               │          │                      │
│  ├─ BT      │             │    561.83/s   │  15.87x  │  49.60%              │
│  └─ DLX     │             │    903.59/s   │  15.97x  │  49.92%              │ ⭐
└─────────────┴─────────────┴───────────────┴──────────┴──────────────────────┘

BT = Backtracking, DLX = Dancing Links
```

#### 🔬 WSL2 Performance Analysis

![Task Manager CPU View](taskview.png)

**Key Observation:** WSL2 DLX (903.59/s) outperforms native Windows (885.59/s) by ~2%!

```
┌─────────────────────────────────────────────────────────────────┐
│                    WSL2 vs Windows Analysis                     │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Task Manager Color Legend:                                     │
│  ┌────────┐                                                     │
│  │ Green  │ = Kernel Mode (WSL2 workload)                       │
│  │ Red    │ = User Mode (Windows native)                        │
│  └────────┘                                                     │
│                                                                 │
│  Why WSL2 shows as Kernel Mode?                                 │
│  ├─ WSL2 runs a real Linux kernel inside Hyper-V                │
│  ├─ From Windows perspective, VM operations = kernel mode       │
│  └─ The Linux userspace code runs inside the VM                 │
│                                                                 │
│  Performance Implications:                                      │
│  ┌─────────────────┬─────────────────┐                          │
│  │    Algorithm    │ WSL2 vs Windows │                          │
│  ├─────────────────┼─────────────────┤                          │
│  │  Backtracking   │  -30% (slower)  │  MSVC recursion wins     │
│  │  DLX            │  +2%  (faster)  │  GCC pointers win        │
│  └─────────────────┴─────────────────┘                          │
│                                                                 │
│  Conclusion:                                                    │
│  - WSL2 virtualization overhead is negligible for CPU-bound     │
│  - Compiler optimization matters more than OS overhead          │
│  - DLX benefits from GCC's superior pointer optimization        │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

#### Mobile SoC: Qualcomm Snapdragon 8 Gen 3

```
./SudokuSolver -b 100 -w 0 -a compare -t 25
┌─────────────────────────────────────────────────────────────────────────────┐
│     Snapdragon 8 Gen 3 (Cortex-X4 + A720 + A520) - 12GB LPDDR5              │
├─────────────┬─────────────┬───────────────┬──────────┬──────────────────────┤
│  Platform   │  Compiler   │  Throughput   │ Speedup  │     Efficiency       │
├─────────────┼─────────────┼───────────────┼──────────┼──────────────────────┤
│  Android    │ Clang 21.0  │               │          │                      │
│  ├─ BT      │             │     95.83/s   │   3.74x  │  46.77%              │
│  └─ DLX     │             │    105.31/s   │   2.74x  │  34.23%              │
└─────────────┴─────────────┴───────────────┴──────────┴──────────────────────┘
```

---

### 📈 Algorithm Performance Analysis

#### Compiler Impact on Algorithm Performance

```
                     Backtracking vs DLX by Compiler
    ═══════════════════════════════════════════════════════════

    MSVC (Windows)
    ├─ Backtracking │██████████████████████████████████████████│ 805/s
    └─ DLX          │██████████████████████████████████████████████│ 886/s
                    Δ = +10% (DLX wins)

    GCC (Linux/WSL2)  
    ├─ Backtracking │██████████████████████████████│ 562/s
    └─ DLX          │██████████████████████████████████████████████████│ 904/s
                    Δ = +61% (DLX dominates)

    ═══════════════════════════════════════════════════════════
```

**Key Findings:**

| Compiler | Backtracking | DLX | Winner | Analysis |
|----------|-------------|-----|--------|----------|
| **MSVC** | 805/s | 886/s | DLX +10% | MSVC optimizes recursion well |
| **GCC** | 562/s | 904/s | DLX +61% | GCC excels at pointer/linked-list ops |
| **Clang** | 96/s | 105/s | DLX +10% | Similar to MSVC pattern |

#### Why the Difference?

```
┌─────────────────────────────────────────────────────────────────┐
│                 Algorithm Characteristics                       │
├─────────────────────────────┬───────────────────────────────────┤
│       Backtracking          │           DLX                     │
├─────────────────────────────┼───────────────────────────────────┤
│ - Deep recursion            │ - Linked-list traversal           │
│ - Stack-heavy operations    │ - Pointer manipulation            │
│ - Predictable branches      │ - Cache-unfriendly access         │
│ - Cache-friendly (array)    │ - Complex data structures         │
├─────────────────────────────┼───────────────────────────────────┤
│ MSVC: Excellent tail-call   │ GCC: Superior pointer aliasing    │
│ optimization & stack mgmt   │ analysis & link-time optimization │
└─────────────────────────────┴───────────────────────────────────┘
```

---

### 🍎 Apple Silicon Deep Dive

#### Why 98.75% Efficiency on M1?

```
┌─────────────────────────────────────────────────────────────────┐
│              Apple Silicon Architecture Advantages              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  1. Unified Memory Architecture (UMA)                           │
│     ┌─────────┐    ┌─────────┐    ┌─────────┐                   │
│     │ P-Core  │    │ E-Core  │    │   GPU   │                   │
│     └────┬────┘    └────┬────┘    └────┬────┘                   │
│          │              │              │                        │
│          └──────────────┼──────────────┘                        │
│                         │                                       │
│                  ┌──────┴──────┐                                │
│                  │ Unified RAM │  << Zero-copy, no NUMA penalty │
│                  └─────────────┘                                │
│                                                                 │
│  2. Low Core Count = Simple Scheduling                          │
│     - 3 cores = minimal context switch overhead                 │
│     - No cross-CCD latency (unlike AMD Ryzen)                   │
│     - macOS scheduler highly optimized for Apple Silicon        │
│                                                                 │
│  3. High IPC (Instructions Per Clock)                           │
│     - M1 IPC rivals desktop CPUs despite lower clock            │
│     - Wide execution units (8-wide decode)                      │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

#### UMA: Double-Edged Sword

```
                    ┌─────────────────────────────────┐
                    │   Unified Memory Architecture   │
                    ├────────────────┬────────────────┤
                    │   Advantages   │ Disadvantages  │
                    ├────────────────┼────────────────┤
                    │ + No NUMA      │ - Shared       │
                    │   penalties    │   bandwidth    │
                    │                │                │
                    │ + Zero-copy    │ - CPU & GPU    │
                    │   GPU <-> CPU  │   compete      │
                    │                │                │
                    │ + Simplified   │ - Limited by   │
                    │   programming  │   single pool  │
                    │                │                │
                    │ + Lower power  │ - Can't add    │
                    │   consumption  │   more RAM     │
                    └────────────────┴────────────────┘
```

---

### 🔬 16x16 and 25x25 Puzzle Benchmarks

**Test Configuration:** AMD Ryzen 9 9950X3D (32T)

```
Puzzle Complexity Comparison
════════════════════════════════════════════════════════════════

 Size   │ Cells │ Empty │ Search Space │ Recommended Runs │
────────┼───────┼───────┼──────────────┼──────────────────┤
  9x9   │   81  │  ~51  │    ~10²⁰     │    5000-10000    │
 16x16  │  256  │ ~220  │    ~10⁸⁰     │    1000-5000     │
 25x25  │  625  │ ~500  │   ~10^200+   │     50-200       │

════════════════════════════════════════════════════════════════
```

#### 16x16 Performance (320,000 total solves)

| Algorithm | System | Throughput | Speedup | Efficiency |
|-----------|--------|------------|---------|------------|
| DLX | 9950X3D (32T) | **23,075 /s** | 19.87x | 62.10% |
| DLX | 5800X (16T) | 7,558 /s | 9.16x | 57.27% |
| Backtracking | 9950X3D (32T) | 9,393 /s | 15.83x | 49.48% |
| Backtracking | 5800X (16T) | 2,731 /s | 7.03x | 43.96% |

#### 25x25 Performance (50 runs x 32 workers)

```
25x25 Mega Puzzle - Extreme Benchmark
─────────────────────────────────────
Algorithm       │ Throughput │ Speedup │ Efficiency
────────────────┼────────────┼─────────┼───────────
Backtracking    │   794/s    │ 14.95x  │  46.71%
DLX             │   877/s    │ 17.26x  │  53.95%     ⭐
─────────────────────────────────────
Note: DLX is 10% faster on 25x25 puzzles
```

---

### 📋 Summary: Algorithm Selection Guide

```
┌─────────────────────────────────────────────────────────────────┐
│                  When to Use Each Algorithm                     │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Use DLX when:                                                  │
│  ├─ Puzzle size ≥ 16x16                                         │
│  ├─ Multi-threaded execution                                    │
│  ├─ Finding ALL solutions                                       │
│  ├─ Running on Linux/GCC                                        │
│  └─ Consistency across platforms is important                   │
│                                                                 │
│  Use Backtracking when:                                         │
│  ├─ Puzzle size = 9x9                                           │
│  ├─ Single-threaded execution                                   │
│  ├─ Only need ONE solution                                      │
│  ├─ Running on Windows/MSVC                                     │
│  └─ Memory efficiency is critical                               │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 🌐 Platform Support Matrix

| Platform | Solver | Benchmark | OCR | System Detection |
|----------|:------:|:---------:|:---:|:----------------:|
| Windows x64 | ✅ | ✅ | ✅ | ✅ Full |
| Linux x64 | ✅ | ✅ | ✅ | ✅ Full |
| macOS (Intel) | ✅ | ✅ | ✅ | ✅ Full |
| macOS (Apple Silicon) | ✅ | ✅ | ✅ | ✅ Full (ref clock) |
| Android ARM64 | ✅ | ✅ | ❌ | ✅ Full (SoC detection) |

### Platform-Specific Notes

- **Windows**: Best Backtracking performance due to MSVC optimizations
- **Linux**: Best DLX performance due to GCC pointer optimizations  
- **macOS**: Excellent efficiency due to UMA and optimized scheduler
- **Android**: Full SoC detection (Snapdragon, Dimensity, Exynos, Tensor)

---

## 📁 JSON Format

### Array Format
```json
{
  "puzzle": [
    [5, 3, 0, 0, 7, 0, 0, 0, 0],
    [6, 0, 0, 1, 9, 5, 0, 0, 0],
    ...
  ]
}
```

### String Format
```json
{
  "puzzle": "530070000600195000098000060800060003400803001700020006060000280000419005000080079"
}
```

### With Custom Dimensions
```json
{
  "size": 16,
  "box_rows": 4,
  "box_cols": 4,
  "puzzle": [...]
}
```

---

## 🏗️ Project Structure

```
sudoku-solver/
├── include/
│   ├── board.hpp            # Board representation
│   ├── solver.hpp           # Solver interface
│   ├── solver_dlx.hpp       # DLX implementation
│   ├── solver_backtrack.hpp # Backtracking implementation
│   ├── ocr_processor.hpp    # OCR processing (conditional)
│   ├── benchmark.hpp        # Benchmarking utilities
│   ├── system_info.hpp      # Cross-platform system detection
│   ├── json_handler.hpp     # JSON I/O
│   └── types.hpp            # Common types
├── src/
│   ├── main.cpp             # Entry point + built-in puzzles
│   ├── board.cpp
│   ├── solver_dlx.cpp
│   ├── solver_backtrack.cpp
│   ├── ocr_processor.cpp    # Conditionally compiled
│   ├── benchmark.cpp
│   ├── system_info.cpp      # Platform-specific detection
│   └── json_handler.cpp
├── .github/
│   └── workflows/           # CI/CD pipelines
├── CMakeLists.txt
├── CMakePresets.json        # Cross-platform build presets
├── CMakeSettings.json       # Visual Studio settings
├── vcpkg.json               # Dependencies
└── README.md
```

---

## 📦 Dependencies

Managed via vcpkg:

| Package | Purpose | Platforms |
|---------|---------|-----------|
| **OpenCV** | Image processing | All |
| **Tesseract** | OCR engine | Windows/Linux/macOS |
| **nlohmann/json** | JSON parsing | All |
| **CLI11** | Command line parsing | All |

---

## 🔧 Building

### Using CMake Presets (Recommended)

```bash
# List available presets
cmake --list-presets

# Configure and build
cmake --preset <preset-name>
cmake --build --preset <preset-name>
```

### Manual Build

```bash
cmake -B build -S . \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build --config Release
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `ENABLE_OCR` | ON | Enable Tesseract OCR support |
| `ENABLE_OPENMP` | OFF | Enable OpenMP parallelization |

---

## 📜 License

MIT License

## 👥 Authors

- **AllenK**
- **Kwyshell**

## 🙏 Acknowledgments

- Donald Knuth for the Dancing Links algorithm
- The Tesseract OCR project
- OpenCV community
- vcpkg maintainers

## Related

- [Sudoku Solver — From a Father-Son Coding Challenge to a CPU Benchmark Tool](https://allenkuo.medium.com/sudoku-solver-from-a-father-son-coding-challenge-to-a-cpu-benchmark-tool-38300c968bf5)
- [Sudoku Solver — Cross-Platform Performance Deep Dive (Part II)](https://allenkuo.medium.com/sudoku-solver-cross-platform-performance-deep-dive-part-ii-7dd1fc60e8b1)

