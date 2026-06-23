#pragma once
#include <string>
#include <vector>
#include "ovpn-mana/ovpn_mana_types.h"

namespace ovpn::cli {

namespace color {
    constexpr const char* RED    = "\033[31m";
    constexpr const char* GREEN  = "\033[32m";
    constexpr const char* YELLOW = "\033[33m";
    constexpr const char* BLUE   = "\033[34m";
    constexpr const char* CYAN   = "\033[36m";
    constexpr const char* RESET  = "\033[0m";
    constexpr const char* BOLD   = "\033[1m";
}

std::string humanReadableBytes(unsigned long long bytes);

std::string renderBanner(std::string_view title, std::string_view version, int width = 70);

std::string renderServiceTable(const std::vector<ovpn_service_t>& services);

std::string renderClientTable(const std::vector<ovpn_client_t>& clients, int totalCount);

}