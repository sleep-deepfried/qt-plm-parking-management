#ifndef SPLASH_H
#define SPLASH_H

#include <QMainWindow>
#include <QTimer>
#include <QLabel>

#include "dashboard.h"

namespace Ui {
class Splash;
}

class Splash : public QMainWindow
{
    Q_OBJECT

public:
    explicit Splash(QWidget *parent = nullptr);
    ~Splash();



private:
    Ui::Splash *ui;
    QTimer* progressTimer;
    QTimer* labelTimer;
    Dashboard *dashboard;


private slots:
    void updateProgressBar();

    void updateLabel();


};

#endif // SPLASH_H
