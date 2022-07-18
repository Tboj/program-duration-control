
#include <stdio.h>
#include <Windows.h>
#include <stdbool.h>

#define SLEEP_TIME 5000 //间隔时间

#define FILE_PATH "D:\\my_log.txt" //信息输出文件

bool brun = false;
SERVICE_STATUS servicestatus;
SERVICE_STATUS_HANDLE hstatus;
int WriteToLog(char *str);
void WINAPI ServiceMain(int argc, char **argv);
void WINAPI CtrlHandler(DWORD request);
int InitService();

int WriteToLog(char *str)
{
    FILE *pfile;
    fopen_s(&pfile, FILE_PATH, "a+");
    if (pfile == NULL)
    {
        return -1;
    }

    fprintf_s(pfile, "%s\n", str);
    fclose(pfile);
    return 0;
}

void WINAPI ServiceMain(int argc, char **argv)
{

    servicestatus.dwServiceType = SERVICE_WIN32;
    servicestatus.dwCurrentState = SERVICE_START_PENDING;
    servicestatus.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_STOP; //在本例中只接受系统关机和停止服务两种控制命令

    servicestatus.dwWin32ExitCode = 0;
    servicestatus.dwServiceSpecificExitCode = 0;
    servicestatus.dwCheckPoint = 0;
    servicestatus.dwWaitHint = 0;
    hstatus = RegisterServiceCtrlHandler("testservice", CtrlHandler);
    if (hstatus == 0)
    {
        WriteToLog("RegisterServiceCtrlHandler failed");
        return;
    }

    WriteToLog("RegisterServiceCtrlHandler success");
    //向SCM 报告运行状态

    servicestatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(hstatus, &servicestatus);
    //下面就开始任务循环了，你可以添加你自己希望服务做的工作

    brun = true;
    MEMORYSTATUS memstatus;
    char str[100];
    memset(str, '\0', 100);
    while (brun)
    {
        // GlobalMemoryStatus(&memstatus);
        // int availmb = memstatus.dwAvailPhys / 1024 / 1024;
        // sprintf_s(str, 100, "available memory is %dMB", availmb);
        // WriteToLog(str);
        WriteToLog("service again");
        extern_this();
        Sleep(SLEEP_TIME);
    }

    WriteToLog("service stopped");
}

void WINAPI CtrlHandler(DWORD request)
{

    switch (request)
    {
    case SERVICE_CONTROL_STOP:

        brun = false;
        servicestatus.dwCurrentState = SERVICE_STOPPED;
        break;
    case SERVICE_CONTROL_SHUTDOWN:

        brun = false;
        servicestatus.dwCurrentState = SERVICE_STOPPED;
        break;
    default:
        break;
    }

    SetServiceStatus(hstatus, &servicestatus);
}

void main()
{
    SERVICE_TABLE_ENTRY entrytable[2];
    entrytable[0].lpServiceName = "testservice";
    entrytable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain;
    entrytable[1].lpServiceName = NULL;
    entrytable[1].lpServiceProc = NULL;
    StartServiceCtrlDispatcher(entrytable);
}
