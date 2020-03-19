#include <crypto.hpp>
#include "catch2/catch.hpp"

TEST_CASE("Crypto functions")
{
        SECTION("password generation") {
                CHECK(verify_password(generate_password("foobar"), "foobar") == true);
        }
}