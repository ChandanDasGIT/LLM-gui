#include "testkit.h"
#include "fakeollama.h"

#include "wrapper/characters/charactersWrap.h"
#include "wrapper/common/appSession/appSessionWrap.h"

#include <QDir>
#include <QFile>

namespace {
QVariant role(const CharactersWrap& c, int row, const char* name)
{
    return c.data(c.index(row), c.roleNames().key(name));
}
}

TEST(qtcharacters, create_edit_duplicate_and_delete)
{
    FakeSession s;
    CharactersWrap characters;
    CHECK(characters.rowCount() == 1 && role(characters, 0, "builtIn").toBool() && role(characters, 0, "active").toBool());
    CHECK(characters.save(0, "Hacked", "x", "", "", false, 0.8) == -1);
    CHECK(characters.message() == "The Default character cannot be changed.");

    const int row = characters.create();
    CHECK(row == 1 && role(characters, 1, "name") == "New character");
    const int saved = characters.save(row, "Tutor", "Ask one question at a time.", "Hi!", "qwen3:8b", true, 0.4);
    CHECK(saved == 1 && role(characters, 1, "name") == "Tutor" && role(characters, 1, "temperature").toDouble() == 0.4);
    CHECK(role(characters, 1, "useTemperature").toBool() && role(characters, 1, "preferredModel") == "qwen3:8b");
    CHECK(characters.save(saved, "", "", "", "", false, 0) == -1 && characters.message() == "The character needs a name.");
    CHECK(QFile::exists(s.dataFolder() + "/LLM-gui/characters/new-character.json"));

    const int copy = characters.duplicate(saved);
    CHECK(copy >= 0 && role(characters, copy, "name") == "Tutor copy" && characters.rowCount() == 3);
    CHECK(characters.remove(copy) && characters.rowCount() == 2);
    CHECK(!characters.remove(0));
}

TEST(qtcharacters, use_sets_the_session_and_deleting_the_active_one_falls_back_to_default)
{
    FakeSession s;
    auto* session = AppSessionWrap::instance();
    CharactersWrap characters;
    const int row = characters.save(characters.create(), "Pirate", "Arr.", "Ahoy!", "pirate-model:1b", false, 0.8);
    characters.use(row);
    CHECK(session->characterName() == "Pirate" && session->characterSystemPrompt() == "Arr.");
    CHECK(session->currentModel() == "pirate-model:1b" && !session->hasCharacterTemperature());
    CHECK(characters.activeIndex() == row && role(characters, row, "active").toBool());

    characters.save(row, "Pirate", "Arrr!", "Ahoy!", "", true, 1.2);   // editing the active one updates the session
    CHECK(session->characterSystemPrompt() == "Arrr!" && session->characterTemperature() == 1.2);

    CHECK(characters.remove(characters.activeIndex()));
    CHECK(session->characterId() == "default" && session->characterSystemPrompt().isEmpty());
}

TEST(qtcharacters, get_returns_a_whole_row)
{
    FakeSession s;
    CharactersWrap characters;
    const int row = characters.save(characters.create(), "Tutor", "Be patient.", "Hi!", "", true, 0.3);
    const auto c = characters.get(row);
    CHECK(c["name"] == "Tutor" && c["systemPrompt"] == "Be patient." && c["greeting"] == "Hi!");
    CHECK(c["useTemperature"].toBool() && c["temperature"].toDouble() == 0.3 && !c["builtIn"].toBool());
    CHECK(characters.get(0)["builtIn"].toBool() && characters.get(9).isEmpty());
}
