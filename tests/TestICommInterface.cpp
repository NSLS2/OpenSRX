#include "TestICommInterface.hpp"

TEST_F(TestICommInterface, TestStripEcho) {
    std::string command = "TEST_COMMAND";
    std::string response = "TEST_COMMAND,Version 1.2.3";
    std::string expected = "Version 1.2.3";
    pMockComm->testStripEcho(response, command, expected);

    command = "ANOTHER_COMMAND";
    response = "Version 4.5.6";
    expected = "Version 4.5.6";
    pMockComm->testStripEcho(response, command, expected);
}

TEST_F(TestICommInterface, TestSetCommFormat) {
    ASSERT_EQ(pMockComm->getInTermStr(), "\r");  // Default should be NO_HEADER_CR_IN_CR_OUT
    ASSERT_EQ(pMockComm->getCommFormat(), OpenSRX::CommFormat::NO_HEADER_CR_IN_CR_OUT);

    pMockComm->setCommFormat(OpenSRX::CommFormat::NO_HEADER_CRLF_IN_CR_OUT);
    ASSERT_EQ(pMockComm->getInTermStr(), "\r\n");
    ASSERT_EQ(pMockComm->getCommFormat(), OpenSRX::CommFormat::NO_HEADER_CRLF_IN_CR_OUT);

    pMockComm->setCommFormat(OpenSRX::CommFormat::STX_HEADER_ETX_IN_ETX_OUT);
    ASSERT_EQ(pMockComm->getInTermStr(), "\x03");
    ASSERT_EQ(pMockComm->getCommFormat(), OpenSRX::CommFormat::STX_HEADER_ETX_IN_ETX_OUT);
}

TEST_F(TestICommInterface, TestAddHeaderAndTerminator) {
    std::string command = "TEST_COMMAND";
    std::string expected = "\x02TEST_COMMAND\x03";
    pMockComm->setCommFormat(OpenSRX::CommFormat::STX_HEADER_ETX_IN_ETX_OUT);
    pMockComm->testAddHeaderAndTerminator(command, expected);

    expected = "TEST_COMMAND\r";
    pMockComm->setCommFormat(OpenSRX::CommFormat::NO_HEADER_CR_IN_CR_OUT);
    pMockComm->testAddHeaderAndTerminator(command, expected);
    pMockComm->setCommFormat(OpenSRX::CommFormat::NO_HEADER_CRLF_IN_CR_OUT);
    pMockComm->testAddHeaderAndTerminator(command, expected);
}
