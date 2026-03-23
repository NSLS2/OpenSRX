#include <iostream>

#include "OpenSRX/API.hpp"

int main() {
    std::cout << "OpenSRX " << OpenSRX::GetVersion<std::string>() << '\n';
    return 0;
}
