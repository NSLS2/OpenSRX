#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "OpenSRX/API.hpp"

using ::testing::HasSubstr;

TEST(AppTest, ReturnsProjectName) { EXPECT_THAT(OpenSRX::app_name(), HasSubstr("OpenSRX")); }

TEST(VersionTest, ExposesVersionProgrammatically) {
    EXPECT_EQ(OpenSRX::GetVersion<std::string>(), "0.1.0");
    EXPECT_EQ(OpenSRX::GetVersion<OpenSRX::VersionTuple>(), OpenSRX::VersionTuple(0, 1, 0));
}
