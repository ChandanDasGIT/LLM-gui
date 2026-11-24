#pragma once

#include "common/chatstore.h"
#include "common/ollamaapi.h"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

// One chat: the system prompt and the messages shown in the transcript. An assistant reply is
// built up from the streamed pieces until it is finished, stopped or fails.
struct ChatEntry {
    enum class Role { User, Assistant, Notice };   // a notice is shown, never sent
    enum class State { Streaming, Finished, Stopped, Failed };

    Role role = Role::User;
    State state = State::Finished;
    std::string content;
    std::string thinking;
    std::string error;
    std::vector<std::string> imagePaths;     // shown in the transcript
    std::vector<std::string> imagesBase64;   // sent to the model
};

class Conversation {
public:
    void setSystemPrompt(std::string prompt) { systemPrompt_ = std::move(prompt); }
    const std::string& systemPrompt() const { return systemPrompt_; }

    const std::vector<ChatEntry>& entries() const { return entries_; }
    bool streaming() const;

    // Each returns the index of the new entry.
    std::size_t addUser(std::string text, std::vector<std::string> imagePaths = {},
                        std::vector<std::string> imagesBase64 = {});
    std::size_t addGreeting(std::string text);   // a finished assistant entry
    std::size_t startReply();                    // a streaming assistant entry
    std::size_t addNotice(std::string text);

    // These act on the streaming reply; they do nothing when there is none.
    void appendContent(const std::string& piece);
    void appendThinking(const std::string& piece);
    void finishReply();
    void stopReply();
    void failReply(std::string error);

    // Removes every entry; the system prompt stays.
    void clear() { entries_.clear(); }

    // What is sent to /api/chat: the system prompt (if any), then every user entry and every
    // assistant entry that has text. The streaming reply, failed replies and notices are left out.
    std::vector<ollama::ChatMessage> requestMessages() const;

    // For saving: user and assistant entries (a streaming reply as "stopped"); notices are left out.
    std::vector<StoredMessage> stored() const;
    // Replaces the entries with saved ones. readImage turns an image path into Base64 (empty when
    // the file is gone, and the image is then not sent again).
    void restore(const std::vector<StoredMessage>& messages,
                 const std::function<std::string(const std::string& path)>& readImage);
    bool hasUserEntry() const;

private:
    ChatEntry* streamingReply();

    std::string systemPrompt_;
    std::vector<ChatEntry> entries_;
};
