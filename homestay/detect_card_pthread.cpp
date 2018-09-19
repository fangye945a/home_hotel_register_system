#include "detect_card_pthread.h"


CARD_INFO card_info = {0};   //用于保存身份证信息

detect_card_pthread::detect_card_pthread()
{
    flagRunning = true;
}

void detect_card_pthread::run()  //线程函数
{
    flagRunning = true;
    qDebug()<<"身份证检测线程启动!!";
    mutex.lock();
    while(flagRunning)
    {
        mutex.unlock();
        cards_detect();
        msleep(CARD_DETECT_CYCLE);       //1s检测一次
        mutex.lock();
    }
    mutex.unlock();
    qDebug()<<"身份证检测线程关闭!!";
}

void detect_card_pthread::stop() //停止线程
{
    mutex.lock();
    flagRunning = false;
    mutex.unlock();
}

void detect_card_pthread::cards_detect()  //身份证检测
{
    unsigned char parase_buf[256]={0};
    QLibrary lib("Sdtapi");
    if (lib.load())  //加载动态库
    {
        InitFunction initComm =(InitFunction)lib.resolve("InitComm");
        AuthenticateFunction authenticate = (AuthenticateFunction)lib.resolve("Authenticate");
        ReadBaseMsgPhotoFunction readBaseMsg = (ReadBaseMsgPhotoFunction)lib.resolve("ReadBaseMsg");
        UninitFunction closeComm = (UninitFunction)lib.resolve("CloseComm");
        if(!initComm)
            qDebug()<<"initComm 解析失败!!";
        if(!authenticate)
            qDebug()<<"authenticate 解析失败!!";
        if(!readBaseMsg)
            qDebug()<<"readBaseMsg 解析失败!!";
        if(!closeComm)
            qDebug()<<"closeComm 解析失败!!";

        int ret = initComm(1001);  //端口初始化
        if(ret == 1)
        {
            ret= authenticate();
            if(ret)
            {
                memset(&card_info,0,sizeof(card_info));//先清空缓冲区再读取
                ret = readBaseMsg(parase_buf,NULL);
                if(ret > 0)
                {
                    memcpy(&card_info.name,parase_buf,31); //姓名
                    memcpy(&card_info.sex,&parase_buf[31],3); //性别
                    memcpy(&card_info.nation,&parase_buf[34],10); //名族
                    memcpy(&card_info.birth,&parase_buf[44],9); //出生日期
                    memcpy(&card_info.address,&parase_buf[53],71); //地址
                    memcpy(&card_info.card_id,&parase_buf[124],19); //姓名
                    memcpy(&card_info.registry,&parase_buf[143],31); //签发机关
                    emit detected();               //发送读取完毕标志
                    flagRunning = 0; //读取成功 退出线程
                }else
                    qDebug()<<"身份证检测--> 读取身份证失败!!";
            }else
                qDebug()<<"身份证检测--> 未检测到身份证...!!";
        }
        else
        {
             qDebug()<<"身份证检测--> 初始化失败!!";
        }
        closeComm();
        lib.unload();
    }
    else
    {
         qDebug()<<"身份证检测--> 加载库失败!!";
    }
}
