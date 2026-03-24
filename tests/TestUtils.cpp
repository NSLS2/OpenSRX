#include <gtest/gtest.h>
#include <OpenSRX/Utils.hpp>

TEST(TestUtils, StripEcho) {
    std::string command = "TEST_COMMAND\r";
    std::string response = "TEST_COMMAND,Version 1.2.3\r";
    std::string expected = "Version 1.2.3\r";
    EXPECT_EQ(stripEcho(response, command), expected);

    command = "ANOTHER_COMMAND\r";
    response = "Version 4.5.6\r";
    expected = "Version 4.5.6\r";
    EXPECT_EQ(stripEcho(response, command), expected);
}