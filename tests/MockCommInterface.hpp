#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "OpenSRX/ICommInterface.hpp"

class MockCommInterface : public OpenSRX::ICommInterface {
public:
MockCommInterface() = default;
~MockCommInterface() override = default;
MOCK_METHOD(std::string, sendCommand, (const std::string& command), (override));

};