#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MockCommInterface.hpp"

using ::testing::_;
using ::testing::DoAll;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrictMock;

class TestICommInterface : public ::testing::Test {
   protected:
    void SetUp() override {
        pMockComm = std::make_unique<StrictMock<OpenSRX::MockCommInterface>>();
    }
    void TearDown() override {}

    std::unique_ptr<StrictMock<OpenSRX::MockCommInterface>> pMockComm;
};
