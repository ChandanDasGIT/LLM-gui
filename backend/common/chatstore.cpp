#include "common/chatstore.h"

#include <algorithm>
#include <ctime>
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

json::Value stringList(const std::vector<std::string>& items)
{
    json::Value list = json::Value::array();
    for (const auto& s : items) list.push(s);
    return list;
}

long long localDay(long long ms)
{
    const std::time_t seconds = std::time_t(ms / 1000);
    std::tm local{};
    localtime_r(&seconds, &local);
    return (long long)(local.tm_year) * 400 + local.tm_yday;
}

} // namespace

json::Value ChatRecord::toJson() const
{
    json::Value v = json::Value::object();
    v.set("title", title);
    v.set("updatedMs", updatedMs);
    v.set("model", model);
    v.set("characterId", characterId);
    v.set("characterName", characterName);
    v.set("systemPrompt", systemPrompt);
    v.set("greeting", greeting);
    v.set("temperature", temperature ? json::Value(*temperature) : json::Value());
    json::Value list = json::Value::array();
    for (const auto& m : messages) {
        json::Value item = json::Value::object();
        item.set("role", m.role);
        item.set("content", m.content);
        if (!m.thinking.empty()) item.set("thinking", m.thinking);
        item.set("state", m.state);
        if (!m.error.empty()) item.set("error", m.error);
        if (!m.imagePaths.empty()) item.set("images", stringList(m.imagePaths));
        list.push(std::move(item));
    }
    v.set("messages", std::move(list));
    return v;
}

std::optional<ChatRecord> ChatRecord::fromJson(const json::Value& v)
{
    if (!v.isObject() || !v["messages"].isArray()) return std::nullopt;
    ChatRecord r;
    r.title = v["title"].asString();
    r.updatedMs = v["updatedMs"].asInt();
    r.model = v["model"].asString();
    r.characterId = v["characterId"].asString();
    r.characterName = v["characterName"].asString();
    r.systemPrompt = v["systemPrompt"].asString();
    r.greeting = v["greeting"].asString();
    if (v["temperature"].isNumber()) r.temperature = v["temperature"].asNumber();
    for (const auto& item : v["messages"].items()) {
        StoredMessage m;
        m.role = item["role"].asString();
        if (m.role != "user" && m.role != "assistant") continue;
        m.content = item["content"].asString();
        m.thinking = item["thinking"].asString();
        m.state = item["state"].asString();
        if (m.state.empty() || m.state == "streaming") m.state = "stopped";
        m.error = item["error"].asString();
        for (const auto& path : item["images"].items()) m.imagePaths.push_back(path.asString());
        r.messages.push_back(std::move(m));
    }
    return r;
}

namespace chatstore {

std::string titleFrom(const std::string& prompt)
{
    std::size_t start = prompt.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "New chat";
    std::string line = prompt.substr(start, prompt.find('\n', start) - start);
    while (!line.empty() && (line.back() == ' ' || line.back() == '\r' || line.back() == '\t')) line.pop_back();
    constexpr std::size_t limit = 48;
    if (line.size() <= limit) return line;
    std::size_t cut = limit;
    while (cut > 0 && (static_cast<unsigned char>(line[cut]) & 0xC0) == 0x80) --cut;   // not inside a UTF-8 character
    return line.substr(0, cut) + "…";
}

std::string dayGroup(long long updatedMs, long long nowMs)
{
    const long long days = localDay(nowMs) - localDay(updatedMs);
    if (days <= 0) return "Today";
    if (days == 1) return "Yesterday";
    return "Earlier";
}

} // namespace chatstore

std::vector<ChatSummary> ChatStore::list() const
{
    std::vector<ChatSummary> out;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(folder_, ec)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json") continue;
        const std::string id = entry.path().stem().string();
        if (!validId(id)) continue;
        const auto record = load(id);
        if (!record) continue;
        out.push_back({id, record->title, record->updatedMs, record->model, record->characterName});
    }
    std::sort(out.begin(), out.end(), [](const ChatSummary& a, const ChatSummary& b) {
        return a.updatedMs != b.updatedMs ? a.updatedMs > b.updatedMs : a.id > b.id;
    });
    return out;
}

std::optional<ChatRecord> ChatStore::load(const std::string& id) const
{
    if (!validId(id)) return std::nullopt;
    std::ifstream file(fileFor(id), std::ios::binary);
    if (!file) return std::nullopt;
    std::stringstream text;
    text << file.rdbuf();
    const auto value = json::parse(text.str());
    if (!value) return std::nullopt;
    auto record = ChatRecord::fromJson(*value);
    if (record) record->id = id;
    return record;
}

std::string ChatStore::save(ChatRecord record, long long nowMs, std::string* error) const
{
    const auto fail = [&](std::string message) {
        if (error) *error = std::move(message);
        return std::string();
    };
    std::error_code ec;
    if (record.id.empty()) {
        const std::string base = "chat-" + std::to_string(nowMs);
        record.id = base;
        for (int n = 2; fs::exists(fileFor(record.id), ec); ++n) record.id = base + "-" + std::to_string(n);
    } else if (!validId(record.id)) {
        return fail("Invalid chat id.");
    }
    record.updatedMs = nowMs;
    fs::create_directories(folder_, ec);
    if (ec) return fail("Could not create " + folder_.string() + ": " + ec.message());

    // Written to a temporary file first, so a failed write never leaves half a chat.
    const fs::path target = fileFor(record.id);
    const fs::path temp = folder_ / ("." + record.id + ".json.tmp");
    {
        std::ofstream file(temp, std::ios::binary | std::ios::trunc);
        file << json::write(record.toJson()) << '\n';
        if (!file.flush()) return fail("Could not write " + temp.string());
    }
    fs::rename(temp, target, ec);
    if (ec) {
        fs::remove(temp, ec);
        return fail("Could not save " + target.string());
    }
    return record.id;
}

bool ChatStore::remove(const std::string& id) const
{
    std::error_code ec;
    return validId(id) && fs::remove(fileFor(id), ec);
}
