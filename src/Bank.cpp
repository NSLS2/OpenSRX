
#include "OpenSRX/Bank.hpp"

namespace OpenSRX {

bool isValidBankId(int id) {
    return id >= 1 && id <= 16;
}

Bank::Bank(Scanner& scanner, int id) : scanner(scanner), id(id) {
    if(!isValidBankId(id)) throw std::out_of_range("Bank ID must be between 1 and 16");
}

};  // namespace OpenSRX