#pragma once

#include "common/character.h"

#include <filesystem>
#include <string>
#include <vector>

// The characters saved as <id>.json files in one folder. The built-in Default is never a file.
class CharacterStore {
public:
    explicit CharacterStore(std::filesystem::path folder) : folder_(std::move(folder)) {}

    const std::filesystem::path& folder() const { return folder_; }

    // Default first, then the saved ones sorted by name. Unreadable files are skipped and named
    // in *problems.
    std::vector<Character> loadAll(std::vector<std::string>* problems = nullptr) const;

    // Saves the character. A new one (empty id) gets an id made from its name. Returns the id,
    // or empty with *error set.
    std::string save(Character character, std::string* error = nullptr) const;
    bool remove(const std::string& id, std::string* error = nullptr) const;
    // A copy named "<name> copy"; returns the new id, or empty.
    std::string duplicate(const std::string& id, std::string* error = nullptr) const;

    // Lowercase letters, digits and dashes from the name, never empty and never "default".
    static std::string idFromName(const std::string& name);

private:
    std::filesystem::path fileFor(const std::string& id) const { return folder_ / (id + ".json"); }
    std::string freeId(const std::string& base) const;
    std::optional<Character> load(const std::string& id) const;

    std::filesystem::path folder_;
};
