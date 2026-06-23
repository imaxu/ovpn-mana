#pragma once
#include <string>
#include <string_view>

namespace ovpn::validators {

struct ValidationResult {
    bool valid;
    std::string reason;
};

ValidationResult validateName(std::string_view name);
ValidationResult validateServiceName(std::string_view name);
ValidationResult validateClientName(std::string_view name);
ValidationResult validateIPv4(std::string_view ip);
ValidationResult validateHostOrIP(std::string_view host);
ValidationResult validateSubnet(std::string_view subnet);
ValidationResult validatePort(int port);

}