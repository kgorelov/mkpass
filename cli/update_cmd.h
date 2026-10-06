#pragma once

#include "CLI11.hpp"
#include "config.h"

#include <string>
#include <vector>
#include <cstdint>

#ifndef MKPASS_VERSION
#define MKPASS_VERSION "0.1.0"
#endif

namespace mkpass {

struct CliReleaseAsset {
    std::string name;
    std::string download_url;
    int64_t size = 0;
};

struct CliReleaseInfo {
    std::string version;
    std::string tag_name;
    std::string release_notes;
    std::string release_url;
    std::vector<CliReleaseAsset> assets;
};

// SHA-512 calculation and verification
std::string ComputeFileSha512(const std::string& filePath);
bool VerifyChecksumSha512(const std::string& filePath, const std::string& sumsContent, const std::string& targetFileName);

// JSON release parser
bool ParseCliReleaseJson(const std::string& json, CliReleaseInfo& out);

// Optimal asset selection
CliReleaseAsset SelectCliOptimalAsset(const std::vector<CliReleaseAsset>& assets);

// Register the "update" subcommand with CLI11
CLI::App* RegisterUpdateCommand(CLI::App& app);
int RunUpdateCommand(CLI::App* update_cmd);

// Passive periodic update notice printed to stderr during regular password derivation
void MaybePrintPassiveUpdateNotification(const Config& cfg);

} // namespace mkpass
