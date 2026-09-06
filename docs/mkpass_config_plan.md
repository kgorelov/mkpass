# Technical Implementation Plan: mkpass Configuration System

This document outlines the detailed technical implementation plan for introducing persistent user configuration to `mkpass`, based on the requirements defined in [`mkpass_config_idea.md`](file:///home/kgorelov/git/mkpass/docs/mkpass_config_idea.md).

---

## 1. Executive Summary & Goals

`mkpass` is a deterministic, stateless password generator. Currently, persistent user choices exist only per-service within a local SQLite database (`~/.mkpass.db`). For new services or one-off executions, default values are hardcoded in source code or temporarily overridden via environment variables and CLI arguments.

### Objectives:
1. **Persistent Configuration**: Provide a user configuration file in TOML format at `~/.config/mkpass/mkpass.conf` to store custom default options across all invocations.
2. **Environment Variable Alignment**: Mirror all existing generator configuration environment variables (excluding `password` and `service` for security and statelessness) as persistent configuration options.
3. **CLI Configuration Management**: Implement a git-like CLI command suite:
   - `mkpass config get <key>`
   - `mkpass config set <key> <value>`
   - `mkpass config unset <key>`
   - `mkpass config print`
4. **Desktop GUI Settings Dialog**: Add a "Settings" / "Preferences" dialog to `mkpass-gui` featuring a 3-column table (Enable Tickbox, Variable Name, Variable Value) with disabled items styled in grey.
5. **Android Settings Dialog**: Add a corresponding "Settings" dialog to the Android application with equivalent 3-column table capabilities.
6. **Deprecate & Guard "Old Algorithm"**: Hide the legacy algorithm (`Algorithm::Old`) by default across CLI, GUI, and Android interfaces. Only display it if explicitly enabled via `enable_old_algorithm` (config or env) or when maintaining an existing service record that already uses it. Prohibit creating new services with the old algorithm when it is disabled.

---

## 2. Configuration Schema & Variables

### 2.1 Option Definitions

All supported options, their types, allowed values, and built-in fallbacks are specified below:

| Option Key | TOML Type | CLI / Env Equivalent | Allowed Values | Built-in Default | Description |
|---|---|---|---|---|---|
| `algorithm` | integer or string | `-a`, `--algorithm`<br>`MKPASS_ALGORITHM` | `1`..`5` or `"argon2"`, `"sha512"`, `"old"`, `"diceware"`, `"wordnet_pattern"` | `1` (`Argon2`) | Default password or passphrase derivation algorithm |
| `char_classes` | string | `-c`, `--char-classes`<br>`MKPASS_CHAR_CLASSES` | Any combination of `'1'`, `'2'`, `'3'`, `'4'`, `'5'` (e.g. `"1234"`, `"123"`) | `"1234"` | Character classes (1: lower, 2: upper, 3: digits, 4: symbols, 5: custom) |
| `custom_chars` | string | `--custom-chars`<br>`MKPASS_CUSTOM_CHARS` | Any UTF-8 / ASCII string | `""` | Custom characters included when class 5 is selected |
| `length` | integer | `-l`, `--length`<br>`MKPASS_LENGTH` | `1`..`128` (password) or `1`..`20` (passphrase) | `16` (pwd)<br>`3` (passphrase)<br>`8` (old) | Default password length or passphrase word count |
| `separator` | string | `--separator`<br>`MKPASS_SEPARATOR` | Any string (`""`, `"-"`, `" "`, `"/"`, etc.) | `""` (None) | Word separator for passphrase generation |
| `passphrase_pattern` | string | `--pattern`<br>`MKPASS_PASSPHRASE_PATTERN` | Pattern letters (`"navrn"`), index (`"1"`..), or `""` (random) | `""` (Random) | WordNet grammatical part-of-speech pattern |
| `digits` | boolean | `--digits`<br>`MKPASS_DIGITS` | `true`, `false` (`y`/`n`, `1`/`0`) | `false` | Include digits in passphrases |
| `symbols` | boolean | `--symbols`<br>`MKPASS_SYMBOLS` | `true`, `false` (`y`/`n`, `1`/`0`) | `false` | Include symbols in passphrases |
| `substitutions` | boolean | `--substitutions`<br>`MKPASS_SUBSTITUTIONS` | `true`, `false` (`y`/`n`, `1`/`0`) | `false` | Allow leet substitutions in passphrases |
| `capitalize` | boolean | `--capitalize`<br>`MKPASS_CAPITALIZE` | `true`, `false` (`y`/`n`, `1`/`0`) | `true` | Capitalize words in passphrases |
| `enable_old_algorithm` | boolean | `MKPASS_ENABLE_OLD_ALGORITHM`<br>`enable_old_algorithm` | `true`, `false` (`y`/`n`, `1`/`0`) | `false` | Enable display and creation of legacy "OldPassword" algorithm |

### 2.2 Security Exclusion Policy
`password` and `service` are explicitly excluded from configuration storage:
- Storing master passwords on disk violates `mkpass` core security model (zero stored secrets).
- Storing a default service name would encourage password reuse and defeat deterministic domain separation.

---

## 3. Precedence Hierarchy

When determining the effective parameter value during password generation, the following hierarchy applies (highest priority first):

1. **Explicit CLI Options** (e.g., `-l 24`, `-a 1`)
2. **Environment Variables** (e.g., `MKPASS_LENGTH=24`)
3. **Database Entry** (if `--service` matches a record previously saved in SQLite `~/.mkpass.db`)
4. **User Configuration File** (`~/.config/mkpass/mkpass.conf`)
5. **Application Built-in Defaults** (e.g., length `16`, algorithm `Argon2`)

*Note*: For the legacy algorithm gate (`enable_old_algorithm`), CLI flag/Env/Config grants global visibility. If global visibility is `false`, the legacy algorithm is only exposed if the matched database record already uses `Algorithm::Old`.

---

## 4. Architecture & Component Design

```
+-------------------------------------------------------------------------+
|                              Frontends                                  |
|   +-------------------+  +-------------------+  +-------------------+   |
|   |  CLI Application  |  |   Qt Desktop GUI  |  |    Android App    |   |
|   | (cli/cli.cpp)     |  | (gui/gui.cpp)     |  | (MainActivity)   |   |
|   |  mkpass config    |  |  Settings Dialog  |  |  Settings Dialog  |   |
|   +---------+---------+  +---------+---------+  +---------+---------+   |
+-------------|----------------------|----------------------|-------------+
              |                      |                      | JNI
+-------------v----------------------v----------------------v-------------+
|                          libmkpass Core                                 |
|                                                                         |
|   +-----------------------------------------------------------------+   |
|   |                     mkpass::Config Engine                       |   |
|   |   - Load / Save TOML (~/.config/mkpass/mkpass.conf)             |   |
|   |   - Typed Getters / Setters                                     |   |
|   |   - Raw String Interface (get_raw, set_raw, unset_raw, print)   |   |
|   |   - Validation & Conversion                                     |   |
|   +--------------------------------+--------------------------------+   |
|                                    |                                    |
|   +--------------------------------v--------------------------------+   |
|   |               Vendored toml++ (Single Header)                   |   |
|   |               (libmkpass/toml.hpp)                              |   |
|   +-----------------------------------------------------------------+   |
+-------------------------------------------------------------------------+
```

---

## 5. Core Engine Implementation (`libmkpass`)

### 5.1 TOML Engine: `toml++`
To preserve the zero-external-dependency model and cross-platform compatibility (Linux, Windows, Android NDK, macOS, WebAssembly), vendor the industry-standard single-header TOML parser:
- Location: `libmkpass/toml.hpp` (from `tomlplusplus` v3.4+, MIT License).
- Modern C++20 compliant, zero third-party dependencies, conforms to TOML 1.0.

### 5.2 Path Resolution & Platform Utilities
Add `GetConfigFilePath()` to [`libmkpass/platform_utils.h`](file:///home/kgorelov/git/mkpass/libmkpass/platform_utils.h):
- **Environment Override**: Check `MKPASS_CONFIG_PATH`. If defined, use it directly (critical for hermetic unit and E2E tests).
- **POSIX** ([`libmkpass/posix_ext.h`](file:///home/kgorelov/git/mkpass/libmkpass/posix_ext.h)):
  - If `$XDG_CONFIG_HOME` is set: `$XDG_CONFIG_HOME/mkpass/mkpass.conf`
  - Otherwise: `wordexp("~/.config/mkpass/mkpass.conf")`
- **Windows** ([`libmkpass/win32_ext.h`](file:///home/kgorelov/git/mkpass/libmkpass/win32_ext.h)):
  - Use `SHGetKnownFolderPath(FOLDERID_RoamingAppData)` -> `%APPDATA%\mkpass\mkpass.conf`
- **Directory Creation**:
  - Automatically create parent directories with `std::filesystem::create_directories()` when saving.

### 5.3 Config Data Structures & Class Interface
Create `libmkpass/config.h` and `libmkpass/config.cpp`:

```cpp
namespace mkpass {

struct ConfigOptions {
    std::optional<Algorithm> algorithm;
    std::optional<std::vector<CharacterClass>> char_classes;
    std::optional<std::string> custom_chars;
    std::optional<size_t> length;
    std::optional<std::string> separator;
    std::optional<std::vector<WordClasses>> passphrase_pattern;
    std::optional<bool> digits;
    std::optional<bool> symbols;
    std::optional<bool> substitutions;
    std::optional<bool> capitalize;
    std::optional<bool> enable_old_algorithm;
};

class Config {
public:
    explicit Config(std::string file_path = "");

    bool load();
    bool save();

    // Key-value inspection for CLI and Settings UI
    std::optional<std::string> get_raw(const std::string& key) const;
    void set_raw(const std::string& key, const std::string& value);
    bool unset_raw(const std::string& key);
    bool is_set(const std::string& key) const;
    std::string print() const;

    // Strongly-typed access
    const ConfigOptions& options() const { return options_; }
    void set_options(const ConfigOptions& opts);

    const std::string& path() const { return path_; }
    bool exists() const;

    // Helpers
    static std::string get_default_config_path();
    static bool is_valid_key(const std::string& key);
    static std::vector<std::string> get_all_keys();
    static std::string get_built_in_default(const std::string& key);

private:
    std::string path_;
    ConfigOptions options_;
    void parse_toml(const std::string& content);
    std::string serialize_toml() const;
};

// Global helper for checking legacy algorithm authorization
bool IsOldAlgorithmEnabled(const Config& config);

} // namespace mkpass
```

### 5.4 Build System Integration
Update build files to include `config.cpp`:
- [`libmkpass/CMakeLists.txt`](file:///home/kgorelov/git/mkpass/libmkpass/CMakeLists.txt): Add `config.cpp` to `SOURCES`.
- [`android/app/src/main/cpp/CMakeLists.txt`](file:///home/kgorelov/git/mkpass/android/app/src/main/cpp/CMakeLists.txt): Add `config.cpp` to `CPP_SOURCES`.

---

## 6. Command-Line Interface (`cli`)

### 6.1 Subcommand Suite
Integrate CLI11 subcommands into [`cli/cli.cpp`](file:///home/kgorelov/git/mkpass/cli/cli.cpp):

```
mkpass config get <key>
mkpass config set <key> <value>
mkpass config unset <key>
mkpass config print
```

#### Behavior & Exit Codes:
- `mkpass config get <key>`:
  - If `<key>` is not valid: output error to `std::cerr`, exit code `1`.
  - If `<key>` is valid but unset in config file: output nothing (or descriptive message to `std::cerr`), exit code `1`.
  - If `<key>` is present: print value to `std::cout`, exit code `0`.
- `mkpass config set <key> <value>`:
  - Validates `<key>` and value format according to target type.
  - Loads existing config, assigns key, writes updated TOML to disk.
  - Exit code `0` on success, `1` on invalid key/value or write failure.
- `mkpass config unset <key>`:
  - Validates `<key>`. If present in config file, removes entry and writes back.
  - Exit code `0`.
- `mkpass config print`:
  - Reads `mkpass.conf` and prints raw TOML to `std::cout`.
  - If file does not exist or has no entries, prints empty output and exits `0`.

### 6.2 Generator Precedence Integration
In `run_cli()`:
1. Initialize `mkpass::Config cfg(GetConfigFilePath()); cfg.load();`.
2. When parsing CLI/Env, if an option is not set via CLI or Env:
   - For algorithm: `Algorithm default_algo = known ? db_entry->algorithm : cfg.options().algorithm.value_or(Algorithm::Argon2);`
   - For password length: `cfg.options().length.value_or(16)`
   - For character classes: `cfg.options().char_classes.value_or(...)`
   - For passphrase settings: `cfg.options().separator`, `cfg.options().digits`, etc.

### 6.3 Legacy Algorithm Enforcement
1. Check if `Algorithm::Old` is globally active:
   ```cpp
   bool old_enabled = cfg.options().enable_old_algorithm.value_or(false);
   if (const char* env1 = std::getenv("MKPASS_ENABLE_OLD_ALGORITHM")) {
       old_enabled = (std::string(env1) == "1" || std::string(env1) == "true");
   } else if (const char* env2 = std::getenv("enable_old_algorithm")) {
       old_enabled = (std::string(env2) == "1" || std::string(env2) == "true");
   }
   ```
2. In `AskForAlgorithm()`:
   - If `!old_enabled && (!known || db_entry->algorithm != Algorithm::Old)`:
     Omit `OldPassword` from the displayed interactive choices.
3. If user attempts to create a new service with `-a 3` or `--algorithm 3` while `!old_enabled`:
   - Throw `std::runtime_error("Legacy algorithm 'OldPassword' is disabled. Set 'enable_old_algorithm = true' in config or environment to enable.")`.

---

## 7. Qt Desktop GUI (`gui`)

### 7.1 Menu Bar & Settings Action
In [`gui/gui.cpp`](file:///home/kgorelov/git/mkpass/gui/gui.cpp):
- Add a new menu:
  ```cpp
  QMenu *settingsMenu = menuBar->addMenu("Settings");
  QAction *preferencesAction = settingsMenu->addAction("Preferences...");
  preferencesAction->setShortcut(QKeySequence::Preferences);
  connect(preferencesAction, &QAction::triggered, this, &MainWindow::showSettings);
  ```

### 7.2 Settings Dialog UI (`SettingsDialog`)
Create `gui/settings_dialog.h` and `gui/settings_dialog.cpp`:
- Modal `QDialog` containing a `QTableWidget` with 3 columns:
  1. **Column 0: Enabled (Tickbox)**
     - `QTableWidgetItem` with `Qt::ItemIsUserCheckable`.
     - Toggling state triggers visual update:
       - Checked: row text color is normal (`QPalette::Text`), value editor is enabled.
       - Unchecked: row text color is grey (`Qt::gray`), value editor is disabled (`setEnabled(false)`).
  2. **Column 1: Variable Name**
     - Non-editable text displaying the option name (`algorithm`, `length`, `char_classes`, etc.).
  3. **Column 2: Default Value**
     - Cell editors tailored to types:
       - `algorithm`: `QComboBox` (`Argon2`, `SHA512 HMAC`, `Diceware`, `Wordnet Pattern`, and `OldPassword` if enabled)
       - `length`: `QSpinBox` (range `1`..`128`)
       - `char_classes`: `QLineEdit` (validated against `1`..`5`)
       - `separator`: `QComboBox` (`None`, `Hyphen (-)`, `Space ( )`, `Slash (/)`)
       - `passphrase_pattern`: `QLineEdit`
       - `digits`, `symbols`, `substitutions`, `capitalize`, `enable_old_algorithm`: `QComboBox` (`true`, `false`)
- **Dialog Controls**:
  - `Save` Button: Writes all enabled entries to `~/.config/mkpass/mkpass.conf` and unsets disabled ones. Notifies `MainWindow`.
  - `Cancel` Button: Reverts changes and closes.
  - `Restore Defaults` Button: Sets all fields to application defaults.

### 7.3 Dynamic Legacy Algorithm Gating in Qt GUI
In [`gui/gui.cpp`](file:///home/kgorelov/git/mkpass/gui/gui.cpp):
1. Create `updateAlgorithmComboBox(bool force_include_old = false)`:
   - Check if `enable_old_algorithm` is active in `Config` or environment.
   - If active or `force_include_old == true`: populate `algorithmComboBox` with all 5 algorithms.
   - Otherwise: populate `algorithmComboBox` with only the 4 modern algorithms (omit `Algorithm::Old`).
2. In `serviceChanged(const QString &service)`:
   - Check if `service` exists in DB.
   - If record exists and `entry->algorithm == Algorithm::Old`:
     Call `updateAlgorithmComboBox(true)` and select `Algorithm::Old`.
   - If record does not exist or uses a modern algorithm:
     Call `updateAlgorithmComboBox(false)` to ensure `Algorithm::Old` cannot be picked for new services.

---

## 8. Android Application (`android`)

### 8.1 JNI Layer (`native-lib.cpp`)
Extend [`android/app/src/main/cpp/native-lib.cpp`](file:///home/kgorelov/git/mkpass/android/app/src/main/cpp/native-lib.cpp):
- Maintain a global `std::unique_ptr<mkpass::Config> config;`.
- Add JNI functions:
  ```cpp
  void Java_app_mkpass_MainActivity_initConfig(JNIEnv *env, jobject thiz, jstring configPath);
  jstring Java_app_mkpass_MainActivity_getConfigValue(JNIEnv *env, jobject thiz, jstring key);
  void Java_app_mkpass_MainActivity_setConfigValue(JNIEnv *env, jobject thiz, jstring key, jstring val);
  void Java_app_mkpass_MainActivity_unsetConfigValue(JNIEnv *env, jobject thiz, jstring key);
  jboolean Java_app_mkpass_MainActivity_isConfigKeySet(JNIEnv *env, jobject thiz, jstring key);
  jboolean Java_app_mkpass_MainActivity_isOldAlgorithmEnabled(JNIEnv *env, jobject thiz);
  ```

### 8.2 Menu Integration
In [`android/app/src/main/res/menu/main_menu.xml`](file:///home/kgorelov/git/mkpass/android/app/src/main/res/menu/main_menu.xml):
- Add the `menu_settings` item:
  ```xml
  <item
      android:id="@+id/menu_settings"
      android:title="Settings"
      app:showAsAction="never" />
  ```

### 8.3 Settings Dialog UI
Create layout `dialog_settings.xml` and row item layout `item_setting_record.xml`:
- **Row Structure**:
  - `CheckBox` (`settingEnabled`): Toggles setting active/inactive.
  - `TextView` (`settingName`): Name of the variable.
  - `TextView` (`settingValue`): Value representation.
- **Visual State**:
  - Disabled state: `settingName` and `settingValue` text color set to `#888888` (grey); click listeners disabled.
  - Enabled state: standard text colors; clicking value opens an input editor (Dialog with Spinner, NumberPicker, or EditText depending on option type).
- **Dialog Actions**:
  - `Save`: Persists configuration through JNI and triggers `MainActivity` defaults refresh.
  - `Cancel`: Discards uncommitted modifications.

### 8.4 Android Algorithm Spinner Gating
In [`android/app/src/main/java/app/mkpass/MainActivity.java`](file:///home/kgorelov/git/mkpass/android/app/src/main/java/app/mkpass/MainActivity.java):
1. Make `ALGORITHMS` array dynamic via `List<AlgorithmItem>` mapping display name to Algorithm ID.
2. If `isOldAlgorithmEnabled()` is false:
   - Exclude "OldPassword" from `algorithmSpinner`.
3. In `loadServiceEntry(String serviceName)`:
   - If service entry uses `Algorithm::Old`: dynamically inject "OldPassword" into the spinner adapter and select it.
   - If moving to a new or modern service: remove "OldPassword" from the adapter if `!isOldAlgorithmEnabled()`.

---

## 9. Testing Strategy

### 9.1 Core Unit Tests (`libmkpass/config_ut.cpp`)
Add a new test suite to [`libmkpass/CMakeLists.txt`](file:///home/kgorelov/git/mkpass/libmkpass/CMakeLists.txt):
1. **ConfigParsing**: Test valid TOML parsing, handling missing files, empty files, and unknown keys.
2. **ConfigSerialization**: Test saving modified values back to TOML format and verifying round-trip equivalence.
3. **GetSetUnset**: Verify `set_raw()`, `get_raw()`, `unset_raw()`, and `is_set()`.
4. **ValidationRules**: Ensure illegal lengths (e.g. `<= 0`), invalid algorithm IDs (`> 5`), and malformed boolean strings are caught.
5. **OldAlgorithmFlag**: Test detection of `enable_old_algorithm` across config options and environment variables.

### 9.2 CLI End-to-End Tests (`cli/e2e_test.cpp`)
Extend [`cli/e2e_test.cpp`](file:///home/kgorelov/git/mkpass/cli/e2e_test.cpp) using a temporary `MKPASS_CONFIG_PATH`:
1. `mkpass config set length 32` -> verify `mkpass config get length` returns `32`.
2. `mkpass config unset length` -> verify `mkpass config get length` exits with code `1`.
3. `mkpass config print` -> verify TOML content is printed.
4. **Precedence Test**: Set config `length = 24`, generate password with `-dd` -> verify length is `24`. Then pass `-l 12` -> verify length is `12`.
5. **Old Algorithm Gating Test**:
   - Run interactive/prompt choice when `enable_old_algorithm` is not set -> verify option `3. OldPassword` is not offered.
   - Attempt `-a 3` for a new service -> verify command fails with descriptive error.
   - Set `mkpass config set enable_old_algorithm true` -> verify `-a 3` succeeds.
   - Create a service with `-a 3` while enabled; then unset `enable_old_algorithm`; generate for that service -> verify password generation succeeds for existing service record.

---

## 10. Documentation Updates

1. **[`docs/man/mkpass.1`](file:///home/kgorelov/git/mkpass/docs/man/mkpass.1)**:
   - Document `mkpass config` subcommands (`get`, `set`, `unset`, `print`).
   - Document `~/.config/mkpass/mkpass.conf` and `MKPASS_CONFIG_PATH`.
   - Document `MKPASS_ENABLE_OLD_ALGORITHM` / `enable_old_algorithm` and algorithm deprecation policy.
2. **[`docs/man/mkpass-gui.1`](file:///home/kgorelov/git/mkpass/docs/man/mkpass-gui.1)**:
   - Document the Settings/Preferences dialog under `Settings` menu.
3. **[`docs/qt_help.html`](file:///home/kgorelov/git/mkpass/docs/qt_help.html)** & **[`docs/android_help.html`](file:///home/kgorelov/git/mkpass/docs/android_help.html)**:
   - Add a dedicated "Configuring Defaults & Settings" section with descriptions of the 3-column table.
4. **[`README.md`](file:///home/kgorelov/git/mkpass/README.md)**:
   - Update feature list and configuration instructions.

---

## 11. Work Breakdown Structure & Milestones

```mermaid
graph TD
    M1["Phase 1: Core Config Engine<br>(toml++, config.h/cpp, paths, unit tests)"] --> M2["Phase 2: CLI Subcommands<br>(config get/set/unset/print, e2e tests)"]
    M1 --> M3["Phase 3: Qt GUI Settings<br>(SettingsDialog, 3-column table, Old algo gate)"]
    M1 --> M4["Phase 4: Android Settings<br>(JNI, SettingsDialog, Spinner gating)"]
    M2 --> M5["Phase 5: Docs & Verification<br>(Man pages, HTML help, full test pass)"]
    M3 --> M5
    M4 --> M5
```

- **Phase 1: Core Configuration Engine (`libmkpass`)**
  - Vendor `toml++` into `libmkpass/toml.hpp`.
  - Implement path resolution in `platform_utils.h` (`posix_ext.h`, `win32_ext.h`).
  - Implement `mkpass::Config` and `ConfigOptions` in `libmkpass/config.h` & `libmkpass/config.cpp`.
  - Add and pass unit tests in `libmkpass/config_ut.cpp`.
- **Phase 2: CLI Subcommands & Precedence Integration (`cli`)**
  - Integrate `config` subcommand into `cli/cli.cpp` with `get`, `set`, `unset`, `print`.
  - Hook configuration defaults into password generation logic.
  - Implement `enable_old_algorithm` gating logic.
  - Add and pass E2E tests in `cli/e2e_test.cpp`.
- **Phase 3: Qt Desktop GUI Settings (`gui`)**
  - Implement `SettingsDialog` (`gui/settings_dialog.h`, `gui/settings_dialog.cpp`).
  - Wire menu action in `gui/gui.cpp`.
  - Implement dynamic visibility and selection for `Algorithm::Old` in `algorithmComboBox`.
- **Phase 4: Android Settings (`android`)**
  - Implement JNI functions in `android/app/src/main/cpp/native-lib.cpp`.
  - Implement Settings menu and dialog in `MainActivity.java` with 3-column table.
  - Implement dynamic `Algorithm::Old` gating in `algorithmSpinner`.
- **Phase 5: Documentation & Polishing**
  - Update man pages and user documentation.
  - Verify complete test suite passes across all platforms.
