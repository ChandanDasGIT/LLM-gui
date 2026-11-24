#include "testkit.h"
#include "modelManager/hardwareprobe.h"

TEST(hardwareprobe, nvidia_smi_picks_the_biggest_gpu)
{
    Hardware hw;
    CHECK(hardwareprobe::parseNvidiaSmi("NVIDIA GeForce RTX 3060 Ti, 8192\n", hw));
    CHECK(hw.gpuName == "RTX 3060 Ti" && hw.vramBytes == 8192LL * 1024 * 1024);
    Hardware two;
    CHECK(hardwareprobe::parseNvidiaSmi("NVIDIA GeForce GTX 1050, 2048\r\nNVIDIA RTX A4000, 16376\n", two));
    CHECK(two.gpuName == "RTX A4000" && two.vramBytes == 16376LL * 1024 * 1024);
    Hardware none;
    CHECK(!hardwareprobe::parseNvidiaSmi("", none));
    CHECK(!hardwareprobe::parseNvidiaSmi("NVIDIA-SMI has failed because it couldn't communicate with the NVIDIA driver.", none));
    CHECK(!hardwareprobe::parseNvidiaSmi("GPU, [N/A]", none));
    CHECK(none.gpuName.empty() && none.vramBytes == 0);
}

TEST(hardwareprobe, amd_sysfs_and_meminfo)
{
    Hardware hw;
    CHECK(hardwareprobe::parseAmdVram("17163091968\n", hw));
    CHECK(hw.vramBytes == 17163091968LL && hw.gpuName == "AMD GPU");
    CHECK(!hardwareprobe::parseAmdVram("x", hw));
    CHECK(hardwareprobe::parseMeminfo("MemTotal:       32665992 kB\nMemFree:        10760836 kB\nMemAvailable:   22754384 kB\n", hw));
    CHECK(hw.ramTotalBytes == 32665992LL * 1024 && hw.ramAvailableBytes == 22754384LL * 1024);
    Hardware partial;
    CHECK(!hardwareprobe::parseMeminfo("MemTotal: 100 kB\n", partial));
}

TEST(hardwareprobe, summary_line)
{
    Hardware hw;
    hw.gpuName = "RTX 3060 Ti";
    hw.vramBytes = 8192LL * 1024 * 1024;
    hw.ramTotalBytes = 32665992LL * 1024;
    CHECK(hardwareprobe::summary(hw) == "RTX 3060 Ti · 8 GB VRAM · 31 GB RAM");
    hw.gpuName.clear();
    CHECK(hardwareprobe::summary(hw) == "No GPU found · 31 GB RAM");
}
