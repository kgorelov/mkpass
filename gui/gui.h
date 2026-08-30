#pragma once

#include <QMainWindow>
#include <QFutureWatcher>
#include <QCloseEvent>

class QLineEdit;
class QCheckBox;
class QSpinBox;
class QPushButton;
class QComboBox;
class QGroupBox;
class QLabel;
class ProgressDialog;
class QCompleter;
class ManualDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void generatePassword();
    void generationFinished();
    void serviceChanged(const QString &service);
    void validateInputs();
    void updateAlgorithmSpecificUI();
    void updateCustomCharsState();
    void updateSubstitutionsState();
    void updatePatternsList();
    void manageDatabase();
    void showManual();
    void showHelp();

private:
    void setupUI();
    void refreshCompleter();

    QLineEdit *masterPasswordLineEdit;
    QLineEdit *repeatPasswordLineEdit;
    QLineEdit *serviceLineEdit;
    QComboBox *algorithmComboBox;
    QGroupBox *characterClassesGroupBox;
    QCheckBox *upperCaseCheckBox;
    QCheckBox *lowerCaseCheckBox;
    QCheckBox *digitsCheckBox;
    QCheckBox *symbolsCheckBox;
    QCheckBox *customCheckBox;
    QCheckBox *allowSubstitutionsCheckBox;
    QCheckBox *capitalizeCheckBox;
    QLineEdit *customCharsLineEdit;
    QWidget *lengthWidget;
    QLabel *lengthLabel;
    QSpinBox *lengthSpinBox;
    QWidget *separatorWidget;
    QLabel *separatorLabel;
    QComboBox *separatorComboBox;
    QWidget *patternWidget;
    QLabel *patternLabel;
    QComboBox *patternComboBox;
    QPushButton *generateButton;
    QPushButton *closeButton;

    QCompleter *serviceCompleter;
    QFutureWatcher<std::string> *generationWatcher;
    std::string generatedPassword;
    ProgressDialog *progressDialog;
    ManualDialog *manualDialog;
};

int run_gui(int argc, char *argv[]);
