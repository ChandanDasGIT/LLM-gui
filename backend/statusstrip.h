#pragma once

#include <string>

// The texts of the status strip: model, character, connection and generation speed.
namespace statusstrip {

enum class Connection { Checking, Connected, Offline };

std::string modelText(const std::string& model);
std::string characterText(const std::string& name);
std::string connectionText(Connection connection, const std::string& version);

// Tokens per second from the last reply's eval_count and eval_duration; 0 when unknown.
double tokensPerSecond(long long evalCount, long long evalDurationNs);
// "23.4 tok/s", or empty when unknown.
std::string tokensPerSecondText(long long evalCount, long long evalDurationNs);

} // namespace statusstrip
