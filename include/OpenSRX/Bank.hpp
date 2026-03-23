#pragma once

#include "OpenSRX/API.hpp"
#include <stdexcept>
#include <string>

namespace OpenSRX {


bool isValidBankId(int id) {
    return id >= 1 && id <= 16;
}

class Bank {
public:
    Bank(int id = 1) : id(id) { if(!isValidBankId(id)) throw std::out_of_range("Bank ID must be between 1 and 16"); }
    ~Bank() = default;

private:
    int id;
    std::string name;

};

};  // namespace OpenSRX