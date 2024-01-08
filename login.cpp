#include "login.h"
#include "ui_login.h"
#include <QLineEdit>


Login::Login(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Login)
{
    ui->setupUi(this);

    //removing the title bar
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // Load the app icon file
    QPixmap logo(":/assets/icons/parking-icon.png");
    QIcon icon(logo);
    // Set the window icon
    this->setWindowIcon(icon);

    // setting up the border-radius
    this->setStyleSheet("background-color: rgb(27, 34, 35); border-radius: 10px;");
    //setting a fix size
    this->setFixedSize(this->size());


    //Setting up the database

   //creating an instance for the MessageBox
   loginMessage.setWindowTitle("Invalid");
   loginMessage.setText("Wrong Username and Password");
   loginMessage.setIcon(QMessageBox::Warning);
   //styling the message box
   loginMessage.setStyleSheet("QMessageBox { background-color: yellow; }"
                        "QPushButton {}");
}

Login::~Login()
{
    delete ui;
}


void Login::on_pushButton_clicked()
{
    close();
}


void Login::on_pushButton_login_clicked()
{
    connOpen();

    QString username = ui->lineEdit_admin->text();
    QString password = ui->lineEdit_password->text();

    QString queryStr = QString("SELECT * FROM User WHERE username = '%1' AND password = '%2'")
                           .arg(username)
                           .arg(password);

    QSqlQuery query(queryStr);
    if (query.exec()) {
        if (query.next()) {
            // Username and password matched
            QString attendantName = query.value("name").toString();
            connClose();
            connOpen();
            // Check if the attendant name already exists in the activeUser table
            QString checkQueryStr = QString("SELECT AttendantName FROM activeUser WHERE AttendantName = '%1'").arg(attendantName);
            QSqlQuery checkQuery(checkQueryStr);
            if (checkQuery.next()) {
                // Attendant name already exists in the activeUser table
                qDebug() << "Attendant name already exists in the activeUser table";
            } else {
                // Insert the attendant name into the activeUser table
                QString insertQueryStr = QString("INSERT INTO activeUser (AttendantName) VALUES ('%1')").arg(attendantName);
                QSqlQuery insertQuery(insertQueryStr);
                if (!insertQuery.exec()) {
                    qDebug() << "Failed to insert attendant name into table";
                }
            }

            connClose();
            emit windowHidden();
            this->hide();
            splash = new Splash(this);
            splash->show();
            // Check if the new window is visible
            if (splash->isVisible()) {
                qDebug() << "The new window is visible";
            } else {
                // The new window is not visible
                qDebug() << "The new window is not visible";
            }
        } else {
            // Username and password did not match
            loginMessage.exec();// Handle the case where the credentials are incorrect
        }
    } else {
        qDebug() << "Query execution failed:" << query.lastError().text();
    }
}

void Login::on_lineEdit_admin_returnPressed()
{
    ui->pushButton_login->click();
}

void Login::on_lineEdit_password_returnPressed()
{
    ui->pushButton_login->click();
}

//bool Login::isHidden() const {
//    qDebug() << "isHidden() called";
//    return QWidget::isHidden();
//}
