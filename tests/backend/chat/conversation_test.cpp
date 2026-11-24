#include "testkit.h"
#include "chat/conversation.h"

TEST(conversation, a_reply_streams_in_and_goes_into_the_next_request)
{
    Conversation c;
    c.setSystemPrompt("You are terse.");
    c.addGreeting("Hello.");
    c.addUser("Describe it", {"/tmp/a.png"}, {"aGk="});
    CHECK(!c.streaming());
    const auto reply = c.startReply();
    CHECK(c.streaming() && reply == 2);
    c.appendThinking("let me ");
    c.appendThinking("see");
    c.appendContent("A ");
    c.appendContent("cat.");

    auto messages = c.requestMessages();                 // the streaming reply is not sent
    CHECK(messages.size() == 3);
    CHECK(messages[0].role == "system" && messages[0].content == "You are terse.");
    CHECK(messages[1].role == "assistant" && messages[1].content == "Hello.");
    CHECK(messages[2].role == "user" && messages[2].imagesBase64.size() == 1);

    c.finishReply();
    CHECK(!c.streaming());
    CHECK(c.entries()[reply].content == "A cat." && c.entries()[reply].thinking == "let me see");
    CHECK(c.entries()[1].imagePaths[0] == "/tmp/a.png");
    messages = c.requestMessages();
    CHECK(messages.size() == 4 && messages[3].content == "A cat." && messages[3].imagesBase64.empty());
}

TEST(conversation, stopped_replies_are_kept_and_failed_ones_are_not_sent)
{
    Conversation c;
    c.addUser("one");
    c.startReply();
    c.appendContent("partial");
    c.stopReply();
    CHECK(c.entries().back().state == ChatEntry::State::Stopped);
    c.addUser("two");
    c.startReply();
    c.failReply("model not found");
    CHECK(c.entries().back().error == "model not found");
    c.appendContent("late piece");                         // nothing is streaming any more
    CHECK(c.entries().back().content.empty());
    const auto messages = c.requestMessages();
    CHECK(messages.size() == 3);                           // no system prompt, failed reply left out
    CHECK(messages[1].content == "partial" && messages[2].content == "two");
}

TEST(conversation, clear_keeps_the_system_prompt)
{
    Conversation c;
    c.setSystemPrompt("p");
    c.addUser("x");
    c.clear();
    CHECK(c.entries().empty());
    CHECK(c.requestMessages().size() == 1);
}

TEST(conversation, notices_are_shown_but_not_sent)
{
    Conversation c;
    c.addUser("hi");
    c.addNotice("Unknown command /x");
    CHECK(c.entries().size() == 2 && c.entries()[1].role == ChatEntry::Role::Notice);
    const auto messages = c.requestMessages();
    CHECK(messages.size() == 1 && messages[0].content == "hi");

    c.startReply();                                        // a notice during a reply
    c.addNotice("Usage: /model <name>");
    CHECK(c.streaming());
    c.appendContent("still mine");
    c.finishReply();
    CHECK(!c.streaming() && c.entries()[2].content == "still mine");
}

TEST(conversation, stored_and_restored)
{
    Conversation c;
    c.addGreeting("Ahoy");
    c.addUser("look", {"/pics/a.png", "/pics/gone.png"}, {"QQ==", "Qg=="});
    c.addNotice("not saved");
    c.startReply();
    c.appendThinking("hm");
    c.appendContent("half");                               // saved while streaming: becomes stopped
    const auto saved = c.stored();
    CHECK(saved.size() == 3);
    CHECK(saved[0].role == "assistant" && saved[0].state == "finished");
    CHECK(saved[1].role == "user" && saved[1].imagePaths.size() == 2);
    CHECK(saved[2].state == "stopped" && saved[2].thinking == "hm");

    Conversation back;
    CHECK(!back.hasUserEntry());
    back.restore(saved, [](const std::string& path) { return path == "/pics/a.png" ? std::string("QQ==") : std::string(); });
    CHECK(back.hasUserEntry() && back.entries().size() == 3 && !back.streaming());
    CHECK(back.entries()[1].imagePaths.size() == 2 && back.entries()[1].imagesBase64.size() == 1);
    CHECK(back.entries()[2].state == ChatEntry::State::Stopped && back.entries()[2].content == "half");
    const auto request = back.requestMessages();
    CHECK(request.size() == 3 && request[2].content == "half");
}
