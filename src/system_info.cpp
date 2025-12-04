#include "system_info.hpp"
#include <iostream>
#include <fstream>
#include <regex>
#include <cstring>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <intrin.h>
#include <powerbase.h>
#pragma comment(lib, "PowrProf.lib")
// NTSTATUS is not always defined
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((LONG)0x00000000L)
#endif
#else
#include <unistd.h>
#include <sys/utsname.h>
#endif

namespace sudoku {

// Helper method implementations
std::string SystemInfo::totalRamFormatted() const {
    return SystemInfoDetector::formatBytes(total_ram_bytes);
}

std::string SystemInfo::availableRamFormatted() const {
    return SystemInfoDetector::formatBytes(available_ram_bytes);
}

std::string SystemInfo::cpuClockFormatted() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(0);
    if (base_clock_mhz > 0) {
        oss << base_clock_mhz << " MHz";
        if (max_clock_mhz > base_clock_mhz) {
            oss << " (Boost: " << max_clock_mhz << " MHz)";
        }
    } else {
        oss << "Unknown";
    }
    return oss.str();
}

SystemInfo SystemInfoDetector::detect() {
    SystemInfo info;

    // CPU
    info.cpu_model = detectCpuModel();
    info.physical_cores = detectPhysicalCores();
    info.logical_cores = detectLogicalCores();
    info.base_clock_mhz = detectBaseClock();
    info.max_clock_mhz = detectMaxClock();
    info.cpu_architecture = detectCpuArchitecture();

    // Memory
    info.total_ram_bytes = detectTotalRam();
    info.available_ram_bytes = detectAvailableRam();

    // OS
    info.os_name = detectOsName();
    info.os_version = detectOsVersion();
    info.os_architecture = detectOsArchitecture();

    // Compiler
    info.compiler_info = detectCompilerInfo();
    info.build_type = detectBuildType();

    return info;
}

std::string SystemInfoDetector::generateReport(const SystemInfo& info) {
    std::ostringstream oss;

    oss << "+-------------------------------------------------------------+\n";
    oss << "|                    System Information                       |\n";
    oss << "+-------------------------------------------------------------+\n";

    // CPU
    oss << "| CPU: " << std::left << std::setw(55) << info.cpu_model << "|\n";

    std::ostringstream coresStr;
    coresStr << info.physical_cores << " cores / " << info.logical_cores << " threads";
    oss << "| Cores: " << std::left << std::setw(53) << coresStr.str() << "|\n";

    oss << "| Clock: " << std::left << std::setw(53) << info.cpuClockFormatted() << "|\n";
    oss << "| Arch: " << std::left << std::setw(54) << info.cpu_architecture << "|\n";

    oss << "+-------------------------------------------------------------+\n";

    // Memory
    std::ostringstream ramStr;
    ramStr << info.totalRamFormatted() << " (Available: " << info.availableRamFormatted() << ")";
    oss << "| RAM: " << std::left << std::setw(55) << ramStr.str() << "|\n";

    oss << "+-------------------------------------------------------------+\n";

    // OS
    std::ostringstream osStr;
    osStr << info.os_name;
    if (!info.os_version.empty()) {
        osStr << " " << info.os_version;
    }
    oss << "| OS: " << std::left << std::setw(56) << osStr.str() << "|\n";
    oss << "| Platform: " << std::left << std::setw(50) << info.os_architecture << "|\n";

    oss << "+-------------------------------------------------------------+\n";

    // Compiler
    oss << "| Compiler: " << std::left << std::setw(50) << info.compiler_info << "|\n";
    oss << "| Build: " << std::left << std::setw(53) << info.build_type << "|\n";

    oss << "+-------------------------------------------------------------+\n";

    return oss.str();
}

void SystemInfoDetector::print(const SystemInfo& info) {
    std::cout << generateReport(info);
}

std::string SystemInfoDetector::getCompactSummary(const SystemInfo& info) {
    std::ostringstream oss;
    oss << info.cpu_model << " | "
        << info.logical_cores << "T | "
        << info.totalRamFormatted() << " | "
        << info.os_name;
    return oss.str();
}

std::string SystemInfoDetector::formatBytes(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024 && unit < 4) {
        size /= 1024;
        unit++;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(unit > 0 ? 1 : 0) << size << " " << units[unit];
    return oss.str();
}

std::string SystemInfoDetector::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

// ============================================================================
// Platform-specific implementations
// ============================================================================

#ifdef _WIN32
// Windows implementations

std::string SystemInfoDetector::detectCpuModel() {
    int cpuInfo[4] = {0};
    char brand[49] = {0};

    // Get extended CPUID info
    __cpuid(cpuInfo, 0x80000000);
    unsigned int maxExtId = cpuInfo[0];

    if (maxExtId >= 0x80000004) {
        __cpuid(cpuInfo, 0x80000002);
        memcpy(brand, cpuInfo, sizeof(cpuInfo));

        __cpuid(cpuInfo, 0x80000003);
        memcpy(brand + 16, cpuInfo, sizeof(cpuInfo));

        __cpuid(cpuInfo, 0x80000004);
        memcpy(brand + 32, cpuInfo, sizeof(cpuInfo));
    }

    std::string result = trim(brand);
    return result.empty() ? "Unknown CPU" : result;
}

int SystemInfoDetector::detectPhysicalCores() {
    DWORD length = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &length);

    std::vector<char> buffer(length);
    auto info = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data());

    if (!GetLogicalProcessorInformationEx(RelationProcessorCore, info, &length)) {
        // Fallback
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        return sysInfo.dwNumberOfProcessors / 2;
    }

    int cores = 0;
    DWORD offset = 0;
    while (offset < length) {
        auto current = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(
            buffer.data() + offset);
        if (current->Relationship == RelationProcessorCore) {
            cores++;
        }
        offset += current->Size;
    }

    return cores > 0 ? cores : 1;
}

int SystemInfoDetector::detectLogicalCores() {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return static_cast<int>(sysInfo.dwNumberOfProcessors);
}

double SystemInfoDetector::detectBaseClock() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                      "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD mhz = 0;
        DWORD size = sizeof(mhz);
        if (RegQueryValueExA(hKey, "~MHz", nullptr, nullptr,
                            reinterpret_cast<LPBYTE>(&mhz), &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return static_cast<double>(mhz);
        }
        RegCloseKey(hKey);
    }
    return 0.0;
}

double SystemInfoDetector::detectMaxClock() {
    // Try to get from power information
    typedef struct _PROCESSOR_POWER_INFORMATION {
        ULONG Number;
        ULONG MaxMhz;
        ULONG CurrentMhz;
        ULONG MhzLimit;
        ULONG MaxIdleState;
        ULONG CurrentIdleState;
    } PROCESSOR_POWER_INFORMATION;

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    std::vector<PROCESSOR_POWER_INFORMATION> powerInfo(sysInfo.dwNumberOfProcessors);

    LONG status = CallNtPowerInformation(
        ProcessorInformation,
        nullptr, 0,
        powerInfo.data(),
        static_cast<ULONG>(powerInfo.size() * sizeof(PROCESSOR_POWER_INFORMATION))
    );

    if (status == STATUS_SUCCESS && !powerInfo.empty()) {
        return static_cast<double>(powerInfo[0].MaxMhz);
    }

    return detectBaseClock();
}

std::string SystemInfoDetector::detectCpuArchitecture() {
    SYSTEM_INFO sysInfo;
    GetNativeSystemInfo(&sysInfo);

    switch (sysInfo.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64: return "x64 (AMD64)";
        case PROCESSOR_ARCHITECTURE_INTEL: return "x86";
        case PROCESSOR_ARCHITECTURE_ARM: return "ARM";
        case PROCESSOR_ARCHITECTURE_ARM64: return "ARM64";
        default: return "Unknown";
    }
}

uint64_t SystemInfoDetector::detectTotalRam() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(memInfo);
    if (GlobalMemoryStatusEx(&memInfo)) {
        return memInfo.ullTotalPhys;
    }
    return 0;
}

uint64_t SystemInfoDetector::detectAvailableRam() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(memInfo);
    if (GlobalMemoryStatusEx(&memInfo)) {
        return memInfo.ullAvailPhys;
    }
    return 0;
}

std::string SystemInfoDetector::detectOsName() {
    return "Windows";
}

std::string SystemInfoDetector::detectOsVersion() {
    // Use RtlGetVersion for accurate version
    typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

    HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
    if (hMod) {
        auto fxPtr = reinterpret_cast<RtlGetVersionPtr>(
            GetProcAddress(hMod, "RtlGetVersion"));
        if (fxPtr) {
            RTL_OSVERSIONINFOW rovi = {0};
            rovi.dwOSVersionInfoSize = sizeof(rovi);
            if (fxPtr(&rovi) == 0) {
                std::ostringstream oss;
                oss << rovi.dwMajorVersion << "."
                    << rovi.dwMinorVersion << "."
                    << rovi.dwBuildNumber;

                // Friendly name
                if (rovi.dwMajorVersion == 10 && rovi.dwBuildNumber >= 22000) {
                    oss << " (Windows 11)";
                } else if (rovi.dwMajorVersion == 10) {
                    oss << " (Windows 10)";
                } else if (rovi.dwMajorVersion == 6 && rovi.dwMinorVersion == 3) {
                    oss << " (Windows 8.1)";
                } else if (rovi.dwMajorVersion == 6 && rovi.dwMinorVersion == 2) {
                    oss << " (Windows 8)";
                } else if (rovi.dwMajorVersion == 6 && rovi.dwMinorVersion == 1) {
                    oss << " (Windows 7)";
                }

                return oss.str();
            }
        }
    }
    return "Unknown";
}

std::string SystemInfoDetector::detectOsArchitecture() {
    SYSTEM_INFO sysInfo;
    GetNativeSystemInfo(&sysInfo);

    return (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
            sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64)
           ? "64-bit" : "32-bit";
}

#else
// Linux/Unix implementations

std::string SystemInfoDetector::detectCpuModel() {
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;

    while (std::getline(cpuinfo, line)) {
        if (line.find("model name") != std::string::npos) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                return trim(line.substr(pos + 1));
            }
        }
    }
    return "Unknown CPU";
}

int SystemInfoDetector::detectPhysicalCores() {
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    int cores = 0;

    while (std::getline(cpuinfo, line)) {
        if (line.find("cpu cores") != std::string::npos) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                cores = std::stoi(trim(line.substr(pos + 1)));
                break;
            }
        }
    }

    if (cores == 0) {
        cores = static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN)) / 2;
    }

    return std::max(1, cores);
}

int SystemInfoDetector::detectLogicalCores() {
    return static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
}

double SystemInfoDetector::detectBaseClock() {
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;

    while (std::getline(cpuinfo, line)) {
        if (line.find("cpu MHz") != std::string::npos) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                return std::stod(trim(line.substr(pos + 1)));
            }
        }
    }
    return 0.0;
}

double SystemInfoDetector::detectMaxClock() {
    // Try to read from scaling_max_freq
    std::ifstream maxFreq("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq");
    if (maxFreq.is_open()) {
        double khz;
        maxFreq >> khz;
        return khz / 1000.0;  // Convert to MHz
    }
    return detectBaseClock();
}

std::string SystemInfoDetector::detectCpuArchitecture() {
    struct utsname info;
    if (uname(&info) == 0) {
        return info.machine;
    }
    return "Unknown";
}

uint64_t SystemInfoDetector::detectTotalRam() {
    std::ifstream meminfo("/proc/meminfo");
    std::string line;

    while (std::getline(meminfo, line)) {
        if (line.find("MemTotal:") != std::string::npos) {
            std::regex re("\\d+");
            std::smatch match;
            if (std::regex_search(line, match, re)) {
                return std::stoull(match.str()) * 1024;  // KB to bytes
            }
        }
    }
    return 0;
}

uint64_t SystemInfoDetector::detectAvailableRam() {
    std::ifstream meminfo("/proc/meminfo");
    std::string line;

    while (std::getline(meminfo, line)) {
        if (line.find("MemAvailable:") != std::string::npos) {
            std::regex re("\\d+");
            std::smatch match;
            if (std::regex_search(line, match, re)) {
                return std::stoull(match.str()) * 1024;  // KB to bytes
            }
        }
    }
    return 0;
}

std::string SystemInfoDetector::detectOsName() {
    // Try to read from os-release
    std::ifstream osRelease("/etc/os-release");
    std::string line;

    while (std::getline(osRelease, line)) {
        if (line.find("PRETTY_NAME=") == 0) {
            std::string name = line.substr(12);
            // Remove quotes
            if (name.front() == '"') name = name.substr(1);
            if (name.back() == '"') name.pop_back();
            return name;
        }
    }

    struct utsname info;
    if (uname(&info) == 0) {
        return info.sysname;
    }
    return "Linux";
}

std::string SystemInfoDetector::detectOsVersion() {
    struct utsname info;
    if (uname(&info) == 0) {
        return info.release;
    }
    return "Unknown";
}

std::string SystemInfoDetector::detectOsArchitecture() {
    struct utsname info;
    if (uname(&info) == 0) {
        std::string machine = info.machine;
        if (machine == "x86_64" || machine == "aarch64") {
            return "64-bit";
        }
        return "32-bit";
    }
    return "Unknown";
}

#endif

// Common implementations

std::string SystemInfoDetector::detectCompilerInfo() {
    std::ostringstream oss;

#if defined(_MSC_VER)
    oss << "MSVC " << _MSC_VER;
    #if _MSC_VER >= 1930
        oss << " (VS 2022)";
    #elif _MSC_VER >= 1920
        oss << " (VS 2019)";
    #elif _MSC_VER >= 1910
        oss << " (VS 2017)";
    #endif
#elif defined(__clang__)
    oss << "Clang " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__;
#elif defined(__GNUC__)
    oss << "GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__;
#else
    oss << "Unknown Compiler";
#endif

    return oss.str();
}

std::string SystemInfoDetector::detectBuildType() {
#ifdef NDEBUG
    return "Release";
#else
    return "Debug";
#endif
}

} // namespace sudoku
