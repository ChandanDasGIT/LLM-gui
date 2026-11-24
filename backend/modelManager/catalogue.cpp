#include "modelManager/catalogue.h"

#include "common/json.h"

#include <cmath>
#include <cstdlib>

namespace catalogue {

namespace {
constexpr long long headroomBytes = 1'500'000'000LL;   // context and runtime buffers beside the weights
}

long long parseSize(std::string_view text)
{
    std::string number;
    std::size_t i = 0;
    while (i < text.size() && ((text[i] >= '0' && text[i] <= '9') || text[i] == '.')) number += text[i++];
    while (i < text.size() && text[i] == ' ') ++i;
    const std::string_view unit = text.substr(i);
    if (number.empty()) return 0;
    const double value = std::strtod(number.c_str(), nullptr);
    double scale = 0;
    if (unit == "KB") scale = 1e3;
    else if (unit == "MB") scale = 1e6;
    else if (unit == "GB") scale = 1e9;
    else if (unit == "TB") scale = 1e12;
    return scale > 0 ? (long long)std::llround(value * scale) : 0;
}

std::optional<Catalogue> parse(std::string_view text, std::string* error)
{
    const auto value = json::parse(text, error);
    if (!value) return std::nullopt;
    if (!(*value)["models"].isArray()) {
        if (error) *error = "no \"models\" list";
        return std::nullopt;
    }
    Catalogue c;
    c.snapshotDate = (*value)["snapshot"].asString();
    for (const auto& cat : (*value)["categories"].items()) c.categories.push_back(cat.asString());
    for (const auto& m : (*value)["models"].items()) {
        CatalogueEntry e;
        e.tag = m["tag"].asString();
        for (const auto& cat : m["categories"].items()) e.categories.push_back(cat.asString());
        e.sizeText = m["size"].asString();
        e.sizeBytes = parseSize(e.sizeText);
        e.licence = m["licence"].asString();
        e.vision = m["vision"].asBool();
        e.thinking = m["thinking"].asBool();
        e.note = m["note"].asString();
        if (e.tag.empty() || e.sizeBytes <= 0) {
            if (error) *error = "model entry without a tag or a readable size: " + e.tag;
            return std::nullopt;
        }
        c.entries.push_back(std::move(e));
    }
    return c;
}

Fit rateFit(long long sizeBytes, const Hardware& hw)
{
    const long long need = sizeBytes + headroomBytes;
    const long long spareRam = hw.ramAvailableBytes / 2;
    if (hw.gpuName.empty() || hw.vramBytes <= 0) return need <= spareRam ? Fit::CpuOnly : Fit::TooBig;
    if (need <= hw.vramBytes) return Fit::FitsGpu;
    if (need <= hw.vramBytes + spareRam) return Fit::PartlyCpu;
    return Fit::TooBig;
}

std::string fitText(Fit fit)
{
    switch (fit) {
    case Fit::FitsGpu: return "Fits GPU";
    case Fit::PartlyCpu: return "Partly CPU (slower)";
    case Fit::CpuOnly: return "CPU only (slow)";
    case Fit::TooBig: return "Too big";
    }
    return {};
}

std::vector<CatalogueEntry> filter(const Catalogue& catalogue, const std::string& category,
                                   const Hardware& hw, bool showAll)
{
    std::vector<CatalogueEntry> out;
    for (const auto& e : catalogue.entries) {
        bool inCategory = category.empty();
        for (const auto& c : e.categories)
            if (c == category) inCategory = true;
        if (!inCategory) continue;
        if (!showAll && rateFit(e.sizeBytes, hw) == Fit::TooBig) continue;
        out.push_back(e);
    }
    return out;
}

} // namespace catalogue
