#pragma once

#include <string>
#include <vector>

#include "OpenSRX/ParamTypeMap.hpp"

namespace OpenSRX {

// ─── Primary template (intentionally undefined) ─────────────────────────────

/// Maps a ParamType enumerator to its corresponding C++ type.
template <ParamType>
struct ParamCppType;

// ─── Non-integer wire formats ───────────────────────────────────────────────

template <>
struct ParamCppType<ParamType::HEX> {
    using type = std::string;
};

template <>
struct ParamCppType<ParamType::ASCII> {
    using type = std::string;
};

template <>
struct ParamCppType<ParamType::INT_VECTOR> {
    using type = std::vector<int>;
};

// ─── Raw integer ────────────────────────────────────────────────────────────

template <>
struct ParamCppType<ParamType::INT> {
    using type = int;
};

// ─── Enumerated integer types ───────────────────────────────────────────────

template <>
struct ParamCppType<ParamType::TOGGLE> {
    using type = Toggle;
};

template <>
struct ParamCppType<ParamType::INTERNAL_LIGHTING_TYPE> {
    using type = InternalLightingType;
};

template <>
struct ParamCppType<ParamType::CONTRAST_ADJUSTMENT> {
    using type = ContrastAdjustment;
};

template <>
struct ParamCppType<ParamType::FILTER_TYPE> {
    using type = FilterType;
};

template <>
struct ParamCppType<ParamType::OUTPUT_DIRECTION> {
    using type = OutputDirection;
};

template <>
struct ParamCppType<ParamType::LENGTH_LIMITATION_METHOD> {
    using type = LengthLimitationMethod;
};

template <>
struct ParamCppType<ParamType::GRID_CORRECTION> {
    using type = GridCorrection;
};

template <>
struct ParamCppType<ParamType::PDF417_READ_CODE_TYPE> {
    using type = PDF417ReadCodeType;
};

template <>
struct ParamCppType<ParamType::NW7_START_STOP_LETTER_TYPE> {
    using type = NW7StartStopLetterType;
};

template <>
struct ParamCppType<ParamType::NW7_CHECK_DIGIT_TYPE> {
    using type = NW7CheckDigitType;
};

template <>
struct ParamCppType<ParamType::UPC_A_OUTPUT> {
    using type = UpcAOutput;
};

template <>
struct ParamCppType<ParamType::INVERSE_MODE> {
    using type = InverseMode;
};

template <>
struct ParamCppType<ParamType::REVERSE_MODE> {
    using type = ReverseMode;
};

template <>
struct ParamCppType<ParamType::PHARMACODE_READ_DIRECTION> {
    using type = PharmacodeReadDirection;
};

template <>
struct ParamCppType<ParamType::IN_TERMINAL_FUNCTION> {
    using type = InTerminalFunction;
};

template <>
struct ParamCppType<ParamType::TEST_MODE_ASSIGNMENT> {
    using type = TestModeAssignment;
};

template <>
struct ParamCppType<ParamType::INPUT_POLARITY> {
    using type = InputPolarity;
};

template <>
struct ParamCppType<ParamType::INPUT_PULSE_WIDTH> {
    using type = InputPulseWidth;
};

template <>
struct ParamCppType<ParamType::STARTUP_TEST_MODE> {
    using type = StartupTestMode;
};

template <>
struct ParamCppType<ParamType::TIMING_MODE> {
    using type = TimingMode;
};

template <>
struct ParamCppType<ParamType::READING_MODE> {
    using type = ReadingMode;
};

template <>
struct ParamCppType<ParamType::DATA_TRANSMISSION> {
    using type = DataTransmission;
};

template <>
struct ParamCppType<ParamType::DUPLICATE_READING_PREVENTION> {
    using type = DuplicateReadingPrevention;
};

template <>
struct ParamCppType<ParamType::ALTERNATE_ORDER> {
    using type = AlternateOrder;
};

template <>
struct ParamCppType<ParamType::AUTO_POINTER_LIGHTING> {
    using type = AutoPointerLighting;
};

template <>
struct ParamCppType<ParamType::MULTIPLE_READING_SAME_CODE> {
    using type = MultipleReadingSameCode;
};

template <>
struct ParamCppType<ParamType::MULTI_CODE_OUTPUT_FORMAT> {
    using type = MultiCodeOutputFormat;
};

template <>
struct ParamCppType<ParamType::IMAGE_SAVING_DESTINATION> {
    using type = ImageSavingDestination;
};

template <>
struct ParamCppType<ParamType::IMAGE_SAVING_MODE> {
    using type = ImageSavingMode;
};

template <>
struct ParamCppType<ParamType::IMAGE_FORMAT> {
    using type = ImageFormat;
};

template <>
struct ParamCppType<ParamType::BINNING> {
    using type = Binning;
};

template <>
struct ParamCppType<ParamType::TRIGGER_COMMAND_RESPONSE> {
    using type = TriggerCommandResponse;
};

template <>
struct ParamCppType<ParamType::PRESENTATION_MODE> {
    using type = PresentationMode;
};

template <>
struct ParamCppType<ParamType::LIVE_VIEW_DISPLAY_IMAGE> {
    using type = LiveViewDisplayImage;
};

template <>
struct ParamCppType<ParamType::VERIFICATION_THRESHOLD> {
    using type = VerificationThreshold;
};

template <>
struct ParamCppType<ParamType::CALIBRATION_LIGHTING> {
    using type = CalibrationLighting;
};

template <>
struct ParamCppType<ParamType::VERIFICATION_METHOD> {
    using type = VerificationMethod;
};

template <>
struct ParamCppType<ParamType::GRADE_EXPRESSION> {
    using type = GradeExpression;
};

template <>
struct ParamCppType<ParamType::SORT_ORDER> {
    using type = SortOrder;
};

template <>
struct ParamCppType<ParamType::BAUD_RATE_LOW> {
    using type = BaudRateLow;
};

template <>
struct ParamCppType<ParamType::BAUD_RATE_HIGH> {
    using type = BaudRateHigh;
};

template <>
struct ParamCppType<ParamType::DATA_LENGTH> {
    using type = DataLength;
};

template <>
struct ParamCppType<ParamType::PARITY_CHECK> {
    using type = ParityCheck;
};

template <>
struct ParamCppType<ParamType::STOP_BIT_LENGTH> {
    using type = StopBitLength;
};

template <>
struct ParamCppType<ParamType::COMM_PROTOCOL> {
    using type = CommProtocol;
};

template <>
struct ParamCppType<ParamType::ETHERNET_COMMAND_MODE> {
    using type = EthernetCommandMode;
};

template <>
struct ParamCppType<ParamType::ETHERNET_DATA_CLIENT_MODE> {
    using type = EthernetDataClientMode;
};

template <>
struct ParamCppType<ParamType::TCP_CLIENT_CONNECTION_TIMING> {
    using type = TCPClientConnectionTiming;
};

template <>
struct ParamCppType<ParamType::PLC_PROTOCOL> {
    using type = PLCProtocol;
};

template <>
struct ParamCppType<ParamType::FTP_CONNECTION_TIMING> {
    using type = FTPConnectionTiming;
};

template <>
struct ParamCppType<ParamType::SUBFOLDER_NAME_METHOD> {
    using type = SubfolderNameMethod;
};

template <>
struct ParamCppType<ParamType::MASTER_SLAVE_OPERATION> {
    using type = MasterSlaveOperation;
};

// ─── Convenience alias ──────────────────────────────────────────────────────

/// Shorthand: the C++ type for a given ParamType.
template <ParamType PT>
using ParamCppT = typename ParamCppType<PT>::type;

// ─── Parsing / formatting helpers ───────────────────────────────────────────

/// Parse a raw response string into the C++ type for a given ParamType.
template <ParamType PT>
ParamCppT<PT> parseParam(const std::string& raw);

/// Format a C++ value into the string to send as a parameter value.
template <ParamType PT>
std::string formatParam(const ParamCppT<PT>& value);

}  // namespace OpenSRX
