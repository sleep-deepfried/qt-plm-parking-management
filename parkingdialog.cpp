#include "parkingdialog.h"
#include "ui_parkingdialog.h"

ParkingDialog::ParkingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ParkingDialog)
{
    ui->setupUi(this);

    ui->comboBox_Vehicle_Type->addItem("4 Wheeler");
    ui->comboBox_Vehicle_Type->addItem("Motorcycle");
    ui->comboBox_Vehicle_Type->addItem("Bicycle");

    // Setting up QIntValidator for the mobile number field
    QIntValidator *validator = new QIntValidator(this);
    ui->lineEdit_Mobile_Numb->setValidator(validator);

    // Setting the date
    dateTimer = new QTimer(this);
    connect(dateTimer, &QTimer::timeout, this, &ParkingDialog::updateDateTime);
    dateTimer->start(1000); // Update every 1000 milliseconds (1 second)
}

ParkingDialog::~ParkingDialog()
{
    delete ui;
    delete dateTimer;
}

void ParkingDialog::on_pushButton_Cancel_clicked()
{
    close();
}

void ParkingDialog::display()
{
    qDebug() << attendantName;
}
void ParkingDialog::on_pushButton_Proceed_clicked()
{
    connOpen();
    // Retrieve data from input fields
    QString name = ui->lineEdit_Name->text();
    QString mobileNumber = ui->lineEdit_Mobile_Numb->text();
    QString plateNumber = ui->lineEdit_Plate_Number->text();
    QString vehicleType = ui->comboBox_Vehicle_Type->currentText();

    // Execute the SQL query to insert data into the table
    QSqlQuery query;
    query.exec("SELECT MAX(ID) FROM Parking");
    query.next();
    int highestId = query.value(0).toInt();

    int newId = highestId + 1;

    query.exec("SELECT AttendantName FROM activeUser LIMIT 1");
    if (query.next()) {
        ParkingAttendant = query.value(0).toString();
    } else {
        qDebug() << "Failed to retrieve AttendantName from activeUser tab";
        connClose();
        return;
    }

    // Execute the SQL query to insert data into the table
    query.prepare("INSERT INTO Parking (ID, CheckIn, Name, PhoneNumber, PlateNumber, VehicleType, ButtonName,ParkingAttendant) "
                  "VALUES (:ID,:CheckIn, :Name, :PhoneNumber, :PlateNumber, :VehicleType, :ButtonName, :ParkingAttendant)");
    query.bindValue(":ID", newId);
    query.bindValue(":CheckIn", sqlDateTimeText);
    query.bindValue(":Name", name);
    query.bindValue(":PhoneNumber", mobileNumber);
    query.bindValue(":PlateNumber", plateNumber);
    query.bindValue(":VehicleType", vehicleType);
    query.bindValue(":ButtonName", ButtonName);
    query.bindValue(":ParkingAttendant", ParkingAttendant);
    if (!query.exec())
    {
        qDebug() << "Failed to insert data into table";
    }
    else
    {
        qDebug() << "Data inserted successfully";
        connClose();
        //dashboard->updateButtonStylesheet();
        this->close();

    }
}

void ParkingDialog::updateDateTime()
{
    QDateTime sqlCurrentDateTime = QDateTime::currentDateTime();
    sqlDateTimeText = sqlCurrentDateTime.toString("MMMM dd,yyyy hh:mm:ss AP");
}

void ParkingDialog::getName(QString buttonName)
{
    ButtonName = buttonName;
}
