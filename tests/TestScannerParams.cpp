#include "TestScanner.hpp"

// ─── Constructor tests ──────────────────────────────────────────────────────

TEST_F(TestScanner, ConstructorParsesModelAndFirmware) {
    EXPECT_EQ(pScanner->getModel(), "SR-X300");
    EXPECT_EQ(pScanner->getFirmwareVersion(), "V1.2.3");
}

TEST(TestScannerStandalone, ConstructorThrowsOnInvalidVersionInfo) {
    StrictMock<OpenSRX::MockCommInterface> mockComm;
    EXPECT_CALL(mockComm, describe()).WillRepeatedly(Return("mock://scanner"));
    EXPECT_CALL(mockComm, sendCommand("KEYENCE")).WillOnce(Return("InvalidNoComma"));
    EXPECT_THROW(OpenSRX::Scanner scanner(mockComm), std::runtime_error);
}

// ─── Bank parameter (RB/WB) tests ──────────────────────────────────────────

TEST_F(TestScanner, GetBankParamInt) {
    // EXPOSURE_TIME = 100, bank 1 -> "RB,01100"
    EXPECT_CALL(*pMockComm, sendCommand("RB,01100")).WillOnce(Return("OK,RB,500"));
    int val = pScanner->getParam<OpenSRX::BankParam::EXPOSURE_TIME>(1);
    EXPECT_EQ(val, 500);
}

TEST_F(TestScanner, SetBankParamInt) {
    // EXPOSURE_TIME = 100, bank 1, value 500 -> "WB,01100,500"
    EXPECT_CALL(*pMockComm, sendCommand("WB,01100,500")).WillOnce(Return("OK,WB"));
    pScanner->setParam<OpenSRX::BankParam::EXPOSURE_TIME>(1, 500);
}

TEST_F(TestScanner, GetBankParamToggle) {
    // INTERNAL_LIGHTING_USE = 0, bank 3 -> "RB,03000"
    EXPECT_CALL(*pMockComm, sendCommand("RB,03000")).WillOnce(Return("OK,RB,1"));
    auto val = pScanner->getParam<OpenSRX::BankParam::INTERNAL_LIGHTING_USE>(3);
    EXPECT_EQ(val, OpenSRX::Toggle::ENABLE);
}

TEST_F(TestScanner, SetBankParamToggle) {
    // INTERNAL_LIGHTING_USE = 0, bank 3, DISABLE -> "WB,03000,0"
    EXPECT_CALL(*pMockComm, sendCommand("WB,03000,0")).WillOnce(Return("OK,WB"));
    pScanner->setParam<OpenSRX::BankParam::INTERNAL_LIGHTING_USE>(3, OpenSRX::Toggle::DISABLE);
}

TEST_F(TestScanner, GetBankParamAscii) {
    // BANK_NAME = 625, bank 1 -> "RB,01625"
    EXPECT_CALL(*pMockComm, sendCommand("RB,01625")).WillOnce(Return("OK,RB,MyBank"));
    std::string val = pScanner->getParam<OpenSRX::BankParam::BANK_NAME>(1);
    EXPECT_EQ(val, "MyBank");
}

TEST_F(TestScanner, SetBankParamAscii) {
    // BANK_NAME = 625, bank 2, "TestBank" -> "WB,02625,TestBank"
    EXPECT_CALL(*pMockComm, sendCommand("WB,02625,TestBank")).WillOnce(Return("OK,WB"));
    pScanner->setParam<OpenSRX::BankParam::BANK_NAME>(2, std::string("TestBank"));
}

TEST_F(TestScanner, GetBankParamHex) {
    // CODE_TYPE = 300, bank 1 -> "RB,01300"
    EXPECT_CALL(*pMockComm, sendCommand("RB,01300")).WillOnce(Return("OK,RB,0F"));
    std::string val = pScanner->getParam<OpenSRX::BankParam::CODE_TYPE>(1);
    EXPECT_EQ(val, "0F");
}

TEST_F(TestScanner, SetBankParamHex) {
    // CODE_TYPE = 300, bank 1, "FF" -> "WB,01300,FF"
    EXPECT_CALL(*pMockComm, sendCommand("WB,01300,FF")).WillOnce(Return("OK,WB"));
    pScanner->setParam<OpenSRX::BankParam::CODE_TYPE>(1, std::string("FF"));
}

TEST_F(TestScanner, GetBankParamEnum) {
    // INTERNAL_LIGHTING_TYPE = 10, bank 1 -> "RB,01010"
    EXPECT_CALL(*pMockComm, sendCommand("RB,01010")).WillOnce(Return("OK,RB,2"));
    auto val = pScanner->getParam<OpenSRX::BankParam::INTERNAL_LIGHTING_TYPE>(1);
    EXPECT_EQ(val, OpenSRX::InternalLightingType::DIFFUSED);
}

TEST_F(TestScanner, SetBankParamEnum) {
    // INTERNAL_LIGHTING_TYPE = 10, bank 1, POLARIZED -> "WB,01010,1"
    EXPECT_CALL(*pMockComm, sendCommand("WB,01010,1")).WillOnce(Return("OK,WB"));
    pScanner->setParam<OpenSRX::BankParam::INTERNAL_LIGHTING_TYPE>(
        1, OpenSRX::InternalLightingType::POLARIZED);
}

TEST_F(TestScanner, GetBankParamIntVector) {
    // QR_LENGTH_LIMITATION_VALUE = 701, bank 1 -> "RB,01701"
    EXPECT_CALL(*pMockComm, sendCommand("RB,01701")).WillOnce(Return("OK,RB,1:100"));
    auto val = pScanner->getParam<OpenSRX::BankParam::QR_LENGTH_LIMITATION_VALUE>(1);
    ASSERT_EQ(val.size(), 2u);
    EXPECT_EQ(val[0], 1);
    EXPECT_EQ(val[1], 100);
}

TEST_F(TestScanner, SetBankParamIntVector) {
    // QR_LENGTH_LIMITATION_VALUE = 701, bank 1, {5, 200} -> "WB,01701,5:200"
    EXPECT_CALL(*pMockComm, sendCommand("WB,01701,5:200")).WillOnce(Return("OK,WB"));
    pScanner->setParam<OpenSRX::BankParam::QR_LENGTH_LIMITATION_VALUE>(1,
                                                                       std::vector<int>{5, 200});
}

TEST_F(TestScanner, GetBankParamFilterType) {
    // FILTER_1ST_TYPE = 200, bank 1 -> "RB,01200"
    EXPECT_CALL(*pMockComm, sendCommand("RB,01200")).WillOnce(Return("OK,RB,7"));
    auto val = pScanner->getParam<OpenSRX::BankParam::FILTER_1ST_TYPE>(1);
    EXPECT_EQ(val, OpenSRX::FilterType::UNSHARP_MASK);
}

TEST_F(TestScanner, GetBankParamInverseMode) {
    // INVERSE = 605, bank 2 -> "RB,02605"
    EXPECT_CALL(*pMockComm, sendCommand("RB,02605")).WillOnce(Return("OK,RB,2"));
    auto val = pScanner->getParam<OpenSRX::BankParam::INVERSE>(2);
    EXPECT_EQ(val, OpenSRX::InverseMode::AUTOMATIC);
}

TEST_F(TestScanner, BankParamFormattingMultiDigitBank) {
    // Bank 16, GAIN = 101 -> "RB,16101"
    EXPECT_CALL(*pMockComm, sendCommand("RB,16101")).WillOnce(Return("OK,RB,42"));
    int val = pScanner->getParam<OpenSRX::BankParam::GAIN>(16);
    EXPECT_EQ(val, 42);
}

// ─── Tuning parameter (RC/WC) tests ────────────────────────────────────────

TEST_F(TestScanner, GetTuningParamInt) {
    // QR_MAX_READ_LENGTH = 100 -> "RC,100"
    EXPECT_CALL(*pMockComm, sendCommand("RC,100")).WillOnce(Return("OK,RC,7089"));
    int val = pScanner->getParam<OpenSRX::TuningParam::QR_MAX_READ_LENGTH>();
    EXPECT_EQ(val, 7089);
}

TEST_F(TestScanner, SetTuningParamInt) {
    // QR_MAX_READ_LENGTH = 100, value 5000 -> "WC,100,5000"
    EXPECT_CALL(*pMockComm, sendCommand("WC,100,5000")).WillOnce(Return("OK,WC"));
    pScanner->setParam<OpenSRX::TuningParam::QR_MAX_READ_LENGTH>(5000);
}

TEST_F(TestScanner, GetTuningParamToggle) {
    // CODE39_SEND_START_STOP = 603 -> "RC,603"
    EXPECT_CALL(*pMockComm, sendCommand("RC,603")).WillOnce(Return("OK,RC,0"));
    auto val = pScanner->getParam<OpenSRX::TuningParam::CODE39_SEND_START_STOP>();
    EXPECT_EQ(val, OpenSRX::Toggle::DISABLE);
}

TEST_F(TestScanner, SetTuningParamToggle) {
    // CODE39_SEND_START_STOP = 603, ENABLE -> "WC,603,1"
    EXPECT_CALL(*pMockComm, sendCommand("WC,603,1")).WillOnce(Return("OK,WC"));
    pScanner->setParam<OpenSRX::TuningParam::CODE39_SEND_START_STOP>(OpenSRX::Toggle::ENABLE);
}

TEST_F(TestScanner, GetTuningParamNW7CheckDigitType) {
    // NW7_CHECK_DIGIT_TYPE = 907 -> "RC,907"
    EXPECT_CALL(*pMockComm, sendCommand("RC,907")).WillOnce(Return("OK,RC,6"));
    auto val = pScanner->getParam<OpenSRX::TuningParam::NW7_CHECK_DIGIT_TYPE>();
    EXPECT_EQ(val, OpenSRX::NW7CheckDigitType::LUHN);
}

TEST_F(TestScanner, SetTuningParamUpcAOutput) {
    // UPC_A_OUTPUT = 1006, OUTPUT_12_DIGITS -> "WC,1006,1"
    EXPECT_CALL(*pMockComm, sendCommand("WC,1006,1")).WillOnce(Return("OK,WC"));
    pScanner->setParam<OpenSRX::TuningParam::UPC_A_OUTPUT>(OpenSRX::UpcAOutput::OUTPUT_12_DIGITS);
}

// ─── Operation parameter (RP/WP) tests ─────────────────────────────────────

TEST_F(TestScanner, GetOperationParamTimingMode) {
    // TIMING_MODE = 101 -> "RP,101"
    EXPECT_CALL(*pMockComm, sendCommand("RP,101")).WillOnce(Return("OK,RP,1"));
    auto val = pScanner->getParam<OpenSRX::OperationParam::TIMING_MODE>();
    EXPECT_EQ(val, OpenSRX::TimingMode::ONE_SHOT_TRIGGER);
}

TEST_F(TestScanner, SetOperationParamTimingMode) {
    // TIMING_MODE = 101, LEVEL_TRIGGER -> "WP,101,0"
    EXPECT_CALL(*pMockComm, sendCommand("WP,101,0")).WillOnce(Return("OK,WP"));
    pScanner->setParam<OpenSRX::OperationParam::TIMING_MODE>(OpenSRX::TimingMode::LEVEL_TRIGGER);
}

TEST_F(TestScanner, GetOperationParamReadingMode) {
    // READING_MODE = 200 -> "RP,200"
    EXPECT_CALL(*pMockComm, sendCommand("RP,200")).WillOnce(Return("OK,RP,3"));
    auto val = pScanner->getParam<OpenSRX::OperationParam::READING_MODE>();
    EXPECT_EQ(val, OpenSRX::ReadingMode::BURST_READ);
}

TEST_F(TestScanner, SetOperationParamReadingMode) {
    // READING_MODE = 200, CONTINUOUS -> "WP,200,1"
    EXPECT_CALL(*pMockComm, sendCommand("WP,200,1")).WillOnce(Return("OK,WP"));
    pScanner->setParam<OpenSRX::OperationParam::READING_MODE>(OpenSRX::ReadingMode::CONTINUOUS);
}

TEST_F(TestScanner, GetOperationParamToggle) {
    // SHORTEN_BANK_TRANSITION = 214 -> "RP,214"
    EXPECT_CALL(*pMockComm, sendCommand("RP,214")).WillOnce(Return("OK,RP,1"));
    auto val = pScanner->getParam<OpenSRX::OperationParam::SHORTEN_BANK_TRANSITION>();
    EXPECT_EQ(val, OpenSRX::Toggle::ENABLE);
}

TEST_F(TestScanner, GetOperationParamInt) {
    // ONE_SHOT_TRIGGER_DURATION = 102 -> "RP,102"
    EXPECT_CALL(*pMockComm, sendCommand("RP,102")).WillOnce(Return("OK,RP,250"));
    int val = pScanner->getParam<OpenSRX::OperationParam::ONE_SHOT_TRIGGER_DURATION>();
    EXPECT_EQ(val, 250);
}

TEST_F(TestScanner, SetOperationParamInt) {
    // ONE_SHOT_TRIGGER_DURATION = 102, value 100 -> "WP,102,100"
    EXPECT_CALL(*pMockComm, sendCommand("WP,102,100")).WillOnce(Return("OK,WP"));
    pScanner->setParam<OpenSRX::OperationParam::ONE_SHOT_TRIGGER_DURATION>(100);
}

TEST_F(TestScanner, GetOperationParamHex) {
    // TRIGGER_ON_COMMAND_STRING = 103 -> "RP,103"
    EXPECT_CALL(*pMockComm, sendCommand("RP,103")).WillOnce(Return("OK,RP,4C4F4E"));
    auto val = pScanner->getParam<OpenSRX::OperationParam::TRIGGER_ON_COMMAND_STRING>();
    EXPECT_EQ(val, "4C4F4E");
}

TEST_F(TestScanner, GetOperationParamInTerminalFunction) {
    // IN1_TERMINAL_FUNCTION = 0 -> "RP,0"
    EXPECT_CALL(*pMockComm, sendCommand("RP,0")).WillOnce(Return("OK,RP,1"));
    auto val = pScanner->getParam<OpenSRX::OperationParam::IN1_TERMINAL_FUNCTION>();
    EXPECT_EQ(val, OpenSRX::InTerminalFunction::TRIGGER_INPUT);
}

TEST_F(TestScanner, SetOperationParamInTerminalFunction) {
    // IN1_TERMINAL_FUNCTION = 0, CAPTURE -> "WP,0,4"
    EXPECT_CALL(*pMockComm, sendCommand("WP,0,4")).WillOnce(Return("OK,WP"));
    pScanner->setParam<OpenSRX::OperationParam::IN1_TERMINAL_FUNCTION>(
        OpenSRX::InTerminalFunction::CAPTURE);
}

TEST_F(TestScanner, GetOperationParamImageFormat) {
    // IMAGE_FORMAT = 511 -> "RP,511"
    EXPECT_CALL(*pMockComm, sendCommand("RP,511")).WillOnce(Return("OK,RP,1"));
    auto val = pScanner->getParam<OpenSRX::OperationParam::IMAGE_FORMAT>();
    EXPECT_EQ(val, OpenSRX::ImageFormat::JPG);
}

TEST_F(TestScanner, GetOperationParamAscii) {
    // READER_DESCRIPTION_1 = 620 -> "RP,620"
    EXPECT_CALL(*pMockComm, sendCommand("RP,620")).WillOnce(Return("OK,RP,Line1 Reader"));
    auto val = pScanner->getParam<OpenSRX::OperationParam::READER_DESCRIPTION_1>();
    EXPECT_EQ(val, "Line1 Reader");
}

TEST_F(TestScanner, SetOperationParamAscii) {
    // READER_DESCRIPTION_1 = 620, "NewDesc" -> "WP,620,NewDesc"
    EXPECT_CALL(*pMockComm, sendCommand("WP,620,NewDesc")).WillOnce(Return("OK,WP"));
    pScanner->setParam<OpenSRX::OperationParam::READER_DESCRIPTION_1>(std::string("NewDesc"));
}

// ─── Communication parameter (RN/WN) tests ─────────────────────────────────

TEST_F(TestScanner, GetCommParamAscii) {
    // IP_ADDRESS = 200 -> "RN,200"
    EXPECT_CALL(*pMockComm, sendCommand("RN,200")).WillOnce(Return("OK,RN,192.168.1.100"));
    auto val = pScanner->getParam<OpenSRX::CommParam::IP_ADDRESS>();
    EXPECT_EQ(val, "192.168.1.100");
}

TEST_F(TestScanner, SetCommParamAscii) {
    // IP_ADDRESS = 200, "10.0.0.1" -> "WN,200,10.0.0.1"
    EXPECT_CALL(*pMockComm, sendCommand("WN,200,10.0.0.1")).WillOnce(Return("OK,WN"));
    pScanner->setParam<OpenSRX::CommParam::IP_ADDRESS>(std::string("10.0.0.1"));
}

TEST_F(TestScanner, GetCommParamToggle) {
    // APPEND_CHECKSUM = 3 -> "RN,3"
    EXPECT_CALL(*pMockComm, sendCommand("RN,3")).WillOnce(Return("OK,RN,0"));
    auto val = pScanner->getParam<OpenSRX::CommParam::APPEND_CHECKSUM>();
    EXPECT_EQ(val, OpenSRX::Toggle::DISABLE);
}

TEST_F(TestScanner, SetCommParamToggle) {
    // APPEND_CHECKSUM = 3, ENABLE -> "WN,3,1"
    EXPECT_CALL(*pMockComm, sendCommand("WN,3,1")).WillOnce(Return("OK,WN"));
    pScanner->setParam<OpenSRX::CommParam::APPEND_CHECKSUM>(OpenSRX::Toggle::ENABLE);
}

TEST_F(TestScanner, GetCommParamBaudRateHigh) {
    // BAUD_RATE_HIGH = 100 -> "RN,100"
    EXPECT_CALL(*pMockComm, sendCommand("RN,100")).WillOnce(Return("OK,RN,4"));
    auto val = pScanner->getParam<OpenSRX::CommParam::BAUD_RATE_HIGH>();
    EXPECT_EQ(val, OpenSRX::BaudRateHigh::BPS_115200);
}

TEST_F(TestScanner, SetCommParamBaudRateHigh) {
    // BAUD_RATE_HIGH = 100, BPS_9600 -> "WN,100,0"
    EXPECT_CALL(*pMockComm, sendCommand("WN,100,0")).WillOnce(Return("OK,WN"));
    pScanner->setParam<OpenSRX::CommParam::BAUD_RATE_HIGH>(OpenSRX::BaudRateHigh::BPS_9600);
}

TEST_F(TestScanner, GetCommParamEthernetCommandMode) {
    // ETHERNET_COMMAND = 203 -> "RN,203"
    EXPECT_CALL(*pMockComm, sendCommand("RN,203")).WillOnce(Return("OK,RN,1"));
    auto val = pScanner->getParam<OpenSRX::CommParam::ETHERNET_COMMAND>();
    EXPECT_EQ(val, OpenSRX::EthernetCommandMode::TCP);
}

TEST_F(TestScanner, GetCommParamHex) {
    // HEADER_SETTINGS = 5 -> "RN,5"
    EXPECT_CALL(*pMockComm, sendCommand("RN,5")).WillOnce(Return("OK,RN,02"));
    auto val = pScanner->getParam<OpenSRX::CommParam::HEADER_SETTINGS>();
    EXPECT_EQ(val, "02");
}

TEST_F(TestScanner, SetCommParamHex) {
    // HEADER_SETTINGS = 5, "03" -> "WN,5,03"
    EXPECT_CALL(*pMockComm, sendCommand("WN,5,03")).WillOnce(Return("OK,WN"));
    pScanner->setParam<OpenSRX::CommParam::HEADER_SETTINGS>(std::string("03"));
}

TEST_F(TestScanner, GetCommParamPLCProtocol) {
    // PLC_PROTOCOL = 303 -> "RN,303"
    EXPECT_CALL(*pMockComm, sendCommand("RN,303")).WillOnce(Return("OK,RN,7"));
    auto val = pScanner->getParam<OpenSRX::CommParam::PLC_PROTOCOL>();
    EXPECT_EQ(val, OpenSRX::PLCProtocol::ETHERNET_IP);
}

TEST_F(TestScanner, SetCommParamPLCProtocol) {
    // PLC_PROTOCOL = 303, PROFINET -> "WN,303,8"
    EXPECT_CALL(*pMockComm, sendCommand("WN,303,8")).WillOnce(Return("OK,WN"));
    pScanner->setParam<OpenSRX::CommParam::PLC_PROTOCOL>(OpenSRX::PLCProtocol::PROFINET);
}

TEST_F(TestScanner, GetCommParamInt) {
    // COMMAND_STANDBY_PORT = 204 -> "RN,204"
    EXPECT_CALL(*pMockComm, sendCommand("RN,204")).WillOnce(Return("OK,RN,9004"));
    int val = pScanner->getParam<OpenSRX::CommParam::COMMAND_STANDBY_PORT>();
    EXPECT_EQ(val, 9004);
}

TEST_F(TestScanner, SetCommParamInt) {
    // COMMAND_STANDBY_PORT = 204, 8080 -> "WN,204,8080"
    EXPECT_CALL(*pMockComm, sendCommand("WN,204,8080")).WillOnce(Return("OK,WN"));
    pScanner->setParam<OpenSRX::CommParam::COMMAND_STANDBY_PORT>(8080);
}

// ─── parseVersionInfo tests ─────────────────────────────────────────────────

TEST(TestParseVersionInfo, ParsesValidInput) {
    auto [model, firmware] = OpenSRX::parseVersionInfo("SR-X100W,V2.0.0");
    EXPECT_EQ(model, "SR-X100W");
    EXPECT_EQ(firmware, "V2.0.0");
}

TEST(TestParseVersionInfo, ThrowsOnMissingComma) {
    EXPECT_THROW(OpenSRX::parseVersionInfo("NoCommaHere"), std::runtime_error);
}

// ─── Error response handling tests ──────────────────────────────────────────

TEST_F(TestScanner, SetBankParamThrowsInvalidArgumentOnUndefinedCommand) {
    EXPECT_CALL(*pMockComm, sendCommand("WB,01100,500")).WillOnce(Return("ER,WB,0"));
    EXPECT_THROW(pScanner->setParam<OpenSRX::BankParam::EXPOSURE_TIME>(1, 500),
                 std::invalid_argument);
}

TEST_F(TestScanner, SetBankParamThrowsInvalidArgumentOnMismatchedFormat) {
    EXPECT_CALL(*pMockComm, sendCommand("WB,01100,500")).WillOnce(Return("ER,WB,1"));
    EXPECT_THROW(pScanner->setParam<OpenSRX::BankParam::EXPOSURE_TIME>(1, 500),
                 std::invalid_argument);
}

TEST_F(TestScanner, SetBankParamThrowsOutOfRangeOnParam1OutOfRange) {
    EXPECT_CALL(*pMockComm, sendCommand("WB,01100,500")).WillOnce(Return("ER,WB,2"));
    EXPECT_THROW(pScanner->setParam<OpenSRX::BankParam::EXPOSURE_TIME>(1, 500),
                 std::out_of_range);
}

TEST_F(TestScanner, SetBankParamThrowsOutOfRangeOnParam2OutOfRange) {
    EXPECT_CALL(*pMockComm, sendCommand("WB,01100,500")).WillOnce(Return("ER,WB,3"));
    EXPECT_THROW(pScanner->setParam<OpenSRX::BankParam::EXPOSURE_TIME>(1, 500),
                 std::out_of_range);
}

TEST_F(TestScanner, SetBankParamThrowsInvalidArgumentOnParam2NotHex) {
    EXPECT_CALL(*pMockComm, sendCommand("WB,01300,GG")).WillOnce(Return("ER,WB,4"));
    EXPECT_THROW(pScanner->setParam<OpenSRX::BankParam::CODE_TYPE>(1, std::string("GG")),
                 std::invalid_argument);
}

TEST_F(TestScanner, SetBankParamThrowsOutOfRangeOnParam2HexOutOfRange) {
    EXPECT_CALL(*pMockComm, sendCommand("WB,01300,FF")).WillOnce(Return("ER,WB,5"));
    EXPECT_THROW(pScanner->setParam<OpenSRX::BankParam::CODE_TYPE>(1, std::string("FF")),
                 std::out_of_range);
}

TEST_F(TestScanner, SetBankParamThrowsRuntimeErrorOnExecutionError) {
    EXPECT_CALL(*pMockComm, sendCommand("WB,01100,500")).WillOnce(Return("ER,WB,20"));
    EXPECT_THROW(pScanner->setParam<OpenSRX::BankParam::EXPOSURE_TIME>(1, 500),
                 std::runtime_error);
}

TEST_F(TestScanner, SetBankParamThrowsOverflowErrorOnBufferOverflow) {
    EXPECT_CALL(*pMockComm, sendCommand("WB,01100,500")).WillOnce(Return("ER,WB,21"));
    EXPECT_THROW(pScanner->setParam<OpenSRX::BankParam::EXPOSURE_TIME>(1, 500),
                 std::overflow_error);
}

TEST_F(TestScanner, SetBankParamThrowsRuntimeErrorOnDeviceFault) {
    EXPECT_CALL(*pMockComm, sendCommand("WB,01100,500")).WillOnce(Return("ER,WB,99"));
    EXPECT_THROW(pScanner->setParam<OpenSRX::BankParam::EXPOSURE_TIME>(1, 500),
                 std::runtime_error);
}

TEST_F(TestScanner, SetTuningParamThrowsOnError) {
    EXPECT_CALL(*pMockComm, sendCommand("WC,100,5000")).WillOnce(Return("ER,WC,20"));
    EXPECT_THROW(pScanner->setParam<OpenSRX::TuningParam::QR_MAX_READ_LENGTH>(5000),
                 std::runtime_error);
}

TEST_F(TestScanner, SetOperationParamThrowsOnError) {
    EXPECT_CALL(*pMockComm, sendCommand("WP,101,0")).WillOnce(Return("ER,WP,3"));
    EXPECT_THROW(
        pScanner->setParam<OpenSRX::OperationParam::TIMING_MODE>(OpenSRX::TimingMode::LEVEL_TRIGGER),
        std::out_of_range);
}

TEST_F(TestScanner, SetCommParamThrowsOnError) {
    EXPECT_CALL(*pMockComm, sendCommand("WN,200,10.0.0.1")).WillOnce(Return("ER,WN,20"));
    EXPECT_THROW(
        pScanner->setParam<OpenSRX::CommParam::IP_ADDRESS>(std::string("10.0.0.1")),
        std::runtime_error);
}

TEST_F(TestScanner, SetParamSucceedsOnOKResponse) {
    EXPECT_CALL(*pMockComm, sendCommand("WB,01100,500")).WillOnce(Return("OK,WB"));
    EXPECT_NO_THROW(pScanner->setParam<OpenSRX::BankParam::EXPOSURE_TIME>(1, 500));
}

TEST_F(TestScanner, SetBankParamThrowsRuntimeErrorOnMalformedError) {
    EXPECT_CALL(*pMockComm, sendCommand("WB,01100,500")).WillOnce(Return("ER,WB,abc"));
    EXPECT_THROW(pScanner->setParam<OpenSRX::BankParam::EXPOSURE_TIME>(1, 500),
                 std::runtime_error);
}

TEST_F(TestScanner, SetBankParamThrowsRuntimeErrorOnUnknownErrorCode) {
    EXPECT_CALL(*pMockComm, sendCommand("WB,01100,500")).WillOnce(Return("ER,WB,42"));
    EXPECT_THROW(pScanner->setParam<OpenSRX::BankParam::EXPOSURE_TIME>(1, 500),
                 std::runtime_error);
}

// ─── Read error response handling tests ─────────────────────────────────────

TEST_F(TestScanner, GetBankParamThrowsOnError) {
    EXPECT_CALL(*pMockComm, sendCommand("RB,01100")).WillOnce(Return("ER,RB,2"));
    EXPECT_THROW(pScanner->getParam<OpenSRX::BankParam::EXPOSURE_TIME>(1), std::out_of_range);
}

TEST_F(TestScanner, GetTuningParamThrowsOnError) {
    EXPECT_CALL(*pMockComm, sendCommand("RC,100")).WillOnce(Return("ER,RC,0"));
    EXPECT_THROW(pScanner->getParam<OpenSRX::TuningParam::QR_MAX_READ_LENGTH>(),
                 std::invalid_argument);
}

TEST_F(TestScanner, GetOperationParamThrowsOnError) {
    EXPECT_CALL(*pMockComm, sendCommand("RP,101")).WillOnce(Return("ER,RP,20"));
    EXPECT_THROW(pScanner->getParam<OpenSRX::OperationParam::TIMING_MODE>(), std::runtime_error);
}

TEST_F(TestScanner, GetCommParamThrowsOnError) {
    EXPECT_CALL(*pMockComm, sendCommand("RN,200")).WillOnce(Return("ER,RN,99"));
    EXPECT_THROW(pScanner->getParam<OpenSRX::CommParam::IP_ADDRESS>(), std::runtime_error);
}
