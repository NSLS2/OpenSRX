#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "OpenSRX/ICommInterface.hpp"

// Put it in the OpenSRX namespace so we don't need to have the
// friend class declaration use the global namespace qualifier.
namespace OpenSRX {

class MockCommInterface : public OpenSRX::ICommInterface {
   public:
    MockCommInterface() = default;
    ~MockCommInterface() override = default;
    MOCK_METHOD(std::string, sendCommand, (const std::string& command), (override));
    MOCK_METHOD(std::string, sendCommandUnlocked, (const std::string& command), (override));
    MOCK_METHOD(std::string, describe, (), (const, override));

    void testStripEcho(const std::string& response, const std::string& command,
                       const std::string& expected) {
        ASSERT_EQ(this->stripEcho(response, command), expected);
    }

    void testAddHeaderAndTerminator(const std::string& command, const std::string& expected) {
        ASSERT_EQ(this->addHeaderAndTerminator(command), expected);
    }

    std::string getInTermStr() { return this->inTermStr; }
};

}  // namespace OpenSRX
