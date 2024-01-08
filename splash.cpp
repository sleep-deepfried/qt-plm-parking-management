#include "splash.h"
#include "ui_splash.h"

Splash::Splash(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Splash)
{
    ui->setupUi(this);

    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    progressTimer = new QTimer(this);
    connect(progressTimer, &QTimer::timeout, this, &Splash::updateProgressBar);
    progressTimer->start(35);

    labelTimer = new QTimer(this);
    connect(labelTimer, &QTimer::timeout, this, &Splash::updateLabel);
    labelTimer->start(1700); // Change the label text after 5 seconds (adjust as needed)
}

Splash::~Splash()
{
    delete ui;
}

void Splash::updateProgressBar()
{
    ui->progressBar->setValue(0);
    static int count = 0;
    if (count <= 100) {
        ui->progressBar->setValue(count);
        count++;
    } else {
        progressTimer->stop();
        this->hide();
    }
}

void Splash::updateLabel()
{
    static int currentIndex = 0;
    QStringList texts = { "<Strong>Opening</Strong> Database", "<Strong>Loding</Strong> User Interface"}; // List of texts to cycle through
    ui->label_Description->setText(texts[currentIndex]); // Change the label text here

    currentIndex++;
    if (currentIndex >= texts.size()) {
        currentIndex = 0;
    }

}
