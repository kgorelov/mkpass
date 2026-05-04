#include <emscripten/bind.h>
#include <string>
#include <vector>
#include <cstdio>
#include "mkpass.h"
#include "context.h"
#include "algorithms.h"
#include "character_classes.h"
#include "qrcode/qrcodegen.hpp"
#include "passphrase_patterns.h"
#include "db.h"

using namespace emscripten;
using qrcodegen::QrCode;

// ... MkPassWasm ...
std::string MkPassWasm(std::string password, std::string service, std::vector<CharacterClass> char_classes, int algorithm, unsigned length, std::string custom_chars, std::string separator, bool capitalize_words, std::string pattern, bool allow_substitutions) {
    Context ctx;
    ctx.password = password;
    ctx.service = service;
    ctx.char_classes = char_classes;
    ctx.algorithm = (Algorithm)algorithm;
    ctx.length = length;
    if (!custom_chars.empty()) {
        ctx.custom_chars = custom_chars;
    } else {
        ctx.custom_chars = std::nullopt;
    }
    ctx.separator = separator;
    ctx.capitalize_words = capitalize_words;
    ctx.passphrase_pattern = mkpass::StringToPattern(pattern);
    ctx.allow_substitutions = allow_substitutions;
    return MkPass(ctx);
}

std::vector<std::string> GetPassphrasePatternsWasm(int length) {
    PatternsList patterns = GetPassphrasePatterns(length);
    std::vector<std::string> result;
    for (const auto& p : patterns) {
        result.push_back(mkpass::PatternToString(p));
    }
    return result;
}

// ... QrCodeData ...
struct QrCodeData {
    int size;
    std::vector<bool> data;
};

QrCodeData GenerateQrCode(std::string text) {
    QrCode qr = QrCode::encodeText(text.c_str(), QrCode::Ecc::LOW);
    QrCodeData result;
    result.size = qr.getSize();
    for (int y = 0; y < result.size; y++) {
        for (int x = 0; x < result.size; x++) {
            result.data.push_back(qr.getModule(x, y));
        }
    }
    return result;
}

EMSCRIPTEN_BINDINGS(mkpass_module) {
    enum_<Algorithm>("Algorithm")
        .value("Argon2", Algorithm::Argon2)
        .value("SlowSha512", Algorithm::SlowSha512)
        .value("Old", Algorithm::Old)
        .value("Passphrase_Diceware_EFF_Large", Algorithm::Passphrase_Diceware_EFF_Large)
        .value("Passphrase_Wordnet_Pattern", Algorithm::Passphrase_Wordnet_Pattern);

    enum_<CharacterClass>("CharacterClass")
        .value("LOWERCASE", CharacterClass::LOWERCASE)
        .value("UPPERCASE", CharacterClass::UPPERCASE)
        .value("DIGITS", CharacterClass::DIGITS)
        .value("SYMBOLS", CharacterClass::SYMBOLS)
        .value("CUSTOM", CharacterClass::CUSTOM);

    register_vector<CharacterClass>("VectorCharacterClass");
    register_vector<bool>("VectorBool");
    register_vector<std::string>("VectorString");

    value_object<QrCodeData>("QrCodeData")
        .field("size", &QrCodeData::size)
        .field("data", &QrCodeData::data);

    function("MkPass", &MkPassWasm);
    function("GenerateQrCode", &GenerateQrCode);
    function("GetMaxPassphrasePatternLength", &GetMaxPassphrasePatternLength);
    function("GetPassphrasePatterns", &GetPassphrasePatternsWasm);
}
