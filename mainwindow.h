#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkReply>
#include <QFileSystemWatcher>
#include <QSystemTrayIcon>
#include <QProcess>

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
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void selectExeFile();
    void fetchMasterFromGitHub();
    void onMasterIPFetched(QNetworkReply *reply);
    void stopProgram();
    void onIniFileChanged(const QString &path);
    void toggleMinimizeToTray();
    void handleTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void showHelp();
    void showCredits();
    void loadConfigFile();
    void saveConfigFile();

private:
    void deleteIniFile();
    void compareAndUpdateIni(const QString &iniFilePath, const QString &newMasterValue);

    Ui::MainWindow *ui;
    QString configFilePath;
    QFileSystemWatcher *watcher;
    QSystemTrayIcon *trayIcon;
    bool minimizeToTray;
};

#endif // MAINWINDOW_H
