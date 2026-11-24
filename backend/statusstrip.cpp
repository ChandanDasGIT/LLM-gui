#include "statusstrip.h"

#include <cstdio>

namespace statusstrip {

std::string modelText(const std::string& model)
{
    return model.empty() ? "No model chosen" : model;
}

std::string characterText(const std::string& name)
{
    return name.empty() ? "Default" : name;
}

std::string connectionText(Connection connection, const std::string& version)
{
    switch (connection) {
    case Connection::Checking: return "Connecting to Ollama…";
    case Connection::Connected: return version.empty() ? "Ollama connected" : "Ollama " + version;
    case Connection::Offline: return "Ollama not running";
    }
    return {};
}

double tokensPerSecond(long long evalCount, long long evalDurationNs)
{
    if (evalCount <= 0 || evalDurationNs <= 0) return 0.0;
    return double(evalCount) * 1e9 / double(evalDurationNs);
}

std::string tokensPerSecondText(long long evalCount, long long evalDurationNs)
{
    const double rate = tokensPerSecond(evalCount, evalDurationNs);
    if (rate <= 0.0) return {};
    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "%.1f tok/s", rate);
    return buffer;
}

} // namespace statusstrip
