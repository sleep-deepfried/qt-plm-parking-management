#ifndef LOGIN_H
#define LOGIN_H

#include <QMainWindow>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>
#include <QMessageBox>
#include <QSqlQuery>

#include "splash.h"
#include "parkingdialog.h"




QT_BEGIN_NAMESPACE
namespace Ui { class Login; }
QT_END_NAMESPACE

class Login : public QMainWindow

{
    Q_OBJECT

public:
    Login(QWidget *parent = nullptr);
    ~Login();

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
    //bool isHidden() const;


signals:
    void windowHidden();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_login_clicked();

    void on_lineEdit_admin_returnPressed();

    void on_lineEdit_password_returnPressed();

private:
    Ui::Login *ui;
    Splash *splash;
    QMessageBox loginMessage;
    QSqlDatabase db;
};

#endif // LOGIN_H
