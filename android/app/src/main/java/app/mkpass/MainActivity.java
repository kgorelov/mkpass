package app.mkpass;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.app.AlertDialog;

import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
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

import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;

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
    private CheckBox lowerCaseCheckBox;
    private CheckBox upperCaseCheckBox;
    private CheckBox digitsCheckBox;
    private CheckBox symbolsCheckBox;
    private CheckBox customCheckBox;
    private TextInputLayout customCharsLayout;
    private TextInputEditText customChars;
    private SeekBar lengthSeekBar;
    private TextView lengthValue;
    private Button generateButton;
    private AlertDialog progressDialog;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        init(getDatabasePath("mkpass.db").getAbsolutePath());


        masterPassword = findViewById(R.id.masterPassword);
        repeatPassword = findViewById(R.id.repeatPassword);
        service = findViewById(R.id.service);
        algorithmSpinner = findViewById(R.id.algorithmSpinner);
        lowerCaseCheckBox = findViewById(R.id.lowerCaseCheckBox);
        upperCaseCheckBox = findViewById(R.id.upperCaseCheckBox);
        digitsCheckBox = findViewById(R.id.digitsCheckBox);
        symbolsCheckBox = findViewById(R.id.symbolsCheckBox);
        customCheckBox = findViewById(R.id.customCheckBox);
        customCharsLayout = findViewById(R.id.customCharsLayout);
        customChars = findViewById(R.id.customChars);
        lengthSeekBar = findViewById(R.id.lengthSeekBar);
        lengthValue = findViewById(R.id.lengthValue);
        generateButton = findViewById(R.id.generateButton);

        // Setup Algorithm Spinner
        ArrayAdapter<CharSequence> adapter = new ArrayAdapter<>(this, android.R.layout.simple_spinner_item, new String[]{"Argon2", "SlowSha512", "Old"});
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        algorithmSpinner.setAdapter(adapter);
        // Setup Custom Chars visibility
        customCheckBox.setOnCheckedChangeListener((buttonView, isChecked) -> {
            customCharsLayout.setVisibility(isChecked ? View.VISIBLE : View.GONE);
        });
        // Setup Length SeekBar
        lengthSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                lengthValue.setText(String.valueOf(progress));
            }
            @Override
            public void onStartTrackingTouch(SeekBar seekBar) { }
            @Override
            public void onStopTrackingTouch(SeekBar seekBar) { }
        });
        lengthValue.setText(String.valueOf(lengthSeekBar.getProgress()));
        // Set default values
        lengthSeekBar.setProgress(16);
        lowerCaseCheckBox.setChecked(true);
        upperCaseCheckBox.setChecked(true);
        digitsCheckBox.setChecked(true);
        symbolsCheckBox.setChecked(true);
        customCheckBox.setChecked(false);
        customChars.setText("");



        // Setup Service AutoComplete
        String[] all_services = getAllServiceNames();
        ArrayAdapter<String> serviceAdapter = new ArrayAdapter<>(this, android.R.layout.simple_dropdown_item_1line, all_services);
        service.setAdapter(serviceAdapter);

        service.setOnItemClickListener((parent, view, position, id) -> {
            String selectedService = (String) parent.getItemAtPosition(position);
            loadServiceEntry(selectedService);
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

    private void loadServiceEntry(String serviceName) {
        ServiceEntry entry = getServiceEntry(serviceName);
        if (entry != null) {
            algorithmSpinner.setSelection(entry.algorithm - 1);
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
            }
        } else {
            // Reset to defaults
            algorithmSpinner.setSelection(0);
            lengthSeekBar.setProgress(16);
            lowerCaseCheckBox.setChecked(true);
            upperCaseCheckBox.setChecked(true);
            digitsCheckBox.setChecked(true);
            symbolsCheckBox.setChecked(true);
            customCheckBox.setChecked(false);
            customChars.setText("");
        }
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

        String serviceName = service.getText().toString();
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
            int algorithm = algorithmSpinner.getSelectedItemPosition() + 1;
            int length = lengthSeekBar.getProgress();

            List<Integer> charClasses = new ArrayList<>();
            if (lowerCaseCheckBox.isChecked()) charClasses.add(0);
            if (upperCaseCheckBox.isChecked()) charClasses.add(1);
            if (digitsCheckBox.isChecked()) charClasses.add(2);
            if (symbolsCheckBox.isChecked()) charClasses.add(3);

            String customCharsStr = null;
            if (customCheckBox.isChecked()) {
                charClasses.add(4);
                customCharsStr = customChars.getText().toString();
            }

            int[] charClassesArray = new int[charClasses.size()];
            for (int i = 0; i < charClasses.size(); i++) {
                charClassesArray[i] = charClasses.get(i);
            }

            String generatedPassword = generatePasswordNative(masterPwd, serviceName, algorithm, length, charClassesArray, customCharsStr);

            // Save entry in background
            saveServiceEntry(serviceName, algorithm, length, charClassesArray, customCharsStr);

            // Post result to UI thread
            handler.post(() -> {
                // UI work
                progressDialog.dismiss();

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
    public native String[] getAllServiceNames();
    public native ServiceEntry getServiceEntry(String serviceName);
    public native void saveServiceEntry(String serviceName, int algorithm, int length, int[] charClasses, String customChars);
    public native String generatePasswordNative(String password, String service, int algorithm, int length, int[] charClasses, String customChars);
    public native android.graphics.Bitmap generateQrCode(String text);
}

// Helper class for passing data from C++ to Java
class ServiceEntry {
    public int algorithm;
    public int length;
    public int[] charClasses;
    public String customChars;
}