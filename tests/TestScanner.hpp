#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MockCommInterface.hpp"
#include "OpenSRX/Scanner.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::StrictMock;

class TestScanner : public ::testing::Test {
   protected:
    void SetUp() override {
        pMockComm = std::make_unique<StrictMock<OpenSRX::MockCommInterface>>();
        // Scanner constructor calls describe() and sends "KEYENCE" command
        EXPECT_CALL(*pMockComm, describe()).WillRepeatedly(Return("mock://scanner"));
        EXPECT_CALL(*pMockComm, sendCommand("KEYENCE"))
            .WillOnce(Return("SR-X300,V1.2.3"));
        pScanner = std::make_unique<OpenSRX::Scanner>(*pMockComm);
    }
    void TearDown() override {}

    std::unique_ptr<StrictMock<OpenSRX::MockCommInterface>> pMockComm;
    std::unique_ptr<OpenSRX::Scanner> pScanner;
};
