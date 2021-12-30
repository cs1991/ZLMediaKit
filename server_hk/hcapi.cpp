#include "hcapi.h"
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include "Util/logger.h"
#include "Common/config.h"
#include "Util/File.h"
#if defined(ENABLE_RTPPROXY)
#include "Rtp/RtpServer.h"
#endif
#if !defined(_WIN32)
#include "include/linux/HCNetSDK.h"
#else
#include "include/win64/HCNetSDK.h"
#include "include/win64/plaympeg4.h"
#endif //! defined(_WIN32)

using namespace toolkit;
using namespace mediakit;
/**
* 海康sdk初始化
*/
bool HcApi::initSdk() {
    GET_CONFIG(string, recordPath, Record::kFilePath);
    m_recordPath = recordPath;
    if (isInit) {
        return true;
    }
    //先清空登录列表
    loginMap.clear();

    isInit = NET_DVR_Init();
    NET_DVR_SetLogToFile(3, "./sdkLog");
    //网络连接超时时间和连接尝试次数设置
    NET_DVR_SetConnectTime(10, 1);
    //设置重连功能。
    NET_DVR_SetReconnect(100, true);
    NET_DVR_SDKSTATE SDKState;
    NET_DVR_GetSDKState(&SDKState);
    if (isInit) {
        InfoL << "initSdk::ret=" << isInit << "   dwTotalLoginNum" << SDKState.dwTotalLoginNum;
    }
    return isInit;
}
bool HcApi::loginDevice(const string &deviceId, const string &kUser,const string &kPwd,const string & kIp,const string &kPort) {
    if (!isInit && !initSdk()) {
        InfoL << "loginDevice deviceId::" << deviceId << " sdk init error";
        return false;
    }
    if (loginMap.find(deviceId) != loginMap.end()) {
        return true;
    }
    NET_DVR_USER_LOGIN_INFO struLoginInfo = { 0 };
    NET_DVR_DEVICEINFO_V40 struDeviceInfoV40 = { 0 };
    struLoginInfo.bUseAsynLogin = false;
    
    struLoginInfo.wPort = atoi(kPort.c_str());
    memcpy(struLoginInfo.sDeviceAddress, kIp.c_str(), NET_DVR_DEV_ADDRESS_MAX_LEN);
    memcpy(struLoginInfo.sUserName, kUser.c_str(), NAME_LEN);
    memcpy(struLoginInfo.sPassword, kPwd.c_str(), NAME_LEN);

    int lUserID = NET_DVR_Login_V40(&struLoginInfo, &struDeviceInfoV40);

    if (lUserID < 0) {
        ErrorL << "loginDevice user::" << kUser << "   ip::" << kIp
              << "  login error code::" << NET_DVR_GetLastError()
              << "  msg::" << NET_DVR_GetErrorMsg();
        

        //释放sdk资源，在程序结束之前调用
        //NET_DVR_Cleanup();
        return false;
    }
    
    int startChan = struDeviceInfoV40.struDeviceV30.byChanNum > 0 ? struDeviceInfoV40.struDeviceV30.byStartChan-1
                                                                   : struDeviceInfoV40.struDeviceV30.byStartDChan-1;
    LoginBean bean(kIp, deviceId, kPwd, kUser, lUserID,startChan);                                                       
    loginMap.insert(pair<string, LoginBean>(deviceId,bean));
    
    InfoL << "loginDevice user::" << kUser << " ip::" << kIp << " startChan::" << bean.startChan << "   userid="<<lUserID;
    return true;
}

bool HcApi::loginOut(string deviceId) {
    if (loginMap.find(deviceId) != loginMap.end()) {
        LoginBean bean = loginMap[deviceId];

        if (NET_DVR_Logout(bean.userId)) {
            loginMap.erase(deviceId);
            return true;
        } else {
            return false;
        }

    } else {
        return true;
    }
}

bool HcApi::downloadFileByTime(
    const string &deviceId, const string &kUser, const string &kPwd, const string &kIp,
    const string &kPort, int channel, const string &start, const string &end, string &fileName, string &errorMsg) {
    if (!isInit && !initSdk()) {
        InfoL << "downloadFileByTime deviceId::" << deviceId << " sdk init error";
        return false;
    }
    if (loginMap.find(deviceId) == loginMap.end()) {
        //先登录
        bool ret = loginDevice(deviceId,kUser,kPwd,kIp,kPort);
        if (!ret) {
            ErrorL << "downloadFileByTime deviceId::" << deviceId << " login error";
            errorMsg = "设备登录失败:deviceId:"+deviceId;
            return false;
        }
    }
    //下载20211224000000
    NET_DVR_PLAYCOND struDownloadCond = { 0 };
    struDownloadCond.struStartTime.dwYear =    atoi(start.substr(0,4).c_str());
    struDownloadCond.struStartTime.dwMonth =   atoi(start.substr(4, 2).c_str());
    struDownloadCond.struStartTime.dwDay =     atoi(start.substr(6, 2).c_str());
    struDownloadCond.struStartTime.dwHour =    atoi(start.substr(8, 2).c_str());
    struDownloadCond.struStartTime.dwMinute =  atoi(start.substr(10, 2).c_str());
    struDownloadCond.struStartTime.dwSecond =  atoi(start.substr(12, 2).c_str());
    struDownloadCond.struStopTime.dwYear = atoi(end.substr(0, 4).c_str());
    struDownloadCond.struStopTime.dwMonth = atoi(end.substr(4, 2).c_str());
    struDownloadCond.struStopTime.dwDay = atoi(end.substr(6, 2).c_str());
    struDownloadCond.struStopTime.dwHour = atoi(end.substr(8, 2).c_str());
    struDownloadCond.struStopTime.dwMinute = atoi(end.substr(10, 2).c_str());
    struDownloadCond.struStopTime.dwSecond = atoi(end.substr(12, 2).c_str());
    LoginBean lbean = loginMap[deviceId];
    
    struDownloadCond.dwChannel = channel+lbean.startChan;
    struDownloadCond.byDrawFrame = 0;
    struDownloadCond.byStreamType = 0;
    struDownloadCond.byCourseFile = false;
    struDownloadCond.byVODFileType = 1;//保存mp4

    auto date = getTimeStr("%Y-%m-%d");
    string mp4FilePath = "hcdownload/" + date + "/";
    mp4FilePath = File::absolutePath(mp4FilePath, m_recordPath);
    if (File::create_path(mp4FilePath.c_str(), 0777) == -1) { //如果不存在就用mkdir函数来创建
        ErrorL << "downloadFileByTime ::deviceid:" << deviceId
               << "  创建文件夹   error";
        errorMsg = "创建文件夹失败:" + mp4FilePath;
        return false;
    }
    fileName = mp4FilePath + deviceId + "_" + to_string(channel) + "_" + start + "_0";
    mp4FilePath = mp4FilePath + deviceId + "_" + to_string(channel) + "_" + start + ".mp4";
   
    char szFileName[256] = { 0 };
    memcpy(szFileName, mp4FilePath.c_str(), mp4FilePath.length());
    int m_lLoadHandle = NET_DVR_GetFileByTime_V40(lbean.userId, szFileName, &struDownloadCond);
    if (m_lLoadHandle < 0) {
        ErrorL << "downloadFileByTime ::deviceid:" << deviceId << "    NET_DVR_GetFileByTime_V40 error,code=" << NET_DVR_GetLastError()
               << "  MSG::" << NET_DVR_GetErrorMsg();
        errorMsg = "获取文件失败";
        return false;
    }
    if (!NET_DVR_PlayBackControl(m_lLoadHandle, NET_DVR_PLAYSTART, 0, NULL)) {
        ErrorL << "downloadFileByTime ::deviceid:" << deviceId << "    NET_DVR_PlayBackControl error,code=" << NET_DVR_GetLastError()
               << "  MSG::" << NET_DVR_GetErrorMsg();
        errorMsg = "开始下载调用失败";
        return false;
    }
    int process = 0;
    while (process != 100) {
        process = NET_DVR_GetDownloadPos(m_lLoadHandle);
        if (process == -1) {
            ErrorL << "downloadFileByTime ::deviceid:" << deviceId
                   << "    NET_DVR_GetDownloadPos error,code=" << NET_DVR_GetLastError()
                   << "  MSG::" << NET_DVR_GetErrorMsg();
            NET_DVR_StopGetFile(m_lLoadHandle);
            errorMsg = "下载进度获取失败";
            return false;
        } else {
            usleep(500*1000);
        }
    }
    if (!NET_DVR_StopGetFile(m_lLoadHandle)) {
        ErrorL << "downloadFileByTime ::deviceid:" << deviceId
               << "    NET_DVR_StopGetFile error,code=" << NET_DVR_GetLastError()
               << "  MSG::" << NET_DVR_GetErrorMsg();
        return false;
    } 

    if (File::isFileExit((fileName + ".mp4").c_str()) == -1) {
        //文件不存在
        fileName = fileName + "_temp.mp4";
    } else {
        fileName = fileName + ".mp4";
    }
    if (File::isFileExit(fileName.c_str()) == -1 || File::fileSize(fileName.c_str()) <= 0) {
        errorMsg = "文件不存在";
        return false;
    }
    return true;

}
string byteToHexStr(unsigned char byte_arr[], int arr_len) {
    string hexstr;
    for (int i = 0; i < arr_len; i++) {
        char hex1;
        char hex2;
        int value = byte_arr[i]; //直接将unsigned char赋值给整型的值，系统会正动强制转换
        int v1 = value / 16;
        int v2 = value % 16;

        //将商转成字母
        if (v1 >= 0 && v1 <= 9)
            hex1 = (char)(48 + v1);
        else
            hex1 = (char)(55 + v1);

        //将余数转成字母
        if (v2 >= 0 && v2 <= 9)
            hex2 = (char)(48 + v2);
        else
            hex2 = (char)(55 + v2);

        //将字母连接成串
        hexstr = hexstr + hex1 + hex2;
    }
    return hexstr;
}
