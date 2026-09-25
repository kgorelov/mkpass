package app.mkpass;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.app.AlertDialog;

import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.AutoCompleteTextView;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.LinearLayout;
import android.widget.ListView;

import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import android.os.Handler;
import android.os.Looper;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'mkpass' library on application startup.
    static {
        System.loadLibrary("mkpass");
    }

    private final ExecutorService executor = Executors.newSingleThreadExecutor();
    private final Handler handler = new Handler(Looper.getMainLooper());

    private TextInputEditText masterPassword;
    private TextInputEditText repeatPassword;
    private AutoCompleteTextView service;
    private Spinner algorithmSpinner;
    private LinearLayout characterClassesLayout;
    private CheckBox lowerCaseCheckBox;
    private CheckBox upperCaseCheckBox;
    private CheckBox digitsCheckBox;
    private CheckBox symbolsCheckBox;
    private CheckBox customCheckBox;
    private CheckBox allowSubstitutionsCheckBox;
    private TextInputLayout customCharsLayout;
    private TextInputEditText customChars;
    private LinearLayout separatorContainer;
    private Spinner separatorSpinner;
    private CheckBox capitalizeWordsCheckBox;
    private LinearLayout passphrasePatternContainer;
    private Spinner patternSpinner;
    private LinearLayout lengthContainer;
    private TextView lengthTitle;
    private SeekBar lengthSeekBar;
    private TextView lengthValue;
    private Button generateButton;
    private AlertDialog progressDialog;

    public static class AlgorithmItem {
        public final int id;
        public final String canonicalName;
        public final String displayName;

        public AlgorithmItem(int id, String canonicalName, String displayName) {
            this.id = id;
            this.canonicalName = canonicalName;
            this.displayName = displayName;
        }

        @Override
        public String toString() {
            return displayName;
        }
    }

    private final List<AlgorithmItem> algorithmList = new ArrayList<>();
    private ArrayAdapter<AlgorithmItem> algorithmAdapter;

    private static final String[] SEPARATORS = {
        "None",
        "Hyphen (-)",
        "Space ( )",
        "Slash (/)"
    };

    private static final String[] SEPARATOR_VALUES = {
        "",
        "-",
        " ",
        "/"
    };

    private List<String> patternsList = new ArrayList<>();
    private List<String> patternValuesList = new ArrayList<>();
    private ArrayAdapter<String> patternAdapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        init(getDatabasePath("mkpass.db").getAbsolutePath());
        File configFile = new File(getFilesDir(), "mkpass.conf");
        initConfig(configFile.getAbsolutePath());

        androidx.appcompat.widget.Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        masterPassword = findViewById(R.id.masterPassword);
        repeatPassword = findViewById(R.id.repeatPassword);
        service = findViewById(R.id.service);
        algorithmSpinner = findViewById(R.id.algorithmSpinner);
        characterClassesLayout = findViewById(R.id.characterClassesLayout);
        lowerCaseCheckBox = findViewById(R.id.lowerCaseCheckBox);
        upperCaseCheckBox = findViewById(R.id.upperCaseCheckBox);
        digitsCheckBox = findViewById(R.id.digitsCheckBox);
        symbolsCheckBox = findViewById(R.id.symbolsCheckBox);
        customCheckBox = findViewById(R.id.customCheckBox);
        allowSubstitutionsCheckBox = findViewById(R.id.allowSubstitutionsCheckBox);
        customCharsLayout = findViewById(R.id.customCharsLayout);
        customChars = findViewById(R.id.customChars);
        separatorContainer = findViewById(R.id.separatorContainer);
        separatorSpinner = findViewById(R.id.separatorSpinner);
        capitalizeWordsCheckBox = findViewById(R.id.capitalizeWordsCheckBox);
        passphrasePatternContainer = findViewById(R.id.passphrasePatternContainer);
        patternSpinner = findViewById(R.id.patternSpinner);
        lengthContainer = findViewById(R.id.lengthContainer);
        lengthTitle = findViewById(R.id.lengthTitle);
        lengthSeekBar = findViewById(R.id.lengthSeekBar);
        lengthValue = findViewById(R.id.lengthValue);
        generateButton = findViewById(R.id.generateButton);

        // Setup Algorithm Spinner
        algorithmAdapter = new ArrayAdapter<>(this, android.R.layout.simple_spinner_item, algorithmList);
        algorithmAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        algorithmSpinner.setAdapter(algorithmAdapter);
        updateAlgorithmChoices(false);

        ArrayAdapter<String> sepAdapter = new ArrayAdapter<>(this, android.R.layout.simple_spinner_item, SEPARATORS);
        sepAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        separatorSpinner.setAdapter(sepAdapter);
        separatorSpinner.setSelection(0); // Default to None

        patternAdapter = new ArrayAdapter<>(this, android.R.layout.simple_spinner_item, patternsList);
        patternAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        patternSpinner.setAdapter(patternAdapter);

        algorithmSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                updateAlgorithmSpecificUI();
            }
            @Override
            public void onNothingSelected(AdapterView<?> parent) {}
        });

        // Setup Custom Chars visibility
        customCheckBox.setOnCheckedChangeListener((buttonView, isChecked) -> {
            customCharsLayout.setVisibility(isChecked ? View.VISIBLE : View.GONE);
        });

        digitsCheckBox.setOnCheckedChangeListener((buttonView, isChecked) -> updateSubstitutionsState());
        symbolsCheckBox.setOnCheckedChangeListener((buttonView, isChecked) -> updateSubstitutionsState());
        capitalizeWordsCheckBox.setOnCheckedChangeListener((buttonView, isChecked) -> updateSubstitutionsState());
        allowSubstitutionsCheckBox.setOnCheckedChangeListener((buttonView, isChecked) -> updateSubstitutionsState());
        // Setup Length SeekBar
        lengthSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                lengthValue.setText(String.valueOf(progress));
                updatePatternsList();
            }
            @Override
            public void onStartTrackingTouch(SeekBar seekBar) { }
            @Override
            public void onStopTrackingTouch(SeekBar seekBar) { }
        });

        // Set default values from config
        applyDefaultsFromConfig();
        updateAlgorithmSpecificUI();

        // Setup Service AutoComplete
        updateServiceSuggestions();

        service.setOnItemClickListener((parent, view, position, id) -> {
            String selectedService = (String) parent.getItemAtPosition(position);
            loadServiceEntry(selectedService);
        });

        service.setOnFocusChangeListener((v, hasFocus) -> {
            if (!hasFocus) {
                String val = service.getText().toString().replaceAll("\\s+$", "");
                service.setText(val);
            }
        });

        // Generate Button
        generateButton.setOnClickListener(v -> generatePassword());

        // Password validation
        TextWatcher passwordTextWatcher = new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {}
            @Override
            public void afterTextChanged(Editable s) {
                checkPasswords();
            }
        };
        masterPassword.addTextChangedListener(passwordTextWatcher);
        repeatPassword.addTextChangedListener(passwordTextWatcher);
    }

    @Override
    public boolean onCreateOptionsMenu(android.view.Menu menu) {
        getMenuInflater().inflate(R.menu.main_menu, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(android.view.MenuItem item) {
        int id = item.getItemId();
        if (id == R.id.menu_db_management) {
            showDbManagementDialog();
            return true;
        } else if (id == R.id.menu_settings) {
            showSettingsDialog();
            return true;
        } else if (id == R.id.menu_manual) {
            showManualDialog();
            return true;
        } else if (id == R.id.menu_about) {
            showAboutDialog();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private void showDbManagementDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Database Management");

        View view = getLayoutInflater().inflate(R.layout.dialog_db_management, null);
        android.widget.ListView listView = view.findViewById(R.id.dbListView);
        android.widget.EditText searchEditText = view.findViewById(R.id.searchEditText);

        updateDbManagementList(listView, "");

        searchEditText.addTextChangedListener(new android.text.TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {}
            @Override
            public void afterTextChanged(android.text.Editable s) {
                updateDbManagementList(listView, s.toString());
            }
        });

        builder.setView(view);
        builder.setPositiveButton("Close", null);
        builder.show();
    }

    private void updateDbManagementList(android.widget.ListView listView, String filter) {
        String[] allServices = getAllServiceNames();
        if (allServices == null || allServices.length == 0) {
            ArrayAdapter<String> emptyAdapter = new ArrayAdapter<>(this, android.R.layout.simple_list_item_1, new String[]{"No records found"});
            listView.setAdapter(emptyAdapter);
            return;
        }

        List<String> filteredList = new ArrayList<>();
        for (String s : allServices) {
            if (filter == null || filter.isEmpty() || s.toLowerCase().contains(filter.toLowerCase())) {
                filteredList.add(s);
            }
        }

        if (filteredList.isEmpty()) {
            ArrayAdapter<String> emptyAdapter = new ArrayAdapter<>(this, android.R.layout.simple_list_item_1, new String[]{"No matches found"});
            listView.setAdapter(emptyAdapter);
            return;
        }

        // Custom adapter to show delete button and service details
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(this, R.layout.item_db_record, R.id.serviceName, filteredList) {
            @Override
            public View getView(int position, View convertView, android.view.ViewGroup parent) {
                View view = super.getView(position, convertView, parent);
                String serviceName = getItem(position);

                TextView nameView = view.findViewById(R.id.serviceName);
                if (nameView != null) {
                    nameView.setText(highlightServiceName(serviceName));
                }

                TextView paramsView = view.findViewById(R.id.serviceParams);
                if (paramsView != null) {
                    ServiceEntry entry = getServiceEntry(serviceName);
                    if (entry != null) {
                        String algoName = getAlgorithmName(entry.algorithm);
                        paramsView.setText(algoName + " • Length: " + entry.length);
                    } else {
                        paramsView.setText("");
                    }
                }

                view.findViewById(R.id.deleteButton).setOnClickListener(v -> {
                    new AlertDialog.Builder(MainActivity.this)
                            .setTitle("Delete")
                            .setMessage("Delete service '" + serviceName + "'?")
                            .setPositiveButton("Yes", (dialog, which) -> {
                                deleteServiceEntry(serviceName);
                                android.widget.EditText searchEditText = ((View)listView.getParent()).findViewById(R.id.searchEditText);
                                updateDbManagementList(listView, searchEditText.getText().toString());
                                updateServiceSuggestions();
                                String currentService = service.getText().toString().replaceAll("\\s+$", "");
                                if (currentService.equals(serviceName)) {
                                    updateAlgorithmChoices(false);
                                    loadServiceEntry(currentService);
                                }
                            })
                            .setNegativeButton("No", null)
                            .show();
                });
                return view;
            }
        };
        listView.setAdapter(adapter);
    }

    private String getAlgorithmName(int algorithm) {
        for (AlgorithmItem item : algorithmList) {
            if (item.id == algorithm) {
                return item.displayName;
            }
        }
        if (algorithm == 1) return "Password (Argon2)";
        if (algorithm == 2) return "Password (SHA512 HMAC)";
        if (algorithm == 3) return "OldPassword";
        if (algorithm == 4) return "Passphrase Diceware (Argon2)";
        if (algorithm == 5) return "Passphrase Wordnet Pattern (Argon2)";
        return "Unknown";
    }

    private CharSequence highlightServiceName(String name) {
        int len = name.length();
        int trailingStart = len;
        while (trailingStart > 0 && Character.isWhitespace(name.charAt(trailingStart - 1))) {
            trailingStart--;
        }

        android.text.SpannableStringBuilder builder = new android.text.SpannableStringBuilder();
        for (int i = 0; i < len; i++) {
            char c = name.charAt(i);
            int start = builder.length();
            if (i >= trailingStart) {
                if (c == ' ') {
                    builder.append("\u00a0");
                    builder.setSpan(new android.text.style.BackgroundColorSpan(android.graphics.Color.parseColor("#FFB3B3")), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    builder.setSpan(new android.text.style.ForegroundColorSpan(android.graphics.Color.parseColor("#B30000")), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    builder.setSpan(new android.text.style.TypefaceSpan("monospace"), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    builder.setSpan(new android.text.style.StyleSpan(android.graphics.Typeface.BOLD), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                } else if (c == '\t') {
                    builder.append("[TAB]");
                    builder.setSpan(new android.text.style.BackgroundColorSpan(android.graphics.Color.parseColor("#FFB3B3")), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    builder.setSpan(new android.text.style.ForegroundColorSpan(android.graphics.Color.parseColor("#B30000")), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    builder.setSpan(new android.text.style.TypefaceSpan("monospace"), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    builder.setSpan(new android.text.style.StyleSpan(android.graphics.Typeface.BOLD), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                } else if (c == '\r') {
                    builder.append("[CR]");
                    builder.setSpan(new android.text.style.BackgroundColorSpan(android.graphics.Color.parseColor("#FFB3B3")), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    builder.setSpan(new android.text.style.ForegroundColorSpan(android.graphics.Color.parseColor("#B30000")), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    builder.setSpan(new android.text.style.TypefaceSpan("monospace"), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    builder.setSpan(new android.text.style.StyleSpan(android.graphics.Typeface.BOLD), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                } else if (c == '\n') {
                    builder.append("[LF]");
                    builder.setSpan(new android.text.style.BackgroundColorSpan(android.graphics.Color.parseColor("#FFB3B3")), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    builder.setSpan(new android.text.style.ForegroundColorSpan(android.graphics.Color.parseColor("#B30000")), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    builder.setSpan(new android.text.style.TypefaceSpan("monospace"), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    builder.setSpan(new android.text.style.StyleSpan(android.graphics.Typeface.BOLD), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                } else {
                    builder.append(String.format("\\x%02X", (int) c));
                    builder.setSpan(new android.text.style.BackgroundColorSpan(android.graphics.Color.parseColor("#FFB3B3")), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    builder.setSpan(new android.text.style.ForegroundColorSpan(android.graphics.Color.parseColor("#B30000")), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    builder.setSpan(new android.text.style.TypefaceSpan("monospace"), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    builder.setSpan(new android.text.style.StyleSpan(android.graphics.Typeface.BOLD), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                }
            } else if (c < 32 || c >= 127) {
                if (c == '\t') {
                    builder.append("[TAB]");
                    builder.setSpan(new android.text.style.BackgroundColorSpan(android.graphics.Color.parseColor("#FFE0B2")), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    builder.setSpan(new android.text.style.ForegroundColorSpan(android.graphics.Color.parseColor("#E65100")), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    builder.setSpan(new android.text.style.TypefaceSpan("monospace"), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    builder.setSpan(new android.text.style.StyleSpan(android.graphics.Typeface.BOLD), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                } else if (c == '\r') {
                    builder.append("[CR]");
                    builder.setSpan(new android.text.style.BackgroundColorSpan(android.graphics.Color.parseColor("#FFE0B2")), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    builder.setSpan(new android.text.style.ForegroundColorSpan(android.graphics.Color.parseColor("#E65100")), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    builder.setSpan(new android.text.style.TypefaceSpan("monospace"), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    builder.setSpan(new android.text.style.StyleSpan(android.graphics.Typeface.BOLD), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                } else if (c == '\n') {
                    builder.append("[LF]");
                    builder.setSpan(new android.text.style.BackgroundColorSpan(android.graphics.Color.parseColor("#FFE0B2")), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    builder.setSpan(new android.text.style.ForegroundColorSpan(android.graphics.Color.parseColor("#E65100")), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    builder.setSpan(new android.text.style.TypefaceSpan("monospace"), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    builder.setSpan(new android.text.style.StyleSpan(android.graphics.Typeface.BOLD), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                } else {
                    builder.append(String.format("\\x%02X", (int) c));
                    builder.setSpan(new android.text.style.BackgroundColorSpan(android.graphics.Color.parseColor("#FFCDD2")), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    builder.setSpan(new android.text.style.ForegroundColorSpan(android.graphics.Color.parseColor("#C62828")), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    builder.setSpan(new android.text.style.TypefaceSpan("monospace"), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    builder.setSpan(new android.text.style.StyleSpan(android.graphics.Typeface.BOLD), start, builder.length(), android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                }
            } else {
                builder.append(c);
            }
        }
        return builder;
    }

    private void showManualDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("mkpass User Manual");

        android.webkit.WebView webView = new android.webkit.WebView(this);
        webView.getSettings().setBuiltInZoomControls(true);
        webView.getSettings().setDisplayZoomControls(false);
        webView.loadUrl("file:///android_asset/help.html");

        builder.setView(webView);
        builder.setPositiveButton("Close", (dialog, which) -> dialog.dismiss());
        AlertDialog dialog = builder.create();
        dialog.show();
    }

    private void showAboutDialog() {
        new AlertDialog.Builder(this)
                .setTitle("About mkpass")
                .setIcon(R.mipmap.ic_launcher)
                .setMessage("mkpass - Password generator\n\nWritten in C++ with Android frontend.")
                .setPositiveButton("OK", null)
                .show();
    }

    private void showSettingsDialog() {
        SettingsDialog dialog = new SettingsDialog(this);
        dialog.setOnDismissListener(d -> {
            if (dialog.isSaved()) {
                String currentService = service.getText().toString().replaceAll("\\s+$", "");
                ServiceEntry entry = getServiceEntry(currentService);
                boolean isOldRecord = (entry != null && entry.algorithm == 3);
                updateAlgorithmChoices(isOldRecord);
                loadServiceEntry(currentService);
            }
        });
        dialog.show();
    }

    public int getSelectedAlgorithmId() {
        AlgorithmItem item = (AlgorithmItem) algorithmSpinner.getSelectedItem();
        return item != null ? item.id : 1;
    }

    public void selectAlgorithmById(int id) {
        for (int i = 0; i < algorithmList.size(); i++) {
            if (algorithmList.get(i).id == id) {
                algorithmSpinner.setSelection(i);
                return;
            }
        }
        if (id == 3) {
            updateAlgorithmChoices(true);
            for (int i = 0; i < algorithmList.size(); i++) {
                if (algorithmList.get(i).id == 3) {
                    algorithmSpinner.setSelection(i);
                    return;
                }
            }
        }
    }

    public void updateAlgorithmChoices(boolean forceIncludeOld) {
        boolean oldEnabled = isOldAlgorithmEnabledNative() || forceIncludeOld;
        int currentId = getSelectedAlgorithmId();

        algorithmList.clear();
        algorithmList.add(new AlgorithmItem(1, "password/argon2", "Password (Argon2)"));
        algorithmList.add(new AlgorithmItem(2, "password/sha512", "Password (SHA512 HMAC)"));
        if (oldEnabled) {
            algorithmList.add(new AlgorithmItem(3, "password/old", "OldPassword"));
        }
        algorithmList.add(new AlgorithmItem(4, "passphrase/diceware", "Passphrase Diceware (Argon2)"));
        algorithmList.add(new AlgorithmItem(5, "passphrase/wordnet", "Passphrase Wordnet Pattern (Argon2)"));

        if (algorithmAdapter != null) {
            algorithmAdapter.notifyDataSetChanged();
            boolean found = false;
            for (int i = 0; i < algorithmList.size(); i++) {
                if (algorithmList.get(i).id == currentId) {
                    algorithmSpinner.setSelection(i);
                    found = true;
                    break;
                }
            }
            if (!found) {
                algorithmSpinner.setSelection(0);
            }
        }
    }

    private void setSeparatorSelection(String sep) {
        for (int i = 0; i < SEPARATOR_VALUES.length; i++) {
            if (SEPARATOR_VALUES[i].equals(sep)) {
                separatorSpinner.setSelection(i);
                return;
            }
        }
        separatorSpinner.setSelection(0);
    }

    private void setPatternSelection(String pat) {
        for (int i = 0; i < patternValuesList.size(); i++) {
            if (patternValuesList.get(i).equals(pat)) {
                patternSpinner.setSelection(i);
                return;
            }
        }
        patternSpinner.setSelection(0);
    }

    private void applyDefaultsFromConfig() {
        String algoVal = getConfigValue("algorithm");
        int defaultAlgo = 1;
        if (algoVal != null) {
            if ("password/argon2".equalsIgnoreCase(algoVal) || "argon2".equalsIgnoreCase(algoVal) || "1".equals(algoVal)) defaultAlgo = 1;
            else if ("password/sha512".equalsIgnoreCase(algoVal) || "sha512".equalsIgnoreCase(algoVal) || "2".equals(algoVal)) defaultAlgo = 2;
            else if ("password/old".equalsIgnoreCase(algoVal) || "old".equalsIgnoreCase(algoVal) || "3".equals(algoVal)) defaultAlgo = isOldAlgorithmEnabledNative() ? 3 : 1;
            else if ("passphrase/diceware".equalsIgnoreCase(algoVal) || "diceware".equalsIgnoreCase(algoVal) || "4".equals(algoVal)) defaultAlgo = 4;
            else if ("passphrase/wordnet".equalsIgnoreCase(algoVal) || "wordnet".equalsIgnoreCase(algoVal) || "5".equals(algoVal)) defaultAlgo = 5;
        }
        selectAlgorithmById(defaultAlgo);

        String lenVal = getConfigValue("length");
        int dfltLen;
        if (lenVal != null) {
            try {
                dfltLen = Integer.parseInt(lenVal);
            } catch (NumberFormatException e) {
                dfltLen = (defaultAlgo == 4 || defaultAlgo == 5) ? 3 : (defaultAlgo == 3 ? 8 : 16);
            }
        } else {
            dfltLen = (defaultAlgo == 4 || defaultAlgo == 5) ? 3 : (defaultAlgo == 3 ? 8 : 16);
        }
        lengthSeekBar.setProgress(dfltLen);

        String ccVal = getConfigValue("char_classes");
        if (ccVal != null) {
            lowerCaseCheckBox.setChecked(false);
            upperCaseCheckBox.setChecked(false);
            digitsCheckBox.setChecked(false);
            symbolsCheckBox.setChecked(false);
            customCheckBox.setChecked(false);
            for (String token : ccVal.split(",")) {
                token = token.trim().toLowerCase();
                if (token.equals("lowercase") || token.equals("lower") || token.equals("1")) lowerCaseCheckBox.setChecked(true);
                else if (token.equals("uppercase") || token.equals("upper") || token.equals("2")) upperCaseCheckBox.setChecked(true);
                else if (token.equals("digits") || token.equals("digit") || token.equals("3")) digitsCheckBox.setChecked(true);
                else if (token.equals("symbols") || token.equals("symbol") || token.equals("4")) symbolsCheckBox.setChecked(true);
                else if (token.equals("custom") || token.equals("5")) customCheckBox.setChecked(true);
            }
        } else {
            lowerCaseCheckBox.setChecked(true);
            upperCaseCheckBox.setChecked(true);
            digitsCheckBox.setChecked(true);
            symbolsCheckBox.setChecked(true);
            customCheckBox.setChecked(false);
        }

        String customVal = getConfigValue("custom_chars");
        customChars.setText(customVal != null ? customVal : "");

        String sepVal = getConfigValue("separator");
        setSeparatorSelection(sepVal != null ? sepVal : "");

        String patVal = getConfigValue("passphrase_pattern");
        updatePatternsList();
        if (patVal != null && !patVal.isEmpty()) {
            setPatternSelection(patVal);
        } else {
            patternSpinner.setSelection(0);
        }

        String digVal = getConfigValue("digits");
        if (defaultAlgo == 4 || defaultAlgo == 5) {
            digitsCheckBox.setChecked("true".equalsIgnoreCase(digVal));
        }
        String symVal = getConfigValue("symbols");
        if (defaultAlgo == 4 || defaultAlgo == 5) {
            symbolsCheckBox.setChecked("true".equalsIgnoreCase(symVal));
        }
        String subVal = getConfigValue("substitutions");
        allowSubstitutionsCheckBox.setChecked("true".equalsIgnoreCase(subVal));

        String capVal = getConfigValue("capitalize");
        capitalizeWordsCheckBox.setChecked(capVal == null || "true".equalsIgnoreCase(capVal));
    }

    private int lastAlgorithm = -1;
    private int lastLength = -1;

    private void updateAlgorithmSpecificUI() {
        int algorithm = getSelectedAlgorithmId();

        boolean showCharClasses = (algorithm == 1 || algorithm == 2 || algorithm == 4 || algorithm == 5);
        boolean showLength = (algorithm != 3); // Diceware, Password or Pattern
        if (algorithm == 3) showLength = true; // Old algo has length
        boolean showSeparator = (algorithm == 4 || algorithm == 5);
        boolean showPattern = (algorithm == 5);

        characterClassesLayout.setVisibility(showCharClasses ? View.VISIBLE : View.GONE);
        lowerCaseCheckBox.setVisibility((algorithm == 1 || algorithm == 2) ? View.VISIBLE : View.GONE);
        upperCaseCheckBox.setVisibility((algorithm == 1 || algorithm == 2) ? View.VISIBLE : View.GONE);
        customCheckBox.setVisibility((algorithm == 1 || algorithm == 2) ? View.VISIBLE : View.GONE);
        allowSubstitutionsCheckBox.setVisibility((algorithm == 4 || algorithm == 5) ? View.VISIBLE : View.GONE);
        capitalizeWordsCheckBox.setVisibility((algorithm == 4 || algorithm == 5) ? View.VISIBLE : View.GONE);

        lengthContainer.setVisibility(showLength ? View.VISIBLE : View.GONE);
        separatorContainer.setVisibility(showSeparator ? View.VISIBLE : View.GONE);
        passphrasePatternContainer.setVisibility(showPattern ? View.VISIBLE : View.GONE);

        // Defaults from config when switching algorithm
        if (lastAlgorithm != algorithm) {
            if (algorithm == 4 || algorithm == 5) {
                String digVal = getConfigValue("digits");
                digitsCheckBox.setChecked("true".equalsIgnoreCase(digVal));
                String symVal = getConfigValue("symbols");
                symbolsCheckBox.setChecked("true".equalsIgnoreCase(symVal));
                String capVal = getConfigValue("capitalize");
                capitalizeWordsCheckBox.setChecked(capVal == null || "true".equalsIgnoreCase(capVal));
                String subVal = getConfigValue("substitutions");
                allowSubstitutionsCheckBox.setChecked("true".equalsIgnoreCase(subVal));

                String sepVal = getConfigValue("separator");
                setSeparatorSelection(sepVal != null ? sepVal : "");

                String lenVal = getConfigValue("length");
                int len = 3;
                if (lenVal != null) {
                    try {
                        int parsed = Integer.parseInt(lenVal);
                        if (parsed >= 1 && parsed <= 20) len = parsed;
                    } catch (Exception ignored) {}
                }
                lengthSeekBar.setProgress(len);

                updatePatternsList();
                String patVal = getConfigValue("passphrase_pattern");
                if (patVal != null && !patVal.isEmpty()) {
                    setPatternSelection(patVal);
                } else {
                    patternSpinner.setSelection(0);
                }
            } else if (algorithm == 1 || algorithm == 2) {
                String ccVal = getConfigValue("char_classes");
                if (ccVal != null) {
                    lowerCaseCheckBox.setChecked(false);
                    upperCaseCheckBox.setChecked(false);
                    digitsCheckBox.setChecked(false);
                    symbolsCheckBox.setChecked(false);
                    customCheckBox.setChecked(false);
                    for (String token : ccVal.split(",")) {
                        token = token.trim().toLowerCase();
                        if (token.equals("lowercase") || token.equals("lower") || token.equals("1")) lowerCaseCheckBox.setChecked(true);
                        else if (token.equals("uppercase") || token.equals("upper") || token.equals("2")) upperCaseCheckBox.setChecked(true);
                        else if (token.equals("digits") || token.equals("digit") || token.equals("3")) digitsCheckBox.setChecked(true);
                        else if (token.equals("symbols") || token.equals("symbol") || token.equals("4")) symbolsCheckBox.setChecked(true);
                        else if (token.equals("custom") || token.equals("5")) customCheckBox.setChecked(true);
                    }
                } else {
                    digitsCheckBox.setChecked(true);
                    symbolsCheckBox.setChecked(true);
                    lowerCaseCheckBox.setChecked(true);
                    upperCaseCheckBox.setChecked(true);
                }
                String customVal = getConfigValue("custom_chars");
                customChars.setText(customVal != null ? customVal : "");

                String lenVal = getConfigValue("length");
                int len = 16;
                if (lenVal != null) {
                    try {
                        int parsed = Integer.parseInt(lenVal);
                        if (parsed >= 1 && parsed <= 128) len = parsed;
                    } catch (Exception ignored) {}
                }
                lengthSeekBar.setProgress(len);
            } else if (algorithm == 3) {
                String lenVal = getConfigValue("length");
                int len = 8;
                if (lenVal != null) {
                    try {
                        int parsed = Integer.parseInt(lenVal);
                        if (parsed >= 1 && parsed <= 128) len = parsed;
                    } catch (Exception ignored) {}
                }
                lengthSeekBar.setProgress(len);
            }
        }
        lastAlgorithm = algorithm;

        if (showLength) {
            if (algorithm == 4) { // Diceware
                lengthTitle.setText("Passphrase words count");
                lengthSeekBar.setMax(20);
                if (lengthSeekBar.getProgress() < 3) lengthSeekBar.setProgress(3);
            } else if (algorithm == 5) { // Pattern
                lengthTitle.setText("Passphrase words count");
                lengthSeekBar.setMax(getMaxPassphrasePatternLengthNative());
                if (lengthSeekBar.getProgress() < 1) lengthSeekBar.setProgress(1);
                updatePatternsList();
            } else {
                lengthTitle.setText("Password Length");
                lengthSeekBar.setMax(128);
                if (lengthSeekBar.getProgress() < 1) lengthSeekBar.setProgress(1);
            }
        }
        updateSubstitutionsState();
    }

    private void updatePatternsList() {
        int algorithm = getSelectedAlgorithmId();
        if (algorithm != 5) return;

        int length = lengthSeekBar.getProgress();
        if (length == lastLength && !patternsList.isEmpty()) return;
        lastLength = length;

        String currentSelection = null;
        if (patternSpinner.getSelectedItemPosition() >= 0 && patternValuesList.size() > patternSpinner.getSelectedItemPosition()) {
            currentSelection = patternValuesList.get(patternSpinner.getSelectedItemPosition());
        }

        patternsList.clear();
        patternValuesList.clear();

        patternsList.add("Random");
        patternValuesList.add("");

        String[] patterns = getPassphrasePatternsWithDescriptionsNative(length);
        for (String p : patterns) {
            String[] parts = p.split("\t");
            patternValuesList.add(parts[0]);
            patternsList.add(parts.length > 1 ? parts[1] : parts[0]);
        }

        patternAdapter.notifyDataSetChanged();

        if (currentSelection != null) {
            for (int i = 0; i < patternValuesList.size(); i++) {
                if (patternValuesList.get(i).equals(currentSelection)) {
                    patternSpinner.setSelection(i);
                    return;
                }
            }
        }
        patternSpinner.setSelection(0); // Default to Random
    }

    private void updateSubstitutionsState() {
        int algorithm = getSelectedAlgorithmId();
        boolean isPassphrase = (algorithm == 4 || algorithm == 5);
        if (isPassphrase) {
            boolean enabled = digitsCheckBox.isChecked() || symbolsCheckBox.isChecked();
            allowSubstitutionsCheckBox.setEnabled(enabled);
            if (!enabled) {
                allowSubstitutionsCheckBox.setChecked(false);
            }
        } else {
            allowSubstitutionsCheckBox.setEnabled(true);
        }
    }

    private void updateServiceSuggestions() {
        String[] all_services = getAllServiceNames();
        ArrayAdapter<String> serviceAdapter = new ArrayAdapter<>(this, android.R.layout.simple_dropdown_item_1line, all_services);
        service.setAdapter(serviceAdapter);
    }

    private void loadServiceEntry(String serviceName) {
        ServiceEntry entry = getServiceEntry(serviceName);
        boolean isOldRecord = (entry != null && entry.algorithm == 3);
        updateAlgorithmChoices(isOldRecord);

        if (entry != null) {
            selectAlgorithmById(entry.algorithm);
            lengthSeekBar.setProgress(entry.length);

            lowerCaseCheckBox.setChecked(false);
            upperCaseCheckBox.setChecked(false);
            digitsCheckBox.setChecked(false);
            symbolsCheckBox.setChecked(false);
            customCheckBox.setChecked(false);

            for (int cc : entry.charClasses) {
                if (cc == 0) lowerCaseCheckBox.setChecked(true);
                if (cc == 1) upperCaseCheckBox.setChecked(true);
                if (cc == 2) digitsCheckBox.setChecked(true);
                if (cc == 3) symbolsCheckBox.setChecked(true);
                if (cc == 4) customCheckBox.setChecked(true);
            }

            if (entry.customChars != null) {
                customChars.setText(entry.customChars);
            } else {
                customChars.setText("");
            }

            if (entry.separator != null) {
                setSeparatorSelection(entry.separator);
            }
            capitalizeWordsCheckBox.setChecked(entry.capitalizeWords);

            if (entry.pattern != null) {
                updatePatternsList();
                setPatternSelection(entry.pattern);
            } else {
                patternSpinner.setSelection(0); // Random
            }
            allowSubstitutionsCheckBox.setChecked(entry.allowSubstitutions);
        } else {
            // Reset to defaults based on config
            applyDefaultsFromConfig();
        }
        updateAlgorithmSpecificUI();
    }

    private void generatePassword() {
        if (!checkPasswords()) {
            Toast.makeText(this, "Passwords do not match", Toast.LENGTH_SHORT).show();
            return;
        }

        String masterPwd = masterPassword.getText().toString();
        if (masterPwd.isEmpty()) {
            Toast.makeText(this, "Master password cannot be empty", Toast.LENGTH_SHORT).show();
            return;
        }

        String serviceName = service.getText().toString().replaceAll("\\s+$", "");
        service.setText(serviceName);
        if (serviceName.isEmpty()) {
            Toast.makeText(this, "Service name cannot be empty", Toast.LENGTH_SHORT).show();
            return;
        }

        generateButton.setEnabled(false);

        AlertDialog.Builder progressBuilder = new AlertDialog.Builder(this);
        progressBuilder.setView(getLayoutInflater().inflate(R.layout.dialog_progress, null));
        progressBuilder.setCancelable(false);
        progressDialog = progressBuilder.create();
        progressDialog.show();

        executor.execute(() -> {
            // Background work
            int algorithm = getSelectedAlgorithmId();
            int length = lengthSeekBar.getProgress();
            String separator = SEPARATOR_VALUES[separatorSpinner.getSelectedItemPosition()];
            String pattern = (algorithm == 5) ? patternValuesList.get(patternSpinner.getSelectedItemPosition()) : "";
            boolean capitalizeWords = capitalizeWordsCheckBox.isChecked();
            boolean allowSubstitutions = allowSubstitutionsCheckBox.isChecked();

            List<Integer> charClasses = new ArrayList<>();
            String customCharsStr = null;

            if (algorithm == 1 || algorithm == 2) {
                if (lowerCaseCheckBox.isChecked()) charClasses.add(0);
                if (upperCaseCheckBox.isChecked()) charClasses.add(1);
                if (digitsCheckBox.isChecked()) charClasses.add(2);
                if (symbolsCheckBox.isChecked()) charClasses.add(3);

                if (customCheckBox.isChecked()) {
                    charClasses.add(4);
                    customCharsStr = customChars.getText().toString();
                }
            } else if (algorithm == 4 || algorithm == 5) {
                if (digitsCheckBox.isChecked()) charClasses.add(2);
                if (symbolsCheckBox.isChecked()) charClasses.add(3);
            }

            int[] charClassesArray = new int[charClasses.size()];
            for (int i = 0; i < charClasses.size(); i++) {
                charClassesArray[i] = charClasses.get(i);
            }

            String generatedPassword = generatePasswordNative(masterPwd, serviceName, algorithm, length, charClassesArray, customCharsStr, separator, capitalizeWords, pattern, allowSubstitutions);

            // Save entry in background
            saveServiceEntry(serviceName, algorithm, length, charClassesArray, customCharsStr, separator, capitalizeWords, pattern, allowSubstitutions);

            // Post result to UI thread
            handler.post(() -> {
                // UI work
                progressDialog.dismiss();
                updateServiceSuggestions();

                // Show password in dialog
                AlertDialog.Builder builder = new AlertDialog.Builder(this);
                View dialogView = getLayoutInflater().inflate(R.layout.dialog_generated_password, null);
                builder.setView(dialogView);

                TextView passwordTextView = dialogView.findViewById(R.id.passwordTextView);
                passwordTextView.setText(generatedPassword);
                passwordTextView.setInputType(android.text.InputType.TYPE_CLASS_TEXT | android.text.InputType.TYPE_TEXT_VARIATION_PASSWORD);

                Button copyButton = dialogView.findViewById(R.id.copyButton);
                copyButton.setOnClickListener(v -> {
                    android.content.ClipboardManager clipboard = (android.content.ClipboardManager) getSystemService(CLIPBOARD_SERVICE);
                    android.content.ClipData clip = android.content.ClipData.newPlainText("Password", generatedPassword);
                    clipboard.setPrimaryClip(clip);
                    Toast.makeText(this, "Password copied to clipboard", Toast.LENGTH_SHORT).show();
                });

                Button revealButton = dialogView.findViewById(R.id.revealButton);
                revealButton.setOnClickListener(v -> {
                    if ((passwordTextView.getInputType() & android.text.InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD) == android.text.InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD) {
                        // Currently visible, so hide it
                        passwordTextView.setInputType(android.text.InputType.TYPE_CLASS_TEXT | android.text.InputType.TYPE_TEXT_VARIATION_PASSWORD);
                        revealButton.setText("Reveal");
                    } else {
                        // Currently hidden, so reveal it
                        passwordTextView.setInputType(android.text.InputType.TYPE_CLASS_TEXT | android.text.InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
                        revealButton.setText("Hide");
                    }
                });

                Button qrButton = dialogView.findViewById(R.id.qrButton);
                ImageView qrCodeImageView = dialogView.findViewById(R.id.qrCodeImageView);
                qrButton.setOnClickListener(v -> {
                    android.graphics.Bitmap qrCodeBitmap = generateQrCode(generatedPassword);
                    qrCodeImageView.setImageBitmap(qrCodeBitmap);
                    qrCodeImageView.setVisibility(qrCodeImageView.getVisibility() == View.GONE ? View.VISIBLE : View.GONE);
                });

                builder.setTitle("Generated Password")
                        .setPositiveButton("OK", null);

                AlertDialog dialog = builder.create();
                dialog.setOnDismissListener(dialogInterface -> {
                    generateButton.setEnabled(true);
                });
                dialog.show();
            });
        });
    }

    private boolean checkPasswords() {
        String pass1 = masterPassword.getText().toString();
        String pass2 = repeatPassword.getText().toString();
        if (!pass2.isEmpty() && !pass1.equals(pass2)) {
            repeatPassword.setError("Passwords do not match");
            return false;
        } else {
            repeatPassword.setError(null);
            return true;
        }
    }

    // Native methods
    public native void init(String dbPath);
    public native void initConfig(String configPath);
    public native void reloadConfig();
    public native String getConfigValue(String key);
    public native String setConfigValue(String key, String value);
    public native boolean unsetConfigValue(String key);
    public native boolean isConfigKeySet(String key);
    public native boolean isOldAlgorithmEnabledNative();
    public native boolean saveConfig();
    public native String getConfigBuiltInDefault(String key);
    public native String[] getAllServiceNames();
    public native ServiceEntry getServiceEntry(String serviceName);
    public native void saveServiceEntry(String serviceName, int algorithm, int length, int[] charClasses, String customChars, String separator, boolean capitalizeWords, String pattern, boolean allowSubstitutions);
    public native void deleteServiceEntry(String serviceName);
    public native String generatePasswordNative(String password, String service, int algorithm, int length, int[] charClasses, String customChars, String separator, boolean capitalizeWords, String pattern, boolean allowSubstitutions);
    public native android.graphics.Bitmap generateQrCode(String text);
    public native int getMaxPassphrasePatternLengthNative();
    public native String[] getPassphrasePatternsNative(int length);
    public native String[] getPassphrasePatternsWithDescriptionsNative(int length);
    public native String[] getAllPassphrasePatternsWithDescriptionsNative();
}

// Helper class for passing data from C++ to Java
class ServiceEntry {
    public int algorithm;
    public int length;
    public int[] charClasses;
    public String customChars;
    public String separator;
    public boolean capitalizeWords;
    public String pattern;
    public boolean allowSubstitutions;
}
