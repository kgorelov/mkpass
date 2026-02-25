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
    void updateCharacterClassesState();
    void updateCustomCharsState();

private:
    void setupUI();

    QLineEdit *masterPasswordLineEdit;
    QLineEdit *repeatPasswordLineEdit;
    QLineEdit *serviceLineEdit;
    QComboBox *algorithmComboBox;
    QCheckBox *upperCaseCheckBox;
    QCheckBox *lowerCaseCheckBox;
    QCheckBox *digitsCheckBox;
    QCheckBox *symbolsCheckBox;
    QCheckBox *customCheckBox;
    QLineEdit *customCharsLineEdit;
    QSpinBox *lengthSpinBox;
    QPushButton *generateButton;
    QPushButton *closeButton;

    QFutureWatcher<std::string> *generationWatcher;
    std::string generatedPassword;
    ProgressDialog *progressDialog;
};

int run_gui(int argc, char *argv[]);
