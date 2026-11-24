#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The Ollama HTTP API (https://github.com/ollama/ollama/blob/main/docs/api.md): request bodies,
// and parsing of the replies and of the streamed lines (one JSON object per line).
namespace ollama {

struct ChatMessage {
    std::string role;                      // "system", "user" or "assistant"
    std::string content;
    std::vector<std::string> imagesBase64;
};

struct ChatOptions {
    std::optional<double> temperature;
    bool think = false;                    // only for models with the "thinking" capability
};

std::string chatRequest(const std::string& model, const std::vector<ChatMessage>& messages,
                        const ChatOptions& options);
std::string modelRequest(const std::string& model);                // /api/show and /api/delete
std::string pullRequest(const std::string& model);
std::string createRequest(const std::string& model, const std::string& fileName, const std::string& sha256Hex);

// One line of a streamed /api/chat reply.
struct ChatChunk {
    std::string content;
    std::string thinking;
    bool done = false;
    std::string doneReason;
    long long evalCount = 0;               // tokens generated, on the last line
    long long evalDurationNs = 0;
    std::string error;                     // set when the server sent {"error": ...}
};
std::optional<ChatChunk> parseChatLine(std::string_view line);

// One line of a streamed /api/pull or /api/create reply.
struct Progress {
    std::string status;
    std::string digest;
    long long total = 0;
    long long completed = 0;
    std::string error;
    bool success() const { return status == "success"; }
};
std::optional<Progress> parseProgressLine(std::string_view line);

// The whole download of a pull: each layer reports its own total and completed bytes.
class PullTotals {
public:
    void add(const Progress& progress);
    long long completed() const;
    long long total() const;
    double fraction() const;               // 0..1; 0 before any layer has a size

private:
    struct Layer { std::string digest; long long total = 0; long long completed = 0; };
    std::vector<Layer> layers_;
};

struct InstalledModel {
    std::string name;
    long long sizeBytes = 0;
    std::string family;
    std::string parameterSize;             // e.g. "12.2B"
    std::string quantization;              // e.g. "Q4_K_M"
};
// /api/tags; nullopt when the body is not a tags reply.
std::optional<std::vector<InstalledModel>> parseTags(std::string_view body);

struct ModelInfo {
    std::vector<std::string> capabilities; // e.g. "completion", "vision", "thinking", "tools"
    bool has(std::string_view capability) const;
};
std::optional<ModelInfo> parseShow(std::string_view body);

std::optional<std::string> parseVersion(std::string_view body);

// The "error" text of a reply body, or empty.
std::string errorText(std::string_view body);

// Splits a streamed body into lines as the bytes arrive.
class LineBuffer {
public:
    // The complete lines in these bytes and those before them, without their line ends.
    std::vector<std::string> feed(std::string_view bytes);
    // What is left after the last line end (the last line of a body without one at its end).
    std::string takeRest();

private:
    std::string pending_;
};

} // namespace ollama
