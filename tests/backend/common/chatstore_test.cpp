#include "testkit.h"
#include "common/chatstore.h"

#include <ctime>
#include <fstream>
#include <random>

namespace fs = std::filesystem;

namespace {
// A scratch folder that is removed afterwards; nothing of the user's is touched.
struct ScratchFolder {
    fs::path path;
    ScratchFolder()
    {
        std::random_device rd;
        path = fs::temp_directory_path() / ("llmgui-chats-test-" + std::to_string(rd()));
    }
    ~ScratchFolder() { std::error_code ec; fs::remove_all(path, ec); }
};

ChatRecord sample(const std::string& title)
{
    ChatRecord r;
    r.title = title;
    r.model = "qwen3:8b";
    r.characterId = "pirate";
    r.characterName = "Pirate";
    r.systemPrompt = "Talk like a pirate.";
    r.greeting = "Ahoy";
    r.temperature = 0.9;
    r.messages.push_back({"assistant", "Ahoy", "", "finished", "", {}});
    r.messages.push_back({"user", "hi", "", "finished", "", {"/a.png"}});
    r.messages.push_back({"assistant", "", "", "failed", "model not found", {}});
    return r;
}
}

TEST(chatstore, titles)
{
    CHECK(chatstore::titleFrom("  How does a hash map work?\nmore lines") == "How does a hash map work?");
    CHECK(chatstore::titleFrom("   \n ") == "New chat");
    const std::string longText(60, 'x');
    CHECK(chatstore::titleFrom(longText) == std::string(48, 'x') + "…");
    std::string accents;
    for (int i = 0; i < 30; ++i) accents += "\xC3\xA9";          // 60 bytes of é
    const std::string cut = chatstore::titleFrom(accents);
    CHECK(cut == accents.substr(0, 48) + "…");
}

TEST(chatstore, day_groups)
{
    const long long now = (long long)std::time(nullptr) * 1000;
    CHECK(chatstore::dayGroup(now, now) == "Today");
    CHECK(chatstore::dayGroup(now - 24LL * 3600 * 1000, now) == "Yesterday");
    CHECK(chatstore::dayGroup(now - 5LL * 24 * 3600 * 1000, now) == "Earlier");
}

TEST(chatstore, save_list_load_and_remove)
{
    ScratchFolder scratch;
    ChatStore store(scratch.path / "chats");
    CHECK(store.list().empty());
    std::string error;
    const auto first = store.save(sample("first"), 1000, &error);
    const auto second = store.save(sample("second"), 2000, &error);
    const auto clash = store.save(sample("same time"), 2000, &error);
    CHECK(first == "chat-1000" && second == "chat-2000" && clash == "chat-2000-2");

    auto list = store.list();
    CHECK(list.size() == 3 && list.back().id == "chat-1000");
    CHECK(list[0].updatedMs == 2000 && list[0].characterName == "Pirate" && list[0].model == "qwen3:8b");

    auto loaded = store.load(first);
    CHECK(loaded.has_value());
    CHECK(loaded->title == "first" && loaded->systemPrompt == "Talk like a pirate." && loaded->greeting == "Ahoy");
    CHECK(loaded->temperature && *loaded->temperature == 0.9);
    CHECK(loaded->messages.size() == 3 && loaded->messages[1].imagePaths[0] == "/a.png");
    CHECK(loaded->messages[2].state == "failed" && loaded->messages[2].error == "model not found");

    loaded->title = "renamed";                               // saving again moves it to the top
    CHECK(store.save(*loaded, 3000, &error) == first);
    list = store.list();
    CHECK(list[0].id == first && list[0].title == "renamed");

    CHECK(store.remove(second));
    CHECK(!store.remove(second));
    CHECK(!store.remove("../x"));
    CHECK(!store.load("../x"));
    CHECK(store.list().size() == 2);

    std::ofstream(scratch.path / "chats" / "chat-9.json") << "{broken";
    CHECK(store.list().size() == 2);
    for (const auto& entry : fs::directory_iterator(scratch.path / "chats"))
        CHECK(entry.path().extension() != ".tmp");
}
