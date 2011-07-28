#include <QNetworkInterface>
#include <QCheckBox>
#include <QMessageBox>
#include <QDateTime>
#include <QSettings>
#include "clipboard.h"
#include "ui_clipboard.h"
#include "messagedialog.h"
#include "settingsdialog.h"

#define BROADCAST_INTERVAL 5
#define BROADCAST_PORT 2562
#define TCP_PORT 8080
#define MAX_DATA_SIZE 1024

#define UI_CHECK_COLUMN 0
#define UI_NICK_COLUMN 1
#define UI_IP_COLUMN 2

void _broadcast(QString msg);

ClipBoard::ClipBoard(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ClipBoard)
{
    ui->setupUi(this);

    // set headers
    QStringList columns;
    columns << "" << "" << "";
    columns [UI_NICK_COLUMN] = "Nickname";
    columns [UI_IP_COLUMN] = "IP Address";
    /*QTreeWidgetItem *item = new QTreeWidgetItem(columns);
    item->setCheckState(UI_CHECK_COLUMN, Qt::Unchecked);
    ui->peopleList->setHeaderItem(item);*/
    ui->peopleList->setHeaderLabels(columns);
    ui->peopleList->header()->setResizeMode(UI_CHECK_COLUMN, QHeaderView::ResizeToContents);

    // header checkbox
    /*QCheckBox *checkBox = new QCheckBox();
    QStringList columns;
    columns << "" << "Nickname" << "IP Address";
    QTreeWidgetItem *header = new QTreeWidgetItem(columns);
    ui->peopleList->setItemWidget(header, 0, checkBox);
    ui->peopleList->setHeaderItem(header);
    connect(checkBox, SIGNAL(toggled(bool)),
            this, SLOT(updateCheckBoxes(bool)));
    checkBox->setChecked(false);*/

    // nick name
    QSettings s("Stealth Flash", "Clipboard");
    m_nick = s.value("nickname", QString(getenv("USER"))).toString();
    if (!s.contains("nickname"))
        s.setValue("nickname", m_nick);

    // clipboard monitor
    m_clipboard = QApplication::clipboard();
    //ui->clipboardContents->setPlainText(m_clipboard->text());
    connect(m_clipboard, SIGNAL(dataChanged()),
            this, SLOT(updateTooltip()));

    // tray icon
    m_tray = new QSystemTrayIcon(QIcon(":/icons/images/clipboard-icon.png"), this);
    m_tray->show();
    m_tray->setToolTip("Clipboard Contents:\n"+m_clipboard->text());
    connect (m_tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
             this, SLOT(toggleWindow(QSystemTrayIcon::ActivationReason)));
    m_trayMenu = new QMenu(this);
    m_trayMenu->addAction(QIcon(":/icons/images/clipboard-icon.png"), "Quick Paste", this, SLOT(on_pasteButton_clicked()));
    m_trayMenu->addAction(QIcon(":/icons/images/configure.png"), "Settings", this, SLOT(on_actionSettings_triggered()));
    m_trayMenu->addAction(QIcon(":/icons/images/application-exit.png"), "Quit", this, SLOT(on_actionQuit_triggered()));
    m_tray->setContextMenu(m_trayMenu);

    // TCP listener
    m_tcpServer = new QTcpServer(this);
    if (!m_tcpServer->listen(QHostAddress::Any, TCP_PORT)) {
        QMessageBox::critical(this, "ClipBoard", m_tcpServer->errorString());
    }
    else {
        connect(m_tcpServer, SIGNAL(newConnection()),
                this, SLOT(connectToClient()));
    }

    // UDP listener
    m_udpSocket = new QUdpSocket(this);
    m_udpSocket->bind(QHostAddress::Any, BROADCAST_PORT);
    connect (m_udpSocket, SIGNAL(readyRead()),
             this, SLOT(readDatagrams()));

    // UDP talker
    m_broadcastTimer = new QTimer(this);
    connect(m_broadcastTimer, SIGNAL(timeout()),
            this, SLOT(broadcast()));
    broadcast();
    m_broadcastTimer->start(BROADCAST_INTERVAL*1000);

    // message box
    connect (ui->incomingList, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
             this, SLOT(showMessageBox(QTreeWidgetItem*,int)));
}

ClipBoard::~ClipBoard()
{
   delete ui;
}

void ClipBoard::updateTooltip()
{
    //ui->clipboardContents->setPlainText(m_clipboard->text());
    m_tray->setToolTip("Clipboard Contents:\n"+m_clipboard->text());
}

void ClipBoard::toggleWindow(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger)
    {
        this->setHidden(!this->isHidden());
    }
}

void ClipBoard::updateCheckBoxes(bool checked)
{
    for (int i=0; i<ui->peopleList->topLevelItemCount(); i++) {
        QTreeWidgetItem *item = ui->peopleList->topLevelItem(i);
        QCheckBox *checkBox = (QCheckBox*) ui->peopleList->itemWidget(item, UI_CHECK_COLUMN);
        checkBox->setChecked(checked);
    }
}

void ClipBoard::connectToClient()
{
    m_tcpClient = m_tcpServer->nextPendingConnection();

    connect (m_tcpClient, SIGNAL(disconnected()),
             m_tcpClient, SLOT(deleteLater()));

    connect (m_tcpClient, SIGNAL(readyRead()),
             this, SLOT(receiveData()));
}

void ClipBoard::receiveData()
{
    QDataStream in(m_tcpClient);
    char data[MAX_DATA_SIZE];
    int len = in.readRawData(data, MAX_DATA_SIZE);
    data[len] = '\0';
    QString string(data);
    QRegExp rx_msg("0x1d0x1e0x1f0x01(.*)0x1f0x1e0x1d");
    rx_msg.indexIn(string);
    QString msg = rx_msg.cap(1);
    if (msg.isEmpty())
        return;

    QString now = QDateTime::currentDateTime().toString("MMM d, h:mmap");
    QStringList columns;
    QString nick = m_people[m_tcpClient->peerAddress().toString()];
    columns << nick << msg.trimmed() << now;
    QTreeWidgetItem *item = new QTreeWidgetItem(ui->incomingList, columns);
    m_tray->showMessage("Incoming text", "Sender: "+nick);
}

void ClipBoard::updatePeople(QString senderIP, QString datagram)
{
    //QRegExp rx("<idmsg>(.*)</idmsg><nick>(.*)</nick>");
    QRegExp rx_st("0x00(.*)0x1f");
    rx_st.indexIn(datagram);
    QString status = rx_st.cap(1);
    //QString nick = rx.cap(2);

    if (status.toLower()=="live") {
        QRegExp rx_nick("0x02(.*)0x00");
        rx_nick.indexIn(datagram);
        QString nick = rx_nick.cap(1);
        if (!m_people.contains(senderIP)) {
            m_people[senderIP] = nick;
            QStringList columns;
            columns << "" << "" << "";
            columns [UI_NICK_COLUMN] = m_people[senderIP];
            columns [UI_IP_COLUMN] = senderIP;
            QTreeWidgetItem *item = new QTreeWidgetItem(ui->peopleList, columns);
            item->setCheckState(UI_CHECK_COLUMN, Qt::Unchecked);
        }
        else {
            m_people[senderIP] = nick;
            QTreeWidgetItem *item = ui->peopleList->findItems(senderIP, Qt::MatchFixedString, UI_IP_COLUMN)[0];
            item->setText(UI_NICK_COLUMN, nick);
        }
    }
    else if (status.toLower()=="dead") {
        QRegExp rx_nick("0x03(.*)0x00");
        rx_nick.indexIn(datagram);
        QString nick = rx_nick.cap(1);
        QList<QTreeWidgetItem*> items_to_remove = ui->peopleList->findItems(senderIP, Qt::MatchFixedString, UI_IP_COLUMN);
        foreach (QTreeWidgetItem* item, items_to_remove) {
            int index_to_remove = ui->peopleList->indexOfTopLevelItem(item);
            ui->peopleList->takeTopLevelItem(index_to_remove);
        }
        m_people.remove(senderIP);
    }
}

void ClipBoard::readDatagrams()
{
    QHostAddress sender;
    quint16 senderPort;
    char data[1024];

    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpSocket->pendingDatagramSize());

        m_udpSocket->readDatagram(datagram.data(), datagram.size(),
                             &sender, &senderPort);

        //if (sender != QHostAddress::LocalHost) {
            QDataStream in(&datagram, QIODevice::ReadOnly);
            int len = in.readRawData(data, 1024);
            data[len]='\0';
            updatePeople(sender.toString(), QString(data));
        //}
    }
}

void ClipBoard::broadcast()
{
    QString msg = "0x1d0x1e0x1f0x02"+m_nick+"0x00LIVE0x1f0x1e0x1d";
    _broadcast(msg);
}

void _broadcast(QString msg)
{
    foreach (QHostAddress host, QNetworkInterface::allAddresses()) {
        if (host.protocol() == QAbstractSocket::IPv4Protocol) {
            QString ip_addr = host.toString();
            //if (ip_addr == "127.0.0.1") continue;

            // get char* for writing raw message from
            QByteArray msgByteArray = msg.toAscii();
            char *data = msgByteArray.data();

            // write the raw message
            QByteArray block;
            QDataStream out(&block, QIODevice::WriteOnly);
            out.writeRawData(data, msg.length());

            // send the raw message
            QUdpSocket socket;
            socket.writeDatagram(block, QHostAddress::Broadcast, BROADCAST_PORT);
            socket.close();
        }
    }
}

void ClipBoard::showMessageBox (QTreeWidgetItem* item, int column)
{
    messageDialog *dlg = new messageDialog(item->text(0), item->text(2), item->text(1), this);
    dlg->exec();
    delete dlg;
}

void ClipBoard::on_actionQuit_triggered()
{
    m_tcpServer->close();
    m_udpSocket->close();
    m_broadcastTimer->stop();
    QString msg = "0x1d0x1e0x1f0x03"+m_nick+"0x00DEATH0x1f0x1e0x1d";
    _broadcast(msg);
    m_people.clear();
    close();
}

QList<QTreeWidgetItem*> getSelectedItems(QTreeWidget* tree)
{
    QList<QTreeWidgetItem*> selected;
    for (int i=0; i<tree->topLevelItemCount(); i++) {
        QTreeWidgetItem *item = tree->topLevelItem(i);
        if (item->checkState(UI_CHECK_COLUMN) == Qt::Checked)
            selected << item;
    }
    return selected;
}

void ClipBoard::on_pasteButton_clicked()
{
    // prepare the data to be sent
    //QString text = ui->clipboardContents->toPlainText();
    QString text = m_clipboard->text();
    if (text.isEmpty())
        return;
    QString msg = "0x1d0x1e0x1f0x01" + text + "0x1f0x1e0x1d";
    QByteArray byteData = msg.toAscii();
    char *data = byteData.data();

    foreach(QTreeWidgetItem *item, getSelectedItems(ui->peopleList)) {
        QTcpSocket *s = new QTcpSocket(this);
        s->connectToHost(item->text(UI_IP_COLUMN), TCP_PORT, QIODevice::WriteOnly);

        // wrap up the data
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.writeRawData(data, msg.length());

        // write the data to the socket and disconnect
        s->write(block);
        s->disconnectFromHost();
    }
}

void ClipBoard::on_actionSettings_triggered()
{
    SettingsDialog *dlg = new SettingsDialog(this);
    if (dlg->exec()) {
        QSettings s("Stealth Flash", "Clipboard");
        m_nick = dlg->getNick();
        s.setValue("nickname", m_nick);
    }
    delete dlg;
}
