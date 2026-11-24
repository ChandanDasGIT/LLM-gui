#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

// A small JSON value with a parser and a writer (RFC 8259). Objects keep their keys in the
// order they were added; numbers are doubles, which hold every integer Ollama sends exactly.
namespace json {

class Value {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };

    Value() = default;
    Value(std::nullptr_t) {}
    Value(bool b) : type_(Type::Bool), bool_(b) {}
    template <typename T>
        requires(std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
    Value(T n) : type_(Type::Number), number_(double(n)) {}
    Value(const char* s) : type_(Type::String), string_(s) {}
    Value(std::string s) : type_(Type::String), string_(std::move(s)) {}
    Value(std::string_view s) : type_(Type::String), string_(s) {}

    static Value array();
    static Value object();

    Type type() const { return type_; }
    bool isNull() const { return type_ == Type::Null; }
    bool isBool() const { return type_ == Type::Bool; }
    bool isNumber() const { return type_ == Type::Number; }
    bool isString() const { return type_ == Type::String; }
    bool isArray() const { return type_ == Type::Array; }
    bool isObject() const { return type_ == Type::Object; }

    // The value, or the fallback when it is of another type.
    bool asBool(bool fallback = false) const { return isBool() ? bool_ : fallback; }
    double asNumber(double fallback = 0.0) const { return isNumber() ? number_ : fallback; }
    long long asInt(long long fallback = 0) const { return isNumber() ? (long long)number_ : fallback; }
    // Empty when not a string.
    const std::string& asString() const;

    // Arrays and objects: the number of items or members; 0 for anything else.
    std::size_t size() const;
    // Array item; a null value when out of range or not an array.
    const Value& operator[](std::size_t index) const;
    const Value& operator[](int index) const { return (*this)[std::size_t(index)]; }
    // Object member; a null value when missing or not an object.
    const Value& operator[](std::string_view key) const;
    const Value& operator[](const char* key) const { return (*this)[std::string_view(key)]; }
    bool contains(std::string_view key) const;
    const std::vector<std::string>& keys() const { return keys_; }
    const std::vector<Value>& items() const { return items_; }

    // Turns a null value into an array/object as needed; replaces a member with the same key.
    Value& push(Value item);
    Value& set(std::string_view key, Value item);

    bool operator==(const Value& other) const;

private:
    Type type_ = Type::Null;
    bool bool_ = false;
    double number_ = 0.0;
    std::string string_;
    std::vector<std::string> keys_;   // objects: keys_[i] names items_[i]
    std::vector<Value> items_;        // array items or object values
};

// nullopt for text that is not exactly one JSON value; *error then says why and where.
std::optional<Value> parse(std::string_view text, std::string* error = nullptr);
std::string write(const Value& value);

} // namespace json
