#include "common/characterstore.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace {

bool validId(const std::string& id)
{
    if (id.empty() || id.size() > 64) return false;
    for (const char c : id)
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')) return false;
    return true;
}

std::string lower(std::string s)
{
    for (auto& c : s)
        if (c >= 'A' && c <= 'Z') c = char(c - 'A' + 'a');
    return s;
}

} // namespace

std::string CharacterStore::idFromName(const std::string& name)
{
    std::string id;
    bool dash = false;
    for (const char raw : name) {
        const char c = (raw >= 'A' && raw <= 'Z') ? char(raw - 'A' + 'a') : raw;
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            if (dash && !id.empty()) id += '-';
            id += c;
            dash = false;
        } else {
            dash = true;
        }
        if (id.size() >= 40) break;
    }
    if (id.empty() || id == Character::defaultId()) id = "character" + (id.empty() ? std::string() : "-" + id);
    return id;
}

std::string CharacterStore::freeId(const std::string& base) const
{
    std::error_code ec;
    if (!fs::exists(fileFor(base), ec)) return base;
    for (int n = 2;; ++n) {
        const std::string id = base + "-" + std::to_string(n);
        if (!fs::exists(fileFor(id), ec)) return id;
    }
}

std::optional<Character> CharacterStore::load(const std::string& id) const
{
    std::ifstream file(fileFor(id), std::ios::binary);
    if (!file) return std::nullopt;
    std::stringstream text;
    text << file.rdbuf();
    const auto value = json::parse(text.str());
    if (!value) return std::nullopt;
    auto c = Character::fromJson(*value);
    if (c) c->id = id;
    return c;
}

std::vector<Character> CharacterStore::loadAll(std::vector<std::string>* problems) const
{
    std::vector<Character> saved;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(folder_, ec)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json") continue;
        const std::string id = entry.path().stem().string();
        auto c = validId(id) && id != Character::defaultId() ? load(id) : std::nullopt;
        if (c && validateCharacter(*c).empty()) saved.push_back(std::move(*c));
        else if (problems) problems->push_back(entry.path().filename().string());
    }
    std::sort(saved.begin(), saved.end(), [](const Character& a, const Character& b) {
        const auto la = lower(a.name), lb = lower(b.name);
        return la != lb ? la < lb : a.id < b.id;
    });
    std::vector<Character> all{Character::builtInDefault()};
    all.insert(all.end(), saved.begin(), saved.end());
    return all;
}

std::string CharacterStore::save(Character character, std::string* error) const
{
    const auto fail = [&](std::string message) {
        if (error) *error = std::move(message);
        return std::string();
    };
    if (character.isDefault()) return fail("The Default character cannot be changed.");
    if (const auto problem = validateCharacter(character); !problem.empty()) return fail(problem);
    if (character.id.empty()) character.id = freeId(idFromName(character.name));
    else if (!validId(character.id)) return fail("Invalid character id.");

    std::error_code ec;
    fs::create_directories(folder_, ec);
    if (ec) return fail("Could not create " + folder_.string() + ": " + ec.message());

    // Written to a temporary file first, so a failed write never leaves half a character.
    const fs::path target = fileFor(character.id);
    const fs::path temp = folder_ / ("." + character.id + ".json.tmp");
    {
        std::ofstream file(temp, std::ios::binary | std::ios::trunc);
        file << json::write(character.toJson()) << '\n';
        if (!file.flush()) return fail("Could not write " + temp.string());
    }
    fs::rename(temp, target, ec);
    if (ec) {
        fs::remove(temp, ec);
        return fail("Could not save " + target.string());
    }
    return character.id;
}

bool CharacterStore::remove(const std::string& id, std::string* error) const
{
    if (id == Character::defaultId() || !validId(id)) {
        if (error) *error = "The Default character cannot be deleted.";
        return false;
    }
    std::error_code ec;
    if (!fs::remove(fileFor(id), ec)) {
        if (error) *error = "No saved character \"" + id + "\".";
        return false;
    }
    return true;
}

std::string CharacterStore::duplicate(const std::string& id, std::string* error) const
{
    Character copy;
    if (id == Character::defaultId()) {
        copy = Character::builtInDefault();
    } else {
        auto c = validId(id) ? load(id) : std::nullopt;
        if (!c) {
            if (error) *error = "No saved character \"" + id + "\".";
            return {};
        }
        copy = std::move(*c);
    }
    copy.id.clear();
    copy.name += " copy";
    return save(std::move(copy), error);
}
