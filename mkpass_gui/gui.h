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
    void checkPasswords();
    void updateAlgorithmSpecificUI();
    void updateCustomCharsState();
    void updateSubstitutionsState();

private:
    void setupUI();

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
    QPushButton *generateButton;
    QPushButton *closeButton;

    QFutureWatcher<std::string> *generationWatcher;
    std::string generatedPassword;
    ProgressDialog *progressDialog;
};

int run_gui(int argc, char *argv[]);
