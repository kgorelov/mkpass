#pragma once

#include <QMainWindow>
#include <QFutureWatcher>
#include <QCloseEvent>
#include <optional>
#include <string>

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
namespace mkpass { class UpdateManager; }

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    friend class MainWindowTest;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void generatePassword();
    void generationFinished();
    void serviceChanged(const QString &service);
    void editServiceComment();
    void validateInputs();
    void updateAlgorithmSpecificUI();
    void updateCustomCharsState();
    void updateSubstitutionsState();
    void updatePatternsList();
    void manageDatabase();
    void showManual();
    void showHelp();
    void showSettings();
    void checkForUpdates();

private:
    void setupUI();
    void refreshCompleter();
    void updateAlgorithmComboBox(bool force_include_old = false);
    void updateCommentButtonState();

    QLineEdit *masterPasswordLineEdit;
    QLineEdit *repeatPasswordLineEdit;
    QLineEdit *serviceLineEdit;
    QPushButton *serviceCommentButton;
    std::optional<std::string> currentComment;
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
    mkpass::UpdateManager *updateManager_ = nullptr;
};

int run_gui(int argc, char *argv[]);
