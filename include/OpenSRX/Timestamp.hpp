#pragma once
#include <vector>
#include <cstddef>

namespace OpenSRX {

class Timestamp {

Timestamp(int second = 0, int minute = 0, int hour = 0, int day = 1, int month = 1, int year = 2000)
    : second(second), minute(minute), hour(hour), day(day), month(month), year(year) {}
Timestamp() : Timestamp(0, 0, 0, 1, 1, 2000) {}
Timestamp(std::vector<std::byte> buff);
~Timestamp() = default;
private:
int second, minute, hour, day, month, year;
};

};  // namespace OpenSRX