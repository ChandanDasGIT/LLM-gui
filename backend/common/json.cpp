#include "common/json.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace json {

namespace {

const Value& nullValue()
{
    static const Value value;
    return value;
}

class Parser {
public:
    explicit Parser(std::string_view text) : text_(text) {}

    std::optional<Value> parseDocument(std::string* error)
    {
        Value value;
        skipSpace();
        if (!parseValue(value, 0)) return fail(error);
        skipSpace();
        if (pos_ != text_.size()) {
            message_ = "unexpected text after the value";
            return fail(error);
        }
        return value;
    }

private:
    static constexpr int maxDepth = 512;

    std::optional<Value> fail(std::string* error)
    {
        if (error) *error = message_ + " at offset " + std::to_string(pos_);
        return std::nullopt;
    }

    bool failWith(const char* message)
    {
        if (message_.empty()) message_ = message;
        return false;
    }

    void skipSpace()
    {
        while (pos_ < text_.size()) {
            const char c = text_[pos_];
            if (c != ' ' && c != '\t' && c != '\n' && c != '\r') break;
            ++pos_;
        }
    }

    bool literal(std::string_view word)
    {
        if (text_.substr(pos_, word.size()) != word) return failWith("invalid literal");
        pos_ += word.size();
        return true;
    }

    bool parseValue(Value& out, int depth)
    {
        if (depth > maxDepth) return failWith("nested too deeply");
        if (pos_ >= text_.size()) return failWith("unexpected end of text");
        switch (text_[pos_]) {
        case 'n': out = Value(); return literal("null");
        case 't': out = Value(true); return literal("true");
        case 'f': out = Value(false); return literal("false");
        case '"': {
            std::string s;
            if (!parseString(s)) return false;
            out = Value(std::move(s));
            return true;
        }
        case '[': return parseArray(out, depth);
        case '{': return parseObject(out, depth);
        default: return parseNumber(out);
        }
    }

    bool parseArray(Value& out, int depth)
    {
        out = Value::array();
        ++pos_;
        skipSpace();
        if (pos_ < text_.size() && text_[pos_] == ']') { ++pos_; return true; }
        while (true) {
            Value item;
            skipSpace();
            if (!parseValue(item, depth + 1)) return false;
            out.push(std::move(item));
            skipSpace();
            if (pos_ >= text_.size()) return failWith("unexpected end of text in array");
            if (text_[pos_] == ',') { ++pos_; continue; }
            if (text_[pos_] == ']') { ++pos_; return true; }
            return failWith("expected , or ] in array");
        }
    }

    bool parseObject(Value& out, int depth)
    {
        out = Value::object();
        ++pos_;
        skipSpace();
        if (pos_ < text_.size() && text_[pos_] == '}') { ++pos_; return true; }
        while (true) {
            skipSpace();
            if (pos_ >= text_.size() || text_[pos_] != '"') return failWith("expected a key in object");
            std::string key;
            if (!parseString(key)) return false;
            skipSpace();
            if (pos_ >= text_.size() || text_[pos_] != ':') return failWith("expected : in object");
            ++pos_;
            skipSpace();
            Value item;
            if (!parseValue(item, depth + 1)) return false;
            out.set(key, std::move(item));
            skipSpace();
            if (pos_ >= text_.size()) return failWith("unexpected end of text in object");
            if (text_[pos_] == ',') { ++pos_; continue; }
            if (text_[pos_] == '}') { ++pos_; return true; }
            return failWith("expected , or } in object");
        }
    }

    bool hex4(unsigned& out)
    {
        if (pos_ + 4 > text_.size()) return failWith("short \\u escape");
        out = 0;
        for (int i = 0; i < 4; ++i) {
            const char c = text_[pos_++];
            out <<= 4;
            if (c >= '0' && c <= '9') out |= unsigned(c - '0');
            else if (c >= 'a' && c <= 'f') out |= unsigned(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') out |= unsigned(c - 'A' + 10);
            else return failWith("bad \\u escape");
        }
        return true;
    }

    static void appendUtf8(std::string& s, unsigned cp)
    {
        if (cp < 0x80) {
            s += char(cp);
        } else if (cp < 0x800) {
            s += char(0xC0 | (cp >> 6));
            s += char(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            s += char(0xE0 | (cp >> 12));
            s += char(0x80 | ((cp >> 6) & 0x3F));
            s += char(0x80 | (cp & 0x3F));
        } else {
            s += char(0xF0 | (cp >> 18));
            s += char(0x80 | ((cp >> 12) & 0x3F));
            s += char(0x80 | ((cp >> 6) & 0x3F));
            s += char(0x80 | (cp & 0x3F));
        }
    }

    bool parseString(std::string& out)
    {
        ++pos_;   // the opening quote
        while (true) {
            if (pos_ >= text_.size()) return failWith("unterminated string");
            const char c = text_[pos_++];
            if (c == '"') return true;
            if ((unsigned char)c < 0x20) return failWith("control character in string");
            if (c != '\\') { out += c; continue; }
            if (pos_ >= text_.size()) return failWith("unterminated string");
            switch (text_[pos_++]) {
            case '"': out += '"'; break;
            case '\\': out += '\\'; break;
            case '/': out += '/'; break;
            case 'b': out += '\b'; break;
            case 'f': out += '\f'; break;
            case 'n': out += '\n'; break;
            case 'r': out += '\r'; break;
            case 't': out += '\t'; break;
            case 'u': {
                unsigned cp = 0;
                if (!hex4(cp)) return false;
                if (cp >= 0xD800 && cp <= 0xDBFF) {
                    unsigned low = 0;
                    if (text_.substr(pos_, 2) != "\\u") return failWith("lone high surrogate");
                    pos_ += 2;
                    if (!hex4(low)) return false;
                    if (low < 0xDC00 || low > 0xDFFF) return failWith("bad low surrogate");
                    cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
                    return failWith("lone low surrogate");
                }
                appendUtf8(out, cp);
                break;
            }
            default: return failWith("bad escape in string");
            }
        }
    }

    bool parseNumber(Value& out)
    {
        const std::size_t start = pos_;
        auto digits = [&] {
            const std::size_t from = pos_;
            while (pos_ < text_.size() && text_[pos_] >= '0' && text_[pos_] <= '9') ++pos_;
            return pos_ > from;
        };
        if (pos_ < text_.size() && text_[pos_] == '-') ++pos_;
        if (pos_ < text_.size() && text_[pos_] == '0') ++pos_;
        else if (!digits()) return failWith("invalid value");
        if (pos_ < text_.size() && text_[pos_] == '.') {
            ++pos_;
            if (!digits()) return failWith("digits expected after the decimal point");
        }
        if (pos_ < text_.size() && (text_[pos_] == 'e' || text_[pos_] == 'E')) {
            ++pos_;
            if (pos_ < text_.size() && (text_[pos_] == '+' || text_[pos_] == '-')) ++pos_;
            if (!digits()) return failWith("digits expected in the exponent");
        }
        const std::string number(text_.substr(start, pos_ - start));
        out = Value(std::strtod(number.c_str(), nullptr));
        return true;
    }

    std::string_view text_;
    std::size_t pos_ = 0;
    std::string message_;
};

void writeString(std::string& out, const std::string& s)
{
    out += '"';
    for (const char c : s) {
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if ((unsigned char)c < 0x20) {
                char buffer[8];
                std::snprintf(buffer, sizeof buffer, "\\u%04x", unsigned((unsigned char)c));
                out += buffer;
            } else {
                out += c;
            }
        }
    }
    out += '"';
}

void writeValue(std::string& out, const Value& v)
{
    switch (v.type()) {
    case Value::Type::Null: out += "null"; break;
    case Value::Type::Bool: out += v.asBool() ? "true" : "false"; break;
    case Value::Type::Number: {
        const double n = v.asNumber();
        char buffer[32];
        if (!std::isfinite(n)) out += "null";   // JSON has no NaN or infinity
        else if (n == std::floor(n) && std::fabs(n) < 9.007199254740992e15) {
            std::snprintf(buffer, sizeof buffer, "%lld", (long long)n);
            out += buffer;
        } else {
            std::snprintf(buffer, sizeof buffer, "%.17g", n);
            out += buffer;
        }
        break;
    }
    case Value::Type::String: writeString(out, v.asString()); break;
    case Value::Type::Array:
        out += '[';
        for (std::size_t i = 0; i < v.size(); ++i) {
            if (i) out += ',';
            writeValue(out, v[i]);
        }
        out += ']';
        break;
    case Value::Type::Object:
        out += '{';
        for (std::size_t i = 0; i < v.keys().size(); ++i) {
            if (i) out += ',';
            writeString(out, v.keys()[i]);
            out += ':';
            writeValue(out, v.items()[i]);
        }
        out += '}';
        break;
    }
}

} // namespace

Value Value::array()
{
    Value v;
    v.type_ = Type::Array;
    return v;
}

Value Value::object()
{
    Value v;
    v.type_ = Type::Object;
    return v;
}

const std::string& Value::asString() const
{
    static const std::string empty;
    return isString() ? string_ : empty;
}

std::size_t Value::size() const
{
    return isArray() || isObject() ? items_.size() : 0;
}

const Value& Value::operator[](std::size_t index) const
{
    return isArray() && index < items_.size() ? items_[index] : nullValue();
}

const Value& Value::operator[](std::string_view key) const
{
    if (isObject())
        for (std::size_t i = 0; i < keys_.size(); ++i)
            if (keys_[i] == key) return items_[i];
    return nullValue();
}

bool Value::contains(std::string_view key) const
{
    if (isObject())
        for (const auto& k : keys_)
            if (k == key) return true;
    return false;
}

Value& Value::push(Value item)
{
    if (isNull()) type_ = Type::Array;
    if (isArray()) items_.push_back(std::move(item));
    return *this;
}

Value& Value::set(std::string_view key, Value item)
{
    if (isNull()) type_ = Type::Object;
    if (!isObject()) return *this;
    for (std::size_t i = 0; i < keys_.size(); ++i)
        if (keys_[i] == key) {
            items_[i] = std::move(item);
            return *this;
        }
    keys_.emplace_back(key);
    items_.push_back(std::move(item));
    return *this;
}

bool Value::operator==(const Value& other) const
{
    if (type_ != other.type_) return false;
    switch (type_) {
    case Type::Null: return true;
    case Type::Bool: return bool_ == other.bool_;
    case Type::Number: return number_ == other.number_;
    case Type::String: return string_ == other.string_;
    case Type::Array: return items_ == other.items_;
    case Type::Object: return keys_ == other.keys_ && items_ == other.items_;
    }
    return false;
}

std::optional<Value> parse(std::string_view text, std::string* error)
{
    return Parser(text).parseDocument(error);
}

std::string write(const Value& value)
{
    std::string out;
    writeValue(out, value);
    return out;
}

} // namespace json
