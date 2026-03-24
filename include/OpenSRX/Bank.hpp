#pragma once

#include "OpenSRX/Scanner.hpp"
#include <stdexcept>
#include <string>

namespace OpenSRX {


bool isValidBankId(int id);

class Bank {
public:
    Bank(Scanner& scanner, int id = 1);
    ~Bank() = default;

private:
    Scanner& scanner;
    int id;
    std::string name;

};

};  // namespace OpenSRX