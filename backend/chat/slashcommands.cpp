#include "chat/slashcommands.h"

namespace {

std::string_view trim(std::string_view s)
{
    const auto isSpace = [](char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; };
    while (!s.empty() && isSpace(s.front())) s.remove_prefix(1);
    while (!s.empty() && isSpace(s.back())) s.remove_suffix(1);
    return s;
}

} // namespace

SlashCommand parseSlashCommand(std::string_view text)
{
    const std::string_view t = trim(text);
    if (t.empty() || t.front() != '/') return {};

    const auto space = t.find_first_of(" \t\n");
    const std::string_view word = t.substr(0, space);
    const std::string_view rest = space == std::string_view::npos ? std::string_view() : trim(t.substr(space));

    if (word == "/clear") return {SlashCommand::Kind::Clear, {}};
    if (word == "/models") return {SlashCommand::Kind::Models, {}};
    if (word == "/characters") return {SlashCommand::Kind::Characters, {}};
    if (word == "/model") {
        if (rest.empty()) return {SlashCommand::Kind::MissingArgument, "/model"};
        return {SlashCommand::Kind::Model, std::string(rest)};
    }
    return {SlashCommand::Kind::Unknown, std::string(word)};
}

std::string slashCommandError(const SlashCommand& command)
{
    switch (command.kind) {
    case SlashCommand::Kind::MissingArgument:
        return "Usage: /model <name>";
    case SlashCommand::Kind::Unknown:
        return "Unknown command " + command.argument + ". Commands: /clear, /model <name>, /models, /characters";
    default:
        return {};
    }
}
