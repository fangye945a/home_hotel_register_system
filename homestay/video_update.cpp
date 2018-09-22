#include "video_update.h"

extern QList<cv::Mat> g_video_frame;
extern CARD_INFO card_info;


#define BOUND "zhixiang"
VIDEO_UPDATE::VIDEO_UPDATE()
{
    get_card_id();  //记录身份证号码
}

void VIDEO_UPDATE::upload_timeout() //上传超时
{
    qDebug()<<"视频上传超时!!";
    stop();
}

void VIDEO_UPDATE::create_video()
{
    int picture_num = g_video_frame.length();
    double fps = 1.0*picture_num/5;
    qDebug()<<"fps = "<<fps;
    cv::VideoWriter capwrite;
    capwrite.open(VIDEO_FILE_NAME,8,fps,cv::Size(640,480),true);
    if(capwrite.isOpened())
    {
        while(!g_video_frame.isEmpty())
        {
            capwrite.write(g_video_frame.first());
            g_video_frame.removeFirst();
        }
    }
    capwrite.release();
    g_video_frame.clear();
}

void VIDEO_UPDATE::get_card_id()
{
    cardid = QString(card_info.card_id);
}

void VIDEO_UPDATE::run()
{
    qDebug()<<"开启视频上传线程!!";

    upload_mannager = new QNetworkAccessManager();  //新建QNetworkAccessManager对象
    connect(upload_mannager,SIGNAL(finished(QNetworkReply*)),this,SLOT(upload_finish(QNetworkReply*)));//关联信号和槽

    upload_timer = new QTimer();
    connect(upload_timer,SIGNAL(timeout()),this,SLOT(upload_timeout()));//关联信号和槽上传超时

    qDebug()<<"生成视频中..";
    create_video();//生成视频
    qDebug()<<"生成视频完成..";
    qDebug()<<"上传视频中...";
    upload_video(); //上传视频
    this->exec();

    upload_timer->stop(); //停止
    disconnect(upload_timer,SIGNAL(timeout()),this,SLOT(upload_timeout()));\
    disconnect(upload_mannager,SIGNAL(finished(QNetworkReply*)),this,SLOT(upload_finish(QNetworkReply*)));//关联信号和槽
    delete upload_timer;
    delete upload_mannager;
    qDebug()<<"结束视频上传线程!!";
}

void VIDEO_UPDATE::stop()
{
    qDebug()<<"上传超时...线程自动退出";
    g_video_frame.clear();//清空缓存视频
    this->quit();
}

void VIDEO_UPDATE::upload_video()   //上传视频
{
    QFile file(VIDEO_FILE_NAME);
    QFileInfo fileInfo(file);   //文件

    QString url_table = "http://hotel.inteink.com/hotel/religion/religionWav?card_id=";
    url_table += cardid;
    QUrl url(url_table);

    qDebug()<<"视频上传请求--> URL"<<url;
    QByteArray data;
    data.append("--" + QByteArray(BOUND) + "\r\n");
    data.append("Content-Disposition: form-data; name=\"file\";filename=\"");
    data.append(fileInfo.fileName().toUtf8());
    data.append("\"\r\n");
    data.append("Content-Type: video/avi\r\n\r\n");

    //将文件内容写到数据中
    if (file.open(QIODevice::ReadOnly))
    {
        data.append(file.readAll());   //let's read the file
        data.append("\r\n");
    }

    data.append("--" + QByteArray(BOUND) + "--\r\n");  //closing

    QNetworkRequest request(url);
    request.setRawHeader(QString("Content-Type").toLocal8Bit(),QString("multipart/form-data; boundary=" + QByteArray(BOUND)).toLocal8Bit());
    request.setRawHeader(QString("Content-Length").toLocal8Bit(),QString::number(data.length()).toLocal8Bit());

    upload_mannager->post(request,data);
    qDebug()<<"开始上传视频...";
    upload_timer->start(10000);
}

void VIDEO_UPDATE::upload_finish(QNetworkReply *reply) //上传完成
{
    QTextCodec *codec = QTextCodec::codecForName("utf8");
    QString Receive_http = codec->toUnicode(reply->readAll());
    qDebug()<<"视频上传回复:"<<Receive_http;
    reply->deleteLater();//释放对象
    this->quit();
}
