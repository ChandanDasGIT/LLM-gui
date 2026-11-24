#include "testkit.h"
#include "statusstrip.h"

TEST(statusstrip, texts)
{
    using statusstrip::Connection;
    CHECK(statusstrip::modelText("") == "No model chosen");
    CHECK(statusstrip::modelText("qwen3:8b") == "qwen3:8b");
    CHECK(statusstrip::characterText("") == "Default");
    CHECK(statusstrip::characterText("Pirate") == "Pirate");
    CHECK(statusstrip::connectionText(Connection::Offline, "") == "Ollama not running");
    CHECK(statusstrip::connectionText(Connection::Connected, "0.40.16") == "Ollama 0.40.16");
    CHECK(statusstrip::connectionText(Connection::Checking, "").find("Connecting") == 0);
}

TEST(statusstrip, tokens_per_second)
{
    CHECK_NEAR(statusstrip::tokensPerSecond(42, 2'000'000'000LL), 21.0, 1e-9);
    CHECK(statusstrip::tokensPerSecond(0, 5) == 0.0);
    CHECK(statusstrip::tokensPerSecond(5, 0) == 0.0);
    CHECK(statusstrip::tokensPerSecondText(47, 2'000'000'000LL) == "23.5 tok/s");
    CHECK(statusstrip::tokensPerSecondText(0, 0).empty());
}
