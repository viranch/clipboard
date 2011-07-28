#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QTimer>
#include <QClipboard>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTreeWidgetItem>
#include <QSystemTrayIcon>

namespace Ui {
    class ClipBoard;
}

class ClipBoard : public QMainWindow
{
    Q_OBJECT

public:
    explicit ClipBoard(QWidget *parent = 0);
    ~ClipBoard();
    void updatePeople(QString senderIP, QString status);

private slots:
    void readDatagrams();
    void broadcast();
    void updateTooltip();
    void connectToClient();
    void receiveData();
    void updateCheckBoxes(bool checked);
    void showMessageBox (QTreeWidgetItem* item, int column);
    void toggleWindow(QSystemTrayIcon::ActivationReason reason);
    void on_actionQuit_triggered();
    void on_pasteButton_clicked();
    void on_actionSettings_triggered();

private:
    Ui::ClipBoard *ui;
    QString m_nick;
    QSystemTrayIcon *m_tray;
    QMenu *m_trayMenu;
    QUdpSocket *m_udpSocket;
    QTimer *m_broadcastTimer;
    QClipboard *m_clipboard;
    QTcpServer *m_tcpServer;
    QTcpSocket *m_tcpClient;
    QHash<QString, QString> m_people;
};

#endif // CLIPBOARD_H
