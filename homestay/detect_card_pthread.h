#ifndef DETECT_CARD_PTHREAD_H
#define DETECT_CARD_PTHREAD_H

#include <QString>
#include <QThread>
#include <QMutex>
#include <QLibrary>
#include <QDebug>

#define CARD_DETECT_CYCLE  1000 //卡片检测周期1s

typedef __stdcall int(*InitFunction)(int iPort);
typedef __stdcall int(*AuthenticateFunction)(void);
typedef __stdcall int(*ReadBaseMsgPhotoFunction)(unsigned char *pMsg, int *len);
typedef __stdcall int(*UninitFunction)(void);

typedef struct _CARD_INFO
{
    char address[128]; //住址
    char name[32];   //姓名
    char card_id[32];    //身份证
    char registry[32];   //签发机关
    char nation[16]; //民族
    char birth[16]; //出生日期
    char sex[4];     //性别
}CARD_INFO; //身份证信息

class detect_card_pthread : public QThread
{
    Q_OBJECT
public:
    detect_card_pthread();
    virtual void run();
    void stop();
    void cards_detect();
signals:
    void detected();

private:
    bool flagRunning;
    QMutex mutex;
};

#endif // DETECT_CARD_PTHREAD_H
