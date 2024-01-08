#include "login.h"
#include "dashboard.h"
#include "splash.h"

#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Login login;

    Dashboard dashboard;
    login.show();

    // Connect the custom signal 'windowHidden' in the Login class to a slot that shows the Dashboard window
    QObject::connect(&login, &Login::windowHidden, [&dashboard]() {
        QTimer::singleShot(5000, [&dashboard]() {
            dashboard.show();
        });
    });



//    Dashboard dashboard;
//        dashboard.show();

    return a.exec();
}
