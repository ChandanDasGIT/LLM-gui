#include "modelManager/hardwareprobe.h"

#include <cmath>
#include <cstdlib>

namespace hardwareprobe {

namespace {

std::string_view trim(std::string_view s)
{
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r')) s.remove_prefix(1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) s.remove_suffix(1);
    return s;
}

bool parseInteger(std::string_view s, long long& out)
{
    s = trim(s);
    if (s.empty()) return false;
    long long value = 0;
    for (const char c : s) {
        if (c < '0' || c > '9') return false;
        value = value * 10 + (c - '0');
    }
    out = value;
    return true;
}

template <typename Fn>
void forEachLine(std::string_view text, Fn fn)
{
    while (!text.empty()) {
        const auto end = text.find('\n');
        fn(text.substr(0, end));
        if (end == std::string_view::npos) break;
        text.remove_prefix(end + 1);
    }
}

std::string shortGpuName(std::string_view name)
{
    for (const std::string_view prefix : {"NVIDIA ", "GeForce ", "AMD ", "Radeon "})
        if (name.substr(0, prefix.size()) == prefix) name.remove_prefix(prefix.size());
    return std::string(name);
}

std::string gigabytes(long long bytes)
{
    return std::to_string((long long)std::llround(double(bytes) / (1024.0 * 1024.0 * 1024.0))) + " GB";
}

} // namespace

bool parseNvidiaSmi(std::string_view text, Hardware& hw)
{
    bool found = false;
    forEachLine(text, [&](std::string_view line) {
        const auto comma = line.rfind(',');
        if (comma == std::string_view::npos) return;
        long long mib = 0;
        if (!parseInteger(line.substr(comma + 1), mib) || mib <= 0) return;
        const long long bytes = mib * 1024 * 1024;
        if (bytes <= hw.vramBytes) return;
        hw.vramBytes = bytes;
        hw.gpuName = shortGpuName(trim(line.substr(0, comma)));
        found = true;
    });
    return found;
}

bool parseAmdVram(std::string_view text, Hardware& hw)
{
    long long bytes = 0;
    if (!parseInteger(text, bytes) || bytes <= 0) return false;
    if (bytes <= hw.vramBytes) return true;
    hw.vramBytes = bytes;
    hw.gpuName = "AMD GPU";
    return true;
}

bool parseMeminfo(std::string_view text, Hardware& hw)
{
    bool total = false;
    bool available = false;
    forEachLine(text, [&](std::string_view line) {
        const auto colon = line.find(':');
        if (colon == std::string_view::npos) return;
        const auto key = line.substr(0, colon);
        auto value = trim(line.substr(colon + 1));
        if (value.size() > 3 && value.substr(value.size() - 3) == " kB") value.remove_suffix(3);
        long long kb = 0;
        if (!parseInteger(value, kb)) return;
        if (key == "MemTotal") { hw.ramTotalBytes = kb * 1024; total = true; }
        if (key == "MemAvailable") { hw.ramAvailableBytes = kb * 1024; available = true; }
    });
    return total && available;
}

std::string summary(const Hardware& hw)
{
    const std::string ram = gigabytes(hw.ramTotalBytes) + " RAM";
    if (hw.gpuName.empty() || hw.vramBytes <= 0) return "No GPU found · " + ram;
    return hw.gpuName + " · " + gigabytes(hw.vramBytes) + " VRAM · " + ram;
}

} // namespace hardwareprobe
