#pragma once

#include "common/json.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

// Saved chats: one <id>.json file per chat in a folder. A chat keeps its model and the
// character it was started with, so reopening it continues the same way.
struct StoredMessage {
    std::string role;                      // "user" or "assistant"
    std::string content;
    std::string thinking;
    std::string state;                     // "finished", "stopped" or "failed"
    std::string error;
    std::vector<std::string> imagePaths;
};

struct ChatRecord {
    std::string id;
    std::string title;
    long long updatedMs = 0;               // milliseconds since 1970
    std::string model;
    std::string characterId;
    std::string characterName;
    std::string systemPrompt;
    std::string greeting;
    std::optional<double> temperature;
    std::vector<StoredMessage> messages;

    json::Value toJson() const;
    static std::optional<ChatRecord> fromJson(const json::Value& value);
};

// The sidebar's line for a chat.
struct ChatSummary {
    std::string id;
    std::string title;
    long long updatedMs = 0;
    std::string model;
    std::string characterName;
};

namespace chatstore {

// The first line of the first prompt, at most 48 characters (cut at a UTF-8 boundary).
std::string titleFrom(const std::string& prompt);

// "Today", "Yesterday" or "Earlier", in local time.
std::string dayGroup(long long updatedMs, long long nowMs);

} // namespace chatstore

class ChatStore {
public:
    explicit ChatStore(std::filesystem::path folder) : folder_(std::move(folder)) {}

    // Newest first. Unreadable files are skipped.
    std::vector<ChatSummary> list() const;
    std::optional<ChatRecord> load(const std::string& id) const;
    // A record without an id gets a new one, made from nowMs. Returns the id, or empty.
    std::string save(ChatRecord record, long long nowMs, std::string* error = nullptr) const;
    bool remove(const std::string& id) const;

private:
    std::filesystem::path fileFor(const std::string& id) const { return folder_ / (id + ".json"); }

    std::filesystem::path folder_;
};
