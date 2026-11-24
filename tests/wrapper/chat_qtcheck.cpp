#include "testkit.h"
#include "fakeollama.h"

#include "wrapper/characters/charactersWrap.h"
#include "wrapper/chat/chatWrap.h"
#include "wrapper/chatHistoryWrap.h"
#include "wrapper/common/appSession/appSessionWrap.h"
#include "wrapper/statusStripWrap.h"

#include "common/json.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QtTest>

namespace {

const QByteArray tags = R"({"models":[{"name":"demo:4b","size":3300000000,"details":{"parameter_size":"4.3B","quantization_level":"Q4_K_M"}}]})";

void serveDemo(FakeOllama* ollama, const QByteArray& capabilities)
{
    ollama->onJson("GET", "/api/tags", tags);
    ollama->onJson("POST", "/api/show", R"({"capabilities":)" + capabilities + "}");
    FakeOllama::Response chat;
    chat.lines = {
        R"({"message":{"role":"assistant","content":"","thinking":"hmm"},"done":false})",
        R"({"message":{"role":"assistant","content":"Hi"},"done":false})",
        R"({"message":{"role":"assistant","content":" there!"},"done":false})",
        R"({"message":{"role":"assistant","content":""},"done":true,"done_reason":"stop","eval_count":42,"eval_duration":2000000000})",
    };
    ollama->on("POST", "/api/chat", chat);
}

QVariant role(const ChatWrap& chat, int row, const char* name)
{
    const auto roles = chat.roleNames();
    return chat.data(chat.index(row), roles.key(name));
}

json::Value lastBody(FakeOllama* ollama, const QByteArray& path)
{
    const auto requests = ollama->requestsTo(path);
    if (requests.isEmpty()) return {};
    return json::parse(requests.last().body.toStdString()).value_or(json::Value());
}

} // namespace

TEST(qtchat, a_reply_streams_in_with_thinking_and_the_chat_is_saved)
{
    FakeSession s;
    serveDemo(s.ollama, R"(["completion","thinking"])");
    auto* session = AppSessionWrap::instance();
    session->setCurrentModel("demo:4b");
    ChatWrap chat;
    StatusStripWrap strip;
    ChatHistoryWrap history;
    CHECK(QTest::qWaitFor([&] { return chat.thinks() && chat.models().size() == 1; }, 3000));
    CHECK(chat.modelIndex() == 0 && chat.models()[0].toMap()["details"].toString() == "3.3 GB · 4.3B");
    CHECK(chat.characters().size() == 1 && chat.characterIndex() == 0);   // only Default
    CHECK(chat.isNewChat() && !chat.canAttach());
    CHECK(QTest::qWaitFor([&] { return strip.connected(); }, 3000));
    CHECK(strip.connectionText() == "Ollama 0.40.16" && strip.modelText() == "demo:4b" && strip.characterText() == "Default");

    CHECK(chat.send("  Hello there  "));
    CHECK(chat.busy() && !chat.isNewChat() && chat.rowCount() == 2);
    CHECK(QTest::qWaitFor([&] { return !chat.busy(); }, 3000));
    CHECK(role(chat, 0, "kind") == "user" && role(chat, 0, "content") == "Hello there");
    CHECK(role(chat, 1, "content") == "Hi there!" && role(chat, 1, "thinking") == "hmm");
    CHECK(role(chat, 1, "replyState") == "finished");
    CHECK(strip.speedText() == "21.0 tok/s");

    const auto body = lastBody(s.ollama, "/api/chat");
    CHECK(body["model"].asString() == "demo:4b" && body["think"].asBool());
    CHECK(body["messages"].size() == 1 && body["messages"][0]["content"].asString() == "Hello there");

    CHECK(!session->currentChatId().isEmpty());
    CHECK(QFile::exists(s.dataFolder() + "/LLM-gui/chats/" + session->currentChatId() + ".json"));
    CHECK(history.rowCount() == 1 && history.currentIndex() == 0);
    CHECK(history.data(history.index(0), history.roleNames().key("title")) == "Hello there");
    CHECK(history.data(history.index(0), history.roleNames().key("meta")) == "demo:4b · Default");
    CHECK(history.data(history.index(0), history.roleNames().key("day")) == "Today");
    CHECK(!chat.send("   "));
}

TEST(qtchat, a_saved_chat_reopens_and_new_chat_starts_empty)
{
    FakeSession s;
    serveDemo(s.ollama, R"(["completion"])");
    auto* session = AppSessionWrap::instance();
    session->setCurrentModel("demo:4b");
    QString firstId;
    {
        ChatWrap chat;
        CHECK(chat.send("first chat"));
        CHECK(QTest::qWaitFor([&] { return !chat.busy(); }, 3000));
        firstId = session->currentChatId();
    }
    ChatWrap again;                                        // like the next start of the app
    ChatHistoryWrap history;
    CHECK(again.rowCount() == 2 && role(again, 1, "content") == "Hi there!");
    again.newChat();
    CHECK(again.isNewChat() && again.rowCount() == 0 && session->currentChatId().isEmpty());
    CHECK(history.currentIndex() == -1);
    CHECK(again.send("second chat"));
    CHECK(QTest::qWaitFor([&] { return !again.busy(); }, 3000));
    CHECK(history.rowCount() == 2 && session->currentChatId() != firstId);

    const int firstRow = history.data(history.index(0), history.roleNames().key("chatId")) == firstId ? 0 : 1;
    history.select(firstRow);
    CHECK(session->currentChatId() == firstId && again.rowCount() == 2);
    CHECK(role(again, 0, "content") == "first chat");

    history.remove(firstRow);                              // deleting the open chat opens a new one
    CHECK(history.rowCount() == 1 && session->currentChatId().isEmpty() && again.isNewChat());
    CHECK(!QFile::exists(s.dataFolder() + "/LLM-gui/chats/" + firstId + ".json"));
}

TEST(qtchat, stop_closes_the_stream_and_keeps_the_text)
{
    FakeSession s;
    serveDemo(s.ollama, R"(["completion"])");
    FakeOllama::Response held;
    held.lines = {R"({"message":{"role":"assistant","content":"partial"},"done":false})"};
    held.holdOpen = true;
    s.ollama->on("POST", "/api/chat", held);
    AppSessionWrap::instance()->setCurrentModel("demo:4b");
    ChatWrap chat;
    CHECK(chat.send("go"));
    CHECK(QTest::qWaitFor([&] { return role(chat, 1, "content") == "partial"; }, 3000));
    chat.stop();
    CHECK(!chat.busy() && role(chat, 1, "replyState") == "stopped");
    CHECK(QTest::qWaitFor([&] { return s.ollama->closedByClient == 1; }, 3000));
}

TEST(qtchat, errors_and_slash_commands)
{
    FakeSession s;
    serveDemo(s.ollama, R"(["completion"])");
    s.ollama->onJson("POST", "/api/chat", R"({"error":"model \"demo:4b\" not found, try pulling it first"})", 404);
    auto* session = AppSessionWrap::instance();
    session->setCurrentModel("demo:4b");
    ChatWrap chat;
    CHECK(chat.send("hi"));
    CHECK(QTest::qWaitFor([&] { return !chat.busy(); }, 3000));
    CHECK(role(chat, 1, "replyState") == "failed");
    CHECK(role(chat, 1, "error") == "model \"demo:4b\" not found, try pulling it first");

    CHECK(chat.send("/clearr"));
    CHECK(role(chat, chat.rowCount() - 1, "kind") == "notice");
    CHECK(role(chat, chat.rowCount() - 1, "content").toString().startsWith("Unknown command /clearr"));
    QSignalSpy models(&chat, &ChatWrap::openModelsRequested);
    QSignalSpy characters(&chat, &ChatWrap::openCharactersRequested);
    CHECK(chat.send("/models") && chat.send("/characters"));
    CHECK(models.count() == 1 && characters.count() == 1);

    s.ollama->onJson("POST", "/api/show", R"({"error":"model 'nope' not found"})", 404);
    const int before = chat.rowCount();
    CHECK(chat.send("/model nope"));
    CHECK(QTest::qWaitFor([&] { return chat.rowCount() == before + 1; }, 3000));
    CHECK(role(chat, before, "content") == "Model nope is not installed. Open /models to install it.");
    CHECK(session->currentModel() == "demo:4b");
    s.ollama->onJson("POST", "/api/show", R"({"capabilities":["completion"]})");
    CHECK(chat.send("/model other:1b"));
    CHECK(QTest::qWaitFor([&] { return session->currentModel() == "other:1b"; }, 3000));

    CHECK(chat.send("/clear"));
    CHECK(chat.isNewChat() && chat.rowCount() == 0);
}

TEST(qtchat, without_ollama_or_model_the_reply_says_why)
{
    FakeSession s;
    auto* session = AppSessionWrap::instance();
    ChatWrap chat;
    CHECK(!chat.send("hi"));                               // no model chosen
    CHECK(role(chat, 0, "kind") == "notice");
    session->setCurrentModel("demo:4b");
    session->setServerUrl("http://127.0.0.1:9");
    CHECK(chat.send("hi"));
    CHECK(QTest::qWaitFor([&] { return !chat.busy(); }, 5000));
    CHECK(role(chat, chat.rowCount() - 1, "error") == "Ollama is not running at http://127.0.0.1:9");
    StatusStripWrap strip;
    CHECK(QTest::qWaitFor([&] { return strip.offline(); }, 5000));
    CHECK(strip.connectionText() == "Ollama not running");
}

TEST(qtchat, images_only_for_models_that_see)
{
    FakeSession s;
    serveDemo(s.ollama, R"(["completion","vision"])");
    AppSessionWrap::instance()->setCurrentModel("demo:4b");
    const QString png = s.folder->path() + "/pic.png";
    const QString txt = s.folder->path() + "/notes.txt";
    { QFile f(png); CHECK(f.open(QIODevice::WriteOnly)); f.write("\x89PNG\r\n"); }
    { QFile f(txt); CHECK(f.open(QIODevice::WriteOnly)); f.write("x"); }
    ChatWrap chat;
    CHECK(QTest::qWaitFor([&] { return chat.canAttach(); }, 3000));
    chat.attach({QUrl::fromLocalFile(png), QUrl::fromLocalFile(txt), QUrl::fromLocalFile(png)});
    CHECK(chat.pendingImages().size() == 1);
    CHECK(role(chat, chat.rowCount() - 1, "content").toString().startsWith("Not a PNG, JPEG or WebP image"));
    CHECK(chat.send("what is it?"));
    CHECK(chat.pendingImages().isEmpty());
    CHECK(QTest::qWaitFor([&] { return !chat.busy(); }, 3000));
    const auto body = lastBody(s.ollama, "/api/chat");
    CHECK(body["messages"][0]["images"][0].asString() == "iVBORw0K");
    int userRow = 0;
    while (role(chat, userRow, "kind") != "user") ++userRow;
    CHECK(role(chat, userRow, "images").toStringList() == QStringList{QUrl::fromLocalFile(png).toString()});
    CHECK(role(chat, userRow, "missingImages").toInt() == 0);
    QFile::remove(png);                                    // moved away after sending
    CHECK(role(chat, userRow, "images").toStringList().isEmpty() && role(chat, userRow, "missingImages").toInt() == 1);

    { QFile f(png); CHECK(f.open(QIODevice::WriteOnly)); f.write("\x89PNG\r\n"); }
    chat.attach({QUrl::fromLocalFile(png)});
    chat.removeAttachment(0);
    CHECK(chat.pendingImages().isEmpty());
}

TEST(qtchat, a_character_brings_its_greeting_and_system_prompt)
{
    FakeSession s;
    serveDemo(s.ollama, R"(["completion"])");
    AppSessionWrap::instance()->setCurrentModel("demo:4b");
    CharactersWrap characters;
    const int row = characters.create();
    CHECK(characters.save(row, "Pirate", "Talk like a pirate.", "Ahoy!", "", true, 0.9) >= 0);
    ChatWrap chat;
    CHECK(chat.characters().size() == 2);
    int pirate = -1;
    for (int i = 0; i < chat.characters().size(); ++i)
        if (chat.characters()[i].toMap()["name"] == "Pirate") pirate = i;
    chat.pickCharacter(pirate);
    CHECK(chat.characterIndex() == pirate);
    CHECK(chat.rowCount() == 1 && role(chat, 0, "content") == "Ahoy!");
    CHECK(chat.send("hello"));
    CHECK(QTest::qWaitFor([&] { return !chat.busy(); }, 3000));
    const auto body = lastBody(s.ollama, "/api/chat");
    CHECK(body["messages"][0]["role"].asString() == "system");
    CHECK(body["messages"][0]["content"].asString() == "Talk like a pirate.");
    CHECK(body["messages"][1]["content"].asString() == "Ahoy!");
    CHECK(body["options"]["temperature"].asNumber() == 0.9);

    chat.pickCharacter(0);                                 // Default mid-chat: no system prompt from now on
    CHECK(chat.send("again"));
    CHECK(QTest::qWaitFor([&] { return !chat.busy(); }, 3000));
    CHECK(lastBody(s.ollama, "/api/chat")["messages"][0]["role"].asString() == "assistant");
}
