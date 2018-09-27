#include "home_hotel.h"
#include "ui_home_hotel.h"

QList<cv::Mat> g_video_frame; //全局视频录制画面
QString dev_id;     //酒店ID

extern CARD_INFO card_info; //身份证信息

home_hotel::home_hotel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::home_hotel)
{
    ui->setupUi(this);

    QPalette palette;   //显示首界面背景图
    if(isFileExist(QString("./pictures/首界面.png")))
    {
        palette.setBrush(QPalette::Background, QBrush(QPixmap("./pictures/首界面.png")));
    }
    else if(isFileExist(QString("../homestay/pictures/首界面.png")))
    {
        palette.setBrush(QPalette::Background, QBrush(QPixmap("../homestay/pictures/首界面.png")));
    }
    this->setPalette(palette);
    ui->phone_number->installEventFilter(this);  //声明QLineEdit focus机制的存在
    ui->test_code->installEventFilter(this);

    signal_slots_connect(); //连接信号与槽
    home_hotel_init();      //参数初始化
    this->setWindowFlags (Qt::Window | Qt::FramelessWindowHint);
    //this->showFullScreen();
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

void home_hotel::read_ini_file()
{
    if(!isFileExist(FILE_PATH)) //不存在则创建,使用默认参数
    {
        QSettings *writeIniFile = new QSettings(FILE_PATH, QSettings::IniFormat);
        writeIniFile->setValue("/setting/dev_id","1");
        writeIniFile->setValue("/setting/city",QString("宁波"));
        writeIniFile->setValue("/setting/confidence_threshold","70");
        writeIniFile->setValue("/setting/api_key",API_Key);
        writeIniFile->setValue("/setting/secret_key",Secret_Key);
        writeIniFile->setValue("/setting/compare_threshold","3"); //3次
        writeIniFile->setValue("/setting/shelving_time","60000");
        delete writeIniFile;
    }
    QSettings *readIniFile = new QSettings(FILE_PATH, QSettings::IniFormat);
    dev_id = readIniFile->value("/setting/dev_id").toString();
    local_city = readIniFile->value("/setting/city").toString();
    confidence_threshold = readIniFile->value("/setting/confidence_threshold").toString().toInt();
    api_key = readIniFile->value("/setting/api_key").toString();
    secret_key = readIniFile->value("/setting/secret_key").toString();
    compare_threshold = readIniFile->value("/setting/compare_threshold").toString().toInt();
    shelving_time = readIniFile->value("/setting/shelving_time").toString().toInt();
    delete readIniFile;
}

void home_hotel::home_hotel_init()
{
    focus_flag = -1;
    opt_code = -1;
    identifying_code.clear();
    ui->exit->hide();

    get_code_timer = NULL;
    page_shelving_timer = NULL;
    frame_data = new cv::Mat;
    camera = new cv::VideoCapture;
    ccf = new cv::CascadeClassifier;
    if(isFileExist(QString("./haarcascade_frontalface_default_2.4.9.xml")))
    {
        if(!ccf->load("./haarcascade_frontalface_default_2.4.9.xml")) //导入opencv自带检测的文件
           qDebug()<<"无法加载xml文件";
        else
           qDebug()<<"加载xml文件成功";
    }
    else if(isFileExist(QString("../homestay/haarcascade_frontalface_default_2.4.9.xml")))
    {
        if(!ccf->load("../homestay/haarcascade_frontalface_default_2.4.9.xml")) //导入opencv自带检测的文件
           qDebug()<<"无法加载xml文件";
        else
           qDebug()<<"加载xml文件成功";
    }
    q_image_data = new QImage;
    time_timer->start(60*1000); //每分钟更新一次时间
    weather_timer->start(60*60*1000); //每小时更新一次天气

    read_ini_file();    //读取配置文件
    weather_inquiry();  //获取天气
    update_time();  //更新时间
}

void home_hotel::signal_slots_connect()
{
    time_timer = new QTimer(this);      //时间更新计时
    connect(time_timer,SIGNAL(timeout()),this,SLOT(update_time()));

    weather_timer = new QTimer(this);      //天气更新计时
    connect(weather_timer,SIGNAL(timeout()),this,SLOT(update_weather()));

    frame_timer = new QTimer(this);      //天气更新计时
    connect(frame_timer,SIGNAL(timeout()),this,SLOT(get_frame()));

    weather_manager = new QNetworkAccessManager(this); //天气数据请求
    connect(weather_manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(update_weather(QNetworkReply*)));

    pthread_card = new detect_card_pthread();
    connect(pthread_card,SIGNAL(detected()),this, SLOT(detect_card_finish()));

    face_compare_manager = new QNetworkAccessManager(this); //人脸比对请求
    connect(face_compare_manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(face_compare_result(QNetworkReply*)));
}

void home_hotel::closeEvent(QCloseEvent *event)  //关闭前退出线程
{
    if(pthread_card != NULL && pthread_card->isRunning())    //退出前先关闭线程
    {
       pthread_card->stop();
       pthread_card->wait();
    }
}

QImage home_hotel::ScaleImage2Label(QImage qImage, QLabel* qLabel) // 使图片能适应Lable部件
{
    QImage qScaledImage;
    QSize qImageSize = qImage.size();
    QSize qLabelSize = qLabel->size();
    double dWidthRatio = 1.0*qImageSize.width() / qLabelSize.width();
    double dHeightRatio = 1.0*qImageSize.height() / qLabelSize.height();
    if (dWidthRatio>dHeightRatio)
    {
        qScaledImage = qImage.scaledToWidth(qLabelSize.width());
    }
    else
    {
        qScaledImage = qImage.scaledToHeight(qLabelSize.height());
    }
    return qScaledImage;
}

void home_hotel::face_compare_show() //人脸比较界面显示
{
    QImage my_id_pic;
    if(isFileExist(QString("./release/photo.bmp")))
    {
        my_id_pic.load(QString("./release/photo.bmp"));
    }
    else if(isFileExist(QString("./photo.bmp")))
    {
        my_id_pic.load(QString("./photo.bmp"));
    }

    QByteArray pic_tmp;
    QBuffer buf(&pic_tmp);
    my_id_pic.save(&buf,"BMP",20);
    pic_IDcard = pic_tmp.toBase64();  //获取身份证图片转为BASE64用于上传
    buf.close();
    qDebug()<<"身份证图片大小:"<<pic_IDcard.length();
    QImage scaleImage = ScaleImage2Label(my_id_pic, ui->id_pic);   // 显示到label上
    ui->id_pic->setPixmap(QPixmap::fromImage(scaleImage));
    ui->id_pic->setAlignment(Qt::AlignCenter);
    ui->id_pic->show();
    ui->help_msg_page3->setText("身份证读取成功，请正视摄像头进行认证比对");
    ui->stackedWidget->setCurrentIndex(FACE_COMPARE);
    open_camera();
}

void home_hotel::open_camera() //打开摄像头
{
    if(!camera->isOpened())  //没打开摄像头则打开
    {
        camera->open(0);
    }else
    {
        ui->help_msg_page3->setText("摄像头打开失败!!");
    }
    frame_timer->start(camera_T); //开始读取视频
    face_detect_flag = 1;       //开启人脸识别
}

void home_hotel::close_camera() //关闭摄像头
{
    frame_timer->stop();
    camera->release();
}

void home_hotel::update_time() //更新时间
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

void home_hotel::update_weather(QNetworkReply* reply) //更新显示天气
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

    if(weather_type == "晴转阴"  || weather_type == "阴转晴")
        weather_index = 1;
    else if(weather_type == "晴")
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
    reply->deleteLater();//释放对象
}

bool home_hotel::isFileExist(QString fullFileName) //判断文件是否存在
{
    QFileInfo fileInfo(fullFileName);
    if(fileInfo.isFile())
    {
        return true; //存在
    }
    return false; //不存在
}

void home_hotel::detect_card_finish() //身份证检测完成
{
#if 0
    QString tmp = QString::fromLocal8Bit(card_info.name);
    qDebug()<<"姓名:"<<tmp;
    tmp = QString::fromLocal8Bit(card_info.sex);
    qDebug()<<"性别:"<<tmp;
    tmp = QString::fromLocal8Bit(card_info.nation);
    qDebug()<<"民族:"<<tmp;
    tmp = QString::fromLocal8Bit(card_info.birth);
    qDebug()<<"出生日期:"<<tmp;
    tmp = QString::fromLocal8Bit(card_info.address);
    qDebug()<<"住址:"<<tmp;
    tmp = QString::fromLocal8Bit(card_info.card_id);
    qDebug()<<"身份证号码："<<tmp;
    tmp = QString::fromLocal8Bit(card_info.registry);
    qDebug()<<"签发机关："<<tmp;
#endif
    face_compare_show();
}

QImage home_hotel::Mat2QImage(cv::Mat& cvImg)  // Mat格式转换为QImage格式
{
    QImage qImg;
    if(cvImg.channels()==3)                             //3 channels color image
    {
        cv::cvtColor(cvImg,cvImg,CV_BGR2RGB);
        qImg =QImage((const unsigned char*)(cvImg.data),
                    cvImg.cols, cvImg.rows,
                    cvImg.cols*cvImg.channels(),
                    QImage::Format_RGB888);
    }
    else if(cvImg.channels()==1)                    //grayscale image
    {
        qImg =QImage((const unsigned char*)(cvImg.data),
                    cvImg.cols,cvImg.rows,
                    cvImg.cols*cvImg.channels(),
                    QImage::Format_Indexed8);
    }
    else
    {
        qImg =QImage((const unsigned char*)(cvImg.data),
                    cvImg.cols,cvImg.rows,
                    cvImg.cols*cvImg.channels(),
                    QImage::Format_RGB888);
    }
    return qImg;
}

bool home_hotel::detectface(cv::Mat &image) //检测人脸
{
    std::vector<cv::Rect> faces;
    cv::Mat gray;
    cv::cvtColor(image,gray,CV_BGR2GRAY);
    cv::equalizeHist(gray,gray);
    ccf->detectMultiScale(gray,faces,1.1,3,0,cv::Size(100,100),cv::Size(300,300));

    cv::Rect outline(120,40,400,400);
    cv::rectangle(image,outline,cv::Scalar(255,166,106),2,8);

    for(std::vector<cv::Rect>::const_iterator iter=faces.begin();iter!=faces.end();iter++)
    {

        cv::Rect rec = *iter;
        if(rec.x>outline.x && rec.y > outline.y &&
           (rec.x+rec.width) < (outline.x+outline.width) &&
           (rec.y+rec.height)< (outline.y+outline.height))
        {
            return true;
        }else
        {
            return false;
        }
    }
    return false;
}

void home_hotel::request_token()
{
    QNetworkRequest ask_tooken(QUrl("https://aip.baidubce.com/oauth/2.0/token"));
    QByteArray array("grant_type=client_credentials&client_id=");
    array += api_key.toLocal8Bit();
    array += "&client_secret=";
    array += secret_key.toLocal8Bit();
    face_compare_manager->post(ask_tooken,array);
}

void home_hotel::compare_face()
{
    if(token.isEmpty()) //如果没有获取token值则先获取token值
    {
        request_token();
        return;
    }

    char quest_url[256]={0};
    sprintf(quest_url,"https://aip.baidubce.com/rest/2.0/face/v3/match?access_token=%s",token.toLocal8Bit().data());
    qDebug()<<"人脸信息上传--> URL:"<<quest_url;
    QUrl qurl(quest_url);
    QNetworkRequest request(qurl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("application/json"));

    QByteArray quest_array("[{\"image\":\"");
    quest_array.append(pic_Live);
    quest_array.append("\",\"image_type\":\"BASE64\",\"face_type\":\"LIVE\",\"quality_control\":\"LOW\",\"liveness_control\":\"NONE\"},{\"image\":\"");
    quest_array.append(pic_IDcard);
    quest_array.append("\",\"image_type\":\"BASE64\",\"face_type\":\"IDCARD\",\"quality_control\":\"LOW\",\"liveness_control\":\"NONE\"}]");
    face_compare_manager->post(request,quest_array);
}

void home_hotel::get_frame()    //获取一帧图像
{
    frame_timer->stop();  //停止计时
    camera->read(*frame_data);
    if(frame_data->empty())
    {
        qDebug()<<"frame is empty!!";
        frame_timer->start(camera_T);
        return;
    }

//    if(video_record_timer->isActive()) //如果开始录制视频则记录该视频
//    {
//        cv::Mat videoframe = frame_data->clone();
//        g_video_frame.append(videoframe);
//    }

    *q_image_data = Mat2QImage(*frame_data);  //Mat转化为QImage
    if(face_detect_flag && detectface(*frame_data))
    {
        QByteArray tmp;
        QBuffer buf(&tmp);
        q_image_data->save(&buf,"BMP",20);
        pic_Live = tmp.toBase64();  //获取摄像头图像数据转为base64存储
        buf.close();
        qDebug()<<"视频图像大小:"<<pic_Live.length();
        compare_face();
        face_detect_flag = 0;
    }

    QImage scaleImage = ScaleImage2Label(*q_image_data, ui->show_face);   // 显示到label上
    ui->show_face->setPixmap(QPixmap::fromImage(scaleImage));
    ui->show_face->setAlignment(Qt::AlignCenter);
    ui->show_face->show();
    frame_timer->start(camera_T);
}

void home_hotel::upload_info() //上传身份证信息
{
    common_manager = new QNetworkAccessManager(this); //身份信息上传请求
    connect(common_manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(upload_info_result(QNetworkReply*))); //连接槽

    QUrl qurl("http://hotel.inteink.com/hotel/login/uploadInfo");
    QNetworkRequest request(qurl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("application/json"));

    QByteArray quest_array("{\"card_img\":\"");
    quest_array.append(pic_IDcard);
    //quest_array.append("pic_IDcard");
    quest_array.append("\",\"face_img\":\"");
    quest_array.append(pic_Live);
    //quest_array.append("pic_Live");
    quest_array.append("\",\"id\":\"");
    quest_array.append(dev_id.toUtf8());
    quest_array.append("\",\"username\":\"");
    quest_array.append(QString::fromLocal8Bit(card_info.name).toUtf8());

    quest_array.append("\",\"cardid\":\"");
    quest_array.append(QString::fromLocal8Bit(card_info.card_id).toUtf8());
    quest_array.append("\",\"sex\":\"");
    quest_array.append(QString::fromLocal8Bit(card_info.sex).toUtf8());
    quest_array.append("\",\"nativeplace\":\"");
    quest_array.append(QString::fromLocal8Bit(card_info.nation).toUtf8());
    quest_array.append("\",\"birthday\":\"");
    quest_array.append(QString::fromLocal8Bit(card_info.birth).toUtf8());
    quest_array.append("\",\"nativeaddress\":\"");
    quest_array.append(QString::fromLocal8Bit(card_info.address).toUtf8());
    quest_array.append("\",\"sign\":\"");
    quest_array.append(QString::fromLocal8Bit(card_info.registry).toUtf8());
    quest_array.append("\"}");

    qDebug()<<"入住上传身份证信息---> URL:"<<qurl;
    qDebug()<<"body:"<<QString::fromLocal8Bit(quest_array);
    common_manager->post(request,quest_array);
}

void home_hotel::upload_info_result(QNetworkReply* reply) //上传身份证信息回复
{
    //QTextCodec *codec = QTextCodec::codecForName("utf8");
    QString Receive_http = reply->readAll();//codec->toUnicode(reply->readAll());
    qDebug()<<"上传身份信息回复-->"<<Receive_http;

    QJsonParseError err;
    QJsonDocument json_recv = QJsonDocument::fromJson(Receive_http.toUtf8(),&err);
    qDebug() << "JSon Error_code ="<<err.error;
    if(!json_recv.isNull() && json_recv.isObject())
    {
        QJsonObject object = json_recv.object();
        if(object.contains("house_ordernumber") && object.contains("result"))
        {
            QJsonValue room_ordernumber = object.value("house_ordernumber");
            ordernumber = room_ordernumber.toString();
            QJsonValue help_msg = object.value("result");
            ui->success_help->setText(help_msg.toString());
            ui->success_msg->setText("*房间人数不得超过2人");
            ui->stackedWidget->setCurrentIndex(CHECK_IN_SUCCESS);
        }
        else if(object.contains("ret_code") && object.contains("result"))
        {
            QJsonValue ret_code = object.value("ret_code");
            switch(ret_code.toInt())
            {
                case 200:
                {
                    qDebug()<<"获取数据成功";
                    ui->stackedWidget->setCurrentIndex(PHONENUMBER_SIGN);
                }break;
                case 400:
                {
                    qDebug()<<"获取数据失败";
                    QJsonValue help_msg = object.value("result");
                    ui->fail_help->setText(help_msg.toString());
                    ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
                }break;
            default:
                qDebug()<<"未知返回码:"<<ret_code.toInt();
                ui->fail_help->setText("服务器数据有误,请联系管理员。");
                ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
                break;
            }
        }else
        {
            qDebug()<<"未包含指定字段!!";
            ui->fail_help->setText("服务器数据有误,请联系管理员。");
            ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
        }
    }else
    {
        qDebug()<<"Json格式有误!!";
        ui->fail_help->setText("服务器数据有误,请联系管理员。");
        ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
    }
    close_camera(); //关闭摄像头

    reply->deleteLater();//释放对象
    disconnect(common_manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(upload_info_result(QNetworkReply*))); //取消连接
    delete common_manager;  //用完释放
    common_manager = NULL;
    qDebug()<<"释放公用http请求句柄..";
}

void home_hotel::upload_info_add_people()
{
    common_manager = new QNetworkAccessManager(this); //身份信息上传请求
    connect(common_manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(upload_info_add_people_result(QNetworkReply*))); //连接槽

    char add_people_url[256]="http://hotel.inteink.com/hotel/login/addTenantInfo";
    sprintf(add_people_url,"%s?house_ordernumber=%s",add_people_url,ordernumber.toLocal8Bit().data());

    QUrl qurl(add_people_url);
    QNetworkRequest request(qurl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("application/json"));

    QByteArray quest_array("{\"id\":");
    quest_array.append(dev_id.toUtf8());

    quest_array.append(",\"card_img\":\"");
    quest_array.append(pic_IDcard);

    quest_array.append("\",\"face_img\":\"");
    quest_array.append(pic_Live);

    quest_array.append("\",\"username\":\"");
    quest_array.append(QString::fromLocal8Bit(card_info.name).toUtf8());
    quest_array.append("\",\"cardid\":\"");
    quest_array.append(QString::fromLocal8Bit(card_info.card_id).toUtf8());
    quest_array.append("\",\"sex\":\"");
    quest_array.append(QString::fromLocal8Bit(card_info.sex).toUtf8());
    quest_array.append("\",\"nativeplace\":\"");
    quest_array.append(QString::fromLocal8Bit(card_info.nation).toUtf8());
    quest_array.append("\",\"birthday\":\"");
    quest_array.append(QString::fromLocal8Bit(card_info.birth).toUtf8());
    quest_array.append("\",\"nativeaddress\":\"");
    quest_array.append(QString::fromLocal8Bit(card_info.address).toUtf8());
    quest_array.append("\",\"sign\":\"");
    quest_array.append(QString::fromLocal8Bit(card_info.registry).toUtf8());
    quest_array.append("\"}");

    qDebug()<<"增加人员上传身份证信息---> URL:"<<qurl;
    common_manager->post(request,quest_array);

}

void home_hotel::upload_info_add_people_result(QNetworkReply *reply)
{
    QString Receive_http = reply->readAll();
    qDebug()<<"增加人员上传身份证信息回复-->"<<Receive_http;

    QJsonParseError err;
    QJsonDocument json_recv = QJsonDocument::fromJson(Receive_http.toUtf8(),&err);
    qDebug() << "JSon Error_code ="<<err.error;
    if(!json_recv.isNull() && json_recv.isObject())
    {
        QJsonObject object = json_recv.object();
        if(object.contains("ret_code") && object.contains("result"))
        {
            QJsonValue ret_code = object.value("ret_code");
            switch(ret_code.toInt())
            {
                case 200:
                {
                    qDebug()<<"添加人员成功";
                    QJsonValue help_msg = object.value("result");
                    ui->success_help->setText(help_msg.toString());
                    ui->success_msg->setText("*房间人数不得超过2人");
                    ui->stackedWidget->setCurrentIndex(CHECK_IN_SUCCESS);
                }break;
                case 400:
                {
                    qDebug()<<"添加人员失败";
                    QJsonValue help_msg = object.value("result");
                    ui->fail_help->setText(help_msg.toString());
                    ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
                }break;
            default:qDebug()<<"未知返回码:"<<ret_code.toInt();break;
            }
        }else
        {
            qDebug()<<"未包含指定字段!!";
            ui->fail_help->setText("服务器数据有误,请联系管理员。");
            ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
        }
    }else
    {
        qDebug()<<"Json格式有误!!";
        ui->fail_help->setText("服务器数据有误,请联系管理员。");
        ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
    }
    close_camera(); //关闭摄像头

    reply->deleteLater();//释放对象
    disconnect(common_manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(upload_info_add_people_result(QNetworkReply*))); //取消连接
    delete common_manager;  //用完释放
    common_manager = NULL;
    qDebug()<<"释放公用http请求句柄..";
}

void home_hotel::face_compare_result(QNetworkReply* reply)
{
    static int times_cnt = 0;
    qDebug()<<"获得人脸比对结果!!";
    QTextCodec *codec = QTextCodec::codecForName("utf8");
    QString Receive_http = codec->toUnicode(reply->readAll());

    qDebug()<<"Receive_http:"<<Receive_http;
    QJsonDocument json_recv = QJsonDocument::fromJson(Receive_http.toLocal8Bit());

    int err_code = 0;
    QString msg;
    if(!json_recv.isNull() && json_recv.isObject())
    {
        QJsonObject object = json_recv.object();
        if(object.contains("error_code"))
        {
            QJsonValue error_code = object.value("error_code");
            err_code = error_code.toInt();
            switch(err_code)
            {
            case 0:break;
            case 4:msg = "集群超限额,请联系管理员";break;
            case 6:msg = "没有接口权限,请联系管理员";break;
            case 17:msg = "每天流量超限额,请联系管理员";break;
            case 18:msg = "QPS超限额,请联系管理员";break;
            case 19:msg = "请求总量超限额,请联系管理员";break;
            case 100:msg = "无效的access_token参数,请联系管理员";break;
            case 110:msg = "Access Token失效,请联系管理员";break;
            case 111:msg = "Access token过期,请联系管理员";break;

            case 222201:
            case 222205:
            case 222302:
            case 222206: msg = "服务端请求失败,请重试";break;

            case 222202:
            case 222203: msg = "图片模糊,请重试";break;


            case 223113:case 223116:case 223121:
            case 223122:case 223123:case 223124:
            case 223125:case 223126:case 223127: msg = "人脸有遮挡,请重试";break;

            case 223114:
            case 223115: msg = "人脸模糊,请重试";break;
            default:  msg = "未知错误,请重试(错误码:";
                msg += QString::number(err_code);
                msg += ")";
                break;
            }
            if(err_code)
            {
                ui->help_msg_page3->setText(msg);
                face_detect_flag = 1;
            }
        }

        if(object.contains("result") && err_code == 0)
        {
            QJsonValue value = object.value("result");  // 获取指定 key 对应的 value
            if (value.isObject())  //判断 value 是否为Object
            {
                QJsonObject result_object = value.toObject();
                if(result_object.contains("score"))
                {
                    QJsonValue result_value = result_object.value("score");  // 获取指定 key 对应的 value
                    if (value.isObject())  //判断 value 是否为Object
                    {
                        double result_score = result_value.toDouble();

                        if(result_score > confidence_threshold)
                        {
                            msg = "人证比对通过,请稍后";
                            times_cnt = 0;
                            ui->help_msg_page3->setText(msg);
                            face_detect_flag = 0;
                            switch(opt_code)
                            {
                                case CHECK_IN:
                                {
                                    upload_info();  //上传身份证信息
                                }break;
                                case ADD_PEOPLE:
                                {
                                    upload_info_add_people();//上传增加人员的身份信息
                                }break;
                                case GET_KEY:
                                {
                                    get_room_info_get_key();    //获取住房信息(取钥匙)
                                }break;
                                case CHECK_OUT:
                                {
                                    get_room_info_check_out();    //获取住房信息(退房)
                                }break;
                            }
                        }else
                        {

                            if(times_cnt < compare_threshold)
                            {
                                msg = "人证比对不一致，请重试";
                                ui->help_msg_page3->setText(msg);
                                face_detect_flag = 1;
                                times_cnt++;
                            }
                            else      //比对错误超过3次,返回首界面
                            {
                                times_cnt = 0;
                                face_detect_flag = 0;
                                on_exit_clicked();
                            }
                        }
                    }
                }
            }
        }
        else if(object.contains("access_token"))
        {
            QJsonValue value = object.value("access_token");  // 获取指定 key 对应的 value
            if (value.isString())  //判断 value 是否为Object
            {
                token = value.toString(); //获取token值
                qDebug()<<"get token:"<<token;
                compare_face();
            }
        }
    }
    else
    {
        qDebug() <<"!json_recv.isNull() && json_recv.isObject()";
        msg = "请求数据失败,网络故障!";
        ui->help_msg_page3->setText(msg);
        face_detect_flag = 1;
    }
    reply->deleteLater();//释放对象
}

void home_hotel::on_get_code_clicked() //获取验证码
{
    if(ui->phone_number->text().length() != 11)
    {
        ui->phone_sign_help_msg->setText("*手机号码长度必须为11位");
        return;
    }

    if(page_shelving_timer != NULL && page_shelving_timer->isActive()) //如果正在定时中
    {
        page_shelving_timer->stop();
        page_shelving_timer->start(shelving_time+60000);  //获取验证码后可多加60s定时延时
        qDebug()<<"关闭页面搁置定时..";
        qDebug()<<"开启页面搁置定时..";
    }
    else
    {
        page_shelving_timer = new QTimer(this);
        connect(page_shelving_timer,SIGNAL(timeout()),this,SLOT(page_shelving_timeout()));
        qDebug()<<"开启页面搁置定时..";
        page_shelving_timer->start(shelving_time+60000);  //重新开始定时
    }


    common_manager = new QNetworkAccessManager(this); //身份信息上传请求
    connect(common_manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(get_code_reply(QNetworkReply*))); //连接槽

    /*设置发送数据*/
    char quest_array[256]={0};
    QNetworkRequest quest;
    if(opt_code == CHECK_IN)//入住时获取验证码
        sprintf(quest_array,"http://hotel.inteink.com/hotel/tenant/getNumber?telephone=%s",ui->phone_number->text().toUtf8().data());
    else
        sprintf(quest_array,"http://hotel.inteink.com/hotel/tenant/updateTelephone?inputTelephone=%s",ui->phone_number->text().toUtf8().data());

    qDebug("请求验证码--> URL:%s\n",quest_array);
    quest.setUrl(QUrl(quest_array));
    quest.setHeader(QNetworkRequest::UserAgentHeader,"RT-Thread ART");
    common_manager->get(quest);
    ui->get_code->setEnabled(false);

    get_code_timer = new QTimer(this);
    connect(get_code_timer,SIGNAL(timeout()),this,SLOT(get_code_timeout()));
    get_code_timer->start(1000);
    get_code_timer_count = 0;
    QString time_display = QString::number(60-get_code_timer_count) + "s";
    ui->get_code->setText(time_display);
    focus_flag = 1;

}

void home_hotel::get_code_reply(QNetworkReply* reply)   //获取验证码响应
{
    QString all = reply->readAll();
    qDebug()<<"请求验证码响应-->"<<all;
    QJsonParseError err;
    QJsonDocument json_recv = QJsonDocument::fromJson(all.toUtf8(),&err);
    qDebug() << "error_code ="<<err.error;
    if(!json_recv.isNull() && json_recv.isObject())
    {
        QJsonObject object = json_recv.object();
        if(object.contains("ret_code") && object.contains("result"))
        {
            QJsonValue ret_code = object.value("ret_code");
            int temp = ret_code.toInt();
            if(temp == 400)
            {
                qDebug()<<"获取验证码失败";
                QJsonValue help_msg = object.value("result");
                ui->fail_help->setText(help_msg.toString());
                ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
            }
            else
            {
                identifying_code = QString::number(temp);
                ui->phone_sign_help_msg->setText("验证码已发送至该手机");
                qDebug()<<"获取验证码成功:"<<identifying_code;
            }

        }else
        {
            qDebug()<<"未包含指定字段!!";
            ui->fail_help->setText("服务器数据有误,请联系管理员。");
            ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
        }
    }else
    {
        qDebug()<<"Json格式有误!!";
        ui->fail_help->setText("服务器数据有误,请联系管理员。");
        ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
    }

    //identifying_code = "123456";
    disconnect(common_manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(get_code_reply(QNetworkReply*))); //取消连接
    delete common_manager;  //用完释放
    common_manager = NULL;
    qDebug()<<"释放公用http请求句柄..";
}

void home_hotel::get_code_timeout()
{
    get_code_timer_count++;
    if(get_code_timer_count == 60)
    {
        ui->get_code->setText("获取验证码");
        ui->get_code->setEnabled(true);
        get_code_timer->stop();
        disconnect(get_code_timer,SIGNAL(timeout()),this,SLOT(get_code_timeout()));
        delete get_code_timer;
        get_code_timer = NULL;
    }else
    {
        QString time_display = QString::number(60-get_code_timer_count) + "s";
        ui->get_code->setText(time_display);
    }
}

void home_hotel::get_room_info_get_key_reply(QNetworkReply *reply) //获取房间信息（取钥匙）
{
    QString all = reply->readAll();
    qDebug()<<"获取房间信息响应(取钥匙)-->"<<all;
    QJsonParseError err;
    QJsonDocument json_recv = QJsonDocument::fromJson(all.toUtf8(),&err);
    qDebug() << "error_code ="<<err.error;
    if(!json_recv.isNull() && json_recv.isObject())
    {
        QJsonObject object = json_recv.object();
        if(object.contains("house_name") && object.contains("house_number") &&
           object.contains("startDate") && object.contains("endDate") &&
           object.contains("telephone") && object.contains("bed_info"))
        {
            qDebug()<<"获取房间信息响应(取钥匙)成功";

            QJsonValue house_name = object.value("house_name");
            QJsonValue house_number = object.value("house_number");
            QJsonValue startDate = object.value("startDate");
            QJsonValue endDate = object.value("endDate");
            QJsonValue telephone = object.value("telephone");
            QJsonValue bed_info = object.value("bed_info");

            QString room_type_and_number = house_name.toString() + "/" + house_number.toString();
            ui->room_type_and_number->setText(room_type_and_number);
            ui->user_info->setText(QString::fromLocal8Bit(card_info.name));
            ui->tel_info->setText(telephone.toString());
            QString date_info = startDate.toString() + "~" + endDate.toString();
            ui->date_info->setText(date_info);
            ui->bed_info->setText(bed_info.toString());

            ui->room_info_bt1->setText("获取钥匙");
            ui->room_info_bt2->setText("更改手机号");

            ui->stackedWidget->setCurrentIndex(ROOM_INFO_PAGE);
        }
        else if(object.contains("ret_code") && object.contains("result"))
        {
            QJsonValue ret_code = object.value("ret_code");
            int temp = ret_code.toInt();
            if(temp == 400)
            {
                qDebug()<<"获取房间信息响应(取钥匙)失败";
                QJsonValue help_msg = object.value("result");
                ui->fail_help->setText(help_msg.toString());
                ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
            }
            else
            {
                ui->fail_help->setText("获取住房信息失败,请联系管理员。");
                ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
            }

        }
        else
        {
            qDebug()<<"未包含指定字段!!";
            ui->fail_help->setText("服务器数据有误,请联系管理员。");
            ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
        }
    }else
    {
        qDebug()<<"Json格式有误!!";
        ui->fail_help->setText("服务器数据有误,请联系管理员。");
        ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
    }

    disconnect(common_manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(get_room_info_get_key_reply(QNetworkReply*))); //取消连接
    delete common_manager;  //用完释放
    common_manager = NULL;
    qDebug()<<"释放公用http请求句柄..";
}

void home_hotel::get_room_info_check_out_reply(QNetworkReply *reply)
{
    QString all = reply->readAll();
    qDebug()<<"获取房间信息响应(退房)-->"<<all;
    QJsonParseError err;
    QJsonDocument json_recv = QJsonDocument::fromJson(all.toUtf8(),&err);
    qDebug() << "error_code ="<<err.error;
    if(!json_recv.isNull() && json_recv.isObject())
    {
        QJsonObject object = json_recv.object();
        if(object.contains("house_name") && object.contains("house_number") &&
           object.contains("startDate") && object.contains("endDate") &&
           object.contains("telephone") && object.contains("bed_info"))
        {
            qDebug()<<"获取房间信息响应(退房)成功";
            QJsonValue house_name = object.value("house_name");
            QJsonValue house_number = object.value("house_number");
            QJsonValue startDate = object.value("startDate");
            QJsonValue endDate = object.value("endDate");
            QJsonValue telephone = object.value("telephone");
            QJsonValue bed_info = object.value("bed_info");
            QJsonValue house_ordernumber = object.value("house_ordernumber");

            ordernumber = house_ordernumber.toString();
            QString room_type_and_number = house_name.toString() + "/" + house_number.toString();
            ui->room_type_and_number->setText(room_type_and_number);
            ui->user_info->setText(QString::fromLocal8Bit(card_info.name));
            ui->tel_info->setText(telephone.toString());
            QString date_info = startDate.toString() + "~" + endDate.toString();
            ui->date_info->setText(date_info);
            ui->bed_info->setText(bed_info.toString());

            ui->room_info_bt1->setText("取消");
            ui->room_info_bt2->setText("确认退房");

            ui->stackedWidget->setCurrentIndex(ROOM_INFO_PAGE);
        }
        else if(object.contains("ret_code") && object.contains("result"))
        {
            QJsonValue ret_code = object.value("ret_code");
            int temp = ret_code.toInt();
            if(temp == 400)
            {
                 qDebug()<<"获取房间信息响应(退房)失败";
                QJsonValue help_msg = object.value("result");
                ui->fail_help->setText(help_msg.toString());
                ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
            }
            else
            {
                ui->fail_help->setText("获取住房信息失败,请联系管理员。");
                ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
            }

        }
        else
        {
            qDebug()<<"未包含指定字段!!";
            ui->fail_help->setText("服务器数据有误,请联系管理员。");
            ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
        }
    }else
    {
        qDebug()<<"Json格式有误!!";
        ui->fail_help->setText("服务器数据有误,请联系管理员。");
        ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
    }

    disconnect(common_manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(get_room_info_check_out_reply(QNetworkReply*))); //取消连接
    delete common_manager;  //用完释放
    common_manager = NULL;
    qDebug()<<"释放公用http请求句柄..";
}

void home_hotel::get_key_reply(QNetworkReply *reply)
{
    QString all = reply->readAll();
    qDebug()<<"获取钥匙响应-->"<<all;
    QJsonParseError err;
    QJsonDocument json_recv = QJsonDocument::fromJson(all.toUtf8(),&err);
    qDebug() << "error_code ="<<err.error;
    if(!json_recv.isNull() && json_recv.isObject())
    {
        QJsonObject object = json_recv.object();
        if(object.contains("ret_code") && object.contains("result"))
        {
            QJsonValue ret_code = object.value("ret_code");
            int temp = ret_code.toInt();
            if(temp == 400)
            {
                qDebug()<<"获取钥匙失败";
                QJsonValue help_msg = object.value("result");
                ui->fail_help->setText(help_msg.toString());
                ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
            }
            else if(temp == 200)
            {
                qDebug()<<"获取钥匙成功";
                QJsonValue help_msg = object.value("result");
                ui->success_help->setText(help_msg.toString());
                ui->success_msg->setText("*一个手机号一天最多获取3次秘钥");
                ui->stackedWidget->setCurrentIndex(CHECK_IN_SUCCESS);
            }
            else
            {
                qDebug()<<"ret_code 有误!!";
                ui->fail_help->setText("服务器数据有误,请联系管理员。");
                ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
            }

        }else
        {
            qDebug()<<"未包含指定字段!!";
            ui->fail_help->setText("服务器数据有误,请联系管理员。");
            ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
        }
    }else
    {
        qDebug()<<"Json格式有误!!";
        ui->fail_help->setText("服务器数据有误,请联系管理员。");
        ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
    }

    disconnect(common_manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(get_key_reply(QNetworkReply*))); //取消连接
    delete common_manager;  //用完释放
    common_manager = NULL;
    qDebug()<<"释放公用http请求句柄..";
}

void home_hotel::check_out_reply(QNetworkReply *reply)
{
    QString all = reply->readAll();
    qDebug()<<"退房请求响应-->"<<all;
    QJsonParseError err;
    QJsonDocument json_recv = QJsonDocument::fromJson(all.toUtf8(),&err);
    qDebug() << "error_code ="<<err.error;
    if(!json_recv.isNull() && json_recv.isObject())
    {
        QJsonObject object = json_recv.object();
        if(object.contains("ret_code") && object.contains("result"))
        {
            QJsonValue ret_code = object.value("ret_code");
            int temp = ret_code.toInt();
            if(temp == 400)
            {
                qDebug()<<"退房失败";
                QJsonValue help_msg = object.value("result");
                ui->fail_help->setText(help_msg.toString());
                ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
            }
            else if(temp == 200)
            {
                qDebug()<<"退房成功";
                QJsonValue help_msg = object.value("result");
                ui->success_help->setText(help_msg.toString());
                ui->success_msg->clear();
                ui->stackedWidget->setCurrentIndex(CHECK_IN_SUCCESS);
            }
            else
            {
                qDebug()<<"ret_code 有误!!";
                ui->fail_help->setText("服务器数据有误,请联系管理员。");
                ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
            }
        }else
        {
            qDebug()<<"未包含指定字段!!";
            ui->fail_help->setText("服务器数据有误,请联系管理员。");
            ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
        }
    }else
    {
        qDebug()<<"Json格式有误!!";
        ui->fail_help->setText("服务器数据有误,请联系管理员。");
        ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
    }

    disconnect(common_manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(check_out_reply(QNetworkReply*))); //取消连接
    delete common_manager;  //用完释放
    common_manager = NULL;
    qDebug()<<"释放公用http请求句柄..";
}


void home_hotel::ensure_change_telphone() //确认更换手机
{
    common_manager = new QNetworkAccessManager(this); //身份信息上传请求
    connect(common_manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(ensure_change_telphone_reply(QNetworkReply*))); //连接槽

    /*设置发送数据*/
    char quest_array[256]="http://hotel.inteink.com/hotel/wav/updateSendMessage";
    QNetworkRequest quest;
    sprintf(quest_array,"%s?telephone=%s&inputTelephone=%s",quest_array,ui->tel_info->text().toUtf8().data(),ui->phone_number->text().toUtf8().data());
    qDebug("确认更改手机号码--> URL:%s\n",quest_array);
    quest.setUrl(QUrl(quest_array));
    quest.setHeader(QNetworkRequest::UserAgentHeader,"RT-Thread ART");
    common_manager->get(quest);
}

void home_hotel::page_shelving_timeout()
{
     on_exit_clicked();
}

void home_hotel::ensure_check_in() //确认登记入住
{
    common_manager = new QNetworkAccessManager(this); //身份信息上传请求
    connect(common_manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(ensure_check_in_reply(QNetworkReply*))); //连接槽

    /*设置发送数据*/
    char quest_array[256]="http://hotel.inteink.com/hotel/wav/sendMessage";
    QNetworkRequest quest;
    sprintf(quest_array,"%s?telephone=%s&cardID=%s",quest_array,ui->phone_number->text().toUtf8().data(),card_info.card_id);
    qDebug("确认入住请求--> URL:%s\n",quest_array);
    quest.setUrl(QUrl(quest_array));
    quest.setHeader(QNetworkRequest::UserAgentHeader,"RT-Thread ART");
    common_manager->get(quest);
    ui->phone_sign_help_msg->setText("登记中，请稍后..");
}

void home_hotel::get_room_info_get_key()  //获取住房信息(取钥匙)
{
    common_manager = new QNetworkAccessManager(this);
    connect(common_manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(get_room_info_get_key_reply(QNetworkReply*))); //连接槽

    QUrl qurl("http://hotel.inteink.com/hotel/login/getVoiceInfo");
    QNetworkRequest request(qurl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("application/json"));

    QByteArray quest_array("{\"cardid\":\"");
    quest_array.append(QString::fromLocal8Bit(card_info.card_id).toUtf8());
    quest_array.append("\"}");

    qDebug()<<"获取房间信息请求(取钥匙)---> URL:"<<qurl;
    common_manager->post(request,quest_array);
}

void home_hotel::get_room_info_check_out()
{
    common_manager = new QNetworkAccessManager(this);
    connect(common_manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(get_room_info_check_out_reply(QNetworkReply*))); //连接槽

    QUrl qurl("http://hotel.inteink.com/hotel/login/tenantCheckOutInfo");
    QNetworkRequest request(qurl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("application/json"));

    QByteArray quest_array("{\"cardid\":\"");
    quest_array.append(QString::fromLocal8Bit(card_info.card_id).toUtf8());
    quest_array.append("\"}");

    qDebug()<<"获取房间信息请求(退房)---> URL:"<<qurl;
    common_manager->post(request,quest_array);
}

void home_hotel::get_key_request() //获取钥匙
{
    common_manager = new QNetworkAccessManager(this); //身份信息上传请求
    connect(common_manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(get_key_reply(QNetworkReply*))); //连接槽

    /*设置发送数据*/
    char quest_array[256]="http://hotel.inteink.com/hotel/wav/getSendMessage";

    QNetworkRequest quest;
    sprintf(quest_array,"%s?telephone=%s",quest_array,ui->tel_info->text().toUtf8().data());
    qDebug("获取钥匙请求--> URL:%s\n",quest_array);
    quest.setUrl(QUrl(quest_array));
    quest.setHeader(QNetworkRequest::UserAgentHeader,"RT-Thread ART");
    common_manager->get(quest);
}

void home_hotel::check_out_request()  //请求退房
{
    common_manager = new QNetworkAccessManager(this); //身份信息上传请求
    connect(common_manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(check_out_reply(QNetworkReply*))); //连接槽

    /*设置发送数据*/
    char quest_array[256]="http://hotel.inteink.com/hotel/tenant/tenantCheckOut";

    QNetworkRequest quest;
    sprintf(quest_array,"%s?house_ordernumber=%s",quest_array,ordernumber.toLocal8Bit().data());
    qDebug("请求退房--> URL:%s\n",quest_array);
    quest.setUrl(QUrl(quest_array));
    quest.setHeader(QNetworkRequest::UserAgentHeader,"RT-Thread ART");
    common_manager->get(quest);
}

void home_hotel::ensure_check_in_reply(QNetworkReply* reply) //确认登记入住
{
    QString all = reply->readAll();
    QString msg;
    qDebug()<<"确认入住请求响应-->"<<all;
    QJsonParseError err;
    QJsonDocument json_recv = QJsonDocument::fromJson(all.toUtf8(),&err);
    qDebug() << "error_code ="<<err.error;
    if(!json_recv.isNull() && json_recv.isObject())
    {
        QJsonObject object = json_recv.object();
        if(object.contains("house_ordernumber") && object.contains("result")) //成功
        {
            QJsonValue success_msg = object.value("result");
            msg = success_msg.toString();
            ui->success_help->setText(msg);
            ui->stackedWidget->setCurrentIndex(CHECK_IN_SUCCESS);
        }
        else if(object.contains("ret_code") && object.contains("result"))
        {
            QJsonValue ret_code = object.value("ret_code");
            int temp = ret_code.toInt();
            if(temp == 400)
            {
                qDebug()<<"入住登记失败";
                QJsonValue help_msg = object.value("result");
                msg = help_msg.toString();
                ui->fail_help->setText(msg);
                ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
            }
        }else
        {
            qDebug()<<"未包含指定字段!!";
            ui->fail_help->setText("服务器数据有误,请联系管理员。");
            ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
        }
    }else
    {
        qDebug()<<"Json格式有误!!";
        ui->fail_help->setText("服务器数据有误,请联系管理员。");
        ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
    }

    disconnect(common_manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(ensure_check_in_reply(QNetworkReply*))); //取消连接
    delete common_manager;  //用完释放
    common_manager = NULL;
    qDebug()<<"释放公用http请求句柄..";
}

void home_hotel::ensure_change_telphone_reply(QNetworkReply *reply)
{
    QString all = reply->readAll();
    QString msg;
    qDebug()<<"确认更改手机号码响应-->"<<all;
    QJsonParseError err;
    QJsonDocument json_recv = QJsonDocument::fromJson(all.toUtf8(),&err);
    qDebug() << "error_code ="<<err.error;
    if(!json_recv.isNull() && json_recv.isObject())
    {
        QJsonObject object = json_recv.object();
        if(object.contains("house_ordernumber") && object.contains("result")) //成功
        {
            QJsonValue success_msg = object.value("result");
            msg = success_msg.toString();
            ui->success_help->setText(msg);
            ui->stackedWidget->setCurrentIndex(CHECK_IN_SUCCESS);
        }
        else if(object.contains("ret_code") && object.contains("result"))
        {
            QJsonValue ret_code = object.value("ret_code");
            int temp = ret_code.toInt();
            if(temp == 400)
            {
                qDebug()<<"更换手机号码失败";
                QJsonValue help_msg = object.value("result");
                msg = help_msg.toString();
                ui->fail_help->setText(msg);
                ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
            }
            else if(temp == 200)
            {
                qDebug()<<"更换手机号码成功";
                QJsonValue help_msg = object.value("result");
                msg = help_msg.toString();
                ui->success_help->setText(msg);
                ui->stackedWidget->setCurrentIndex(CHECK_IN_SUCCESS);
            }
            else
            {
                ui->fail_help->setText("服务器数据有误,请联系管理员。");
                ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
            }
        }
        else
        {
            qDebug()<<"未包含指定字段!!";
            ui->fail_help->setText("服务器数据有误,请联系管理员。");
            ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
        }
    }else
    {
        qDebug()<<"Json格式有误!!";
        ui->fail_help->setText("服务器数据有误,请联系管理员。");
        ui->stackedWidget->setCurrentIndex(ASK_ERROR_PAGE);
    }

    disconnect(common_manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(ensure_change_telphone_reply(QNetworkReply*))); //取消连接
    delete common_manager;  //用完释放
    common_manager = NULL;
    qDebug()<<"释放公用http请求句柄..";
}

void home_hotel::on_ensure_sign_clicked() //确认登记
{
    if(ui->test_code->text() == identifying_code)
    {
        if(opt_code == CHECK_IN)
        {
            ensure_check_in(); //确认入住
        }
        else if(opt_code == GET_KEY)
        {
            ensure_change_telphone();//确认更改手机号
        }

        if(get_code_timer->isActive())//比对通过关闭定时器
        {

            qDebug()<<"-------------delete get_code_timer";
            get_code_timer->stop();
            disconnect(get_code_timer,SIGNAL(timeout()),this,SLOT(get_code_timeout()));
            delete get_code_timer;
            get_code_timer = NULL;
        }
    }
    else
    {
        if(ui->test_code->text().length() != 6)
            ui->phone_sign_help_msg->setText("*验证码长度必须为6位");
        else
            ui->phone_sign_help_msg->setText("*验证码有误,请重新输入");
    } 
}

void home_hotel::on_add_people_clicked() //增加人员
{
    opt_code = ADD_PEOPLE;
    ui->stackedWidget->setCurrentIndex(CARD_DETECT);
    pthread_card->start();
}

void home_hotel::on_check_in_clicked() //入住
{
    opt_code = CHECK_IN;
    ui->stackedWidget->setCurrentIndex(CARD_DETECT);
    pthread_card->start();

}

void home_hotel::on_get_the_key_clicked() //取钥匙
{
    opt_code = GET_KEY;
    ui->stackedWidget->setCurrentIndex(CARD_DETECT);
    pthread_card->start();
    ui->room_info_help_msg->setText("*一个手机号一天最多获取三次秘钥");
}

void home_hotel::on_check_out_clicked() //退房
{
    opt_code = CHECK_OUT;
    ui->stackedWidget->setCurrentIndex(CARD_DETECT);
    pthread_card->start();
    ui->room_info_help_msg->setText("*退房后将无法打开房门，提前退房请主动联系房东结算费用");
}

void home_hotel::on_exit_clicked()  //退出按钮
{
    if(pthread_card->isRunning())
    {
        pthread_card->stop();
        pthread_card->wait();
    }

    if(camera->isOpened()) //如果视频已经打开则关闭视频
    {
         close_camera();
    }

    if(focus_flag != -1)
    {
        qDebug()<<"focus_flag = -1";
        focus_flag = -1;
        ui->test_code->setText("请输入验证码");
        ui->test_code->setStyleSheet("#test_code{color: rgb(184, 184, 184);"
                                        "font: 75 16pt \"Microsoft YaHei UI Light\";"
                                        "border-image: url(:/new/prefix1/pictures/输入文本框.png);}");

        ui->phone_number->setText("请输入手机号");
        ui->phone_number->setStyleSheet("#phone_number{color: rgb(184, 184, 184);"
                                        "font: 75 16pt \"Microsoft YaHei UI Light\";"
                                        "border-image: url(:/new/prefix1/pictures/输入文本框.png);}");
    }
    if(get_code_timer != NULL  &&  get_code_timer->isActive())
    {

        qDebug()<<"-------------delete get_code_timer";
        get_code_timer->stop();
        disconnect(get_code_timer,SIGNAL(timeout()),this,SLOT(get_code_timeout()));
        delete get_code_timer;
        get_code_timer = NULL;
    }
    ui->stackedWidget->setCurrentIndex(FIRST_PAGE);
//-----------------用于预览界面-----------------
//    int i = ui->stackedWidget->currentIndex();
//    int max = ui->stackedWidget->count();
//    if(i<max-1)
//    {
//        i++;
//    }else
//        i = 0;
//    ui->stackedWidget->setCurrentIndex(i);
}

void home_hotel::set_lineEdit_text(int opt_code) //显示输入字符
{
    qDebug()<<"set_lineEdit_text"<<" focus_flag="<<focus_flag<<" opt_code="<<opt_code;
    if(focus_flag == -1)
    {
        ui->phone_number->setFocus();
        ui->phone_number->clear();
        ui->phone_number->setStyleSheet("#phone_number{color: rgb(0,0,0);"
                                        "font: 75 16pt \"Microsoft YaHei UI Light\";"
                                        "border-image: url(:/new/prefix1/pictures/输入文本框.png);}");
        focus_flag = 0;
    }
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
        if( ui->test_code->text() == "请输入验证码")
        {
            ui->test_code->setFocus();
            ui->test_code->clear();
            ui->test_code->setStyleSheet("#test_code{color: rgb(0,0,0);"
                                            "font: 75 16pt \"Microsoft YaHei UI Light\";"
                                            "border-image: url(:/new/prefix1/pictures/输入文本框.png);}");
        }

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

void home_hotel::on_num_clean_clicked() //按键清空
{
    set_lineEdit_text(-2);
}

void home_hotel::on_num_del_clicked()   //按键删除
{
    set_lineEdit_text(-1);
}

void home_hotel::on_num_0_clicked()     //按键0
{
    set_lineEdit_text(0);
}

void home_hotel::on_num_1_clicked()     //按键1
{
    set_lineEdit_text(1);
}

void home_hotel::on_num_2_clicked()     //按键2
{
    set_lineEdit_text(2);
}

void home_hotel::on_num_3_clicked()     //按键3
{
    set_lineEdit_text(3);
}

void home_hotel::on_num_4_clicked()     //按键4
{
    set_lineEdit_text(4);
}

void home_hotel::on_num_5_clicked()     //按键5
{
    set_lineEdit_text(5);
}

void home_hotel::on_num_6_clicked()     //按键6
{
    set_lineEdit_text(6);
}

void home_hotel::on_num_7_clicked()     //按键7
{
    set_lineEdit_text(7);
}

void home_hotel::on_num_8_clicked()     //按键8
{
    set_lineEdit_text(8);
}

void home_hotel::on_num_9_clicked()     //按键9
{
    set_lineEdit_text(9);
}

void home_hotel::on_stackedWidget_currentChanged(int arg1)
{
    if(arg1 == FIRST_PAGE)  //首页面不显示退出按钮
    {
        ui->exit->hide();
        if(page_shelving_timer != NULL && page_shelving_timer->isActive()) //如果正在定时中
        {
            page_shelving_timer->stop();
            disconnect(page_shelving_timer,SIGNAL(timeout()),this,SLOT(page_shelving_timeout()));
            delete page_shelving_timer;
            page_shelving_timer = NULL;
            qDebug()<<"关闭页面搁置定时..";
        }
    }else
    {
        ui->exit->show();
        if(page_shelving_timer != NULL && page_shelving_timer->isActive()) //如果正在定时中
        {
            page_shelving_timer->stop();
            page_shelving_timer->start(shelving_time);  //重新开始定时
            qDebug()<<"关闭页面搁置定时..";
            qDebug()<<"开启页面搁置定时..";
        }
        else
        {
            page_shelving_timer = new QTimer(this);
            connect(page_shelving_timer,SIGNAL(timeout()),this,SLOT(page_shelving_timeout()));
            qDebug()<<"开启页面搁置定时..";
            page_shelving_timer->start(shelving_time);  //重新开始定时
        }
    }

    if(opt_code == GET_KEY || opt_code == CHECK_OUT) //取钥匙和退房不显示增加人员按钮
    {
        ui->horizontalSpacer_28->changeSize(0,0, QSizePolicy::Expanding);
        ui->add_people->hide(); //隐藏增加人员按钮
    }
    else
    {
        ui->horizontalSpacer_28->changeSize(40, 20, QSizePolicy::Expanding);
        ui->add_people->show(); //显示增加人员按钮
    }

    if(arg1 == PHONENUMBER_SIGN)
    {
        ui->get_code->setText("获取验证码");
        ui->get_code->setEnabled(true);
    }
}

void home_hotel::on_success_finish_clicked()
{
    on_exit_clicked();
}

void home_hotel::on_fail_finish_clicked()
{
    on_exit_clicked();
}

void home_hotel::on_room_info_bt1_clicked()
{
    if(ui->room_info_bt1->text() == "获取钥匙")
    {
        get_key_request();
    }
    else if(ui->room_info_bt1->text() == "取消")
    {
        on_exit_clicked();
    }
}

void home_hotel::on_room_info_bt2_clicked()
{
     if(ui->room_info_bt2->text() == "更改手机号")
     {
         ui->stackedWidget->setCurrentIndex(PHONENUMBER_SIGN);
     }
     else if(ui->room_info_bt2->text() == "确认退房")
     {
        check_out_request();
     }
}
