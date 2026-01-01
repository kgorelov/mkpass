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
    QSpinBox *lengthSpinBox;
    QPushButton *generateButton;
    QPushButton *closeButton;

    QFutureWatcher<std::string> *generationWatcher;
    std::string generatedPassword;
};

int run_gui(int argc, char *argv[]);
