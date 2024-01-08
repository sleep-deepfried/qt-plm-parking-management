#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <QMainWindow>
#include <QDateTime>
#include <QDate>
#include <QTimer>
#include <QDir>
#include <QtSql>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTableWidgetItem>
#include <QtWidgets>
#include <QSqlQueryModel>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QPieModelMapper>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLineSeries>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QCategoryAxis>


namespace Ui {
class Dashboard;
class QSqlTableModel;
}


class Dashboard : public QMainWindow
{
    Q_OBJECT

public:
    explicit Dashboard(QWidget *parent = nullptr);
    ~Dashboard();
    void getName(QString buttonName);
    QString namesearch;

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

    QPushButton* getButtonByName(const QString& buttonName);
    QSqlQueryModel* model;
    QChartView *chartView = nullptr;
    QChartView *chartView1 = nullptr;
    QChart *chart;
    QChart *chart1;
    QLineSeries *series;
    void setName(QString name);
    QString attendant;

    void logoutfunc();
    void closefunc();


private slots:
    void on_pushButton_Exit_clicked();

    void updateDateTimeLabel();

    void on_pushButton_Home_clicked();

    void on_pushButton_Parking_clicked();

    void on_pushButton_Reports_clicked();

    void on_pushButton_Logout_clicked();

    void on_pushButton_Uac_clicked();

    void on_pushButton_Tanghalang_Bayan_clicked();

    void on_pushButton_Canteen_clicked();

    void on_pushButton_Entrance_clicked();

    void on_pushButton_Exit_1_clicked();

    void on_pushButton_Exit_2_clicked();



    void on_pushButton_Home_1_clicked();

    void on_pushButton_Parking_1_clicked();

    void on_pushButton_Reports_1_clicked();

    void on_pushButton_Home_2_clicked();

    void on_pushButton_Parking_2_clicked();

    void on_pushButton_Reports_2_clicked();

    void on_pushButton_Canteen_01_clicked();

    void on_pushButton_Canteen_02_clicked();

    void on_pushButton_Canteen_08_clicked();

    void on_pushButton_Canteen_03_clicked();

    void on_pushButton_Canteen_04_clicked();

    void on_pushButton_Canteen_05_clicked();

    void on_pushButton_Canteen_06_clicked();

    void on_pushButton_Canteen_07_clicked();

    void on_pushButton_UAC_01_clicked();

    void on_pushButton_UAC_02_clicked();

    void on_pushButton_UAC_03_clicked();

    void on_pushButton_UAC_04_clicked();

    void on_pushButton_UAC_05_clicked();

    void on_pushButton_UAC_06_clicked();

    void on_pushButton_UAC_07_clicked();

    void on_pushButton_Tanghalang_Bayan_01_clicked();

    void on_pushButton_Tanghalang_Bayan_02_clicked();

    void on_pushButton_Tanghalang_Bayan_03_clicked();

    void on_pushButton_Tanghalang_Bayan_04_clicked();

    void on_pushButton_Tanghalang_Bayan_05_clicked();

    void on_pushButton_Tanghalang_Bayan_06_clicked();

    void on_pushButton_Tanghalang_Bayan_07_clicked();

    void on_pushButton_Tanghalang_Bayan_08_clicked();

    void on_pushButton_Tanghalang_Bayan_09_clicked();

    void on_pushButton_Tanghalang_Bayan_10_clicked();

    void on_pushButton_Tanghalang_Bayan_11_clicked();

    void on_pushButton_Tanghalang_Bayan_12_clicked();

    void on_pushButton_Tanghalang_Bayan_13_clicked();

    void on_pushButton_Tanghalang_Bayan_14_clicked();

    void on_pushButton_Tanghalang_Bayan_15_clicked();

    void on_pushButton_Tanghalang_Bayan_16_clicked();

    void on_pushButton_Tanghalang_Bayan_17_clicked();

    void on_pushButton_Tanghalang_Bayan_18_clicked();

    void on_pushButton_Tanghalang_Bayan_19_clicked();

    void on_pushButton_Tanghalang_Bayan_20_clicked();

    void on_pushButton_Tanghalang_Bayan_21_clicked();

    void on_pushButton_Tanghalang_Bayan_22_clicked();

    void on_pushButton_Tanghalang_Bayan_23_clicked();

    void on_pushButton_Tanghalang_Bayan_24_clicked();

    void on_pushButton_Tanghalang_Bayan_25_clicked();

    void on_pushButton_Tanghalang_Bayan_26_clicked();

    void on_pushButton_Tanghalang_Bayan_27_clicked();

    void on_pushButton_Tanghalang_Bayan_28_clicked();

    void on_pushButton_Tanghalang_Bayan_29_clicked();

    void on_pushButton_Tanghalang_Bayan_30_clicked();

    void on_pushButton_Tanghalang_Bayan_31_clicked();

    void on_pushButton_Tanghalang_Bayan_32_clicked();

    void on_pushButton_Tanghalang_Bayan_33_clicked();

    void on_pushButton_Tanghalang_Bayan_34_clicked();

    void on_pushButton_Tanghalang_Bayan_35_clicked();

    void on_pushButton_Tanghalang_Bayan_36_clicked();

    void on_pushButton_Tanghalang_Bayan_37_clicked();

    void on_pushButton_Tanghalang_Bayan_38_clicked();

    void on_pushButton_Tanghalang_Bayan_39_clicked();

    void on_pushButton_Tanghalang_Bayan_40_clicked();

    void on_pushButton_57pushButton_Tanghalang_Bayan_41_clicked();

    void on_pushButton_Entrance_42_clicked();

    void on_pushButton_Entrance_41_clicked();

    void on_pushButton_Entrance_01_clicked();

    void on_pushButton_Entrance_02_clicked();

    void on_pushButton_Entrance_03_clicked();

    void on_pushButton_Entrance_04_clicked();

    void on_pushButton_Entrance_05_clicked();

    void on_pushButton_Entrance_06_clicked();

    void on_pushButton_Entrance_07_clicked();

    void on_pushButton_Entrance_08_clicked();

    void on_pushButton_Entrance_09_clicked();

    void on_pushButton_Entrance_10_clicked();

    void on_pushButton_Entrance_11_clicked();

    void on_pushButton_Entrance_12_clicked();

    void on_pushButton_Entrance_13_clicked();

    void on_pushButton_Entrance_14_clicked();

    void on_pushButton_Entrance_15_clicked();

    void on_pushButton_Entrance_16_clicked();

    void on_pushButton_Entrance_17_clicked();

    void on_pushButton_Entrance_18_clicked();

    void on_pushButton_Entrance_19_clicked();

    void on_pushButton_Entrance_20_clicked();

    void on_pushButton_Entrance_21_clicked();

    void on_pushButton_Entrance_22_clicked();

    void on_pushButton_Entrance_23_clicked();

    void on_pushButton_Entrance_24_clicked();

    void on_pushButton_Entrance_25_clicked();

    void on_pushButton_Entrance_26_clicked();

    void on_pushButton_Entrance_27_clicked();

    void on_pushButton_Entrance_28_clicked();

    void on_pushButton_Entrance_29_clicked();

    void on_pushButton_Entrance_30_clicked();

    void on_pushButton_Entrance_31_clicked();

    void on_pushButton_Entrance_32_clicked();

    void on_pushButton_Entrance_33_clicked();

    void on_pushButton_Entrance_34_clicked();

    void on_pushButton_Entrance_35_clicked();

    void on_pushButton_Entrance_36_clicked();

    void on_pushButton_Entrance_37_clicked();

    void on_pushButton_Entrance_38_clicked();

    void on_pushButton_Entrance_39_clicked();

    void on_pushButton_Entrance_40_clicked();

    void on_pushButton_UAC_001_clicked();

    void on_pushButton_UAC_002_clicked();

    void on_pushButton_UAC_003_clicked();

    void on_pushButton_UAC_004_clicked();

    void on_pushButton_UAC_005_clicked();

    void on_pushButton_UAC_006_clicked();

    void on_pushButton_UAC_007_clicked();

    void on_pushButton_Tanghalang_Bayan_001_clicked();

    void on_pushButton_Tanghalang_Bayan_002_clicked();

    void on_pushButton_Tanghalang_Bayan_003_clicked();

    void on_pushButton_Tanghalang_Bayan_004_clicked();

    void on_pushButton_Tanghalang_Bayan_005_clicked();

    void on_pushButton_Tanghalang_Bayan_006_clicked();

    void on_pushButton_Tanghalang_Bayan_007_clicked();

    void on_pushButton_Tanghalang_Bayan_008_clicked();

    void on_pushButton_Tanghalang_Bayan_009_clicked();

    void on_pushButton_Tanghalang_Bayan_010_clicked();

    void on_pushButton_Tanghalang_Bayan_011_clicked();

    void on_pushButton_Tanghalang_Bayan_012_clicked();

    void on_pushButton_Tanghalang_Bayan_013_clicked();

    void on_pushButton_Tanghalang_Bayan_014_clicked();

    void on_pushButton_Tanghalang_Bayan_015_clicked();

    void on_pushButton_Tanghalang_Bayan_016_clicked();

    void on_pushButton_Tanghalang_Bayan_017_clicked();

    void on_pushButton_Tanghalang_Bayan_018_clicked();

    void on_pushButton_Tanghalang_Bayan_019_clicked();

    void on_pushButton_Tanghalang_Bayan_020_clicked();

    void on_pushButton_Tanghalang_Bayan_021_clicked();

    void on_pushButton_Tanghalang_Bayan_022_clicked();

    void on_pushButton_Tanghalang_Bayan_023_clicked();

    void on_pushButton_Tanghalang_Bayan_024_clicked();

    void on_pushButton_Tanghalang_Bayan_025_clicked();

    void on_pushButton_Tanghalang_Bayan_026_clicked();

    void on_pushButton_Tanghalang_Bayan_027_clicked();

    void on_pushButton_Tanghalang_Bayan_028_clicked();

    void on_pushButton_Tanghalang_Bayan_029_clicked();

    void on_pushButton_Tanghalang_Bayan_030_clicked();

    void on_pushButton_Tanghalang_Bayan_031_clicked();

    void on_pushButton_Tanghalang_Bayan_032_clicked();

    void on_pushButton_Tanghalang_Bayan_033_clicked();

    void on_pushButton_Tanghalang_Bayan_034_clicked();

    void on_pushButton_Tanghalang_Bayan_035_clicked();

    void on_pushButton_Tanghalang_Bayan_036_clicked();

    void on_pushButton_Tanghalang_Bayan_037_clicked();

    void on_pushButton_Tanghalang_Bayan_038_clicked();

    void on_pushButton_Tanghalang_Bayan_039_clicked();

    void on_pushButton_Tanghalang_Bayan_040_clicked();

    void on_pushButton_Tanghalang_Bayan_041_clicked();

    void on_pushButton_Canteen_001_clicked();

    void on_pushButton_Canteen_002_clicked();

    void on_pushButton_Canteen_003_clicked();

    void on_pushButton_Canteen_004_clicked();

    void on_pushButton_Canteen_005_clicked();

    void on_pushButton_Canteen_006_clicked();

    void on_pushButton_Canteen_007_clicked();

    void on_pushButton_Canteen_008_clicked();

    void on_pushButton_Entrance_001_clicked();

    void on_pushButton_Entrance_002_clicked();

    void on_pushButton_Entrance_003_clicked();

    void on_pushButton_Entrance_004_clicked();

    void on_pushButton_Entrance_005_clicked();

    void on_pushButton_Entrance_006_clicked();

    void on_pushButton_Entrance_007_clicked();

    void on_pushButton_Entrance_008_clicked();

    void on_pushButton_Entrance_009_clicked();

    void on_pushButton_Entrance_010_clicked();

    void on_pushButton_Entrance_011_clicked();

    void on_pushButton_Entrance_012_clicked();

    void on_pushButton_Entrance_013_clicked();

    void on_pushButton_Entrance_014_clicked();

    void on_pushButton_Entrance_015_clicked();

    void on_pushButton_Entrance_016_clicked();

    void on_pushButton_Entrance_017_clicked();

    void on_pushButton_Entrance_018_clicked();

    void on_pushButton_Entrance_019_clicked();

    void on_pushButton_Entrance_020_clicked();

    void on_pushButton_Entrance_021_clicked();

    void on_pushButton_Entrance_022_clicked();

    void on_pushButton_Entrance_023_clicked();

    void on_pushButton_Entrance_024_clicked();

    void on_pushButton_Entrance_025_clicked();

    void on_pushButton_Entrance_026_clicked();

    void on_pushButton_Entrance_027_clicked();

    void on_pushButton_Entrance_028_clicked();

    void on_pushButton_Entrance_029_clicked();

    void on_pushButton_Entrance_030_clicked();

    void on_pushButton_Entrance_031_clicked();

    void on_pushButton_Entrance_032_clicked();

    void on_pushButton_Entrance_033_clicked();

    void on_pushButton_Entrance_034_clicked();

    void on_pushButton_Entrance_035_clicked();

    void on_pushButton_Entrance_036_clicked();

    void on_pushButton_Entrance_037_clicked();

    void on_pushButton_Entrance_038_clicked();

    void on_pushButton_Entrance_039_clicked();

    void on_pushButton_Entrance_040_clicked();

    void on_pushButton_Entrance_041_clicked();

    void on_pushButton_Entrance_042_clicked();

    void setButtonOccupiedStyleSheet(const QString& buttonName);

    void setButtonAvailableStyleSheet(const QString& buttonName);

    void updateButtonStylesheet();

    void updateTable();

    void searchTable(const QString &searchTable);

    void createPieChart();

    void createLineChart();

    void createIntervalChart();


    void on_pushButton_Logout_3_clicked();

    void on_pushButton_Logout_4_clicked();

private:
    Ui::Dashboard *ui;
    QTimer* dateTimer;
    QString DateTimeText;
    QString sqlDateTimeText;

    QVector<QString> xValues;
    QVector<qint64> yValues;
};

#endif // DASHBOARD_H
