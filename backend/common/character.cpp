#include "common/character.h"

namespace {

bool isBlank(const std::string& s)
{
    for (const char c : s)
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') return false;
    return true;
}

} // namespace

Character Character::builtInDefault()
{
    Character c;
    c.id = defaultId();
    c.name = "Default";
    return c;
}

json::Value Character::toJson() const
{
    json::Value v = json::Value::object();
    v.set("name", name);
    v.set("systemPrompt", systemPrompt);
    v.set("greeting", greeting);
    v.set("preferredModel", preferredModel);
    v.set("temperature", temperature ? json::Value(*temperature) : json::Value());
    return v;
}

std::optional<Character> Character::fromJson(const json::Value& value)
{
    if (!value.isObject() || !value["name"].isString()) return std::nullopt;
    Character c;
    c.name = value["name"].asString();
    c.systemPrompt = value["systemPrompt"].asString();
    c.greeting = value["greeting"].asString();
    c.preferredModel = value["preferredModel"].asString();
    if (value["temperature"].isNumber()) c.temperature = value["temperature"].asNumber();
    return c;
}

std::string validateCharacter(const Character& character)
{
    if (isBlank(character.name)) return "The character needs a name.";
    if (character.name.size() > 100) return "The name is longer than 100 characters.";
    if (character.temperature && (*character.temperature < 0.0 || *character.temperature > 2.0))
        return "Temperature must be between 0 and 2.";
    return {};
}
