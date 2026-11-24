#include "chat/conversation.h"

bool Conversation::streaming() const
{
    for (auto it = entries_.rbegin(); it != entries_.rend(); ++it)
        if (it->state == ChatEntry::State::Streaming) return true;
    return false;
}

std::size_t Conversation::addUser(std::string text, std::vector<std::string> imagePaths,
                                  std::vector<std::string> imagesBase64)
{
    ChatEntry entry;
    entry.role = ChatEntry::Role::User;
    entry.content = std::move(text);
    entry.imagePaths = std::move(imagePaths);
    entry.imagesBase64 = std::move(imagesBase64);
    entries_.push_back(std::move(entry));
    return entries_.size() - 1;
}

std::size_t Conversation::addGreeting(std::string text)
{
    ChatEntry entry;
    entry.role = ChatEntry::Role::Assistant;
    entry.content = std::move(text);
    entries_.push_back(std::move(entry));
    return entries_.size() - 1;
}

std::size_t Conversation::startReply()
{
    ChatEntry entry;
    entry.role = ChatEntry::Role::Assistant;
    entry.state = ChatEntry::State::Streaming;
    entries_.push_back(std::move(entry));
    return entries_.size() - 1;
}

std::size_t Conversation::addNotice(std::string text)
{
    ChatEntry entry;
    entry.role = ChatEntry::Role::Notice;
    entry.content = std::move(text);
    entries_.push_back(std::move(entry));
    return entries_.size() - 1;
}

ChatEntry* Conversation::streamingReply()
{
    for (auto it = entries_.rbegin(); it != entries_.rend(); ++it)
        if (it->state == ChatEntry::State::Streaming) return &*it;
    return nullptr;
}

void Conversation::appendContent(const std::string& piece)
{
    if (auto* reply = streamingReply()) reply->content += piece;
}

void Conversation::appendThinking(const std::string& piece)
{
    if (auto* reply = streamingReply()) reply->thinking += piece;
}

void Conversation::finishReply()
{
    if (auto* reply = streamingReply()) reply->state = ChatEntry::State::Finished;
}

void Conversation::stopReply()
{
    if (auto* reply = streamingReply()) reply->state = ChatEntry::State::Stopped;
}

void Conversation::failReply(std::string error)
{
    if (auto* reply = streamingReply()) {
        reply->state = ChatEntry::State::Failed;
        reply->error = std::move(error);
    }
}

std::vector<ollama::ChatMessage> Conversation::requestMessages() const
{
    std::vector<ollama::ChatMessage> messages;
    if (!systemPrompt_.empty()) messages.push_back({"system", systemPrompt_, {}});
    for (const auto& e : entries_) {
        if (e.role == ChatEntry::Role::User) {
            messages.push_back({"user", e.content, e.imagesBase64});
        } else if (e.role == ChatEntry::Role::Assistant && e.state != ChatEntry::State::Streaming
                   && e.state != ChatEntry::State::Failed && !e.content.empty()) {
            messages.push_back({"assistant", e.content, {}});
        }
    }
    return messages;
}

namespace {

std::string stateText(ChatEntry::State state)
{
    switch (state) {
    case ChatEntry::State::Finished: return "finished";
    case ChatEntry::State::Failed: return "failed";
    case ChatEntry::State::Streaming:
    case ChatEntry::State::Stopped: return "stopped";
    }
    return "finished";
}

ChatEntry::State stateFrom(const std::string& text)
{
    if (text == "failed") return ChatEntry::State::Failed;
    if (text == "stopped") return ChatEntry::State::Stopped;
    return ChatEntry::State::Finished;
}

} // namespace

std::vector<StoredMessage> Conversation::stored() const
{
    std::vector<StoredMessage> out;
    for (const auto& e : entries_) {
        if (e.role == ChatEntry::Role::Notice) continue;
        StoredMessage m;
        m.role = e.role == ChatEntry::Role::User ? "user" : "assistant";
        m.content = e.content;
        m.thinking = e.thinking;
        m.state = stateText(e.state);
        m.error = e.error;
        m.imagePaths = e.imagePaths;
        out.push_back(std::move(m));
    }
    return out;
}

void Conversation::restore(const std::vector<StoredMessage>& messages,
                           const std::function<std::string(const std::string& path)>& readImage)
{
    entries_.clear();
    for (const auto& m : messages) {
        ChatEntry e;
        e.role = m.role == "user" ? ChatEntry::Role::User : ChatEntry::Role::Assistant;
        e.state = stateFrom(m.state);
        e.content = m.content;
        e.thinking = m.thinking;
        e.error = m.error;
        e.imagePaths = m.imagePaths;
        for (const auto& path : m.imagePaths) {
            std::string data = readImage ? readImage(path) : std::string();
            if (!data.empty()) e.imagesBase64.push_back(std::move(data));
        }
        entries_.push_back(std::move(e));
    }
}

bool Conversation::hasUserEntry() const
{
    for (const auto& e : entries_)
        if (e.role == ChatEntry::Role::User) return true;
    return false;
}
