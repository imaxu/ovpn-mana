#include "core/validators.hpp"
#include <cctype>
#include <cstdint>
#include <algorithm>

namespace ovpn::validators {

static bool isForbiddenChar(char c) {
    switch (c) {
        case ';': case '$': case '\\': case '|': case '&':
        case '\'': case '"': case '`':
            return true;
        default:
            return !std::isprint(static_cast<unsigned char>(c));
    }
}

ValidationResult validateName(std::string_view name) {
    if (name.empty()) {
        return {false, "Name cannot be empty"};
    }
    if (name.size() > 64) {
        return {false, "Name too long (max 64 characters)"};
    }
    if (!std::isalnum(static_cast<unsigned char>(name[0]))) {
        return {false, "Name must start with alphanumeric character"};
    }
    for (char c : name) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '.' && c != '-') {
            return {false, "Name contains forbidden character '" + std::string(1, c) + "'"};
        }
        if (isForbiddenChar(c)) {
            return {false, "Name contains forbidden character '" + std::string(1, c) + "'"};
        }
    }
    if (name.find("../") != std::string_view::npos || name.find("./") == 0) {
        return {false, "Name contains path traversal pattern"};
    }
    return {true, ""};
}

ValidationResult validateServiceName(std::string_view name) {
    auto baseResult = validateName(name);
    if (!baseResult.valid) {
        return baseResult;
    }
    static const std::string_view suffix = "-server";
    if (name.size() >= suffix.size() && name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0) {
        return {false, "Service name cannot end with '-server' suffix"};
    }
    return {true, ""};
}

ValidationResult validateClientName(std::string_view name) {
    return validateName(name);
}

ValidationResult validateIPv4(std::string_view ip) {
    if (ip.empty() || ip.size() < 7 || ip.size() > 15) {
        return {false, "Invalid IPv4 length"};
    }
    int dots = 0;
    int start = 0;
    for (size_t i = 0; i <= ip.size(); ++i) {
        if (i == ip.size() || ip[i] == '.') {
            if (i == start) {
                return {false, "Empty segment in IPv4"};
            }
            if (i - start > 3) {
                return {false, "IPv4 segment too long"};
            }
            if (i - start > 1 && ip[start] == '0') {
                return {false, "Leading zero not allowed in IPv4"};
            }
            int val = 0;
            for (int j = start; j < static_cast<int>(i); ++j) {
                if (!std::isdigit(static_cast<unsigned char>(ip[j]))) {
                    return {false, "Non-digit character in IPv4"};
                }
                val = val * 10 + (ip[j] - '0');
            }
            if (val < 0 || val > 255) {
                return {false, "IPv4 segment out of range 0-255"};
            }
            start = static_cast<int>(i) + 1;
            dots++;
        }
    }
    if (dots != 4) {
        return {false, "IPv4 must have exactly 4 segments"};
    }
    return {true, ""};
}

ValidationResult validateSubnet(std::string_view subnet) {
    size_t slashPos = subnet.find('/');
    if (slashPos == std::string_view::npos) {
        return {false, "Subnet must be in CIDR format x.x.x.x/prefix"};
    }
    auto ipPart = subnet.substr(0, slashPos);
    auto prefixPart = subnet.substr(slashPos + 1);
    auto ipResult = validateIPv4(ipPart);
    if (!ipResult.valid) {
        return {false, "Invalid subnet: " + ipResult.reason};
    }
    int prefix = 0;
    for (char c : prefixPart) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return {false, "Non-digit prefix in CIDR"};
        }
        prefix = prefix * 10 + (c - '0');
    }
    if (prefix < 8 || prefix > 30) {
        return {false, "CIDR prefix must be between 8 and 30"};
    }
    uint32_t addr = 0;
    int octet = 0;
    int current = 0;
    for (char c : ipPart) {
        if (c == '.') {
            addr = (addr << 8) | static_cast<uint32_t>(current);
            current = 0;
            octet++;
        } else {
            current = current * 10 + (c - '0');
        }
    }
    addr = (addr << 8) | static_cast<uint32_t>(current);
    uint32_t mask = ~((1U << (32 - prefix)) - 1);
    if ((addr & ~mask) != 0) {
        return {false, "Network address has non-zero host bits"};
    }
    return {true, ""};
}

ValidationResult validatePort(int port) {
    if (port < 1024 || port > 65535) {
        return {false, "Port must be between 1024 and 65535"};
    }
    return {true, ""};
}

ValidationResult validateHostOrIP(std::string_view host) {
    if (host.empty()) {
        return {false, "Host cannot be empty"};
    }
    if (host.size() > 253) {
        return {false, "Host too long (max 253 characters)"};
    }
    for (char c : host) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '.' && c != '-' && c != '_') {
            return {false, "Host contains forbidden character '" + std::string(1, c) + "'"};
        }
        if (isForbiddenChar(c)) {
            return {false, "Host contains forbidden character '" + std::string(1, c) + "'"};
        }
    }
    if (host[0] == '.' || host.back() == '.') {
        return {false, "Host cannot start or end with '.'"};
    }
    if (host.find("..") != std::string_view::npos) {
        return {false, "Host contains consecutive dots"};
    }
    return {true, ""};
}

}