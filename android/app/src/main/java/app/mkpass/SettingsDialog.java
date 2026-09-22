package app.mkpass;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.GridLayout;
import android.widget.Spinner;
import android.widget.TextView;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatDialog;

import java.util.ArrayList;
import java.util.List;

public class SettingsDialog extends AppCompatDialog {

    private final MainActivity activity;
    private boolean saved = false;
    private int normalTextColor = Color.BLACK;

    private static class SettingRowDef {
        final String key;
        final CheckBox checkBox;
        final TextView nameView;
        final View editorView;

        SettingRowDef(String key, CheckBox checkBox, TextView nameView, View editorView) {
            this.key = key;
            this.checkBox = checkBox;
            this.nameView = nameView;
            this.editorView = editorView;
        }
    }

    private static class AlgorithmOption {
        final String displayName;
        final String canonical;

        AlgorithmOption(String displayName, String canonical) {
            this.displayName = displayName;
            this.canonical = canonical;
        }

        @Override
        public String toString() {
            return displayName;
        }
    }

    private static class SeparatorOption {
        final String displayName;
        final String value;

        SeparatorOption(String displayName, String value) {
            this.displayName = displayName;
            this.value = value;
        }

        @Override
        public String toString() {
            return displayName;
        }
    }

    private static class PatternOption {
        final String displayName;
        final String value;

        PatternOption(String displayName, String value) {
            this.displayName = displayName;
            this.value = value;
        }

        @Override
        public String toString() {
            return displayName;
        }
    }

    private final List<SettingRowDef> rowDefs = new ArrayList<>();

    // General
    private Spinner spinnerAlgorithm;
    private EditText editLength;
    private Spinner spinnerEnableOldAlgo;

    // Password
    private GridLayout layoutCharClasses;
    private CheckBox checkCharLower;
    private CheckBox checkCharUpper;
    private CheckBox checkCharDigits;
    private CheckBox checkCharSymbols;
    private CheckBox checkCharCustom;
    private EditText editCustomChars;

    // Passphrase
    private Spinner spinnerSeparator;
    private Spinner spinnerPassphrasePattern;
    private Spinner spinnerDigits;
    private Spinner spinnerSymbols;
    private Spinner spinnerSubstitutions;
    private Spinner spinnerCapitalize;

    private final List<AlgorithmOption> algorithmList = new ArrayList<>();
    private ArrayAdapter<AlgorithmOption> algorithmAdapter;

    private final List<SeparatorOption> separatorList = new ArrayList<>();
    private ArrayAdapter<SeparatorOption> separatorAdapter;
    private int lastSeparatorSelection = 0;

    private final List<PatternOption> patternList = new ArrayList<>();
    private ArrayAdapter<PatternOption> patternAdapter;
    private int lastPatternSelection = 0;

    public SettingsDialog(MainActivity activity) {
        super(activity);
        this.activity = activity;
    }

    public boolean isSaved() {
        return saved;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setTitle("Preferences");
        setContentView(R.layout.dialog_settings);

        TypedArray typedArray = getContext().obtainStyledAttributes(new int[]{android.R.attr.textColorPrimary});
        normalTextColor = typedArray.getColor(0, Color.BLACK);
        typedArray.recycle();

        setupUI();
        loadFromConfig();
    }

    @Override
    protected void onStart() {
        super.onStart();
        if (getWindow() != null) {
            getWindow().setLayout(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            );
        }
    }

    private void setupUI() {
        // Find editors
        spinnerAlgorithm = findViewById(R.id.spinner_algorithm);
        editLength = findViewById(R.id.edit_length);
        spinnerEnableOldAlgo = findViewById(R.id.spinner_enable_old_algorithm);

        layoutCharClasses = findViewById(R.id.layout_char_classes);
        checkCharLower = findViewById(R.id.check_char_lower);
        checkCharUpper = findViewById(R.id.check_char_upper);
        checkCharDigits = findViewById(R.id.check_char_digits);
        checkCharSymbols = findViewById(R.id.check_char_symbols);
        checkCharCustom = findViewById(R.id.check_char_custom);
        editCustomChars = findViewById(R.id.edit_custom_chars);

        spinnerSeparator = findViewById(R.id.spinner_separator);
        spinnerPassphrasePattern = findViewById(R.id.spinner_passphrase_pattern);
        spinnerDigits = findViewById(R.id.spinner_digits);
        spinnerSymbols = findViewById(R.id.spinner_symbols);
        spinnerSubstitutions = findViewById(R.id.spinner_substitutions);
        spinnerCapitalize = findViewById(R.id.spinner_capitalize);

        setupBoolSpinner(spinnerEnableOldAlgo);
        setupBoolSpinner(spinnerDigits);
        setupBoolSpinner(spinnerSymbols);
        setupBoolSpinner(spinnerSubstitutions);
        setupBoolSpinner(spinnerCapitalize);

        // Algorithm adapter
        algorithmAdapter = new ArrayAdapter<>(getContext(), android.R.layout.simple_spinner_item, algorithmList);
        algorithmAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinnerAlgorithm.setAdapter(algorithmAdapter);

        spinnerEnableOldAlgo.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                updateAlgorithmChoices();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {}
        });

        // Separator adapter
        setupSeparatorSpinner();

        // Pattern adapter
        setupPatternSpinner();

        // Register row definitions
        rowDefs.clear();
        rowDefs.add(new SettingRowDef("algorithm", findViewById(R.id.check_algorithm), findViewById(R.id.name_algorithm), spinnerAlgorithm));
        rowDefs.add(new SettingRowDef("length", findViewById(R.id.check_length), findViewById(R.id.name_length), editLength));
        rowDefs.add(new SettingRowDef("enable_old_algorithm", findViewById(R.id.check_enable_old_algorithm), findViewById(R.id.name_enable_old_algorithm), spinnerEnableOldAlgo));

        rowDefs.add(new SettingRowDef("char_classes", findViewById(R.id.check_char_classes), findViewById(R.id.name_char_classes), layoutCharClasses));
        rowDefs.add(new SettingRowDef("custom_chars", findViewById(R.id.check_custom_chars), findViewById(R.id.name_custom_chars), editCustomChars));

        rowDefs.add(new SettingRowDef("separator", findViewById(R.id.check_separator), findViewById(R.id.name_separator), spinnerSeparator));
        rowDefs.add(new SettingRowDef("passphrase_pattern", findViewById(R.id.check_passphrase_pattern), findViewById(R.id.name_passphrase_pattern), spinnerPassphrasePattern));
        rowDefs.add(new SettingRowDef("digits", findViewById(R.id.check_digits), findViewById(R.id.name_digits), spinnerDigits));
        rowDefs.add(new SettingRowDef("symbols", findViewById(R.id.check_symbols), findViewById(R.id.name_symbols), spinnerSymbols));
        rowDefs.add(new SettingRowDef("substitutions", findViewById(R.id.check_substitutions), findViewById(R.id.name_substitutions), spinnerSubstitutions));
        rowDefs.add(new SettingRowDef("capitalize", findViewById(R.id.check_capitalize), findViewById(R.id.name_capitalize), spinnerCapitalize));

        for (SettingRowDef def : rowDefs) {
            def.checkBox.setOnCheckedChangeListener((buttonView, isChecked) -> updateRowAppearance(def));
        }

        Button btnRestoreDefaults = findViewById(R.id.btnRestoreDefaults);
        Button btnCancel = findViewById(R.id.btnCancel);
        Button btnSave = findViewById(R.id.btnSave);

        if (btnRestoreDefaults != null) {
            btnRestoreDefaults.setOnClickListener(v -> onRestoreDefaults());
        }
        if (btnCancel != null) {
            btnCancel.setOnClickListener(v -> dismiss());
        }
        if (btnSave != null) {
            btnSave.setOnClickListener(v -> onSave());
        }
    }

    private void setupBoolSpinner(Spinner spinner) {
        ArrayAdapter<String> adapter = new ArrayAdapter<>(getContext(), android.R.layout.simple_spinner_item, new String[]{"false", "true"});
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinner.setAdapter(adapter);
    }

    private String getBoolSpinnerValue(Spinner spinner) {
        Object item = spinner.getSelectedItem();
        return item != null ? item.toString() : "false";
    }

    private void setBoolSpinnerValue(Spinner spinner, String val) {
        spinner.setSelection("true".equalsIgnoreCase(val) ? 1 : 0);
    }

    private void setupSeparatorSpinner() {
        separatorList.clear();
        separatorList.add(new SeparatorOption("None", ""));
        separatorList.add(new SeparatorOption("Hyphen (-)", "-"));
        separatorList.add(new SeparatorOption("Space ( )", " "));
        separatorList.add(new SeparatorOption("Slash (/)", "/"));
        separatorList.add(new SeparatorOption("Custom...", "__CUSTOM__"));

        separatorAdapter = new ArrayAdapter<>(getContext(), android.R.layout.simple_spinner_item, separatorList);
        separatorAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinnerSeparator.setAdapter(separatorAdapter);

        spinnerSeparator.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                SeparatorOption selected = separatorList.get(position);
                if ("__CUSTOM__".equals(selected.value)) {
                    showCustomSeparatorDialog();
                } else {
                    lastSeparatorSelection = position;
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {}
        });
    }

    private void showCustomSeparatorDialog() {
        EditText input = new EditText(getContext());
        input.setHint("Separator string");

        new AlertDialog.Builder(getContext())
            .setTitle("Custom Separator")
            .setView(input)
            .setPositiveButton("OK", (dialog, which) -> {
                String val = input.getText().toString();
                SeparatorOption opt = new SeparatorOption("Custom ('" + val + "')", val);
                separatorList.add(separatorList.size() - 1, opt);
                separatorAdapter.notifyDataSetChanged();
                spinnerSeparator.setSelection(separatorList.size() - 2);
            })
            .setNegativeButton("Cancel", (dialog, which) -> {
                spinnerSeparator.setSelection(lastSeparatorSelection);
            })
            .show();
    }

    private void setupPatternSpinner() {
        patternList.clear();
        patternList.add(new PatternOption("Random", ""));

        String[] allPatterns = activity.getAllPassphrasePatternsWithDescriptionsNative();
        for (String p : allPatterns) {
            String[] parts = p.split("\t");
            String val = parts[0];
            String label = parts.length > 1 ? parts[1] : parts[0];
            patternList.add(new PatternOption(label, val));
        }
        patternList.add(new PatternOption("Custom...", "__CUSTOM__"));

        patternAdapter = new ArrayAdapter<>(getContext(), android.R.layout.simple_spinner_item, patternList);
        patternAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinnerPassphrasePattern.setAdapter(patternAdapter);

        spinnerPassphrasePattern.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                PatternOption selected = patternList.get(position);
                if ("__CUSTOM__".equals(selected.value)) {
                    showCustomPatternDialog();
                } else {
                    lastPatternSelection = position;
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {}
        });
    }

    private void showCustomPatternDialog() {
        EditText input = new EditText(getContext());
        input.setHint("e.g. navrn");

        new AlertDialog.Builder(getContext())
            .setTitle("Custom Passphrase Pattern")
            .setView(input)
            .setPositiveButton("OK", (dialog, which) -> {
                String val = input.getText().toString().trim().toLowerCase();
                PatternOption opt = new PatternOption("Custom (" + val + ")", val);
                patternList.add(patternList.size() - 1, opt);
                patternAdapter.notifyDataSetChanged();
                spinnerPassphrasePattern.setSelection(patternList.size() - 2);
            })
            .setNegativeButton("Cancel", (dialog, which) -> {
                spinnerPassphrasePattern.setSelection(lastPatternSelection);
            })
            .show();
    }

    private void updateAlgorithmChoices() {
        boolean oldEnabled = activity.isOldAlgorithmEnabledNative();
        if ("true".equalsIgnoreCase(getBoolSpinnerValue(spinnerEnableOldAlgo))) {
            oldEnabled = true;
        }

        String currentVal = getSelectedAlgorithmCanonical();

        algorithmList.clear();
        algorithmList.add(new AlgorithmOption("Password (Argon2)", "password/argon2"));
        algorithmList.add(new AlgorithmOption("Password (SHA512 HMAC)", "password/sha512"));
        if (oldEnabled) {
            algorithmList.add(new AlgorithmOption("OldPassword", "password/old"));
        }
        algorithmList.add(new AlgorithmOption("Passphrase Diceware (Argon2)", "passphrase/diceware"));
        algorithmList.add(new AlgorithmOption("Passphrase Wordnet Pattern (Argon2)", "passphrase/wordnet"));

        algorithmAdapter.notifyDataSetChanged();
        selectAlgorithmByCanonical(currentVal != null ? currentVal : "password/argon2");
    }

    private String getSelectedAlgorithmCanonical() {
        int pos = spinnerAlgorithm.getSelectedItemPosition();
        if (pos >= 0 && pos < algorithmList.size()) {
            return algorithmList.get(pos).canonical;
        }
        return null;
    }

    private void selectAlgorithmByCanonical(String canonical) {
        if (canonical == null) return;
        for (int i = 0; i < algorithmList.size(); i++) {
            if (algorithmList.get(i).canonical.equalsIgnoreCase(canonical)) {
                spinnerAlgorithm.setSelection(i);
                return;
            }
        }
        if ("password/old".equalsIgnoreCase(canonical) || "old".equalsIgnoreCase(canonical) || "3".equals(canonical)) {
            algorithmList.add(new AlgorithmOption("OldPassword", "password/old"));
            algorithmAdapter.notifyDataSetChanged();
            spinnerAlgorithm.setSelection(algorithmList.size() - 1);
        }
    }

    private void updateRowAppearance(SettingRowDef def) {
        boolean checked = def.checkBox.isChecked();
        def.nameView.setTextColor(checked ? normalTextColor : Color.GRAY);
        def.editorView.setEnabled(checked);
        if (def.editorView instanceof TextView && !(def.editorView instanceof EditText)) {
            ((TextView) def.editorView).setTextColor(checked ? normalTextColor : Color.GRAY);
        }
        if (def.editorView instanceof ViewGroup) {
            ViewGroup vg = (ViewGroup) def.editorView;
            for (int i = 0; i < vg.getChildCount(); i++) {
                vg.getChildAt(i).setEnabled(checked);
            }
        }
    }

    private void loadFromConfig() {
        updateAlgorithmChoices();

        for (SettingRowDef def : rowDefs) {
            boolean isSet = activity.isConfigKeySet(def.key);
            String val;
            if (isSet) {
                val = activity.getConfigValue(def.key);
            } else {
                val = activity.getConfigBuiltInDefault(def.key);
            }
            setEditorValue(def.key, val);
            def.checkBox.setChecked(isSet);
            updateRowAppearance(def);
        }
    }

    private void setEditorValue(String key, String value) {
        if ("algorithm".equals(key)) {
            selectAlgorithmByCanonical(value);
        } else if ("length".equals(key)) {
            editLength.setText(value != null && !value.isEmpty() ? value : "16");
        } else if ("enable_old_algorithm".equals(key)) {
            setBoolSpinnerValue(spinnerEnableOldAlgo, value);
        } else if ("char_classes".equals(key)) {
            String effective = (value != null && !value.trim().isEmpty()) ? value : "lowercase,uppercase,digits,symbols";
            List<String> tokens = parseTokens(effective);
            checkCharLower.setChecked(tokens.contains("lowercase"));
            checkCharUpper.setChecked(tokens.contains("uppercase"));
            checkCharDigits.setChecked(tokens.contains("digits"));
            checkCharSymbols.setChecked(tokens.contains("symbols"));
            checkCharCustom.setChecked(tokens.contains("custom"));
        } else if ("custom_chars".equals(key)) {
            editCustomChars.setText(value != null ? value : "");
        } else if ("separator".equals(key)) {
            String target = value != null ? value : "";
            boolean found = false;
            for (int i = 0; i < separatorList.size() - 1; i++) {
                if (separatorList.get(i).value.equals(target)) {
                    spinnerSeparator.setSelection(i);
                    found = true;
                    break;
                }
            }
            if (!found) {
                SeparatorOption opt = new SeparatorOption("Custom ('" + target + "')", target);
                separatorList.add(separatorList.size() - 1, opt);
                separatorAdapter.notifyDataSetChanged();
                spinnerSeparator.setSelection(separatorList.size() - 2);
            }
        } else if ("passphrase_pattern".equals(key)) {
            String target = (value != null) ? value.trim().toLowerCase() : "";
            if (target.isEmpty() || "random".equals(target) || "1".equals(target)) {
                spinnerPassphrasePattern.setSelection(0);
            } else {
                boolean found = false;
                for (int i = 0; i < patternList.size() - 1; i++) {
                    if (patternList.get(i).value.equalsIgnoreCase(target)) {
                        spinnerPassphrasePattern.setSelection(i);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    PatternOption opt = new PatternOption("Custom (" + target + ")", target);
                    patternList.add(patternList.size() - 1, opt);
                    patternAdapter.notifyDataSetChanged();
                    spinnerPassphrasePattern.setSelection(patternList.size() - 2);
                }
            }
        } else if ("digits".equals(key)) {
            setBoolSpinnerValue(spinnerDigits, value);
        } else if ("symbols".equals(key)) {
            setBoolSpinnerValue(spinnerSymbols, value);
        } else if ("substitutions".equals(key)) {
            setBoolSpinnerValue(spinnerSubstitutions, value);
        } else if ("capitalize".equals(key)) {
            setBoolSpinnerValue(spinnerCapitalize, value);
        }
    }

    private String getEditorValue(String key) {
        if ("algorithm".equals(key)) {
            String canonical = getSelectedAlgorithmCanonical();
            return canonical != null ? canonical : "password/argon2";
        } else if ("length".equals(key)) {
            return editLength.getText().toString().trim();
        } else if ("enable_old_algorithm".equals(key)) {
            return getBoolSpinnerValue(spinnerEnableOldAlgo);
        } else if ("char_classes".equals(key)) {
            List<String> selected = new ArrayList<>();
            if (checkCharLower.isChecked()) selected.add("lowercase");
            if (checkCharUpper.isChecked()) selected.add("uppercase");
            if (checkCharDigits.isChecked()) selected.add("digits");
            if (checkCharSymbols.isChecked()) selected.add("symbols");
            if (checkCharCustom.isChecked()) selected.add("custom");
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < selected.size(); i++) {
                if (i > 0) sb.append(",");
                sb.append(selected.get(i));
            }
            return sb.toString();
        } else if ("custom_chars".equals(key)) {
            return editCustomChars.getText().toString();
        } else if ("separator".equals(key)) {
            int pos = spinnerSeparator.getSelectedItemPosition();
            if (pos >= 0 && pos < separatorList.size()) {
                return separatorList.get(pos).value;
            }
            return "";
        } else if ("passphrase_pattern".equals(key)) {
            int pos = spinnerPassphrasePattern.getSelectedItemPosition();
            if (pos >= 0 && pos < patternList.size()) {
                return patternList.get(pos).value;
            }
            return "";
        } else if ("digits".equals(key)) {
            return getBoolSpinnerValue(spinnerDigits);
        } else if ("symbols".equals(key)) {
            return getBoolSpinnerValue(spinnerSymbols);
        } else if ("substitutions".equals(key)) {
            return getBoolSpinnerValue(spinnerSubstitutions);
        } else if ("capitalize".equals(key)) {
            return getBoolSpinnerValue(spinnerCapitalize);
        }
        return "";
    }

    private void onRestoreDefaults() {
        for (SettingRowDef def : rowDefs) {
            String dflt = activity.getConfigBuiltInDefault(def.key);
            setEditorValue(def.key, dflt);
            def.checkBox.setChecked(false);
            updateRowAppearance(def);
        }
        updateAlgorithmChoices();
    }

    private void onSave() {
        for (SettingRowDef def : rowDefs) {
            boolean enabled = def.checkBox.isChecked();
            if (enabled) {
                String val = getEditorValue(def.key);
                String err = activity.setConfigValue(def.key, val);
                if (err != null) {
                    activity.reloadConfig();
                    new AlertDialog.Builder(getContext())
                        .setTitle("Validation Error")
                        .setMessage("Invalid value for '" + def.key + "': " + err)
                        .setPositiveButton("OK", null)
                        .show();
                    return;
                }
            } else {
                activity.unsetConfigValue(def.key);
            }
        }

        if (!activity.saveConfig()) {
            activity.reloadConfig();
            new AlertDialog.Builder(getContext())
                .setTitle("Save Error")
                .setMessage("Failed to save configuration file.")
                .setPositiveButton("OK", null)
                .show();
            return;
        }

        saved = true;
        dismiss();
    }

    private List<String> parseTokens(String str) {
        List<String> list = new ArrayList<>();
        if (str == null || str.trim().isEmpty()) return list;
        for (String s : str.split(",")) {
            String token = s.trim().toLowerCase();
            if (token.equals("lower") || token.equals("1")) token = "lowercase";
            else if (token.equals("upper") || token.equals("2")) token = "uppercase";
            else if (token.equals("digit") || token.equals("3")) token = "digits";
            else if (token.equals("symbol") || token.equals("4")) token = "symbols";
            else if (token.equals("5")) token = "custom";
            if (!token.isEmpty() && !list.contains(token)) {
                list.add(token);
            }
        }
        return list;
    }
}
