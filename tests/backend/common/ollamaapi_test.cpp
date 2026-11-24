#include "testkit.h"
#include "common/json.h"
#include "common/ollamaapi.h"

TEST(ollamaapi, chat_request_has_messages_images_think_and_temperature)
{
    std::vector<ollama::ChatMessage> messages{
        {"system", "Be brief.", {}},
        {"user", "What is this?", {"aGk="}},
    };
    ollama::ChatOptions options;
    options.temperature = 0.7;
    options.think = true;
    const auto body = json::parse(ollama::chatRequest("llava:7b", messages, options));
    CHECK(body.has_value());
    CHECK((*body)["model"].asString() == "llava:7b");
    CHECK((*body)["stream"].asBool());
    CHECK((*body)["think"].asBool());
    CHECK_NEAR((*body)["options"]["temperature"].asNumber(), 0.7, 1e-12);
    CHECK((*body)["messages"].size() == 2);
    CHECK((*body)["messages"][0]["role"].asString() == "system");
    CHECK(!(*body)["messages"][0].contains("images"));
    CHECK((*body)["messages"][1]["images"][0].asString() == "aGk=");
}

TEST(ollamaapi, chat_request_leaves_out_unset_options)
{
    const auto body = json::parse(ollama::chatRequest("m", {{"user", "hi", {}}}, {}));
    CHECK(body.has_value());
    CHECK(!body->contains("think"));
    CHECK(!body->contains("options"));
}

TEST(ollamaapi, other_requests)
{
    CHECK(ollama::modelRequest("qwen3:8b") == R"({"model":"qwen3:8b"})");
    CHECK(ollama::pullRequest("hf.co/a/b:Q4_K_M") == R"({"model":"hf.co/a/b:Q4_K_M","stream":true})");
    CHECK(ollama::createRequest("mine", "x.gguf", "ab12") ==
          R"({"model":"mine","files":{"x.gguf":"sha256:ab12"},"stream":true})");
}

TEST(ollamaapi, parses_chat_stream_lines)
{
    auto c = ollama::parseChatLine(
        R"({"model":"m","created_at":"t","message":{"role":"assistant","content":"Hel","thinking":"hmm"},"done":false})");
    CHECK(c && c->content == "Hel" && c->thinking == "hmm" && !c->done);
    c = ollama::parseChatLine(
        R"({"model":"m","message":{"role":"assistant","content":""},"done":true,"done_reason":"stop","eval_count":42,"eval_duration":2100000000})");
    CHECK(c && c->done && c->doneReason == "stop" && c->evalCount == 42 && c->evalDurationNs == 2100000000LL);
    c = ollama::parseChatLine(R"({"error":"model \"x\" not found"})");
    CHECK(c && c->error == "model \"x\" not found");
    CHECK(!ollama::parseChatLine("not json"));
}

TEST(ollamaapi, parses_pull_and_create_progress)
{
    auto p = ollama::parseProgressLine(R"({"status":"pulling manifest"})");
    CHECK(p && p->status == "pulling manifest" && p->total == 0 && !p->success());
    p = ollama::parseProgressLine(
        R"({"status":"pulling 6a0746a1ec1a","digest":"sha256:6a07","total":4661211424,"completed":1234567})");
    CHECK(p && p->digest == "sha256:6a07" && p->total == 4661211424LL && p->completed == 1234567);
    p = ollama::parseProgressLine(R"({"status":"success"})");
    CHECK(p && p->success());
    p = ollama::parseProgressLine(R"({"error":"pull model manifest: file does not exist"})");
    CHECK(p && p->error == "pull model manifest: file does not exist");
}

TEST(ollamaapi, parses_tags_show_version_and_errors)
{
    const auto tags = ollama::parseTags(R"({"models":[
        {"name":"magnum12b:latest","model":"magnum12b:latest","size":7477000000,
         "details":{"format":"gguf","family":"llama","parameter_size":"12.2B","quantization_level":"Q4_K_M"}},
        {"model":"other:1b","size":1}]})");
    CHECK(tags && tags->size() == 2);
    CHECK((*tags)[0].name == "magnum12b:latest" && (*tags)[0].sizeBytes == 7477000000LL);
    CHECK((*tags)[0].parameterSize == "12.2B" && (*tags)[0].quantization == "Q4_K_M" && (*tags)[0].family == "llama");
    CHECK((*tags)[1].name == "other:1b");
    CHECK(ollama::parseTags(R"({"models":[]})")->empty());
    CHECK(!ollama::parseTags("{}"));

    const auto info = ollama::parseShow(R"({"license":"x","capabilities":["completion","vision"]})");
    CHECK(info && info->has("vision") && !info->has("thinking"));
    CHECK(!ollama::parseShow(R"({"error":"not found"})"));

    CHECK(ollama::parseVersion(R"({"version":"0.40.16"})") == std::optional<std::string>("0.40.16"));
    CHECK(!ollama::parseVersion("<html>"));
    CHECK(ollama::errorText(R"({"error":"boom"})") == "boom");
    CHECK(ollama::errorText("<html>").empty());
}

TEST(ollamaapi, line_buffer_joins_lines_split_across_reads)
{
    ollama::LineBuffer buffer;
    CHECK(buffer.feed("{\"a\":").empty());
    auto lines = buffer.feed("1}\r\n{\"b\"");
    CHECK(lines.size() == 1 && lines[0] == "{\"a\":1}");
    lines = buffer.feed(":2}\n\n{\"c\":3}\n");
    CHECK(lines.size() == 2 && lines[0] == "{\"b\":2}" && lines[1] == "{\"c\":3}");
    CHECK(buffer.feed("{\"d\":4}").empty());
    CHECK(buffer.takeRest() == "{\"d\":4}");
    CHECK(buffer.takeRest().empty());
}

TEST(ollamaapi, pull_totals_add_up_the_layers)
{
    ollama::PullTotals totals;
    CHECK(totals.fraction() == 0.0);
    totals.add(*ollama::parseProgressLine(R"({"status":"pulling manifest"})"));
    totals.add(*ollama::parseProgressLine(R"({"status":"pulling a","digest":"sha256:a","total":900,"completed":0})"));
    totals.add(*ollama::parseProgressLine(R"({"status":"pulling b","digest":"sha256:b","total":100,"completed":100})"));
    totals.add(*ollama::parseProgressLine(R"({"status":"pulling a","digest":"sha256:a","total":900,"completed":400})"));
    CHECK(totals.total() == 1000 && totals.completed() == 500);
    CHECK_NEAR(totals.fraction(), 0.5, 1e-12);
    totals.add(*ollama::parseProgressLine(R"({"status":"pulling a","digest":"sha256:a","total":900,"completed":5000})"));
    CHECK(totals.completed() == 1000);
}
