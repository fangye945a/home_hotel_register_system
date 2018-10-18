#include "home_hotel.h"
#include "ui_home_hotel.h"

QList<cv::Mat> g_video_frame; //全局视频录制画面


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
    signal_slots_connect(); //连接信号与槽
    home_hotel_init();      //参数初始化
    this->setWindowFlags (Qt::Window | Qt::FramelessWindowHint);
    //this->showFullScreen();
}

home_hotel::~home_hotel()
{
    delete ui;
}

void home_hotel::read_ini_file()  //读取配置文件
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
        writeIniFile->setValue("/setting/shelving_time","60000"); //一分钟
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
    face_compare_try_times = 0;
    face_detect_flag = 0;
    ui->exit->hide();

    page_shelving_timer = NULL;
    frame_data = new cv::Mat;
    camera = new cv::VideoCapture;
    ccf = new cv::CascadeClassifier;
    q_image_data = new QImage;

    if(!ccf->load("./haarcascade_frontalface_default_2.4.9.xml")) //导入opencv自带检测的文件
       qDebug()<<"无法加载xml文件";
    else
       qDebug()<<"加载xml文件成功";

    read_ini_file(); //读取配置文件

    time_timer->start(60*1000); //每分钟更新一次时间
    weather_timer->start(30*60*1000); //每半个小时更新一次天气
    weather_inquiry();  //获取天气
    update_time();  //更新时间
}

void home_hotel::signal_slots_connect()
{
    time_timer = new QTimer(this);      //时间更新计时
    connect(time_timer,SIGNAL(timeout()),this,SLOT(update_time()));

    weather_timer = new QTimer(this);      //天气更新计时
    connect(weather_timer,SIGNAL(timeout()),this,SLOT(weather_inquiry()));

    frame_timer = new QTimer(this);      //天气更新计时
    connect(frame_timer,SIGNAL(timeout()),this,SLOT(get_frame()));

    weather_manager = new QNetworkAccessManager(this); //天气数据请求
    connect(weather_manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(update_weather(QNetworkReply*)));

    pthread_card = new detect_card_pthread();           //检测身份证信息
    connect(pthread_card,SIGNAL(detected()),this, SLOT(detect_card_finish()));

    face_compare_manager = new QNetworkAccessManager(this); //人脸比对请求
    connect(face_compare_manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(face_compare_result(QNetworkReply*)));

    video_record_timer = new QTimer(this);   //视频录制计时
    connect(video_record_timer, SIGNAL(timeout()), this, SLOT(start_upload_video()));         //时间定时器
}

void home_hotel::page_shelving_timeout()
{
    add_to_log(QString("页面搁置超时"));
    on_exit_clicked();
}

void home_hotel::closeEvent(QCloseEvent *event)  //关闭前退出线程
{
    if(pthread_card->isRunning())    //退出前先关闭线程
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
    ui->help_msg_page3->setText("身份证读取成功，请正视摄像头进行人证比对。");
    ui->stackedWidget->setCurrentIndex(FACE_COMPARE);
    face_detect_flag = 1;
}

void home_hotel::open_camera() //打开摄像头
{
    if(!camera->isOpened())  //没打开摄像头则打开
    {
        add_to_log(QString("打开摄像头"));
        camera->open(0);
    }else
    {
        add_to_log(QString("打开摄像头失败"));
        ui->help_msg_page3->setText("摄像头打开失败!!");
    }
    frame_timer->start(camera_T); //开始读取视频
    //video_record_timer->start(5000); //录制5秒
}
void home_hotel::start_upload_video()
{
    video_record_timer->stop();

    video_upload = new VIDEO_UPDATE();
    video_upload->start();  //开启压缩上传线程

}
void home_hotel::close_camera() //关闭摄像头
{
    add_to_log(QString("关闭摄像头"));
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

    QString log_msg = "请求天气数据--> URL:";
    log_msg += quest_array;
    add_to_log(log_msg);

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
    qDebug()<<"Recieve:"<<all;

    QString log_msg = "请求天气数据响应--> Recieve:";
    log_msg += all;
    add_to_log(log_msg);

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
        qDebug()<<"更新天气:更新天气失败,json解析出错!!";
        QString log_msg = "更新天气失败,json解析出错!!";
        add_to_log(log_msg);

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
    QString log_msg = "身份证检测完成:\n";
    log_msg += "\t\t\t姓名:";
    QString tmp = QString::fromLocal8Bit(card_info.name);
    log_msg += tmp;
    log_msg += "\n";
    log_msg += "\t\t\t性别:";
    tmp = QString::fromLocal8Bit(card_info.sex);
    log_msg += tmp;
    log_msg += "\n";
    log_msg += "\t\t\t民族:";
    tmp = QString::fromLocal8Bit(card_info.nation);
    log_msg += tmp;
    log_msg += "\n";
    log_msg += "\t\t\t出生日期:";
    tmp = QString::fromLocal8Bit(card_info.birth);
    log_msg += tmp;
    log_msg += "\n";
    log_msg += "\t\t\t住址:";
    tmp = QString::fromLocal8Bit(card_info.address);
    log_msg += tmp;
    log_msg += "\n";
    log_msg += "\t\t\t身份证号码:";
    tmp = QString::fromLocal8Bit(card_info.card_id);
    log_msg += tmp;
    log_msg += "\n";
    log_msg += "\t\t\t签发机关：";
    tmp = QString::fromLocal8Bit(card_info.registry);
    log_msg += tmp;
    add_to_log(log_msg);
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
            QString log_msg = "检测到人脸";
            add_to_log(log_msg);
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
    qDebug()<<QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")<<":获取token值";
    QNetworkRequest ask_tooken(QUrl("https://aip.baidubce.com/oauth/2.0/token"));
    QByteArray array("grant_type=client_credentials&client_id=");
    array += api_key.toLocal8Bit();
    array += "&client_secret=";
    array += secret_key.toLocal8Bit();
    face_compare_manager->post(ask_tooken,array);
    QString log = "请求获取token值--> URL:";
    log += ask_tooken.url().toString();
    log += array;
    add_to_log(log);
}

void home_hotel::compare_face()
{
    qDebug()<<QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")<<":人脸比对请求";
    if(token.isEmpty()) //如果没有获取token值则先获取token值
    {
        request_token();
        return;
    }

    char quest_url[256]={0};
    sprintf(quest_url,"https://aip.baidubce.com/rest/2.0/face/v3/match?access_token=%s",token.toLocal8Bit().data());
    qDebug()<<"人脸信息上传--> URL:"<<quest_url;

    QString log = "人脸比对请求--> URL:";
    log += quest_url;
    add_to_log(log);

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

   // if(video_record_timer->isActive()) //如果开始录制视频则记录该视频
   // {
   //    cv::Mat videoframe = frame_data->clone();
   //    g_video_frame.append(videoframe);
   // }

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


void home_hotel::upload_info(char *authentication_flag) //上传身份证信息
{
    qDebug()<<QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")<<":上传身份证信息";
    upload_info_manager = new QNetworkAccessManager(this); //身份信息上传请求
    connect(upload_info_manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(upload_info_result(QNetworkReply*))); //连接槽

    QUrl qurl("http://hotel.inteink.com/hotel/religion/religionInfoGet");
    QNetworkRequest request(qurl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("application/json"));

    QByteArray quest_array("{\"card_img\":\"");
    quest_array.append(pic_IDcard);
    quest_array.append("\",\"face_img\":\"");
    quest_array.append(pic_Live);
    quest_array.append("\",\"id\":\"");
    quest_array.append(dev_id.toUtf8());
    quest_array.append("\",\"authentication\":\"");
    quest_array.append(authentication_flag);
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

    qDebug()<<"上传身份证信息---> URL:"<<qurl;
    upload_info_manager->post(request,quest_array);
    ui->help_msg_page3->setText("登记中,请稍后...");
    QString log = "上传身份证信息---> URL:";
    log += qurl.toString();
    add_to_log(log);
}

void home_hotel::add_to_log(QString log)
{
    QString fullpath = LOG_DIR_PATH;
    if(isDirExist(fullpath))  //文件夹存在才保存日志
    {
        QString year_month_day = QDateTime::currentDateTime().toString("yyyy-MM-dd"); //年月日
        fullpath += "/";
        fullpath += year_month_day;
        fullpath += ".log";

        QString hour_minutes_s_ms = QDateTime::currentDateTime().toString("HH:mm:ss zzz");   //时 分

        QString time_str = year_month_day + " " + hour_minutes_s_ms;
        QFile file(fullpath);
        if(!file.open(QIODevice::WriteOnly  | QIODevice::Text|QIODevice::Append))
        {
            qDebug()<<"打开日志文件失败!!";
            return;
        }
        QTextStream in(&file);
        time_str += "-->";
        time_str += log;
        time_str += "\n";
        in << time_str;
        file.close();
    }
}

bool home_hotel::isDirExist(QString fullPath)
{
    QDir dir(fullPath);
    if(dir.exists())
    {
      return true;
    }
    return false;
}

void home_hotel::upload_info_result(QNetworkReply *reply)
{
    qDebug()<<QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")<<":身份证信息上传完成";
    //QTextCodec *codec = QTextCodec::codecForName("utf8");
    QString Receive_http = reply->readAll();//codec->toUnicode(reply->readAll());

    qDebug()<<"上传身份信息回复-->"<<Receive_http;

    QString log = "上传身份信息回复---> Receive:";
    log += Receive_http;
    add_to_log(log);

    QJsonParseError err;
    QJsonDocument json_recv = QJsonDocument::fromJson(Receive_http.toUtf8(),&err);
    qDebug() << "error_code ="<<err.error;
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
                    qDebug()<<"获取数据成功";
                    add_to_log(QString("登记成功"));
                    ui->stackedWidget->setCurrentIndex(SIGN_SUCCESS_PAGE);
                }break;
                case 400:
                {
                    qDebug()<<"获取数据失败";
                    add_to_log(QString("登记失败"));
                    ui->stackedWidget->setCurrentIndex(SIGN_FAIL_PAGE);
                }break;
            default:
                qDebug()<<"未知返回码:"<<ret_code.toInt();
                ui->stackedWidget->setCurrentIndex(SIGN_FAIL_PAGE);
                break;
            }
        }else
        {
            qDebug()<<"未包含指定字段!!";
            ui->stackedWidget->setCurrentIndex(SIGN_FAIL_PAGE);
        }
    }else
    {
        qDebug()<<"Json格式有误!!";
        ui->stackedWidget->setCurrentIndex(SIGN_FAIL_PAGE);
    }
    close_camera(); //关闭摄像头


    disconnect(upload_info_manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(upload_info_result(QNetworkReply*))); //取消连接
    delete upload_info_manager;  //用完释放
    upload_info_manager = NULL;
    qDebug()<<"释放公用http请求句柄..";
}


void home_hotel::face_compare_result(QNetworkReply* reply)
{
    qDebug()<<QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")<<":人脸比对完成";
    QTextCodec *codec = QTextCodec::codecForName("utf8");
    QString Receive_http = codec->toUnicode(reply->readAll());

    qDebug()<<"Receive_http:"<<Receive_http;
    QString log = "人脸比对相关参数请求结果---> Receive:";
    log += Receive_http;
    add_to_log(log);

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
                            face_detect_flag = 0;
                            upload_info("a");//上传身份证信息
                            face_compare_try_times = 0;
                        }else
                        {
                            msg = "人证比对失败,请重试";
                            ui->help_msg_page3->setText(msg);
                            face_detect_flag = 1;
                            if(face_compare_try_times >= compare_threshold) //大于设置次数登记失败
                            {
                                upload_info("b");//上传身份证信息
                                face_detect_flag = 0;
                                face_compare_try_times = 0;
                            }
                            face_compare_try_times++;
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
        msg = "请求数据失败,请重试";
        ui->help_msg_page3->setText(msg);
        face_detect_flag = 1;
    }
    reply->deleteLater();//释放对象
}


void home_hotel::on_exit_clicked()  //退出按钮
{
    add_to_log(QString("返回首界面"));
    if(pthread_card->isRunning())
    {
        pthread_card->stop();
        pthread_card->wait();
    }
    if(camera->isOpened()) //如果视频已经打开则关闭视频
        close_camera();

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

void home_hotel::on_stackedWidget_currentChanged(int arg1)
{
    if(arg1 == FIRST_PAGE)
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
    }
    else
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
            page_shelving_timer->start(shelving_time);
        }

    }
}

void home_hotel::on_self_help_regist_clicked()
{
    ui->stackedWidget->setCurrentIndex(CARD_DETECT);
    add_to_log(QString("开始检测身份证"));
    pthread_card->start();
    open_camera();
}

void home_hotel::on_finish_clicked()
{
    on_exit_clicked();
}

void home_hotel::on_finish_fail_clicked()
{
    on_exit_clicked();
}
