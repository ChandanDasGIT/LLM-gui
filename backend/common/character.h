#pragma once

#include "common/json.h"

#include <optional>
#include <string>

// A personality for the chat: a name, a system prompt and the optional extras.
struct Character {
    std::string id;                    // also the file name (<id>.json); "default" is built in
    std::string name;
    std::string systemPrompt;
    std::string greeting;
    std::string preferredModel;        // empty: keep the current model
    std::optional<double> temperature; // empty: the model's own default

    bool isDefault() const { return id == defaultId(); }

    static std::string defaultId() { return "default"; }
    // The built-in character: no system prompt, no greeting.
    static Character builtInDefault();

    json::Value toJson() const;
    // nullopt when the value is not an object with a name.
    static std::optional<Character> fromJson(const json::Value& value);
};

// Empty when the character can be saved, otherwise the reason it cannot.
std::string validateCharacter(const Character& character);
