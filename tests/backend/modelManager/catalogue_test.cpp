#include "testkit.h"
#include "modelManager/catalogue.h"

#include <fstream>
#include <sstream>

namespace {
Hardware machine()   // the RTX 3060 Ti box: 8 GiB VRAM, ~21.7 GiB RAM available
{
    Hardware hw;
    hw.gpuName = "RTX 3060 Ti";
    hw.vramBytes = 8192LL * 1024 * 1024;
    hw.ramTotalBytes = 32665992LL * 1024;
    hw.ramAvailableBytes = 22754384LL * 1024;
    return hw;
}
}

TEST(catalogue, sizes_as_ollama_shows_them)
{
    CHECK(catalogue::parseSize("5.2GB") == 5'200'000'000LL);
    CHECK(catalogue::parseSize("523MB") == 523'000'000LL);
    CHECK(catalogue::parseSize("19GB") == 19'000'000'000LL);
    CHECK(catalogue::parseSize("1.1 GB") == 1'100'000'000LL);
    CHECK(catalogue::parseSize("big") == 0);
    CHECK(catalogue::parseSize("5.2XB") == 0);
}

TEST(catalogue, fit_rating)
{
    using catalogue::Fit;
    const Hardware hw = machine();
    CHECK(catalogue::rateFit(5'200'000'000LL, hw) == Fit::FitsGpu);       // 6.7 GB <= 8.59 GB
    CHECK(catalogue::rateFit(8'200'000'000LL, hw) == Fit::PartlyCpu);     // 9.7 GB
    CHECK(catalogue::rateFit(19'000'000'000LL, hw) == Fit::TooBig);       // 20.5 GB > 8.59 + 11.65
    Hardware cpu = hw;
    cpu.gpuName.clear();
    cpu.vramBytes = 0;
    CHECK(catalogue::rateFit(5'200'000'000LL, cpu) == Fit::CpuOnly);
    CHECK(catalogue::rateFit(14'000'000'000LL, cpu) == Fit::TooBig);
    CHECK(catalogue::fitText(Fit::PartlyCpu) == "Partly CPU (slower)");
}

TEST(catalogue, parse_and_filter)
{
    const auto c = catalogue::parse(R"({"snapshot":"2026-11-24","categories":["General","Vision"],"models":[
        {"tag":"a:1b","categories":["General"],"size":"1.0GB","licence":"x","note":"n"},
        {"tag":"v:4b","categories":["General","Vision"],"size":"3.4GB","vision":true},
        {"tag":"huge:70b","categories":["General"],"size":"43GB"}]})");
    CHECK(c.has_value());
    CHECK(c->snapshotDate == "2026-11-24" && c->categories.size() == 2 && c->entries.size() == 3);
    CHECK(c->entries[1].vision && !c->entries[0].vision && c->entries[0].licence == "x");
    const Hardware hw = machine();
    CHECK(catalogue::filter(*c, "General", hw, false).size() == 2);
    CHECK(catalogue::filter(*c, "General", hw, true).size() == 3);
    const auto vision = catalogue::filter(*c, "Vision", hw, false);
    CHECK(vision.size() == 1 && vision[0].tag == "v:4b");
    CHECK(catalogue::filter(*c, "", hw, true).size() == 3);

    std::string error;
    CHECK(!catalogue::parse(R"({"models":[{"tag":"x","size":"?"}]})", &error));
    CHECK(error.find("readable size") != std::string::npos);
    CHECK(!catalogue::parse("{}"));
}

TEST(catalogue, the_shipped_file_loads)
{
    std::ifstream file(LLMGUI_SOURCE_DIR "/resources/catalogue.json");
    std::stringstream text;
    text << file.rdbuf();
    std::string error;
    const auto c = catalogue::parse(text.str(), &error);
    CHECK(c.has_value());
    if (!c) { std::printf("    %s\n", error.c_str()); return; }
    CHECK(c->categories.size() == 6);
    for (const auto& e : c->entries) {
        bool known = !e.categories.empty();
        for (const auto& cat : e.categories) {
            bool found = false;
            for (const auto& k : c->categories) found = found || k == cat;
            known = known && found;
        }
        CHECK(known);
    }
    for (const auto& category : c->categories)
        CHECK(!catalogue::filter(*c, category, machine(), false).empty());
}
