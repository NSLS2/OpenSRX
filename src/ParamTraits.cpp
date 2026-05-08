#include "OpenSRX/ParamTraits.hpp"

#include <sstream>
#include <stdexcept>

namespace OpenSRX {

// ─── String types (HEX, ASCII) ──────────────────────────────────────────────

template <>
std::string parseParam<ParamType::HEX>(const std::string& raw) {
    return raw;
}

template <>
std::string formatParam<ParamType::HEX>(const std::string& value) {
    return value;
}

template <>
std::string parseParam<ParamType::ASCII>(const std::string& raw) {
    return raw;
}

template <>
std::string formatParam<ParamType::ASCII>(const std::string& value) {
    return value;
}

// ─── INT_VECTOR (colon-separated integers) ───────────────────────────────────

template <>
std::vector<int> parseParam<ParamType::INT_VECTOR>(const std::string& raw) {
    std::vector<int> result;
    std::istringstream ss(raw);
    std::string token;
    while (std::getline(ss, token, ':')) {
        result.push_back(std::stoi(token));
    }
    return result;
}

template <>
std::string formatParam<ParamType::INT_VECTOR>(const std::vector<int>& value) {
    std::string result;
    for (size_t i = 0; i < value.size(); ++i) {
        if (i > 0) result += ':';
        result += std::to_string(value[i]);
    }
    return result;
}

// ─── Raw integer ────────────────────────────────────────────────────────────

template <>
int parseParam<ParamType::INT>(const std::string& raw) {
    return std::stoi(raw);
}

template <>
std::string formatParam<ParamType::INT>(const int& value) {
    return std::to_string(value);
}

// ─── Enum helpers ───────────────────────────────────────────────────────────
// All enum-typed parameters are integers on the wire; we just cast.

#define OPENSRX_DEFINE_ENUM_PARAM(PT, EnumType)                            \
    template <>                                                            \
    EnumType parseParam<ParamType::PT>(const std::string& raw) {           \
        return static_cast<EnumType>(std::stoi(raw));                      \
    }                                                                      \
    template <>                                                            \
    std::string formatParam<ParamType::PT>(const EnumType& value) {        \
        return std::to_string(static_cast<int>(value));                    \
    }

OPENSRX_DEFINE_ENUM_PARAM(TOGGLE, Toggle)
OPENSRX_DEFINE_ENUM_PARAM(INTERNAL_LIGHTING_TYPE, InternalLightingType)
OPENSRX_DEFINE_ENUM_PARAM(CONTRAST_ADJUSTMENT, ContrastAdjustment)
OPENSRX_DEFINE_ENUM_PARAM(FILTER_TYPE, FilterType)
OPENSRX_DEFINE_ENUM_PARAM(OUTPUT_DIRECTION, OutputDirection)
OPENSRX_DEFINE_ENUM_PARAM(LENGTH_LIMITATION_METHOD, LengthLimitationMethod)
OPENSRX_DEFINE_ENUM_PARAM(GRID_CORRECTION, GridCorrection)
OPENSRX_DEFINE_ENUM_PARAM(PDF417_READ_CODE_TYPE, PDF417ReadCodeType)
OPENSRX_DEFINE_ENUM_PARAM(NW7_START_STOP_LETTER_TYPE, NW7StartStopLetterType)
OPENSRX_DEFINE_ENUM_PARAM(NW7_CHECK_DIGIT_TYPE, NW7CheckDigitType)
OPENSRX_DEFINE_ENUM_PARAM(UPC_A_OUTPUT, UpcAOutput)
OPENSRX_DEFINE_ENUM_PARAM(INVERSE_MODE, InverseMode)
OPENSRX_DEFINE_ENUM_PARAM(REVERSE_MODE, ReverseMode)
OPENSRX_DEFINE_ENUM_PARAM(PHARMACODE_READ_DIRECTION, PharmacodeReadDirection)
OPENSRX_DEFINE_ENUM_PARAM(IN_TERMINAL_FUNCTION, InTerminalFunction)
OPENSRX_DEFINE_ENUM_PARAM(TEST_MODE_ASSIGNMENT, TestModeAssignment)
OPENSRX_DEFINE_ENUM_PARAM(INPUT_POLARITY, InputPolarity)
OPENSRX_DEFINE_ENUM_PARAM(INPUT_PULSE_WIDTH, InputPulseWidth)
OPENSRX_DEFINE_ENUM_PARAM(STARTUP_TEST_MODE, StartupTestMode)
OPENSRX_DEFINE_ENUM_PARAM(TIMING_MODE, TimingMode)
OPENSRX_DEFINE_ENUM_PARAM(READING_MODE, ReadingMode)
OPENSRX_DEFINE_ENUM_PARAM(DATA_TRANSMISSION, DataTransmission)
OPENSRX_DEFINE_ENUM_PARAM(DUPLICATE_READING_PREVENTION, DuplicateReadingPrevention)
OPENSRX_DEFINE_ENUM_PARAM(ALTERNATE_ORDER, AlternateOrder)
OPENSRX_DEFINE_ENUM_PARAM(AUTO_POINTER_LIGHTING, AutoPointerLighting)
OPENSRX_DEFINE_ENUM_PARAM(MULTIPLE_READING_SAME_CODE, MultipleReadingSameCode)
OPENSRX_DEFINE_ENUM_PARAM(MULTI_CODE_OUTPUT_FORMAT, MultiCodeOutputFormat)
OPENSRX_DEFINE_ENUM_PARAM(IMAGE_SAVING_DESTINATION, ImageSavingDestination)
OPENSRX_DEFINE_ENUM_PARAM(IMAGE_SAVING_MODE, ImageSavingMode)
OPENSRX_DEFINE_ENUM_PARAM(IMAGE_FORMAT, ImageFormat)
OPENSRX_DEFINE_ENUM_PARAM(BINNING, Binning)
OPENSRX_DEFINE_ENUM_PARAM(TRIGGER_COMMAND_RESPONSE, TriggerCommandResponse)
OPENSRX_DEFINE_ENUM_PARAM(PRESENTATION_MODE, PresentationMode)
OPENSRX_DEFINE_ENUM_PARAM(LIVE_VIEW_DISPLAY_IMAGE, LiveViewDisplayImage)
OPENSRX_DEFINE_ENUM_PARAM(VERIFICATION_THRESHOLD, VerificationThreshold)
OPENSRX_DEFINE_ENUM_PARAM(CALIBRATION_LIGHTING, CalibrationLighting)
OPENSRX_DEFINE_ENUM_PARAM(VERIFICATION_METHOD, VerificationMethod)
OPENSRX_DEFINE_ENUM_PARAM(GRADE_EXPRESSION, GradeExpression)
OPENSRX_DEFINE_ENUM_PARAM(SORT_ORDER, SortOrder)
OPENSRX_DEFINE_ENUM_PARAM(BAUD_RATE_LOW, BaudRateLow)
OPENSRX_DEFINE_ENUM_PARAM(BAUD_RATE_HIGH, BaudRateHigh)
OPENSRX_DEFINE_ENUM_PARAM(DATA_LENGTH, DataLength)
OPENSRX_DEFINE_ENUM_PARAM(PARITY_CHECK, ParityCheck)
OPENSRX_DEFINE_ENUM_PARAM(STOP_BIT_LENGTH, StopBitLength)
OPENSRX_DEFINE_ENUM_PARAM(COMM_PROTOCOL, CommProtocol)
OPENSRX_DEFINE_ENUM_PARAM(ETHERNET_COMMAND_MODE, EthernetCommandMode)
OPENSRX_DEFINE_ENUM_PARAM(ETHERNET_DATA_CLIENT_MODE, EthernetDataClientMode)
OPENSRX_DEFINE_ENUM_PARAM(TCP_CLIENT_CONNECTION_TIMING, TCPClientConnectionTiming)
OPENSRX_DEFINE_ENUM_PARAM(PLC_PROTOCOL, PLCProtocol)
OPENSRX_DEFINE_ENUM_PARAM(FTP_CONNECTION_TIMING, FTPConnectionTiming)
OPENSRX_DEFINE_ENUM_PARAM(SUBFOLDER_NAME_METHOD, SubfolderNameMethod)
OPENSRX_DEFINE_ENUM_PARAM(MASTER_SLAVE_OPERATION, MasterSlaveOperation)

#undef OPENSRX_DEFINE_ENUM_PARAM

}  // namespace OpenSRX
