#pragma once
#include <cstddef>
#include <tuple>
#include <vector>

namespace OpenSRX {

/**
 * @brief Simple date/time representation used by the scanner's clock.
 */
class Timestamp {
   public:
    /**
     * @brief Construct a Timestamp with explicit components.
     *
     * @param second Seconds (0–59).
     * @param minute Minutes (0–59).
     * @param hour   Hours (0–23).
     * @param day    Day of month (1–31).
     * @param month  Month (1–12).
     * @param year   Four-digit year.
     */
    Timestamp(int second = 0, int minute = 0, int hour = 0, int day = 1, int month = 1,
              int year = 1970)
        : second(second), minute(minute), hour(hour), day(day), month(month), year(year) {}

    /// @brief Default constructor (1970-01-01 00:00:00).
    Timestamp() : Timestamp(0, 0, 0, 1, 1, 1970) {}

    /**
     * @brief Construct a Timestamp from a raw byte buffer.
     * @param buff Buffer containing encoded date/time data.
     */
    Timestamp(std::vector<std::byte> buff);
    ~Timestamp() = default;

    /**
     * @brief Return all components as a tuple.
     * @return (year, month, day, hour, minute, second).
     */
    std::tuple<int, int, int, int, int, int> asTuple() const {
        return std::make_tuple(year, month, day, hour, minute, second);
    }

    /**
     * @brief Return the date portion as a tuple.
     * @return (year, month, day).
     */
    std::tuple<int, int, int> asDate() const { return std::make_tuple(year, month, day); }

    int second;  ///< Seconds (0–59).
    int minute;  ///< Minutes (0–59).
    int hour;    ///< Hours (0–23).
    int day;     ///< Day of month (1–31).
    int month;   ///< Month (1–12).
    int year;    ///< Four-digit year.
};

}  // namespace OpenSRX
