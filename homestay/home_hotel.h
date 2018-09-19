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

#include "detect_card_pthread.h"

enum {
      FIRST_PAGE=0, //首页面
      CARD_DETECT,  //身份证检测
      FACE_COMPARE, //人证对比
      PHONENUMBER_SIGN, //登记号码
      ASK_ERROR_PAGE    //请求返回错误提示页面
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
    void home_hotel_init();         //参数初始化
    void signal_slots_connect();    //连接信号与槽
    void closeEvent(QCloseEvent *event);
public slots:
    void update_time();

    void weather_inquiry(); //请求天气

    void update_weather(QNetworkReply *reply); //更新天气

    void detect_card_finish();  //身份证检测完成
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

private:
    Ui::home_hotel *ui;
    int focus_flag;   //-1 无焦点 0 电话号码  1 验证码
    QString local_city;   //当前所在城市

    QTimer *time_timer;            //时间定时器
    QTimer *weather_timer;         //天气定时器

    QNetworkAccessManager *face_compare_manager;    //人脸比较
    QNetworkAccessManager *weather_manager;         //天气请求

    detect_card_pthread *pthread_card;
};

#endif // HOME_HOTEL_H
