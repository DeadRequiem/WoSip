#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCloseEvent>
#include <QDir>

const QString gitMasterUrl = "https://raw.githubusercontent.com/DeadRequiem/DeadRequiem.github.io/main/master_value.txt";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , minimizeToTray(false)
    , trayIcon(new QSystemTrayIcon(this))
    , watcher(new QFileSystemWatcher(this))
{
    ui->setupUi(this);
    configFilePath = "config.txt";

    // Set the application icon globally
    QApplication::setWindowIcon(QIcon(":/ico/ico.png"));  // .exe icon

    // Set the tray icon for system tray
    trayIcon->setIcon(QIcon(":/ico/ico32.png"));
    trayIcon->setToolTip("WoSip Configuration");

    // Create the tray menu
    QMenu *trayMenu = new QMenu(this);
    QAction *restoreAction = new QAction("Restore", this);
    QAction *quitAction = new QAction("Quit", this);
    trayMenu->addAction(restoreAction);
    trayMenu->addAction(quitAction);
    trayIcon->setContextMenu(trayMenu);

    // Connect tray actions
    connect(restoreAction, &QAction::triggered, this, &QWidget::showNormal);
    connect(quitAction, &QAction::triggered, this, &QApplication::quit);
    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::handleTrayIconActivated);
    trayIcon->show();  // Ensure tray icon is visible when the app starts

    // Connect other buttons
    connect(ui->pushButton_selectExe, &QPushButton::clicked, this, &MainWindow::selectExeFile);
    connect(ui->pushButton_stopProgram, &QPushButton::clicked, this, &MainWindow::stopProgram);

    // Help and Credits buttons
    connect(ui->pushButton_help, &QPushButton::clicked, this, &MainWindow::showHelp);
    connect(ui->pushButton_credits, &QPushButton::clicked, this, &MainWindow::showCredits);

    // Minimize to tray toggle
    connect(ui->checkBox_minimizeTray, &QCheckBox::toggled, this, &MainWindow::toggleMinimizeToTray);

    // Fetch master IP from GitHub
    fetchMasterFromGitHub();

    // Monitor the .ini file for changes
    connect(watcher, &QFileSystemWatcher::fileChanged, this, &MainWindow::onIniFileChanged);
}

MainWindow::~MainWindow() {
    delete ui;
}

// Function to show Help message
void MainWindow::showHelp() {
    QMessageBox::information(this, "Help",
                             "Steps:\n"
                             "1. Navigate to the Well of Souls folder.\n"
                             "2. Select the WoS.exe.\n"
                             "3. Be sure to leave the program running.\n"
                             "4. Run the game.\n"
                             "5. Navigate to the Server Select screen.\n"
                             "6. Back out to the Solo/Multiplayer select screen.\n"
                             "7. Navigate back to the Server Select screen.");
}

// Function to show Credits message
void MainWindow::showCredits() {
    QMessageBox::information(this, "Credits",
                             "Credits:\n"
                             "1. Starix - For discovering the black magic needed to make this possible.\n"
                             "2. Demi - For the creation of the program.");
}

// Handle minimize-to-tray logic based on checkbox state
void MainWindow::toggleMinimizeToTray() {
    minimizeToTray = ui->checkBox_minimizeTray->isChecked();
}

// Handle closing/minimizing window event
void MainWindow::closeEvent(QCloseEvent *event) {
    if (minimizeToTray) {
        hide();              // Hide the window
        trayIcon->show();    // Ensure the tray icon is shown
        event->ignore();     // Ignore the close event (prevent app from closing)
    } else {
        trayIcon->hide();    // Ensure tray icon is hidden before closing
        delete trayIcon;     // Explicitly delete the tray icon to remove it
        event->accept();     // Close normally if not minimizing to tray
    }
}

// Handle window state changes (e.g., minimize)
void MainWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::WindowStateChange && isMinimized() && minimizeToTray) {
        hide();              // Hide the window
        trayIcon->show();    // Ensure the tray icon is visible when minimized
        event->ignore();     // Ignore the state change event
    }
    QMainWindow::changeEvent(event);  // Call base class method
}

// Handle tray icon activation (e.g., restoring window)
void MainWindow::handleTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        showNormal();    // Restore window
        trayIcon->hide(); // Hide tray icon when window is restored
    }
}

// Function to select the game's .exe file
void MainWindow::selectExeFile() {
    QString exeFilePath = QFileDialog::getOpenFileName(this, tr("Select Game Executable"), QDir::homePath(), tr("Executable Files (*.exe)"));

    if (!exeFilePath.isEmpty()) {
        ui->lineEdit_exeFilePath->setText(exeFilePath);

        // Get the directory of the .exe file
        QDir exeDir = QFileInfo(exeFilePath).absoluteDir();

        // The 'temp' folder location within the same directory as the .exe
        QDir tempDir = exeDir.filePath("temp");

        if (tempDir.exists()) {
            QString iniFilePath = tempDir.absoluteFilePath("synReal.ini");

            if (!QFile::exists(iniFilePath)) {
                watcher->addPath(tempDir.absolutePath());
            } else {
                watcher->addPath(iniFilePath);
                compareAndUpdateIni(iniFilePath, ui->lineEdit_masterFromGit->text());
            }

            // Save ini file path to config.txt
            QFile configFile(configFilePath);
            if (configFile.open(QIODevice::WriteOnly)) {
                QTextStream out(&configFile);
                out << iniFilePath;
                configFile.close();
            }
        }
    }
}

// Monitor for changes in the .ini file
void MainWindow::onIniFileChanged(const QString &path) {
    if (QFile::exists(path)) {
        if (QFileInfo(path).isDir()) {
            QString iniFilePath = QDir(path).absoluteFilePath("synReal.ini");
            if (QFile::exists(iniFilePath)) {
                watcher->addPath(iniFilePath);
                watcher->removePath(path);  // Stop watching folder
            }
        } else {
            compareAndUpdateIni(path, ui->lineEdit_masterFromGit->text());
        }
    }
}

// Fetch the master IP from GitHub
void MainWindow::fetchMasterFromGitHub() {
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, &MainWindow::onMasterIPFetched);

    // Make the network request
    QNetworkRequest request((QUrl(gitMasterUrl)));
    QNetworkReply* reply = manager->get(request);

    // Handle network errors immediately and gracefully
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            // Log the error or show a message
            QMessageBox::warning(this, "Error", "Failed to fetch the master IP from GitHub: " + reply->errorString());
        }
        reply->deleteLater();  // Clean up the reply
    });
}

void MainWindow::onMasterIPFetched(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QString masterValue = reply->readAll().trimmed();
        ui->lineEdit_masterFromGit->setText(masterValue);  // Update UI with the fetched master IP
    } else {
        QMessageBox::warning(this, "Error", "Failed to fetch the master IP: " + reply->errorString());
    }
    reply->deleteLater();
}

// Compare and update master value in the .ini file
void MainWindow::compareAndUpdateIni(const QString &iniFilePath, const QString &newMasterValue) {
    QFile iniFile(iniFilePath);

    if (!iniFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
        return;
    }

    QStringList lines;
    bool masterUpdated = false;
    QString currentMasterValue;
    QTextStream in(&iniFile);

    // Read and update master IP
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.trimmed().startsWith("master=")) {
            currentMasterValue = line.mid(QString("master=").length()).trimmed();  // Extract the current master IP
            ui->lineEdit_currentMasterIP->setText(currentMasterValue);             // Display current master IP in the UI

            if (line != newMasterValue) {
                lines.append(newMasterValue);
                masterUpdated = true;
            } else {
                lines.append(line);
            }
        } else {
            lines.append(line);
        }
    }
    iniFile.close();

    // Write back to ini file if updated
    if (masterUpdated) {
        if (iniFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            QTextStream out(&iniFile);
            for (const QString &line : lines) {
                out << line << "\n";
            }
            iniFile.close();
        }
    }
}

// Function to stop the program
void MainWindow::stopProgram() {
    QApplication::quit();
}
