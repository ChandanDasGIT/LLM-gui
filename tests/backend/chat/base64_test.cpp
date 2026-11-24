#include "testkit.h"
#include "chat/base64.h"

TEST(base64, rfc4648_test_vectors)
{
    CHECK(base64Encode("") == "");
    CHECK(base64Encode("f") == "Zg==");
    CHECK(base64Encode("fo") == "Zm8=");
    CHECK(base64Encode("foo") == "Zm9v");
    CHECK(base64Encode("foob") == "Zm9vYg==");
    CHECK(base64Encode("fooba") == "Zm9vYmE=");
    CHECK(base64Encode("foobar") == "Zm9vYmFy");
}

TEST(base64, binary_bytes)
{
    CHECK(base64Encode(std::string("\x00\xff\xfe\x80", 4)) == "AP/+gA==");
    CHECK(base64Encode("\x89PNG\r\n") == "iVBORw0K");
}
