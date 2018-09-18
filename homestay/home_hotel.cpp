#include "home_hotel.h"
#include "ui_home_hotel.h"

home_hotel::home_hotel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::home_hotel)
{
    ui->setupUi(this);
    home_hotel_init();
    QPalette palette;
    palette.setBrush(QPalette::Background, QBrush(QPixmap("../homestay/pictures/首界面.png")));
    this->setPalette(palette);
    ui->phone_number->installEventFilter(this);  //声明机制的存在
    ui->test_code->installEventFilter(this);
}

home_hotel::~home_hotel()
{
    delete ui;
}

bool home_hotel::eventFilter(QObject *obj, QEvent *event)
{
    if( event->type() == QEvent::MouseButtonPress)
    {
        if(ui->test_code->text().isEmpty())
        {
            ui->test_code->setText("请输入验证码");
            ui->test_code->setStyleSheet("#test_code{color: rgb(184, 184, 184);"
                                            "font: 75 16pt \"Microsoft YaHei UI Light\";"
                                            "border-image: url(:/new/prefix1/pictures/输入文本框.png);}");
        }
        if(ui->phone_number->text().isEmpty())
        {
            ui->phone_number->setText("请输入手机号");
            ui->phone_number->setStyleSheet("#phone_number{color: rgb(184, 184, 184);"
                                            "font: 75 16pt \"Microsoft YaHei UI Light\";"
                                            "border-image: url(:/new/prefix1/pictures/输入文本框.png);}");
        }
        QMouseEvent *me = (QMouseEvent*)event;
        if( me->button() == Qt::LeftButton )
        {
            if( obj == ui->phone_number )
            {
                if(ui->phone_number->text() == "请输入手机号")
                {
                    ui->phone_number->clear();
                    ui->phone_number->setStyleSheet("#phone_number{color: rgb(0,0,0);"
                                                    "font: 75 16pt \"Microsoft YaHei UI Light\";"
                                                    "border-image: url(:/new/prefix1/pictures/输入文本框.png);}");
                }
                focus_flag = 0;
            }
            else if(obj == ui->test_code )
            {
                if(ui->test_code->text() == "请输入验证码")
                {
                    ui->test_code->clear();
                    ui->test_code->setStyleSheet("#test_code{color: rgb(0,0,0);"      //"color: rgb(184, 184, 184);"
                                                 "font: 75 16pt \"Microsoft YaHei UI Light\";"
                                                 "border-image: url(:/new/prefix1/pictures/输入文本框.png);}");
                }
                focus_flag = 1;
            }
        }
    }
    return QWidget::eventFilter(obj,event);
}

void home_hotel::home_hotel_init()
{
    focus_flag = -1;
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

void home_hotel::set_lineEdit_text(int opt_code)
{
    qDebug()<<"set_lineEdit_text"<<" focus_flag="<<focus_flag<<" opt_code="<<opt_code;
    if(focus_flag == 0)
    {
        QString msg = ui->phone_number->text();
        switch (opt_code)
        {
            case -2:     //清空
                ui->phone_number->clear();
                break;
            case -1:     //删除
                msg.remove(msg.length()-1,1);
                ui->phone_number->setText(msg);
                break;
            case 0:
                msg.append("0");
                ui->phone_number->setText(msg);
                break;
            case 1:
                msg.append("1");
                ui->phone_number->setText(msg);
                break;
            case 2:
                msg.append("2");
                ui->phone_number->setText(msg);
                break;
            case 3:
                msg.append("3");
                ui->phone_number->setText(msg);
                break;
            case 4:
                msg.append("4");
                ui->phone_number->setText(msg);
                break;
            case 5:
                msg.append("5");
                ui->phone_number->setText(msg);
                break;
            case 6:
                msg.append("6");
                ui->phone_number->setText(msg);
                break;
            case 7:
                msg.append("7");
                ui->phone_number->setText(msg);
                break;
            case 8:
                msg.append("8");
                ui->phone_number->setText(msg);
                break;
            case 9:
                msg.append("9");
                ui->phone_number->setText(msg);
                break;
        default: qDebug()<<"Unknow opt code = "<<opt_code;
            break;
        }
    }
    else if(focus_flag == 1)
    {
        QString msg = ui->test_code->text();
        switch (opt_code)
        {
            case -2:     //清空
                ui->test_code->clear();
                break;
            case -1:     //删除
                msg.remove(msg.length()-1,1);
                ui->test_code->setText(msg);
                break;
            case 0:
                msg.append("0");
                ui->test_code->setText(msg);
                break;
            case 1:
                msg.append("1");
                ui->test_code->setText(msg);
                break;
            case 2:
                msg.append("2");
                ui->test_code->setText(msg);
                break;
            case 3:
                msg.append("3");
                ui->test_code->setText(msg);
                break;
            case 4:
                msg.append("4");
                ui->test_code->setText(msg);
                break;
            case 5:
                msg.append("5");
                ui->test_code->setText(msg);
                break;
            case 6:
                msg.append("6");
                ui->test_code->setText(msg);
                break;
            case 7:
                msg.append("7");
                ui->test_code->setText(msg);
                break;
            case 8:
                msg.append("8");
                ui->test_code->setText(msg);
                break;
            case 9:
                msg.append("9");
                ui->test_code->setText(msg);
                break;
        default: qDebug()<<"Unknow opt code = "<<opt_code;
            break;
        }
    }
}

void home_hotel::on_num_clean_clicked()
{
    set_lineEdit_text(-2);
}

void home_hotel::on_num_del_clicked()
{
    set_lineEdit_text(-1);
}

void home_hotel::on_num_0_clicked()
{
    set_lineEdit_text(0);
}

void home_hotel::on_num_1_clicked()
{
    set_lineEdit_text(1);
}

void home_hotel::on_num_2_clicked()
{
    set_lineEdit_text(2);
}

void home_hotel::on_num_3_clicked()
{
    set_lineEdit_text(3);
}

void home_hotel::on_num_4_clicked()
{
    set_lineEdit_text(4);
}

void home_hotel::on_num_5_clicked()
{
    set_lineEdit_text(5);
}

void home_hotel::on_num_6_clicked()
{
    set_lineEdit_text(6);
}

void home_hotel::on_num_7_clicked()
{
    set_lineEdit_text(7);
}

void home_hotel::on_num_8_clicked()
{
    set_lineEdit_text(8);
}

void home_hotel::on_num_9_clicked()
{
    set_lineEdit_text(9);
}
