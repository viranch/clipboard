#include "messagedialog.h"
#include "ui_messagedialog.h"

messageDialog::messageDialog(QString sender, QString sent, QString text, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::messageDialog)
{
    ui->setupUi(this);
    this->setWindowTitle("Text from "+sender);
    ui->senderLabel->setText(sender);
    ui->timeLabel->setText(sent);
    ui->messageBox->setPlainText(text);
}

messageDialog::~messageDialog()
{
    delete ui;
}
