#ifndef HOME_HOTEL_H
#define HOME_HOTEL_H

#include <QWidget>
#include <QDebug>
#include <QMouseEvent>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QTimer>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QCloseEvent>
#include <QTextCodec>
#include <QBuffer>
#include <QLabel>
#include <QFileInfo>
#include <QFile>
#include <QSettings>
#include <QImage>
#include <QDateTime>
#include <QDir>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>

#include "detect_card_pthread.h"
#include "video_update.h"


#define camera_T 20  //20ms获取一次图像
#define SHELVING_TIME  120000 //页面搁置时间
#define  API_Key       "2Z51fFjIkF1AM0ZTFwiZG0LI"
#define  Secret_Key    "rxW2gMF4TnIAAoyqedO8jnTL4h08jZbE"
#define  FILE_PATH      "./cfg.ini"
#define LOG_DIR_PATH    "./log"

enum {
      FIRST_PAGE=0, //首页面
      CARD_DETECT,  //身份证检测
      FACE_COMPARE, //人证对比
      SIGN_SUCCESS_PAGE, //登记成功提示页面
      SIGN_FAIL_PAGE    //登记失败提示页面
     };

namespace Ui {
class home_hotel;
}

class home_hotel : public QWidget
{
    Q_OBJECT

public:
    explicit home_hotel(QWidget *parent = 0);
    ~home_hotel();
    void read_ini_file();
    void home_hotel_init();         //参数初始化
    void signal_slots_connect();    //连接信号与槽
    void closeEvent(QCloseEvent *event);  //关闭事件
    void face_compare_show(); //人脸比较界面显示
    void open_camera();       //打开摄像头
    void close_camera();      //关闭摄像头
    QImage Mat2QImage(cv::Mat &cvImg); //Mat转QImage
    QImage ScaleImage2Label(QImage qImage, QLabel *qLabel);     //使图片能适应Lable部件
    bool detectface(cv::Mat &image);     //检测人脸
    void request_token();           //获取token值
    void compare_face();            //人脸比对请求
    void upload_info(char *authentication_flag);             //上传身份证信息
    void add_to_log(QString log);               //添加到日志
    bool isDirExist(QString fullPath);          //判断文件夹是否存在
    bool isFileExist(QString fullFileName);     //判断文件是否存在
public slots:
    void page_shelving_timeout();  //页面搁置超时

    void update_time();

    void weather_inquiry(); //请求天气

    void update_weather(QNetworkReply *reply); //更新天气

    void detect_card_finish();  //身份证检测完成

    void get_frame(); //从摄像头获取一帧图像

    void face_compare_result(QNetworkReply *reply); //获取人脸比对结果

    void upload_info_result(QNetworkReply *reply);  //上传身份证信息结果

    void start_upload_video(); //开始上传视频

private slots:
    void on_exit_clicked();

    void on_stackedWidget_currentChanged(int arg1); //页面切换

    void on_self_help_regist_clicked();

    void on_finish_clicked();

    void on_finish_fail_clicked();

private:
    Ui::home_hotel *ui;
    int face_detect_flag;           //人脸检测完成标志 
    int face_compare_try_times;     //人脸比对失败次数

    QString token;          //百度接口参数值
    int confidence_threshold;
    int compare_threshold; //人脸比对次数阈值
    int shelving_time;      //页面搁置超时时间
    QString local_city;   //当前所在城市
    QString dev_id;                     //设备ID
    QString api_key;
    QString secret_key;

    QTimer *page_shelving_timer;   //页面搁置定时器
    QTimer *time_timer;            //时间定时器
    QTimer *weather_timer;         //天气定时器
    QTimer *frame_timer;
    QTimer *video_record_timer;           //视频帧获取定时器

    QNetworkAccessManager *face_compare_manager;    //人脸比较
    QNetworkAccessManager *weather_manager;         //天气请求
    QNetworkAccessManager *upload_info_manager;         //上传人员信息

    detect_card_pthread *pthread_card;  //身份证检测线程
    VIDEO_UPDATE *video_upload; //视频压缩上传线程

    cv::VideoCapture *camera;       //视频获取对象
    cv::CascadeClassifier *ccf;      //创建脸部对象
    cv::Mat *frame_data;            //视频图像数据
    QImage *q_image_data;           //人脸数据

    QByteArray pic_IDcard;  //证件照 BASE64
    QByteArray pic_Live;    //生活照 BASE64
};

#endif // HOME_HOTEL_H
