#include "TestScanner.hpp"

namespace OpenSRX {

// ─── Constructor ─────────────────────────────────────────────────────────────

TEST_F(TestScanner, ConstructorParsesModelFirmwareAndMac) {
    EXPECT_EQ(pScanner->getModel(), "SR-X300");
    EXPECT_EQ(pScanner->getFirmwareVersion(), "V1.2.3");
    EXPECT_EQ(pScanner->getMacAddress(), "001122334455");
}

// ─── Reading ─────────────────────────────────────────────────────────────────

TEST_F(TestScanner, StartReadingSendsLON) {
    EXPECT_CALL(*pMockComm, sendCommandUnlocked("LON")).WillOnce(Return("BARCODE123"));
    expectParseReadDefaults();
    Code result = pScanner->startReading();
    EXPECT_EQ(result.data, "BARCODE123");
}

TEST_F(TestScanner, StartReadingThrowsOnError) {
    EXPECT_CALL(*pMockComm, sendCommandUnlocked("LON")).WillOnce(Return("ERROR"));
    EXPECT_THROW(pScanner->startReading(), std::runtime_error);
}

TEST_F(TestScanner, StartReadingWithBankSendsLONWithBank) {
    EXPECT_CALL(*pMockComm, sendCommandUnlocked("LON,03")).WillOnce(Return("DATA456"));
    expectParseReadDefaults();
    Code result = pScanner->startReading(3);
    EXPECT_EQ(result.data, "DATA456");
}

TEST_F(TestScanner, StopReadingSendsLOFF) {
    EXPECT_CALL(*pMockComm, sendCommandUnlocked("LOFF")).WillOnce(Return(""));
    pScanner->stopReading();
}

TEST_F(TestScanner, StartReadingUsesUnlockedSend) {
    // Verify LON goes through sendCommandUnlocked (not sendCommand)
    EXPECT_CALL(*pMockComm, sendCommandUnlocked("LON")).WillOnce(Return("CODE"));
    EXPECT_CALL(*pMockComm, sendCommand("LON")).Times(0);
    expectParseReadDefaults();
    pScanner->startReading();
}

TEST_F(TestScanner, StopReadingUsesUnlockedSend) {
    EXPECT_CALL(*pMockComm, sendCommandUnlocked("LOFF")).WillOnce(Return(""));
    EXPECT_CALL(*pMockComm, sendCommand("LOFF")).Times(0);
    pScanner->stopReading();
}

TEST_F(TestScanner, StartReadingParsesVertexData) {
    EXPECT_CALL(*pMockComm, sendCommandUnlocked("LON"))
        .WillOnce(Return("HELLO,10,20,30,40,50,60,70,80"));
    EXPECT_CALL(*pMockComm, sendCommand("RP,602")).WillOnce(Return("OK,RP,2C"));
    EXPECT_CALL(*pMockComm, sendCommand("RP,308")).WillOnce(Return("OK,RP,1"));  // vertex ON
    EXPECT_CALL(*pMockComm, sendCommand("RP,309")).WillOnce(Return("OK,RP,0"));
    EXPECT_CALL(*pMockComm, sendCommand("RP,301")).WillOnce(Return("OK,RP,0"));
    EXPECT_CALL(*pMockComm, sendCommand("RP,303")).WillOnce(Return("OK,RP,0"));
    EXPECT_CALL(*pMockComm, sendCommand("RP,371")).WillOnce(Return("OK,RP,0"));
    Code result = pScanner->startReading();
    EXPECT_EQ(result.data, "HELLO");
    ASSERT_TRUE(result.boundingBox.has_value());
    EXPECT_EQ(result.boundingBox->topLeft.x, 10);
    EXPECT_EQ(result.boundingBox->topLeft.y, 20);
    EXPECT_EQ(result.boundingBox->bottomRight.x, 50);
    EXPECT_EQ(result.boundingBox->bottomRight.y, 60);
}

TEST_F(TestScanner, StartReadingParsesCenterAndCodeType) {
    EXPECT_CALL(*pMockComm, sendCommandUnlocked("LON")).WillOnce(Return("DATA,100,200,CODE128"));
    EXPECT_CALL(*pMockComm, sendCommand("RP,602")).WillOnce(Return("OK,RP,2C"));
    EXPECT_CALL(*pMockComm, sendCommand("RP,308")).WillOnce(Return("OK,RP,0"));
    EXPECT_CALL(*pMockComm, sendCommand("RP,309")).WillOnce(Return("OK,RP,1"));  // center ON
    EXPECT_CALL(*pMockComm, sendCommand("RP,301")).WillOnce(Return("OK,RP,1"));  // code type ON
    EXPECT_CALL(*pMockComm, sendCommand("RP,303")).WillOnce(Return("OK,RP,0"));
    EXPECT_CALL(*pMockComm, sendCommand("RP,371")).WillOnce(Return("OK,RP,0"));
    Code result = pScanner->startReading();
    EXPECT_EQ(result.data, "DATA");
    ASSERT_TRUE(result.center.has_value());
    EXPECT_EQ(result.center->x, 100);
    EXPECT_EQ(result.center->y, 200);
    ASSERT_TRUE(result.codeType.has_value());
    EXPECT_EQ(*result.codeType, "CODE128");
}

TEST_F(TestScanner, StartReadingParsesAllAppendedFields) {
    // vertex(8) + center(2) + type(1) + bank(1) + angle(1) = 13 appended fields
    EXPECT_CALL(*pMockComm, sendCommandUnlocked("LON"))
        .WillOnce(Return("ABC,1,2,3,4,5,6,7,8,10,20,QR,03,2.5"));
    EXPECT_CALL(*pMockComm, sendCommand("RP,602")).WillOnce(Return("OK,RP,2C"));
    EXPECT_CALL(*pMockComm, sendCommand("RP,308")).WillOnce(Return("OK,RP,1"));
    EXPECT_CALL(*pMockComm, sendCommand("RP,309")).WillOnce(Return("OK,RP,1"));
    EXPECT_CALL(*pMockComm, sendCommand("RP,301")).WillOnce(Return("OK,RP,1"));
    EXPECT_CALL(*pMockComm, sendCommand("RP,303")).WillOnce(Return("OK,RP,1"));
    EXPECT_CALL(*pMockComm, sendCommand("RP,371")).WillOnce(Return("OK,RP,1"));
    Code result = pScanner->startReading();
    EXPECT_EQ(result.data, "ABC");
    ASSERT_TRUE(result.boundingBox.has_value());
    ASSERT_TRUE(result.center.has_value());
    EXPECT_EQ(result.center->x, 10);
    EXPECT_EQ(result.center->y, 20);
    ASSERT_TRUE(result.codeType.has_value());
    EXPECT_EQ(*result.codeType, "QR");
    ASSERT_TRUE(result.bankNumber.has_value());
    EXPECT_EQ(*result.bankNumber, 3);
    ASSERT_TRUE(result.angle.has_value());
    EXPECT_DOUBLE_EQ(*result.angle, 2.5);
}

// ─── Quick setup code reading ────────────────────────────────────────────────

TEST_F(TestScanner, StartQuickSetupCodeReading) {
    EXPECT_CALL(*pMockComm, sendCommand("RCON")).WillOnce(Return("OK,RCON"));
    pScanner->startQuickSetupCodeReading();
}

TEST_F(TestScanner, FinishQuickSetupCodeReading) {
    EXPECT_CALL(*pMockComm, sendCommand("RCOFF")).WillOnce(Return("OK,RCOFF"));
    pScanner->finishQuickSetupCodeReading();
}

TEST_F(TestScanner, CheckQuickSetupCodeResult) {
    EXPECT_CALL(*pMockComm, sendCommand("RCCHK")).WillOnce(Return("OK,RCCHK,SomeCodeData"));
    auto result = pScanner->checkQuickSetupCodeResult();
    EXPECT_EQ(result, "SomeCodeData");
}

// ─── Test mode ───────────────────────────────────────────────────────────────

TEST_F(TestScanner, ReadingRateTest) {
    EXPECT_CALL(*pMockComm, sendCommandUnlocked("TEST1")).WillOnce(Return("OK,TEST1"));
    pScanner->readingRateTest();
}

TEST_F(TestScanner, ReadingRateTestWithBank) {
    EXPECT_CALL(*pMockComm, sendCommandUnlocked("TEST1,05")).WillOnce(Return("OK,TEST1"));
    pScanner->readingRateTest(5);
}

TEST_F(TestScanner, ReadTimeTest) {
    EXPECT_CALL(*pMockComm, sendCommandUnlocked("TEST2")).WillOnce(Return("OK,TEST2"));
    pScanner->readTimeTest();
}

TEST_F(TestScanner, ReadTimeTestWithBank) {
    EXPECT_CALL(*pMockComm, sendCommandUnlocked("TEST2,16")).WillOnce(Return("OK,TEST2"));
    pScanner->readTimeTest(16);
}

TEST_F(TestScanner, QuitTestMode) {
    EXPECT_CALL(*pMockComm, sendCommandUnlocked("QUIT")).WillOnce(Return("OK,QUIT"));
    pScanner->quitTestMode();
}

TEST_F(TestScanner, TestModeUsesUnlockedSend) {
    EXPECT_CALL(*pMockComm, sendCommandUnlocked("TEST1")).WillOnce(Return("OK,TEST1"));
    EXPECT_CALL(*pMockComm, sendCommand("TEST1")).Times(0);
    pScanner->readingRateTest();
}

// ─── I/O terminal control ───────────────────────────────────────────────────

TEST_F(TestScanner, GetInputTerminalStateOn) {
    EXPECT_CALL(*pMockComm, sendCommand("INCHK,1")).WillOnce(Return("OK,INCHK,ON"));
    EXPECT_TRUE(pScanner->getInputTerminalState(1));
}

TEST_F(TestScanner, GetInputTerminalStateOff) {
    EXPECT_CALL(*pMockComm, sendCommand("INCHK,2")).WillOnce(Return("OK,INCHK,OFF"));
    EXPECT_FALSE(pScanner->getInputTerminalState(2));
}

TEST_F(TestScanner, TurnOnOutputTerminal) {
    EXPECT_CALL(*pMockComm, sendCommand("OUTON,1")).WillOnce(Return("OK,OUTON"));
    pScanner->turnOnOutputTerminal(1);
}

TEST_F(TestScanner, TurnOffOutputTerminal) {
    EXPECT_CALL(*pMockComm, sendCommand("OUTOFF,3")).WillOnce(Return("OK,OUTOFF"));
    pScanner->turnOffOutputTerminal(3);
}

TEST_F(TestScanner, TurnOnAllOutputTerminals) {
    EXPECT_CALL(*pMockComm, sendCommand("ALLON")).WillOnce(Return("OK,ALLON"));
    pScanner->turnOnAllOutputTerminals();
}

TEST_F(TestScanner, TurnOffAllOutputTerminals) {
    EXPECT_CALL(*pMockComm, sendCommand("ALLOFF")).WillOnce(Return("OK,ALLOFF"));
    pScanner->turnOffAllOutputTerminals();
}

// ─── Reset and buffer ────────────────────────────────────────────────────────

TEST_F(TestScanner, Reset) {
    EXPECT_CALL(*pMockComm, sendCommand("RESET")).WillOnce(Return("OK,RESET"));
    pScanner->reset();
}

TEST_F(TestScanner, ClearSendBuffer) {
    EXPECT_CALL(*pMockComm, sendCommand("BCLR")).WillOnce(Return("OK,BCLR"));
    pScanner->clearSendBuffer();
}

// ─── Image capture ───────────────────────────────────────────────────────────

TEST_F(TestScanner, CaptureImage) {
    EXPECT_CALL(*pMockComm, sendCommand("SHOT,01"))
        .WillOnce(Return("OK,SHOT,A:\\IMAGE\\capture001.bmp"));
    auto result = pScanner->captureImage(1);
    EXPECT_EQ(result, "A:\\IMAGE\\capture001.bmp");
}

// ─── Focus and tuning ────────────────────────────────────────────────────────

TEST_F(TestScanner, AdjustFocus) {
    EXPECT_CALL(*pMockComm, sendCommandUnlocked("FTUNE")).WillOnce(Return("OK,FTUNE"));
    pScanner->adjustFocus();
}

TEST_F(TestScanner, AdjustFocusUsesUnlockedSend) {
    EXPECT_CALL(*pMockComm, sendCommandUnlocked("FTUNE")).WillOnce(Return("OK,FTUNE"));
    EXPECT_CALL(*pMockComm, sendCommand("FTUNE")).Times(0);
    pScanner->adjustFocus();
}

TEST_F(TestScanner, StartTuningSuccess) {
    EXPECT_CALL(*pMockComm, sendCommandUnlocked("TUNE,01"))
        .WillOnce(Return("Tuning SUCCEEDED,500ms,00000000"));
    auto [succeeded, advice, reason] = pScanner->startTuning(1);
    EXPECT_TRUE(succeeded);
    EXPECT_EQ(advice, TuningAdvice::NONE);
    EXPECT_EQ(reason, TuningFailureReason(0));
}

TEST_F(TestScanner, StartTuningFailureWithAdvice) {
    EXPECT_CALL(*pMockComm, sendCommandUnlocked("TUNE,02"))
        .WillOnce(Return("Tuning FAILED,300ms,00000102"));
    auto [succeeded, advice, reason] = pScanner->startTuning(2);
    EXPECT_FALSE(succeeded);
    EXPECT_EQ(advice, TuningAdvice::USE_AN_IMAGE_FILTER);
    EXPECT_EQ(reason, TuningFailureReason::UNSTABLE_READING);
}

TEST_F(TestScanner, StartTuningFailureCodeDetectionImpossible) {
    EXPECT_CALL(*pMockComm, sendCommandUnlocked("TUNE,05"))
        .WillOnce(Return("Tuning FAILED,100ms,00000201"));
    auto [succeeded, advice, reason] = pScanner->startTuning(5);
    EXPECT_FALSE(succeeded);
    EXPECT_EQ(advice, TuningAdvice::CONSIDER_INSTALLATION_LIGHTING_PRINTING_CONDITIONS);
    EXPECT_EQ(reason, TuningFailureReason::CODE_DETECTION_IMPOSSIBLE);
}

TEST_F(TestScanner, StartTuningBrightnessInsufficient) {
    EXPECT_CALL(*pMockComm, sendCommandUnlocked("TUNE,01"))
        .WillOnce(Return("Tuning FAILED,200ms,00000401"));
    auto [succeeded, advice, reason] = pScanner->startTuning(1);
    EXPECT_FALSE(succeeded);
    EXPECT_EQ(advice, TuningAdvice::BRIGHTNESS_INSUFFICIENT);
}

TEST_F(TestScanner, StartTuningUsesUnlockedSend) {
    EXPECT_CALL(*pMockComm, sendCommandUnlocked("TUNE,01"))
        .WillOnce(Return("Tuning SUCCEEDED,100ms,00000000"));
    EXPECT_CALL(*pMockComm, sendCommand("TUNE,01")).Times(0);
    pScanner->startTuning(1);
}

TEST_F(TestScanner, StopTuning) {
    EXPECT_CALL(*pMockComm, sendCommandUnlocked("TQUIT")).WillOnce(Return("OK,TQUIT"));
    pScanner->stopTuning();
}

TEST_F(TestScanner, StopTuningUsesUnlockedSend) {
    EXPECT_CALL(*pMockComm, sendCommandUnlocked("TQUIT")).WillOnce(Return("OK,TQUIT"));
    EXPECT_CALL(*pMockComm, sendCommand("TQUIT")).Times(0);
    pScanner->stopTuning();
}

// ─── Pointer control ─────────────────────────────────────────────────────────

TEST_F(TestScanner, EnablePointer) {
    EXPECT_CALL(*pMockComm, sendCommand("AMON")).WillOnce(Return("OK,AMON"));
    pScanner->enablePointer();
}

TEST_F(TestScanner, DisablePointer) {
    EXPECT_CALL(*pMockComm, sendCommand("AMOFF")).WillOnce(Return("OK,AMOFF"));
    pScanner->disablePointer();
}

// ─── Time settings ───────────────────────────────────────────────────────────

TEST_F(TestScanner, SetTime) {
    EXPECT_CALL(*pMockComm, sendCommand("TMSET,20260513101530")).WillOnce(Return("OK,TMSET"));
    Timestamp ts(30, 15, 10, 13, 5, 2026);
    pScanner->setTime(ts);
}

TEST_F(TestScanner, SetTimeZeroPadded) {
    EXPECT_CALL(*pMockComm, sendCommand("TMSET,20260101000000")).WillOnce(Return("OK,TMSET"));
    Timestamp ts(0, 0, 0, 1, 1, 2026);
    pScanner->setTime(ts);
}

TEST_F(TestScanner, GetTime) {
    EXPECT_CALL(*pMockComm, sendCommand("TMGET")).WillOnce(Return("OK,TMGET,20260315143000"));
    auto ts = pScanner->getTime();
    EXPECT_EQ(ts.year, 2026);
    EXPECT_EQ(ts.month, 3);
    EXPECT_EQ(ts.day, 15);
    EXPECT_EQ(ts.hour, 14);
    EXPECT_EQ(ts.minute, 30);
    EXPECT_EQ(ts.second, 0);
}

// ─── Status queries ──────────────────────────────────────────────────────────

TEST_F(TestScanner, GetCommandStatusNone) {
    EXPECT_CALL(*pMockComm, sendCommand("CMDSTAT")).WillOnce(Return("OK,CMDSTAT,none"));
    EXPECT_EQ(pScanner->getCommandStatus(), CommandStatus::NO_PROCESSING);
}

TEST_F(TestScanner, GetCommandStatusWait) {
    EXPECT_CALL(*pMockComm, sendCommand("CMDSTAT")).WillOnce(Return("OK,CMDSTAT,wait"));
    EXPECT_EQ(pScanner->getCommandStatus(), CommandStatus::WAIT_FOR_SETTING);
}

TEST_F(TestScanner, GetCommandStatusUpdate) {
    EXPECT_CALL(*pMockComm, sendCommand("CMDSTAT")).WillOnce(Return("OK,CMDSTAT,update"));
    EXPECT_EQ(pScanner->getCommandStatus(), CommandStatus::UPDATING);
}

TEST_F(TestScanner, GetCommandStatusThrowsOnUnknown) {
    EXPECT_CALL(*pMockComm, sendCommand("CMDSTAT")).WillOnce(Return("OK,CMDSTAT,bogus"));
    EXPECT_THROW(pScanner->getCommandStatus(), std::runtime_error);
}

TEST_F(TestScanner, GetErrorStatusNone) {
    EXPECT_CALL(*pMockComm, sendCommand("ERRSTAT")).WillOnce(Return("OK,ERRSTAT,none"));
    EXPECT_EQ(pScanner->getErrorStatus(), ErrorStatus::NO_ERROR);
}

TEST_F(TestScanner, GetErrorStatusSystem) {
    EXPECT_CALL(*pMockComm, sendCommand("ERRSTAT")).WillOnce(Return("OK,ERRSTAT,system"));
    EXPECT_EQ(pScanner->getErrorStatus(), ErrorStatus::SYSTEM_ERROR);
}

TEST_F(TestScanner, GetErrorStatusUpdate) {
    EXPECT_CALL(*pMockComm, sendCommand("ERRSTAT")).WillOnce(Return("OK,ERRSTAT,update"));
    EXPECT_EQ(pScanner->getErrorStatus(), ErrorStatus::UPDATE_ERROR);
}

TEST_F(TestScanner, GetErrorStatusCfg) {
    EXPECT_CALL(*pMockComm, sendCommand("ERRSTAT")).WillOnce(Return("OK,ERRSTAT,cfg"));
    EXPECT_EQ(pScanner->getErrorStatus(), ErrorStatus::SET_VALUE_ERROR);
}

TEST_F(TestScanner, GetErrorStatusIp) {
    EXPECT_CALL(*pMockComm, sendCommand("ERRSTAT")).WillOnce(Return("OK,ERRSTAT,ip"));
    EXPECT_EQ(pScanner->getErrorStatus(), ErrorStatus::DUPLICATE_IP_ERROR);
}

TEST_F(TestScanner, GetErrorStatusOver) {
    EXPECT_CALL(*pMockComm, sendCommand("ERRSTAT")).WillOnce(Return("OK,ERRSTAT,over"));
    EXPECT_EQ(pScanner->getErrorStatus(), ErrorStatus::BUFF_OVERFLOW_ERROR);
}

TEST_F(TestScanner, GetErrorStatusPlc) {
    EXPECT_CALL(*pMockComm, sendCommand("ERRSTAT")).WillOnce(Return("OK,ERRSTAT,plc"));
    EXPECT_EQ(pScanner->getErrorStatus(), ErrorStatus::PLC_LINK_ERROR);
}

TEST_F(TestScanner, GetErrorStatusProfinet) {
    EXPECT_CALL(*pMockComm, sendCommand("ERRSTAT")).WillOnce(Return("OK,ERRSTAT,profinet"));
    EXPECT_EQ(pScanner->getErrorStatus(), ErrorStatus::PROFINET_ERROR);
}

TEST_F(TestScanner, GetErrorStatusLua) {
    EXPECT_CALL(*pMockComm, sendCommand("ERRSTAT")).WillOnce(Return("OK,ERRSTAT,lua"));
    EXPECT_EQ(pScanner->getErrorStatus(), ErrorStatus::LUA_SCRIPT_ERROR);
}

TEST_F(TestScanner, GetErrorStatusHostConnect) {
    EXPECT_CALL(*pMockComm, sendCommand("ERRSTAT")).WillOnce(Return("OK,ERRSTAT,hostconnect"));
    EXPECT_EQ(pScanner->getErrorStatus(), ErrorStatus::CONNECTION_ERROR);
}

TEST_F(TestScanner, GetErrorStatusThrowsOnUnknown) {
    EXPECT_CALL(*pMockComm, sendCommand("ERRSTAT")).WillOnce(Return("OK,ERRSTAT,bogus"));
    EXPECT_THROW(pScanner->getErrorStatus(), std::runtime_error);
}

TEST_F(TestScanner, GetBusyStatusNone) {
    EXPECT_CALL(*pMockComm, sendCommand("BUSYSTAT")).WillOnce(Return("OK,BUSYSTAT,none"));
    EXPECT_EQ(pScanner->getBusyStatus(), BusyStatus::IDLE);
}

TEST_F(TestScanner, GetBusyStatusTrg) {
    EXPECT_CALL(*pMockComm, sendCommand("BUSYSTAT")).WillOnce(Return("OK,BUSYSTAT,trg"));
    EXPECT_EQ(pScanner->getBusyStatus(), BusyStatus::TRG_BUSY);
}

TEST_F(TestScanner, GetBusyStatusUpdate) {
    EXPECT_CALL(*pMockComm, sendCommand("BUSYSTAT")).WillOnce(Return("OK,BUSYSTAT,update"));
    EXPECT_EQ(pScanner->getBusyStatus(), BusyStatus::UPDATE_PROCESSING);
}

TEST_F(TestScanner, GetBusyStatusFile) {
    EXPECT_CALL(*pMockComm, sendCommand("BUSYSTAT")).WillOnce(Return("OK,BUSYSTAT,file"));
    EXPECT_EQ(pScanner->getBusyStatus(), BusyStatus::SAVING_FILE);
}

TEST_F(TestScanner, GetBusyStatusAf) {
    EXPECT_CALL(*pMockComm, sendCommand("BUSYSTAT")).WillOnce(Return("OK,BUSYSTAT,af"));
    EXPECT_EQ(pScanner->getBusyStatus(), BusyStatus::AUTO_FOCUSING);
}

TEST_F(TestScanner, GetBusyStatusThrowsOnUnknown) {
    EXPECT_CALL(*pMockComm, sendCommand("BUSYSTAT")).WillOnce(Return("OK,BUSYSTAT,bogus"));
    EXPECT_THROW(pScanner->getBusyStatus(), std::runtime_error);
}

// ─── Settings management ─────────────────────────────────────────────────────

TEST_F(TestScanner, CopyBankConfiguration) {
    EXPECT_CALL(*pMockComm, sendCommand("BCOPY,01,05")).WillOnce(Return("OK,BCOPY"));
    pScanner->copyBankConfiguration(1, 5);
}

TEST_F(TestScanner, SaveSettings) {
    EXPECT_CALL(*pMockComm, sendCommand("SAVE")).WillOnce(Return("OK,SAVE"));
    pScanner->saveSettings();
}

TEST_F(TestScanner, LoadSavedSettings) {
    EXPECT_CALL(*pMockComm, sendCommand("LOAD")).WillOnce(Return("OK,LOAD"));
    pScanner->loadSavedSettings();
}

TEST_F(TestScanner, ResetToFactorySettings) {
    EXPECT_CALL(*pMockComm, sendCommand("DFLT")).WillOnce(Return("OK,DFLT"));
    pScanner->resetToFactorySettings();
}

TEST_F(TestScanner, SaveBackupSettings) {
    EXPECT_CALL(*pMockComm, sendCommand("BSAVE,1")).WillOnce(Return("OK,BSAVE"));
    pScanner->saveBackupSettings(1);
}

TEST_F(TestScanner, SaveBackupSettingsHighNumber) {
    EXPECT_CALL(*pMockComm, sendCommand("BSAVE,256")).WillOnce(Return("OK,BSAVE"));
    pScanner->saveBackupSettings(256);
}

TEST_F(TestScanner, LoadBackupSettings) {
    EXPECT_CALL(*pMockComm, sendCommand("BLOAD,1")).WillOnce(Return("OK,BLOAD"));
    pScanner->loadBackupSettings(1);
}

TEST_F(TestScanner, LoadBackupSettingsHighNumber) {
    EXPECT_CALL(*pMockComm, sendCommand("BLOAD,256")).WillOnce(Return("OK,BLOAD"));
    pScanner->loadBackupSettings(256);
}

// ─── Error clearing ──────────────────────────────────────────────────────────

TEST_F(TestScanner, ClearFTPCommsError) {
    EXPECT_CALL(*pMockComm, sendCommand("HCLR")).WillOnce(Return("OK,HCLR"));
    pScanner->clearFTPCommsError();
}

TEST_F(TestScanner, ClearPLCLinkError) {
    EXPECT_CALL(*pMockComm, sendCommand("PCLR")).WillOnce(Return("OK,PCLR"));
    pScanner->clearPLCLinkError();
}

}  // namespace OpenSRX
