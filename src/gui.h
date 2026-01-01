#pragma once

#include <QMainWindow>
#include <QFutureWatcher>

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

private slots:
    void generatePassword();
    void generationFinished();
    void serviceChanged(const QString &service);

private:
    void setupUI();

    QLineEdit *masterPasswordLineEdit;
    QLineEdit *repeatPasswordLineEdit;
    QGroupBox *repeatPasswordGroupBox;
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
