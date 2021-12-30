#include <iostream>
#include <map>
#include <string>

#include "Util/logger.h"
using namespace std;

struct LoginBean {
    string ip;
    string deviceId;
    string user;
    string pwd;
    int userId;
    int startChan;
    LoginBean() {}
    LoginBean(
        const string &kip, const string &kdeviceId, const string &kpwd, const string &kuser, int &kuserId,
        int &kstartChan)
        : ip(kip)
        , deviceId(kdeviceId)
        , user(kuser)
        , pwd(kpwd)
        , userId(kuserId)
        , startChan(kstartChan) {};
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
    bool downloadFileByTime(
        const string &deviceId,const string &kUser, const string &kPwd, const string &kIp,
        const string &kPort, int channel, const string &start, const string &end, string &fileName, string &errorMsg);

    //bool startRealPlay(const string& deviceId, int channel,int port);

private:
    HcApi() {
        std::cout << "constructor called!" << std::endl;
        
    }
    std::map<string, LoginBean> loginMap;
    bool isInit = false; //sdk是否初始化
    std::string m_recordPath;
};