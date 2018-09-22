#ifndef VIDEO_UPDATE_H
#define VIDEO_UPDATE_H

#include <QThread>
#include <QString>
#include <QTimer>
#include <QList>
#include <QFile>
#include <QFileInfo>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QTextCodec>
#include <QDebug>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/video.hpp>
#include "detect_card_pthread.h"


#define VIDEO_FILE_NAME "./test.avi"



class VIDEO_UPDATE : public QThread
{
    Q_OBJECT
public:
    VIDEO_UPDATE();
    virtual void run();
    void stop();
    void upload_video(); //上传文件
    void create_video(); //创建视频
    void get_card_id();//获取身份证号码
public slots:
    void upload_finish(QNetworkReply* reply);


    void upload_timeout();
private:
    QNetworkAccessManager *upload_mannager;
    QTimer *upload_timer; //上传视频定时器
    QString cardid;

};

#endif // VIDEO_UPDATE_H
