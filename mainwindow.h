#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkReply>
#include <QFileSystemWatcher>
#include <QSystemTrayIcon>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;  // Handle closing/minimizing to tray
    void changeEvent(QEvent *event) override;      // Handle window state changes

private slots:
    void selectExeFile();                          // Slot for selecting the game's .exe file
    void fetchMasterFromGitHub();                  // Slot to fetch the master IP from GitHub
    void onMasterIPFetched(QNetworkReply *reply);  // Slot to handle the response from GitHub
    void stopProgram();                            // Slot to stop/exit the program
    void onIniFileChanged(const QString &path);    // Slot to handle .ini file changes
    void toggleMinimizeToTray();                   // Slot to toggle minimize-to-tray behavior
    void handleTrayIconActivated(QSystemTrayIcon::ActivationReason reason);  // Handle tray icon clicks
    void showHelp();                               // Slot for Help button
    void showCredits();                            // Slot for Credits button
    void loadConfigFile();                         // Load config data at startup
    void saveConfigFile();                         // Save config data when closing

private:
    void deleteIniFile();                          // Function to delete the INI file if needed
    void compareAndUpdateIni(const QString &iniFilePath, const QString &newMasterValue);  // Compare and update .ini

    Ui::MainWindow *ui;                            // Pointer to the UI
    QString configFilePath;                        // Path to store config file location
    QFileSystemWatcher *watcher;                   // File watcher to monitor the .ini file
    QSystemTrayIcon *trayIcon;                     // Tray icon for system tray support
    bool minimizeToTray;                           // Flag to control minimize-to-tray behavior
};

#endif // MAINWINDOW_H
