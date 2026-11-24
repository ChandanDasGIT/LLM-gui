#include "testkit.h"
#include "modelManager/sha256.h"

namespace {
std::string sha(std::string_view s)
{
    Sha256 h;
    h.update(s);
    return h.finishHex();
}
}

TEST(sha256, fips_180_test_vectors)
{
    CHECK(sha("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    CHECK(sha("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    CHECK(sha("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq")
          == "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    CHECK(sha("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu")
          == "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1");
}

TEST(sha256, a_million_a_fed_in_odd_pieces)
{
    Sha256 h;
    const std::string piece(997, 'a');
    std::size_t left = 1'000'000;
    while (left > 0) {
        const std::size_t n = std::min(left, piece.size());
        h.update(piece.data(), n);
        left -= n;
    }
    CHECK(h.finishHex() == "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
}

TEST(sha256, piece_boundaries_do_not_change_the_digest)
{
    std::string text;
    for (int i = 0; i < 300; ++i) text += char(i * 7);
    const std::string whole = sha(text);
    for (std::size_t cut : {1u, 55u, 56u, 63u, 64u, 65u, 128u}) {
        Sha256 h;
        h.update(std::string_view(text).substr(0, cut));
        h.update(std::string_view(text).substr(cut));
        CHECK(h.finishHex() == whole);
    }
    CHECK(Sha256::hex(std::string("\x00\xff\x10", 3)) == "00ff10");
}
