// player.cpp : 定义应用程序的入口点。
//
// C 运行时头文件
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <string>

#include <InitGuid.h>
#include <uuids.h>
#include <Shobjidl.h>  

#include "player.h"
#include "player_interface.h"
#include <CommCtrl.h>
#define MAX_LOADSTRING 100
#define STATEOPEN WM_USER + 0x100

// 全局变量:
HINSTANCE hInst;								// 当前实例
TCHAR szTitle[MAX_LOADSTRING];					// 标题栏文本
TCHAR szWindowClass[MAX_LOADSTRING];			// 主窗口类名
HMENU menu = NULL;
TPlayerState state = psReady;
HMODULE dll = NULL;
ICorePlayer* player = NULL;
IPlayerConfig* playerConfig = NULL;
HWND control = NULL;
TAudioInfo info = {0};
std::wstring filePath;


#if (_MSC_VER >= 1600)
ITaskbarList3* itl = NULL;  
#endif

#define AUDIOMENUIDBASE 0x1000
int outputCount = 0;
int audioRender = AUDIOMENUIDBASE;
int videoRender = ID_VMR7WINDOWNED;
BOOL slidKeyDown = FALSE;

// {203C3DAB-212B-42d4-8DE5-23EC204C1251}
DEFINE_GUID(IID_IPlayerConfig, 
            0x203c3dab, 0x212b, 0x42d4, 0x8d, 0xe5, 0x23, 0xec, 0x20, 0x4c,
            0x12, 0x51);

// 此代码模块中包含的函数的前向声明:
ATOM				MyRegisterClass(HINSTANCE hInstance);
HWND				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK Control(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: 在此放置代码。
	MSG msg;
	HACCEL hAccelTable;

	// 初始化全局字符串
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_PLAYER, szWindowClass, MAX_LOADSTRING);

	MyRegisterClass(hInstance);
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED); // 由于播放器程用到COM，所以这里一定要初始化
	// 执行应用程序初始化:
    HWND hWnd = InitInstance (hInstance, nCmdShow);
	if (!hWnd)
		return FALSE;

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PLAYER));

    control = CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_CONTROL), hWnd, Control, 0);
    SendMessage(GetDlgItem(control, IDC_SLIDER1), TBM_SETRANGE, TRUE,
                MAKELPARAM(0, 1000));  
    RECT workArea; 
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0); 
    RECT rect;
    GetWindowRect(control, &rect);
    SetWindowPos(control, NULL, (workArea.right - workArea.left - (rect.right - rect.left)) / 2,
                 workArea.bottom - workArea.top - (rect.bottom - rect.top), 0, 0, 
                 SWP_NOSIZE | SWP_SHOWWINDOW);
    if (__argc > 1)
    {
        filePath = __wargv[1];
        player->OpenFile(filePath.c_str(),NULL);
        player->Play();
        SetWindowText(hWnd, filePath.c_str());
    }
#if (_MSC_VER >= 1600)
    HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&itl));  
    if (SUCCEEDED(hr))
    {  
        hr = itl->HrInit();  
        if (FAILED(hr))
        {  
            itl->Release();  
            itl = NULL;  
        }  
    }
#endif
    SetProcessWorkingSetSize( GetCurrentProcess(), 0xFFFFFFFF, 0xFFFFFFFF );
	// 主消息循环:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	if (player)
	{
 		player->Stop();
		player->Release();
		player = NULL;
	}
    DestroyWindow(control);
	if (playerConfig)
	{
		playerConfig->Release();
		player = NULL;
	}
#if (_MSC_VER >= 1600)
    if (itl)
    {  
        itl->Release();  
        itl = NULL;  
    }  
#endif
    if (dll)
	{
		FreeLibrary(dll);
	}
 	CoUninitialize();
	return (int) msg.wParam;
}

//
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
//
//  注释:
//
//    仅当希望
//    此代码与添加到 Windows 95 中的“RegisterClassEx”
//    函数之前的 Win32 系统兼容时，才需要此函数及其用法。调用此函数十分重要，
//    这样应用程序就可以获得关联的
//    “格式正确的”小图标。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PLAYER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)GetStockObject(BLACK_BRUSH);
	wcex.lpszMenuName	= NULL;//MAKEINTRESOURCE(IDC_PLAYER);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
HWND InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   HWND hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
      return NULL;

   DragAcceptFiles(hWnd, TRUE);
   menu = LoadMenu(hInst, MAKEINTRESOURCE(IDC_PLAYER));

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   TCHAR path[256];
   ::GetModuleFileName(hInstance, path, 256);
   std::wstring wpath = path;
   int i = wpath.rfind('\\');
   wpath = wpath.substr( 0 ,i);
   wpath += _T("\\KGPlayer.dll");
   dll = LoadLibrary(wpath.c_str());
   if (!dll || INVALID_HANDLE_VALUE == dll)
   {
       std::wstring msg = _T("加载\"") + wpath + _T("\"失败！");
       MessageBox(NULL, msg.c_str(), _T("出错啦！"), MB_OK);
       return NULL;
   }

	typedef HRESULT (_stdcall *CreatePlayerFun)(ICorePlayer** player,HWND recv);
	CreatePlayerFun CreatePlayer = (CreatePlayerFun)GetProcAddress(dll ,
		(LPCSTR)MAKEINTRESOURCE(3));
    if (!CreatePlayer)
    {
		MessageBox(NULL, _T("创建播放器失败！"), _T("出错啦！"), MB_OK);
        return NULL;
    }
	
    CreatePlayer(&player,hWnd);
    if (!player)
    {
		MessageBox(NULL, _T("创建播放器失败！"), _T("出错啦！"), MB_OK);
        return NULL;
    }

    player->QueryInterface(IID_IPlayerConfig, (void**)&playerConfig);
    if (!playerConfig)
        return NULL;

    HMENU m = GetSubMenu(menu, 0);
    HMENU outMenu = GetSubMenu(m, 3);
    if (!outMenu)
        return NULL;

    DeleteMenu(outMenu, 0, MF_BYPOSITION); 

    MENUITEMINFO info;
    ZeroMemory(&info, sizeof(info));
    info.cbSize = sizeof(info);
    info.fMask = MIIM_STRING | MIIM_ID | MIIM_CHECKMARKS | MIIM_FTYPE | MIIM_STATE;
    info.fType = MFT_RADIOCHECK;
    
    outputCount = 0;
    wchar_t name[255] = {0};
    while(playerConfig->GetOutputTypeNameList(outputCount, name, 255) != -1)
    {
        info.wID = AUDIOMENUIDBASE + outputCount;
        info.dwTypeData = name;
        if (!outputCount)
            info.fState = MFS_CHECKED;
        else
            info.fState = 0;

//         info.cch = wcslen(name);
        InsertMenuItem(outMenu, outputCount, TRUE, &info);
        ++outputCount;
    }
	return hWnd;
}

INT_PTR CALLBACK Control(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
        PostMessage(GetParent(hDlg), message, wParam, lParam);
        return TRUE;
    case WM_CLOSE:
        ShowWindow(hDlg, SW_HIDE);
        return TRUE;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
        ::PostMessage(hDlg, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        return 0;
    case WM_HSCROLL:
        {
            int minpos = 0;
            int maxpos = 1000;

             // Get the current position of scroll box.
            int curpos = SendMessage(GetDlgItem(control, IDC_SLIDER1), TBM_GETPOS, 0, 0L);
            POINT point;
            GetCursorPos(&point); 
            RECT rt;
            GetWindowRect(GetDlgItem(control, IDC_SLIDER1), &rt);
            RECT channelRect;
            SendMessage(GetDlgItem(control, IDC_SLIDER1), TBM_GETCHANNELRECT, 0, (LPARAM)&channelRect);
            int left = rt.left + channelRect.left;
            int right = rt.left + channelRect.right;

            int nPos = (point.x - left) * maxpos / (right - left);	
            if (nPos < minpos)
                nPos = minpos;
            if (nPos > maxpos)
                nPos = maxpos;

            switch (wParam)
            {
            case SB_LEFT:      // Scroll to far left.
                curpos = minpos;
                break;

            case SB_RIGHT:      // Scroll to far right.
                curpos = maxpos;
                break;

            case SB_ENDSCROLL:   // End scroll.
            case SB_LINELEFT:      // Scroll left.
            case SB_LINERIGHT:   // Scroll right.
                return FALSE;

            case SB_PAGELEFT:    // Scroll one page left.
            case SB_PAGERIGHT:      // Scroll one page right.
            case SB_THUMBPOSITION: // Scroll to absolute position. nPos is the position
            case SB_THUMBTRACK:   // Drag scroll box to specified position. nPos is the
                curpos = nPos;     // position that the scroll box has been dragged to.
                break;
            }
            slidKeyDown = TRUE;
            // Set the new position of the thumb (scroll box).
            SendMessage(GetDlgItem(control, IDC_SLIDER1), TBM_SETPOS, TRUE, curpos);
        }
        return FALSE;
    case WM_NOTIFY:
        if (NM_RELEASEDCAPTURE == ((LPNMHDR)lParam)->code)
        {
            int pos = SendMessage(GetDlgItem(control, IDC_SLIDER1), TBM_GETPOS, 0, 0L);
            player->SetPosition(info.Duration / 1000 * pos);
            slidKeyDown = FALSE;
        }

        return FALSE;
	}
	return (INT_PTR)FALSE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的: 处理主窗口的消息。
//
//  WM_COMMAND	- 处理应用程序菜单
//  WM_PAINT	- 绘制主窗口
//  WM_DESTROY	- 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

        // 选择音频渲染器
        if (wmId >= AUDIOMENUIDBASE && wmId < AUDIOMENUIDBASE + outputCount)
        {
            HMENU m = GetSubMenu(menu, 0);
            HMENU outMenu = GetSubMenu(m, 3);
            playerConfig->SetOutputDevice(wmId - AUDIOMENUIDBASE, 0);
            ::CheckMenuItem(outMenu, audioRender, MF_UNCHECKED);
            ::CheckMenuItem(outMenu, wmId, MF_CHECKED);
            audioRender = wmId;
            return 0;
        }
        
        // 选择视频渲染器
        if (wmId == ID_OLDRENDER || wmId == ID_VMR7WINDOWNED || 
            wmId == ID_VMR7WINDOWNLESS || wmId == ID_VMR7RENDERLESS || 
            wmId == ID_VMR9WINDOWNED || wmId == ID_VMR9WINDOWNLESS ||
            wmId == ID_VMR9RENDERLESS)
        {
            HMENU m = GetSubMenu(menu, 0);
            HMENU outMenu = GetSubMenu(m, 4);
            ::CheckMenuItem(outMenu, videoRender, MF_UNCHECKED);
            ::CheckMenuItem(outMenu, wmId, MF_CHECKED);   
            videoRender = wmId;
		    switch (wmId)
            {
            case ID_OLDRENDER:
                playerConfig->SetDefaultVedioRenderer((GUID)CLSID_VideoRendererDefault,
                    RENDER_MODE_WINDOWED);
                break;
            case ID_VMR7WINDOWNED:
                playerConfig->SetDefaultVedioRenderer((GUID)CLSID_VideoMixingRenderer,
                    RENDER_MODE_WINDOWED);
                break;
            case ID_VMR7WINDOWNLESS:
                playerConfig->SetDefaultVedioRenderer((GUID)CLSID_VideoMixingRenderer,
                    RENDER_MODE_WINDOWLESS);
                break;
            case ID_VMR7RENDERLESS:
                playerConfig->SetDefaultVedioRenderer((GUID)CLSID_VideoMixingRenderer,
                    RENDER_MODE_RENDERLESS);
                break;
            case ID_VMR9WINDOWNED:
                playerConfig->SetDefaultVedioRenderer((GUID)CLSID_VideoMixingRenderer9,
                    RENDER_MODE_WINDOWED);
                break;
            case ID_VMR9WINDOWNLESS:
                playerConfig->SetDefaultVedioRenderer((GUID)CLSID_VideoMixingRenderer9,
                    RENDER_MODE_WINDOWLESS);
                break;
            case ID_VMR9RENDERLESS:
                playerConfig->SetDefaultVedioRenderer((GUID)CLSID_VideoMixingRenderer9,
                    RENDER_MODE_RENDERLESS);
                break;
            }
            return 0;
        }

		// 分析菜单选择:
		switch (wmId)
		{
		case IDM_CONTROL:
			ShowWindow(control, SW_SHOWNORMAL);
			break;
		case IDM_PLAY:
            if (state == psPlayEnd || )
            {
                player->OpenFile(filePath.c_str(), NULL);
            }
			player->Play();
			break;
		case IDM_STOP:
			player->Stop();
			SetWindowText(hWnd, _T(""));
			break;
		case IDM_PAUSE:
			player->Pause();
			break;			
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: 在此添加任意绘图代码...
		EndPaint(hWnd, &ps);
		break;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
        ::PostMessage(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        return 0;
    case WM_DROPFILES:
		{
			int count = DragQueryFile((HDROP)wParam, -1, NULL, 0);//取得被拖动文件的数目
			if (count > 0)
			{
				TCHAR name[MAX_PATH] = {0};
				DragQueryFile((HDROP)wParam, 0, name, MAX_PATH);//把文件名拷贝到缓冲区
				player->OpenFile(name, NULL);
				SetWindowText(hWnd, name);
				SetForegroundWindow(hWnd);
				player->Play();
				break;
			}
			break;
		}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_KGPLAYER_GET_MV_WINDOW:
		*((HWND*)wParam) = hWnd;
		break;
	case WM_KEYDOWN:
		if (wParam == 32)// 空格
		{
			if (state == psPlaying)
			{
				player->Pause();
			}
			else
			{
				player->Play();
			}
		}
		else if (wParam == 27) //esc
		{
			player->Stop();
			SetWindowText(hWnd, _T(""));
		}

		break;
	case WM_RBUTTONUP:
		if (menu)
		{
			HMENU m = GetSubMenu(menu, 0);
			POINT point;
			GetCursorPos(&point); 
			TrackPopupMenu(m, TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,
						   0, hWnd, NULL);
		}
		break;
	case WM_KGPLAYER_PLAYER_STATE:
		state = (TPlayerState)wParam;
        SetProcessWorkingSetSize( GetCurrentProcess(), 0xFFFFFFFF, 0xFFFFFFFF );
        if (psOpen == state)
            player->GetAudioInfo(&info);
        break;
#if 1
	case WM_KGPLAYER_PLAYING_PROGRESS:
        if (!slidKeyDown)
        {
            __int64 time = 0;
            int* p = (int*)&time;
            *p = wParam;
            p++;
            *p = lParam;

            int pos = (int)((double)time / info.Duration * 1000.0);
            SendMessage(GetDlgItem(control, IDC_SLIDER1), TBM_SETPOS, TRUE, pos);
#if (_MSC_VER >= 1600)
            if (itl)
                itl->SetProgressValue(hWnd, pos, 1000);  
#endif
        }
        break;
#endif

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
