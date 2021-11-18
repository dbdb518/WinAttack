#include "framework.h"
#include "WinAttack.h"
#include "tlhelp32.h"

#define MAX_LOADSTRING  100
#define ID_MAIN_LISTBOX 1000
#define ID_LOGVIEW 2000
#define ID_PRCESSINFO_LISTBOX 3000
#define ID_MODULE_LISTBOX  4000
#define ID_DLL_LISTBOX  5000
#define ID_SCAN_BUTTON  6000
#define ID_INJECT_BUTTON  7000
#define ID_EJECT_BUTTON  8000

#define TOPMARGIN   30
#define BOTTOMMARGIN   30
#define VMARGIN 10
#define HMARGIN 10
#define WLISTBOX    600
#define HLISTBOX    700
#define WLOGVIEW    1800
#define HLOGVIEW    200
#define HTEXT   13
#define HMODULELISTBOX    400
#define WMODULELISTBOX    400
#define HDLLLISTBOX    320
#define WDLLLISTBOX    200
#define BUTTONCORDX 1550
#define BUTTONCORDY 50
#define WBUTTON 200
#define HBUTTON 40
#define BUTTONMARGIN    20


// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.
HWND g_hWnd;
DWORD arPID[2048];                              // PID 배열
TCHAR *arProcName[2048];                        // 프로세스명 배열
BYTE *arModAddr[256];                           // 모듈주소 배열
TCHAR *arModName[2048];                         // 모듈명 배열
int processNum;                                 // 조회된 프로세스의 갯수
int moduleNum;                                  // 조회된 모듈의 갯수
SYSTEMTIME stime;
TCHAR szProcessNum[128];
TCHAR szDllDirInfo1[128];
TCHAR szDllDirInfo2[MAX_PATH];
TCHAR szProcPath[MAX_PATH];                     // 선택된 프로세스의 실행경로
DWORD dwInjectPID;                              // Inject된 대상 프로세스 아이디
TCHAR szInjectProcName[2048];                   // Inject된 대상 프로세스명
TCHAR szInjectDllName[MAX_PATH];                // Inject된 Dll 파일명

// 윈도우 핸들
HWND hMainListBox;
HWND hLogView;
HWND hModuleListBox;
HWND hDllListBox;
HWND hProcessInfoListBox;
HWND hScanButton;
HWND hInjectButton;
HWND hEjectButton;

// Function ProtoType
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
BOOL SetPrivilege(LPCTSTR);
void LogViewOutput(LPCTSTR, int);
void GetProcessList();
void GetModuleList(DWORD, LPTSTR);
void DrawMainListBox();
void DrawDllListBox();
void SetClientRect(HWND, int, int);
LPCTSTR GetMyFileName();
BOOL InjectDll();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 여기에 코드를 입력합니다.

    // 전역 문자열을 초기화합니다.
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WINATTACK, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 애플리케이션 초기화를 수행합니다:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINATTACK));

    MSG msg;

    // 기본 메시지 루프입니다:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  함수: MyRegisterClass()
//
//  용도: 창 클래스를 등록합니다.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINATTACK));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_WINATTACK);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   함수: InitInstance(HINSTANCE, int)
//
//   용도: 인스턴스 핸들을 저장하고 주 창을 만듭니다.
//
//   주석:
//
//        이 함수를 통해 인스턴스 핸들을 전역 변수에 저장하고
//        주 프로그램 창을 만든 다음 표시합니다.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  용도: 주 창의 메시지를 처리합니다.
//
//  WM_COMMAND  - 애플리케이션 메뉴를 처리합니다.
//  WM_PAINT    - 주 창을 그립니다.
//  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HFONT hLogFont;
    int index = 0;
    int i;

    switch (message)
    {
    case WM_CREATE:
        {
            g_hWnd = hWnd;

            //to-do : 메뉴를 구성한다 
            //        도움말을 보강한다 
            //        아이콘 만들기
            //        윈도우 데코레이션
            //        프로세스정보 뷰에 나올 내용을 추가
            //        프로세스 검색 기능을 추가
            //        특정 프로세스를 선택하면 다운되는 오류

            //로그뷰를 생성
            hLogView = CreateWindow(L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY, HMARGIN, TOPMARGIN + HLISTBOX + HTEXT + VMARGIN, WLOGVIEW, HLOGVIEW, hWnd, (HMENU)ID_LOGVIEW, hInst, NULL);

            //로그뷰에 폰트 적용
            hLogFont = CreateFont(14, 0, 0, 0, 0, 0, 0, 0, HANGEUL_CHARSET, 3, 2, 1, VARIABLE_PITCH | FF_ROMAN, L"휴먼고딕");
            SendMessage(hLogView, WM_SETFONT, (WPARAM)hLogFont, MAKELPARAM(FALSE, 0));

            //Privilege를 조정한다.
            LogViewOutput(L"Privilege 조정", 100);
            if (SetPrivilege(SE_DEBUG_NAME))
            {
                LogViewOutput(L"Privilege 조정", 1);
            }
            else
            {
                LogViewOutput(L"Privilege 조정", -1);
                //MessageBox(hWnd, L"Error Occured!\n\nThis Application must be Stopped Immediately!", L"Execution Error", MB_OK);
            }
            LogViewOutput(L"Privilege 조정", 900);

            //메인 리스트박스를 생성
            hMainListBox = CreateWindow(L"listbox", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY | WS_VSCROLL | WS_HSCROLL, HMARGIN, TOPMARGIN, WLISTBOX, HLISTBOX, hWnd, (HMENU)ID_MAIN_LISTBOX, hInst, NULL);

            //Module 리스트박스를 생성
            hModuleListBox = CreateWindow(L"listbox", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY | WS_VSCROLL | WS_HSCROLL, HMARGIN + WLISTBOX + HMARGIN, TOPMARGIN, WMODULELISTBOX, HMODULELISTBOX, hWnd, (HMENU)ID_MODULE_LISTBOX, hInst, NULL);

            //DLL 리스트박스를 생성
            hDllListBox = CreateWindow(L"listbox", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY | WS_VSCROLL | WS_HSCROLL, HMARGIN + WLISTBOX + HMARGIN + WMODULELISTBOX + HMARGIN * 10, TOPMARGIN, WDLLLISTBOX, HDLLLISTBOX, hWnd, (HMENU)ID_DLL_LISTBOX, hInst, NULL);

            //Process Info 리스트박스를 생성
            hProcessInfoListBox = CreateWindow(L"listbox", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY | WS_VSCROLL | WS_HSCROLL, HMARGIN + WLISTBOX + HMARGIN, TOPMARGIN + HMODULELISTBOX, WLOGVIEW - WLISTBOX - HMARGIN, HLISTBOX - HMODULELISTBOX - VMARGIN, hWnd, (HMENU)ID_PRCESSINFO_LISTBOX, hInst, NULL);

            //Scan 버튼을 생성
            hScanButton = CreateWindow(L"button", L"Scan", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, BUTTONCORDX, BUTTONCORDY, WBUTTON, HBUTTON, hWnd, (HMENU)ID_SCAN_BUTTON, hInst, NULL);

            //Inject 버튼을 생성
            hInjectButton = CreateWindow(L"button", L"Inject", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, BUTTONCORDX, BUTTONCORDY + HBUTTON + BUTTONMARGIN, WBUTTON, HBUTTON, hWnd, (HMENU)ID_INJECT_BUTTON, hInst, NULL);

            //Eject 버튼을 생성
            hEjectButton = CreateWindow(L"button", L"Eject", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, BUTTONCORDX, BUTTONCORDY + HBUTTON * 2 + BUTTONMARGIN * 2, WBUTTON, HBUTTON, hWnd, (HMENU)ID_EJECT_BUTTON, hInst, NULL);

            //윈도우 크기 조정
            int width = HMARGIN + WLOGVIEW + HMARGIN;
            int height = TOPMARGIN + HLISTBOX + HTEXT + VMARGIN + HLOGVIEW + BOTTOMMARGIN;

            SetClientRect(hWnd, width, height);

            //실행중인 프로세스 목록을 가져와서 배열에 넣어준다
            LogViewOutput(L"실행중인 프로세스 조회", 100);
            GetProcessList();
            LogViewOutput(L"실행중인 프로세스 조회", 900);

            //프로세스 목록을 배열에서 가져와서 메인 리스트박스에 보여줌
            LogViewOutput(L"프로세스 목록 출력", 100);
            DrawMainListBox();
            LogViewOutput(L"프로세스 목록 출력", 900);

            //현재 디렉토리의 DLL 목록을 DLL 리스트박스에 보여줌
            LogViewOutput(L"DLL 목록 출력", 100);
            DrawDllListBox();
            LogViewOutput(L"DLL 목록 출력", 900);
        }
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 메뉴 선택을 구문 분석합니다:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case ID_MAIN_LISTBOX:
                switch (HIWORD(wParam))
                {
                case LBN_SELCHANGE:
                    // DLL 뷰에 모듈목록을 보여줌
                    SendMessage(hModuleListBox, LB_RESETCONTENT, 0, 0);
                    index = SendMessage(hMainListBox, LB_GETCURSEL, 0, 0);

                    LogViewOutput(L"모듈 목록 조회", 100);
                    GetModuleList(arPID[index], arProcName[index]);
                    LogViewOutput(L"모듈 목록 조회", 900);

                    for (i = 0; i < moduleNum; i++)
                    {
                        // 모듈이 Inject된 dll일 경우의 처리
                        if (!lstrcmpi(arModName[i], szInjectDllName))
                        {
                            TCHAR s[2048];
                            lstrcpy(s, szInjectDllName);
                            lstrcat(s, L"       [ --- Module Injected --- ]");
                            SendMessage(hModuleListBox, LB_ADDSTRING, 0, (LPARAM)s);
                        }
                        else
                        {
                            SendMessage(hModuleListBox, LB_ADDSTRING, 0, (LPARAM)arModName[i]);
                        }
                    }

                    // 프로세스정보 뷰에 프로세스의 실행경로를 보여줌
                    SendMessage(hProcessInfoListBox, LB_RESETCONTENT, 0, 0);
                    SendMessage(hProcessInfoListBox, LB_ADDSTRING, 0, (LPARAM)szProcPath);
                    break;
                }
                break;
            case ID_SCAN_BUTTON:
                SendMessage(hMainListBox, LB_RESETCONTENT, 0, 0);
                SendMessage(hModuleListBox, LB_RESETCONTENT, 0, 0);
                SendMessage(hDllListBox, LB_RESETCONTENT, 0, 0);
                SendMessage(hProcessInfoListBox, LB_RESETCONTENT, 0, 0);

                LogViewOutput(L"실행중인 프로세스 조회", 100);
                GetProcessList();
                LogViewOutput(L"실행중인 프로세스 조회", 900);

                //프로세스 목록을 배열에서 가져와서 메인 리스트박스에 보여줌
                LogViewOutput(L"프로세스 목록 출력", 100);
                DrawMainListBox();
                LogViewOutput(L"프로세스 목록 출력", 900);

                //현재 디렉토리의 DLL 목록을 DLL 리스트박스에 보여줌
                LogViewOutput(L"DLL 목록 출력", 100);
                DrawDllListBox();
                LogViewOutput(L"DLL 목록 출력", 900);
                break;
            case ID_INJECT_BUTTON:
                LogViewOutput(L"DLL Injection", 100);
                if (InjectDll())
                {
                    LogViewOutput(L"DLL Injection", 900);

                    TCHAR s[1024];
                    wsprintf(s, L"DLL Injection 성공!\n\n\n\n대상 프로세스 : %s\n\n대상 프로세스의 PID : %d\n\nInjected DLL : %s", szInjectProcName, dwInjectPID, szInjectDllName);
                    MessageBox(g_hWnd, s, L"Notice", MB_OK);

                    //화면을 갱신함
                    SendMessage(hScanButton, BM_CLICK, (WPARAM)0, (LPARAM)0);
                }
                else LogViewOutput(L"DLL Injection", -1);
                break;
            case ID_EJECT_BUTTON:
                MessageBox(hWnd, L"Eject", L"Button", MB_OK);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            // 조회된 프로세스 갯수
            TextOutW(hdc, HMARGIN, TOPMARGIN + HLISTBOX, szProcessNum, lstrlen(szProcessNum));

            // DLL 리스트박스 정보(타이틀)
            TextOut(hdc, HMARGIN + WLISTBOX + HMARGIN + WMODULELISTBOX + HMARGIN * 8, TOPMARGIN + HDLLLISTBOX + 5, szDllDirInfo1, lstrlen(szDllDirInfo1));

            // DLL 리스트박스 정보(현재 디렉토리)
            TextOut(hdc, HMARGIN + WLISTBOX + HMARGIN + WMODULELISTBOX + HMARGIN * 8, TOPMARGIN + HDLLLISTBOX + 25, szDllDirInfo2, lstrlen(szDllDirInfo2));

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// 이 프로세스에 Privilege를 부여함
// 반환값 : 성공 - TRUE
//          실패 - FALSE
BOOL SetPrivilege(LPCTSTR lpszPriv)
{
    TOKEN_PRIVILEGES tp;
    HANDLE hToken;
    LUID luid;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
    {
        LogViewOutput(L"OpenProcessToken", -1);
        OutputDebugString(L"[Error]OpenProcessToken");
        return FALSE;
    }

    if (!LookupPrivilegeValue(NULL, lpszPriv, &luid))
    {
        LogViewOutput(L"LookupPrivilegeValue", -1); 
        OutputDebugString(L"[Error]LookupPrivilegeValue");
        return FALSE;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hToken,
                               FALSE,
                               &tp,
                               sizeof(TOKEN_PRIVILEGES),
                               (PTOKEN_PRIVILEGES)NULL,
                               (PDWORD)NULL))
    {
        LogViewOutput(L"AdjustTokenPrivileges", -1);
        OutputDebugString(L"[Error]AdjustTokenPrivileges");
        return FALSE;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
    {
        OutputDebugString(L"GetLastError() : ERROR_NOT_ALL_ASSIGNED");
        return FALSE;
    }

    return TRUE;
}

// 로그뷰에 메시지를 출력함
// 인수값 : 시작 : 100
//          성공 : 1
//          실패, 또는 에러 : -1
//          종료 : 900
void LogViewOutput(LPCTSTR lpszMsg, int code)
{
    TCHAR str[128];
    TCHAR strTime[128];

    GetLocalTime(&stime);

    if (code == 100)
        wsprintf(strTime, L"\r\n%d:%d:%d      ", stime.wHour, stime.wMinute, stime.wSecond);
    else
        wsprintf(strTime, L"%d:%d:%d      ", stime.wHour, stime.wMinute, stime.wSecond);

    switch (code)
    {
    case 100:
        lstrcpy(str, L"[시작] ");
        break;
    case 1:
        lstrcpy(str, L"[성공] ");
        break;
    case -1:
        lstrcpy(str, L"[에러] ");
        break;
    case 900:
        lstrcpy(str, L"[종료] ");
        break;
    }


    lstrcat(str, lpszMsg);
    lstrcat(str, L"\r\n");
    lstrcat(strTime, str);

    int textLength = GetWindowTextLength(hLogView);

    SendMessage(hLogView, EM_SETSEL, (WPARAM)textLength, (LPARAM)textLength);
    SendMessage(hLogView, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)strTime);
}

// 현재 실행중인 프로세스 목록을 가져와서 배열에 넣어줌
void GetProcessList()
{
    HANDLE hSnapshot;
    PROCESSENTRY32 pe = {sizeof(PROCESSENTRY32), };

    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);

    int i = 0;
    if (Process32First(hSnapshot, &pe))
    {
        do
        {
            if (_tcsicmp((LPCTSTR)GetMyFileName(), (LPCTSTR)pe.szExeFile)) // 내 프로세스명은 조회 안되도록 제외시킴
            {
                arPID[i] = pe.th32ProcessID;
                    arProcName[i] = (TCHAR*)malloc(sizeof(pe.szExeFile));
                    lstrcpy(arProcName[i], pe.szExeFile);
                    i++;
            }
        } while (Process32Next(hSnapshot, &pe));
    }

    processNum = i;

    CloseHandle(hSnapshot);
}

// PID를 입력받아 모듈 목록을 조회하여 배열에 넣어줌
void GetModuleList(DWORD dwPID, LPTSTR szProcName)
{
    HANDLE hSnapshot;
    TCHAR s[128];

    MODULEENTRY32 me = {sizeof(MODULEENTRY32), };

    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE32 | TH32CS_SNAPMODULE, dwPID);

    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        DWORD dwErr = GetLastError();
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, 
            dwErr,
            MAKELANGID(LANG_NEUTRAL, 
            SUBLANG_DEFAULT),
            s, 256, NULL);

        OutputDebugString(s);
        wsprintf(s, L"dwErr=%d", dwErr);
        OutputDebugString(s);
    }

    int i = 0;
    if (Module32First(hSnapshot, &me))
    {
        do
        {
            if (_tcsicmp((LPCTSTR)me.szModule, (LPCTSTR)szProcName))  // 내 프로세스명과 다른 값은 데이터만 나열함
            {
                arModName[i] = (TCHAR*)malloc(sizeof(me.szModule));
                lstrcpy(arModName[i], me.szModule);
                arModAddr[i] = me.modBaseAddr;
                i++;
            }
            else // 내 프로세스명과 같은 값이 나오면 PRCESSINFO VIEW에 전체경로를 보여줌
            {
                lstrcpy(szProcPath, me.szExePath);
            }
        } while (Module32Next(hSnapshot, &me));
    }

    moduleNum = i;
    wsprintf(s, L"moduleNum=%d", moduleNum);
    OutputDebugString(s);
 
    CloseHandle(hSnapshot);
}

// 프로세스 목록을 배열에서 가져와서 메인 리스트박스에 보여줌
void DrawMainListBox()
{
    int i;
    for (i = 0; i < processNum; i++)
    {
            TCHAR output[128];

            wsprintf(output, L"%s  [%d]", arProcName[i], arPID[i]);
            SendMessage(hMainListBox, LB_ADDSTRING, 0, (LPARAM)output);
    }

    wsprintf(szProcessNum, L"Processes : %d", processNum);
    InvalidateRect(g_hWnd, NULL, TRUE);
}

//현재 디렉토리의 Dll 목록을 가져와서 Dll 리스트박스에 보여줌
void DrawDllListBox()
{
    SendMessage(hDllListBox, LB_DIR, (WPARAM)DDL_READWRITE, (LPARAM)L"*.dll");

    lstrcpy(szDllDirInfo1, L"[Current Directory]");
    GetCurrentDirectory(MAX_PATH, szDllDirInfo2);
    InvalidateRect(g_hWnd, NULL, TRUE);
}

void SetClientRect(HWND hWnd, int w, int h)
{
    RECT crt;
    DWORD Style, ExStyle;

    SetRect(&crt, 0, 0, w, h);
    Style = GetWindowLong(hWnd, GWL_STYLE);
    ExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);

    AdjustWindowRectEx(&crt, Style, GetMenu(hWnd) != NULL, ExStyle);

    if (Style & WS_VSCROLL) crt.right += GetSystemMetrics(SM_CXVSCROLL);
    if (Style & WS_HSCROLL) crt.bottom += GetSystemMetrics(SM_CYVSCROLL);

    SetWindowPos(hWnd, NULL, 0, 0, crt.right - crt.left, crt.bottom - crt.top, SWP_NOMOVE | SWP_NOZORDER);
}

LPCTSTR GetMyFileName()
{
    TCHAR szFileName[MAX_PATH];

    if (!GetModuleFileName(NULL, szFileName, MAX_PATH)) return NULL;

    TCHAR* p = _tcsrchr(szFileName, '\\');

    if (p == NULL) NULL;

    return p + 1;
}

BOOL InjectDll()
{
    int indexMain = -1;
    int indexDll = -1;

    DWORD dwPID = NULL;  // 해킹될 프로세스의 pid
    TCHAR szProcName[2048];  // 해킹될 프로세스의 프로세스명
    TCHAR szDllName[MAX_PATH]; //inject할 DLL의 파일명
    TCHAR szDllPath[MAX_PATH]; //inject할 DLL의 전체경로

    HANDLE hProcess = NULL;  // 해킹될 프로세스의 핸들
    LPVOID pMemAlloc = NULL; // 해킹될 프로세스의 새로 할당된 메모리 영역

    HMODULE hMod = NULL;    // Handle of kernel32.dll
    LPTHREAD_START_ROUTINE pThreadProc; // Address of LoadLibraryW
    HANDLE hRemoteThread = NULL;   // Handle of CreateRemoteThread

    indexMain = SendMessage(hMainListBox, LB_GETCURSEL, 0, 0);

    if (indexMain == -1)
    {
        MessageBox(g_hWnd, L"DLL을 Inject할 프로세스를 선택하세요.", L"Error", MB_OK);
        return FALSE;
    }
    else
    {
        indexDll = SendMessage(hDllListBox, LB_GETCURSEL, 0, 0);

        if (indexDll == -1)
        {
            MessageBox(g_hWnd, L"Inject할 DLL을 선택하세요.", L"Error", MB_OK);
            return FALSE;
        }
        else
        {
            dwPID = arPID[indexMain]; // 대상 프로세스의 pid를 셋팅
            lstrcpy(szProcName, arProcName[indexMain]);   // 대상 프로세스의 프로세스명을 셋팅

            DlgDirSelectEx(g_hWnd, szDllName, MAX_PATH, ID_DLL_LISTBOX); // inject할 DLL의 파일명을 셋팅
            GetCurrentDirectory(MAX_PATH, szDllPath); //현재 디렉토리 전체경로를 셋팅

            lstrcat(szDllPath, L"\\");
            lstrcat(szDllPath, szDllName);

            //TCHAR s[128];
            //wsprintf(s, L"dwPID=%d, szDllPath=%s", dwPID, szDllPath);
            //OutputDebugString(s);
        }
    }

    // Injection 시작
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);

    if (!hProcess)
    {
        OutputDebugString(L"InjectDll Error : OpenProcess Call");
        return FALSE;
    }

    DWORD dwAllocSize = (DWORD)(_tcslen(szDllPath) + 1) * sizeof(TCHAR);

    pMemAlloc = VirtualAllocEx(hProcess, NULL, dwAllocSize, MEM_COMMIT, PAGE_READWRITE);

    if (!pMemAlloc)
    {
        OutputDebugString(L"InjectDll Error : VirtualAllocEx Call");
        TCHAR s[128];
        DWORD dwErr = GetLastError();
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dwErr,
            MAKELANGID(LANG_NEUTRAL,
                SUBLANG_DEFAULT),
            s, 256, NULL);

        OutputDebugString(s);
        return FALSE;
    }

    if (!WriteProcessMemory(hProcess, pMemAlloc, (LPVOID)szDllPath, dwAllocSize, NULL))
    {
        OutputDebugString(L"InjectDll Error : WriteProcessMemory Call");
        TCHAR s[128];
        DWORD dwErr = GetLastError();
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dwErr,
            MAKELANGID(LANG_NEUTRAL,
                SUBLANG_DEFAULT),
            s, 256, NULL);

        OutputDebugString(s);
        return FALSE;
    }

    hMod = GetModuleHandle(L"kernel32.dll");

    pThreadProc = (LPTHREAD_START_ROUTINE)GetProcAddress(hMod, "LoadLibraryW");

    if (!pThreadProc)
    {
        OutputDebugString(L"InjectDll Error : GetProcAddress Call");
        TCHAR s[128];
        DWORD dwErr = GetLastError();
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dwErr,
            MAKELANGID(LANG_NEUTRAL,
                SUBLANG_DEFAULT),
            s, 256, NULL);

        OutputDebugString(s);
        return FALSE;
    }

    hRemoteThread = CreateRemoteThread(hProcess,
                                       NULL,
                                       0,
                                       pThreadProc,
                                       pMemAlloc,
                                       0,
                                       NULL);

    if (!hRemoteThread)
    {
        OutputDebugString(L"InjectDll Error : CreateRemoteThread Call");
        TCHAR s[128];
        DWORD dwErr = GetLastError();
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dwErr,
            MAKELANGID(LANG_NEUTRAL,
                SUBLANG_DEFAULT),
            s, 256, NULL);

        OutputDebugString(s);
        return FALSE;
    }

    WaitForSingleObject(hRemoteThread, INFINITE);

    dwInjectPID = dwPID;
    lstrcpy(szInjectProcName, szProcName);
    lstrcpy(szInjectDllName, szDllName);

    CloseHandle(hRemoteThread);
    CloseHandle(hProcess);

    return TRUE;
}