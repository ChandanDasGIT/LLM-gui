#include "testkit.h"
#include "chat/slashcommands.h"

TEST(slashcommands, recognises_each_command)
{
    CHECK(parseSlashCommand("hello /clear").kind == SlashCommand::Kind::None);
    CHECK(parseSlashCommand("").kind == SlashCommand::Kind::None);
    CHECK(parseSlashCommand("  /clear  ").kind == SlashCommand::Kind::Clear);
    CHECK(parseSlashCommand("/models").kind == SlashCommand::Kind::Models);
    CHECK(parseSlashCommand("/characters").kind == SlashCommand::Kind::Characters);
    const auto model = parseSlashCommand("/model   hf.co/user/repo:Q4_K_M \n");
    CHECK(model.kind == SlashCommand::Kind::Model && model.argument == "hf.co/user/repo:Q4_K_M");
}

TEST(slashcommands, explains_bad_commands)
{
    const auto missing = parseSlashCommand("/model");
    CHECK(missing.kind == SlashCommand::Kind::MissingArgument);
    CHECK(slashCommandError(missing) == "Usage: /model <name>");
    const auto unknown = parseSlashCommand("/clearr now");
    CHECK(unknown.kind == SlashCommand::Kind::Unknown && unknown.argument == "/clearr");
    CHECK(slashCommandError(unknown).find("Unknown command /clearr") == 0);
    CHECK(slashCommandError(parseSlashCommand("/clear")).empty());
}
