#pragma once

#include <string>
#include <string_view>

// The prompt box's slash commands: /clear, /model <name>, /models, /characters.
struct SlashCommand {
    enum class Kind {
        None,             // not a command: the text is a prompt
        Clear,
        Model,            // argument: the model name
        Models,
        Characters,
        MissingArgument,  // /model without a name
        Unknown,          // argument: the word that was typed
    };
    Kind kind = Kind::None;
    std::string argument;
};

SlashCommand parseSlashCommand(std::string_view text);

// The line shown in the transcript for a command that could not run.
std::string slashCommandError(const SlashCommand& command);
