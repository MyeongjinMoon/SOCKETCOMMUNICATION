#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "framework.h"
#include "InhaWinServerProj.h"

// >> : 
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <stdio.h>
#include <list>
// << :

#define MAX_LOADSTRING 100

// >> : 서버단
#define WM_ASYNCSERVER WM_USER + 1

struct SOCKETNAME
{
	char* ip;
	SOCKET socket;
};

std::list<SOCKETNAME> socketList;

static WSADATA wsadata;
static SOCKET s, cs;
static SOCKADDR_IN addr = { 0 }, c_addr = { 0 };
static TCHAR msgS[200] = { 0 };
static TCHAR msgC[200] = { 0 };

int size, msgLenS, msgCountS;
char bufferS[100];

int InitServer(HWND hWnd);
int CloseServer();
SOCKET AcceptSocket(HWND hWnd, SOCKET s, SOCKADDR_IN& c_addr);

int SendMessageToClient(TCHAR* str);
void ReadMessage(TCHAR* msg, char* buffer);
void CloseClient(SOCKET socket);
// << :

// >> : 클라이언트단
#define WM_ASYNCCLIENT WM_USER + 2

char bufferC[100];
int msgLenC, msgCountC;

int InitClient(HWND hWnd);
void CloseClient();
int SendMessageToServer(TCHAR*);
// << :

// >> : 출력창
BOOL CALLBACK Server_ClientProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);

HWND BtnStartServer, BtnCloseServer, BtnSendServer, BtnConnectServer, BtnCloseClient, BtnSendClient;

TCHAR word[100];
// << :

HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
					 _In_opt_ HINSTANCE hPrevInstance,
					 _In_ LPWSTR    lpCmdLine,
					 _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_INHAWINSERVERPROJ, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_INHAWINSERVERPROJ));

	MSG msg;

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

int InitServer(HWND hWnd)
{
	WSAStartup(MAKEWORD(2, 2), &wsadata);
	s = socket(AF_INET, SOCK_STREAM, 0);

	if (s == INVALID_SOCKET)
	{
		MessageBox(NULL, _T("socket() failed"), _T("Error"), MB_OK);
		return 0;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = 20;
	addr.sin_addr.S_un.S_addr = inet_addr("172.30.1.77");

	if (bind(s, (LPSOCKADDR)&addr, sizeof(addr)))
	{
		MessageBox(NULL, _T("bind() failed"), _T("Error"), MB_OK);
		return 0;
	}

	if (listen(s, 5) == SOCKET_ERROR)
	{
		MessageBox(NULL, _T("listen() failed"), _T("Error"), MB_OK);
		return 0;
	}

	WSAAsyncSelect(s, hWnd, WM_ASYNCSERVER, FD_ACCEPT);

	return 1;
}

int CloseServer()
{
	closesocket(s);
	WSACleanup();

	return 0;
}

SOCKET AcceptSocket(HWND hWnd, SOCKET s, SOCKADDR_IN& c_addr)
{
	char* ip = inet_ntoa(c_addr.sin_addr);

	SOCKETNAME cs;
	int size = sizeof(c_addr);
	cs.socket = accept(s, (LPSOCKADDR)&c_addr, &size);
	cs.ip = ip;
	WSAAsyncSelect(cs.socket, hWnd, WM_ASYNCSERVER, FD_READ | FD_CLOSE);

	socketList.push_back(cs);

	return cs.socket;
}

int SendMessageToClient(TCHAR* strS)
{
	for (std::list<SOCKETNAME>::iterator it = socketList.begin(); it != socketList.end(); it++)
	{
		SOCKET cs = (*it).socket;
#ifdef _UNICODE
		msgLenS = WideCharToMultiByte(CP_ACP, 0, strS, -1, NULL, 0, NULL, NULL);
		WideCharToMultiByte(CP_ACP, 0, strS, -1, bufferS, msgLenS, NULL, NULL);
		msgS[msgLenS] = NULL;
#else
		strcpy_s(buffer, strS);
		msgLen = strlen(buffer);
#endif
		send(cs, (LPSTR)bufferS, strlen(bufferS) + 1, 0);
		msgCountS = 0;
	}
	return 1;
}

void ReadMessage(TCHAR* msg, char* buffer)
{
	for (std::list<SOCKETNAME>::iterator it = socketList.begin(); it != socketList.end(); it++)
	{
		SOCKET cs = (*it).socket;
		int msgLen = recv(cs, buffer, 99, 0);

		if (msgLen > 0)
		{
			char* temp = new char(NULL);
			strcat(temp, (*it).ip);
			strcat(temp, " : ");
			buffer[msgLen] = NULL;
			strcat(temp, buffer);

	#ifdef _UNICODE
			msgLen = MultiByteToWideChar(CP_ACP, 0, temp, strlen(temp), NULL, NULL);
			MultiByteToWideChar(CP_ACP, 0, temp, strlen(temp), msg, msgLen);
			msg[msgLen] = NULL;
	#else
			strcpy_s(msg, buffer);
	#endif
		}
	}
}

void CloseClient(SOCKET socket)
{
	for (std::list<SOCKETNAME>::iterator it = socketList.begin(); it != socketList.end(); it++)
	{
		SOCKET cs = (*it).socket;

		if (cs == socket)
		{
			closesocket(cs);
			it = socketList.erase(it);
			break;
		}
	}
}

BOOL CALLBACK Server_ClientProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg)
	{
	case WM_INITDIALOG:
		BtnStartServer = GetDlgItem(hDlg, ID_STARTSERVERBtn);
		BtnCloseServer = GetDlgItem(hDlg, ID_CLOSESERVERBtn);
		BtnSendServer = GetDlgItem(hDlg, ID_SENDSERVERBtn);
		BtnConnectServer = GetDlgItem(hDlg, ID_CONNECTSERVERBtn);
		BtnCloseClient = GetDlgItem(hDlg, ID_CLOSECLIENTBtn);
		BtnSendClient = GetDlgItem(hDlg, ID_SENDCLIENTBtn);

		EnableWindow(BtnCloseServer, FALSE);
		EnableWindow(BtnConnectServer, FALSE);
		EnableWindow(BtnCloseClient, FALSE);

		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_STARTSERVERBtn:
			if(InitServer(hDlg))
			{
				EnableWindow(BtnStartServer, FALSE);
				EnableWindow(BtnCloseServer, TRUE);
				EnableWindow(BtnConnectServer, TRUE);
			}
			break;
		case ID_CLOSESERVERBtn:
			CloseServer();
			EnableWindow(BtnStartServer, TRUE);
			EnableWindow(BtnCloseServer, FALSE);
			break;
		case ID_SENDSERVERBtn:
			GetDlgItemText(hDlg, IDC_SERVERSENDTEXT, word, 100);
			SendMessageToClient(word);
			SetDlgItemText(hDlg, IDC_SERVERSENDTEXT, _T(""));
			break;
		case ID_SENDCLIENTBtn:
			GetDlgItemText(hDlg, IDC_CLIENTSENDTEXT, word, 100);
			SendMessageToServer(word);
			SetDlgItemText(hDlg, IDC_CLIENTSENDTEXT, _T(""));
			break;
		case ID_CONNECTSERVERBtn:
			if(InitClient(hDlg))
			{
				EnableWindow(BtnConnectServer, FALSE);
				EnableWindow(BtnCloseClient, TRUE);
			}
			break;
		case ID_CLOSECLIENTBtn:
			CloseClient();
			{
				EnableWindow(BtnConnectServer, TRUE);
				EnableWindow(BtnCloseClient, FALSE);
			}
			break;
		case IDCANCEL:
			EndDialog(hDlg, 0);
			break;
		case WM_ASYNCSERVER:
			switch (lParam)
			{
				case FD_ACCEPT:
					cs = AcceptSocket(hDlg, s, c_addr);
				break;

				case FD_READ:
					ReadMessage(msgS, bufferS);
					SetDlgItemText(hDlg, IDC_SERVERTEXT, msgS);
					//InvalidateRect(hWnd, NULL, TRUE);
				break;

			case FD_CLOSE:
					CloseClient(wParam);	// wParam : 소켓 정보
					break;
				}
			break;
		case WM_ASYNCCLIENT:
		{
			switch (lParam)
			{
				case FD_READ:
					msgLenC = recv(s, bufferC, 100, 0);
					bufferC[msgLenC] = NULL;
		#ifdef _UNICODE
					msgLenC = MultiByteToWideChar(CP_ACP, 0, bufferC, strlen(bufferC), NULL, NULL);
					MultiByteToWideChar(CP_ACP, 0, bufferC, strlen(bufferC), msgC, msgLenC);
					msgC[msgLenC] = NULL;
		#else
					strcpy_s(msg, buffer);
		#endif
					/*InvalidateRgn(hWnd, NULL, TRUE);*/
					SetDlgItemText(hDlg, IDC_CLIENTTEXT, msgC);
				break;
			}
		}
			break;
		}
	}
	return 0;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style          = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc    = WndProc;
	wcex.cbClsExtra     = 0;
	wcex.cbWndExtra     = 0;
	wcex.hInstance      = hInstance;
	wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_INHAWINSERVERPROJ));
	wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_INHAWINSERVERPROJ);
	wcex.lpszClassName  = szWindowClass;
	wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance;

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

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	switch (message)
	{
	case WM_CREATE:
		DialogBox(hInst, MAKEINTRESOURCE(IDD_SERVER), hWnd, Server_ClientProc);
		break;
	/*case WM_CHAR:
		if (wParam == VK_RETURN)
			return SendMessageToClient(str);
		str[msgCount++] = wParam;
		str[msgCount] = NULL;
		InvalidateRgn(hWnd, NULL, TRUE);
		break;*/
	//case WM_ASYNCSERVER:
	//	switch (lParam)
	//	{
	//	case FD_ACCEPT:
	//		cs = AcceptSocket(hWnd, s, c_addr);
	//		break;

	//	case FD_READ:
	//		ReadMessage(msgS, bufferS);
	//		SetDlgItemText(hWnd, IDC_SERVERTEXT, msgS);
	//		//InvalidateRect(hWnd, NULL, TRUE);
	//		break;

	//	case FD_CLOSE:
	//		CloseClient(wParam);	// wParam : 소켓 정보
	//		break;
	//	}
	//	break;
	//case WM_ASYNCCLIENT:
	//{
	//	switch (lParam)
	//	{
	//	case FD_READ:
	//		msgLenC = recv(s, bufferC, 100, 0);
	//		bufferC[msgLenC] = NULL;
	//#ifdef _UNICODE
	//		msgLenC = MultiByteToWideChar(CP_ACP, 0, bufferC, strlen(bufferC), NULL, NULL);
	//		MultiByteToWideChar(CP_ACP, 0, bufferC, strlen(bufferC), msgC, msgLenC);
	//		msgC[msgLenC] = NULL;
	//#else
	//		strcpy_s(msg, buffer);
	//#endif
	//		/*InvalidateRgn(hWnd, NULL, TRUE);*/
	//		SetDlgItemText(hWnd, IDC_CLIENTTEXT, msgC);
	//		break;
	//	}
	//}
	//	break;
	case WM_COMMAND:
		{
			int wmId = LOWORD(wParam);

			switch (wmId)
			{
			case IDM_ABOUT:
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
				break;
			case IDM_EXIT:
				DestroyWindow(hWnd);
				break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
		}
		break;
	//case WM_PAINT:
	//	{
	//		PAINTSTRUCT ps;
	//		HDC hdc = BeginPaint(hWnd, &ps);

	//		TextOut(hdc, 0, 0, str, _tcslen(str));  // 서버 입력 내용 출력

	//		if (_tcscmp(msg, _T("")))
	//		{
	//			TextOut(hdc, 0, 30, msg, (int)_tcslen(msg));    // 클라이언트에서 받은 내용 출력
	//		}

	//		EndPaint(hWnd, &ps);
	//	}
	//	break;
	case WM_DESTROY:
		CloseServer();
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

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

/*
	Q1. 채팅 프로그램 만들기
		채팅 로그 출력
		채팅 입력창 구현
*/

int InitClient(HWND hWnd)
{
	WSAStartup(MAKEWORD(2, 2), &wsadata);
	s = socket(AF_INET, SOCK_STREAM, 0);

	if (s == INVALID_SOCKET)
	{
		MessageBox(NULL, _T("socket() failed"), _T("Error"), MB_OK);
		return 0;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = 20;
	addr.sin_addr.S_un.S_addr = inet_addr("172.30.1.77");

	if (connect(s, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		MessageBox(NULL, _T("connect() failed"), _T("Error"), MB_OK);
		return 0;
	}

	WSAAsyncSelect(s, hWnd, WM_ASYNCCLIENT, FD_READ);
	return 1;
}

void CloseClient()
{
	closesocket(s);
	WSACleanup();
}

int SendMessageToServer(TCHAR* strC)
{
	if (s == INVALID_SOCKET)return 0;

#ifdef _UNICODE
	msgLenC = WideCharToMultiByte(CP_ACP, 0, strC, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, strC, -1, bufferC, msgLenC, NULL, NULL);
	msgC[msgLenC] = NULL;
#else
	strcpy_s(buffer, strC);
	msgLen = strlen(buffer);
#endif

	send(s, (LPSTR)bufferC, msgLenC + 1, 0);
	msgCountC = 0;

	return 1;
}