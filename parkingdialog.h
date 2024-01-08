#ifndef PARKINGDIALOG_H
#define PARKINGDIALOG_H


#include <QDateTime>
#include <QTimer>
#include <QDialog>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>
#include <QMessageBox>
#include <QSqlQuery>
#include "dashboard.h"
#include "login.h"

namespace Ui {
class ParkingDialog;
}

class ParkingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ParkingDialog(QWidget *parent = nullptr);
    ~ParkingDialog();

    QSqlDatabase mydb;
    void connClose()
    {
        mydb.close();
        mydb.removeDatabase(QSqlDatabase::defaultConnection);
    }
    bool connOpen()
    {
        mydb = QSqlDatabase::addDatabase("QSQLITE");
        mydb.setDatabaseName("database/database.db");

        if (!mydb.open()){
            qDebug() << ("Failed to open database");
            return false;
        }

        else{
            qDebug() << ("Conneceted....");
            return true;
        }
    }
    Dashboard *dashboard;
    void setName(QString name);
    void getName(QString buttonName);
    void display();
    QString attendantName;
private slots:
    void on_pushButton_Cancel_clicked();

    void on_pushButton_Proceed_clicked();

    void updateDateTime();

private:
    Ui::ParkingDialog *ui;
    QTimer* dateTimer;
    QString sqlDateTimeText;
    QString ParkingAttendant;
    QString ButtonName;

};

#endif // PARKINGDIALOG_H
