#ifndef MESSAGEDIALOG_H
#define MESSAGEDIALOG_H

#include <QDialog>

namespace Ui {
    class messageDialog;
}

class messageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit messageDialog(QString sender, QString sent, QString text, QWidget *parent = 0);
    ~messageDialog();

private:
    Ui::messageDialog *ui;
};

#endif // MESSAGEDIALOG_H
