#pragma once

#include "modelManager/hardwareprobe.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The recommended models (resources/catalogue.json) and how well each fits this machine.
struct CatalogueEntry {
    std::string tag;                        // e.g. "qwen3:8b"
    std::vector<std::string> categories;    // General, Coding, Reasoning, Writing, Vision, Tiny/fast
    std::string sizeText;                   // as ollama.com shows it, e.g. "5.2GB"
    long long sizeBytes = 0;
    std::string licence;
    bool vision = false;
    bool thinking = false;
    std::string note;
};

struct Catalogue {
    std::string snapshotDate;
    std::vector<std::string> categories;    // in display order
    std::vector<CatalogueEntry> entries;
};

namespace catalogue {

std::optional<Catalogue> parse(std::string_view text, std::string* error = nullptr);

// "5.2GB" / "523MB" -> bytes (decimal units, as ollama.com uses them); 0 when unreadable.
long long parseSize(std::string_view text);

enum class Fit {
    FitsGpu,       // size + 1.5 GB fits in VRAM
    PartlyCpu,     // fits in VRAM plus half the available RAM
    CpuOnly,       // no GPU, fits in half the available RAM
    TooBig,
};
Fit rateFit(long long sizeBytes, const Hardware& hw);
std::string fitText(Fit fit);

// Entries in the category (all when empty), in catalogue order; TooBig ones only with showAll.
std::vector<CatalogueEntry> filter(const Catalogue& catalogue, const std::string& category,
                                   const Hardware& hw, bool showAll);

} // namespace catalogue
