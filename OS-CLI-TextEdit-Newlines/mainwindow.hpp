#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QProcess>

class QTextEdit;
class QTcpServer;
class QTcpSocket;
class QToolButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void playButtonClicked(bool t_checked);

    void onRunProcessFinished(int exitCode, QProcess::ExitStatus status);

    void onRunProcessErrored(QProcess::ProcessError error);

    void onNewConnection();

    void onRunDataReady();
    void readyReadStandardError();
    void readyReadStandardOutput();


    QTextEdit* m_textInfo;
    QProcess* m_runProcess;
    QTcpServer* m_runTcpServer;
    QTcpSocket* m_runSocket;
    QToolButton* m_playButton;
    bool m_hasSocketConnexion = false;

};
#endif // MAINWINDOW_HPP
