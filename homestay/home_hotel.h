#ifndef HOME_HOTEL_H
#define HOME_HOTEL_H

#include <QWidget>
#include <QDebug>
#include <QMouseEvent>

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
    void home_hotel_init();
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

private:
    Ui::home_hotel *ui;
    int focus_flag;   //-1 无焦点 0 电话号码  1 验证码
};

#endif // HOME_HOTEL_H
