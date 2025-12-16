#include "system_info.hpp"
#include <iostream>
#include <fstream>
#include <regex>
#include <cstring>
#include <set>
#include <algorithm>

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

#ifdef __ANDROID__
#include <sys/system_properties.h>
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
// Linux/Unix/Android implementations

// ============================================================================
// SoC Model Name Lookup (for Android)
// ============================================================================
#ifdef __ANDROID__
static std::string getSocName(const std::string& model) {
    // Qualcomm Snapdragon
    if (model == "SM8750") return "Snapdragon 8 Elite";
    if (model == "SM8650") return "Snapdragon 8 Gen 3";
    if (model == "SM8550") return "Snapdragon 8 Gen 2";
    if (model == "SM8475") return "Snapdragon 8+ Gen 1";
    if (model == "SM8450") return "Snapdragon 8 Gen 1";
    if (model == "SM8350") return "Snapdragon 888";
    if (model == "SM8250") return "Snapdragon 865";
    if (model == "SM8150") return "Snapdragon 855";
    if (model == "SM7675") return "Snapdragon 7+ Gen 3";
    if (model == "SM7550") return "Snapdragon 7+ Gen 2";
    if (model == "SM7450") return "Snapdragon 7 Gen 1";
    if (model == "SM6450") return "Snapdragon 6 Gen 1";
    if (model == "SM4450") return "Snapdragon 4 Gen 2";
    // Qualcomm X Elite (Oryon)
    if (model == "SC8380XP") return "Snapdragon X Elite";
    if (model == "SC8280XP") return "Snapdragon 8cx Gen 3";

    // MediaTek Dimensity
    if (model == "MT6991") return "Dimensity 9400";
    if (model == "MT6989") return "Dimensity 9300";
    if (model == "MT6985") return "Dimensity 9200";
    if (model == "MT6983") return "Dimensity 9000";
    if (model == "MT6895") return "Dimensity 8100";
    if (model == "MT6893") return "Dimensity 1200";
    if (model == "MT6891") return "Dimensity 1100";
    if (model == "MT6877") return "Dimensity 900";

    // Samsung Exynos
    if (model == "s5e9945") return "Exynos 2400";
    if (model == "s5e9925") return "Exynos 2200";
    if (model == "s5e9920") return "Exynos 2100";
    if (model == "s5e9830") return "Exynos 990";

    // Google Tensor
    if (model == "gs201") return "Google Tensor G2";
    if (model == "gs101") return "Google Tensor";
    if (model == "zuma") return "Google Tensor G3";

    return model;  // Return original if unknown
}
#endif

// ============================================================================
// ARM CPU Part ID Lookup
// ============================================================================
static std::string getArmCpuName(uint32_t implementer, uint32_t part) {
    // ARM Ltd (0x41)
    if (implementer == 0x41) {
        switch (part) {
            // Cortex-A5x series
            case 0xd03: return "Cortex-A53";
            case 0xd04: return "Cortex-A35";
            case 0xd05: return "Cortex-A55";
            case 0xd07: return "Cortex-A57";
            case 0xd08: return "Cortex-A72";
            case 0xd09: return "Cortex-A73";
            case 0xd0a: return "Cortex-A75";
            case 0xd0b: return "Cortex-A76";
            case 0xd0d: return "Cortex-A77";
            case 0xd41: return "Cortex-A78";
            case 0xd44: return "Cortex-X1";
            case 0xd4b: return "Cortex-A78C";
            // Cortex-A5xx series (ARMv9)
            case 0xd46: return "Cortex-A510";
            case 0xd47: return "Cortex-A710";
            case 0xd48: return "Cortex-X2";
            case 0xd4d: return "Cortex-A715";
            case 0xd4e: return "Cortex-X3";
            // Snapdragon 8 Gen 3 / Dimensity 9300 era
            case 0xd80: return "Cortex-X4";
            case 0xd81: return "Cortex-A720";
            case 0xd82: return "Cortex-A520";
            // Snapdragon 8 Elite / Dimensity 9400 era
            case 0xd83: return "Cortex-X5";
            case 0xd84: return "Cortex-A725";
            case 0xd85: return "Cortex-A520 (v2)";
            // Neoverse (Server)
            case 0xd0c: return "Neoverse-N1";
            case 0xd40: return "Neoverse-V1";
            case 0xd49: return "Neoverse-N2";
            case 0xd4f: return "Neoverse-V2";
            default: break;
        }
    }
    // Qualcomm (0x51)
    else if (implementer == 0x51) {
        switch (part) {
            case 0x001: return "Oryon";
            case 0x800: return "Kryo 260";
            case 0x801: return "Kryo 280";
            case 0x802: return "Kryo 385";
            case 0x803: return "Kryo 485";
            case 0x804: return "Kryo 585";
            case 0x805: return "Kryo 680";
            default: break;
        }
    }
    // Samsung (0x53)
    else if (implementer == 0x53) {
        switch (part) {
            case 0x001: return "Exynos M1";
            case 0x002: return "Exynos M3";
            case 0x003: return "Exynos M4";
            case 0x004: return "Exynos M5";
            default: break;
        }
    }
    // Apple (0x61)
    else if (implementer == 0x61) {
        switch (part) {
            case 0x022: return "Icestorm (M1)";
            case 0x023: return "Firestorm (M1)";
            case 0x024: return "Blizzard (M2)";
            case 0x025: return "Avalanche (M2)";
            case 0x028: return "Everest (M3)";
            case 0x029: return "Sawtooth (M3)";
            default: break;
        }
    }

    return "";
}

// ============================================================================
// Get Android property helper
// ============================================================================
#ifdef __ANDROID__
static std::string getAndroidProp(const char* name) {
    char value[PROP_VALUE_MAX] = {0};
    if (__system_property_get(name, value) > 0) {
        return std::string(value);
    }
    return "";
}
#endif

std::string SystemInfoDetector::detectCpuModel() {
    std::string socName;

#ifdef __ANDROID__
    // Try Android system properties first
    std::string socModel = getAndroidProp("ro.soc.model");
    std::string chipname = getAndroidProp("ro.hardware.chipname");
    std::string platform = getAndroidProp("ro.board.platform");

    // Get SoC name from model
    if (!socModel.empty()) {
        socName = getSocName(socModel);
    } else if (!chipname.empty()) {
        socName = getSocName(chipname);
    } else if (!platform.empty()) {
        socName = getSocName(platform);
    }
#endif

    // Parse /proc/cpuinfo for ARM CPU parts
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;

    std::set<std::pair<uint32_t, uint32_t>> armCpus;
    std::string cpuHardware;
    uint32_t lastImplementer = 0;

    while (std::getline(cpuinfo, line)) {
        // Standard Linux x86: model name
        if (line.find("model name") != std::string::npos) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                return trim(line.substr(pos + 1));
            }
        }

        // ARM/Android: Hardware field
        if (line.find("Hardware") != std::string::npos) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                cpuHardware = trim(line.substr(pos + 1));
            }
        }

        // ARM: CPU implementer
        if (line.find("CPU implementer") != std::string::npos) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                try {
                    lastImplementer = static_cast<uint32_t>(
                        std::stoul(trim(line.substr(pos + 1)), nullptr, 0));
                } catch (...) {
                    lastImplementer = 0;
                }
            }
        }

        // ARM: CPU part
        if (line.find("CPU part") != std::string::npos) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                try {
                    uint32_t part = static_cast<uint32_t>(
                        std::stoul(trim(line.substr(pos + 1)), nullptr, 0));
                    if (lastImplementer != 0) {
                        armCpus.insert({lastImplementer, part});
                    }
                } catch (...) {
                    // Ignore parse errors
                }
            }
        }
    }

    // Build ARM CPU cores string
    std::string coresStr;
    if (!armCpus.empty()) {
        std::vector<std::string> coreNames;
        for (const auto& cpu : armCpus) {
            std::string name = getArmCpuName(cpu.first, cpu.second);
            if (!name.empty()) {
                bool found = false;
                for (const auto& existing : coreNames) {
                    if (existing == name) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    coreNames.push_back(name);
                }
            }
        }

        if (!coreNames.empty()) {
            std::ostringstream oss;
            for (size_t i = 0; i < coreNames.size(); ++i) {
                if (i > 0) oss << " + ";
                oss << coreNames[i];
            }
            coresStr = oss.str();
        }
    }

    // Combine SoC name with core info
    if (!socName.empty()) {
        if (!coresStr.empty()) {
            return socName + " (" + coresStr + ")";
        }
        return socName;
    }

    // Return cores string if we have it
    if (!coresStr.empty()) {
        return coresStr;
    }

    // Fallback to Hardware field
    if (!cpuHardware.empty()) {
        return cpuHardware;
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
                try {
                    cores = std::stoi(trim(line.substr(pos + 1)));
                    break;
                } catch (...) {
                    cores = 0;
                }
            }
        }
    }

    if (cores == 0) {
        int logical = static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
        #if defined(__aarch64__) || defined(__arm__)
            // ARM typically has no SMT
            cores = logical;
        #else
            // x86 typically has SMT
            cores = logical / 2;
        #endif
    }

    return std::max(1, cores);
}

int SystemInfoDetector::detectLogicalCores() {
    return static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
}

double SystemInfoDetector::detectBaseClock() {
    // Try cpufreq (works on Android and modern Linux)
    std::ifstream curFreq("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq");
    if (curFreq.is_open()) {
        double khz;
        if (curFreq >> khz) {
            return khz / 1000.0;
        }
    }

    std::ifstream scalingFreq("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
    if (scalingFreq.is_open()) {
        double khz;
        if (scalingFreq >> khz) {
            return khz / 1000.0;
        }
    }

    // Fallback: /proc/cpuinfo
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;

    while (std::getline(cpuinfo, line)) {
        if (line.find("cpu MHz") != std::string::npos) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                try {
                    return std::stod(trim(line.substr(pos + 1)));
                } catch (...) {
                    return 0.0;
                }
            }
        }
    }

    return 0.0;
}

double SystemInfoDetector::detectMaxClock() {
    // For big.LITTLE, find the highest frequency across all CPUs
    double maxMhz = 0.0;
    for (int i = 0; i < 16; ++i) {
        std::string path = "/sys/devices/system/cpu/cpu" + std::to_string(i) +
                           "/cpufreq/cpuinfo_max_freq";
        std::ifstream freq(path);
        if (freq.is_open()) {
            double khz;
            if (freq >> khz) {
                maxMhz = std::max(maxMhz, khz / 1000.0);
            }
        }
    }

    if (maxMhz > 0) return maxMhz;

    // Fallback to single CPU max
    std::ifstream maxFreq("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
    if (maxFreq.is_open()) {
        double khz;
        if (maxFreq >> khz) {
            return khz / 1000.0;
        }
    }
    return detectBaseClock();
}

std::string SystemInfoDetector::detectCpuArchitecture() {
    struct utsname info;
    if (uname(&info) == 0) {
        std::string machine = info.machine;
        if (machine == "aarch64") return "ARM64 (AArch64)";
        if (machine == "armv7l") return "ARM32 (ARMv7)";
        if (machine == "armv8l") return "ARM64 (ARMv8)";
        if (machine == "x86_64") return "x64 (AMD64)";
        if (machine == "i686" || machine == "i386") return "x86";
        return machine;
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
#ifdef __ANDROID__
    return "Android";
#else
    // Try to read from os-release
    std::ifstream osRelease("/etc/os-release");
    std::string line;

    while (std::getline(osRelease, line)) {
        if (line.find("PRETTY_NAME=") == 0) {
            std::string name = line.substr(12);
            if (!name.empty() && name.front() == '"') name = name.substr(1);
            if (!name.empty() && name.back() == '"') name.pop_back();
            return name;
        }
    }

    struct utsname info;
    if (uname(&info) == 0) {
        return info.sysname;
    }
    return "Linux";
#endif
}

std::string SystemInfoDetector::detectOsVersion() {
#ifdef __ANDROID__
    std::string version = getAndroidProp("ro.build.version.release");
    std::string sdk = getAndroidProp("ro.build.version.sdk");
    if (!version.empty()) {
        if (!sdk.empty()) {
            return version + " (API " + sdk + ")";
        }
        return version;
    }
#endif

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

// ============================================================================
// Common implementations
// ============================================================================

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
