const mkpass_wasm = require('./../build_wasm/mkpass_webasm.js');

async function runTests() {
    console.log("Starting WASM tests...");
    let failed = false;

    function assert(condition, message) {
        if (!condition) {
            console.error("  FAIL: " + message);
            failed = true;
        } else {
            console.log("  PASS: " + message);
        }
    }

    try {
        const module = await mkpass_wasm();
        console.log("WASM Module Loaded.");

        // Test 1: Algorithm Enum
        assert(module.Algorithm.Argon2.value === 1, "Argon2 enum value should be 1");
        assert(module.Algorithm.SlowSha512.value === 2, "SlowSha512 enum value should be 2");

        // Test 2: CharacterClass Enum
        assert(module.CharacterClass.LOWERCASE.value === 0, "LOWERCASE enum value should be 0");
        assert(module.CharacterClass.UPPERCASE.value === 1, "UPPERCASE enum value should be 1");

        // Test 3: Basic Password Generation (Argon2)
        const charClasses = new module.VectorCharacterClass();
        charClasses.push_back(module.CharacterClass.LOWERCASE);
        charClasses.push_back(module.CharacterClass.UPPERCASE);
        charClasses.push_back(module.CharacterClass.DIGITS);

        const password = module.MkPass("master", "service", charClasses, module.Algorithm.Argon2.value, 16, "");
        assert(typeof password === 'string' && password.length === 16, "Password generation should return 16-char string");

        // Ensure it's deterministic
        const passwordAgain = module.MkPass("master", "service", charClasses, module.Algorithm.Argon2.value, 16, "");
        assert(password === passwordAgain, "Password generation should be deterministic");

        charClasses.delete();

        // Test 4: QR Code Generation
        const qrData = module.GenerateQrCode("test password");
        assert(qrData.size > 0, "QR Code size should be > 0");
        assert(qrData.data.size() === qrData.size * qrData.size, "QR Code data size should match size*size");

        if (failed) {
            console.error("\nSome tests FAILED.");
            process.exit(1);
        } else {
            console.log("\nAll WASM tests PASSED.");
            process.exit(0);
        }

    } catch (e) {
        console.error("Test execution failed with error:", e);
        process.exit(1);
    }
}

runTests();
