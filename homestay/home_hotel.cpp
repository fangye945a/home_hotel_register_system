#include "home_hotel.h"
#include "ui_home_hotel.h"

home_hotel::home_hotel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::home_hotel)
{
    ui->setupUi(this);
    QPalette palette;
    palette.setBrush(QPalette::Background, QBrush(QPixmap("../homestay/pictures/首界面.png")));
    this->setPalette(palette);
}

home_hotel::~home_hotel()
{
    delete ui;
}

void home_hotel::on_check_in_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void home_hotel::on_exit_clicked()
{
    int i = ui->stackedWidget->currentIndex();
    int max = ui->stackedWidget->count();
    if(i<max-1)
    {
        i++;
    }else
        i = 0;
    ui->stackedWidget->setCurrentIndex(i);
}
