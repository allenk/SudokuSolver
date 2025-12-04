#pragma once

#include <string>
#include <vector>
#include <thread>
#include <sstream>
#include <iomanip>

namespace sudoku {

/**
 * @brief System hardware and OS information for benchmark reports
 */
struct SystemInfo {
    // CPU
    std::string cpu_model;
    int physical_cores = 0;
    int logical_cores = 0;
    double base_clock_mhz = 0.0;
    double max_clock_mhz = 0.0;
    std::string cpu_architecture;

    // Memory
    uint64_t total_ram_bytes = 0;
    uint64_t available_ram_bytes = 0;

    // OS
    std::string os_name;
    std::string os_version;
    std::string os_architecture;

    // Compiler
    std::string compiler_info;
    std::string build_type;

    // Helper methods
    std::string totalRamFormatted() const;
    std::string availableRamFormatted() const;
    std::string cpuClockFormatted() const;
};

/**
 * @brief Utility class to detect system information
 */
class SystemInfoDetector {
public:
    /**
     * @brief Detect all system information
     * @return SystemInfo structure with detected values
     */
    static SystemInfo detect();

    /**
     * @brief Generate formatted report string
     * @param info System information
     * @return Formatted multi-line string
     */
    static std::string generateReport(const SystemInfo& info);

    /**
     * @brief Print system info to console
     * @param info System information
     */
    static void print(const SystemInfo& info);

    /**
     * @brief Get compact one-line summary
     * @param info System information
     * @return Single line summary
     */
    static std::string getCompactSummary(const SystemInfo& info);

    /**
     * @brief Format bytes to human readable string (KB, MB, GB, etc.)
     */
    static std::string formatBytes(uint64_t bytes);

private:
    static std::string detectCpuModel();
    static int detectPhysicalCores();
    static int detectLogicalCores();
    static double detectBaseClock();
    static double detectMaxClock();
    static std::string detectCpuArchitecture();

    static uint64_t detectTotalRam();
    static uint64_t detectAvailableRam();

    static std::string detectOsName();
    static std::string detectOsVersion();
    static std::string detectOsArchitecture();

    static std::string detectCompilerInfo();
    static std::string detectBuildType();

    static std::string trim(const std::string& str);
};

} // namespace sudoku
