#include "testkit.h"
#include "common/characterstore.h"

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
        path = fs::temp_directory_path() / ("llmgui-characters-test-" + std::to_string(rd()));
    }
    ~ScratchFolder() { std::error_code ec; fs::remove_all(path, ec); }
};

Character named(const std::string& name)
{
    Character c;
    c.name = name;
    c.systemPrompt = "prompt of " + name;
    return c;
}
}

TEST(characterstore, safe_ids_from_names)
{
    CHECK(CharacterStore::idFromName("Captain Jack!") == "captain-jack");
    CHECK(CharacterStore::idFromName("../../etc/passwd") == "etc-passwd");
    CHECK(CharacterStore::idFromName("Ünïcode") == "n-code");
    CHECK(CharacterStore::idFromName("!!!") == "character");
    CHECK(CharacterStore::idFromName("Default") == "character-default");
    CHECK(CharacterStore::idFromName(std::string(200, 'a')).size() == 40);
}

TEST(characterstore, save_load_rename_and_delete)
{
    ScratchFolder scratch;
    CharacterStore store(scratch.path / "characters");
    auto all = store.loadAll();
    CHECK(all.size() == 1 && all[0].isDefault());          // no folder yet: only Default

    std::string error;
    const auto zed = store.save(named("Zed"), &error);
    const auto amy = store.save(named("amy"), &error);
    const auto amy2 = store.save(named("Amy"), &error);
    CHECK(zed == "zed" && amy == "amy" && amy2 == "amy-2");
    CHECK(fs::exists(scratch.path / "characters" / "zed.json"));

    all = store.loadAll();
    CHECK(all.size() == 4);
    CHECK(all[0].isDefault() && all[1].id == "amy" && all[2].id == "amy-2" && all[3].name == "Zed");
    CHECK(all[3].systemPrompt == "prompt of Zed");

    Character edited = all[3];                              // a rename keeps the id and file
    edited.name = "Zed the Great";
    CHECK(store.save(edited, &error) == "zed");
    all = store.loadAll();
    CHECK(all.size() == 4 && all.back().name == "Zed the Great");

    CHECK(store.remove("amy", &error));
    CHECK(!store.remove("amy", &error));
    CHECK(!store.remove("default", &error));
    CHECK(!store.remove("../x", &error));
    CHECK(store.loadAll().size() == 3);
}

TEST(characterstore, refuses_bad_characters_and_skips_bad_files)
{
    ScratchFolder scratch;
    CharacterStore store(scratch.path);
    std::string error;
    CHECK(store.save(named(""), &error).empty() && error == "The character needs a name.");
    CHECK(store.save(Character::builtInDefault(), &error).empty());
    Character badId = named("X");
    badId.id = "../escape";
    CHECK(store.save(badId, &error).empty());

    CHECK(!store.save(named("Good"), &error).empty());
    std::ofstream(scratch.path / "broken.json") << "{not json";
    std::ofstream(scratch.path / "noname.json") << "{\"systemPrompt\":\"x\"}";
    std::ofstream(scratch.path / "default.json") << "{\"name\":\"Fake default\"}";
    std::ofstream(scratch.path / "notes.txt") << "ignored";
    std::vector<std::string> problems;
    const auto all = store.loadAll(&problems);
    CHECK(all.size() == 2 && all[1].name == "Good");
    CHECK(problems.size() == 3);
    for (const auto& entry : fs::directory_iterator(scratch.path))
        CHECK(entry.path().extension() != ".tmp");
}

TEST(characterstore, duplicate)
{
    ScratchFolder scratch;
    CharacterStore store(scratch.path);
    Character pirate = named("Pirate");
    pirate.temperature = 1.1;
    const auto id = store.save(pirate);
    const auto copy = store.duplicate(id);
    CHECK(copy == "pirate-copy");
    const auto all = store.loadAll();
    CHECK(all.size() == 3 && all[2].name == "Pirate copy" && all[2].temperature && *all[2].temperature == 1.1);
    CHECK(store.duplicate("default") == "default-copy");
    std::string error;
    CHECK(store.duplicate("nobody", &error).empty() && !error.empty());
}
