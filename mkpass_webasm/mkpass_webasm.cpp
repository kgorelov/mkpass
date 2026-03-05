#include <emscripten/bind.h>
#include <string>
#include <vector>
#include "mkpass.h"
#include "context.h"
#include "algorithms.h"
#include "character_classes.h"

using namespace emscripten;

struct ContextWasm {
    std::string password;
    std::string service;
    std::vector<CharacterClass> char_classes;
    Algorithm algorithm = Algorithm::Argon2;
    unsigned length = 0;
    std::string custom_chars;
};

std::string MkPassWasm(const ContextWasm& ctx_wasm) {
    Context ctx;
    ctx.password = ctx_wasm.password;
    ctx.service = ctx_wasm.service;
    ctx.char_classes = ctx_wasm.char_classes;
    ctx.algorithm = ctx_wasm.algorithm;
    ctx.length = ctx_wasm.length;
    if (!ctx_wasm.custom_chars.empty()) {
        ctx.custom_chars = ctx_wasm.custom_chars;
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

    value_object<ContextWasm>("Context")
        .field("password", &ContextWasm::password)
        .field("service", &ContextWasm::service)
        .field("char_classes", &ContextWasm::char_classes)
        .field("algorithm", &ContextWasm::algorithm)
        .field("length", &ContextWasm::length)
        .field("custom_chars", &ContextWasm::custom_chars);

    function("MkPass", &MkPassWasm);
}
