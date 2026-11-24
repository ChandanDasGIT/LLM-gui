#include "common/ollamaapi.h"

#include "common/json.h"

#include <algorithm>

namespace ollama {

std::string chatRequest(const std::string& model, const std::vector<ChatMessage>& messages,
                        const ChatOptions& options)
{
    json::Value list = json::Value::array();
    for (const auto& m : messages) {
        json::Value message = json::Value::object();
        message.set("role", m.role);
        message.set("content", m.content);
        if (!m.imagesBase64.empty()) {
            json::Value images = json::Value::array();
            for (const auto& image : m.imagesBase64) images.push(image);
            message.set("images", std::move(images));
        }
        list.push(std::move(message));
    }
    json::Value body = json::Value::object();
    body.set("model", model);
    body.set("messages", std::move(list));
    body.set("stream", true);
    if (options.think) body.set("think", true);
    if (options.temperature) {
        json::Value opts = json::Value::object();
        opts.set("temperature", *options.temperature);
        body.set("options", std::move(opts));
    }
    return json::write(body);
}

std::string modelRequest(const std::string& model)
{
    return json::write(json::Value::object().set("model", model));
}

std::string pullRequest(const std::string& model)
{
    json::Value body = json::Value::object();
    body.set("model", model);
    body.set("stream", true);
    return json::write(body);
}

std::string createRequest(const std::string& model, const std::string& fileName, const std::string& sha256Hex)
{
    json::Value files = json::Value::object();
    files.set(fileName, "sha256:" + sha256Hex);
    json::Value body = json::Value::object();
    body.set("model", model);
    body.set("files", std::move(files));
    body.set("stream", true);
    return json::write(body);
}

std::optional<ChatChunk> parseChatLine(std::string_view line)
{
    const auto value = json::parse(line);
    if (!value || !value->isObject()) return std::nullopt;
    ChatChunk chunk;
    chunk.error = (*value)["error"].asString();
    const auto& message = (*value)["message"];
    chunk.content = message["content"].asString();
    chunk.thinking = message["thinking"].asString();
    chunk.done = (*value)["done"].asBool();
    chunk.doneReason = (*value)["done_reason"].asString();
    chunk.evalCount = (*value)["eval_count"].asInt();
    chunk.evalDurationNs = (*value)["eval_duration"].asInt();
    return chunk;
}

std::optional<Progress> parseProgressLine(std::string_view line)
{
    const auto value = json::parse(line);
    if (!value || !value->isObject()) return std::nullopt;
    Progress p;
    p.status = (*value)["status"].asString();
    p.digest = (*value)["digest"].asString();
    p.total = (*value)["total"].asInt();
    p.completed = (*value)["completed"].asInt();
    p.error = (*value)["error"].asString();
    return p;
}

void PullTotals::add(const Progress& progress)
{
    if (progress.digest.empty() || progress.total <= 0) return;
    for (auto& layer : layers_)
        if (layer.digest == progress.digest) {
            layer.total = progress.total;
            layer.completed = std::min(progress.completed, progress.total);
            return;
        }
    layers_.push_back({progress.digest, progress.total, std::min(progress.completed, progress.total)});
}

long long PullTotals::completed() const
{
    long long sum = 0;
    for (const auto& layer : layers_) sum += layer.completed;
    return sum;
}

long long PullTotals::total() const
{
    long long sum = 0;
    for (const auto& layer : layers_) sum += layer.total;
    return sum;
}

double PullTotals::fraction() const
{
    const long long all = total();
    return all > 0 ? double(completed()) / double(all) : 0.0;
}

std::optional<std::vector<InstalledModel>> parseTags(std::string_view body)
{
    const auto value = json::parse(body);
    if (!value || !(*value)["models"].isArray()) return std::nullopt;
    std::vector<InstalledModel> models;
    for (const auto& m : (*value)["models"].items()) {
        InstalledModel model;
        model.name = m["name"].asString();
        if (model.name.empty()) model.name = m["model"].asString();
        model.sizeBytes = m["size"].asInt();
        const auto& details = m["details"];
        model.family = details["family"].asString();
        model.parameterSize = details["parameter_size"].asString();
        model.quantization = details["quantization_level"].asString();
        models.push_back(std::move(model));
    }
    return models;
}

bool ModelInfo::has(std::string_view capability) const
{
    for (const auto& c : capabilities)
        if (c == capability) return true;
    return false;
}

std::optional<ModelInfo> parseShow(std::string_view body)
{
    const auto value = json::parse(body);
    if (!value || !value->isObject() || value->contains("error")) return std::nullopt;
    ModelInfo info;
    for (const auto& c : (*value)["capabilities"].items())
        if (c.isString()) info.capabilities.push_back(c.asString());
    return info;
}

std::optional<std::string> parseVersion(std::string_view body)
{
    const auto value = json::parse(body);
    if (!value || !(*value)["version"].isString()) return std::nullopt;
    return (*value)["version"].asString();
}

std::string errorText(std::string_view body)
{
    const auto value = json::parse(body);
    return value ? (*value)["error"].asString() : std::string();
}

std::vector<std::string> LineBuffer::feed(std::string_view bytes)
{
    pending_.append(bytes);
    std::vector<std::string> lines;
    std::size_t start = 0;
    for (std::size_t end; (end = pending_.find('\n', start)) != std::string::npos; start = end + 1) {
        std::size_t stop = end;
        if (stop > start && pending_[stop - 1] == '\r') --stop;
        if (stop > start) lines.emplace_back(pending_, start, stop - start);
    }
    pending_.erase(0, start);
    return lines;
}

std::string LineBuffer::takeRest()
{
    std::string rest;
    rest.swap(pending_);
    while (!rest.empty() && (rest.back() == '\r' || rest.back() == ' ')) rest.pop_back();
    return rest;
}

} // namespace ollama
