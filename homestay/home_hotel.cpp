#include "home_hotel.h"
#include "ui_home_hotel.h"

home_hotel::home_hotel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::home_hotel)
{
    ui->setupUi(this);

    QPalette palette;   //显示首界面背景图
    palette.setBrush(QPalette::Background, QBrush(QPixmap("../homestay/pictures/首界面.png")));
    this->setPalette(palette);

    ui->phone_number->installEventFilter(this);  //声明QLineEdit focus机制的存在
    ui->test_code->installEventFilter(this);

    signal_slots_connect(); //连接信号与槽
    home_hotel_init();      //参数初始化
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
    local_city = "昆明";
    time_timer->start(60*1000); //每分钟更新一次时间
    weather_timer->start(60*60*1000); //每小时更新一次天气
    weather_inquiry();  //获取天气
    update_time();  //更新时间
}

void home_hotel::signal_slots_connect()
{
    time_timer = new QTimer(this);      //时间更新计时
    connect(time_timer,SIGNAL(timeout()),this,SLOT(update_time()));

    weather_timer = new QTimer(this);      //天气更新计时
    connect(weather_timer,SIGNAL(timeout()),this,SLOT(update_weather()));

    weather_manager = new QNetworkAccessManager(this);
    connect(weather_manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(update_weather(QNetworkReply*)));
}

void home_hotel::update_time()
{
    QString year_month_day= QDateTime::currentDateTime().toString("yyyy-MM-dd"); //年月日
    QString hour_minutes= QDateTime::currentDateTime().toString("HH:mm");   //时 分
    ui->yyyy_mm_dd->setText(year_month_day);
    ui->hh_mm->setText(hour_minutes);
}

void home_hotel::weather_inquiry()  //获取天气
{
    /*设置发送数据*/
    char quest_array[256]="http://wthrcdn.etouch.cn/weather_mini?city=";
    QNetworkRequest quest;
    sprintf(quest_array,"%s%s",quest_array,local_city.toUtf8().data());
    qDebug("请求天气数据--> URL:%s\n",quest_array);
    quest.setUrl(QUrl(quest_array));
    quest.setHeader(QNetworkRequest::UserAgentHeader,"RT-Thread ART");
    weather_manager->get(quest);
}
void home_hotel::update_weather(QNetworkReply* reply)
{
    static QString wind_state;     //风向
    static QString Temperature;    //气温
    static QString weather_type;   //天气状况

    QString all = reply->readAll();
    QJsonParseError err;
    QJsonDocument json_recv = QJsonDocument::fromJson(all.toUtf8(),&err);
    qDebug() << err.error;
    if(!json_recv.isNull())
    {
        QJsonObject object = json_recv.object();
        if(object.contains("data"))
        {
            QJsonValue value = object.value("data");  // 获取指定 key 对应的 value
            if(value.isObject())
            {
                QJsonObject object_data = value.toObject();
                if(object_data.contains("forecast"))
                {
                    QJsonValue value = object_data.value("forecast");
                    if(value.isArray())
                    {
                        QJsonObject today_weather = value.toArray().at(0).toObject();
                        weather_type = today_weather.value("type").toString();
                        QString low = today_weather.value("low").toString();
                        QString high = today_weather.value("high").toString();
                        Temperature = "气温:";
                        Temperature += low.mid(low.length()-3,2) +"~"+ high.mid(high.length()-3,4);
                        QString strength = today_weather.value("fengli").toString();
                        strength.remove(0,9);
                        strength.remove(strength.length()-3,3);
                        wind_state = today_weather.value("fengxiang").toString() + ":" + strength;
                    }
                }
            }
        }

    }else
    {
        qDebug()<<"更新天气:更新天气失败，json解析出错!!";
        weather_type = "暂无数据";
        Temperature = "--";
        wind_state = "--";
    }
    char style_sheet_cmd[64]={"border-image: url(:/new/prefix1/pictures/"};
    int weather_index = 0;

    if(weather_type == "晴")
        weather_index = 2;
    else if(weather_type == "阵雨")
        weather_index = 3;
    else if(weather_type == "雷阵雨")
        weather_index = 4;
    else if(weather_type.contains("雪"))
        weather_index = 5;
    else if(weather_type == "多云" || weather_type == "阴" )
        weather_index = 6;
    else if(weather_type.contains("雷"))
        weather_index = 7;
    else if(weather_type.contains("雨"))
        weather_index = 8;

    sprintf(style_sheet_cmd,"%s%d.png);",style_sheet_cmd,weather_index);
    ui->weather_log->setStyleSheet(style_sheet_cmd);  //更新天气图标
    ui->weather_type->setText(weather_type);
    ui->wind->setText(wind_state);
    ui->tempreture->setText(Temperature);
    ui->city->setText(local_city);
    reply->deleteLater();
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
                msg.chop(1);
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
                msg.chop(1);
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
