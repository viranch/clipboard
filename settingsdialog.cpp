#include <QSettings>

#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    QSettings s("Stealth Flash", "Clipboard");
    QString nick = s.value("nickname", QString(getenv("USER"))).toString();
    if (!s.contains("nickname"))
        s.setValue("nickname", nick);
    ui->nickEdit->setText(nick);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

QString SettingsDialog::getNick()
{
    return ui->nickEdit->text();
}
