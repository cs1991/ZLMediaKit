#include <iostream>
#include <map>
#include <string>

#include "Util/logger.h"
using namespace std;

// 1 sdk��ʼ��ʧ��  2.sdk��¼ʧ�� 3.�ļ��д���ʧ�� 4.��ȡ�ļ�ʧ��  5.��ʼ���ص���ʧ�� 6.���ؽ��Ȼ�ȡʧ��
// 7 ֹͣ����ʧ��  8 �ļ�������
#define HK_ERR_INIT 1L 
#define HK_ERR_LOGIN 2L
#define HK_ERR_MKDIR 3L
#define HK_ERR_GETFILE 4L
#define HK_ERR_START 5L
#define HK_ERR_GETPROCESS 6L
#define HK_ERR_STOP 7L
#define HK_ERR_FILENOTFOUND 8L
#define HK_ERR_FINDFILE 9L  //�����ļ�ʧ��
#define HK_ERR_NORMAL 10L //�����ļ�ʧ��


struct LoginBean {
    string ip;
    string deviceId;
    string user;
    string pwd;
    int userId;
    int startChan;
    uint64_t time;
    LoginBean() {}
    LoginBean(
        const string &kip, const string &kdeviceId, const string &kpwd, const string &kuser, int &kuserId,
        int &kstartChan, uint64_t tmpTime)
        : ip(kip)
        , deviceId(kdeviceId)
        , user(kuser)
        , pwd(kpwd)
        , userId(kuserId)
        , startChan(kstartChan), time(tmpTime){};
};
class HcApi {
public:
    ~HcApi() {}
    HcApi(const HcApi &) = delete;
    HcApi &operator=(const HcApi &) = delete;
    static HcApi &get_instance() {
        static HcApi instance;
        return instance;
    }
    /*
    * ��ʼ��sdk
    */
    bool initSdk();

    bool loginDevice(
        const string &deviceId, const string &kUser, const string &kPwd, const string &kIp, const string &kPort);
    bool loginOut(string ip);
    int downloadFileByTime(
        const string &deviceId,const string &kUser, const string &kPwd, const string &kIp,
        const string &kPort, int channel, const string &start, const string &end, string &fileName, string &errorMsg,bool isRetry = false);

    //bool startRealPlay(const string& deviceId, int channel,int port);

    int findFileByTime(long loginId, int channel, const string &start, const string &end);

private:
    HcApi() {
        std::cout << "constructor called!" << std::endl;
        
    }
    std::map<string, LoginBean> loginMap;
    bool isInit = false; //sdk�Ƿ��ʼ��
    std::string m_recordPath;
};