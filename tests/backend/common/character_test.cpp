#include "testkit.h"
#include "common/character.h"

TEST(character, json_round_trip)
{
    Character c;
    c.name = "Pirate";
    c.systemPrompt = "Talk like a pirate.\nAlways.";
    c.greeting = "Ahoy!";
    c.preferredModel = "qwen3:8b";
    c.temperature = 0.9;
    const auto back = Character::fromJson(c.toJson());
    CHECK(back.has_value());
    CHECK(back->name == "Pirate" && back->systemPrompt == c.systemPrompt && back->greeting == "Ahoy!");
    CHECK(back->preferredModel == "qwen3:8b" && back->temperature && *back->temperature == 0.9);

    c.temperature.reset();
    CHECK(c.toJson()["temperature"].isNull());
    CHECK(!Character::fromJson(c.toJson())->temperature);
    CHECK(!Character::fromJson(json::Value(3)));
    CHECK(!Character::fromJson(json::Value::object()));
}

TEST(character, validation_and_the_default)
{
    Character c;
    CHECK(validateCharacter(c) == "The character needs a name.");
    c.name = "  \n";
    CHECK(!validateCharacter(c).empty());
    c.name = "Ok";
    CHECK(validateCharacter(c).empty());
    c.temperature = 2.5;
    CHECK(validateCharacter(c).find("Temperature") == 0);
    c.temperature = 2.0;
    CHECK(validateCharacter(c).empty());

    const auto d = Character::builtInDefault();
    CHECK(d.isDefault() && d.name == "Default" && d.systemPrompt.empty() && d.greeting.empty());
}
