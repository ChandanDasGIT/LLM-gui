#include "testkit.h"
#include "common/json.h"

TEST(json, parses_every_kind_of_value)
{
    const auto v = json::parse(R"( {"a": 1, "b": [true, false, null], "c": "hi", "d": -2.5e2, "e": {}} )");
    CHECK(v.has_value());
    CHECK((*v)["a"].asInt() == 1);
    CHECK((*v)["b"].size() == 3);
    CHECK((*v)["b"][0].asBool() == true);
    CHECK((*v)["b"][1].isBool() && !(*v)["b"][1].asBool());
    CHECK((*v)["b"][2].isNull());
    CHECK((*v)["c"].asString() == "hi");
    CHECK_NEAR((*v)["d"].asNumber(), -250.0, 1e-12);
    CHECK((*v)["e"].isObject() && (*v)["e"].size() == 0);
    CHECK((*v)["missing"].isNull());
    CHECK((*v)["b"][7].isNull());
    CHECK((*v)["c"].asInt(42) == 42);
}

TEST(json, decodes_escapes_and_surrogate_pairs_to_utf8)
{
    const auto v = json::parse(R"("q\" b\\ s\/ \n\t é € 😀")");
    CHECK(v.has_value());
    CHECK(v->asString() == "q\" b\\ s/ \n\t \xC3\xA9 \xE2\x82\xAC \xF0\x9F\x98\x80");
}

TEST(json, refuses_bad_text_and_says_where)
{
    std::string error;
    CHECK(!json::parse("{\"a\": }", &error).has_value());
    CHECK(error.find("offset") != std::string::npos);
    CHECK(!json::parse("[1, 2").has_value());
    CHECK(!json::parse("tru").has_value());
    CHECK(!json::parse("01").has_value());
    CHECK(!json::parse("1.").has_value());
    CHECK(!json::parse("\"a\nb\"").has_value());           // raw control character
    CHECK(!json::parse(R"("\ud83d")").has_value());        // lone surrogate
    CHECK(!json::parse("{} {}").has_value());
    CHECK(!json::parse("").has_value());
    CHECK(!json::parse(std::string(2000, '[')).has_value());
}

TEST(json, writes_what_it_reads)
{
    json::Value v = json::Value::object();
    v.set("name", "a \"b\"\n\x01");
    v.set("n", 12);
    v.set("big", 123456789012LL);
    v.set("half", 0.5);
    v.set("list", json::Value::array().push(true).push(nullptr));
    v.set("n", 13);                                         // replaces, keeps the order
    const std::string text = json::write(v);
    CHECK(text == R"({"name":"a \"b\"\n\u0001","n":13,"big":123456789012,"half":0.5,"list":[true,null]})");
    const auto back = json::parse(text);
    CHECK(back.has_value() && *back == v);
}

TEST(json, keeps_utf8_text_as_it_is)
{
    const std::string text = "\"caf\xC3\xA9 \xE2\x82\xAC\"";
    const auto v = json::parse(text);
    CHECK(v.has_value() && json::write(*v) == text);
}
