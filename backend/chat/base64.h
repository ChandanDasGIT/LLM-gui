#pragma once

#include <string>
#include <string_view>

// Standard Base64 (RFC 4648, with padding), for the images sent with a prompt.
std::string base64Encode(std::string_view bytes);
