#pragma once

#include <string>
#include <string_view>

// What the machine has for running models. The wrapper reads the texts (nvidia-smi output,
// sysfs, /proc/meminfo) and these functions turn them into numbers.
struct Hardware {
    std::string gpuName;              // empty: no usable GPU found, models run on the CPU
    long long vramBytes = 0;
    long long ramTotalBytes = 0;
    long long ramAvailableBytes = 0;
};

namespace hardwareprobe {

// `nvidia-smi --query-gpu=name,memory.total --format=csv,noheader,nounits`: one "name, MiB" line
// per GPU. The GPU with the most memory is used; false when there is no such line.
bool parseNvidiaSmi(std::string_view text, Hardware& hw);
// /sys/class/drm/cardN/device/mem_info_vram_total of an AMD GPU: bytes as a decimal number.
bool parseAmdVram(std::string_view text, Hardware& hw);
// /proc/meminfo: MemTotal and MemAvailable.
bool parseMeminfo(std::string_view text, Hardware& hw);

// "RTX 3060 Ti · 8 GB VRAM · 31 GB RAM", or "No GPU found · 31 GB RAM".
std::string summary(const Hardware& hw);

} // namespace hardwareprobe
