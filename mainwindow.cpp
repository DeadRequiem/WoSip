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
#include <QProcess>

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

    QApplication::setWindowIcon(QIcon(":/ico/ico.png"));
    trayIcon->setIcon(QIcon(":/ico/ico32.png"));
    trayIcon->setToolTip("WoSip Configuration");

    QMenu *trayMenu = new QMenu(this);
    QAction *restoreAction = new QAction("Restore", this);
    QAction *quitAction = new QAction("Quit", this);
    trayMenu->addAction(restoreAction);
    trayMenu->addAction(quitAction);
    trayIcon->setContextMenu(trayMenu);

    connect(restoreAction, &QAction::triggered, this, &QWidget::showNormal);
    connect(quitAction, &QAction::triggered, this, &QApplication::quit);
    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::handleTrayIconActivated);
    trayIcon->show();

    connect(ui->pushButton_selectExe, &QPushButton::clicked, this, &MainWindow::selectExeFile);
    connect(ui->pushButton_stopProgram, &QPushButton::clicked, this, &MainWindow::stopProgram);
    connect(ui->pushButton_help, &QPushButton::clicked, this, &MainWindow::showHelp);
    connect(ui->pushButton_credits, &QPushButton::clicked, this, &MainWindow::showCredits);
    connect(ui->checkBox_minimizeTray, &QCheckBox::toggled, this, &MainWindow::toggleMinimizeToTray);

    connect(ui->pushButton_refreshMaster, &QPushButton::clicked, this, &MainWindow::fetchMasterFromGitHub);
    connect(ui->checkBox_overrideGit, &QCheckBox::toggled, this, [this](bool checked) {
        ui->lineEdit_masterFromGit->setReadOnly(!checked);
    });

    connect(ui->pushButton_runGame, &QPushButton::clicked, this, [this]() {
        QString path = ui->lineEdit_exeFilePath->text();
        if (!path.isEmpty())
            QProcess::startDetached(path);
    });

    ui->label_currentMasterIP->setVisible(false);
    ui->lineEdit_currentMasterIP->setVisible(false);

    loadConfigFile();
    fetchMasterFromGitHub();
    connect(watcher, &QFileSystemWatcher::fileChanged, this, &MainWindow::onIniFileChanged);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::showHelp() {
    QMessageBox::information(this, "Help",
                             "Steps:\n"
                             "1. Navigate to the Well of Souls folder.\n"
                             "2. Select the WoS.exe.\n"
                             "3. Leave this program running.\n"
                             "4. Start the game.\n"
                             "5. Go to Server Select screen.\n"
                             "6. Back out and re-enter the screen.");
}

void MainWindow::showCredits() {
    QMessageBox::information(this, "Credits",
                             "1. Starix - Black magic knowledge.\n"
                             "2. Demi - Program creation.");
}

void MainWindow::toggleMinimizeToTray() {
    minimizeToTray = ui->checkBox_minimizeTray->isChecked();
}

void MainWindow::loadConfigFile() {
    QFile configFile(configFilePath);
    if (configFile.exists() && configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&configFile);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();

            if (line.startsWith("iniFilePath = "))
                ui->lineEdit_exeFilePath->setText(line.mid(14).trimmed());

            if (line.startsWith("deleteIniOnClose = "))
                ui->checkBox_deleteIniOnClose->setChecked(line.mid(20).trimmed() == "true");

            if (line.startsWith("minimizeToTray = ")) {
                minimizeToTray = (line.mid(17).trimmed() == "true");
                ui->checkBox_minimizeTray->setChecked(minimizeToTray);
            }
        }
        configFile.close();
    }
}

void MainWindow::saveConfigFile() {
    QFile configFile(configFilePath);
    if (configFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&configFile);
        out << "# WoSip Configuration Settings\n\n";
        out << "[Paths]\n";
        out << "iniFilePath = " << ui->lineEdit_exeFilePath->text() << "\n\n";
        out << "[Settings]\n";
        out << "deleteIniOnClose = " << (ui->checkBox_deleteIniOnClose->isChecked() ? "true" : "false") << "\n";
        out << "minimizeToTray = " << (ui->checkBox_minimizeTray->isChecked() ? "true" : "false") << "\n";
        configFile.close();
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (minimizeToTray) {
        hide();
        trayIcon->show();
        event->ignore();
    } else {
        if (ui->checkBox_deleteIniOnClose->isChecked())
            deleteIniFile();
        saveConfigFile();
        trayIcon->hide();
        delete trayIcon;
        event->accept();
    }
}

void MainWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::WindowStateChange && isMinimized() && minimizeToTray) {
        hide();
        trayIcon->show();
        event->ignore();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::handleTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        showNormal();
        trayIcon->hide();
    }
}

void MainWindow::deleteIniFile() {
    QString iniFilePath = ui->lineEdit_exeFilePath->text();
    if (QFile::exists(iniFilePath))
        QFile::remove(iniFilePath);
}

void MainWindow::selectExeFile() {
    QString exeFilePath = QFileDialog::getOpenFileName(this, tr("Select Game Executable"), QDir::homePath(), tr("Executable Files (*.exe)"));
    if (!exeFilePath.isEmpty()) {
        ui->lineEdit_exeFilePath->setText(exeFilePath);
        ui->pushButton_runGame->setEnabled(true);

        QDir exeDir = QFileInfo(exeFilePath).absoluteDir();
        QDir tempDir = exeDir.filePath("temp");
        if (tempDir.exists()) {
            QString iniFilePath = tempDir.absoluteFilePath("synReal.ini");
            if (!QFile::exists(iniFilePath))
                watcher->addPath(tempDir.absolutePath());
            else {
                watcher->addPath(iniFilePath);
                compareAndUpdateIni(iniFilePath, ui->lineEdit_masterFromGit->text());
            }
            saveConfigFile();
        }
    }
}

void MainWindow::onIniFileChanged(const QString &path) {
    if (QFile::exists(path)) {
        if (QFileInfo(path).isDir()) {
            QString iniFilePath = QDir(path).absoluteFilePath("synReal.ini");
            if (QFile::exists(iniFilePath)) {
                watcher->addPath(iniFilePath);
                watcher->removePath(path);
            }
        } else {
            compareAndUpdateIni(path, ui->lineEdit_masterFromGit->text());
        }
    }
}

void MainWindow::fetchMasterFromGitHub() {
    if (ui->checkBox_overrideGit->isChecked())
        return;

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, &MainWindow::onMasterIPFetched);

    QNetworkRequest request((QUrl(gitMasterUrl)));
    QNetworkReply *reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::warning(this, "Error", "Failed to fetch the master IP: " + reply->errorString());
        }
        reply->deleteLater();
    });
}

void MainWindow::onMasterIPFetched(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QString masterValue = reply->readAll().trimmed();
        ui->lineEdit_masterFromGit->setText(masterValue);
    } else {
        QMessageBox::warning(this, "Error", "Failed to fetch the master IP: " + reply->errorString());
    }
    reply->deleteLater();
}

void MainWindow::compareAndUpdateIni(const QString &iniFilePath, const QString &newMasterValue) {
    QFile iniFile(iniFilePath);
    if (!iniFile.open(QIODevice::ReadWrite | QIODevice::Text))
        return;

    QStringList lines;
    bool masterUpdated = false;
    QTextStream in(&iniFile);
    QString currentMasterValue;

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.trimmed().startsWith("master=")) {
            currentMasterValue = line.mid(QString("master=").length()).trimmed();
            ui->lineEdit_currentMasterIP->setText(currentMasterValue);
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

    if (masterUpdated) {
        if (iniFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            QTextStream out(&iniFile);
            for (const QString &line : lines)
                out << line << "\n";
            iniFile.close();
        }
    }
}

void MainWindow::stopProgram() {
    QApplication::quit();
}
