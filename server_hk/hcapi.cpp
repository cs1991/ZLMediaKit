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
* ����sdk��ʼ��
*/
bool HcApi::initSdk() {
    GET_CONFIG(string, recordPath, Record::kFilePath);
    m_recordPath = recordPath;
    if (isInit) {
        return true;
    }
    //����յ�¼�б�
    loginMap.clear();

    isInit = NET_DVR_Init();
    NET_DVR_SetLogToFile(3, "./sdkLog");
    //�������ӳ�ʱʱ������ӳ��Դ�������
    NET_DVR_SetConnectTime(10, 1);
    //�����������ܡ�
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
        

        //�ͷ�sdk��Դ���ڳ������֮ǰ����
        //NET_DVR_Cleanup();
        return false;
    }
    
    int startChan = struDeviceInfoV40.struDeviceV30.byChanNum > 0 ? struDeviceInfoV40.struDeviceV30.byStartChan-1
                                                                   : struDeviceInfoV40.struDeviceV30.byStartDChan-1;
    LoginBean bean(kIp, deviceId, kPwd, kUser, lUserID, startChan, getCurrentMillisecond());                                                       
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

//1 sdk��ʼ��ʧ��  2.sdk��¼ʧ�� 3.�ļ��д���ʧ�� 4.��ȡ�ļ�ʧ��  5.��ʼ���ص���ʧ�� 6.���ؽ��Ȼ�ȡʧ��
//7 ֹͣ����ʧ��  8 �ļ�������
int HcApi::downloadFileByTime(
    const string &deviceId, const string &kUser, const string &kPwd, const string &kIp,
    const string &kPort, int channel, const string &start, const string &end, string &fileName, string &errorMsg, bool isRetry) {
    if (!isInit && !initSdk()) {
        InfoL << "downloadFileByTime deviceId::" << deviceId << " sdk init error";
        return HK_ERR_INIT;
    }
    if (loginMap.find(deviceId) == loginMap.end()) {
        //�ȵ�¼
        bool ret = loginDevice(deviceId,kUser,kPwd,kIp,kPort);
        if (!ret) {
            ErrorL << "downloadFileByTime deviceId::" << deviceId << " login error";
            errorMsg = "�豸��¼ʧ��:deviceId:"+deviceId;
            return HK_ERR_LOGIN;
        }
    }
    //����20211224000000
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
    if (isRetry) {
        int second = atoi(end.substr(12, 2).c_str());
        int minute = atoi(end.substr(10, 2).c_str());
        if (second + 5 > 59) {
            struDownloadCond.struStopTime.dwMinute = 59;
            minute = minute + 1;
            struDownloadCond.struStopTime.dwMinute = minute > 59 ? 59 : minute;
        } else {
            struDownloadCond.struStopTime.dwMinute = second+5;
            struDownloadCond.struStopTime.dwMinute = minute;
        }
    } else {
        struDownloadCond.struStopTime.dwMinute = atoi(end.substr(10, 2).c_str());
        struDownloadCond.struStopTime.dwSecond = atoi(end.substr(12, 2).c_str());
    }
    
    LoginBean lbean = loginMap[deviceId];
    
    struDownloadCond.dwChannel = channel+lbean.startChan;
    struDownloadCond.byDrawFrame = 0;
    struDownloadCond.byStreamType = 0;
    struDownloadCond.byCourseFile = false;
    struDownloadCond.byVODFileType = 1;//����mp4

    auto date = getTimeStr("%Y-%m-%d");
    string mp4FilePath = "hcdownload/" + date + "/";
    mp4FilePath = File::absolutePath(mp4FilePath, m_recordPath);
    if (File::create_path(mp4FilePath.c_str(), 0777) == -1) { //��������ھ���mkdir����������
        ErrorL << "downloadFileByTime ::deviceid:" << deviceId
               << "  �����ļ���   error";
        errorMsg = "�����ļ���ʧ��:" + mp4FilePath;
        return HK_ERR_MKDIR;
    }
    fileName = mp4FilePath + deviceId + "_" + to_string(channel) + "_" + start + "_0";
    mp4FilePath = mp4FilePath + deviceId + "_" + to_string(channel) + "_" + start + ".mp4";
   
    char szFileName[256] = { 0 };
    memcpy(szFileName, mp4FilePath.c_str(), mp4FilePath.length());
    int m_lLoadHandle = NET_DVR_GetFileByTime_V40(lbean.userId, szFileName, &struDownloadCond);
    if (m_lLoadHandle < 0) {
        ErrorL << "downloadFileByTime ::deviceid:" << deviceId << "    NET_DVR_GetFileByTime_V40 error,code=" << NET_DVR_GetLastError()
               << "  MSG::" << NET_DVR_GetErrorMsg();
        errorMsg = "downloadFileByTime from nvr error:msg:" + string(NET_DVR_GetErrorMsg());
        return HK_ERR_GETFILE;
    }
    if (!NET_DVR_PlayBackControl(m_lLoadHandle, NET_DVR_PLAYSTART, 0, NULL)) {
        ErrorL << "downloadFileByTime ::deviceid:" << deviceId << "    NET_DVR_PlayBackControl error,code=" << NET_DVR_GetLastError()
               << "  MSG::" << NET_DVR_GetErrorMsg();
        errorMsg = "NET_DVR_PlayBackControl from nvr error:msg:" + string(NET_DVR_GetErrorMsg());
        return HK_ERR_START;
    }
    int process = 0;
    while (process != 100) {
        if (File::fileSize((fileName + "_tmp.mp4").c_str()) > 1572864) {//>1.5M
            ErrorL << "downloadFileByTime ::deviceid:" << deviceId
                   << "    NET_DVR_StopGetFile file size too lager to aoto stop";
            break;
        }
        process = NET_DVR_GetDownloadPos(m_lLoadHandle);
        if (process == -1) {
            ErrorL << "downloadFileByTime ::deviceid:" << deviceId
                   << "    NET_DVR_GetDownloadPos error,code=" << NET_DVR_GetLastError()
                   << "  MSG::" << NET_DVR_GetErrorMsg();
            NET_DVR_StopGetFile(m_lLoadHandle);
            errorMsg = "NET_DVR_GetDownloadPos from nvr error:msg:" + string(NET_DVR_GetErrorMsg());
            return HK_ERR_GETPROCESS;
        } else {
            usleep(100*1000);
        }
    }
    if (!NET_DVR_StopGetFile(m_lLoadHandle)) {
        ErrorL << "downloadFileByTime ::deviceid:" << deviceId
               << "    NET_DVR_StopGetFile error,code=" << NET_DVR_GetLastError()
               << "  MSG::" << NET_DVR_GetErrorMsg();
        errorMsg = "NET_DVR_StopGetFile from nvr error:msg:" + string(NET_DVR_GetErrorMsg());
        return HK_ERR_STOP;
    } 

    if (File::isFileExit((fileName + ".mp4").c_str()) == -1) {
        //�ļ�������
        fileName = fileName + "_tmp.mp4";
    } else {
        fileName = fileName + ".mp4";
    }
    if (File::isFileExit(fileName.c_str()) == -1 || File::fileSize(fileName.c_str()) <= 0) {
        int ret = findFileByTime(lbean.userId, channel + lbean.startChan, start, end);
        if (ret == 0) {//�ļ����ڣ�ȥ��������
            if (!isRetry) {
                return downloadFileByTime(
                    deviceId, kUser, kPwd, kIp, kPort, channel, start, end, fileName, errorMsg, true);
            } else {
                errorMsg = "down from nvr over but file not exit";
                return HK_ERR_NORMAL;//��������
            }
        } else if (ret == HK_ERR_FINDFILE) { //�ļ�����ʧ��
            errorMsg = "find file from nvr error";
            return HK_ERR_FINDFILE;
        } else if (ret == HK_ERR_FILENOTFOUND) {//�ļ�δ�ҵ�
            errorMsg = "file not found from nvr";
            return HK_ERR_FILENOTFOUND;
        }
    }
    if (getCurrentMillisecond() - lbean.time > 60 * 60 * 1000) //����15���ӣ����µ�¼
    {
        InfoL << "login too long to stop";
        loginOut(deviceId);
    }
    return 0;

}

 int HcApi::findFileByTime(long loginId, int channel, const string &start, const string &end) {
     NET_DVR_FILECOND_V40 m_struFileCond;
     memset(&m_struFileCond, 0, sizeof(NET_DVR_FILECOND_V40));
     m_struFileCond.lChannel = channel;
     m_struFileCond.dwFileType = 0;//��ʱ¼��
     m_struFileCond.struStartTime.dwYear = atoi(start.substr(0, 4).c_str());
     m_struFileCond.struStartTime.dwMonth = atoi(start.substr(4, 2).c_str());
     m_struFileCond.struStartTime.dwDay = atoi(start.substr(6, 2).c_str());
     m_struFileCond.struStartTime.dwHour = atoi(start.substr(8, 2).c_str());
     m_struFileCond.struStartTime.dwMinute = atoi(start.substr(10, 2).c_str());
     m_struFileCond.struStartTime.dwSecond = atoi(start.substr(12, 2).c_str());
     m_struFileCond.struStopTime.dwYear = atoi(end.substr(0, 4).c_str());
     m_struFileCond.struStopTime.dwMonth = atoi(end.substr(4, 2).c_str());
     m_struFileCond.struStopTime.dwDay = atoi(end.substr(6, 2).c_str());
     m_struFileCond.struStopTime.dwHour = atoi(end.substr(8, 2).c_str());
     m_struFileCond.struStopTime.dwMinute = atoi(end.substr(10, 2).c_str());
     m_struFileCond.struStopTime.dwSecond = atoi(end.substr(12, 2).c_str());
     m_struFileCond.byStreamType
         = 0xfe; //˫��������(���ȷ���������¼��û��������¼��ʱ����������¼��)����ֵ���ᷢ���豸��SDK�ڲ�����
     m_struFileCond.byQuickSearch = 1; //���ٲ���
 
     long fineId = NET_DVR_FindFile_V40(loginId, &m_struFileCond);
     if (fineId < 0) {
         ErrorL << "Fail to get file list channel=" << channel << " start=" << start << "   end=" << end << "   code="<<
             NET_DVR_GetLastError() << "  MSG::" << NET_DVR_GetErrorMsg();;
         return HK_ERR_FINDFILE;
     }
     NET_DVR_FINDDATA_V50 struFileInfo = { 0 };
     LONG lRet = 0;
     long ret = 0;
     while (true) {
         lRet = NET_DVR_FindNextFile_V50(fineId, &struFileInfo);
         if (lRet == NET_DVR_FILE_SUCCESS) {
             ret = 0;
             break;
         } else if (lRet == NET_DVR_FILE_NOFIND) {
             ret = HK_ERR_FILENOTFOUND;
             break;
         } else if (lRet == NET_DVR_ISFINDING) {
             continue;
         } else {
             ret = HK_ERR_FINDFILE;
             ErrorL << "NET_DVR_FindNextFile_V50 HK_ERR_FINDFILE::";
             break;
         }
         usleep(100*1000);
     }
     NET_DVR_FindClose_V30(fineId);//�ر��ļ����Ҿ��
     return ret;
 }
string byteToHexStr(unsigned char byte_arr[], int arr_len) {
    string hexstr;
    for (int i = 0; i < arr_len; i++) {
        char hex1;
        char hex2;
        int value = byte_arr[i]; //ֱ�ӽ�unsigned char��ֵ�����͵�ֵ��ϵͳ������ǿ��ת��
        int v1 = value / 16;
        int v2 = value % 16;

        //����ת����ĸ
        if (v1 >= 0 && v1 <= 9)
            hex1 = (char)(48 + v1);
        else
            hex1 = (char)(55 + v1);

        //������ת����ĸ
        if (v2 >= 0 && v2 <= 9)
            hex2 = (char)(48 + v2);
        else
            hex2 = (char)(55 + v2);

        //����ĸ���ӳɴ�
        hexstr = hexstr + hex1 + hex2;
    }
    return hexstr;
}
