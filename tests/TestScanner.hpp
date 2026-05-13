#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MockCommInterface.hpp"
#include "OpenSRX/Scanner.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::StrictMock;

namespace OpenSRX {

class TestScanner : public ::testing::Test {
   protected:
    void SetUp() override {
        pMockComm = std::make_unique<StrictMock<MockCommInterface>>();
        // Scanner constructor calls describe(), KEYENCE, and EMAC
        EXPECT_CALL(*pMockComm, describe()).WillRepeatedly(Return("mock://scanner"));
        EXPECT_CALL(*pMockComm, sendCommand("KEYENCE"))
            .WillOnce(Return("OK,KEYENCE,SR-X300,V1.2.3"));
        EXPECT_CALL(*pMockComm, sendCommand("EMAC"))
            .WillOnce(Return("OK,EMAC,001122334455"));
        pScanner = std::make_unique<Scanner>(*pMockComm);
    }
    void TearDown() override {}

    std::string testCheckResponse(const std::string& response) {
        return pScanner->checkResponse(response);
    }

    std::unique_ptr<StrictMock<MockCommInterface>> pMockComm;
    std::unique_ptr<Scanner> pScanner;
};

}  // namespace OpenSRX
