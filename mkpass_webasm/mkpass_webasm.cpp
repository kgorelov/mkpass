#include <emscripten/bind.h>
#include <string>
#include <vector>
#include <cstdio>
#include "mkpass.h"
#include "context.h"
#include "algorithms.h"
#include "character_classes.h"

using namespace emscripten;

//std::string MkPassWasm(std::string password, std::string service, std::vector<CharacterClass> char_classes, Algorithm algorithm, unsigned length, std::string custom_chars) {
std::string MkPassWasm(std::string password, std::string service, std::vector<CharacterClass> char_classes, int algorithm, unsigned length, std::string custom_chars) {
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
    return MkPass(ctx);
}

EMSCRIPTEN_BINDINGS(mkpass_module) {
    enum_<Algorithm>("Algorithm")
        .value("Argon2", Algorithm::Argon2)
        .value("SlowSha512", Algorithm::SlowSha512)
        .value("Old", Algorithm::Old);

    enum_<CharacterClass>("CharacterClass")
        .value("LOWERCASE", CharacterClass::LOWERCASE)
        .value("UPPERCASE", CharacterClass::UPPERCASE)
        .value("DIGITS", CharacterClass::DIGITS)
        .value("SYMBOLS", CharacterClass::SYMBOLS)
        .value("CUSTOM", CharacterClass::CUSTOM);

    register_vector<CharacterClass>("VectorCharacterClass");

    function("MkPass", &MkPassWasm);
}
