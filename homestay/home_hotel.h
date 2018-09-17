#ifndef HOME_HOTEL_H
#define HOME_HOTEL_H

#include <QWidget>

namespace Ui {
class home_hotel;
}

class home_hotel : public QWidget
{
    Q_OBJECT

public:
    explicit home_hotel(QWidget *parent = 0);
    ~home_hotel();

private slots:
    void on_check_in_clicked();

    void on_exit_clicked();

private:
    Ui::home_hotel *ui;
};

#endif // HOME_HOTEL_H
