#include "cli/console_renderer.hpp"
#include "ovpn-mana/ovpn_mana_version.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstring>

namespace ovpn::cli {

std::string humanReadableBytes(unsigned long long bytes)
{
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIdx = 0;
    double val = static_cast<double>(bytes);
    while (val >= 1024.0 && unitIdx < 4)
    {
        val /= 1024.0;
        ++unitIdx;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(unitIdx == 0 ? 0 : 1) << val << " " << units[unitIdx];
    return oss.str();
}

static std::string drawLine(const std::string& left, const std::string& mid, const std::string& right, const std::string& fill, int width)
{
    std::string line;
    line.reserve(width * 3 + 16);
    line += left;
    for (int i = 1; i < width - 1; ++i) line += fill;
    line += right;
    line += '\n';
    return line;
}

static std::string centerText(std::string_view text, int width)
{
    int pad = (width - 2 - static_cast<int>(text.size())) / 2;
    if (pad < 0) pad = 0;
    std::string result;
    result.reserve(width * 3 + 16);
    result += "\xE2\x94\x82";
    result.append(pad, ' ');
    result += text;
    result.append(width - 2 - pad - static_cast<int>(text.size()), ' ');
    result += "\xE2\x94\x82";
    result += '\n';
    return result;
}

std::string renderBanner(std::string_view title, std::string_view version, int width)
{
    static const std::string kH = "\xE2\x94\x80";
    static const std::string kTL = "\xE2\x94\x8C";
    static const std::string kTR = "\xE2\x94\x90";
    static const std::string kL = "\xE2\x94\x9C";
    static const std::string kR = "\xE2\x94\xA4";
    static const std::string kBL = "\xE2\x94\x94";
    static const std::string kBR = "\xE2\x94\x98";

    std::ostringstream oss;
    std::string top    = drawLine(kTL, kH, kTR, kH, width);
    std::string sep    = drawLine(kL, kH, kR, kH, width);
    std::string bottom = drawLine(kBL, kH, kBR, kH, width);

    oss << top;
    oss << centerText(title, width);
    oss << sep;
    oss << centerText(std::string("Version: ") + std::string(version), width);
    oss << bottom;
    return oss.str();
}

std::string renderServiceTable(const std::vector<ovpn_service_t>& services)
{
    if (services.empty()) return std::string(color::YELLOW) + "No services found." + std::string(color::RESET) + "\n";

    static const std::string kH = "\xE2\x94\x80";
    static const std::string kTL = "\xE2\x94\x8C";
    static const std::string kTR = "\xE2\x94\x90";
    static const std::string kL = "\xE2\x94\x9C";
    static const std::string kR = "\xE2\x94\xA4";
    static const std::string kBL = "\xE2\x94\x94";
    static const std::string kBR = "\xE2\x94\x98";
    static const std::string kV = "\xE2\x94\x82";

    const int colName = 24;
    const int colStatus = 10;
    const int colPort = 8;
    const int colSubnet = 18;
    const int colPath = 36;
    const int totalWidth = colName + colStatus + colPort + colSubnet + colPath + 5 * 3 + 1;

    std::ostringstream oss;

    auto titleLine = [&]() {
        std::ostringstream o;
        o << color::BOLD << color::CYAN << "Services" << color::RESET << "\n";
        return o.str();
    };
    auto top    = drawLine(kTL, kH, kTR, kH, totalWidth);
    auto headerSep = drawLine(kL, kH, kR, kH, totalWidth);
    auto bottom = drawLine(kBL, kH, kBR, kH, totalWidth);

    auto header = [&](const std::string& h1, const std::string& h2, const std::string& h3, const std::string& h4, const std::string& h5) {
        std::ostringstream o;
        o << kV << ' ';
        o << std::left << std::setw(colName) << h1 << " " << kV << " ";
        o << std::left << std::setw(colStatus) << h2 << " " << kV << " ";
        o << std::left << std::setw(colPort) << h3 << " " << kV << " ";
        o << std::left << std::setw(colSubnet) << h4 << " " << kV << " ";
        o << std::left << std::setw(colPath) << h5 << " " << kV << "\n";
        return o.str();
    };
    auto row = [&](const std::string& c1, const std::string& c2, const std::string& c3, const std::string& c4, const std::string& c5) {
        std::ostringstream o;
        o << kV << ' ';
        o << std::left << std::setw(colName) << c1 << " " << kV << " ";
        o << std::left << std::setw(colStatus) << c2 << " " << kV << " ";
        o << std::left << std::setw(colPort) << c3 << " " << kV << " ";
        o << std::left << std::setw(colSubnet) << c4 << " " << kV << " ";
        o << std::left << std::setw(colPath) << c5 << " " << kV << "\n";
        return o.str();
    };

    oss << titleLine();
    oss << top;
    oss << header("Name", "Status", "Port", "Subnet", "Config Path");
    oss << headerSep;

    for (const auto& svc : services)
    {
        std::string status = svc.is_activated
            ? std::string(color::GREEN) + "ACTIVE" + std::string(color::RESET)
            : std::string(color::RED) + "STOPPED" + std::string(color::RESET);
        oss << row(svc.name, status, std::to_string(svc.port), svc.subnet, svc.configPath);
    }
    oss << bottom;
    return oss.str();
}

std::string renderClientTable(const std::vector<ovpn_client_t>& clients, int totalCount)
{
    static const std::string kH = "\xE2\x94\x80";
    static const std::string kTL = "\xE2\x94\x8C";
    static const std::string kTR = "\xE2\x94\x90";
    static const std::string kL = "\xE2\x94\x9C";
    static const std::string kR = "\xE2\x94\xA4";
    static const std::string kBL = "\xE2\x94\x94";
    static const std::string kBR = "\xE2\x94\x98";
    static const std::string kV = "\xE2\x94\x82";

    int colName = 4;
    int colPrivateIP = 10;
    int colPublicIP = 9;
    int colSince = 5;
    int colRx = 2;
    int colTx = 2;

    for (const auto& c : clients) {
        colName      = std::max(colName,      static_cast<int>(strlen(c.name)));
        colPrivateIP = std::max(colPrivateIP, static_cast<int>(strlen(c.private_ipv4)));
        colPublicIP  = std::max(colPublicIP,  static_cast<int>(strlen(c.public_ipv4)));
        colSince     = std::max(colSince,     static_cast<int>(strlen(c.since)));
        std::string rx = humanReadableBytes(c.bytes_received);
        std::string tx = humanReadableBytes(c.bytes_sent);
        colRx = std::max(colRx, static_cast<int>(rx.size()));
        colTx = std::max(colTx, static_cast<int>(tx.size()));
    }

    colName += 2;
    colPrivateIP += 2;
    colPublicIP += 2;
    colSince += 2;
    colRx += 2;
    colTx += 2;

    const int totalWidth = colName + colPrivateIP + colPublicIP + colSince + colRx + colTx + 6 * 3 + 1;

    std::ostringstream oss;

    auto top    = drawLine(kTL, kH, kTR, kH, totalWidth);
    auto headerSep = drawLine(kL, kH, kR, kH, totalWidth);
    auto bottom = drawLine(kBL, kH, kBR, kH, totalWidth);

    auto header = [&]() {
        std::ostringstream o;
        o << kV << ' ';
        o << std::left << std::setw(colName) << "Name" << " " << kV << " ";
        o << std::left << std::setw(colPrivateIP) << "Private IP" << " " << kV << " ";
        o << std::left << std::setw(colPublicIP) << "Public IP" << " " << kV << " ";
        o << std::left << std::setw(colSince) << "Since" << " " << kV << " ";
        o << std::left << std::setw(colRx) << "Rx" << " " << kV << " ";
        o << std::left << std::setw(colTx) << "Tx" << " " << kV << "\n";
        return o.str();
    };
    auto row = [&](const ovpn_client_t& c) {
        std::ostringstream o;
        o << kV << ' ';
        o << std::left << std::setw(colName) << c.name << " " << kV << " ";
        o << std::left << std::setw(colPrivateIP) << c.private_ipv4 << " " << kV << " ";
        o << std::left << std::setw(colPublicIP) << c.public_ipv4 << " " << kV << " ";
        o << std::left << std::setw(colSince) << c.since << " " << kV << " ";
        o << std::left << std::setw(colRx) << humanReadableBytes(c.bytes_received) << " " << kV << " ";
        o << std::left << std::setw(colTx) << humanReadableBytes(c.bytes_sent) << " " << kV << "\n";
        return o.str();
    };

    std::string summary = std::string(color::BOLD) + color::CYAN + "Online Clients"
        + std::string(color::RESET) + "  (Total: " + std::to_string(totalCount)
        + ", Online: " + std::to_string(clients.size()) + ")";

    oss << summary << "\n";

    if (clients.empty())
    {
        oss << color::YELLOW << "No online clients." << color::RESET << "\n";
        return oss.str();
    }

    oss << top;
    oss << header();
    oss << headerSep;

    for (const auto& c : clients)
    {
        oss << row(c);
    }
    oss << bottom;
    oss << "\n" << summary << "\n";
    return oss.str();
}

}