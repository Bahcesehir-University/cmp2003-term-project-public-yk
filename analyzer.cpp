#include "analyzer.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <array>
#include <algorithm>
#include <cctype>


namespace {
    static std::unordered_map<std::string, long long> zoneCounts;
    static std::unordered_map<std::string, std::array<long long, 24>> zoneHourCounts;
}

void TripAnalyzer::ingestFile(const std::string& csvPath) {
    zoneCounts.clear();
    zoneHourCounts.clear();
    std::ifstream in(csvPath);
    if (!in.is_open()) {
        return;
    }
    std::string line;
    auto isHeader = [](const std::string& l) {
        return l.rfind("TripID", 0) == 0;
    };
    if (std::getline(in, line)) {
        if (!isHeader(line)) {
            std::stringstream ss(line);
            std::vector<std::string> cols;
            cols.reserve(6);
            std::string tok;
            while (std::getline(ss, tok, ',')) cols.push_back(tok);
            if (cols.size() >= 6) {
                const std::string &zone = cols[1];
                const std::string &datetime = cols[3];
                if (!zone.empty() && !datetime.empty()) {
                    size_t pos = datetime.find(' ');
                    if (pos != std::string::npos) {
                        std::string timePart = datetime.substr(pos + 1);
                        if (timePart.size() >= 2) {
                            char c0 = timePart[0];
                            char c1 = timePart[1];
                            if (std::isdigit(c0) && std::isdigit(c1)) {
                                int hour = (c0 - '0') * 10 + (c1 - '0');
                                if (hour >= 0 && hour <= 23) {
                                    zoneCounts[zone]++;
                                    auto it = zoneHourCounts.find(zone);
                                    if (it == zoneHourCounts.end()) {
                                        std::array<long long, 24> arr{};
                                        arr.fill(0);
                                        arr[hour] = 1;
                                        zoneHourCounts.emplace(zone, arr);
                                    } else {
                                        it->second[hour]++;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    while (std::getline(in, line)) {
        std::stringstream ss(line);
        std::vector<std::string> cols;
        cols.reserve(6);
        std::string tok;
        while (std::getline(ss, tok, ',')) cols.push_back(tok);
        // A well‑formed record should have at least 6 columns
        if (cols.size() < 6) continue;
        const std::string &zone = cols[1];
        const std::string &datetime = cols[3];
        if (zone.empty() || datetime.empty()) continue;
        size_t pos = datetime.find(' ');
        if (pos == std::string::npos) continue;
        std::string timePart = datetime.substr(pos + 1);
        if (timePart.size() < 2) continue;
        char c0 = timePart[0];
        char c1 = timePart[1];
        if (!std::isdigit(c0) || !std::isdigit(c1)) continue;
        int hour = (c0 - '0') * 10 + (c1 - '0');
        if (hour < 0 || hour > 23) continue;
        zoneCounts[zone]++;
        auto it = zoneHourCounts.find(zone);
        if (it == zoneHourCounts.end()) {
            std::array<long long, 24> arr{};
            arr.fill(0);
            arr[hour] = 1;
            zoneHourCounts.emplace(zone, arr);
        } else {
            it->second[hour]++;
        }
    }
}

std::vector<ZoneCount> TripAnalyzer::topZones(int k) const {
    std::vector<ZoneCount> result;
    result.reserve(zoneCounts.size());
    for (const auto &kv : zoneCounts) {
        result.push_back({kv.first, kv.second});
    }
    std::sort(result.begin(), result.end(), [](const ZoneCount &a, const ZoneCount &b) {
        if (a.count != b.count) return a.count > b.count;
        return a.zone < b.zone;
    });
    if (k < 0) k = 0;
    if (static_cast<size_t>(k) < result.size()) {
        result.resize(static_cast<size_t>(k));
    }
    return result;
}

std::vector<SlotCount> TripAnalyzer::topBusySlots(int k) const {
    std::vector<SlotCount> result;
    result.reserve(zoneHourCounts.size() * 24);
    for (const auto &kv : zoneHourCounts) {
        const std::string &zone = kv.first;
        const auto &arr = kv.second;
        for (int h = 0; h < 24; ++h) {
            long long c = arr[h];
            if (c > 0) {
                result.push_back({zone, h, c});
            }
        }
    }
    std::sort(result.begin(), result.end(), [](const SlotCount &a, const SlotCount &b) {
        if (a.count != b.count) return a.count > b.count;
        if (a.zone != b.zone) return a.zone < b.zone;
        return a.hour < b.hour;
    });
    if (k < 0) k = 0;
    if (static_cast<size_t>(k) < result.size()) {
        result.resize(static_cast<size_t>(k));
    }
    return result;
}
