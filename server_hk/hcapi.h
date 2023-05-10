#include <iostream>
#include <map>
#include <string>

#include "Util/logger.h"
using namespace std;

// 1 sdk初始化失败  2.sdk登录失败 3.文件夹创建失败 4.获取文件失败  5.开始下载调用失败 6.下载进度获取失败
// 7 停止下载失败  8 文件不存在
#define HK_ERR_INIT 1L 
#define HK_ERR_LOGIN 2L
#define HK_ERR_MKDIR 3L
#define HK_ERR_GETFILE 4L
#define HK_ERR_START 5L
#define HK_ERR_GETPROCESS 6L
#define HK_ERR_STOP 7L
#define HK_ERR_FILENOTFOUND 8L
#define HK_ERR_FINDFILE 9L  //查找文件失败
#define HK_ERR_NORMAL 10L //查找文件失败


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
    * 初始化sdk
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
    bool isInit = false; //sdk是否初始化
    std::string m_recordPath;
};