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
#include <QSettings>
#include <QFileInfo>
#include <QFile>
#include <QImage>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/gpu/gpu.hpp>

#include "detect_card_pthread.h"

#define camera_T 20  //20ms获取一次图像
#define  API_Key       "6SUZQHmrbhK3f6Rfk799AN5C"
#define  Secret_Key    "LAc7GQf5eqstueUHbqND9tPgOaminD0e"
#define  FILE_PATH      "./cfg.ini"
enum {
      FIRST_PAGE=0, //首页面
      CARD_DETECT,  //身份证检测
      FACE_COMPARE, //人证对比
      PHONENUMBER_SIGN, //登记号码
      CHECK_IN_SUCCESS, //入住成功页面
      ASK_ERROR_PAGE,    //请求返回错误提示页面
      ROOM_INFO_PAGE
     };

enum {
      CHECK_IN=0, //入住
      ADD_PEOPLE, //增加人员
      GET_KEY,   //取钥匙
      CHECK_OUT,  //退房
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
    bool eventFilter(QObject *obj, QEvent *event);
    void read_ini_file();           //读取配置文件
    void home_hotel_init();         //参数初始化
    void signal_slots_connect();    //连接信号与槽
    void closeEvent(QCloseEvent *event);  //关闭事件
    void face_compare_show(); //人脸比较界面显示
    void open_camera();       //打开摄像头
    void close_camera();      //关闭摄像头
    QImage Mat2QImage(cv::Mat &cvImg); //Mat转QImage
    bool isFileExist(QString fullFileName); //判断文件是否存在
    QImage ScaleImage2Label(QImage qImage, QLabel *qLabel);     //使图片能适应Lable部件
    bool detectface(cv::Mat &image);     //检测人脸
    void request_token();           //获取token值
    void compare_face();            //人脸比对请求
    void upload_info();             //上传身份证信息
    void upload_info_add_people();             //增加人员上传身份证信息
    void ensure_check_in();                    // 确认登记
    void get_room_info_get_key();             //获取住房信息(取钥匙)
    void get_room_info_check_out();           //获取住房信息(退房)
    void get_key_request();                   //请求获取钥匙
    void check_out_request();                  //请求退房
    void ensure_change_telphone();              // 确认更换手机
public slots:
    void update_time();

    void weather_inquiry(); //请求天气

    void update_weather(QNetworkReply *reply); //更新天气

    void detect_card_finish();  //身份证检测完成

    void get_frame(); //从摄像头获取一帧图像

    void face_compare_result(QNetworkReply *reply); //获取人脸比对结果

    void upload_info_result(QNetworkReply *reply);  //上传入住信息结果

    void upload_info_add_people_result(QNetworkReply *reply); //添加同住人员

    void get_code_reply(QNetworkReply *reply); //获取验证码响应

    void get_code_timeout();  //获取验证码计时

    void get_room_info_get_key_reply(QNetworkReply *reply);  //获取住房信息回复(取钥匙)

    void get_room_info_check_out_reply(QNetworkReply *reply);  //获取住房信息回复(退房)

    void get_key_reply(QNetworkReply *reply);   //获取钥匙回复

    void check_out_reply(QNetworkReply *reply); //退房回复

    void ensure_check_in_reply(QNetworkReply *reply);  //确认入住

    void ensure_change_telphone_reply(QNetworkReply *reply); //确认更改手机

private slots:
    void on_check_in_clicked();

    void on_exit_clicked();

    void set_lineEdit_text(int opt_code);

    void on_num_clean_clicked();

    void on_num_0_clicked();

    void on_num_del_clicked();

    void on_num_1_clicked();

    void on_num_2_clicked();

    void on_num_3_clicked();

    void on_num_4_clicked();

    void on_num_5_clicked();

    void on_num_6_clicked();

    void on_num_7_clicked();

    void on_num_8_clicked();

    void on_num_9_clicked();

    void on_get_the_key_clicked();

    void on_check_out_clicked();

    void on_stackedWidget_currentChanged(int arg1); //页面切换

    void on_get_code_clicked();

    void on_ensure_sign_clicked();

    void on_add_people_clicked();

    void on_success_finish_clicked();

    void on_fail_finish_clicked();

    void on_room_info_bt1_clicked();

    void on_room_info_bt2_clicked();

private:
    Ui::home_hotel *ui;
    int focus_flag;   //-1 无焦点 0 电话号码  1 验证码
    int face_detect_flag;           //是否进行人脸识别标志
    int confidence_threshold;      //人脸比对置信度阈值
    int compare_threshold;      //比对失败次数阈值
    int opt_code;       //操作码
    int get_code_timer_count;

    QString identifying_code;           //验证码
    QString ordernumber;                //订单号
    QString local_city;   //当前所在城市

    QString token;          //百度接口参数值
    QString api_key;
    QString secret_key;

    QTimer *time_timer;            //时间定时器
    QTimer *weather_timer;         //天气定时器
    QTimer *frame_timer;           //视频帧获取定时器
    QTimer *common_timer;           //视频帧获取定时器

    QNetworkAccessManager *face_compare_manager;    //人脸比较
    QNetworkAccessManager *weather_manager;         //天气请求

    QNetworkAccessManager *common_manager;          //公用http请求句柄

    detect_card_pthread *pthread_card;  //身份证检测线程

    cv::VideoCapture *camera;       //视频获取对象
    cv::CascadeClassifier *ccf;      //创建脸部对象
    cv::Mat *frame_data;            //视频图像数据
    QImage *q_image_data;           //人脸数据

    QByteArray pic_IDcard;  //证件照 BASE64
    QByteArray pic_Live;    //生活照 BASE64
};

#endif // HOME_HOTEL_H
