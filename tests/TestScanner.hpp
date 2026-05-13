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
        EXPECT_CALL(*pMockComm, sendCommand("EMAC")).WillOnce(Return("OK,EMAC,001122334455"));
        pScanner = std::make_unique<Scanner>(*pMockComm);
    }
    void TearDown() override {}

    std::string testCheckResponse(const std::string& response) {
        return pScanner->checkResponse(response);
    }

    /**
     * @brief Set up EXPECT_CALLs for parseReadResult with all appending disabled.
     *
     * parseReadResult queries the inter-delimiter and each appending flag.
     * This helper expects all of them and returns "disabled" defaults.
     */
    void expectParseReadDefaults() {
        // Inter-delimiter (RP,602) -> "2C" (comma)
        EXPECT_CALL(*pMockComm, sendCommand("RP,602")).WillOnce(Return("OK,RP,2C"));
        // All appending flags disabled (Toggle 0 = DISABLE)
        EXPECT_CALL(*pMockComm, sendCommand("RP,308")).WillOnce(Return("OK,RP,0"));
        EXPECT_CALL(*pMockComm, sendCommand("RP,309")).WillOnce(Return("OK,RP,0"));
        EXPECT_CALL(*pMockComm, sendCommand("RP,301")).WillOnce(Return("OK,RP,0"));
        EXPECT_CALL(*pMockComm, sendCommand("RP,303")).WillOnce(Return("OK,RP,0"));
        EXPECT_CALL(*pMockComm, sendCommand("RP,371")).WillOnce(Return("OK,RP,0"));
    }

    std::unique_ptr<StrictMock<MockCommInterface>> pMockComm;
    std::unique_ptr<Scanner> pScanner;
};

}  // namespace OpenSRX
