#include "dashboard.h"
#include "ui_dashboard.h"

#include <QApplication>
#include <QScreen>
#include <QPushButton>
#include <QTransform>
#include <QGraphicsEffect>
#include <QtCharts>
#include <QVBoxLayout>


#include "splash.h"
#include "parkingdialog.h"

Dashboard::Dashboard(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Dashboard)
{
    ui->setupUi(this);

    this->setWindowIcon(QIcon(":/assets/icons/parking-icon.png"));
    //removing the title bar
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    this->setStyleSheet("background-color: rgb(27, 34, 35); border-radius:10px; color: rgb(244, 243, 253);");

    ui->tabWidget->setStyleSheet("QTabWidget::tab-bar { width: 0; height: 0; margin: 0; padding: 0;}"
                                 "QTabWidget{border:none}"
                                 "QTabWidget::pane { border: none; margin: -1px; padding: 0px; }"
                                 "QPushButton{background-color: rgb(58, 79, 80); color: rgb(244, 243, 253);}"
                                 "QPushButton:hover{background-color: rgb(41, 55, 56);}");

    // centering the window
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    int x = (screenGeometry.width() - this->width()) / 2;
    int y = (screenGeometry.height() - this->height()) / 2;
    this->move(x, y);

    //setting the date
    dateTimer = new QTimer(this);
    connect(dateTimer, &QTimer::timeout, this, &Dashboard::updateDateTimeLabel);
    dateTimer->start(1000); // Update every 1000 milliseconds (1 second)

    //set default tab
    ui->stackedWidget->setCurrentIndex(0);
    ui->tabWidget->setCurrentIndex(0);

    updateButtonStylesheet();

    connect(ui->lineEdit_Table_Search, &QLineEdit::textChanged, this, &Dashboard::searchTable);

}

Dashboard::~Dashboard()
{
    delete ui;
}

void Dashboard::on_pushButton_Exit_clicked()
{
    closefunc();
}

void Dashboard:: closefunc()
{
    connOpen();
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(nullptr, "Logout", "Closing the application will logout you from the database", QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        QSqlQuery query;
        bool success = query.exec("DELETE FROM activeUser");

        if (success) {
            // Values nullified successfully
            this->close();
        } else {
            // Error occurred during the query execution, handle it
        }
    } else {
        // User clicked No or closed the message box, do nothing or handle differently
        // ...
    }
    connClose();
}

void Dashboard::logoutfunc()
{
    connOpen();
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(nullptr, "Logout", "Are you sure you want to logout?", QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        QSqlQuery query;
        bool success = query.exec("UPDATE activeUser SET AttendantName = NULL");

        if (success) {
            // Values nullified successfully
            this->close();
        } else {
            // Error occurred during the query execution, handle it
        }
    } else {
        // User clicked No or closed the message box, do nothing or handle differently
        // ...
    }
    connClose();
}

void Dashboard::createIntervalChart()
{
    // Close the previous chart view, if it exists
    if (chartView1) {
        delete chartView1;
    }
    connOpen();
    // Create a line series for the chart
    QLineSeries *series1 = new QLineSeries();

    // Retrieve data from the database
    QSqlQuery query;
    query.exec("SELECT CheckOut FROM Parking");

    // Initialize a frequency map to count the number of data points for each hour
    QHash<int, int> frequencyMap;
    for (int hour = 6; hour <= 21; hour++) {
        frequencyMap[hour] = 0;
    }

    // Count the number of data points for each hour
    while (query.next()) {
        QDateTime checkOutTime = QDateTime::fromString(query.value(0).toString(), "MMMM dd,yyyy hh:mm:ss AP");
        int hourOfDay = checkOutTime.time().hour();
        if (frequencyMap.contains(hourOfDay)) {
            frequencyMap[hourOfDay]++;
        }
    }

    // Add the data points to the line series using the frequency map
    for (int hour = 6; hour <= 21; hour++) {
        series1->append(hour, frequencyMap[hour]);
    }

    // Create a chart and add the line series to it
    QChart *chart1 = new QChart();
    chart1->addSeries(series1);

    // Set the title and axes labels for the chart
    QFont titleFont;
    titleFont.setPointSize(18);
    titleFont.setWeight(QFont::Bold);
    chart1->setTitleFont(titleFont);
    chart1->setTitle("Peak CheckOut Hours");

    // Create a value axis for the x-axis
    QValueAxis *axisX = new QValueAxis();
    axisX->setTitleText("Time (hour)");
    axisX->setLabelFormat("%d");
    axisX->setTickCount(16);
    axisX->setRange(6, 21);
    chart1->addAxis(axisX, Qt::AlignBottom);
    series1->attachAxis(axisX);

    // Create a value axis for the y-axis
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("No. of Check-Outs");
    axisY->setLabelFormat("%d");
    int maxFrequency = *std::max_element(frequencyMap.constBegin(), frequencyMap.constEnd());
    axisY->setRange(0, maxFrequency);
    axisY->setTickCount(maxFrequency + 1); // Set the tick count for y-axis
    chart1->addAxis(axisY, Qt::AlignLeft);
    series1->attachAxis(axisY);

    // Create a chart view and set the chart as its content
    QChartView *chartView1 = new QChartView(chart1);
    chartView1->setRenderHint(QPainter::Antialiasing);

    // Set the widget_Bar_Chart as the chart view's parent
    chartView1->setParent(ui->widget_Bar_Chart);

    // Create a layout for the widget_Bar_Chart if it doesn't exist
    if (!ui->widget_Bar_Chart->layout()) {
        QVBoxLayout *layout = new QVBoxLayout(ui->widget_Bar_Chart);
        layout->setContentsMargins(0, 0, 0, 0);  // Set layout margins to 0
    }

    // Clear the layout and add the chart view to it
    QLayout *layout = ui->widget_Bar_Chart->layout();
    QLayoutItem *child;
    while ((child = layout->takeAt(0)) != nullptr) {
        delete child;
    }
    layout->addWidget(chartView1);

    // Show the chart view
    chartView1->show();


    connClose();
}


void Dashboard::createLineChart()
{
    // Close the previous chart view, if it exists
    if (chartView) {
        delete chartView;
    }

    connOpen();

    QLineSeries *series = new QLineSeries();

    // Retrieve data from the database
    QSqlQuery query;
    query.exec("SELECT CheckIn FROM Parking");

    // Initialize a frequency map to count the number of data points for each hour
    QHash<int, int> frequencyMap;
    for (int hour = 6; hour <= 21; hour++) {
        frequencyMap[hour] = 0;
    }

    // Count the number of data points for each hour
    while (query.next()) {
        QDateTime checkInTime = QDateTime::fromString(query.value(0).toString(), "MMMM dd,yyyy hh:mm:ss AP");
        int hourOfDay = checkInTime.time().hour();
        if (frequencyMap.contains(hourOfDay)) {
            frequencyMap[hourOfDay]++;
        }
    }

    // Add the data points to the line series using the frequency map
    for (int hour = 6; hour <= 21; hour++) {
        series->append(hour, frequencyMap[hour]);
    }

    // Create a chart and add the line series to it
    QChart *chart = new QChart();
    chart->addSeries(series);

    // Set the title and axes labels for the chart
    QFont title;
    title.setPointSize(18);
    title.setWeight(QFont::Bold);
    chart->setTitle("Peak Parking Hours");
    chart->setTitleFont(title);

    // Create a value axis for the x-axis
    QValueAxis *axisX = new QValueAxis();
    axisX->setTitleText("Time (hour)");
    axisX->setLabelFormat("%d");
    axisX->setTickCount(16);
    axisX->setRange(6, 21);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    // Create a value axis for the y-axis
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("No. of Check-Ins");
    axisY->setLabelFormat("%d");
    int maxFrequency = *std::max_element(frequencyMap.constBegin(), frequencyMap.constEnd());
    axisY->setRange(0, maxFrequency);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // Set the widget_Line_Chart as the chart view's parent
    chartView = new QChartView(chart);
    chartView->setParent(ui->widget_Line_Chart);

    // Create a layout for the widget_Line_Chart if it doesn't exist
    if (!ui->widget_Line_Chart->layout()) {
        QVBoxLayout *layout = new QVBoxLayout(ui->widget_Line_Chart);
        layout->setContentsMargins(0, 0, 0, 0);  // Set layout margins to 0
    }

    // Clear the layout and add the chart view to it
    QLayout *layout = ui->widget_Line_Chart->layout();
    QLayoutItem *child;
    while ((child = layout->takeAt(0)) != nullptr) {
        delete child;
    }
    layout->addWidget(chartView);

    // Show the chart view
    chartView->show();

    connClose();
}


void Dashboard::createPieChart()
{
    connOpen();

    // Create the pie series
    QPieSeries* series = new QPieSeries();

    // Query the database to retrieve the count of each vehicle type
    QSqlQuery query;
    if (query.exec("SELECT VehicleType, COUNT(*) FROM Parking GROUP BY VehicleType")) {
        while (query.next()) {
            QString vehicleType = query.value(0).toString();
            int count = query.value(1).toInt();
            series->append(vehicleType, count);
        }

        // Create the chart and set the pie series as its data source
        QChart* chart = new QChart();
        chart->addSeries(series);

        QFont title;
        title.setPointSize(18);
        title.setWeight(QFont::Bold);
        // Set the chart title
        chart->setTitle("Vehicle Types");
        chart->setTitleFont(title);

        // Create a color palette for the slices
        QStringList colorPalette;
        colorPalette << "#003f5c" << "#bc5090" << "#ffa600"; // Add more colors as needed

        // Assign colors from the palette to each pie slice
        int colorIndex = 0;
        for (QPieSlice* slice : series->slices()) {
            slice->setColor(QColor(colorPalette.at(colorIndex)));
            colorIndex = (colorIndex + 1) % colorPalette.size();

            // Set the label format for the slice to show the percentage value
            QString label = QString("%1 %2%").arg(slice->label()).arg(slice->percentage() * 100, 0, 'f', 1);
            slice->setLabel(label);
        }

        // Create a chart view and set the chart as its model
        QChartView* chartView = new QChartView(chart);
        chartView->setRenderHint(QPainter::Antialiasing);

        // Increase the size of the chart view
        //chartView->setFixedSize(600, 400); // Adjust the size as needed

        // Set the font size for the label
        QFont label;
        label.setPointSize(11); // Adjust the font size as needed
        label.setWeight(QFont::Bold);
        chart->legend()->setFont(label);

        // Clear the existing layout of the widget
        QLayout* existingLayout = ui->widget_Pie_Chart->layout();
        if (existingLayout != nullptr) {
            delete existingLayout;
        }

        // Create a new layout and add the chart view to it
        QVBoxLayout* layout = new QVBoxLayout(ui->widget_Pie_Chart);
        layout->addWidget(chartView);

        // Set the new layout for the widget
        ui->widget_Pie_Chart->setLayout(layout);
    } else {
        qDebug() << "Failed to execute query";
    }

    // Close the database connection
    connClose();
}



void Dashboard::searchTable(const QString &searchTable)
{
    connOpen();
    // Execute the filtered query
    QSqlQuery query;
    QString queryString = QString("SELECT CheckIn, CheckOut, Name, PhoneNumber, PlateNumber, VehicleType, ButtonName, ParkingAttendant FROM Parking WHERE Name LIKE '%%1%' OR PhoneNumber LIKE '%%1%' OR PlateNumber LIKE '%%1%' OR ButtonName LIKE '%%1%' OR ParkingAttendant LIKE '%%1%'").arg(searchTable);
    if (!query.exec(queryString)) {
        qWarning() << "Failed to execute SELECT query:" << query.lastError().text();
        return;
    }
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // Create a QSqlQueryModel to store and display the filtered query results
    QSqlQueryModel *model = new QSqlQueryModel(this);
    model->setQuery(query);

    // Set the model for the tableView widget
    ui->tableView->setModel(model);

    // Set the column headers
    model->setHeaderData(0, Qt::Horizontal, "Check In");
    model->setHeaderData(1, Qt::Horizontal, "Check Out");
    model->setHeaderData(2, Qt::Horizontal, "Name");
    model->setHeaderData(3, Qt::Horizontal, "Phone Number");
    model->setHeaderData(4, Qt::Horizontal, "Plate Number");
    model->setHeaderData(5, Qt::Horizontal, "Vehicle Type");
    model->setHeaderData(6, Qt::Horizontal, "Location");
    model->setHeaderData(7, Qt::Horizontal, "Parking Attendant");

    connClose();
}

void Dashboard::updateTable()
{
    connOpen();
    // Execute the SELECT query
    QSqlQuery query;
    if (!query.exec("SELECT CheckIn, CheckOut, Name, PhoneNumber, PlateNumber, VehicleType, ButtonName, ParkingAttendant FROM Parking")) {
        qWarning() << "Failed to execute SELECT query:" << query.lastError().text();
        return;
    }

    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Create a QSqlQueryModel to store and display the query results
    QSqlQueryModel *model = new QSqlQueryModel(this);
    model->setQuery(query);

    // Set the model for the tableView_transaction widget
    ui->tableView->setModel(model);

    // Set the column headers
    model->setHeaderData(0, Qt::Horizontal, "Check In");
    model->setHeaderData(1, Qt::Horizontal, "Check Out");
    model->setHeaderData(2, Qt::Horizontal, "Name");
    model->setHeaderData(3, Qt::Horizontal, "Phone Number");
    model->setHeaderData(4, Qt::Horizontal, "Plate Number");
    model->setHeaderData(5, Qt::Horizontal, "Vehicle Type");
    model->setHeaderData(6, Qt::Horizontal, "Location");
    model->setHeaderData(7, Qt::Horizontal, "Parking Attendant");

    ui->tableView->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: rgb(58, 79, 80); }");
    ui->tableView->verticalHeader()->setStyleSheet("QHeaderView::section { background-color: rgb(58, 79, 80); }");
    connClose();
}

void Dashboard::updateButtonStylesheet()
{
    connOpen();

    // List of button names to check
    QStringList buttonNames = {"Canteen_01", "Canteen_03", "Canteen_04", "Canteen_05",
        "Canteen_06","UAC_01","UAC_02","UAC_03","UAC_04","UAC_05","UAC_06","UAC_07",
        "Tanghalang_Bayan_01","Tanghalang_Bayan_02","Tanghalang_Bayan_03","Tanghalang_Bayan_04","Tanghalang_Bayan_05",
        "Tanghalang_Bayan_06","Tanghalang_Bayan_07","Tanghalang_Bayan_08","Tanghalang_Bayan_09","Tanghalang_Bayan_10",
        "Tanghalang_Bayan_11","Tanghalang_Bayan_12","Tanghalang_Bayan_13","Tanghalang_Bayan_14","Tanghalang_Bayan_15",
        "Tanghalang_Bayan_16","Tanghalang_Bayan_17","Tanghalang_Bayan_18","Tanghalang_Bayan_19","Tanghalang_Bayan_20",
        "Tanghalang_Bayan_21","Tanghalang_Bayan_22","Tanghalang_Bayan_23","Tanghalang_Bayan_24","Tanghalang_Bayan_25",
        "Tanghalang_Bayan_26","Tanghalang_Bayan_27","Tanghalang_Bayan_28","Tanghalang_Bayan_29","Tanghalang_Bayan_30",
        "Tanghalang_Bayan_31","Tanghalang_Bayan_32","Tanghalang_Bayan_33","Tanghalang_Bayan_34","Tanghalang_Bayan_35",
        "Tanghalang_Bayan_36","Tanghalang_Bayan_37","Tanghalang_Bayan_38","Entrance_01","Entrance_02","Entrance_03","Entrance_04","Entrance_05","Entrance_06","Entrance_07",
        "Entrance_08","Entrance_09","Entrance_10","Entrance_11","Entrance_12","Entrance_13","Entrance_14",
        "Entrance_15","Entrance_16","Entrance_17","Entrance_18","Entrance_19","Entrance_20","Entrance_21",
        "Entrance_22","Entrance_23","Entrance_24","Entrance_25","Entrance_26","Entrance_27","Entrance_28",
        "Entrance_29","Entrance_30","Entrance_31","Entrance_32","Entrance_33","Entrance_34","Entrance_35",
                                   "Entrance_36","Entrance_37","Entrance_38","Entrance_39","Entrance_40","Entrance_41","Entrance_42"};
    // Loop through each button name
    foreach (const QString& buttonName, buttonNames) {
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName");
        query.bindValue(":buttonName", buttonName);

        if (query.exec() && query.next()) {
            // Row with matching ButtonName found
            setButtonOccupiedStyleSheet(buttonName);
        } else {
            // No row with matching ButtonName found
            setButtonAvailableStyleSheet(buttonName);
        }
    }

    connClose();

    QString buttonStyleSheetCanteen01 = ui->pushButton_Canteen_01->styleSheet();
    if (buttonStyleSheetCanteen01.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Canteen_001->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetCanteen01.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Canteen_001->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetCanteen01.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Canteen_001->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetCanteen02 = ui->pushButton_Canteen_02->styleSheet();
    if (buttonStyleSheetCanteen02.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Canteen_002->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetCanteen02.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Canteen_002->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetCanteen02.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Canteen_002->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetCanteen03 = ui->pushButton_Canteen_03->styleSheet();
    if (buttonStyleSheetCanteen03.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Canteen_003->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetCanteen03.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Canteen_003->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetCanteen03.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Canteen_003->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetCanteen04 = ui->pushButton_Canteen_04->styleSheet();
    if (buttonStyleSheetCanteen04.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Canteen_004->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetCanteen04.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Canteen_004->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetCanteen04.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Canteen_004->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetCanteen05 = ui->pushButton_Canteen_05->styleSheet();
    if (buttonStyleSheetCanteen05.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Canteen_005->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetCanteen05.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Canteen_005->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetCanteen05.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Canteen_005->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetCanteen06 = ui->pushButton_Canteen_06->styleSheet();
    if (buttonStyleSheetCanteen06.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Canteen_006->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetCanteen06.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Canteen_006->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetCanteen06.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Canteen_006->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetCanteen07 = ui->pushButton_Canteen_07->styleSheet();
    if (buttonStyleSheetCanteen07.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Canteen_007->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetCanteen07.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Canteen_007->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetCanteen07.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Canteen_007->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetCanteen08 = ui->pushButton_Canteen_08->styleSheet();
    if (buttonStyleSheetCanteen08.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Canteen_008->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetCanteen08.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Canteen_008->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetCanteen08.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Canteen_008->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetUAC01 = ui->pushButton_UAC_01->styleSheet();
    if (buttonStyleSheetUAC01.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_UAC_001->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetUAC01.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_UAC_001->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetUAC01.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_UAC_001->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetUAC02 = ui->pushButton_UAC_02->styleSheet();
    if (buttonStyleSheetUAC02.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_UAC_002->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetUAC02.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_UAC_002->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetUAC02.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_UAC_002->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetUAC03 = ui->pushButton_UAC_03->styleSheet();
    if (buttonStyleSheetUAC03.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_UAC_003->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetUAC03.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_UAC_003->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetUAC03.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_UAC_003->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetUAC04 = ui->pushButton_UAC_04->styleSheet();
    if (buttonStyleSheetUAC04.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_UAC_004->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetUAC04.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_UAC_004->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetUAC04.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_UAC_004->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetUAC05 = ui->pushButton_UAC_05->styleSheet();
    if (buttonStyleSheetUAC05.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_UAC_005->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetUAC05.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_UAC_005->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetUAC05.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_UAC_005->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetUAC06 = ui->pushButton_UAC_06->styleSheet();
    if (buttonStyleSheetUAC06.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_UAC_006->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetUAC06.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_UAC_006->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetUAC06.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_UAC_006->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetUAC07 = ui->pushButton_UAC_07->styleSheet();
    if (buttonStyleSheetUAC07.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_UAC_007->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetUAC07.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_UAC_007->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetUAC07.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_UAC_007->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan01 = ui->pushButton_Tanghalang_Bayan_01->styleSheet();
    if (buttonStyleSheetTanghalangBayan01.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_001->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan01.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_001->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan01.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_001->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan02 = ui->pushButton_Tanghalang_Bayan_02->styleSheet();
    if (buttonStyleSheetTanghalangBayan02.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_002->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan02.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_002->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan02.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_002->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan03 = ui->pushButton_Tanghalang_Bayan_03->styleSheet();
    if (buttonStyleSheetTanghalangBayan03.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_003->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan03.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_003->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan03.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_003->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan04 = ui->pushButton_Tanghalang_Bayan_04->styleSheet();
    if (buttonStyleSheetTanghalangBayan04.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_004->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan04.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_004->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan04.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_004->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan05 = ui->pushButton_Tanghalang_Bayan_05->styleSheet();
    if (buttonStyleSheetTanghalangBayan05.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_005->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan05.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_005->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan05.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_005->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan06 = ui->pushButton_Tanghalang_Bayan_06->styleSheet();
    if (buttonStyleSheetTanghalangBayan06.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_006->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan06.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_006->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan06.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_006->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan07 = ui->pushButton_Tanghalang_Bayan_07->styleSheet();
    if (buttonStyleSheetTanghalangBayan07.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_007->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan07.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_007->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan07.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_007->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan08 = ui->pushButton_Tanghalang_Bayan_08->styleSheet();
    if (buttonStyleSheetTanghalangBayan08.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_008->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan08.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_008->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan08.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_008->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan09 = ui->pushButton_Tanghalang_Bayan_09->styleSheet();
    if (buttonStyleSheetTanghalangBayan09.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_009->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan09.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_009->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan09.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_009->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan10 = ui->pushButton_Tanghalang_Bayan_10->styleSheet();
    if (buttonStyleSheetTanghalangBayan10.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_010->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan10.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_010->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan10.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_010->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan11 = ui->pushButton_Tanghalang_Bayan_11->styleSheet();
    if (buttonStyleSheetTanghalangBayan11.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_011->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan11.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_011->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan11.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_011->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan12 = ui->pushButton_Tanghalang_Bayan_12->styleSheet();
    if (buttonStyleSheetTanghalangBayan12.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_012->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan02.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_012->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan12.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_012->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan13 = ui->pushButton_Tanghalang_Bayan_13->styleSheet();
    if (buttonStyleSheetTanghalangBayan13.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_013->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan13.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_013->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan13.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_013->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan14 = ui->pushButton_Tanghalang_Bayan_14->styleSheet();
    if (buttonStyleSheetTanghalangBayan14.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_014->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan14.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_014->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan14.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_014->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan15 = ui->pushButton_Tanghalang_Bayan_15->styleSheet();
    if (buttonStyleSheetTanghalangBayan15.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_015->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan15.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_015->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan15.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_015->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan16 = ui->pushButton_Tanghalang_Bayan_16->styleSheet();
    if (buttonStyleSheetTanghalangBayan16.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_016->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan16.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_016->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan16.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_016->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan17 = ui->pushButton_Tanghalang_Bayan_17->styleSheet();
    if (buttonStyleSheetTanghalangBayan17.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_017->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan17.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_017->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan17.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_017->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan18 = ui->pushButton_Tanghalang_Bayan_18->styleSheet();
    if (buttonStyleSheetTanghalangBayan18.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_018->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan18.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_018->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan18.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_018->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan19 = ui->pushButton_Tanghalang_Bayan_19->styleSheet();
    if (buttonStyleSheetTanghalangBayan19.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_019->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan19.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_019->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan19.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_019->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan20 = ui->pushButton_Tanghalang_Bayan_20->styleSheet();
    if (buttonStyleSheetTanghalangBayan20.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_020->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan20.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_020->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan20.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_020->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan21 = ui->pushButton_Tanghalang_Bayan_21->styleSheet();
    if (buttonStyleSheetTanghalangBayan21.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_021->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan21.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_021->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan21.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_021->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan22 = ui->pushButton_Tanghalang_Bayan_22->styleSheet();
    if (buttonStyleSheetTanghalangBayan22.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_022->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan22.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_022->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan22.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_022->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan23 = ui->pushButton_Tanghalang_Bayan_23->styleSheet();
    if (buttonStyleSheetTanghalangBayan23.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_023->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan23.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_023->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan23.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_023->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan24 = ui->pushButton_Tanghalang_Bayan_24->styleSheet();
    if (buttonStyleSheetTanghalangBayan24.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_024->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan24.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_024->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan24.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_024->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan25 = ui->pushButton_Tanghalang_Bayan_25->styleSheet();
    if (buttonStyleSheetTanghalangBayan25.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_025->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan25.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_025->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan25.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_025->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan26 = ui->pushButton_Tanghalang_Bayan_26->styleSheet();
    if (buttonStyleSheetTanghalangBayan26.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_026->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan26.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_026->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan26.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_026->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan27 = ui->pushButton_Tanghalang_Bayan_27->styleSheet();
    if (buttonStyleSheetTanghalangBayan27.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_027->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan27.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_027->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan27.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_027->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan28 = ui->pushButton_Tanghalang_Bayan_28->styleSheet();
    if (buttonStyleSheetTanghalangBayan28.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_028->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan28.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_028->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan08.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_028->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan29 = ui->pushButton_Tanghalang_Bayan_29->styleSheet();
    if (buttonStyleSheetTanghalangBayan29.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_029->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan29.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_029->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan29.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_029->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan30 = ui->pushButton_Tanghalang_Bayan_30->styleSheet();
    if (buttonStyleSheetTanghalangBayan30.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_030->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan30.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_030->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan30.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_030->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan31 = ui->pushButton_Tanghalang_Bayan_31->styleSheet();
    if (buttonStyleSheetTanghalangBayan31.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_031->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan31.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_031->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan31.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_031->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan32 = ui->pushButton_Tanghalang_Bayan_32->styleSheet();
    if (buttonStyleSheetTanghalangBayan32.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_032->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan32.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_032->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan32.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_032->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan33 = ui->pushButton_Tanghalang_Bayan_33->styleSheet();
    if (buttonStyleSheetTanghalangBayan33.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_033->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan33.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_033->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan33.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_033->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan34 = ui->pushButton_Tanghalang_Bayan_34->styleSheet();
    if (buttonStyleSheetTanghalangBayan34.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_034->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan34.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_034->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan34.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_034->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan35 = ui->pushButton_Tanghalang_Bayan_35->styleSheet();
    if (buttonStyleSheetTanghalangBayan35.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_035->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan35.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_035->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan35.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_035->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan36 = ui->pushButton_Tanghalang_Bayan_36->styleSheet();
    if (buttonStyleSheetTanghalangBayan36.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_036->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan36.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_036->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan36.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_036->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan37 = ui->pushButton_Tanghalang_Bayan_37->styleSheet();
    if (buttonStyleSheetTanghalangBayan37.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_037->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan37.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_037->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan37.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_037->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan38 = ui->pushButton_Tanghalang_Bayan_38->styleSheet();
    if (buttonStyleSheetTanghalangBayan38.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_038->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan38.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_038->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan38.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_038->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan39 = ui->pushButton_Tanghalang_Bayan_39->styleSheet();
    if (buttonStyleSheetTanghalangBayan39.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_039->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan39.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_039->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan39.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_039->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan40 = ui->pushButton_Tanghalang_Bayan_40->styleSheet();
    if (buttonStyleSheetTanghalangBayan40.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_040->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan40.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_040->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan40.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_040->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetTanghalangBayan41 = ui->pushButton_57pushButton_Tanghalang_Bayan_41->styleSheet();
    if (buttonStyleSheetTanghalangBayan41.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Tanghalang_Bayan_041->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan41.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Tanghalang_Bayan_041->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetTanghalangBayan41.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Tanghalang_Bayan_041->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance01 = ui->pushButton_Entrance_01->styleSheet();
    if (buttonStyleSheetEntrance01.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_001->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance01.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_001->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance01.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_001->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance02 = ui->pushButton_Entrance_02->styleSheet();
    if (buttonStyleSheetEntrance02.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_002->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance01.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_002->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance01.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_002->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance03 = ui->pushButton_Entrance_03->styleSheet();
    if (buttonStyleSheetEntrance03.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_003->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance03.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_003->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance03.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_003->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance04 = ui->pushButton_Entrance_04->styleSheet();
    if (buttonStyleSheetEntrance04.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_004->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance04.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_004->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance04.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_004->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance05 = ui->pushButton_Entrance_05->styleSheet();
    if (buttonStyleSheetEntrance05.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_005->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance05.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_005->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance05.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_005->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance06 = ui->pushButton_Entrance_06->styleSheet();
    if (buttonStyleSheetEntrance06.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_006->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance06.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_006->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance06.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_006->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance07 = ui->pushButton_Entrance_07->styleSheet();
    if (buttonStyleSheetEntrance07.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_007->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance07.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_007->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance07.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_007->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance08 = ui->pushButton_Entrance_08->styleSheet();
    if (buttonStyleSheetEntrance01.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_008->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance08.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_008->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance08.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_008->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance09 = ui->pushButton_Entrance_09->styleSheet();
    if (buttonStyleSheetEntrance09.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_009->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance09.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_009->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance09.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_009->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance10 = ui->pushButton_Entrance_10->styleSheet();
    if (buttonStyleSheetEntrance10.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_010->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance10.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_010->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance10.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_010->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance11 = ui->pushButton_Entrance_11->styleSheet();
    if (buttonStyleSheetEntrance11.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_011->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance11.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_011->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance11.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_011->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance12 = ui->pushButton_Entrance_12->styleSheet();
    if (buttonStyleSheetEntrance12.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_012->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance11.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_012->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance11.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_012->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance13 = ui->pushButton_Entrance_13->styleSheet();
    if (buttonStyleSheetEntrance13.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_013->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance13.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_013->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance13.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_013->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance14 = ui->pushButton_Entrance_14->styleSheet();
    if (buttonStyleSheetEntrance14.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_014->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance14.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_014->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance14.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_014->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance15 = ui->pushButton_Entrance_15->styleSheet();
    if (buttonStyleSheetEntrance15.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_015->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance15.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_015->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance15.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_015->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance16 = ui->pushButton_Entrance_16->styleSheet();
    if (buttonStyleSheetEntrance16.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_016->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance16.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_016->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance16.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_016->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance17 = ui->pushButton_Entrance_17->styleSheet();
    if (buttonStyleSheetEntrance17.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_017->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance17.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_017->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance17.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_017->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance18 = ui->pushButton_Entrance_18->styleSheet();
    if (buttonStyleSheetEntrance18.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_018->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance18.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_018->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance18.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_018->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance19 = ui->pushButton_Entrance_19->styleSheet();
    if (buttonStyleSheetEntrance19.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_019->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance19.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_019->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance19.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_019->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance20 = ui->pushButton_Entrance_20->styleSheet();
    if (buttonStyleSheetEntrance20.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_020->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance20.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_020->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance20.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_020->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance21 = ui->pushButton_Entrance_21->styleSheet();
    if (buttonStyleSheetEntrance21.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_021->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance21.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_021->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance21.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_021->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance22 = ui->pushButton_Entrance_22->styleSheet();
    if (buttonStyleSheetEntrance22.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_022->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance21.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_022->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance21.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_022->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance23 = ui->pushButton_Entrance_23->styleSheet();
    if (buttonStyleSheetEntrance23.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_023->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance23.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_023->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance23.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_023->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance24 = ui->pushButton_Entrance_24->styleSheet();
    if (buttonStyleSheetEntrance24.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_024->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance24.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_024->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance24.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_024->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance25 = ui->pushButton_Entrance_25->styleSheet();
    if (buttonStyleSheetEntrance25.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_025->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance25.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_025->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance25.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_025->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance26 = ui->pushButton_Entrance_26->styleSheet();
    if (buttonStyleSheetEntrance26.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_026->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance26.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_026->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance26.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_026->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance27 = ui->pushButton_Entrance_27->styleSheet();
    if (buttonStyleSheetEntrance27.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_027->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance27.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_027->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance27.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_027->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance28 = ui->pushButton_Entrance_28->styleSheet();
    if (buttonStyleSheetEntrance21.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_028->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance28.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_028->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance28.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_028->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance29 = ui->pushButton_Entrance_29->styleSheet();
    if (buttonStyleSheetEntrance29.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_029->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance29.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_029->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance29.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_029->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance30 = ui->pushButton_Entrance_30->styleSheet();
    if (buttonStyleSheetEntrance30.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_030->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance30.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_030->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance30.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_030->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance31 = ui->pushButton_Entrance_31->styleSheet();
    if (buttonStyleSheetEntrance31.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_031->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance31.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_031->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance31.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_031->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance32 = ui->pushButton_Entrance_32->styleSheet();
    if (buttonStyleSheetEntrance32.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_032->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance32.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_032->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance32.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_032->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance33 = ui->pushButton_Entrance_33->styleSheet();
    if (buttonStyleSheetEntrance33.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_033->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance33.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_033->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance33.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_033->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance34 = ui->pushButton_Entrance_34->styleSheet();
    if (buttonStyleSheetEntrance34.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_034->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance34.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_034->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance34.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_034->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance35 = ui->pushButton_Entrance_35->styleSheet();
    if (buttonStyleSheetEntrance35.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_035->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance35.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_035->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance35.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_035->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance36 = ui->pushButton_Entrance_36->styleSheet();
    if (buttonStyleSheetEntrance36.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_036->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance36.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_036->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance16.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_036->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance37 = ui->pushButton_Entrance_37->styleSheet();
    if (buttonStyleSheetEntrance37.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_037->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance37.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_037->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance37.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_037->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance38 = ui->pushButton_Entrance_38->styleSheet();
    if (buttonStyleSheetEntrance38.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_038->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance38.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_038->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance38.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_038->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance39 = ui->pushButton_Entrance_39->styleSheet();
    if (buttonStyleSheetEntrance39.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_039->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance39.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_039->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance39.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_039->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance40 = ui->pushButton_Entrance_40->styleSheet();
    if (buttonStyleSheetEntrance40.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_040->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance40.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_040->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance40.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_040->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance41 = ui->pushButton_Entrance_41->styleSheet();
    if (buttonStyleSheetEntrance41.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_041->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance41.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_041->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance41.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_041->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }

    QString buttonStyleSheetEntrance42 = ui->pushButton_Entrance_42->styleSheet();
    if (buttonStyleSheetEntrance42.contains("image: url(:/assets/icons/available 2.png);")){
        ui->pushButton_Entrance_042->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance42.contains("image: url(:/assets/icons/occupied 1.png);")){
        ui->pushButton_Entrance_042->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/occupied 1.png); border-radius: 8px;");
    }else if (buttonStyleSheetEntrance42.contains("image: url(:/assets/icons/reserve.png);")){
        ui->pushButton_Entrance_042->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/reserve.png); border-radius: 8px;");
    }




}

void Dashboard::setButtonOccupiedStyleSheet(const QString& buttonName)
{
    QPushButton* button = getButtonByName(buttonName);
    if (button) {
        button->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
    }
}

void Dashboard::setButtonAvailableStyleSheet(const QString& buttonName)
{
    QPushButton* button = getButtonByName(buttonName);
    if (button) {
        button->setStyleSheet("background-color: rgb(244, 243, 253); image: url(:/assets/icons/available 2.png); border-radius: 8px;");
    }
}

QPushButton* Dashboard::getButtonByName(const QString& buttonName)
{
    if (buttonName == "Canteen_01") {
        return ui->pushButton_Canteen_01;
    } else if (buttonName == "Canteen_02") {
        return ui->pushButton_Canteen_02;
    } else if (buttonName == "Canteen_03") {
        return ui->pushButton_Canteen_03;
    }else if (buttonName == "Canteen_04") {
        return ui->pushButton_Canteen_04;
    }else if (buttonName == "Canteen_05") {
        return ui->pushButton_Canteen_05;
    }else if (buttonName == "Canteen_06") {
        return ui->pushButton_Canteen_06;
    }else if (buttonName == "Canteen_07") {
        return ui->pushButton_Canteen_07;
    }else if (buttonName == "Canteen_08") {
        return ui->pushButton_Canteen_08;
    }else if (buttonName == "UAC_01") {
        return ui->pushButton_UAC_01;
    }else if (buttonName == "UAC_02") {
        return ui->pushButton_UAC_02;
    }else if (buttonName == "UAC_03") {
        return ui->pushButton_UAC_03;
    }else if (buttonName == "UAC_04") {
        return ui->pushButton_UAC_04;
    }else if (buttonName == "UAC_05") {
        return ui->pushButton_UAC_05;
    }else if (buttonName == "UAC_06") {
        return ui->pushButton_UAC_06;
    }else if (buttonName == "UAC_07") {
        return ui->pushButton_UAC_07;
    }else if (buttonName == "Tanghalang_Bayan_01") {
        return ui->pushButton_Tanghalang_Bayan_01;
    }else if (buttonName == "Tanghalang_Bayan_02") {
        return ui->pushButton_Tanghalang_Bayan_02;
    }else if (buttonName == "Tanghalang_Bayan_03") {
        return ui->pushButton_Tanghalang_Bayan_03;
    }else if (buttonName == "Tanghalang_Bayan_04") {
        return ui->pushButton_Tanghalang_Bayan_04;
    }else if (buttonName == "Tanghalang_Bayan_05") {
        return ui->pushButton_Tanghalang_Bayan_05;
    }else if (buttonName == "Tanghalang_Bayan_06") {
        return ui->pushButton_Tanghalang_Bayan_06;
    }else if (buttonName == "Tanghalang_Bayan_07") {
        return ui->pushButton_Tanghalang_Bayan_07;
    }else if (buttonName == "Tanghalang_Bayan_08") {
        return ui->pushButton_Tanghalang_Bayan_08;
    }else if (buttonName == "Tanghalang_Bayan_09") {
        return ui->pushButton_Tanghalang_Bayan_09;
    }else if (buttonName == "Tanghalang_Bayan_10") {
        return ui->pushButton_Tanghalang_Bayan_10;
    }else if (buttonName == "Tanghalang_Bayan_11") {
        return ui->pushButton_Tanghalang_Bayan_11;
    }else if (buttonName == "Tanghalang_Bayan_12") {
        return ui->pushButton_Tanghalang_Bayan_12;
    }else if (buttonName == "Tanghalang_Bayan_13") {
        return ui->pushButton_Tanghalang_Bayan_13;
    }else if (buttonName == "Tanghalang_Bayan_14") {
        return ui->pushButton_Tanghalang_Bayan_14;
    }else if (buttonName == "Tanghalang_Bayan_15") {
        return ui->pushButton_Tanghalang_Bayan_15;
    }else if (buttonName == "Tanghalang_Bayan_16") {
        return ui->pushButton_Tanghalang_Bayan_16;
    }else if (buttonName == "Tanghalang_Bayan_17") {
        return ui->pushButton_Tanghalang_Bayan_17;
    }else if (buttonName == "Tanghalang_Bayan_18") {
        return ui->pushButton_Tanghalang_Bayan_18;
    }else if (buttonName == "Tanghalang_Bayan_19") {
        return ui->pushButton_Tanghalang_Bayan_19;
    }else if (buttonName == "Tanghalang_Bayan_20") {
        return ui->pushButton_Tanghalang_Bayan_20;
    }else if (buttonName == "Tanghalang_Bayan_21") {
        return ui->pushButton_Tanghalang_Bayan_21;
    }else if (buttonName == "Tanghalang_Bayan_22") {
        return ui->pushButton_Tanghalang_Bayan_22;
    }else if (buttonName == "Tanghalang_Bayan_23") {
        return ui->pushButton_Tanghalang_Bayan_23;
    }else if (buttonName == "Tanghalang_Bayan_24") {
        return ui->pushButton_Tanghalang_Bayan_24;
    }else if (buttonName == "Tanghalang_Bayan_25") {
        return ui->pushButton_Tanghalang_Bayan_25;
    }else if (buttonName == "Tanghalang_Bayan_26") {
        return ui->pushButton_Tanghalang_Bayan_26;
    }else if (buttonName == "Tanghalang_Bayan_27") {
        return ui->pushButton_Tanghalang_Bayan_27;
    }else if (buttonName == "Tanghalang_Bayan_28") {
        return ui->pushButton_Tanghalang_Bayan_28;
    }else if (buttonName == "Tanghalang_Bayan_29") {
        return ui->pushButton_Tanghalang_Bayan_29;
    }else if (buttonName == "Tanghalang_Bayan_30") {
        return ui->pushButton_Tanghalang_Bayan_30;
    }else if (buttonName == "Tanghalang_Bayan_31") {
        return ui->pushButton_Tanghalang_Bayan_31;
    }else if (buttonName == "Tanghalang_Bayan_32") {
        return ui->pushButton_Tanghalang_Bayan_32;
    }else if (buttonName == "Tanghalang_Bayan_33") {
        return ui->pushButton_Tanghalang_Bayan_33;
    }else if (buttonName == "Tanghalang_Bayan_34") {
        return ui->pushButton_Tanghalang_Bayan_34;
    }else if (buttonName == "Tanghalang_Bayan_35") {
        return ui->pushButton_Tanghalang_Bayan_35;
    }else if (buttonName == "Tanghalang_Bayan_36") {
        return ui->pushButton_Tanghalang_Bayan_36;
    }else if (buttonName == "Tanghalang_Bayan_37") {
        return ui->pushButton_Tanghalang_Bayan_37;
    }else if (buttonName == "Tanghalang_Bayan_38") {
        return ui->pushButton_Tanghalang_Bayan_38;
    }else if (buttonName == "Tanghalang_Bayan_39") {
        return ui->pushButton_Tanghalang_Bayan_39;
    }else if (buttonName == "Tanghalang_Bayan_40") {
        return ui->pushButton_Tanghalang_Bayan_40;
    }else if (buttonName == "Tanghalang_Bayan_41") {
        return ui->pushButton_57pushButton_Tanghalang_Bayan_41;
    }else if (buttonName == "Entrance_01") {
        return ui->pushButton_Entrance_01;
    }else if (buttonName == "Entrance_02") {
        return ui->pushButton_Entrance_02;
    }else if (buttonName == "Entrance_03") {
        return ui->pushButton_Entrance_03;
    }else if (buttonName == "Entrance_04") {
        return ui->pushButton_Entrance_04;
    }else if (buttonName == "Entrance_05") {
        return ui->pushButton_Entrance_05;
    }else if (buttonName == "Entrance_06") {
        return ui->pushButton_Entrance_06;
    }else if (buttonName == "Entrance_07") {
        return ui->pushButton_Entrance_07;
    }else if (buttonName == "Entrance_08") {
        return ui->pushButton_Entrance_08;
    }else if (buttonName == "Entrance_09") {
        return ui->pushButton_Entrance_09;
    }else if (buttonName == "Entrance_10") {
        return ui->pushButton_Entrance_10;
    }else if (buttonName == "Entrance_11") {
        return ui->pushButton_Entrance_11;
    }else if (buttonName == "Entrance_12") {
        return ui->pushButton_Entrance_12;
    }else if (buttonName == "Entrance_13") {
        return ui->pushButton_Entrance_13;
    }else if (buttonName == "Entrance_14") {
        return ui->pushButton_Entrance_14;
    }else if (buttonName == "Entrance_15") {
        return ui->pushButton_Entrance_15;
    }else if (buttonName == "Entrance_16") {
        return ui->pushButton_Entrance_16;
    }else if (buttonName == "Entrance_17") {
        return ui->pushButton_Entrance_17;
    }else if (buttonName == "Entrance_18") {
        return ui->pushButton_Entrance_18;
    }else if (buttonName == "Entrance_19") {
        return ui->pushButton_Entrance_19;
    }else if (buttonName == "Entrance_20") {
        return ui->pushButton_Entrance_20;
    }else if (buttonName == "Entrance_21") {
        return ui->pushButton_Entrance_21;
    }else if (buttonName == "Entrance_22") {
        return ui->pushButton_Entrance_22;
    }else if (buttonName == "Entrance_23") {
        return ui->pushButton_Entrance_23;
    }else if (buttonName == "Entrance_24") {
        return ui->pushButton_Entrance_24;
    }else if (buttonName == "Entrance_25") {
        return ui->pushButton_Entrance_25;
    }else if (buttonName == "Entrance_26") {
        return ui->pushButton_Entrance_26;
    }else if (buttonName == "Entrance_27") {
        return ui->pushButton_Entrance_27;
    }else if (buttonName == "Entrance_28") {
        return ui->pushButton_Entrance_28;
    }else if (buttonName == "Entrance_29") {
        return ui->pushButton_Entrance_29;
    }else if (buttonName == "Entrance_30") {
        return ui->pushButton_Entrance_30;
    }else if (buttonName == "Entrance_31") {
        return ui->pushButton_Entrance_31;
    }else if (buttonName == "Entrance_32") {
        return ui->pushButton_Entrance_32;
    }else if (buttonName == "Entrance_33") {
        return ui->pushButton_Entrance_33;
    }else if (buttonName == "Entrance_34") {
        return ui->pushButton_Entrance_34;
    }else if (buttonName == "Entrance_35") {
        return ui->pushButton_Entrance_35;
    }else if (buttonName == "Entrance_36") {
        return ui->pushButton_Entrance_36;
    }else if (buttonName == "Entrance_37") {
        return ui->pushButton_Entrance_37;
    }else if (buttonName == "Entrance_38") {
        return ui->pushButton_Entrance_38;
    }else if (buttonName == "Entrance_39") {
        return ui->pushButton_Entrance_39;
    }else if (buttonName == "Entrance_40") {
        return ui->pushButton_Entrance_40;
    }else if (buttonName == "Entrance_41") {
        return ui->pushButton_Entrance_41;
    }else if (buttonName == "Entrance_42") {
        return ui->pushButton_Entrance_42;
    }
    // Add more buttonName-button mapping here if needed

    return nullptr; // Button not found
}

void Dashboard::updateDateTimeLabel()
{
    QDateTime sqlCurrentDateTime = QDateTime::currentDateTime();
    sqlDateTimeText = sqlCurrentDateTime.toString("MMMM dd,yyyy hh:mm:ss AP");

    QDateTime CurrentDateTime = QDateTime::currentDateTime();
    DateTimeText = CurrentDateTime.toString("MMMM dd,yyyy hh:mm AP");
    // You can use a different format as per your requirement
    ui->label_Date->setText(DateTimeText);
}


void Dashboard::on_pushButton_Home_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
    ui->tabWidget->setCurrentIndex(0);
    updateButtonStylesheet();
}


void Dashboard::on_pushButton_Parking_clicked()
{
   ui->tabWidget->setCurrentIndex(1);
   updateButtonStylesheet();
   QString tableSearch = ui->lineEdit_Table_Search->text();
   if (tableSearch.isEmpty()){
        updateTable();
   }else{
        searchTable(tableSearch);
   }

}

void Dashboard::on_pushButton_Reports_clicked()
{
    ui->tabWidget->setCurrentIndex(2);
    updateButtonStylesheet();
    createPieChart();
    createLineChart();
    createIntervalChart();

}


void Dashboard::on_pushButton_Logout_clicked()
{
    logoutfunc();
}


void Dashboard::on_pushButton_Uac_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    updateButtonStylesheet();
}


void Dashboard::on_pushButton_Tanghalang_Bayan_clicked()
{
    ui->stackedWidget->setCurrentIndex(2);
   updateButtonStylesheet();
}


void Dashboard::on_pushButton_Canteen_clicked()
{
    ui->stackedWidget->setCurrentIndex(3);
   updateButtonStylesheet();
}


void Dashboard::on_pushButton_Entrance_clicked()
{
    ui->stackedWidget->setCurrentIndex(4);
    updateButtonStylesheet();
}


void Dashboard::on_pushButton_Exit_1_clicked()
{
    closefunc();
}


void Dashboard::on_pushButton_Exit_2_clicked()
{
    closefunc();
}



void Dashboard::on_pushButton_Home_1_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
    ui->tabWidget->setCurrentIndex(0);
    updateButtonStylesheet();
}


void Dashboard::on_pushButton_Parking_1_clicked()
{
    ui->tabWidget->setCurrentIndex(1);
    updateButtonStylesheet();
    updateTable();
}


void Dashboard::on_pushButton_Reports_1_clicked()
{
    ui->tabWidget->setCurrentIndex(2);
    updateButtonStylesheet();
    createPieChart();
    createLineChart();
    createIntervalChart();
}


void Dashboard::on_pushButton_Home_2_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
    ui->tabWidget->setCurrentIndex(0);
    updateButtonStylesheet();
}


void Dashboard::on_pushButton_Parking_2_clicked()
{
    ui->tabWidget->setCurrentIndex(1);
    updateButtonStylesheet();
    updateTable();
}


void Dashboard::on_pushButton_Reports_2_clicked()
{
    ui->tabWidget->setCurrentIndex(2);
    updateButtonStylesheet();
    createPieChart();
    createLineChart();
    createIntervalChart();
}


void Dashboard::on_pushButton_Canteen_01_clicked()
{
    QString buttonName = "Canteen_01";
    QString buttonStyleSheet = ui->pushButton_Canteen_01->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Canteen_01->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Canteen_02_clicked()
{
    QString buttonName = "Canteen_02";
    QString buttonStyleSheet = ui->pushButton_Canteen_02->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Canteen_02->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}

void Dashboard::on_pushButton_Canteen_03_clicked()
{
    QString buttonName = "Canteen_03";
    QString buttonStyleSheet = ui->pushButton_Canteen_03->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Canteen_03->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}

void Dashboard::on_pushButton_Canteen_04_clicked()
{
    QString buttonName = "Canteen_04";
    QString buttonStyleSheet = ui->pushButton_Canteen_04->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Canteen_04->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Canteen_05_clicked()
{
    QString buttonName = "Canteen_05";
    QString buttonStyleSheet = ui->pushButton_Canteen_05->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Canteen_05->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Canteen_06_clicked()
{
    QString buttonName = "Canteen_06";
    QString buttonStyleSheet = ui->pushButton_Canteen_06->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Canteen_06->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Canteen_07_clicked()
{
    QString buttonName = "Canteen_07";
    QString buttonStyleSheet = ui->pushButton_Canteen_07->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Canteen_07->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Canteen_08_clicked()
{
    QString buttonName = "Canteen_08";
    QString buttonStyleSheet = ui->pushButton_Canteen_08->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Canteen_08->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}




//UAC
void Dashboard::on_pushButton_UAC_01_clicked()
{
    QString buttonName = "UAC_01";
    QString buttonStyleSheet = ui->pushButton_UAC_01->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_UAC_01->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}



void Dashboard::on_pushButton_UAC_02_clicked()
{
    QString buttonName = "UAC_02";
    QString buttonStyleSheet = ui->pushButton_UAC_02->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_UAC_02->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}



void Dashboard::on_pushButton_UAC_03_clicked()
{
    QString buttonName = "UAC_03";
    QString buttonStyleSheet = ui->pushButton_UAC_03->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_UAC_03->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}



void Dashboard::on_pushButton_UAC_04_clicked()
{
    QString buttonName = "UAC_04";
    QString buttonStyleSheet = ui->pushButton_Canteen_08->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_UAC_04->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_UAC_05_clicked()
{
    QString buttonName = "UAC_05";
    QString buttonStyleSheet = ui->pushButton_UAC_05->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_UAC_05->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}

void Dashboard::on_pushButton_UAC_06_clicked()
{
    QString buttonName = "UAC_06";
    QString buttonStyleSheet = ui->pushButton_UAC_06->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_UAC_06->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_UAC_07_clicked()
{
    QString buttonName = "UAC_07";
    QString buttonStyleSheet = ui->pushButton_UAC_07->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_UAC_07->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


//TANGHALANG BAYAN

void Dashboard::on_pushButton_Tanghalang_Bayan_01_clicked()
{
    QString buttonName = "Tanghalang_Bayan_01";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_01->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_01->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_02_clicked()
{
    QString buttonName = "Tanghalang_Bayan_02";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_02->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_02->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}




void Dashboard::on_pushButton_Tanghalang_Bayan_03_clicked()
{
    QString buttonName = "Tanghalang_Bayan_03";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_03->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_03->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}



void Dashboard::on_pushButton_Tanghalang_Bayan_04_clicked()
{
    QString buttonName = "Tanghalang_Bayan_04";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_04->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_04->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_05_clicked()
{
    QString buttonName = "Tanghalang_Bayan_05";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_05->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_05->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}



void Dashboard::on_pushButton_Tanghalang_Bayan_06_clicked()
{
    QString buttonName = "Tanghalang_Bayan_06";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_06->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_06->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_07_clicked()
{
    QString buttonName = "Tanghalang_Bayan_07";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_07->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_07->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_08_clicked()
{
    QString buttonName = "Tanghalang_Bayan_08";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_08->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_08->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_09_clicked()
{
    QString buttonName = "Tanghalang_Bayan_09";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_09->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_09->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_10_clicked()
{
    QString buttonName = "Tanghalang_Bayan_10";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_10->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_10->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_11_clicked()
{
    QString buttonName = "Tanghalang_Bayan_11";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_11->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_11->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_12_clicked()
{
    QString buttonName = "Tanghalang_Bayan_12";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_12->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_12->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_13_clicked()
{
    QString buttonName = "Tanghalang_Bayan_13";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_13->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_13->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_14_clicked()
{
    QString buttonName = "Tanghalang_Bayan_14";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_14->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_14->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_15_clicked()
{
    QString buttonName = "Tanghalang_Bayan_15";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_15->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_15->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_16_clicked()
{
    QString buttonName = "Tanghalang_Bayan_16";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_16->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_16->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_17_clicked()
{
    QString buttonName = "Tanghalang_Bayan_17";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_17->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_17->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_18_clicked()
{
    QString buttonName = "Tanghalang_Bayan_18";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_18->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_18->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_19_clicked()
{
    QString buttonName = "Tanghalang_Bayan_19";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_19->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_19->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_20_clicked()
{
    QString buttonName = "Tanghalang_Bayan_20";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_20->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_20->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_21_clicked()
{
    QString buttonName = "Tanghalang_Bayan_21";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_21->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_21->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_22_clicked()
{
    QString buttonName = "Tanghalang_Bayan_22";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_22->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_22->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_23_clicked()
{
    QString buttonName = "Tanghalang_Bayan_23";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_23->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_23->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_24_clicked()
{
    QString buttonName = "Tanghalang_Bayan_24";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_24->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_24->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_25_clicked()
{
    QString buttonName = "Tanghalang_Bayan_25";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_25->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_25->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_26_clicked()
{
    QString buttonName = "Tanghalang_Bayan_26";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_26->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_26->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_27_clicked()
{
    QString buttonName = "Tanghalang_Bayan_27";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_27->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_27->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_28_clicked()
{
    QString buttonName = "Tanghalang_Bayan_28";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_28->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_28->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_29_clicked()
{
    QString buttonName = "Tanghalang_Bayan_29";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_29->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_29->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_30_clicked()
{
    QString buttonName = "Tanghalang_Bayan_30";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_30->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_30->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_31_clicked()
{
    QString buttonName = "Tanghalang_Bayan_31";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_31->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_31->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_32_clicked()
{
    QString buttonName = "Tanghalang_Bayan_32";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_32->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_32->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_33_clicked()
{
    QString buttonName = "Tanghalang_Bayan_33";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_33->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_33->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_34_clicked()
{
    QString buttonName = "Tanghalang_Bayan_34";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_34->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_34->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_35_clicked()
{
    QString buttonName = "Tanghalang_Bayan_35";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_35->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_35->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_36_clicked()
{
    QString buttonName = "Tanghalang_Bayan_36";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_36->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_36->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_37_clicked()
{
    QString buttonName = "Tanghalang_Bayan_37";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_37->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_37->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_38_clicked()
{
    QString buttonName = "Tanghalang_Bayan_38";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_38->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_38->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_39_clicked()
{
    QString buttonName = "Tanghalang_Bayan_39";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_39->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_39->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_40_clicked()
{
    QString buttonName = "Tanghalang_Bayan_40";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_40->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_40->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_57pushButton_Tanghalang_Bayan_41_clicked()
{
    QString buttonName = "Tanghalang_Bayan_41";
    QString buttonStyleSheet = ui->pushButton_57pushButton_Tanghalang_Bayan_41->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_57pushButton_Tanghalang_Bayan_41->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}



//ENTRANCE

void Dashboard::on_pushButton_Entrance_01_clicked()
{
    QString buttonName = "Entrance_01";
    QString buttonStyleSheet = ui->pushButton_Entrance_01->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_01->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_02_clicked()
{
    QString buttonName = "Entrance_02";
    QString buttonStyleSheet = ui->pushButton_Entrance_02->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_02->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_03_clicked()
{
    QString buttonName = "Entrance_03";
    QString buttonStyleSheet = ui->pushButton_Entrance_03->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_03->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_04_clicked()
{
    QString buttonName = "Entrance_04";
    QString buttonStyleSheet = ui->pushButton_Entrance_04->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_04->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_05_clicked()
{
    QString buttonName = "Entrance_05";
    QString buttonStyleSheet = ui->pushButton_Entrance_05->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_05->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_06_clicked()
{
    QString buttonName = "Entrance_06";
    QString buttonStyleSheet = ui->pushButton_Entrance_06->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_06->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_07_clicked()
{
    QString buttonName = "Entrance_07";
    QString buttonStyleSheet = ui->pushButton_Entrance_07->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_07->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_08_clicked()
{
    QString buttonName = "Entrance_08";
    QString buttonStyleSheet = ui->pushButton_Entrance_08->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_08->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_09_clicked()
{
    QString buttonName = "Entrance_09";
    QString buttonStyleSheet = ui->pushButton_Entrance_09->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_09->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_10_clicked()
{
    QString buttonName = "Entrance_10";
    QString buttonStyleSheet = ui->pushButton_Entrance_10->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_10->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_11_clicked()
{
    QString buttonName = "Entrance_11";
    QString buttonStyleSheet = ui->pushButton_Entrance_11->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_11->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_12_clicked()
{
    QString buttonName = "Entrance_12";
    QString buttonStyleSheet = ui->pushButton_Entrance_12->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_12->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_13_clicked()
{
    QString buttonName = "Entrance_13";
    QString buttonStyleSheet = ui->pushButton_Entrance_13->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_13->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_14_clicked()
{
    QString buttonName = "Entrance_14";
    QString buttonStyleSheet = ui->pushButton_Entrance_14->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_14->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_15_clicked()
{
    QString buttonName = "Entrance_15";
    QString buttonStyleSheet = ui->pushButton_Entrance_15->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_15->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_16_clicked()
{
    QString buttonName = "Entrance_16";
    QString buttonStyleSheet = ui->pushButton_Entrance_16->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_16->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_17_clicked()
{
    QString buttonName = "Entrance_17";
    QString buttonStyleSheet = ui->pushButton_Entrance_17->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_17->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_18_clicked()
{
    QString buttonName = "Entrance_18";
    QString buttonStyleSheet = ui->pushButton_Entrance_18->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_18->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_19_clicked()
{
    QString buttonName = "Entrance_19";
    QString buttonStyleSheet = ui->pushButton_Entrance_19->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_19->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_20_clicked()
{
    QString buttonName = "Entrance_20";
    QString buttonStyleSheet = ui->pushButton_Entrance_20->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_20->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_21_clicked()
{
    QString buttonName = "Entrance_21";
    QString buttonStyleSheet = ui->pushButton_Entrance_21->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_21->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_22_clicked()
{
    QString buttonName = "Entrance_22";
    QString buttonStyleSheet = ui->pushButton_Entrance_22->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_22->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_23_clicked()
{
    QString buttonName = "Entrance_23";
    QString buttonStyleSheet = ui->pushButton_Entrance_23->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_23->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_24_clicked()
{
    QString buttonName = "Entrance_24";
    QString buttonStyleSheet = ui->pushButton_Entrance_24->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_24->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_25_clicked()
{
    QString buttonName = "Entrance_25";
    QString buttonStyleSheet = ui->pushButton_Entrance_25->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_25->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_26_clicked()
{
    QString buttonName = "Entrance_26";
    QString buttonStyleSheet = ui->pushButton_Entrance_26->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_26->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_27_clicked()
{
    QString buttonName = "Entrance_27";
    QString buttonStyleSheet = ui->pushButton_Entrance_27->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_27->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_28_clicked()
{
    QString buttonName = "Entrance_28";
    QString buttonStyleSheet = ui->pushButton_Entrance_28->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_28->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_29_clicked()
{
    QString buttonName = "Entrance_29";
    QString buttonStyleSheet = ui->pushButton_Entrance_29->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_29->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_30_clicked()
{
    QString buttonName = "Entrance_30";
    QString buttonStyleSheet = ui->pushButton_Entrance_30->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_30->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_31_clicked()
{
    QString buttonName = "Entrance_31";
    QString buttonStyleSheet = ui->pushButton_Entrance_31->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_31->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_32_clicked()
{
    QString buttonName = "Entrance_32";
    QString buttonStyleSheet = ui->pushButton_Entrance_32->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_30->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_33_clicked()
{
    QString buttonName = "Entrance_33";
    QString buttonStyleSheet = ui->pushButton_Entrance_33->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_30->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_34_clicked()
{
    QString buttonName = "Entrance_34";
    QString buttonStyleSheet = ui->pushButton_Entrance_34->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_34->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_35_clicked()
{
    QString buttonName = "Entrance_35";
    QString buttonStyleSheet = ui->pushButton_Entrance_35->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_35->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_36_clicked()
{
    QString buttonName = "Entrance_36";
    QString buttonStyleSheet = ui->pushButton_Entrance_36->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_36->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_37_clicked()
{
    QString buttonName = "Entrance_37";
    QString buttonStyleSheet = ui->pushButton_Entrance_37->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_37->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_38_clicked()
{
    QString buttonName = "Entrance_38";
    QString buttonStyleSheet = ui->pushButton_Entrance_38->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_38->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_39_clicked()
{
    QString buttonName = "Entrance_39";
    QString buttonStyleSheet = ui->pushButton_Entrance_39->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_39->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_40_clicked()
{
    QString buttonName = "Entrance_40";
    QString buttonStyleSheet = ui->pushButton_Entrance_40->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_40->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_41_clicked()
{
    QString buttonName = "Entrance_41";
    QString buttonStyleSheet = ui->pushButton_Entrance_41->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_41->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_42_clicked()
{
    QString buttonName = "Entrance_42";
    QString buttonStyleSheet = ui->pushButton_Entrance_42->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_42->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_UAC_001_clicked()
{
    QString buttonName = "UAC_01";
    QString buttonStyleSheet = ui->pushButton_UAC_01->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_UAC_01->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

        updateButtonStylesheet();

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_UAC_002_clicked()
{

    QString buttonName = "UAC_02";
    QString buttonStyleSheet = ui->pushButton_UAC_02->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_UAC_02->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_UAC_003_clicked()
{
    QString buttonName = "UAC_03";
    QString buttonStyleSheet = ui->pushButton_UAC_03->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_UAC_03->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

        updateButtonStylesheet();

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_UAC_004_clicked()
{
    QString buttonName = "UAC_04";
    QString buttonStyleSheet = ui->pushButton_UAC_04->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_UAC_04->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

        updateButtonStylesheet();

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_UAC_005_clicked()
{
    QString buttonName = "UAC_05";
    QString buttonStyleSheet = ui->pushButton_UAC_05->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_UAC_05->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

        updateButtonStylesheet();

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_UAC_006_clicked()
{
    QString buttonName = "UAC_06";
    QString buttonStyleSheet = ui->pushButton_UAC_06->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_UAC_06->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

        updateButtonStylesheet();

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_UAC_007_clicked()
{
    QString buttonName = "UAC_07";
    QString buttonStyleSheet = ui->pushButton_UAC_07->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_UAC_07->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

        updateButtonStylesheet();

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_001_clicked()
{
    QString buttonName = "Tanghalang_Bayan_01";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_01->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_01->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_002_clicked()
{
    QString buttonName = "Tanghalang_Bayan_02";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_02->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_02->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_003_clicked()
{
    QString buttonName = "Tanghalang_Bayan_03";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_03->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_03->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_004_clicked()
{
    QString buttonName = "Tanghalang_Bayan_04";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_04->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_04->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_005_clicked()
{
    QString buttonName = "Tanghalang_Bayan_05";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_05->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_05->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_006_clicked()
{
    QString buttonName = "Tanghalang_Bayan_06";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_06->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_06->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_007_clicked()
{
    QString buttonName = "Tanghalang_Bayan_07";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_07->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_07->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_008_clicked()
{
    QString buttonName = "Tanghalang_Bayan_08";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_08->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_08->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_009_clicked()
{
    QString buttonName = "Tanghalang_Bayan_09";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_09->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_09->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_010_clicked()
{
    QString buttonName = "Tanghalang_Bayan_010";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_010->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_010->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_011_clicked()
{
    QString buttonName = "Tanghalang_Bayan_011";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_011->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_011->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_012_clicked()
{
    QString buttonName = "Tanghalang_Bayan_012";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_012->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_012->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_013_clicked()
{
    QString buttonName = "Tanghalang_Bayan_013";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_013->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_013->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_014_clicked()
{
    QString buttonName = "Tanghalang_Bayan_014";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_014->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_014->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_015_clicked()
{
    QString buttonName = "Tanghalang_Bayan_015";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_015->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_015->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_016_clicked()
{
    QString buttonName = "Tanghalang_Bayan_016";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_016->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_016->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_017_clicked()
{
    QString buttonName = "Tanghalang_Bayan_017";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_017->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_017->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_018_clicked()
{
    QString buttonName = "Tanghalang_Bayan_018";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_018->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_018->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_019_clicked()
{
    QString buttonName = "Tanghalang_Bayan_019";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_019->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_019->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_020_clicked()
{
    QString buttonName = "Tanghalang_Bayan_020";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_020->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_020->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_021_clicked()
{
    QString buttonName = "Tanghalang_Bayan_021";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_021->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_021->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_022_clicked()
{
    QString buttonName = "Tanghalang_Bayan_022";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_022->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_022->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_023_clicked()
{
    QString buttonName = "Tanghalang_Bayan_023";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_023->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_023->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_024_clicked()
{
    QString buttonName = "Tanghalang_Bayan_024";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_024->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_024->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_025_clicked()
{
    QString buttonName = "Tanghalang_Bayan_025";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_025->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_025->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_026_clicked()
{
    QString buttonName = "Tanghalang_Bayan_026";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_026->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_026->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_027_clicked()
{
    QString buttonName = "Tanghalang_Bayan_027";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_027->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_027->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_028_clicked()
{
    QString buttonName = "Tanghalang_Bayan_028";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_028->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_028->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_029_clicked()
{
    QString buttonName = "Tanghalang_Bayan_029";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_029->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_029->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_030_clicked()
{
    QString buttonName = "Tanghalang_Bayan_030";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_030->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_030->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_031_clicked()
{
    QString buttonName = "Tanghalang_Bayan_031";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_031->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_031->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_032_clicked()
{
    QString buttonName = "Tanghalang_Bayan_032";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_032->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_032->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_033_clicked()
{
    QString buttonName = "Tanghalang_Bayan_033";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_033->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_033->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_034_clicked()
{
    QString buttonName = "Tanghalang_Bayan_034";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_034->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_034->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_035_clicked()
{
    QString buttonName = "Tanghalang_Bayan_035";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_035->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_035->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_036_clicked()
{
    QString buttonName = "Tanghalang_Bayan_036";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_036->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_036->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_037_clicked()
{
    QString buttonName = "Tanghalang_Bayan_037";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_037->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_037->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_038_clicked()
{
    QString buttonName = "Tanghalang_Bayan_038";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_038->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_038->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_039_clicked()
{
    QString buttonName = "Tanghalang_Bayan_039";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_039->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_039->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_040_clicked()
{
    QString buttonName = "Tanghalang_Bayan_040";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_040->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_040->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Tanghalang_Bayan_041_clicked()
{
    QString buttonName = "Tanghalang_Bayan_041";
    QString buttonStyleSheet = ui->pushButton_Tanghalang_Bayan_041->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Tanghalang_Bayan_041->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Canteen_001_clicked()
{
    QString buttonName = "Canteen_01";
    QString buttonStyleSheet = ui->pushButton_Canteen_01->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Canteen_01->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

        updateButtonStylesheet();

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Canteen_002_clicked()
{
    QString buttonName = "Canteen_02";
    QString buttonStyleSheet = ui->pushButton_Canteen_02->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Canteen_02->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

        updateButtonStylesheet();

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Canteen_003_clicked()
{
    QString buttonName = "Canteen_03";
    QString buttonStyleSheet = ui->pushButton_Canteen_03->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Canteen_03->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

        updateButtonStylesheet();

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Canteen_004_clicked()
{
    QString buttonName = "Canteen_04";
    QString buttonStyleSheet = ui->pushButton_Canteen_04->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Canteen_04->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

        updateButtonStylesheet();

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Canteen_005_clicked()
{
    QString buttonName = "Canteen_05";
    QString buttonStyleSheet = ui->pushButton_Canteen_05->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Canteen_05->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

        updateButtonStylesheet();

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Canteen_006_clicked()
{
    QString buttonName = "Canteen_06";
    QString buttonStyleSheet = ui->pushButton_Canteen_06->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Canteen_06->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

        updateButtonStylesheet();

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Canteen_007_clicked()
{
    QString buttonName = "Canteen_07";
    QString buttonStyleSheet = ui->pushButton_Canteen_07->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Canteen_07->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

        updateButtonStylesheet();

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Canteen_008_clicked()
{
    QString buttonName = "Canteen_08";
    QString buttonStyleSheet = ui->pushButton_Canteen_08->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Canteen_08->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

        updateButtonStylesheet();

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_001_clicked()
{
    QString buttonName = "Entrance_01";
    QString buttonStyleSheet = ui->pushButton_Entrance_01->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_01->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_002_clicked()
{
    QString buttonName = "Entrance_02";
    QString buttonStyleSheet = ui->pushButton_Entrance_02->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_02->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_003_clicked()
{
    QString buttonName = "Entrance_03";
    QString buttonStyleSheet = ui->pushButton_Entrance_03->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_03->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_004_clicked()
{
    QString buttonName = "Entrance_04";
    QString buttonStyleSheet = ui->pushButton_Entrance_04->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_04->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_005_clicked()
{
    QString buttonName = "Entrance_05";
    QString buttonStyleSheet = ui->pushButton_Entrance_05->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_05->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_006_clicked()
{
    QString buttonName = "Entrance_06";
    QString buttonStyleSheet = ui->pushButton_Entrance_06->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_06->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_007_clicked()
{
    QString buttonName = "Entrance_07";
    QString buttonStyleSheet = ui->pushButton_Entrance_07->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_07->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_008_clicked()
{
    QString buttonName = "Entrance_08";
    QString buttonStyleSheet = ui->pushButton_Entrance_08->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_08->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_009_clicked()
{
    QString buttonName = "Entrance_09";
    QString buttonStyleSheet = ui->pushButton_Entrance_09->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_09->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_010_clicked()
{
    QString buttonName = "Entrance_010";
    QString buttonStyleSheet = ui->pushButton_Entrance_010->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_010->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_011_clicked()
{
    QString buttonName = "Entrance_011";
    QString buttonStyleSheet = ui->pushButton_Entrance_011->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_011->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_012_clicked()
{
    QString buttonName = "Entrance_012";
    QString buttonStyleSheet = ui->pushButton_Entrance_012->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_012->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_013_clicked()
{
    QString buttonName = "Entrance_013";
    QString buttonStyleSheet = ui->pushButton_Entrance_013->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_013->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_014_clicked()
{
    QString buttonName = "Entrance_014";
    QString buttonStyleSheet = ui->pushButton_Entrance_014->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_014->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_015_clicked()
{
    QString buttonName = "Entrance_015";
    QString buttonStyleSheet = ui->pushButton_Entrance_015->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_015->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_016_clicked()
{
    QString buttonName = "Entrance_06";
    QString buttonStyleSheet = ui->pushButton_Entrance_016->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_016->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_017_clicked()
{
    QString buttonName = "Entrance_017";
    QString buttonStyleSheet = ui->pushButton_Entrance_017->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_017->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_018_clicked()
{
    QString buttonName = "Entrance_018";
    QString buttonStyleSheet = ui->pushButton_Entrance_018->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_018->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_019_clicked()
{
    QString buttonName = "Entrance_019";
    QString buttonStyleSheet = ui->pushButton_Entrance_019->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_019->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_020_clicked()
{
    QString buttonName = "Entrance_020";
    QString buttonStyleSheet = ui->pushButton_Entrance_020->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_020->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_021_clicked()
{
    QString buttonName = "Entrance_021";
    QString buttonStyleSheet = ui->pushButton_Entrance_021->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_021->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_022_clicked()
{
    QString buttonName = "Entrance_022";
    QString buttonStyleSheet = ui->pushButton_Entrance_022->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_022->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_023_clicked()
{
    QString buttonName = "Entrance_023";
    QString buttonStyleSheet = ui->pushButton_Entrance_023->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_023->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_024_clicked()
{
    QString buttonName = "Entrance_024";
    QString buttonStyleSheet = ui->pushButton_Entrance_024->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_024->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_025_clicked()
{
    QString buttonName = "Entrance_025";
    QString buttonStyleSheet = ui->pushButton_Entrance_025->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_025->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_026_clicked()
{
    QString buttonName = "Entrance_026";
    QString buttonStyleSheet = ui->pushButton_Entrance_026->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_026->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_027_clicked()
{
    QString buttonName = "Entrance_027";
    QString buttonStyleSheet = ui->pushButton_Entrance_027->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_027->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_028_clicked()
{
    QString buttonName = "Entrance_028";
    QString buttonStyleSheet = ui->pushButton_Entrance_028->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_028->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_029_clicked()
{
    QString buttonName = "Entrance_029";
    QString buttonStyleSheet = ui->pushButton_Entrance_029->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_029->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_030_clicked()
{
    QString buttonName = "Entrance_030";
    QString buttonStyleSheet = ui->pushButton_Entrance_030->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_030->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_031_clicked()
{
    QString buttonName = "Entrance_031";
    QString buttonStyleSheet = ui->pushButton_Entrance_031->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_031->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_032_clicked()
{
    QString buttonName = "Entrance_032";
    QString buttonStyleSheet = ui->pushButton_Entrance_032->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_032->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_033_clicked()
{
    QString buttonName = "Entrance_033";
    QString buttonStyleSheet = ui->pushButton_Entrance_033->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_033->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_034_clicked()
{
    QString buttonName = "Entrance_034";
    QString buttonStyleSheet = ui->pushButton_Entrance_034->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_034->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_035_clicked()
{
    QString buttonName = "Entrance_035";
    QString buttonStyleSheet = ui->pushButton_Entrance_035->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_035->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_036_clicked()
{
    QString buttonName = "Entrance_036";
    QString buttonStyleSheet = ui->pushButton_Entrance_036->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_036->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_037_clicked()
{
    QString buttonName = "Entrance_037";
    QString buttonStyleSheet = ui->pushButton_Entrance_037->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_037->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_038_clicked()
{
    QString buttonName = "Entrance_038";
    QString buttonStyleSheet = ui->pushButton_Entrance_038->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_038->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_039_clicked()
{
    QString buttonName = "Entrance_039";
    QString buttonStyleSheet = ui->pushButton_Entrance_039->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_039->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_040_clicked()
{
    QString buttonName = "Entrance_040";
    QString buttonStyleSheet = ui->pushButton_Entrance_040->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_040->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_041_clicked()
{
    QString buttonName = "Entrance_041";
    QString buttonStyleSheet = ui->pushButton_Entrance_041->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_041->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Entrance_042_clicked()
{
    QString buttonName = "Entrance_042";
    QString buttonStyleSheet = ui->pushButton_Entrance_042->styleSheet();
    if (buttonStyleSheet.contains("image: url(:/assets/icons/available 2.png);"))
    {
        ParkingDialog parkingDialog;
        parkingDialog.setModal(true);
        parkingDialog.getName(buttonName);
        parkingDialog.exec();

        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            ui->pushButton_Entrance_042->setStyleSheet("image: url(:/assets/icons/occupied 1.png); background-color: rgb(244, 243, 253); border-radius: 8px;");
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/occupied 1.png")){
        connOpen();
        QSqlQuery query;
        query.prepare("SELECT * FROM Parking WHERE ButtonName = :buttonName AND CheckOut IS NULL");
        query.bindValue(":buttonName", buttonName);
        if (query.exec() && query.next()) {
            int reply = QMessageBox::warning(nullptr,"Checkout?","Do you want to Checkout?", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes){

                query.prepare("UPDATE Parking SET ButtonName = NULL, CheckOut = :CheckOut WHERE ButtonName = :ButtonName");
                query.bindValue(":CheckOut", sqlDateTimeText);
                query.bindValue(":ButtonName", buttonName);

                if (query.exec()) {
                    int rowsAffected = query.numRowsAffected();
                    if (rowsAffected > 0) {
                        qDebug() << "Cell updated successfully";
                        updateButtonStylesheet();
                        // Perform any additional operations if needed
                        connClose();
                    } else {
                        qDebug() << "No rows were affected, handle accordingly";
                    }
                } else {
                    qDebug() << "Error executing the query, handle the error";
                }
            }
            connClose();
        }else{
            qDebug() << "No match found for ButtonName and CheckOut is NULL";
            connClose();
        }

    }else if (buttonStyleSheet.contains(":/assets/icons/reserve.png")){
        QMessageBox::warning(nullptr, "Reserved", "This spot is already reserved contact the Admin for more details");
    }
}


void Dashboard::on_pushButton_Logout_3_clicked()
{
    logoutfunc();
}


void Dashboard::on_pushButton_Logout_4_clicked()
{
    logoutfunc();
}

