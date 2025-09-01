#include<winsock2.h>
#include<ws2tcpip.h>
#include<windows.h>
#include<thread>
#include<queue>
#include<mutex>
#include<gdiplus.h>
#include <mysql.h>
#include<memory>
#include<condition_variable>
#include <commctrl.h> 
#pragma comment (lib,"libmysql.lib")
#include"resource.h"
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"gdiplus.lib")

//登录时加载新登录用户的个人信息
struct my_user_information
{
	std::string password;//limit 32
	std::string nickname;//limit 32
	std::string gender;//limit 男 or 女
	std::string age;//limit 8
	std::string signature;//limit 128
};

//服务器程序退出标志//
bool g_exitFlag = false;

//广播框用户列表更新//
std::mutex users_mutex;
std::condition_variable users_cv;
bool users_update_signal = true;

int z = 0;//消息框第一次接受消息标志//
//int z_one = 0;//消息框第一次接受消息标志//
HANDLE hThread = NULL;//总线程句柄//
HWND g_hInfoDialogphoto = NULL;//更新头像对话框//
//消息队列//
HWND g_hInfoDialog = NULL;//对话框句柄//

HWND g_hInfoDialogmonitor = NULL;//监控框句柄//
HWND g_hInfoDialogbroadcast = NULL;//广播框句柄//
HWND g_hInfoDialogbroadcast_information = NULL;//广播框用户信息对话框句柄//

std::queue<std::wstring>g_msgQueue;//消息队列//
CRITICAL_SECTION g_csMsgQueue;//线程同步锁//

std::queue<std::wstring>g_msg_two_Queue;//客户端+服务器消息队列//
CRITICAL_SECTION g_csMsg_two_Queue;//线程同步锁//

//图像队列//
std::queue<std::vector<BYTE>>g_imgQueue;
std::mutex g_imgQueueMutex;//来确保线程安全//

std::mutex g_usersMutex;//广播用户列表锁//
//定义数据接收结构体//
struct receivedData {
	std::string account;
	std::string password;
	std::string nickname;
	std::string gender;
	std::string age;
	std::string signature;
	std::vector<BYTE> imgData;
	int step = 0; // 当前收到了第几项//
};


MYSQL* conn = mysql_init(NULL);//数据库初始化//

void HandleClient_informationset(SOCKET client_server);
void HandleClient_login(SOCKET client_server,std::string);//登录处理线程//
void HandleClient_passwordset(SOCKET client_server);
void HandleClient_photoset(SOCKET client_server);
void Handlelogin_pro(SOCKET client_server, receivedData my_data,std::string);

//数据接收函数//
int recvAll(SOCKET socket, char* buffer, int len, int y = 0)
{
	int r = 0;//单次接收字节数//
	int sum = 0;//累次接收字节数//
	while (sum < len)
	{
		r = recv(socket, buffer + sum, len - sum, 0);
		if (r <= 0)
		{
			break;
		}
		sum += r;
	}
	return sum;
}

struct Userdata
{
	std::string nickname;//用户昵称//
	std::vector<BYTE>imgData;//用户头像数据//
	std::string account;//用户账号//
};
std::vector<Userdata> g_users;//用于加载数据库里的所有用户信息，来保持用户列表更新//

std::mutex g_onlineUsersMutex;//在线用户列表锁//
struct User_account
{
	std::string account;//用户账号//
};
std::vector<User_account> g_onlineUsers;//用来记录在线的用户//


std::mutex g_users_online_Mutex;//在线用户锁//
//存储用户账号和socket来实行广播的信息分发//
struct User_online
{
	std::string account;
	SOCKET socket;
};
std::vector<User_online> g_users_online;

//在线用户列表更新//
std::mutex users_online_mutex;
std::condition_variable users_online_cv;
bool users_online_update_signal = false;

std::vector<std::string>g_select_offline_users_account;//存储广播框中选中的用户中离线的用户账号//
std::mutex g_users_account_sel_broadcast_Mutex;//来确保线程安全//

struct offline_users_and_information//存储离线用户账号和广播消息//
{
	std::string account;
	std::string information;
};
std::vector<offline_users_and_information>g_offline_users_and_information;
std::mutex g_users_account_and_information_sel_broadcast_Mutex;//来确保线程安全//

// 存储离线用户昵称消息
std::vector<std::string>g_offline_users_nickname;
std::mutex g_users_nickname_sel_broadcast_Mutex;//来确保线程安全//

struct users_anii_on_chartroom
{
	std::string user_on_chartroom_account;
	std::string user_on_chartroom_nickname;
	std::vector<BYTE> user_on_chartroom_image;
	std::string user_on_chartroom_inf;
};
std::vector  <users_anii_on_chartroom> users_account_on_chartroom;//存储已经进入聊天室的用户账号
std::mutex users_anii_on_chartroom_mutex;//加锁


struct users_chart_information
{
	std::string inf_send_account;
	std::string inf_recv_account;
	std::string inf;
};
std::vector<users_chart_information>g_users_chart_information;
std::mutex g_users_chart_information_mutex;


INT_PTR CALLBACK Dialog_one(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//开始对话框//
{
	static UINT_PTR TimeID = 0;//启用定时器//
	static HBRUSH hBackgroundBrush = NULL;//背景画刷//
	static HBITMAP hBmp = NULL;//位图句柄//
	switch (uMsg)
	{
	case WM_INITDIALOG:
		TimeID = SetTimer(hwndDlg, 1, 4000, NULL);//显示第一个对话框的内容，延迟4秒；1为定时器的ID，当定时结束后，传递给wParam的参数为其ID，即1.//
		hBackgroundBrush = CreateSolidBrush((RGB(200, 230, 255)));//设置背景为淡蓝色//
		hBmp = (HBITMAP)LoadImage(NULL, L"image_one.bmp", IMAGE_BITMAP, 200, 170, LR_LOADFROMFILE);//加载图片位置//
		if (hBmp)
		{
			SendDlgItemMessage(hwndDlg, IDC_MY_IMAGE_ONE, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmp);
		}
		else
		{
			MessageBox(hwndDlg, L"image lost", L"QQ", MB_ICONERROR);
		}
		return (INT_PTR)TRUE;//设置焦点//
	case WM_DRAWITEM: {
		DRAWITEMSTRUCT* pdis = (DRAWITEMSTRUCT*)lParam;
		if (pdis->CtlID == IDC_CUSTOM_TEXT) {
			SetTextColor(pdis->hDC, RGB(255, 0, 0));
			DrawText(pdis->hDC, L"Welcome to QQ's server", -1, &pdis->rcItem,//对话框标题//
				DT_CENTER | DT_VCENTER);
		}
		return (INT_PTR)TRUE;
	}
	case WM_CTLCOLORDLG:
		return (INT_PTR)hBackgroundBrush;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDSTART)
		{
			EndDialog(hwndDlg, IDOK);//关闭对话框//
		}
		return (INT_PTR)TRUE;
		/*
	case WM_TIMER:
		if (wParam == 1)
		{
			EndDialog(hwndDlg, IDOK);//关闭对话框//
		}
		return (INT_PTR)TRUE;
		*/
	case WM_DESTROY:
		KillTimer(hwndDlg, TimeID);//销毁定时器//
		DeleteObject(hBackgroundBrush);//释放画刷//
		DeleteObject(hBmp);//释放位图资源//
		break;
	case WM_CLOSE:
		EndDialog(hwndDlg, IDEND);//关闭对话框//
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK Dialog_end(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//结束对话框//
{
	static UINT_PTR TimeID = 0;//启用定时器//
	static HBRUSH hBackgroundBrush = NULL;//背景画刷//
	static HBITMAP hBmp = NULL;//位图句柄//
	switch (uMsg)
	{
	case WM_INITDIALOG:
		TimeID = SetTimer(hwndDlg, 1, 0, NULL);//显示第一个对话框的内容，延迟4秒//
		hBackgroundBrush = CreateSolidBrush((RGB(200, 230, 255)));//设置背景为淡蓝色//
		return (INT_PTR)TRUE;//设置焦点//
	case WM_DRAWITEM: {
		DRAWITEMSTRUCT* pdis = (DRAWITEMSTRUCT*)lParam;
		if (pdis->CtlID == IDC_CUSTOM_TEXT_TWO) {
			SetTextColor(pdis->hDC, RGB(255, 0, 0));
			DrawText(pdis->hDC, L"Goodbye", -1, &pdis->rcItem,//对话框标题//
				DT_CENTER | DT_VCENTER);
		}
		return (INT_PTR)TRUE;
	}
	case WM_CTLCOLORDLG:
		return (INT_PTR)hBackgroundBrush;
	case WM_TIMER:
		if (wParam == 1)
		{
			KillTimer(hwndDlg, TimeID);//销毁定时器//
			DeleteObject(hBackgroundBrush);//释放画刷//
			//EndDialog(hwndDlg, IDOK);//关闭对话框//
			exit(0);//退出程序//
		}
		return (INT_PTR)TRUE;
	case WM_CLOSE:
		EndDialog(hwndDlg, IDEND);//关闭对话框//
		break;
	}
	return (INT_PTR)FALSE;
}

//消息更新函数//
void UpdateDialogContent(HWND hwndDlg)
{
	EnterCriticalSection(&g_csMsgQueue);//加锁//
	if (!g_msgQueue.empty())//队列非空//
	{
		std::wstring msg = g_msgQueue.front();//取队首元素//
		g_msgQueue.pop();//将队首元素弹出//
		int len = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_INFORMATION));//获取当前控件文本长度，不包含'\0'//
		wchar_t* currentText = new wchar_t[len + 1];
		GetWindowText(GetDlgItem(hwndDlg, IDC_INFORMATION), currentText, len + 1);//获取当前控件文本,在结尾加上'\0'//
		std::wstring combined = L"";
		if (len > 0)
		{
			if (z)//第一次接收消息时，去除对话框的“等待消息...”//
			{
				combined = currentText;//复制时不会带上\0//
				combined += L"\r\n";
			}
			z = 1;
		}
		combined += msg;
		SetDlgItemText(hwndDlg, IDC_INFORMATION, combined.c_str());//更新消息//
		delete[] currentText;
	}
	LeaveCriticalSection(&g_csMsgQueue);//解锁//
}

//获取图像显示句柄//
HBITMAP CreateHBITMAPFromBytes(const BYTE* pData, int len, int wide, int high)
{
	HBITMAP hbmp = NULL;//初始化位图句柄//
	IStream* pStream = NULL;
	//创建内存流//
	if (CreateStreamOnHGlobal(NULL, TRUE, &pStream) == S_OK)
	{
		ULONG written = 0;//实际写入流的字节数//
		pStream->Write(pData, len, &written);
		//重置指针到流开头//
		LARGE_INTEGER li{};
		pStream->Seek(li, STREAM_SEEK_SET, NULL);
		using namespace Gdiplus;
		Bitmap* bmp = Bitmap::FromStream(pStream);
		if (bmp && bmp->GetLastStatus() == Ok)
		{
			//缩放到控件大小//
			Bitmap* pThumb = new Bitmap(wide, high, bmp->GetPixelFormat());
			Graphics g(pThumb);
			g.DrawImage(bmp, 0, 0, wide, high);//根据原图进行重绘//
			pThumb->GetHBITMAP(Color(200, 230, 255), &hbmp);
			delete pThumb;
		}
		delete bmp;
		pStream->Release();
	}
	return hbmp;
}


//头像更新框//
void UpdateDialogphoto(HWND hwndDlg)
{
	std::vector<BYTE>imgData;
	std::lock_guard<std::mutex> lock(g_imgQueueMutex);//线程安全，自动加锁和解锁//
	if (!g_imgQueue.empty())
	{
		imgData = std::move(g_imgQueue.front());//取出队首元素//
		g_imgQueue.pop();//弹出队首元素//
	}
	HWND hwnd = GetDlgItem(hwndDlg, IDC_PHOTO);//获取控件尺寸//
	RECT rc;
	GetClientRect(hwnd, &rc);//初始化rc结构体//
	int wide = rc.right - rc.left;
	int high = rc.bottom - rc.top;
	//获取图像显示句柄//
	HBITMAP hbmp = CreateHBITMAPFromBytes(imgData.data(), imgData.size(), wide, high);
	if (hbmp)
	{
		SendMessage(hwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmp);
		DeleteObject(hbmp);//释放句柄资源//
	}
	else
	{
		MessageBox(NULL, L"无法显示图像句柄", L"QQ", MB_ICONERROR);
		DeleteObject(hbmp);//释放句柄资源//
		exit(1);
	}
}


INT_PTR CALLBACK Dialog_information(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//消息对话框//
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, IDC_INFORMATION, L"等待消息...");//更新消息//
		return (INT_PTR)TRUE;
	case WM_APP_UPDATE_MSG:
		UpdateDialogContent(hwndDlg);
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL)
		{
			DestroyWindow(hwndDlg);
			g_hInfoDialog = NULL;
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MYDIALOG_END), NULL, Dialog_end);//结束框//
			return (INT_PTR)TRUE;
		}
	}
	return (INT_PTR)FALSE;
}


INT_PTR CALLBACK Dialog_photo(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//头像显示框//
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, IDC_PHOTO, L"等待用户上传头像...");//更新头像//
		return (INT_PTR)TRUE;
	case WM_APP_UPDATEPHOTO_MSG:
		UpdateDialogphoto(hwndDlg);
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL)
		{
			DestroyWindow(hwndDlg);
			g_hInfoDialogphoto = NULL;
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MYDIALOG_END), NULL, Dialog_end);//结束框//
			return (INT_PTR)TRUE;
		}
	}
	return (INT_PTR)FALSE;
}

void UpdateDialogmonitor(HWND hwndDlg)
{
	static int z_one = 0;//消息框第一次接受消息标志//
	EnterCriticalSection(&g_csMsg_two_Queue);//加锁//
	if (!g_msg_two_Queue.empty())
	{
		std::wstring msg_one = g_msg_two_Queue.front();//取队首元素//
		g_msg_two_Queue.pop();//将队首元素弹出//
		HWND hwnd = GetDlgItem(hwndDlg, IDC_MONITOR);
		if (!hwnd)
		{
			MessageBox(NULL, L"获取聊天框句柄失效", L"QQ", MB_ICONERROR);
			LeaveCriticalSection(&g_csMsg_two_Queue);//解锁//
			return;
		}
		int len = GetWindowTextLength(hwnd);//获取当前控件文本长度，不包含'\0'//
		wchar_t* currentText = new wchar_t[len + 1];
		GetWindowText(hwnd, currentText, len + 1);//获取当前控件文本,在结尾加上'\0'//
		std::wstring combined = L"";
		if (len > 0)
		{
			if (z_one)//第一次接收消息时，去除对话框的“等待消息...”//
			{
				combined = currentText;//复制时不会带上\0//
				combined += L"\r\n";
			}
			z_one = 1;
		}
		combined += msg_one;
		SetDlgItemText(hwndDlg, IDC_MONITOR, combined.c_str());//更新消息//
		delete[] currentText;
	}

	LeaveCriticalSection(&g_csMsg_two_Queue);//解锁//
}


INT_PTR CALLBACK Dialog_monitor(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//监控框//
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, IDC_MONITOR, L"未监控到信息传递...");//监控框初始化//
		return (INT_PTR)TRUE;
	case WM_APP_UPDATEMONITOR_MSG:
		UpdateDialogmonitor(hwndDlg);
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL)
		{
			DestroyWindow(hwndDlg);
			g_hInfoDialogmonitor= NULL;
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MYDIALOG_END), NULL, Dialog_end);//结束框//
			return (INT_PTR)TRUE;
		}
	}
	return (INT_PTR)FALSE;
}


INT_PTR CALLBACK Dialog_broadcast_information(HWND hndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//广播信息对话框//
{
	switch (uMsg)
{
	case WM_INITDIALOG:
	{
		HWND hEdit = GetDlgItem(hndDlg, IDC_BROADCAST_INFORAMATION_EDIT);//获取消息框句柄
		HWND hBtn = GetDlgItem(hndDlg, IDOK);//获取发送按钮句柄
		int len = GetWindowTextLength(hEdit);//获取消息框文本长度
		EnableWindow(hBtn, len > 0 ? TRUE : FALSE);
		return (INT_PTR)TRUE;
	}
	case WM_COMMAND:
	{
		// 检测编辑框内容变化
		if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_BROADCAST_INFORAMATION_EDIT)
		{
			HWND hEdit = GetDlgItem(hndDlg, IDC_BROADCAST_INFORAMATION_EDIT);
			HWND hBtn = GetDlgItem(hndDlg, IDOK);
			int len = GetWindowTextLength(hEdit);
			EnableWindow(hBtn, len > 0 ? TRUE : FALSE);
			return (INT_PTR)TRUE;
		}
		switch (LOWORD(wParam))
	  {
		case IDCANCEL:
		{
			EndDialog(hndDlg, IDCANCEL);
			return (INT_PTR)TRUE;
		}
		case IDOK:
		{
			
			//获取控件IDC_BROADCAST_INFORAMATION_EDIT的句柄
			HWND hEdit = GetDlgItem(hndDlg, IDC_BROADCAST_INFORAMATION_EDIT);
			//获取该控件文本信息
			int len = GetWindowTextLength(hEdit);//获取文本长度,不含'\0'//
			if (len <= 0)
			{
				MessageBox(NULL,L"发送消息不允许为空",L"QQ",MB_ICONERROR);
				return (INT_PTR)TRUE;
			}
			std::wstring text(len+1, L'\0');
			GetWindowText(hEdit, &text[0], len + 1);//给文本自动添加L'\0'
			//转为UTF-8
			int utf8len = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, NULL, 0, NULL, NULL);//包含'\0'本身
			std::string msg(utf8len, '\0');
			WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, &msg[0], utf8len, NULL, NULL);//存储要发送的广播消息
			//清空文本框
			SetDlgItemText(hndDlg, IDC_BROADCAST_INFORAMATION_EDIT, L"");
			
			HWND hBtn = GetDlgItem(hndDlg, IDOK);
			EnableWindow(hBtn,FALSE);//手动禁用发送按钮


			//将消息显示		
			for (const auto &acc_nickname:g_offline_users_nickname)
			{
				//获取当前时间//
				time_t now_one = time(0);//获取当前时间，精确到秒//
				tm tm_one;//声明一个结构体，用于存储本地时间的各个组成部分//
				localtime_s(&tm_one, &now_one);//将now_one（输入）里的内容复制到tm_one里输出//
				wchar_t timeBuffer_one[64];
				wcsftime(timeBuffer_one, sizeof(timeBuffer_one) / sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", &tm_one);
				//MessageBox(NULL, L"已经准备好了时间字段", L"QQ", MB_ICONINFORMATION);


				std::wstring msg_one = L"[";
				msg_one += timeBuffer_one;
				msg_one += L"][";
				msg_one += L"服务器";
				msg_one += L"->";

				//用户昵称
				int  w_s_nickname_len = 0;
				w_s_nickname_len = MultiByteToWideChar(CP_UTF8, 0, acc_nickname.c_str(), acc_nickname.size(), NULL, 0);
				std::wstring w_s_nickname_str(w_s_nickname_len, L'\0');
				MultiByteToWideChar(CP_UTF8, 0, acc_nickname.c_str(), acc_nickname.size(), &w_s_nickname_str[0], w_s_nickname_len);

				msg_one += w_s_nickname_str;
				msg_one += L"]";

				int  w_s_len = 0;
				w_s_len = MultiByteToWideChar(CP_UTF8, 0,msg.c_str(),msg.size(), NULL, 0);
				std::wstring w_s_str(w_s_len, L'\0');
				MultiByteToWideChar(CP_UTF8, 0, msg.c_str(), msg.size(), &w_s_str[0], w_s_len);
				msg_one += w_s_str.c_str();

				//获取原来的文本
				HWND h_text = GetDlgItem(hndDlg, IDC_BROADCAST_INFORAMATION);
				int w_text = GetWindowTextLength(h_text);
				std::wstring old_text(w_text+1,L'\0');
				GetWindowText(h_text,&old_text[0],w_text+1);
				
				
					if (!old_text.empty() && old_text.back() == L'\0')
					{
						old_text.pop_back();
						old_text += L"\r\n";
					}
				
				old_text += msg_one;
				SetDlgItemText(hndDlg, IDC_BROADCAST_INFORAMATION,old_text.c_str());

			}

			//对于离线用户的消息处理//
			for (const auto& offline_account : g_select_offline_users_account)
			{
				offline_users_and_information info;
				info.account = offline_account;//存储离线账号
				info.information = msg;//存储消息string格式,含'\0'
				std::lock_guard<std::mutex> lock_broadcast_account(g_users_account_and_information_sel_broadcast_Mutex);//来确保线程安全//
				g_offline_users_and_information.push_back(info);
			}
			if (g_offline_users_and_information.empty())
			{
				MessageBox(NULL, L"未能将消息和对应的用户账号一起存储", L"QQ", MB_ICONERROR);
			}
			return (INT_PTR)TRUE;
		}

	  }
	}
  }
	return (INT_PTR)FALSE;
}


INT_PTR CALLBACK Dialog_broadcast(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//广播框//
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		//广播框初始化//
		HWND hList = GetDlgItem(hwndDlg, IDC_USERS);//获取用户列表句柄//
		SendMessage(hList, LB_RESETCONTENT, 0, 0);//清空文本框//

		// 添加“等待初始化”占位项//
		int idx = SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)L"正在初始化");//返回该项内容的索引//
		SendMessage(hList, LB_SETITEMDATA, idx, (LPARAM)-1); // 用-1区分//

		SendMessage(hList, LB_SETSEL, FALSE, -1);//取消所有选中项//
		LONG style = GetWindowLong(hList, GWL_STYLE);//获取当前控件的样式//
		style &= ~(LBS_SINGLESEL);//清除LBS_OWNERDRAWFIXED样式//
		style |= LBS_MULTIPLESEL;  // 增加多选，支持Ctrl/Shift
		SetWindowLong(hList, GWL_STYLE, style); // 应用新样式
		HWND hBtn = GetDlgItem(hwndDlg, IDOK); //播按钮句柄//
		EnableWindow(hBtn, FALSE); // 初始禁用广播按钮
		return (INT_PTR)TRUE;
	}

	case  WM_APP_UPDATEBROADCAST_MSG:          //更新用户列表消息//
	{
		HWND hList = GetDlgItem(hwndDlg, IDC_USERS);//获取用户列表句柄
		SendMessage(hList, LB_RESETCONTENT, 0, 0);//清空文本框//
		std::lock_guard<std::mutex> lock(g_usersMutex);
		for (int i = 0; i < g_users.size(); ++i)
		{
			//SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)L"");
			//SendMessage(hList, LB_SETITEMDATA,i, (LPARAM)i);
			 // 展示账号+昵称
			std::wstring wdisplay;
			{
				// 账号
				int wlen_acc = MultiByteToWideChar(CP_UTF8, 0, g_users[i].account.c_str(), (int)g_users[i].account.size(), NULL, 0);
				std::wstring wacc(wlen_acc, 0);
				MultiByteToWideChar(CP_UTF8, 0, g_users[i].account.c_str(), (int)g_users[i].account.size(), &wacc[0], wlen_acc);

				// 昵称
				int wlen_nick = MultiByteToWideChar(CP_UTF8, 0, g_users[i].nickname.c_str(), (int)g_users[i].nickname.size(), NULL, 0);
				std::wstring wnick(wlen_nick, 0);
				MultiByteToWideChar(CP_UTF8, 0, g_users[i].nickname.c_str(), (int)g_users[i].nickname.size(), &wnick[0], wlen_nick);

				wdisplay = wacc + L" " + wnick;
			}
			int itemIdx = (int)SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)wdisplay.c_str());
			SendMessage(hList, LB_SETITEMDATA, itemIdx, (LPARAM)i);
		}
		return (INT_PTR)TRUE;
	}
	case WM_COMMAND:
	{
		HWND hList = GetDlgItem(hwndDlg, IDC_USERS);
		HWND hBtn = GetDlgItem(hwndDlg, IDOK);
		if (HIWORD(wParam) == LBN_SELCHANGE && LOWORD(wParam) == IDC_USERS)
		{
			int selCount = (int)SendMessage(hList, LB_GETSELCOUNT, 0, 0);
			EnableWindow(hBtn, selCount > 0 ? TRUE : FALSE);
		}


		switch (LOWORD(wParam))
		{
		case IDCANCEL:
		{
			DestroyWindow(hwndDlg);
			g_hInfoDialogbroadcast = NULL;
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MYDIALOG_END), NULL, Dialog_end);//结束框//
			return (INT_PTR)TRUE;
		}
		case IDOK:
		{
			// 清空上次的选择
			g_select_offline_users_account.clear();
			g_offline_users_nickname.clear();

			int selCount = (int)SendMessage(hList, LB_GETSELCOUNT, 0, 0);
			if (selCount > 0)
			{
				std::vector<int> selItems(selCount, 0);
				SendMessage(hList, LB_GETSELITEMS, selCount, (LPARAM)selItems.data());

				for (int i = 0; i < selCount; ++i)
				{
					int itemIdx = selItems[i];
					int userIdx = (int)SendMessage(hList, LB_GETITEMDATA, itemIdx, 0);
					if (userIdx < 0 || userIdx >= (int)g_users.size()) continue;
					const std::string& account = g_users[userIdx].account;
					std::lock_guard<std::mutex> lock_broadcast_account(g_users_account_sel_broadcast_Mutex);//来确保线程安全//
					g_select_offline_users_account.push_back(account);//将全部选中的用户账号存储

					// 存储选中用户昵称消息
					std::lock_guard<std::mutex>lock_broadcast_nickname(g_users_nickname_sel_broadcast_Mutex);
					g_offline_users_nickname.push_back(g_users[userIdx].nickname);

				}
			}
			if (g_select_offline_users_account.empty())
			{
				MessageBox(NULL,L"未能将选中的广播对象账号存储",L"QQ",MB_ICONERROR);
				return (INT_PTR)TRUE;
			}
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MYDIALOG_BROADCAST_INFORMATION), NULL, Dialog_broadcast_information);//广播信息对话框//
			return (INT_PTR)TRUE;
		}

		}
		break;
	}
	case WM_MEASUREITEM:
	{
		LPMEASUREITEMSTRUCT mis = (LPMEASUREITEMSTRUCT) lParam;
		if (mis->CtlID == IDC_USERS)
		{
			mis->itemHeight = 72;//设置IDC_USERS的项高度为72//
		}
		return (INT_PTR)TRUE;
	}
	case WM_DRAWITEM:
	{
		LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;//指向一个DRAWITEMSTRUCT结构体的指针//
		if (dis->CtlID == IDC_USERS)
		{
			size_t idx = dis->itemData;

			std::lock_guard<std::mutex> lock(g_usersMutex);
			if (idx == (size_t)-1 || idx >= g_users.size())
			{
				return (INT_PTR)FALSE;
			}

			const Userdata& user = g_users[idx];
			HDC hdc = dis->hDC;//绘图设备环境//
			RECT rc = dis->rcItem;//需绘制的项的矩形区域//

			//画背景//
			SetBkMode(hdc,OPAQUE);//设置设备环境的背景混合模式为OPAQUE//
			//SetBkColor(hdc, (dis->itemState & ODS_SELECTED) ? RGB(230, 230, 250) : RGB(255, 255, 255));
			//ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);//填充rc的背景色//
			SetBkColor(hdc, (dis->itemState & ODS_SELECTED) ? RGB(230, 230, 250) : RGB(255, 255, 255));
			ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);//填充rc的背景色//
			//画头像
			int imgsize = 64;//头像大小//
			if (!user.imgData.empty())//头像数据非空//
			{
				HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE,user.imgData.size());//创建一块可移动内存，并返回内存句柄//
				if (!hMem)
				{
					MessageBox(NULL,L"可移动内存分配失败",L"QQ",MB_ICONERROR);
					return (INT_PTR)FALSE;
				}
				void* pMem = GlobalLock(hMem);//加锁//
				memcpy(pMem, user.imgData.data(), user.imgData.size());//把头像图片的字节数据复制到刚才分配的内存块中//
				GlobalUnlock(hMem);//解锁//
				IStream* pStream = NULL;
				if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) == S_OK)//把全局内存包装为IStream内存对象,pStream指向这个对象//
				{
					Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromStream(pStream);//生成Bitmap对象//
					if (bmp && bmp->GetLastStatus() == Gdiplus::Ok)//如果成功创建对象//
					{
						Gdiplus::Graphics g(hdc);//创建Graphics 对象g,并绑定 hdc设备运行环境//
						g.DrawImage(bmp,rc.left+2,rc.top+2,imgsize,imgsize);

					}
					delete bmp;
					pStream->Release();
				}
				GlobalFree(hMem);//释放内存//
			}

			//查询在线用户中该用户的账号是否存在//
			bool isOnline = false;
			{
				std::lock_guard<std::mutex> lock(g_onlineUsersMutex);
				for (const auto& user_account_online : g_onlineUsers)
				{
					std::string x = user_account_online.account;//g_user不含‘\0’结尾//
					if (!x.empty()&&x.back() == '\0')
					{
						x.pop_back();
					}
					if (strcmp(x.c_str(), user.account.c_str()) == 0)
					{
						isOnline = true;
						break;
					}
				}
			}

			// 设置颜色
			COLORREF textColor = isOnline ? RGB(0, 128, 0) : RGB(255, 0, 0);

			//画账号//
			RECT rcAccount = rc;
			rcAccount.left += imgsize + 8;
			rcAccount.top += 12;
			int wlen = MultiByteToWideChar(CP_UTF8, 0, user.account.c_str(), user.account.size() + 1, NULL, 0);
			std::wstring wnick(wlen, 0);
			MultiByteToWideChar(CP_UTF8, 0, user.account.c_str(), user.account.size() + 1, &wnick[0], wlen);//将文本转化为宽字符串//
			//DrawTextW(hdc,wnick.c_str(),wlen,&rcText,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
			SetTextColor(hdc, textColor); // 设置文本颜色
			DrawTextW(hdc, wnick.c_str(), wlen, &rcAccount, DT_LEFT | DT_TOP | DT_SINGLELINE);


			// 计算账号高度
			DrawTextW(hdc, wnick.c_str(), wlen, &rcAccount, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_CALCRECT);

			//画昵称//
			RECT rcText = rcAccount;
			rcText.top = rcAccount.bottom + 8;
			rcText.bottom = rcText.top + (rcAccount.bottom - rcAccount.top); //更新rcText.bottom//
			rcText.right +=1000;
			int wlen2 = MultiByteToWideChar(CP_UTF8, 0, user.nickname.c_str(), user.nickname.size() + 1, NULL, 0);
			std::wstring wnick_2(wlen2, 0);
			MultiByteToWideChar(CP_UTF8, 0, user.nickname.c_str(), user.nickname.size() + 1, &wnick_2[0], wlen2);//将文本转化为宽字符串//
			SetTextColor(hdc, textColor);
			DrawTextW(hdc, wnick_2.c_str(), wlen2, &rcText, DT_LEFT | DT_TOP | DT_SINGLELINE);

			//画登录状态//
			int big_num = (wlen2 > wlen) ? wlen2 : wlen;
			std::wstring big_str = (wnick.size() >wnick_2.size()) ? wnick:wnick_2;
			SIZE sz;
			GetTextExtentPoint32W(hdc, big_str.c_str(), big_num, &sz);//获取文本的宽度和高度//
			RECT rcRegister= rcAccount;
			rcRegister.left +=sz.cx;//更新文本框的左边框//
			rcRegister.right += sz.cx;//更新文本框的右边框//
			SetTextColor(hdc, textColor);
			DrawTextW(hdc, isOnline ? L"在线" : L"离线", -1, & rcRegister, DT_LEFT | DT_VCENTER| DT_SINGLELINE);

			//画焦点框//
			if (dis->itemState & ODS_FOCUS)
			{
				DrawFocusRect(hdc, &rc);
			}
		}
		return (INT_PTR)TRUE;
	}
	}
	return (INT_PTR)FALSE;
}




void SaveToDatabase(receivedData regData)
{
	char sql[1024];
	snprintf(sql, sizeof(sql),
		"INSERT INTO users (account,password,nickname,gender,age,signature,imgData) VALUES ('%s','%s','%s','%s','%s','%s',?)",
		regData.account.c_str(), regData.password.c_str(), regData.nickname.c_str(), regData.gender.c_str(), regData.age.c_str(), regData.signature.c_str());
	MYSQL_STMT* stmt = mysql_stmt_init(conn);
	mysql_stmt_prepare(stmt, sql, strlen(sql));
	// 设置参数(BLOB)
	MYSQL_BIND bind[1];
	memset(bind, 0, sizeof(bind));
	bind[0].buffer_type = MYSQL_TYPE_LONG_BLOB;
	bind[0].buffer = (void*)regData.imgData.data();
	bind[0].buffer_length = regData.imgData.size();
	mysql_stmt_bind_param(stmt, bind);
	if (mysql_stmt_execute(stmt))
	{
		MessageBox(NULL, L"MySQL Execute Error", L"QQ", MB_ICONERROR);
	}
	mysql_stmt_close(stmt);
}




void update_new_users_list()//更新广播框用户列表
{
	    MYSQL* conn_5 = mysql_init(NULL);
		int con_num = 0;
		while (!mysql_real_connect(conn_5, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))//连接数据库//
		{
			MessageBox(NULL, L"连接数据库失败", L"QQ", MB_ICONERROR);
			con_num++;
			if (con_num > 3)
			{
				return ;
			}
		}

		std::vector<Userdata>temp_users;
		temp_users.clear();
		const char* sql = "SELECT nickname,imgData,account FROM users";//SQL命令语句//
		if (mysql_query(conn_5, sql))//如果没有符合的查询结果，返回非零值//
		{
			//MessageBox(NULL, L"没有符合的昵称和头像", L"QQ", MB_ICONERROR);
			return;
		}

		MYSQL_RES* res = mysql_store_result(conn_5);//一个结果集//
		if (!res)
		{
			return;
		}
		MYSQL_ROW  row;//指向字符串数组的指针//
		unsigned long* lengths;
		while ((row = mysql_fetch_row(res)))//取某一行查询结果集//
		{
			lengths = mysql_fetch_lengths(res);//取某一行查询结果长度集//
			Userdata user;
			if (row[0])
			{
				user.nickname = row[0];
			}
			if (row[1] && lengths[1] > 0)
			{
				user.imgData.assign((BYTE*)row[1], (BYTE*)row[1] + lengths[1]);
			}
			if (row[2])
			{
				user.account = row[2];
			}
			temp_users.push_back(std::move(user));//将填充好的用户对象移动到全局用户列表里，防止不必要的拷贝//
		}
		mysql_free_result(res);
		std::lock_guard<std::mutex>lock(g_usersMutex);//加锁//
		g_users.swap(temp_users);

		if (g_hInfoDialogbroadcast && IsWindow(g_hInfoDialogbroadcast))//等待句柄有效//
			PostMessage(g_hInfoDialogbroadcast, WM_APP_UPDATEBROADCAST_MSG, 0, 0); // 自定义消息
}

void HandleClient_register(SOCKET client_server)//注册处理线程//
{
	std::string new_online_user_account;
	//第一个判断//
	int len_one = 0;//接收客户端得到账号后的选择，继续 或 取消// 
	recvAll(client_server, (char*)&len_one, sizeof(len_one), 0);//先接收字节长度//
	if ((int)len_one <= 0)
	{
		MessageBox(NULL, L"接收'客户端得到账号后的选择，继续 或 取消'长度错误", L"QQ", MB_ICONERROR);//标识长度接收错误提示//
		return;
	}
	std::string received_one(len_one, 0);//分配空间// 
	recvAll(client_server, &received_one[0], len_one, 0);//接受标识内容//
	int wlen_one = MultiByteToWideChar(CP_UTF8, 0, &received_one[0], len_one, NULL, 0);//计算转换长度//
	std::wstring wreceived_one(wlen_one, 0);
	MultiByteToWideChar(CP_UTF8, 0, &received_one[0], len_one, &wreceived_one[0], wlen_one);//转换为宽字符数组//
	if (wcscmp(wreceived_one.c_str(), L"取消") == 0)
	{
		//判断具体执行哪种线程,登录或注册//
		int recv_len = 0;
		recv(client_server, (char*)&recv_len, sizeof(recv_len), 0);//接收消息长度//
		if (recv_len <= 0)
		{
			//MessageBox(NULL, L"接收线程类型的消息长度失败", L"QQ", MB_ICONERROR);
			return;
		}
		//MessageBox(NULL, L"接收线程类型的消息长度成功", L"QQ", MB_ICONINFORMATION);
		std::string recvchar(recv_len, 0);
		int r = 0;
		r = recv(client_server, &recvchar[0], recv_len, 0);
		{
			if (r <= 0)
			{
				//MessageBox(NULL, L"接收线程类型的消息失败", L"QQ", MB_ICONERROR);
				return;
			}
		}
		//MessageBox(NULL, L"接收线程类型的消息成功", L"QQ", MB_ICONINFORMATION);
		int wlen = MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, NULL, 0);//转换为宽字节所需要的字节数//
		std::wstring wrecvchar(wlen, 0);//分配空间//
		MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, &wrecvchar[0], wlen);//实际转换//
		//注册处理//
		if (wcscmp(wrecvchar.c_str(), L"注册") == 0)//注册线程//
		{
			//MessageBox(NULL, L"接收到注册消息，即将启用注册线程", L"QQ", MB_ICONINFORMATION);
			std::thread(HandleClient_register, client_server).detach();//创建注册线程//
		}
		//登录处理//
		else if (wcscmp(wrecvchar.c_str(), L"登录") == 0)//登录线程//
		{
			//MessageBox(NULL, L"接收到登录消息，即将启用登录线程", L"QQ", MB_ICONINFORMATION);
		
			std::thread(HandleClient_login, client_server,new_online_user_account).detach();//创建登录线程//
		}
		else
		{
			//MessageBox(NULL, L"接收不到线程消息，即将退出", L"QQ", MB_ICONINFORMATION);
			return;
		}
	}
	//sockaddr_in clientAddr;
	//int addrlen = sizeof(clientAddr);
	//获取客户端地址//
	//getpeername(client_server, (sockaddr*)&clientAddr, &addrlen);
	//char ipStr[INET_ADDRSTRLEN];
	//inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
	//转换为Unicode字符串（WCHAR）
	//wchar_t wIP[20];
	//MultiByteToWideChar(CP_UTF8,0,ipStr,-1,wIP,sizeof(wIP)/sizeof(wchar_t));
	else if (wcscmp(wreceived_one.c_str(), L"继续") == 0)
	{
		//第二个判断//
		//客户端密码设置选择判断 完成或取消 //
		int len_two = 0;//接收客户端得到账号后的选择，继续 或 取消//
		recvAll(client_server, (char*)&len_two, sizeof(len_two), 0);//先接收字节长度//
		if ((int)len_two <= 0)
		{
			MessageBox(NULL, L"接收'客户端密码设置选择判断 完成或取消'长度错误", L"QQ", MB_ICONERROR);//标识长度接收错误提示//
			return;
		}
		std::string received_two(len_two, 0);//分配空间//
		recvAll(client_server, &received_two[0], len_two, 0);//接受标识内容//
		int wlen_two = MultiByteToWideChar(CP_UTF8, 0, &received_two[0], len_two, NULL, 0);//计算转换长度//
		std::wstring wreceived_two(wlen_two, 0);
		MultiByteToWideChar(CP_UTF8, 0, &received_two[0], len_two, &wreceived_two[0], wlen_two);//转换为宽字符数组//
		if (wcscmp(wreceived_two.c_str(), L"取消") == 0)
		{
			std::thread(HandleClient_register, client_server).detach();//跟随客户端返回到注册账号生成处//
		}
		else if (wcscmp(wreceived_two.c_str(), L"完成") == 0)
		{
			//第三个判断//
			//客户端个人信息设置选择判断 确定或取消 //
			int len_three = 0;//接收客户端得到账号后的选择，继续 或 取消//
			recvAll(client_server, (char*)&len_three, sizeof(len_three), 0);//先接收字节长度//
			if (len_three <= 0)
			{
				MessageBox(NULL, L"接收'客户端密码设置选择判断 完成或取消'长度错误", L"QQ", MB_ICONERROR);//标识长度接收错误提示//
				return;
			}
			std::string received_three(len_three, 0);//分配空间//
			recvAll(client_server, &received_three[0], len_three, 0);//接受标识内容//
			int wlen_three = MultiByteToWideChar(CP_UTF8, 0, &received_three[0], len_three, NULL, 0);//计算转换长度//
			std::wstring wreceived_three(wlen_three, 0);
			MultiByteToWideChar(CP_UTF8, 0, &received_three[0], len_three, &wreceived_three[0], wlen_three);//转换为宽字符数组//
			if (wcscmp(wreceived_three.c_str(), L"取消") == 0)
			{
				MessageBox(NULL, L"服务器即将由个人信息流程返回密码设置线程", L"QQ", MB_ICONINFORMATION);
				std::thread(HandleClient_passwordset, client_server).detach();//返回到密码设置线程//
			}
			else if (wcscmp(wreceived_three.c_str(), L"确定") == 0)
			{
				//第四个判断//
			   //客户端头像设置选择判断 确定或取消 //
				MessageBox(NULL, L"服务器即将由个人信息流程进入头像设置线程", L"QQ", MB_ICONINFORMATION);
				int len_four = 0;//接收客户端头像设置后的选择，继续 或 取消//
				recvAll(client_server, (char*)&len_four, sizeof(len_four), 0);//先接收字节长度//
				if ((int)len_four <= 0)
				{
					MessageBox(NULL, L"接收'客户端头像设置选择判断 确认或取消'长度错误", L"QQ", MB_ICONERROR);//标识长度接收错误提示//
					return;
				}
				std::string received_four(len_four, 0);//分配空间//
				recvAll(client_server, &received_four[0], len_four, 0);//接受标识内容//
				int wlen_four = MultiByteToWideChar(CP_UTF8, 0, &received_four[0], len_four, NULL, 0);//计算转换长度//
				std::wstring wreceived_four(wlen_four, 0);
				MultiByteToWideChar(CP_UTF8, 0, &received_four[0], len_four, &wreceived_four[0], wlen_four);//转换为宽字符数组//
				if (wcscmp(wreceived_four.c_str(), L"取消") == 0)
				{
					std::thread(HandleClient_informationset, client_server).detach();//返回到个人信息设置处//
				}
				else if (wcscmp(wreceived_four.c_str(), L"确定") == 0)
				{
					//第五个判断//
					//客户端注册成功选择判断 确定或取消 //
					int len_five = 0;//接收客户端得到账号后的选择，继续 或 取消//
					recvAll(client_server, (char*)&len_five, sizeof(len_five), 0);//先接收字节长度//
					if ((int)len_five <= 0)
					{
						MessageBox(NULL, L"接收'客户端注册成功选择判断 确认或取消'长度错误", L"QQ", MB_ICONERROR);//标识长度接收错误提示//
						return;
					}
					std::string received_five(len_five, 0);//分配空间//
					recvAll(client_server, &received_five[0], len_five, 0);//接受标识内容//
					int wlen_five = MultiByteToWideChar(CP_UTF8, 0, &received_five[0], len_five, NULL, 0);//计算转换长度//
					std::wstring wreceived_five(wlen_five, 0);
					MultiByteToWideChar(CP_UTF8, 0, &received_five[0], len_five, &wreceived_five[0], wlen_five);//转换为宽字符数组//
					if (wcscmp(wreceived_five.c_str(), L"取消") == 0)
					{
						std::thread(HandleClient_photoset, client_server).detach();//返回头像设置线程//
					}
					else if (wcscmp(wreceived_five.c_str(), L"确定") == 0)
					{
						//std::lock_guard<std::mutex>lk(users_mutex);
						//users_update_signal = true;
						//users_cv.notify_one();//通知函数重新加载数据库里的用户昵称和头像//
						
						int x = 0;
						receivedData regData;//结构体，接受完整数据//
						while (true)
						{
							int sign_strlen = 0;//接收标识长度//
							recvAll(client_server, (char*)&sign_strlen, sizeof(sign_strlen), 0);//先接收标识字节长度//
							if ((int)sign_strlen <= 0)
							{
								MessageBox(NULL, L"接收标识长度错误", L"QQ", MB_ICONERROR);//标识长度接收错误提示//
								return;
							}
							std::string sign_received(sign_strlen, 0);//分配空间//
							recvAll(client_server, &sign_received[0], sign_strlen, 0);//接受标识内容//
							int sign_wcharlen = MultiByteToWideChar(CP_UTF8, 0, &sign_received[0], sign_strlen, NULL, 0);//计算转换长度//
							std::wstring sign_wreceived(sign_wcharlen, 0);
							MultiByteToWideChar(CP_UTF8, 0, &sign_received[0], sign_strlen, &sign_wreceived[0], sign_wcharlen);//转换为宽字符数组//

							int con_strlen = 0;//接收内容长度//
							recvAll(client_server, (char*)&con_strlen, sizeof(con_strlen), 0);//先接收内容字节长度//
							if ((int)con_strlen <= 0)
							{
								MessageBox(NULL, L"接收内容长度错误", L"QQ", MB_ICONERROR);//内容长度接收错误提示//
								return;
							}

							if (wcscmp(sign_wreceived.c_str(), L"头像") != 0)//标识不是头像，而是账号、密码、昵称、性别、年龄和个性签名的时候就实行//
							{
								std::string received(con_strlen, 0);//分配空间//
								int rec = recvAll(client_server, &received[0], con_strlen, 0);//接受内容//
								int wcharlen = MultiByteToWideChar(CP_UTF8, 0, &received[0], con_strlen, NULL, 0);//计算转换长度//
								std::wstring wreceived(wcharlen, 0);
								MultiByteToWideChar(CP_UTF8, 0, &received[0], con_strlen, &wreceived[0], wcharlen);//转换为宽字符数组//
								if (!sign_wreceived.empty() && sign_wreceived.back() == L'\0')//去除标识字符串的终止符'\0',才可以拼接并显示后面的内容//
								{
									sign_wreceived.pop_back();
									//将数据存储到结构体，以便存储到数据库//
									if (wcscmp(sign_wreceived.c_str(), L"账号") == 0)
									{
										regData.account = received;
										regData.step++;
									}
									else if (wcscmp(sign_wreceived.c_str(), L"密码") == 0)
									{
										regData.password = received;
										regData.step++;
									}
									else if (wcscmp(sign_wreceived.c_str(), L"昵称") == 0)
									{
										regData.nickname = received;
										regData.step++;
									}
									else if (wcscmp(sign_wreceived.c_str(), L"性别") == 0)
									{
										regData.gender = received;
										regData.step++;
									}
									else if (wcscmp(sign_wreceived.c_str(), L"年龄") == 0)
									{
										regData.age = received;
										regData.step++;
									}
									else if (wcscmp(sign_wreceived.c_str(), L"个性签名") == 0)
									{
										regData.signature = received;
										regData.step++;
									}
								}
								//获取当前时间//
								time_t now = time(0);
								tm tm;
								localtime_s(&tm, &now);
								wchar_t timeBuffer[64];
								wcsftime(timeBuffer, sizeof(timeBuffer) / sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", &tm);
								std::wstring fullMessage = L"[";
								fullMessage += timeBuffer;
								fullMessage += L"][";
								fullMessage += sign_wreceived;
								fullMessage += L"]";
								fullMessage += wreceived;
								EnterCriticalSection(&g_csMsgQueue);//加锁//
								g_msgQueue.push(fullMessage);//创建副本//
								LeaveCriticalSection(&g_csMsgQueue);//解锁//
								if (x == 0)
								{
									MessageBox(NULL, L"服务器已成功接收来自客户端的信息", L"QQ", MB_ICONINFORMATION);//只提示一次//
									x = 1;
								}
								//通知现有对话框更新内容//
								PostMessage(g_hInfoDialog, WM_APP_UPDATE_MSG, 0, 0);
							}
							else if (wcscmp(sign_wreceived.c_str(), L"头像") == 0)
							{
								BYTE* received = new BYTE[con_strlen];//分配空间//
								int r = 0;//单次接收的字节数//
								int sum = 0;//累计接受的字节数//
								while (sum < con_strlen)
								{
									r = recv(client_server, (char*)received + sum, con_strlen - sum, 0);//接收图像二进制内容//
									if (r <= 0)
									{
										break;//接收结束//
									}
									sum += r;
								}
								//将数据存储到结构体，以便存储到数据库//
								regData.imgData.assign(received, received + sum);
								regData.step++;
								if (sum == con_strlen)
								{
									std::vector<BYTE>imgData(received, received + sum);//图片数据存储到imgData容器//
									std::lock_guard<std::mutex>lock(g_imgQueueMutex);//线程安全//
									g_imgQueue.push(std::move(imgData));//图片加入队列//
									delete[]received;
								}
								//通知现有图片框更新内容//
								PostMessage(g_hInfoDialogphoto, WM_APP_UPDATEPHOTO_MSG, 0, 0);
							}
							if (regData.step == 7)
							{
								SaveToDatabase(regData); // 将结构体数据保存到数据库//
								regData = receivedData(); // 清空，准备下一个用户//
								MessageBox(NULL, L"服务器已成功将接收的注册数据存储到数据库", L"QQ", MB_ICONINFORMATION);
								break;
							}
						}
						update_new_users_list();//通知函数重新加载数据库里的用户昵称和头像//
						//跳出注册线程//
					   //判断具体执行哪种线程,登录或注册//
						int recv_len = 0;
						recvAll(client_server, (char*)&recv_len, sizeof(recv_len), 0);//接收消息长度//
						if (recv_len <= 0)
						{
							//MessageBox(NULL, L"接收线程类型的消息长度失败", L"QQ", MB_ICONERROR);
							return;
						}
						//MessageBox(NULL, L"接收线程类型的消息长度成功", L"QQ", MB_ICONINFORMATION);
						std::string recvchar(recv_len, 0);
						int r = 0;
						r = recvAll(client_server, &recvchar[0], recv_len, 0);

						if (r <= 0)
						{
							//MessageBox(NULL, L"接收线程类型的消息失败", L"QQ", MB_ICONERROR);
							return;
						}

						//MessageBox(NULL, L"接收线程类型的消息成功", L"QQ", MB_ICONINFORMATION);
						int wlen = MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, NULL, 0);//转换为宽字节所需要的字节数//
						std::wstring wrecvchar(wlen, 0);//分配空间//
						MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, &wrecvchar[0], wlen);//实际转换//
						//注册处理//
						if (wcscmp(wrecvchar.c_str(), L"注册") == 0)//注册线程//
						{
							//MessageBox(NULL, L"接收到注册消息，即将启用注册线程", L"QQ", MB_ICONINFORMATION);
							std::thread(HandleClient_register, client_server).detach();//创建注册线程//
						}
						//登录处理//
						else if (wcscmp(wrecvchar.c_str(), L"登录") == 0)//登录线程//
						{
							//MessageBox(NULL, L"接收到登录消息，即将启用登录线程", L"QQ", MB_ICONINFORMATION);
							std::thread(HandleClient_login, client_server,new_online_user_account).detach();//创建登录线程//
						}
						else
						{
							//MessageBox(NULL, L"接收不到线程消息，即将退出", L"QQ", MB_ICONINFORMATION);
							return;
						}
						//MessageBox(NULL, L"服务器已无法接收来自客户端的信息", L"QQ", MB_ICONERROR);
						//closesocket(client_server);
					}
				}
			}
		}
	}
}


//客户端密码设置选择判断线程//
void HandleClient_passwordset(SOCKET client_server)
{
	std::string new_online_user_account;
	//第二个判断//
	//客户端密码设置选择判断 完成或取消 //
	int len_two = 0;//接收客户端密码设置后的选择，完成 或 取消//
	MessageBox(NULL, L"服务器即将开始接收来自密码框的判断 完成或取消", L"QQ", MB_ICONINFORMATION);
	recvAll(client_server, (char*)&len_two, sizeof(len_two), 0);//先接收字节长度//
	if ((int)len_two <= 0)
	{
		MessageBox(NULL, L"接收'客户端密码设置选择判断 完成或取消'长度错误", L"QQ", MB_ICONERROR);//标识长度接收错误提示//
		return;
	}
	std::string received_two(len_two, 0);//分配空间//
	recvAll(client_server, &received_two[0], len_two, 0);//接受标识内容//
	int wlen_two = MultiByteToWideChar(CP_UTF8, 0, &received_two[0], len_two, NULL, 0);//计算转换长度//
	std::wstring wreceived_two(wlen_two, 0);
	MultiByteToWideChar(CP_UTF8, 0, &received_two[0], len_two, &wreceived_two[0], wlen_two);//转换为宽字符数组//
	if (wcscmp(wreceived_two.c_str(), L"取消") == 0)
	{
		std::thread(HandleClient_register, client_server).detach();//跟随客户端返回到注册账号生成处//
	}
	else if (wcscmp(wreceived_two.c_str(), L"完成") == 0)//客户端密码设置选择判断 确定或取消 //
	{
		//第三个判断//
		//客户端个人信息设置选择判断 确定或取消 //
		int len_three = 0;//接收客户端得到账号后的选择，继续 或 取消//
		recvAll(client_server, (char*)&len_three, sizeof(len_three), 0);//先接收字节长度//
		if ((int)len_three <= 0)
		{
			MessageBox(NULL, L"接收'客户端个人信息设置选择判断 确认或取消'长度错误", L"QQ", MB_ICONERROR);//标识长度接收错误提示//
			return;
		}
		std::string received_three(len_three, 0);//分配空间//
		recvAll(client_server, &received_three[0], len_three, 0);//接受标识内容//
		int wlen_three = MultiByteToWideChar(CP_UTF8, 0, &received_three[0], len_three, NULL, 0);//计算转换长度//
		std::wstring wreceived_three(wlen_three, 0);
		MultiByteToWideChar(CP_UTF8, 0, &received_three[0], len_three, &wreceived_three[0], wlen_three);//转换为宽字符数组//
		if (wcscmp(wreceived_three.c_str(), L"取消") == 0)
		{
			std::thread(HandleClient_passwordset, client_server).detach();//返回到客户端密码设置处//
		}
		else if (wcscmp(wreceived_three.c_str(), L"确定") == 0)
		{
			//第四个判断//
			//客户端头像设置选择判断 确定或取消 //
			int len_four = 0;//接收客户端头像设置后的选择，确定 或 取消//
			recvAll(client_server, (char*)&len_four, sizeof(len_four), 0);//先接收字节长度//
			if ((int)len_four <= 0)
			{
				MessageBox(NULL, L"接收'客户端头像设置选择判断 确认或取消'长度错误", L"QQ", MB_ICONERROR);//标识长度接收错误提示//
				return;
			}
			std::string received_four(len_four, 0);//分配空间//
			recvAll(client_server, &received_four[0], len_four, 0);//接受标识内容//
			int wlen_four = MultiByteToWideChar(CP_UTF8, 0, &received_four[0], len_four, NULL, 0);//计算转换长度//
			std::wstring wreceived_four(wlen_four, 0);
			MultiByteToWideChar(CP_UTF8, 0, &received_four[0], len_four, &wreceived_four[0], wlen_four);//转换为宽字符数组//
			if (wcscmp(wreceived_four.c_str(), L"取消") == 0)
			{
				std::thread(HandleClient_informationset, client_server).detach();//返回到个人信息设置处//
			}
			else if (wcscmp(wreceived_four.c_str(), L"确定") == 0)
			{

				//第五个判断//
				//客户端注册成功选择判断 确定或取消 //
				int len_five = 0;//接收客户端得到账号后的选择，继续 或 取消//
				recvAll(client_server, (char*)&len_five, sizeof(len_five), 0);//先接收字节长度//
				if ((int)len_five <= 0)
				{
					MessageBox(NULL, L"接收'客户端注册成功选择判断 确认或取消'长度错误", L"QQ", MB_ICONERROR);//标识长度接收错误提示//
					return;
				}
				std::string received_five(len_five, 0);//分配空间//
				recvAll(client_server, &received_five[0], len_five, 0);//接受标识内容//
				int wlen_five = MultiByteToWideChar(CP_UTF8, 0, &received_five[0], len_five, NULL, 0);//计算转换长度//
				std::wstring wreceived_five(wlen_five, 0);
				MultiByteToWideChar(CP_UTF8, 0, &received_five[0], len_five, &wreceived_five[0], wlen_five);//转换为宽字符数组//
				if (wcscmp(wreceived_five.c_str(), L"取消") == 0)
				{
					std::thread(HandleClient_photoset, client_server).detach();//返回头像设置线程//
				}
				else if (wcscmp(wreceived_five.c_str(), L"确定") == 0)
				{
					//std::lock_guard<std::mutex>lk(users_mutex);
					//users_update_signal = true;
					//users_cv.notify_one();//通知函数重新加载数据库里的用户昵称和头像//
					//update_new_users_list();//通知函数重新加载数据库里的用户昵称和头像//
					int x = 0;
					receivedData regData;//结构体，接受完整数据//
					while (true)
					{
						int sign_strlen = 0;//接收标识长度//
						recvAll(client_server, (char*)&sign_strlen, sizeof(sign_strlen), 0);//先接收标识字节长度//
						if ((int)sign_strlen <= 0)
						{
							MessageBox(NULL, L"接收标识长度错误", L"QQ", MB_ICONERROR);//标识长度接收错误提示//
							return;
						}
						std::string sign_received(sign_strlen, 0);//分配空间//
						recvAll(client_server, &sign_received[0], sign_strlen, 0);//接受标识内容//
						int sign_wcharlen = MultiByteToWideChar(CP_UTF8, 0, &sign_received[0], sign_strlen, NULL, 0);//计算转换长度//
						std::wstring sign_wreceived(sign_wcharlen, 0);
						MultiByteToWideChar(CP_UTF8, 0, &sign_received[0], sign_strlen, &sign_wreceived[0], sign_wcharlen);//转换为宽字符数组//

						int con_strlen = 0;//接收内容长度//
						recvAll(client_server, (char*)&con_strlen, sizeof(con_strlen), 0);//先接收内容字节长度//
						if ((int)con_strlen <= 0)
						{
							MessageBox(NULL, L"接收内容长度错误", L"QQ", MB_ICONERROR);//内容长度接收错误提示//
							return;
						}

						if (wcscmp(sign_wreceived.c_str(), L"头像") != 0)//标识不是头像，而是账号、密码、昵称、性别、年龄和个性签名的时候就实行//
						{
							std::string received(con_strlen, 0);//分配空间//
							int rec = recvAll(client_server, &received[0], con_strlen, 0);//接受内容//
							int wcharlen = MultiByteToWideChar(CP_UTF8, 0, &received[0], con_strlen, NULL, 0);//计算转换长度//
							std::wstring wreceived(wcharlen, 0);
							MultiByteToWideChar(CP_UTF8, 0, &received[0], con_strlen, &wreceived[0], wcharlen);//转换为宽字符数组//
							if (!sign_wreceived.empty() && sign_wreceived.back() == L'\0')//去除标识字符串的终止符'\0',才可以拼接并显示后面的内容//
							{
								sign_wreceived.pop_back();
								//将数据存储到结构体，以便存储到数据库//
								if (wcscmp(sign_wreceived.c_str(), L"账号") == 0)
								{
									regData.account = received;
									regData.step++;
								}
								else if (wcscmp(sign_wreceived.c_str(), L"密码") == 0)
								{
									regData.password = received;
									regData.step++;
								}
								else if (wcscmp(sign_wreceived.c_str(), L"昵称") == 0)
								{
									regData.nickname = received;
									regData.step++;
								}
								else if (wcscmp(sign_wreceived.c_str(), L"性别") == 0)
								{
									regData.gender = received;
									regData.step++;
								}
								else if (wcscmp(sign_wreceived.c_str(), L"年龄") == 0)
								{
									regData.age = received;
									regData.step++;
								}
								else if (wcscmp(sign_wreceived.c_str(), L"个性签名") == 0)
								{
									regData.signature = received;
									regData.step++;
								}
							}
							//获取当前时间//
							time_t now = time(0);
							tm tm;
							localtime_s(&tm, &now);
							wchar_t timeBuffer[64];
							wcsftime(timeBuffer, sizeof(timeBuffer) / sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", &tm);
							std::wstring fullMessage = L"[";
							fullMessage += timeBuffer;
							fullMessage += L"][";
							fullMessage += sign_wreceived;
							fullMessage += L"]";
							fullMessage += wreceived;
							EnterCriticalSection(&g_csMsgQueue);//加锁//
							g_msgQueue.push(fullMessage);//创建副本//
							LeaveCriticalSection(&g_csMsgQueue);//解锁//
							if (x == 0)
							{
								MessageBox(NULL, L"服务器已成功接收来自客户端的信息", L"QQ", MB_ICONINFORMATION);//只提示一次//
								x = 1;
							}
							//通知现有对话框更新内容//
							PostMessage(g_hInfoDialog, WM_APP_UPDATE_MSG, 0, 0);
						}
						else if (wcscmp(sign_wreceived.c_str(), L"头像") == 0)
						{
							BYTE* received = new BYTE[con_strlen];//分配空间//
							int r = 0;//单次接收的字节数//
							int sum = 0;//累计接受的字节数//
							while (sum < con_strlen)
							{
								r = recv(client_server, (char*)received + sum, con_strlen - sum, 0);//接收图像二进制内容//
								if (r <= 0)
								{
									break;//接收结束//
								}
								sum += r;
							}
							//将数据存储到结构体，以便存储到数据库//
							regData.imgData.assign(received, received + sum);
							regData.step++;
							if (sum == con_strlen)
							{
								std::vector<BYTE>imgData(received, received + sum);//图片数据存储到imgData容器//
								std::lock_guard<std::mutex>lock(g_imgQueueMutex);//线程安全//
								g_imgQueue.push(std::move(imgData));//图片加入队列//
								delete[]received;
							}
							//通知现有图片框更新内容//
							PostMessage(g_hInfoDialogphoto, WM_APP_UPDATEPHOTO_MSG, 0, 0);
						}
						if (regData.step == 7)
						{
							SaveToDatabase(regData); // 将结构体数据保存到数据库//
							regData = receivedData(); // 清空，准备下一个用户//
							MessageBox(NULL, L"服务器已成功将接收的注册数据存储到数据库", L"QQ", MB_ICONINFORMATION);
							break;
						}
					}
					update_new_users_list();//通知函数重新加载数据库里的用户昵称和头像//
					//跳出注册线程//
				   //判断具体执行哪种线程,登录或注册//
					int recv_len = 0;
					recv(client_server, (char*)&recv_len, sizeof(recv_len), 0);//接收消息长度//
					if (recv_len <= 0)
					{
						//MessageBox(NULL, L"接收线程类型的消息长度失败", L"QQ", MB_ICONERROR);
						return;
					}
					//MessageBox(NULL, L"接收线程类型的消息长度成功", L"QQ", MB_ICONINFORMATION);
					std::string recvchar(recv_len, 0);
					int r = 0;
					r = recv(client_server, &recvchar[0], recv_len, 0);

					if (r <= 0)
					{
						//MessageBox(NULL, L"接收线程类型的消息失败", L"QQ", MB_ICONERROR);
						return;
					}

					//MessageBox(NULL, L"接收线程类型的消息成功", L"QQ", MB_ICONINFORMATION);
					int wlen = MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, NULL, 0);//转换为宽字节所需要的字节数//
					std::wstring wrecvchar(wlen, 0);//分配空间//
					MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, &wrecvchar[0], wlen);//实际转换//
					//注册处理//
					if (wcscmp(wrecvchar.c_str(), L"注册") == 0)//注册线程//
					{
						//MessageBox(NULL, L"接收到注册消息，即将启用注册线程", L"QQ", MB_ICONINFORMATION);
						std::thread(HandleClient_register, client_server).detach();//创建注册线程//
					}
					//登录处理//
					else if (wcscmp(wrecvchar.c_str(), L"登录") == 0)//登录线程//
					{
						//MessageBox(NULL, L"接收到登录消息，即将启用登录线程", L"QQ", MB_ICONINFORMATION);
						std::thread(HandleClient_login, client_server,new_online_user_account).detach();//创建登录线程//
					}
					else
					{
						//MessageBox(NULL, L"接收不到线程消息，即将退出", L"QQ", MB_ICONINFORMATION);
						return;
					}
					//MessageBox(NULL, L"服务器已无法接收来自客户端的信息", L"QQ", MB_ICONERROR);
				   //closesocket(client_server);
				}
			}
		}
	}
}


//个人信息框设置线程//
void HandleClient_informationset(SOCKET client_server)
{
	std::string new_online_user_account;
	MessageBox(NULL, L"你即将启动个人信息设置线程", L"QQ", MB_ICONINFORMATION);
	//第三个判断//
	//客户端个人信息设置选择判断 确定或取消 //
	int len_three = 0;//接收客户端个人信息设置后的选择，继续 或 取消//
	recvAll(client_server, (char*)&len_three, sizeof(len_three), 0);//先接收字节长度//
	MessageBox(NULL, L"你已接收选择字段长度", L"QQ", MB_ICONINFORMATION);
	if (len_three <= 0)
	{
		MessageBox(NULL, L"接收'客户端个人信息设置选择判断 确认或取消'长度错误", L"QQ", MB_ICONERROR);//标识长度接收错误提示//
		return;
	}
	std::string received_three(len_three, 0);//分配空间//
	recvAll(client_server, &received_three[0], len_three, 0);//接受标识内容//
	int wlen_three = MultiByteToWideChar(CP_UTF8, 0, &received_three[0], len_three, NULL, 0);//计算转换长度//
	std::wstring wreceived_three(wlen_three, 0);
	MultiByteToWideChar(CP_UTF8, 0, &received_three[0], len_three, &wreceived_three[0], wlen_three);//转换为宽字符数组//
	if (wcscmp(wreceived_three.c_str(), L"取消") == 0)
	{
		std::thread(HandleClient_passwordset, client_server).detach();//返回到客户端密码设置处//
	}
	else if (wcscmp(wreceived_three.c_str(), L"确定") == 0)
	{
		//第四个判断//
		//客户端头像设置选择判断 确定或取消 //
		int len_four = 0;//接收客户端得到账号后的选择，继续 或 取消//
		recvAll(client_server, (char*)&len_four, sizeof(len_four), 0);//先接收字节长度//
		if ((int)len_four <= 0)
		{
			MessageBox(NULL, L"接收'客户端头像设置选择判断 确认或取消'长度错误", L"QQ", MB_ICONERROR);//标识长度接收错误提示//
			return;
		}
		std::string received_four(len_four, 0);//分配空间//
		recvAll(client_server, &received_four[0], len_four, 0);//接受标识内容//
		int wlen_four = MultiByteToWideChar(CP_UTF8, 0, &received_four[0], len_four, NULL, 0);//计算转换长度//
		std::wstring wreceived_four(wlen_four, 0);
		MultiByteToWideChar(CP_UTF8, 0, &received_four[0], len_four, &wreceived_four[0], wlen_four);//转换为宽字符数组//
		if (wcscmp(wreceived_four.c_str(), L"取消") == 0)
		{
			std::thread(HandleClient_informationset, client_server).detach();//返回到个人信息设置处//
		}
		else if (wcscmp(wreceived_four.c_str(), L"确定") == 0)
		{
			//第五个判断//
		   //客户端注册成功选择判断 确定或取消 //
			int len_five = 0;//接收客户端得到账号后的选择，继续 或 取消//
			recvAll(client_server, (char*)&len_five, sizeof(len_five), 0);//先接收字节长度//
			if ((int)len_five <= 0)
			{
				MessageBox(NULL, L"接收'客户端注册成功选择判断 确认或取消'长度错误", L"QQ", MB_ICONERROR);//标识长度接收错误提示//
				return;
			}
			std::string received_five(len_five, 0);//分配空间//
			recvAll(client_server, &received_five[0], len_five, 0);//接受标识内容//
			int wlen_five = MultiByteToWideChar(CP_UTF8, 0, &received_five[0], len_five, NULL, 0);//计算转换长度//
			std::wstring wreceived_five(wlen_five, 0);
			MultiByteToWideChar(CP_UTF8, 0, &received_five[0], len_five, &wreceived_five[0], wlen_five);//转换为宽字符数组//
			if (wcscmp(wreceived_five.c_str(), L"取消") == 0)
			{
				std::thread(HandleClient_photoset, client_server).detach();//返回头像设置线程//
			}
			else if (wcscmp(wreceived_five.c_str(), L"确定") == 0)
			{
				//std::lock_guard<std::mutex>lk(users_mutex);
				//users_update_signal = true;
				//users_cv.notify_one();//通知函数重新加载数据库里的用户昵称和头像//
				//update_new_users_list();//通知函数重新加载数据库里的用户昵称和头像//
				int x = 0;
				receivedData regData;//结构体，接受完整数据//
				while (true)
				{
					int sign_strlen = 0;//接收标识长度//
					recvAll(client_server, (char*)&sign_strlen, sizeof(sign_strlen), 0);//先接收标识字节长度//
					if ((int)sign_strlen <= 0)
					{
						MessageBox(NULL, L"接收标识长度错误", L"QQ", MB_ICONERROR);//标识长度接收错误提示//
						return;
					}
					std::string sign_received(sign_strlen, 0);//分配空间//
					recvAll(client_server, &sign_received[0], sign_strlen, 0);//接受标识内容//
					int sign_wcharlen = MultiByteToWideChar(CP_UTF8, 0, &sign_received[0], sign_strlen, NULL, 0);//计算转换长度//
					std::wstring sign_wreceived(sign_wcharlen, 0);
					MultiByteToWideChar(CP_UTF8, 0, &sign_received[0], sign_strlen, &sign_wreceived[0], sign_wcharlen);//转换为宽字符数组//

					int con_strlen = 0;//接收内容长度//
					recvAll(client_server, (char*)&con_strlen, sizeof(con_strlen), 0);//先接收内容字节长度//
					if ((int)con_strlen <= 0)
					{
						MessageBox(NULL, L"接收内容长度错误", L"QQ", MB_ICONERROR);//内容长度接收错误提示//
						return;
					}

					if (wcscmp(sign_wreceived.c_str(), L"头像") != 0)//标识不是头像，而是账号、密码、昵称、性别、年龄和个性签名的时候就实行//
					{
						std::string received(con_strlen, 0);//分配空间//
						int rec = recvAll(client_server, &received[0], con_strlen, 0);//接受内容//
						int wcharlen = MultiByteToWideChar(CP_UTF8, 0, &received[0], con_strlen, NULL, 0);//计算转换长度//
						std::wstring wreceived(wcharlen, 0);
						MultiByteToWideChar(CP_UTF8, 0, &received[0], con_strlen, &wreceived[0], wcharlen);//转换为宽字符数组//
						if (!sign_wreceived.empty() && sign_wreceived.back() == L'\0')//去除标识字符串的终止符'\0',才可以拼接并显示后面的内容//
						{
							sign_wreceived.pop_back();
							//将数据存储到结构体，以便存储到数据库//
							if (wcscmp(sign_wreceived.c_str(), L"账号") == 0)
							{
								regData.account = received;
								regData.step++;
							}
							else if (wcscmp(sign_wreceived.c_str(), L"密码") == 0)
							{
								regData.password = received;
								regData.step++;
							}
							else if (wcscmp(sign_wreceived.c_str(), L"昵称") == 0)
							{
								regData.nickname = received;
								regData.step++;
							}
							else if (wcscmp(sign_wreceived.c_str(), L"性别") == 0)
							{
								regData.gender = received;
								regData.step++;
							}
							else if (wcscmp(sign_wreceived.c_str(), L"年龄") == 0)
							{
								regData.age = received;
								regData.step++;
							}
							else if (wcscmp(sign_wreceived.c_str(), L"个性签名") == 0)
							{
								regData.signature = received;
								regData.step++;
							}
						}
						//获取当前时间//
						time_t now = time(0);
						tm tm;
						localtime_s(&tm, &now);
						wchar_t timeBuffer[64];
						wcsftime(timeBuffer, sizeof(timeBuffer) / sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", &tm);
						std::wstring fullMessage = L"[";
						fullMessage += timeBuffer;
						fullMessage += L"][";
						fullMessage += sign_wreceived;
						fullMessage += L"]";
						fullMessage += wreceived;
						EnterCriticalSection(&g_csMsgQueue);//加锁//
						g_msgQueue.push(fullMessage);//创建副本//
						LeaveCriticalSection(&g_csMsgQueue);//解锁//
						if (x == 0)
						{
							MessageBox(NULL, L"服务器已成功接收来自客户端的信息", L"QQ", MB_ICONINFORMATION);//只提示一次//
							x = 1;
						}
						//通知现有对话框更新内容//
						PostMessage(g_hInfoDialog, WM_APP_UPDATE_MSG, 0, 0);
					}
					else if (wcscmp(sign_wreceived.c_str(), L"头像") == 0)
					{
						BYTE* received = new BYTE[con_strlen];//分配空间//
						int r = 0;//单次接收的字节数//
						int sum = 0;//累计接受的字节数//
						while (sum < con_strlen)
						{
							r = recv(client_server, (char*)received + sum, con_strlen - sum, 0);//接收图像二进制内容//
							if (r <= 0)
							{
								break;//接收结束//
							}
							sum += r;
						}
						//将数据存储到结构体，以便存储到数据库//
						regData.imgData.assign(received, received + sum);
						regData.step++;
						if (sum == con_strlen)
						{
							std::vector<BYTE>imgData(received, received + sum);//图片数据存储到imgData容器//
							std::lock_guard<std::mutex>lock(g_imgQueueMutex);//线程安全//
							g_imgQueue.push(std::move(imgData));//图片加入队列//
							delete[]received;
						}
						//通知现有图片框更新内容//
						PostMessage(g_hInfoDialogphoto, WM_APP_UPDATEPHOTO_MSG, 0, 0);
					}
					if (regData.step == 7)
					{
						SaveToDatabase(regData); // 将结构体数据保存到数据库//
						regData = receivedData(); // 清空，准备下一个用户//
						MessageBox(NULL, L"服务器已成功将接收的注册数据存储到数据库", L"QQ", MB_ICONINFORMATION);
						break;
					}
				}
				update_new_users_list();//通知函数重新加载数据库里的用户昵称和头像//
				//跳出注册线程//
			   //判断具体执行哪种线程,登录或注册//
				int recv_len = 0;
				recv(client_server, (char*)&recv_len, sizeof(recv_len), 0);//接收消息长度//
				if (recv_len <= 0)
				{
					//MessageBox(NULL, L"接收线程类型的消息长度失败", L"QQ", MB_ICONERROR);
					return;
				}
				//MessageBox(NULL, L"接收线程类型的消息长度成功", L"QQ", MB_ICONINFORMATION);
				std::string recvchar(recv_len, 0);
				int r = 0;
				r = recv(client_server, &recvchar[0], recv_len, 0);
				{
					if (r <= 0)
					{
						//MessageBox(NULL, L"接收线程类型的消息失败", L"QQ", MB_ICONERROR);
						return;
					}
				}
				//MessageBox(NULL, L"接收线程类型的消息成功", L"QQ", MB_ICONINFORMATION);
				int wlen = MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, NULL, 0);//转换为宽字节所需要的字节数//
				std::wstring wrecvchar(wlen, 0);//分配空间//
				MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, &wrecvchar[0], wlen);//实际转换//
				//注册处理//
				if (wcscmp(wrecvchar.c_str(), L"注册") == 0)//注册线程//
				{
					//MessageBox(NULL, L"接收到注册消息，即将启用注册线程", L"QQ", MB_ICONINFORMATION);
					std::thread(HandleClient_register, client_server).detach();//创建注册线程//
				}
				//登录处理//
				else if (wcscmp(wrecvchar.c_str(), L"登录") == 0)//登录线程//
				{
					//MessageBox(NULL, L"接收到登录消息，即将启用登录线程", L"QQ", MB_ICONINFORMATION);
					std::thread(HandleClient_login, client_server,new_online_user_account).detach();//创建登录线程//
				}
				else
				{
					//MessageBox(NULL, L"接收不到线程消息，即将退出", L"QQ", MB_ICONINFORMATION);
					return;
				}
				//MessageBox(NULL, L"服务器已无法接收来自客户端的信息", L"QQ", MB_ICONERROR);
				//closesocket(client_server);
			}
		}
	}
}

//头像设置线程//
void HandleClient_photoset(SOCKET client_server)
{
	std::string new_online_user_account;
	//第四个判断//
	//客户端头像设置选择判断 确定或取消 //
	int len_four = 0;//接收客户端头像设置后的选择，继续 或 取消//
	recvAll(client_server, (char*)&len_four, sizeof(len_four), 0);//先接收字节长度//
	if ((int)len_four <= 0)
	{
		MessageBox(NULL, L"接收'客户端头像设置选择判断 确认或取消'长度错误", L"QQ", MB_ICONERROR);//标识长度接收错误提示//
		return;
	}
	std::string received_four(len_four, 0);//分配空间//
	recvAll(client_server, &received_four[0], len_four, 0);//接受标识内容//
	int wlen_four = MultiByteToWideChar(CP_UTF8, 0, &received_four[0], len_four, NULL, 0);//计算转换长度//
	std::wstring wreceived_four(wlen_four, 0);
	MultiByteToWideChar(CP_UTF8, 0, &received_four[0], len_four, &wreceived_four[0], wlen_four);//转换为宽字符数组//
	if (wcscmp(wreceived_four.c_str(), L"取消") == 0)
	{
		std::thread(HandleClient_informationset, client_server).detach();//返回到个人信息设置处//
	}
	else if (wcscmp(wreceived_four.c_str(), L"确定") == 0)
	{
		//第五个判断//
		//客户端注册成功选择判断 确定或取消 //
		int len_five = 0;//接收客户端得到账号后的选择，继续 或 取消//
		recvAll(client_server, (char*)&len_five, sizeof(len_five), 0);//先接收字节长度//
		if ((int)len_five <= 0)
		{
			MessageBox(NULL, L"接收'客户端注册成功选择判断 确认或取消'长度错误", L"QQ", MB_ICONERROR);//标识长度接收错误提示//
			return;
		}
		std::string received_five(len_five, 0);//分配空间//
		recvAll(client_server, &received_five[0], len_five, 0);//接受标识内容//
		int wlen_five = MultiByteToWideChar(CP_UTF8, 0, &received_five[0], len_five, NULL, 0);//计算转换长度//
		std::wstring wreceived_five(wlen_five, 0);
		MultiByteToWideChar(CP_UTF8, 0, &received_five[0], len_five, &wreceived_five[0], wlen_five);//转换为宽字符数组//
		if (wcscmp(wreceived_five.c_str(), L"取消") == 0)
		{
			std::thread(HandleClient_photoset, client_server).detach();//返回头像设置线程//
		}
		else if (wcscmp(wreceived_five.c_str(), L"确定") == 0)//注册成功确定线程//
		{
			//std::lock_guard<std::mutex>lk(users_mutex);
			//users_update_signal = true;
			//users_cv.notify_one();//通知函数重新加载数据库里的用户昵称和头像//
			//update_new_users_list();//通知函数重新加载数据库里的用户昵称和头像//
			int x = 0;
			receivedData regData;//结构体，接受完整数据//
			while (true)
			{
				int sign_strlen = 0;//接收标识长度//
				recvAll(client_server, (char*)&sign_strlen, sizeof(sign_strlen), 0);//先接收标识字节长度//
				if ((int)sign_strlen <= 0)
				{
					MessageBox(NULL, L"接收标识长度错误", L"QQ", MB_ICONERROR);//标识长度接收错误提示//
					return;
				}
				std::string sign_received(sign_strlen, 0);//分配空间//
				recvAll(client_server, &sign_received[0], sign_strlen, 0);//接受标识内容//
				int sign_wcharlen = MultiByteToWideChar(CP_UTF8, 0, &sign_received[0], sign_strlen, NULL, 0);//计算转换长度//
				std::wstring sign_wreceived(sign_wcharlen, 0);
				MultiByteToWideChar(CP_UTF8, 0, &sign_received[0], sign_strlen, &sign_wreceived[0], sign_wcharlen);//转换为宽字符数组//

				int con_strlen = 0;//接收内容长度//
				recvAll(client_server, (char*)&con_strlen, sizeof(con_strlen), 0);//先接收内容字节长度//
				if ((int)con_strlen <= 0)
				{
					MessageBox(NULL, L"接收内容长度错误", L"QQ", MB_ICONERROR);//内容长度接收错误提示//
					return;
				}

				if (wcscmp(sign_wreceived.c_str(), L"头像") != 0)//标识不是头像，而是账号、密码、昵称、性别、年龄和个性签名的时候就实行//
				{
					std::string received(con_strlen, 0);//分配空间//
					int rec = recvAll(client_server, &received[0], con_strlen, 0);//接受内容//
					int wcharlen = MultiByteToWideChar(CP_UTF8, 0, &received[0], con_strlen, NULL, 0);//计算转换长度//
					std::wstring wreceived(wcharlen, 0);
					MultiByteToWideChar(CP_UTF8, 0, &received[0], con_strlen, &wreceived[0], wcharlen);//转换为宽字符数组//
					if (!sign_wreceived.empty() && sign_wreceived.back() == L'\0')//去除标识字符串的终止符'\0',才可以拼接并显示后面的内容//
					{
						sign_wreceived.pop_back();
						//将数据存储到结构体，以便存储到数据库//
						if (wcscmp(sign_wreceived.c_str(), L"账号") == 0)
						{
							regData.account = received;
							regData.step++;
						}
						else if (wcscmp(sign_wreceived.c_str(), L"密码") == 0)
						{
							regData.password = received;
							regData.step++;
						}
						else if (wcscmp(sign_wreceived.c_str(), L"昵称") == 0)
						{
							regData.nickname = received;
							regData.step++;
						}
						else if (wcscmp(sign_wreceived.c_str(), L"性别") == 0)
						{
							regData.gender = received;
							regData.step++;
						}
						else if (wcscmp(sign_wreceived.c_str(), L"年龄") == 0)
						{
							regData.age = received;
							regData.step++;
						}
						else if (wcscmp(sign_wreceived.c_str(), L"个性签名") == 0)
						{
							regData.signature = received;
							regData.step++;
						}
					}
					//获取当前时间//
					time_t now = time(0);
					tm tm;
					localtime_s(&tm, &now);
					wchar_t timeBuffer[64];
					wcsftime(timeBuffer, sizeof(timeBuffer) / sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", &tm);
					std::wstring fullMessage = L"[";
					fullMessage += timeBuffer;
					fullMessage += L"][";
					fullMessage += sign_wreceived;
					fullMessage += L"]";
					fullMessage += wreceived;
					EnterCriticalSection(&g_csMsgQueue);//加锁//
					g_msgQueue.push(fullMessage);//创建副本//
					LeaveCriticalSection(&g_csMsgQueue);//解锁//
					if (x == 0)
					{
						MessageBox(NULL, L"服务器已成功接收来自客户端的信息", L"QQ", MB_ICONINFORMATION);//只提示一次//
						x = 1;
					}
					//通知现有对话框更新内容//
					PostMessage(g_hInfoDialog, WM_APP_UPDATE_MSG, 0, 0);
				}
				else if (wcscmp(sign_wreceived.c_str(), L"头像") == 0)
				{
					BYTE* received = new BYTE[con_strlen];//分配空间//
					int r = 0;//单次接收的字节数//
					int sum = 0;//累计接受的字节数//
					while (sum < con_strlen)
					{
						r = recv(client_server, (char*)received + sum, con_strlen - sum, 0);//接收图像二进制内容//
						if (r <= 0)
						{
							break;//接收结束//
						}
						sum += r;
					}
					//将数据存储到结构体，以便存储到数据库//
					regData.imgData.assign(received, received + sum);
					regData.step++;
					if (sum == con_strlen)
					{
						std::vector<BYTE>imgData(received, received + sum);//图片数据存储到imgData容器//
						std::lock_guard<std::mutex>lock(g_imgQueueMutex);//线程安全//
						g_imgQueue.push(std::move(imgData));//图片加入队列//
						delete[]received;
					}
					//通知现有图片框更新内容//
					PostMessage(g_hInfoDialogphoto, WM_APP_UPDATEPHOTO_MSG, 0, 0);
				}
				if (regData.step == 7)
				{
					SaveToDatabase(regData); // 将结构体数据保存到数据库//
					regData = receivedData(); // 清空，准备下一个用户//
					MessageBox(NULL, L"服务器已成功将接收的注册数据存储到数据库", L"QQ", MB_ICONINFORMATION);
					break;
				}
			}
			update_new_users_list();//通知函数重新加载数据库里的用户昵称和头像//
			//跳出注册线程//
			//判断具体执行哪种线程,登录或注册//
			int recv_len = 0;
			recv(client_server, (char*)&recv_len, sizeof(recv_len), 0);//接收消息长度//
			if (recv_len <= 0)
			{
				//MessageBox(NULL, L"接收线程类型的消息长度失败", L"QQ", MB_ICONERROR);
				return;
			}
			//MessageBox(NULL, L"接收线程类型的消息长度成功", L"QQ", MB_ICONINFORMATION);
			std::string recvchar(recv_len, 0);
			int r = 0;
			r = recv(client_server, &recvchar[0], recv_len, 0);
			{
				if (r <= 0)
				{
					//MessageBox(NULL, L"接收线程类型的消息失败", L"QQ", MB_ICONERROR);
					return;
				}
			}
			//MessageBox(NULL, L"接收线程类型的消息成功", L"QQ", MB_ICONINFORMATION);
			int wlen = MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, NULL, 0);//转换为宽字节所需要的字节数//
			std::wstring wrecvchar(wlen, 0);//分配空间//
			MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, &wrecvchar[0], wlen);//实际转换//
			//注册处理//
			if (wcscmp(wrecvchar.c_str(), L"注册") == 0)//注册线程//
			{
				//MessageBox(NULL, L"接收到注册消息，即将启用注册线程", L"QQ", MB_ICONINFORMATION);
				std::thread(HandleClient_register, client_server).detach();//创建注册线程//
			}
			//登录处理//
			else if (wcscmp(wrecvchar.c_str(), L"登录") == 0)//登录线程//
			{
				//MessageBox(NULL, L"接收到登录消息，即将启用登录线程", L"QQ", MB_ICONINFORMATION);
				std::thread(HandleClient_login, client_server,new_online_user_account).detach();//创建登录线程//
			}
			else
			{
				//MessageBox(NULL, L"接收不到线程消息，即将退出", L"QQ", MB_ICONINFORMATION);
				return;
			}
			//MessageBox(NULL, L"服务器已无法接收来自客户端的信息", L"QQ", MB_ICONERROR);
			//closesocket(client_server);
		}
	}
}


std::string my_recv_one(SOCKET socket)
{
	int recv_len = 0;
	recv(socket, (char*)&recv_len, sizeof(recv_len), 0);//接收长度//
	if (recv_len <= 0)
	{
		MessageBox(NULL, L"接收账号长度失败", L"QQ", MB_ICONERROR);
		return "";
	}
	std::string recvchar(recv_len, 0);//分配空间//
	recv(socket, &recvchar[0], recv_len, 0);//接收内容//
	return recvchar;
}


//数据库匹配账号和密码的函数//
bool is_account_registered(const char* account, const char* password, MYSQL* conn,receivedData &my_data)
{
	// 1. 预处理 SQL 查询
	const char* sql = "SELECT account,password,nickname,gender,age,signature,imgData FROM users WHERE account=? AND password=? LIMIT 1";//定义一个SQL查询语句的字符串，最多返回一条记录//
	MYSQL_STMT* stmt = mysql_stmt_init(conn);//初始化一个预处理语句句柄，conn是数据库连接句柄//
	if (!stmt)
	{
		MessageBox(NULL, L"数据库预处理语句句柄初始化失败", L"QQ", MB_ICONERROR);
		return false;
	}

	if (mysql_stmt_prepare(stmt, sql, strlen(sql)))//把SQL语句从字符串转换为数据库内部结构，提高执行效率// 
	{
		MessageBox(NULL, L"SQL语句转换为数据库内部结构失败", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}


	// 2. 绑定输入参数
	MYSQL_BIND bind[2];//参数个数为2//
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_STRING;
	bind[0].buffer = (char*)account;
	bind[0].buffer_length = strlen(account);

	bind[1].buffer_type = MYSQL_TYPE_STRING;
	bind[1].buffer = (char*)password;
	bind[1].buffer_length = strlen(password);

	if (mysql_stmt_bind_param(stmt, bind))//绑定参数//
	{
		MessageBox(NULL, L"绑定参数失败", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}

	// 3. 绑定结果（输出缓冲区）
	char*account_buf = new char[65];     // 假设字段最大长度为64
	char*password_buf=new char[65];
	char*nickname_buf=new char[65];
	char*gender_buf=new char[9];
	char*age_buf=new char[9];
	char*signature_buf=new char[257];  // 假设最大256
	BYTE*img_buf=new BYTE[10*1024 * 1024]; // 10MB最大头像

	unsigned long lens[7] = { 0 };//用于储存每个字段的实际长度//
	bool is_null[7] = { 0 };//用于标识每个字段是否为NULL//
	
	// 4. 绑定结果
	MYSQL_BIND result_bind[7];//参数个数为1//
	memset(result_bind, 0, sizeof(result_bind));
	result_bind[0].buffer_type = MYSQL_TYPE_STRING;
	result_bind[0].buffer = account_buf;
	result_bind[0].buffer_length =65;//最大的写入字节数//
	result_bind[0].is_null =&is_null[0];//初始化//
	result_bind[0].length =&lens[0];//初始化//

	result_bind[1].buffer_type = MYSQL_TYPE_STRING;
	result_bind[1].buffer = password_buf;
	result_bind[1].buffer_length = 65;
	result_bind[1].is_null =&is_null[1];//初始化//
	result_bind[1].length =&lens[1];//初始化//

	result_bind[2].buffer_type = MYSQL_TYPE_STRING;
	result_bind[2].buffer = nickname_buf;
	result_bind[2].buffer_length= 65;
	result_bind[2].is_null = &is_null[2];//初始化//
	result_bind[2].length = &lens[2];//初始化//

	result_bind[3].buffer_type = MYSQL_TYPE_STRING;
	result_bind[3].buffer = gender_buf;
	result_bind[3].buffer_length = 9;
	result_bind[3].is_null = &is_null[3];//初始化//
	result_bind[3].length = &lens[3];//初始化//

	result_bind[4].buffer_type = MYSQL_TYPE_STRING;
	result_bind[4].buffer = age_buf;
	result_bind[4].buffer_length = 9;
	result_bind[4].is_null =&is_null[4];//初始化//
	result_bind[4].length =&lens[4];//初始化//

	result_bind[5].buffer_type = MYSQL_TYPE_STRING;
	result_bind[5].buffer = signature_buf;
	result_bind[5].buffer_length = 257;
	result_bind[5].is_null =&is_null[5];//初始化//
	result_bind[5].length =&lens[5];//初始化//

	result_bind[6].buffer_type = MYSQL_TYPE_BLOB;
	result_bind[6].buffer = img_buf;
	result_bind[6].buffer_length = 10*1024*1024;
	result_bind[6].is_null = &is_null[6];//初始化//
	result_bind[6].length =&lens[6];//初始化//

	// 3. 执行
	if (mysql_stmt_execute(stmt))
	{
		MessageBox(NULL, L"执行SQL语句失败", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}

	if (mysql_stmt_bind_result(stmt, result_bind))//绑定结果参数//
	{
		MessageBox(NULL, L"绑定结果参数失败", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}
	// 5. 获取结果
	bool found = false;
	if (mysql_stmt_store_result(stmt) == 0) //把结果集缓冲到本地//
	{
		MessageBox(NULL, L"成功把结果缓冲到本地", L"QQ", MB_ICONINFORMATION);
		if (mysql_stmt_fetch(stmt) == 0) // 有一条记录，变量会被绑定你绑定的变量//
		{      
			MessageBox(NULL, L"成功查询到结果", L"QQ", MB_ICONINFORMATION);
			found = true;
			my_data.account=(!is_null[0])?std::string(account_buf,lens[0]) : "";
			my_data.password =(!is_null[1])?std::string(password_buf,lens[1]) : "";
			my_data.nickname = (!is_null[2]) ? std::string(nickname_buf, lens[2]):"";
			my_data.gender = (!is_null[3])?std::string(gender_buf,lens[3]) : "";
			my_data.age = (!is_null[4])?std::string(age_buf,lens[4]) :"";
			my_data.signature =(is_null[5]) ? std::string(signature_buf,lens[5]) : "";
			if(!is_null[6]&&lens[6])
			my_data.imgData.assign(img_buf,img_buf+lens[6]);
		}
	}
    delete[] account_buf ;     
	delete[] password_buf ;
	delete[] nickname_buf;
	delete[] gender_buf;
	delete[] age_buf ;
	delete[] signature_buf ; 
	delete[] img_buf; 
	mysql_stmt_free_result(stmt);
	mysql_stmt_close(stmt);
	return found;
}



// 删除指定账号和密码的用户
bool delete_user_by_account_password(const char* account, const char* password, MYSQL* conn)
{
	// 1. 预处理 SQL 删除语句
	const char* sql = "DELETE FROM users WHERE account=? AND password=? LIMIT 1";
	MYSQL_STMT* stmt = mysql_stmt_init(conn);//获取预处理语句句柄//
	if (!stmt)
	{
		MessageBox(NULL, L"数据库预处理语句句柄初始化失败", L"QQ", MB_ICONERROR);
		return false;
	}

	if (mysql_stmt_prepare(stmt, sql, strlen(sql)))//将SQL语句转换为数据库内部结构//
	{
		MessageBox(NULL, L"SQL语句转换为数据库内部结构失败", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);//释放预处理语句句柄//
		return false;
	}

	// 2. 绑定输入参数
	MYSQL_BIND bind[2];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_STRING;
	bind[0].buffer = (char*)account;
	bind[0].buffer_length = strlen(account);

	bind[1].buffer_type = MYSQL_TYPE_STRING;
	bind[1].buffer = (char*)password;
	bind[1].buffer_length = strlen(password);

	if (mysql_stmt_bind_param(stmt, bind))//绑定参数//
	{
		MessageBox(NULL, L"绑定参数失败", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}

	// 3. 执行
	if (mysql_stmt_execute(stmt))
	{
		MessageBox(NULL, L"执行SQL语句失败", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}

	// 4. 判断是否真的删除
	my_ulonglong affected_rows = mysql_stmt_affected_rows(stmt);//查看最近一次预处理语句处理后所影响的行数//
	mysql_stmt_close(stmt);

	if (affected_rows > 0)
	{
		MessageBox(NULL, L"用户删除成功", L"QQ", MB_ICONINFORMATION);
		return true;
	}
	else
	{
		MessageBox(NULL, L"未找到匹配的用户，删除失败", L"QQ", MB_ICONERROR);
		return false;
	}
}



// 依据用户账号查找用户图片数据
bool find_imgdata_according_to_account(const char* account, MYSQL* conn_3,receivedData& mydata)
{
	int con_num = 0;
	while (!mysql_real_connect(conn_3, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))//连接数据库//
	{
		//MessageBox(NULL, L"连接数据库失败", L"QQ", MB_ICONERROR);
		con_num++;
		if (con_num > 3)
		{
			return false;
		}
	}

	// 1. 预处理 SQL 语句
	const char* sql = "SELECT ImgData FROM users WHERE account=? LIMIT 1";
	MYSQL_STMT* stmt = mysql_stmt_init(conn_3);//获取预处理语句句柄//
	if (!stmt)
	{
		MessageBox(NULL, L"数据库预处理语句句柄初始化失败", L"QQ", MB_ICONERROR);
		return false;
	}
	if (mysql_stmt_prepare(stmt, sql, strlen(sql)))//将SQL语句转换为数据库内部结构//
	{
		MessageBox(NULL, L"SQL语句转换为数据库内部结构失败", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);//释放预处理语句句柄//
		return false;
	}
	// 2. 绑定输入参数
	MYSQL_BIND bind[1];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_STRING;
	bind[0].buffer = (char*)account;
	bind[0].buffer_length = strlen(account);


	if (mysql_stmt_bind_param(stmt, bind))//绑定参数//
	{
		MessageBox(NULL, L"绑定参数失败", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}

	// 3. 执行
	if (mysql_stmt_execute(stmt))
	{
		MessageBox(NULL, L"执行SQL语句失败", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}

	//4.绑定结果
	unsigned long img_length = 0;
	bool is_null = false;
	bool error = false;
	
	BYTE img_buffer[1];
	MYSQL_BIND bind_result[1];
	memset(bind_result,0,sizeof(bind_result));
	bind_result[0].buffer_type = MYSQL_TYPE_BLOB;
	bind_result[0].buffer = img_buffer;
	bind_result[0].buffer_length = sizeof(img_buffer);//允许写入的最大字节数
	bind_result[0].length = &img_length;//实际写入的字节数
	bind_result[0].is_null = &is_null;
	bind_result[0].error = &error;

	if (mysql_stmt_bind_result(stmt, bind_result))
	{
		MessageBox(NULL, L"绑定结果失败", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}

	//5.获取结果
	if (mysql_stmt_store_result(stmt))
	{
		MessageBox(NULL,L"储存结果失败",L"QQ",MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}
	int ret = mysql_stmt_fetch(stmt);
	if (ret == 0 || ret == MYSQL_DATA_TRUNCATED)//成功获取结果
	{
		//获取图片数据长度
		if (!is_null && img_length > 0)
		{
			mydata.imgData.resize(img_length);
			//重新设置bind_result填充mydata.imgData
			bind_result[0].buffer = mydata.imgData.data();
			bind_result[0].buffer_length = img_length;
			//再次获取完整数据
			if (mysql_stmt_fetch_column(stmt, &bind_result[0], 0, 0))
			{
				MessageBox(NULL,L"获取图片数据失败",L"QQ",MB_ICONERROR);
				mysql_stmt_free_result(stmt);//先释放本地结果集缓冲区
				mysql_stmt_close(stmt);
				return false;
			}
		}
		else
		{
			//没有头像
			mydata.imgData.clear();
			return false;
		}
	}
	mysql_stmt_free_result(stmt);
	mysql_stmt_close(stmt);
	return true;
}





//用户昵称和头像查询语句//
void LoadUsersFromDB(MYSQL* conn)
{
	while (true)//加载用户账号、昵称和头像信息//
	{
		std::unique_lock<std::mutex>lk(users_mutex);
		//等待信号被设置为true//
		users_cv.wait(lk, []{return users_update_signal; });
		users_update_signal = false;
		lk.unlock();

		std::vector<Userdata>temp_users;
		temp_users.clear();
		const char* sql = "SELECT nickname,imgData,account FROM users";//SQL命令语句//
		if (mysql_query(conn, sql))//如果没有符合的查询结果，返回非零值//
		{
			MessageBox(NULL, L"没有符合的昵称和头像", L"QQ", MB_ICONERROR);
			continue;
		}

		MYSQL_RES* res = mysql_store_result(conn);//一个结果集//
		if (!res)
		{
			continue;
		}
		MYSQL_ROW  row;//指向字符串数组的指针//
		unsigned long* lengths;
		while ((row = mysql_fetch_row(res)))//取某一行查询结果集//
		{
			lengths = mysql_fetch_lengths(res);//取某一行查询结果长度集//
			Userdata user;
			if (row[0])
			{
				user.nickname = row[0];
			}
			if (row[1] && lengths[1] > 0)
			{
				user.imgData.assign((BYTE*)row[1], (BYTE*)row[1] + lengths[1]);
			}
			if (row[2])
			{
				user.account = row[2];
			}
			temp_users.push_back(std::move(user));//将填充好的用户对象移动到全局用户列表里，防止不必要的拷贝//
		}
		mysql_free_result(res);
		std::lock_guard<std::mutex>lock(g_usersMutex);//加锁//
		g_users.swap(temp_users);

		if(g_hInfoDialogbroadcast&&IsWindow(g_hInfoDialogbroadcast))//等待句柄有效//
		PostMessage(g_hInfoDialogbroadcast, WM_APP_UPDATEBROADCAST_MSG, 0, 0); // 自定义消息
	}
}


//处理用户头像数据的加载线程
void load_user_img(SOCKET client_server,std::string str_account)
{
	//接收客户端加载用户图片数据的请求
	int request_len = 0;
	recv(client_server, (char*)&request_len, sizeof(request_len), 0);
	std::string request_str(request_len, '\0');
	recv(client_server, &request_str[0], request_len, 0);
	int wrequest_len = MultiByteToWideChar(CP_UTF8, 0, request_str.c_str(), request_str.size(), NULL, 0);
	std::wstring request_wstr(wrequest_len, 0);
	MultiByteToWideChar(CP_UTF8, 0, request_str.c_str(), request_str.size(), &request_wstr[0], wrequest_len);

	if (wcscmp(request_wstr.c_str(), L"请求加载登录用户头像") == 0)
	{
		int user_img = 0;//图片数据的大小
		BYTE* user_imgage;//储存图片数据的数组
		MessageBox(NULL, L"成功接收用户图片数据的请求", L"QQ", MB_ICONINFORMATION);
		//用该用户的账号在数据库里搜索该用户的图片数据
		receivedData my_data2;
		std::string acc = str_account;
		if (acc.back() == '\0')
		{
			acc.pop_back();
		}
		bool loadImg = true;
		MYSQL* conn_3 = mysql_init(NULL);
		loadImg = find_imgdata_according_to_account(acc.c_str(), conn_3, my_data2);
		mysql_close(conn_3);
		if (!loadImg)
		{
			return;
		}
		int img_len = my_data2.imgData.size();
		send(client_server, (char*)&img_len, sizeof(img_len), 0);//先发图片数据长度
		send(client_server, (char*)my_data2.imgData.data(), img_len, 0);//后发图片数据内容
		MessageBox(NULL,L"已经成功将用户的头像数据发出",L"QQ",MB_ICONINFORMATION);
	}
	else
	{
		return;
	}
}

//从数据库加载用户个人信息
bool load_personal_information(std::string new_online_user_account, my_user_information &my_user_infor_edit, MYSQL* conn_2)
{
	int con_num = 0;
	while (!mysql_real_connect(conn_2, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))//连接数据库//
	{
		//MessageBox(NULL, L"连接数据库失败", L"QQ", MB_ICONERROR);
		con_num++;
		if (con_num > 3)
		{
			return false;
		}
	}
	
	//MessageBox(NULL, L"服务器已经成功连接数据库", L"QQ", MB_ICONINFORMATION);

	// 1. 预处理 SQL 查询
	const char* sql = "SELECT password,nickname,gender,age,signature FROM users WHERE account=? LIMIT 1";//定义一个SQL查询语句的字符串，最多返回一条记录//
	MYSQL_STMT* stmt = mysql_stmt_init(conn_2);//初始化一个预处理语句句柄，conn是数据库连接句柄//
	if (!stmt)
	{
		MessageBox(NULL, L"数据库预处理语句句柄初始化失败", L"QQ", MB_ICONERROR);
		return false;
	}
	//MessageBox(NULL, L"服务器已经成功初始化一个预处理语句句柄", L"QQ", MB_ICONINFORMATION);

	if (mysql_stmt_prepare(stmt, sql, strlen(sql)))//把SQL语句从字符串转换为数据库内部结构，提高执行效率// 
	{
		MessageBox(NULL, L"SQL语句转换为数据库内部结构失败", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}
	//MessageBox(NULL, L"服务器已经成功将SQL语句转化为数据库内部结构", L"QQ", MB_ICONINFORMATION);

	// 2. 绑定输入参数
	MYSQL_BIND bind[1];//参数个数为1//
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_STRING;
	bind[0].buffer =(void*)new_online_user_account.c_str();
	bind[0].buffer_length =(unsigned long)new_online_user_account.size();

	if (mysql_stmt_bind_param(stmt, bind))//绑定参数//
	{
		MessageBox(NULL, L"绑定参数失败", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}
	//MessageBox(NULL, L"服务器已经成功绑定参数", L"QQ", MB_ICONINFORMATION);

	// 3. 绑定结果（输出缓冲区）
	char* password_buf = new char[65];
	char* nickname_buf = new char[65];
	char* gender_buf = new char[9];
	char* age_buf = new char[9];
	char* signature_buf = new char[257];  // 假设最大256

	unsigned long lens[5] = { 0 };//用于储存每个字段的实际长度//
	bool is_null[5] = { 0 };//用于标识每个字段是否为NULL//

	// 4. 绑定结果
	MYSQL_BIND result_bind[5];//参数个数为1//
	memset(result_bind, 0, sizeof(result_bind));

	result_bind[0].buffer_type = MYSQL_TYPE_STRING;
	result_bind[0].buffer = password_buf;
	result_bind[0].buffer_length = 65;
	result_bind[0].is_null = &is_null[0];//初始化//
	result_bind[0].length = &lens[0];//初始化//

	result_bind[1].buffer_type = MYSQL_TYPE_STRING;
	result_bind[1].buffer = nickname_buf;
	result_bind[1].buffer_length = 65;
	result_bind[1].is_null = &is_null[1];//初始化//
	result_bind[1].length = &lens[1];//初始化//

	result_bind[2].buffer_type = MYSQL_TYPE_STRING;
	result_bind[2].buffer = gender_buf;
	result_bind[2].buffer_length = 9;
	result_bind[2].is_null = &is_null[2];//初始化//
	result_bind[2].length = &lens[2];//初始化//

	result_bind[3].buffer_type = MYSQL_TYPE_STRING;
	result_bind[3].buffer = age_buf;
	result_bind[3].buffer_length = 9;
	result_bind[3].is_null = &is_null[3];//初始化//
	result_bind[3].length = &lens[3];//初始化//

	result_bind[4].buffer_type = MYSQL_TYPE_STRING;
	result_bind[4].buffer = signature_buf;
	result_bind[4].buffer_length = 257;
	result_bind[4].is_null = &is_null[4];//初始化//
	result_bind[4].length = &lens[4];//初始化//

	// 3. 执行
	if (mysql_stmt_execute(stmt))
	{
		MessageBox(NULL, L"执行SQL语句失败", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}
	//MessageBox(NULL, L"服务器已经执行了SQL语句", L"QQ", MB_ICONINFORMATION);

	if (mysql_stmt_bind_result(stmt, result_bind))//绑定结果参数//
	{
		MessageBox(NULL, L"绑定结果参数失败", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}
	//MessageBox(NULL, L"服务器已经成功绑定结果参数", L"QQ", MB_ICONINFORMATION);

	// 5. 获取结果
	bool found = false;
	if (mysql_stmt_store_result(stmt) == 0) //把结果集缓冲到本地//
	{
		//MessageBox(NULL, L"服务器成功把结果缓冲到本地", L"QQ", MB_ICONINFORMATION);
		if (mysql_stmt_fetch(stmt) == 0) // 有一条记录，变量会被绑定你绑定的变量//
		{
			//MessageBox(NULL, L"服务器成功查询到结果", L"QQ", MB_ICONINFORMATION);
			found = true;
			my_user_infor_edit.password = (!is_null[0]) ? std::string(password_buf, lens[0]) : "";
			my_user_infor_edit.nickname = (!is_null[1]) ? std::string(nickname_buf, lens[1]) : "";
			my_user_infor_edit.gender = (!is_null[2]) ? std::string(gender_buf, lens[2]) : "";
			my_user_infor_edit.age = (!is_null[3]) ? std::string(age_buf, lens[3]) : "";
			my_user_infor_edit.signature = (!is_null[4]) ? std::string(signature_buf, lens[4]) : "";
		}
	}
	else
	{
		MessageBox(NULL, L"服务器无法把结果缓冲到本地", L"QQ", MB_ICONERROR);
	}
	delete[] password_buf;
	delete[] nickname_buf;
	delete[] gender_buf;
	delete[] age_buf;
	delete[] signature_buf;
	mysql_stmt_free_result(stmt);
	mysql_stmt_close(stmt);
	return found;
}

void HandleClient_login(SOCKET client_server,std::string new_online_user_account)//登录处理线程//
{
	//先接收客户端是要执行确定还是取消操作//
	int utf8_len = 0;
	recv(client_server, (char*)&utf8_len, sizeof(utf8_len), 0);//接收长度//
	if (utf8_len <= 0)
	{
		return;
	}
	std::string utf8_str(utf8_len, 0);//分配空间//
	recv(client_server, &utf8_str[0], utf8_len, 0);//接受内容//
	int wchar_len = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), utf8_len, NULL, 0);//计算转换长度//
	if (wchar_len <= 0)
	{
		return;
	}
	std::wstring wide_str(wchar_len, 0);//分配空间//
	MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), utf8_len, &wide_str[0], wchar_len);//实际转换//


	if (wcscmp(wide_str.c_str(), L"确定") == 0)
	{
		//先记录该账号的信息//
		receivedData my_data;
		std::string str_account = my_recv_one(client_server);//接收账号//
		//显示接收到的账号//
		int wchar_len_account = MultiByteToWideChar(CP_UTF8, 0, str_account.c_str(), str_account.size() + 1, NULL, 0);
		std::wstring  w_account_pro(wchar_len_account, 0);
		MultiByteToWideChar(CP_UTF8, 0, str_account.c_str(), str_account.size() + 1, &w_account_pro[0], w_account_pro.size());
		MessageBox(NULL, w_account_pro.c_str(), L"QQ", MB_ICONINFORMATION);

		std::string str_password = my_recv_one(client_server);//接受密码//
		//显示接收到的密码//
		int wchar_len_password= MultiByteToWideChar(CP_UTF8, 0, str_password.c_str(), str_password.size() + 1, NULL, 0);
		std::wstring  w_password_pro(wchar_len_password, 0);
		MultiByteToWideChar(CP_UTF8, 0, str_password.c_str(), str_password.size() + 1, &w_password_pro[0], w_password_pro.size());
		MessageBox(NULL, w_password_pro.c_str(), L"QQ", MB_ICONINFORMATION);


		MessageBox(NULL, L"服务器已成功接收准备验证的账号和密码", L"QQ", MB_ICONINFORMATION);
		//在数据库查找，查看该用户的账号和密码是否已经注册//
		if (is_account_registered(str_account.c_str(), str_password.c_str(), conn,my_data))//查询到该账号密码已经注册//
		{

			//MessageBox(NULL, L"查询到该账号密码已经注册", L"QQ", MB_ICONINFORMATION);
			//记录新的登录用户账号
			new_online_user_account = str_account;//含'\0'

		
			

            //将用户记录为在线用户//
			User_account users_accounts;
			users_accounts.account = str_account;
			{
				std::lock_guard<std::mutex>lock(g_onlineUsersMutex);
				g_onlineUsers.push_back(users_accounts);
			}

			//更新用户列表的在线状态//
			{
				std::lock_guard<std::mutex> lk(users_mutex);//上锁
				users_update_signal = true;      // 设置条件
				users_cv.notify_one();           // 唤醒一个等待线程
			}

			//通知离线用户在线状态是否更新
			{
				std::lock_guard<std::mutex>lock_users_online(users_online_mutex);//上锁
				users_online_update_signal = true;//设置条件
				users_online_cv.notify_one();//唤醒一个等待线程
			}

			//保存socket和对应的用户账号//
			User_online user_online;
		    std::lock_guard<std::mutex>lock(g_users_online_Mutex);
			user_online.socket = client_server;
			user_online.account = str_account;//账号含'\0'终止符//
			g_users_online.push_back(user_online);//将用户账号和socket保存到在线用户列表里//


			std::wstring recv_back = L"sucess";
			int utf8_len = WideCharToMultiByte(CP_UTF8, 0, recv_back.c_str(), recv_back.size() + 1, NULL, 0, NULL, NULL);//转换为utf8所需的空间//
			std::string utf8str(utf8_len, 0);//分配空间//
			WideCharToMultiByte(CP_UTF8, 0, recv_back.c_str(), recv_back.size() + 1, &utf8str[0], utf8_len, NULL, NULL);//实际转换//
			send(client_server, (char*)&utf8_len, sizeof(utf8_len), 0);//先发长度//
			send(client_server, utf8str.c_str(), utf8_len, 0);//后发内容//
			
			

			//进入登录主页面之后的线程//
			//MessageBox(NULL, L"即将进入登录界面的处理线程", L"QQ", MB_ICONINFORMATION);
			std::thread(Handlelogin_pro,client_server,my_data,new_online_user_account).detach();
		}
		else
		{
			MessageBox(NULL, L"查询到该账号密码还没有注册,请检查账号和密码输入是否正确", L"QQ", MB_ICONINFORMATION);
			std::wstring recv_back = L"faile";
			int utf8_len = WideCharToMultiByte(CP_UTF8, 0, recv_back.c_str(), recv_back.size() + 1, NULL, 0, NULL, NULL);//转换为utf8所需的空间//
			std::string utf8str(utf8_len, 0);//分配空间//
			WideCharToMultiByte(CP_UTF8, 0, recv_back.c_str(), recv_back.size() + 1, &utf8str[0], utf8_len, NULL, NULL);//实际转换//
			send(client_server, (char*)&utf8_len, sizeof(utf8_len), 0);//先发长度//
			send(client_server, utf8str.c_str(), utf8_len, 0);//后发内容//
			std::thread(HandleClient_login, client_server,new_online_user_account).detach();//登录处理线程//
		}
	}
	else if (wcscmp(wide_str.c_str(), L"取消") == 0)
	{
		//判断具体执行哪种线程,登录或注册//
		int recv_len = 0;
		recv(client_server, (char*)&recv_len, sizeof(recv_len), 0);//接收消息长度//
		if (recv_len <= 0)
		{
			MessageBox(NULL, L"接收线程类型的消息长度失败", L"QQ", MB_ICONERROR);
			return;
		}
		MessageBox(NULL, L"接收线程类型的消息长度成功", L"QQ", MB_ICONINFORMATION);
		std::string recvchar(recv_len, 0);
		int r = 0;
		r = recv(client_server, &recvchar[0], recv_len, 0);
		{
			if (r <= 0)
			{
				MessageBox(NULL, L"接收线程类型的消息失败", L"QQ", MB_ICONERROR);
				return;
			}
		}
		MessageBox(NULL, L"接收线程类型的消息成功", L"QQ", MB_ICONINFORMATION);
		int wlen = MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, NULL, 0);//转换为宽字节所需要的字节数//
		std::wstring wrecvchar(wlen, 0);//分配空间//
		MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, &wrecvchar[0], wlen);//实际转换//
		//注册处理//
		if (wcscmp(wrecvchar.c_str(), L"注册") == 0)//注册线程//
		{
			MessageBox(NULL, L"接收到注册消息，即将启用注册线程", L"QQ", MB_ICONINFORMATION);
			std::thread(HandleClient_register, client_server).detach();//创建注册线程//
		}
		//登录处理//
		else if (wcscmp(wrecvchar.c_str(), L"登录") == 0)//登录线程//
		{
			MessageBox(NULL, L"接收到登录消息，即将启用登录线程", L"QQ", MB_ICONINFORMATION);
			std::thread(HandleClient_login, client_server,new_online_user_account).detach();//创建登录线程//
		}
		else
		{
			MessageBox(NULL, L"接收不到线程消息，即将退出", L"QQ", MB_ICONINFORMATION);
			return;
		}
	}
}

std::wstring strTowstr(std::string str)
{
	int wlen=MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.size() + 1, NULL, 0);//计算转换字节数//
	if (wlen <= 0)
	{
		MessageBox(NULL,L"数据库中该字段列表为空",L"QQ",MB_ICONERROR);
		return L"";
	}
	std::wstring w_str(wlen, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.size() + 1,&w_str[0], wlen);//实际转换//
	return w_str;
}


// 传入参数：账号、图片数据和长度
bool update_user_img(const std::string& account, const BYTE* imgData, int imgLen, MYSQL* conn)
{
	const char* sql = "UPDATE users SET imgData=? WHERE account=? LIMIT 1";
	MYSQL_STMT* stmt = mysql_stmt_init(conn);
	if (!stmt) return false;
	if (mysql_stmt_prepare(stmt, sql, strlen(sql)))
	{
		mysql_stmt_close(stmt);
		return false;
	}

	MYSQL_BIND bind[2];
	memset(bind, 0, sizeof(bind));
	// 头像BLOB
	bind[0].buffer_type = MYSQL_TYPE_LONG_BLOB;
	bind[0].buffer = (void*)imgData;
	bind[0].buffer_length = imgLen;
	// 账号
	bind[1].buffer_type = MYSQL_TYPE_STRING;
	bind[1].buffer = (void*)account.c_str();
	bind[1].buffer_length = account.length();

	if (mysql_stmt_bind_param(stmt, bind))
	{
		mysql_stmt_close(stmt); 
		return false;
	}
	bool ok = (mysql_stmt_execute(stmt) == 0);
	mysql_stmt_close(stmt);
	return ok;
}

void send_personal_information_to_client(SOCKET client_server,std::string new_online_user_account)
{
	//从数据库加载用户信息
	my_user_information my_user_infor_edit;
	std::string account_new = new_online_user_account;
	if (!account_new.empty() && account_new.back() == '\0')
	{
		account_new.pop_back();
	}
	//将加载的个人信息发送给客户端
	MYSQL* conn_2 = mysql_init(NULL);
	if (load_personal_information(account_new, my_user_infor_edit, conn_2))
	{
		mysql_close(conn_2);
		//结构体已经成功加载用户的个人信息
		MessageBox(NULL, L"服务器已经成功加载所有个人信息", L"QQ", MB_ICONINFORMATION);
		//发送密码
		int password_buf_len = my_user_infor_edit.password.size();
		send(client_server, (char*)&password_buf_len, sizeof(password_buf_len), 0);
		send(client_server, my_user_infor_edit.password.c_str(), password_buf_len, 0);//不含'\0'

		//发送昵称
		int nickname_buf_len = my_user_infor_edit.nickname.size();
		send(client_server, (char*)&nickname_buf_len, sizeof(nickname_buf_len), 0);
		send(client_server, my_user_infor_edit.nickname.c_str(), nickname_buf_len, 0);//不含'\0'

		//发送性别
		int gender_buf_len = my_user_infor_edit.gender.size();
		send(client_server, (char*)&gender_buf_len, sizeof(gender_buf_len), 0);
		send(client_server, my_user_infor_edit.gender.c_str(), gender_buf_len, 0);//不含'\0'

		//发送年龄
		int age_buf_len = my_user_infor_edit.age.size();
		send(client_server, (char*)&age_buf_len, sizeof(age_buf_len), 0);
		send(client_server, my_user_infor_edit.age.c_str(), age_buf_len, 0);//不含'\0'

		//发送个性签名
		int signature_buf_len = my_user_infor_edit.signature.size();
		send(client_server, (char*)&signature_buf_len, sizeof(signature_buf_len), 0);
		send(client_server, my_user_infor_edit.signature.c_str(), signature_buf_len, 0);//不含'\0'

		MessageBox(NULL, L"已成功将用户个人信息发给客户端", L"QQ", MB_ICONINFORMATION);
	}
	else
	{
		mysql_close(conn_2);
		MessageBox(NULL, L"无法将用户个人信息发给客户端", L"QQ", MB_ICONERROR);
		return;
	}

}


int recv_all_information(SOCKET socket,std::string &user_infor)
{
	int buf_len = 0;
	recv(socket,(char*)&buf_len,sizeof(buf_len),0);//先接收长度
	int r = 0;
	int sum = 0;
	std::string str(buf_len, '\0');
	while (sum < buf_len)
	{
		r = recv(socket,&str[0]+sum,buf_len-sum,0);
		if (r > 0)
		{
			sum += r;
		}
		if (sum == buf_len)
		{
			user_infor = str;//填充信息
			return sum;
		}

	}
}


// 更新用户信息
bool update_user_information(const std::string& account,MYSQL* conn_4, my_user_information& new_information)
{
	//连接数据库
	int con_num = 0;
	while (!mysql_real_connect(conn_4, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))//连接数据库//
	{
		MessageBox(NULL, L"连接数据库失败", L"QQ", MB_ICONERROR);
		con_num++;
		if (con_num > 3)
		{
			return false;
		}
	}

	const char* sql = "UPDATE users SET password=?,nickname=?,gender=?,age=?,signature=? WHERE account=? LIMIT 1";
	MYSQL_STMT* stmt = mysql_stmt_init(conn_4);
	if (!stmt) return false;
	if (mysql_stmt_prepare(stmt, sql, strlen(sql)))
	{
		mysql_stmt_close(stmt);
		return false;
	}

	MYSQL_BIND bind[6]; // 6个参数
	memset(bind, 0, sizeof(bind));

	// 1. password
	bind[0].buffer_type = MYSQL_TYPE_STRING;
	bind[0].buffer = (void*)new_information.password.c_str();
	bind[0].buffer_length = new_information.password.length();

	// 2. nickname
	bind[1].buffer_type = MYSQL_TYPE_STRING;
	bind[1].buffer = (void*)new_information.nickname.c_str();
	bind[1].buffer_length = new_information.nickname.length();

	// 3. gender
	bind[2].buffer_type = MYSQL_TYPE_STRING;
	bind[2].buffer = (void*)new_information.gender.c_str();
	bind[2].buffer_length = new_information.gender.length();

	// 4. age
	bind[3].buffer_type = MYSQL_TYPE_STRING;
	bind[3].buffer = (void*)new_information.age.c_str();
	bind[3].buffer_length = new_information.age.length();

	// 5. signature
	bind[4].buffer_type = MYSQL_TYPE_STRING;
	bind[4].buffer = (void*)new_information.signature.c_str();
	bind[4].buffer_length = new_information.signature.length();

	// 6. account (WHERE条件)
	bind[5].buffer_type = MYSQL_TYPE_STRING;
	bind[5].buffer = (void*)account.c_str();
	bind[5].buffer_length = account.length();


	if (mysql_stmt_bind_param(stmt, bind))
	{
		mysql_stmt_close(stmt);
		return false;
	}
	bool ok = (mysql_stmt_execute(stmt) == 0);
	mysql_stmt_close(stmt);
	return ok;
}


//登录界面处理线程//
void Handlelogin_pro(SOCKET client_server,receivedData my_data,std::string new_online_user_account)//登录界面处理线程//
{
	MessageBox(NULL, L"成功进入登录界面的处理线程", L"QQ", MB_ICONINFORMATION);
	 
	//加载用户头像数据线程
	std::string str_account = new_online_user_account;
	//std::thread(load_user_img, client_server, str_account).detach();
	load_user_img(client_server, str_account);

	int len = 0;
	recvAll(client_server, (char*)&len, sizeof(len), 0);//先接收长度//

	MessageBox(NULL, L"接收登录界面客户端发送的长度消息成功", L"QQ", MB_ICONINFORMATION);
	if (len <= 0)
	{
		MessageBox(NULL, L"接收登录界面按钮信息长度失败", L"QQ", MB_ICONERROR);
		return;
	}
	std::string utf8str(len, 0);//分配空间//
	recvAll(client_server, &utf8str[0], len, 0);//接收内容//
	MessageBox(NULL, L"接收登录界面客户端发送的按钮内容消息成功", L"QQ", MB_ICONINFORMATION);
	int wlen = MultiByteToWideChar(CP_UTF8,0,utf8str.c_str(),len,NULL,0);//计算出转换长度//
	std::wstring wstr(wlen, 0);//分配空间//
	MultiByteToWideChar(CP_UTF8, 0, utf8str.c_str(), len,&wstr[0],wlen);//实际转换//
	//进行接受内容判断//
	if (wcscmp(wstr.c_str(),L"服务器")==0)
	{

		//接收客户端服务器交互时的按钮信息//
		while (true)
		{
		
			int len_utf8 = 0;
			recvAll(client_server, (char*)&len_utf8, sizeof(len_utf8), 0);//先接收长度//
			//MessageBox(NULL, L"接收登录界面客户端发送的长度消息成功", L"QQ", MB_ICONINFORMATION);
			if (len_utf8 <= 0)
			{
				MessageBox(NULL, L"接收登录界面按钮信息长度失败", L"QQ", MB_ICONERROR);
				return;
			}

			std::string str_utf8(len_utf8, 0);//分配空间//
			recvAll(client_server, &str_utf8[0], len_utf8, 0);//接收内容//
			//MessageBox(NULL, L"接收登录界面客户端发送的按钮内容消息成功", L"QQ", MB_ICONINFORMATION);
			int wchar_len = MultiByteToWideChar(CP_UTF8, 0, str_utf8.c_str(), len_utf8, NULL, 0);//计算出转换长度//
			std::wstring wchar_str(wchar_len, 0);//分配空间//
			MultiByteToWideChar(CP_UTF8, 0, str_utf8.c_str(), len_utf8, &wchar_str[0], wchar_len);//实际转换//
			if (wcscmp(wchar_str.c_str(), L"退出") == 0)
			{
				MessageBox(NULL, L"即将由与服务器的对话线程返回登陆界面处理线程", L"QQ", MB_ICONINFORMATION);
				break;
			}
			else if (wcscmp(wchar_str.c_str(), L"发送") == 0)
			{
				//MessageBox(NULL, L"成功进入登录界面的服务器处理线程", L"QQ", MB_ICONINFORMATION);
				//接收客户端发来的消息//
				int str_len = 0;
				recvAll(client_server, (char*)&str_len, sizeof(str_len), 0);//先接收长度//

				//将长度转化为宽字符串//
				//std::wstring n = std::to_wstring(str_len);
				wchar_t nbuf[32];
				swprintf_s(nbuf, _countof(nbuf), L"%d", str_len);
				std::wstring n = nbuf;
				//MessageBox(NULL, n.c_str(), L"QQ", MB_ICONINFORMATION);

				if (str_len <= 0)
				{
					MessageBox(NULL, L"接收登录界面客户端发送的信息长度失败", L"QQ", MB_ICONERROR);
					return;
				}
				std::string str_char(str_len, 0);//分配空间//

				recvAll(client_server, &str_char[0], str_len, 0);//接收内容//
				int w_len = MultiByteToWideChar(CP_UTF8, 0, str_char.c_str(), str_len, NULL, 0);//计算出转换长度//
				std::wstring w_str(w_len, 0);//分配空间//
				MultiByteToWideChar(CP_UTF8, 0, str_char.c_str(), str_len, &w_str[0], w_len);//实际转换//
				//MessageBox(NULL, L"接收登录界面客户端发送的对话框消息成功", L"QQ", MB_ICONINFORMATION);

				//MessageBox(NULL, w_str.c_str(), L"QQ", MB_ICONINFORMATION);
				//获取当前时间//
				time_t now_one = time(0);//获取当前时间，精确到秒//
				tm tm_one;//声明一个结构体，用于存储本地时间的各个组成部分//
				localtime_s(&tm_one, &now_one);//将now_one（输入）里的内容复制到tm_one里输出//
				wchar_t timeBuffer_one[64];
				wcsftime(timeBuffer_one, sizeof(timeBuffer_one) / sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", &tm_one);
				//MessageBox(NULL, L"已经准备好了时间字段", L"QQ", MB_ICONINFORMATION);

				std::wstring msg_one = L"[";
				msg_one += timeBuffer_one;
				msg_one += L"][";
				std::wstring w = strTowstr(my_data.nickname);
				if (!w.empty() && w.back() == L'\0')//去除标识字符串的终止符'\0',才可以拼接并显示后面的内容//
				{
					w.pop_back();
				}
				//MessageBox(NULL, L"已经成功将昵称末尾的‘0’弹出", L"QQ", MB_ICONINFORMATION);
				msg_one += w.c_str();
				//MessageBox(NULL, L"已经成功拼接昵称", L"QQ", MB_ICONINFORMATION);
				msg_one += L"->";
				//MessageBox(NULL, L"已经成功拼接好->", L"QQ", MB_ICONINFORMATION);
				msg_one += L"服务器";
				msg_one += L"]";
				//MessageBox(NULL, L"已经成功拼接好服务器字段", L"QQ", MB_ICONINFORMATION);
				msg_one += w_str.c_str();
				//MessageBox(NULL, w_str.c_str(), L"QQ", MB_ICONINFORMATION);
				//MessageBox(NULL, msg_one.c_str(), L"QQ", MB_ICONINFORMATION);
				//MessageBox(NULL, L"已经成功拼接好客户端发来的消息字段", L"QQ", MB_ICONINFORMATION);
				//MessageBox(NULL, L"即将准备将消息加入队列", L"QQ", MB_ICONINFORMATION);

				//将接收到的内容处理后再发回去//
				int u_len = WideCharToMultiByte(CP_UTF8, 0, msg_one.c_str(), msg_one.size() + 1, NULL, 0, NULL, NULL);//计算转换长度//
				//std::wstring num = std::to_wstring(u_len);//将数字转化为宽字符串//
				//MessageBox(NULL,num.c_str(), L"QQ", MB_ICONINFORMATION);
				std::string u_str(u_len, 0);//分配空间//
				WideCharToMultiByte(CP_UTF8, 0, msg_one.c_str(), msg_one.size() + 1, &u_str[0], u_len, NULL, NULL);//实际转换//

				send(client_server, (char*)&u_len, sizeof(u_len), 0);//先发长度//
				send(client_server, u_str.c_str(), u_len, 0);//后发内容//

				EnterCriticalSection(&g_csMsg_two_Queue);//加锁//
				g_msg_two_Queue.push(msg_one);//创建副本//
				LeaveCriticalSection(&g_csMsg_two_Queue);//解锁//
				//MessageBox(NULL, L"已经成功将信息拼接", L"QQ", MB_ICONINFORMATION);
				// 记录到服务器监控室//
				PostMessage(g_hInfoDialogmonitor, WM_APP_UPDATEMONITOR_MSG, 0, 0);
				//MessageBox(NULL, L"已经成功通知信息的监控显示", L"QQ", MB_ICONINFORMATION);

			}
			else if (wcscmp(wchar_str.c_str(), L"查看广播") == 0)
			{
				//接收用户账号
				int account_user_len = 0;
				recv(client_server,(char*)&account_user_len,sizeof(account_user_len),0);
				std::string account_user_str(account_user_len,'\0');
				recv(client_server,&account_user_str[0],account_user_len,0);//账号不含'\0'
				char buf2[256];
				sprintf_s(buf2, "account_user_str: [%s] len=%d\n", account_user_str.c_str(), (int)account_user_str.size());
				OutputDebugStringA(buf2);
				std::string s_nickname;
				//匹配用户昵称
				for (auto it=g_users.begin();it!=g_users.end();)
				{
					if (strcmp(it->account.c_str(), account_user_str.c_str()) == 0)
					{
						s_nickname = it->nickname.c_str();//不含‘\0'
						break;
					}
					it++;
				}

				char buf3[256];
				sprintf_s(buf3, "s_nickname: [%s] len=%d\n", s_nickname.c_str(), (int)s_nickname.size());
				OutputDebugStringA(buf3);
				
				int s_nickname_len =s_nickname.size();
				send(client_server,(char*)&s_nickname_len,sizeof(s_nickname_len),0);
				send(client_server,s_nickname.c_str(),s_nickname_len,0);

				//匹配g_offline_users_and_information里的对应用户账号存储的消息
				
				int infor_sum = 0;
			
				//统计一共多少条广播
				for (auto it = g_offline_users_and_information.begin(); it != g_offline_users_and_information.end();)//从最早插入的消息遍历到最后插入的消息，消息发送顺序正确
				{
					std::string acc1 = it->account;
					std::string acc2 = account_user_str;
					while (!acc1.empty() && acc1.back() == '\0') acc1.pop_back();
					while (!acc2.empty() && acc2.back() == '\0') acc2.pop_back();

					// 输出内容和长度
					char buf[256];
					sprintf_s(buf, "acc1: [%s] len=%d, acc2: [%s] len=%d\n", acc1.c_str(), (int)acc1.size(), acc2.c_str(), (int)acc2.size());
					OutputDebugStringA(buf);

					if (strcmp(acc1.c_str(),acc2.c_str())==0)//匹配g_offline_users_and_information里的对应用户账号存储的消息 
					{
						infor_sum++;
					}
					it++;
				
				}

				if (infor_sum == 0)
				{
					MessageBox(NULL, L"该用户没有未接收的广播消息", L"QQ", MB_ICONERROR);
				}
				//先发送广播条数
				send(client_server,(char*)&infor_sum,sizeof(infor_sum),0);
				
				if (g_offline_users_and_information.empty())
				{
					MessageBox(NULL, L"服务器没有未发出的广播消息", L"QQ", MB_ICONERROR);
					continue;
				}

				if (infor_sum > 0)//需要发送广播消息
				{
					for (auto it = g_offline_users_and_information.begin(); it != g_offline_users_and_information.end();)//从最早插入的消息遍历到最后插入的消息，消息发送顺序正确
					{
						if (it->account == account_user_str)//匹配g_offline_users_and_information里的对应用户账号存储的消息 
						{

									// 发送消息
									int l = it->information.size();
									send(client_server,(char*)&l,sizeof(l),0);
									send(client_server, it->information.c_str(), it->information.size(), 0);//发送先前该用户存储的广播消息，含'\0'
									
									EnterCriticalSection(&g_csMsg_two_Queue);//加锁//
									int w_str_len = 0;
									w_str_len = MultiByteToWideChar(CP_UTF8,0,it->information.c_str(),it->information.size(),NULL,0);
									std::wstring w_str_s(w_str_len,L'\0');
									MultiByteToWideChar(CP_UTF8, 0, it->information.c_str(), it->information.size(),&w_str_s[0], w_str_len); 

									//获取当前时间//
									time_t now_one = time(0);//获取当前时间，精确到秒//
									tm tm_one;//声明一个结构体，用于存储本地时间的各个组成部分//
									localtime_s(&tm_one, &now_one);//将now_one（输入）里的内容复制到tm_one里输出//
									wchar_t timeBuffer_one[64];
									wcsftime(timeBuffer_one, sizeof(timeBuffer_one) / sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", &tm_one);
									//MessageBox(NULL, L"已经准备好了时间字段", L"QQ", MB_ICONINFORMATION);


									std::wstring msg_one = L"[";
									msg_one += timeBuffer_one;
									msg_one += L"][";
									msg_one += L"服务器";
									msg_one += L"->";
									
									//用户昵称
									int  w_s_nickname_len = 0;
									w_s_nickname_len = MultiByteToWideChar(CP_UTF8,0,s_nickname.c_str(),s_nickname.size(),NULL,0);
									std::wstring w_s_nickname_str(w_s_nickname_len,L'\0');
									MultiByteToWideChar(CP_UTF8, 0, s_nickname.c_str(), s_nickname.size(),&w_s_nickname_str[0],w_s_nickname_len);

									msg_one +=w_s_nickname_str;
									msg_one += L"]";
								
									int  w_s_len = 0;
									w_s_len = MultiByteToWideChar(CP_UTF8, 0,it->information.c_str(),it->information.size(), NULL, 0);
									std::wstring w_s_str(w_s_len, L'\0');
									MultiByteToWideChar(CP_UTF8, 0, it->information.c_str(),it->information.size(), &w_s_str[0], w_s_len);
                                    msg_one += w_s_str.c_str();
									g_msg_two_Queue.push(msg_one);//创建副本//
									LeaveCriticalSection(&g_csMsg_two_Queue);//解锁//
									PostMessage(g_hInfoDialogmonitor, WM_APP_UPDATEMONITOR_MSG, 0, 0);//通知监控框显示该条消息

								
							// 删除该条记录，并更新迭代器
							std::lock_guard<std::mutex> lock_broadcast_account(g_users_account_and_information_sel_broadcast_Mutex);//来确保线程安全
							it = g_offline_users_and_information.erase(it);//返回删除元素的下一条
						}
						else
						{
							++it;
						}
					}
				}
	
			}
	      


		}
		//接收退出消息后，退出while 循环
		std::thread(Handlelogin_pro, client_server, my_data,new_online_user_account).detach();//返回登录首界面处理线程//
	}
	else if (wcscmp(wstr.c_str(),L"聊天室")==0)
	{
	door_1:
		//接收客户端加载聊天室原有的所有消息的请求
		int r_len = 0;
		recv(client_server, (char*)&r_len, sizeof(r_len), 0);
		std::string r_str(r_len,'\0');
		recv(client_server,&r_str[0],r_len,0);

		//OutputDebugStringA(("r_str is"+r_str+"\n").c_str());
		//比较接收的字符串是否正确
		if (strcmp(r_str.c_str(),"请求加载聊天室的消息") != 0)
		{
			//接收失败
			MessageBox(NULL, L"服务器加载聊天室消息的请求失败", L"QQ", MB_ICONERROR);
			return;
		}
		MessageBox(NULL, L"服务器加载聊天室消息的请求成功", L"QQ", MB_ICONINFORMATION);
		//成功接收加载请求
		//std::lock_guard<std::mutex>lock(users_anii_on_chartroom_mutex);//加锁
		//先发消息总条数
		int num = 0;
		for (auto it = users_account_on_chartroom.begin(); it != users_account_on_chartroom.end();)
		{
			num++;
			it++;
		}
		send(client_server, (char*)&num, sizeof(num), 0);
		MessageBox(NULL, L"服务器成功向客户端发送消息总条数", L"QQ", MB_ICONINFORMATION);

		if(num>0)
		{
			for (auto it = users_account_on_chartroom.begin(); it != users_account_on_chartroom.end();)
			{
				//发送用户头像
				const BYTE* data = it->user_on_chartroom_image.data();
				int img_len = it->user_on_chartroom_image.size();

				int r = 0;
				int sum = 0;

				//发送相片长度
				send(client_server, (char*)&img_len, sizeof(img_len), 0);
				//发送相片内容
				while (sum < img_len)
				{
					r = send(client_server, (const char*)data + sum, img_len - sum, 0);
					if (r > 0)
					{
						sum += r;
					}
				}

				//发送用户昵称
				int nickname_len = 0;
				nickname_len = it->user_on_chartroom_nickname.size();
				send(client_server, (char*)&nickname_len, sizeof(nickname_len), 0);
				send(client_server, it->user_on_chartroom_nickname.c_str(), nickname_len, 0);

				//发送用户消息
				int inf_len = 0;
				inf_len = it->user_on_chartroom_inf.size();
				send(client_server, (char*)&inf_len, sizeof(inf_len), 0);
				send(client_server, it->user_on_chartroom_inf.c_str(), inf_len, 0);

				OutputDebugStringA(("the information user send in chartroom is" + it->user_on_chartroom_inf + "\n").c_str());
				it++;
			}
			MessageBox(NULL, L"服务器成功向客户端发送聊天室的聊天记录", L"QQ", MB_ICONINFORMATION);
		}

		int si = 1;
		//判断接收到的是什么消息
		while (si)
		{
			//接收客户端发送或退出按钮
			int inf_len = 0;
			recv(client_server, (char*)&inf_len, sizeof(inf_len), 0);
			std::string inf_str(inf_len, '\0');
			recv(client_server, &inf_str[0],inf_len, 0);

			if (strcmp(inf_str.c_str(), "发送") == 0)
			{
				std::wstring w_nickname;
				std::wstring w_information;

				MessageBox(NULL, L"已经成功接受来自客户端的聊天室的信息’发送’按钮", L"QQ", MB_ICONINFORMATION);
				//接收用户发送文本内容
				int text_len = 0;
				recv(client_server,(char*)&text_len,sizeof(text_len), 0);
				std::string text_str(text_len,'\0');

				int t_r = 0;
				int t_sum = 0;
				while (t_sum <text_len)
				{
					t_r = recv(client_server,&text_str[0]+t_sum,text_len-t_sum,0);
					if (t_r > 0)
					{
						t_sum += t_r;
					}
				}
				
				MessageBox(NULL, L"已经成功接收客户端的聊天消息", L"QQ", MB_ICONINFORMATION);
			   //加载已记录的所有聊天室信息到用户聊天室框，包含用户头像、用户昵称和用户发出的消息

		      //需要依据用户账号查询用户头像和昵称
				users_anii_on_chartroom user_i;
				user_i = users_anii_on_chartroom();
				//将进入聊天室的用户账号存储
				user_i.user_on_chartroom_account = new_online_user_account;
				if (!user_i.user_on_chartroom_account.empty() && user_i.user_on_chartroom_account.back() == '\0')
				{
					user_i.user_on_chartroom_account.pop_back();
				}
				//将用户在编辑室发的消息存储
				user_i.user_on_chartroom_inf = text_str;//含'\0'
				
				//转换为宽字符串
			    int w_lx= 0;
				w_lx=MultiByteToWideChar(CP_UTF8, 0, user_i.user_on_chartroom_inf.c_str(), user_i.user_on_chartroom_inf.size(), NULL, 0);
				w_information.resize(w_lx);
				MultiByteToWideChar(CP_UTF8, 0, user_i.user_on_chartroom_inf.c_str(), user_i.user_on_chartroom_inf.size(), &w_information[0], w_lx);

				//加载用户的昵称和头像并储存
				{
					//std::vector  <users_anii_on_chartroom> temp;//暂时存储用户消息

					MYSQL* conn_6 = mysql_init(NULL);
					int con_num = 0;
					while (!mysql_real_connect(conn_6, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))//连接数据库//
					{
						MessageBox(NULL, L"连接数据库失败", L"QQ", MB_ICONERROR);
						con_num++;
						if (con_num > 3)
						{
							mysql_close(conn_6);
							return;
						}
					}

					//SQL命令语句//
					std::string sql = "SELECT nickname,imgData FROM users WHERE account='" + user_i.user_on_chartroom_account + "' LIMIT 1";
					if (mysql_query(conn_6, sql.c_str()))//如果没有符合的查询结果，返回非零值//
					{
						MessageBox(NULL, L"没有符合的昵称和头像", L"QQ", MB_ICONERROR);
						return;
					}

					MYSQL_RES* res = mysql_store_result(conn_6);//一个结果集//
					if (!res)
					{
						return;
					}
					MYSQL_ROW  row;//指向字符串数组的指针//
					unsigned long* lengths;
					row = mysql_fetch_row(res);//取某一行查询结果集//
					{
						lengths = mysql_fetch_lengths(res);//取某一行查询结果长度集//

						if (row[0])
						{
							user_i.user_on_chartroom_nickname = row[0];
							//将其转换为宽字符
							int w_l = 0;
							w_l = MultiByteToWideChar(CP_UTF8,0,user_i.user_on_chartroom_nickname.c_str(),user_i.user_on_chartroom_nickname.size(),NULL,0);
							w_nickname.resize(w_l);
							MultiByteToWideChar(CP_UTF8, 0, user_i.user_on_chartroom_nickname.c_str(), user_i.user_on_chartroom_nickname.size(), &w_nickname[0], w_l);
						}
						if (row[1] && lengths[1] > 0)
						{
							user_i.user_on_chartroom_image.assign((BYTE*)row[1], (BYTE*)row[1] + lengths[1]);
						}

						//temp.push_back(std::move(user_i));//将填充好的用户对象移动到全局用户列表里，防止不必要的拷贝
						//std::lock_guard<std::mutex>lock(users_anii_on_chartroom_mutex);//加锁
						users_account_on_chartroom.push_back(std::move(user_i));//将填充好的用户对象移动到全局用户列表里，防止不必要的拷贝
					}
					mysql_free_result(res);
					//std::lock_guard<std::mutex>lock(users_anii_on_chartroom_mutex);//加锁
					//users_account_on_chartroom.swap(temp);
					mysql_close(conn_6);

					MessageBox(NULL, L"服务器已经成功保存该用户的头像、昵称和所发的消息", L"QQ", MB_ICONINFORMATION);
					
					//通知客户端更新聊天室更新聊天消息
					std::string no_utf8_inf = "请更新聊天室消息";
					int no_utf8_string_len = no_utf8_inf.size();
					send(client_server, (char*)&no_utf8_string_len, sizeof(no_utf8_string_len), 0);
					send(client_server, no_utf8_inf.c_str(), no_utf8_inf.size(), 0);


					//将此条消息显示在服务器监控框
					//MessageBox(NULL, w_str.c_str(), L"QQ", MB_ICONINFORMATION);
				    //获取当前时间//
					time_t now_one = time(0);//获取当前时间，精确到秒//
					tm tm_one;//声明一个结构体，用于存储本地时间的各个组成部分//
					localtime_s(&tm_one, &now_one);//将now_one（输入）里的内容复制到tm_one里输出//
					wchar_t timeBuffer_one[64];
					wcsftime(timeBuffer_one, sizeof(timeBuffer_one) / sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", &tm_one);
					//MessageBox(NULL, L"已经准备好了时间字段", L"QQ", MB_ICONINFORMATION);

					std::wstring msg_one = L"[";
					msg_one += timeBuffer_one;
					msg_one += L"][";
					
					//MessageBox(NULL, L"已经成功将昵称末尾的‘0’弹出", L"QQ", MB_ICONINFORMATION);
					msg_one += w_nickname;
					//MessageBox(NULL, L"已经成功拼接昵称", L"QQ", MB_ICONINFORMATION);
					msg_one += L"->";
					//MessageBox(NULL, L"已经成功拼接好->", L"QQ", MB_ICONINFORMATION);
					msg_one += L"聊天室";
					msg_one += L"]";
					//MessageBox(NULL, L"已经成功拼接好服务器字段", L"QQ", MB_ICONINFORMATION);
					msg_one += w_information;
					
					EnterCriticalSection(&g_csMsg_two_Queue);//加锁//
					g_msg_two_Queue.push(msg_one);//创建副本//
					LeaveCriticalSection(&g_csMsg_two_Queue);//解锁//
					//MessageBox(NULL, L"已经成功将信息拼接", L"QQ", MB_ICONINFORMATION);
					// 记录到服务器监控室//
					PostMessage(g_hInfoDialogmonitor, WM_APP_UPDATEMONITOR_MSG, 0, 0);
					//MessageBox(NULL, L"已经成功通知信息的监控显示", L"QQ", MB_ICONINFORMATION);

					MessageBox(NULL, L"服务器已经成功向客户端发送更新聊天室消息的通知", L"QQ", MB_ICONINFORMATION);
					goto door_1;
				}

			}
			else if (strcmp(inf_str.c_str(), "更新消息") == 0)
			{
				//接收客户端加载聊天室原有的所有消息的请求
				int r_len = 0;
				recv(client_server, (char*)&r_len, sizeof(r_len), 0);
				std::string r_str(r_len, '\0');
				recv(client_server, &r_str[0], r_len, 0);

				//OutputDebugStringA(("r_str is"+r_str+"\n").c_str());
				//比较接收的字符串是否正确
				if (strcmp(r_str.c_str(), "请求加载聊天室的消息") != 0)
				{
					//接收失败
					MessageBox(NULL, L"服务器加载聊天室消息的请求失败", L"QQ", MB_ICONERROR);
					return;
				}
				MessageBox(NULL, L"服务器加载聊天室消息的请求成功", L"QQ", MB_ICONINFORMATION);
				//成功接收加载请求
				//std::lock_guard<std::mutex>lock(users_anii_on_chartroom_mutex);//加锁
				//先发消息总条数
				int num = 0;
				for (auto it = users_account_on_chartroom.begin(); it != users_account_on_chartroom.end();)
				{
					num++;
					it++;
				}
				send(client_server, (char*)&num, sizeof(num), 0);
				MessageBox(NULL, L"服务器成功向客户端发送消息总条数", L"QQ", MB_ICONINFORMATION);

				if (num > 0)
				{
					for (auto it = users_account_on_chartroom.begin(); it != users_account_on_chartroom.end();)
					{
						//发送用户头像
						const BYTE* data = it->user_on_chartroom_image.data();
						int img_len = it->user_on_chartroom_image.size();

						int r = 0;
						int sum = 0;

						//发送相片长度
						send(client_server, (char*)&img_len, sizeof(img_len), 0);
						//发送相片内容
						while (sum < img_len)
						{
							r = send(client_server, (const char*)data + sum, img_len - sum, 0);
							if (r > 0)
							{
								sum += r;
							}
						}

						//发送用户昵称
						int nickname_len = 0;
						nickname_len = it->user_on_chartroom_nickname.size();
						send(client_server, (char*)&nickname_len, sizeof(nickname_len), 0);
						send(client_server, it->user_on_chartroom_nickname.c_str(), nickname_len, 0);

						//发送用户消息
						int inf_len = 0;
						inf_len = it->user_on_chartroom_inf.size();
						send(client_server, (char*)&inf_len, sizeof(inf_len), 0);
						send(client_server, it->user_on_chartroom_inf.c_str(), inf_len, 0);

						OutputDebugStringA(("the information user send in chartroom is" + it->user_on_chartroom_inf + "\n").c_str());
						it++;
					}
					MessageBox(NULL, L"服务器成功向客户端发送聊天室的聊天记录", L"QQ", MB_ICONINFORMATION);
				}
			}
			else if (strcmp(inf_str.c_str(), "退出") == 0)
			{
				si = 0;//退出消息接收循环
				MessageBox(NULL, L"已经成功接受来自客户端的聊天室的信息’退出’按钮", L"QQ", MB_ICONINFORMATION);

				std::string account = new_online_user_account;
				if (!account.empty() && account.back() == '\0')
				{
					account.pop_back();
				}

				//将该用户账号弹出队列
				//std::lock_guard<std::mutex>lock(users_anii_on_chartroom_mutex);//加锁//
				auto it = std::find_if(users_account_on_chartroom.begin(), users_account_on_chartroom.end(), [&account](const users_anii_on_chartroom& user) {return user.user_on_chartroom_account == account; });//若查找成功，返回第一个指向recv_user_account_str的指针
				if (it != users_account_on_chartroom.end()) 
				{
					users_account_on_chartroom.erase(it);
				}
				break;
			}
			else
			{
				si = 0;
				MessageBox(NULL, L"无法接受来自客户端的聊天室的信息请求按钮", L"QQ", MB_ICONERROR);
				return;
			}
		}
		
		//退出后将进入登录界面首页线程
		std::thread(Handlelogin_pro,client_server,my_data,new_online_user_account).detach();

	}
	else if (wcscmp(wstr.c_str(),L"好友")==0)
	{
		door_102:
		{
			//客户端进入好友功能
			//接收加载客户端用户好友列表的请求
			int msg_len = 0;
			recv(client_server, (char*)&msg_len, sizeof(msg_len), 0);
			std::string msg(msg_len, '\0');
			recv(client_server, &msg[0], msg_len, 0);
			//比较接收字符内容
			if (strcmp(msg.c_str(), "请求加载好友列表") != 0)
			{
				std::string msg_x = "failure";
				int msg_x_len = msg_x.size();
				send(client_server, (char*)&msg_x_len, sizeof(msg_x_len), 0);
				send(client_server, msg_x.c_str(), msg_x.size(), 0);
				//返回登录首界面处理线程
				std::thread(Handlelogin_pro, client_server, my_data, new_online_user_account).detach();//返回登录首界面处理线程
			}
			std::string msg_x = "success";
			int msg_x_len = msg_x.size();
			send(client_server, (char*)&msg_x_len, sizeof(msg_x_len), 0);
			send(client_server, msg_x.c_str(), msg_x.size(), 0);


			//依据用户账号加载用户的好友列表，new_online_user_account含‘\0'
			std::string user_account = new_online_user_account;
			if (!user_account.empty() && user_account.back() == '\0')
			{
				user_account.pop_back();
			}
			char *buf=new char[50]();
			snprintf(buf,50, "the user_account is %s\n",user_account.c_str());
			OutputDebugStringA(buf);
			delete[]buf;

			std::vector <std::string>user_friends_account;//好友账号列表队列
			user_friends_account.clear();



			//依据用户账号查询表该用户账号拥有的总行数
			MYSQL* conn_9 = mysql_init(NULL);
			int conn_9_count = 0;
			while (!mysql_real_connect(conn_9, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))
			{
				conn_9_count++;
				if (conn_9_count > 3)
				{
					break;
				}
			}

			const char* sql = "SELECT friend_account FROM friends WHERE account=?";
			MYSQL_STMT* stmt = mysql_stmt_init(conn_9);
			if (!stmt)
			{
				return;
			}
			if (mysql_stmt_prepare(stmt, sql, strlen(sql)))
			{
				mysql_stmt_close(stmt);
				return;
			}

			MYSQL_BIND bind[1]; // 1个参数
			memset(bind, 0, sizeof(bind));

			bind[0].buffer_type = MYSQL_TYPE_STRING;
			bind[0].buffer = (void*)user_account.c_str();
			bind[0].buffer_length = user_account.length();

			if (mysql_stmt_bind_param(stmt, bind))
			{
				mysql_stmt_close(stmt);
				return;
			}

			if (mysql_stmt_execute(stmt))
			{
				mysql_stmt_close(stmt);
				return;
			}

			//绑定输出结果
			char friend_account[65] = {0};
		
			unsigned long friend_account_len;
			MYSQL_BIND bind_result[1];
			memset(bind_result, 0, sizeof(bind_result));

			bind_result[0].buffer_type = MYSQL_TYPE_STRING;
			bind_result[0].buffer = (void*)friend_account;
			bind_result[0].buffer_length = sizeof(friend_account);
		
			if (mysql_stmt_bind_result(stmt,bind_result))
			{
				mysql_stmt_close(stmt);
				return;
			}

			int total_count = 0;
		
				while (mysql_stmt_fetch(stmt) == 0)
				{
					user_friends_account.push_back(friend_account);
				
					char *buf_3=new char[50]();
					snprintf(buf_3,50, "the friend_acount is %s\n",friend_account);
					OutputDebugStringA(buf_3);
					delete[]buf_3;
					total_count++;
				}
			
			mysql_stmt_close(stmt);
			mysql_close(conn_9);


			//服务器已经成功加载用户好友账户列表
			//还需要根据好友账号加载好友的昵称和头像

			//向客户端发送好友个数
			send(client_server, (char*)&total_count, sizeof(total_count), 0);
			char *buf_2=new char[50]();
			snprintf(buf_2,50, "the total_count is %d\n", total_count);
			OutputDebugStringA(buf_2);
			delete[]buf_2;


			int* online_user_signal = new int[total_count]();//好有用户在线标志
			int k = 0;


			MessageBox(NULL, L"已经成功向客户端发送好友个数消息", L"QQ",MB_ICONINFORMATION);

			if (total_count > 0 && !user_friends_account.empty())
			{
				std::vector <std::string>user_friends_nickname;//好友昵称列表队列
				std::vector< std::vector <BYTE>>user_friends_image;//好友头像列表

				user_friends_nickname.clear();
				user_friends_image.clear();

				//依据用户账号查询表该用户账号拥有的总行数
				MYSQL* conn_10 = mysql_init(NULL);
				int conn_10_count = 0;
				while (!mysql_real_connect(conn_10, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))
				{
					conn_10_count++;
					if (conn_10_count > 3)
					{
						break;
					}
				}


				//遍历好友账号列表
				for (auto it = user_friends_account.begin(); it != user_friends_account.end();)
				{
					//查询好友是否在线
					
					std::lock_guard<std::mutex>lok(g_onlineUsersMutex);//加锁

					for (auto i=g_onlineUsers.begin();i!=g_onlineUsers.end();)
					{
						std::string x = i->account;
						if (!x.empty() && x.back() == '\0')
						{
							x.pop_back();
						}

						if (strcmp((*it).c_str(), x.c_str()) == 0)
						{
							*(online_user_signal+k) = 1;
							break;
						}
						
						i++;
					}

					const char* sql_123 = "SELECT nickname,imgData FROM users WHERE account=? LIMIT 1";
					MYSQL_STMT* stmt = mysql_stmt_init(conn_10);
					if (!stmt)
					{
						return;
					}
					if (mysql_stmt_prepare(stmt, sql_123, strlen(sql_123)))
					{
						mysql_stmt_close(stmt);
						return;
					}

					MYSQL_BIND bind[1]; // 1个参数
					memset(bind, 0, sizeof(bind));

					bind[0].buffer_type = MYSQL_TYPE_STRING;
					bind[0].buffer = (void*)(*it).c_str();
					bind[0].buffer_length = (*it).length();


					if (mysql_stmt_bind_param(stmt, bind))
					{
						mysql_stmt_close(stmt);
						return;
					}

					if (mysql_stmt_execute(stmt))
					{
						mysql_stmt_close(stmt);
						return;
					}


					//绑定输出结果
					char* friend_nickname = new char[65]();
					BYTE* friend_image = new BYTE[3 * 1024 * 1024];

					unsigned long friend_nickname_len = 0;
					unsigned long friend_image_len = 0;


					MYSQL_BIND bind_result[2];
					memset(bind_result, 0, sizeof(bind_result));

					bind_result[0].buffer_type = MYSQL_TYPE_STRING;
					bind_result[0].buffer = (void*)friend_nickname;
					bind_result[0].buffer_length = 65;
					bind_result[0].length = &friend_nickname_len;

					bind_result[1].buffer_type = MYSQL_TYPE_BLOB;
					bind_result[1].buffer = (void*)friend_image;
					bind_result[1].buffer_length = 3 * 1024 * 1024;
					bind_result[1].length = &friend_image_len;

					if (mysql_stmt_bind_result(stmt, bind_result))
					{
						mysql_stmt_close(stmt);
						return;
					}

					if (mysql_stmt_fetch(stmt) == 0)
					{
						
						user_friends_nickname.push_back(friend_nickname);

						char* buf_5_5 = new char[50]();
						snprintf(buf_5_5, 50, "the friend_nickname is %s\n",friend_nickname);
						OutputDebugStringA(buf_5_5);
						delete[]buf_5_5;


						if (friend_image_len < 3 * 1024 * 1024)
						{
							//储存好友头像列表
							std::vector<BYTE>one_image(friend_image, friend_image + friend_image_len);
							//压入好友队列
							user_friends_image.push_back(std::move(one_image));
						}
						else
						{
							std::vector<BYTE>one_image(friend_image, friend_image + 3 * 1024 * 1024);
							//压入好友队列
							user_friends_image.push_back(std::move(one_image));
						}

					}

					delete[]friend_nickname;
					delete[]friend_image;
					mysql_stmt_close(stmt);
					it++;
					k++;
				}

				mysql_close(conn_10);

				//MessageBox(NULL,L"服务器已经成功加载用户的好友列表",L"QQ",MB_ICONINFORMATION);

				//将存储的用户好友账号、昵称和头像数据发送给客户端
				int i = 0;
				while (i < total_count)
				{
					std::string account_x = user_friends_account[i];//好友账号列表队列
					std::string nickname_x = user_friends_nickname[i];//好友昵称列表队列
					std::vector<BYTE>image_x = user_friends_image[i];//好友头像列表
					int signal = *(online_user_signal + i);
					
					//发送好友在线标志
					send(client_server, (char*)&signal, sizeof(signal), 0);

					//发送好友账号
					int account_x_len = account_x.size();
					send(client_server, (char*)&account_x_len, sizeof(account_x_len), 0);
					send(client_server, account_x.c_str(), account_x_len, 0);

					char* buf_4 = new char[50]();
					snprintf(buf_4, 50, "the send_account is %s\n",account_x.c_str());
					OutputDebugStringA(buf_4);
					delete[]buf_4;


					//发送好友昵称
					int nickname_x_len = nickname_x.size();
					send(client_server, (char*)&nickname_x_len, sizeof(nickname_x_len), 0);
					send(client_server, nickname_x.c_str(), nickname_x_len, 0);

					char* buf_5= new char[50]();
					snprintf(buf_5, 50, "the send_nickname is %s\n", nickname_x.c_str());
					OutputDebugStringA(buf_5);
					delete[]buf_5;


					//发送好友头像
					int image_x_len = image_x.size();
					send(client_server, (char*)&image_x_len, sizeof(image_x_len), 0);

					char* buf_6 = new char[50]();
					snprintf(buf_6, 50, "the image_len is %d\n", image_x_len);
					OutputDebugStringA(buf_6);
					delete[]buf_6;

					int x_r = 0;
					int x_sum = 0;
					while (x_sum < image_x_len)
					{
						x_r = send(client_server, (char*)image_x.data() + x_sum, image_x_len - x_sum, 0);
						if (x_r > 0)
						{
							x_sum += x_r;
						}
					}

					i++;
				}

				delete[]online_user_signal;
				MessageBox(NULL, L"服务器已经将用户的好友列表消息全部发送", L"QQ", MB_ICONINFORMATION);
			}


			//判断客户端所选择的操作

			while (true)
			{
				int oper_len = 0;
				recv(client_server, (char*)&oper_len, sizeof(oper_len), 0);
				std::string oper_str(oper_len, '\0');
				recv(client_server, &oper_str[0], oper_len, 0);

				char* buf_102 = new char[50];
				snprintf(buf_102,50,"the oper_str is %s\n",oper_str.c_str());
				OutputDebugStringA(buf_102);
				delete[]buf_102;

				if (strcmp(oper_str.c_str(), "退出") == 0)
				{
					//返回登录首界面处理线程
					std::thread(Handlelogin_pro, client_server, my_data, new_online_user_account).detach();//返回登录首界面处理线程
					return;
				}
				else if (strcmp(oper_str.c_str(), "刷新列表") == 0)
				{
					//重新加载好友列表
					goto door_102;
				}
				else if (strcmp(oper_str.c_str(), "聊天") == 0)
				{
					
					users_chart_information ix;

					//消息发送方的账号
					std::string account_sender = new_online_user_account;
					if (!account_sender.empty() && account_sender.back() == '\0')
					{
						account_sender.pop_back();
					}

					//接收消息接收方的账号
					int account_recver_len = 0;
					recv(client_server, (char*)&account_recver_len, sizeof(account_recver_len), 0);
					std::string account_recver(account_recver_len, '\0');
					recv(client_server, &account_recver[0], account_recver_len, 0);

					char* buf = new char[50];
					snprintf(buf, 50, "the account_receiver is %s\n", account_recver.c_str());
					OutputDebugStringA(buf);
					delete[]buf;

					//查询用户的昵称并发送
					std::string user_nickname1;
					std::string user_nickname2;

					MYSQL* conn_235 = mysql_init(NULL);
					int xxc = 0;
					while (!mysql_real_connect(conn_235, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))
					{
						xxc++;
						if (xxc > 3)
						{
							MessageBox(NULL, L"conn_235连接数据库失败", L"QQ", MB_ICONERROR);
						}
					}
					
					char* buffx = new char[80];
					snprintf(buffx, 80, "SELECT nickname FROM users WHERE account=%s LIMIT 1", account_sender.c_str());
					if (mysql_query(conn_235, buffx)==0)
					{
						MYSQL_RES* res = mysql_store_result(conn_235);
						if (res)
						{
							MYSQL_ROW row;
							if (row = mysql_fetch_row(res))
							{
								user_nickname1 = row[0];
							}
						}
						mysql_free_result(res);
					}

					char* buffy = new char[80];
					snprintf(buffy, 80, "SELECT nickname FROM users WHERE account=%s LIMIT 1", account_recver.c_str());
					if (mysql_query(conn_235, buffy) == 0)
					{
						MYSQL_RES* res2 = mysql_store_result(conn_235);
						if (res2)
						{
							MYSQL_ROW row2;
							if (row2 = mysql_fetch_row(res2))
							{
								user_nickname2 = row2[0];
							}
						}
						mysql_free_result(res2);
					}

					delete[]buffx;
					delete[]buffy;
					mysql_close(conn_235);

					int nickname1_len = user_nickname1.size();
					send(client_server, (char*)&nickname1_len, sizeof(nickname1_len), 0);
					send(client_server, user_nickname1.c_str(), nickname1_len, 0);


				door_1314:
					
					//接收好友聊天框初始化请求
					int client_len = 0;
					recv(client_server, (char*)&client_len, sizeof(client_len), 0);
					std::string client_str(client_len, '\0');
					recv(client_server, &client_str[0], client_len, 0);

					char* buf2 = new char[50];
					snprintf(buf2, 50, "the client_str is %s\n", client_str.c_str());
					OutputDebugStringA(buf2);
					delete[]buf2;

					if (strcmp("请求更新聊天框消息", client_str.c_str()) == 0)
					{
						if (g_users_chart_information.empty())
						{
							//通知客户端暂时没有聊天记录
							int kk = 0;
							send(client_server, (char*)&kk, sizeof(kk), 0);
						}
						else
						{
							users_chart_information ss;
							std::vector<users_chart_information>g_ss;
							int inf_sum = 0;

							for (auto & acc : g_users_chart_information)
							{
								if (strcmp(acc.inf_recv_account.c_str(), account_sender.c_str()) == 0  || strcmp(acc.inf_recv_account.c_str(), account_recver.c_str()) == 0)
								{
									//清空结构体
									ss = users_chart_information();

									ss.inf_send_account = acc.inf_send_account;
									ss.inf_recv_account = acc.inf_recv_account;
									ss.inf = acc.inf;

									g_ss.push_back(ss);

									inf_sum++;
								}
							}

							//向客户端发送消息条数
							send(client_server, (char*)&inf_sum, sizeof(inf_sum), 0);

							if (inf_sum>0)
							{
								//将符合条件的聊天记录发送
								int i = 0;
								while (i < inf_sum)
								{
									//消息发送方账号
									int inf_send_account_len=g_ss[i].inf_send_account.size();
									send(client_server, (char*)&inf_send_account_len, sizeof(inf_send_account_len), 0);
									send(client_server, g_ss[i].inf_send_account.c_str(), inf_send_account_len, 0);
									
									char* buf3 = new char[50];
									snprintf(buf3, 50, "the inf_send_account is %s\n",g_ss[i].inf_send_account.c_str());
									OutputDebugStringA(buf3);
									delete[]buf3;

									//消息接收方账号
									int inf_recv_account_len = g_ss[i].inf_recv_account.size();
									send(client_server, (char*)&inf_recv_account_len, sizeof(inf_recv_account_len), 0);
									send(client_server, g_ss[i].inf_recv_account.c_str(), inf_recv_account_len, 0);

									char* buf4 = new char[50];
									snprintf(buf4, 50, "the inf_recv_account is %s\n", g_ss[i].inf_recv_account.c_str());
									OutputDebugStringA(buf4);
									delete[]buf4;

									//消息
									int inf_len = g_ss[i].inf.size();
									send(client_server, (char*)&inf_len, sizeof(inf_len), 0);
									send(client_server, g_ss[i].inf.c_str(), inf_len, 0);

									char* buf5 = new char[50];
									snprintf(buf5, 50, "the inf is %s\n", g_ss[i].inf.c_str());
									OutputDebugStringA(buf5);
									delete[]buf5;

									i++;
								}
							}

						}

						
					//接收来自客户端的消息
					while (true)
					{
						//接收客户端发来的在好友聊天框的按钮选择
						int sel_len = 0;
						recv(client_server, (char*)&sel_len, sizeof(sel_len), 0);
						std::string sel_str(sel_len,'\0');
						recv(client_server,&sel_str[0],sel_len,0);

						char* buf6 = new char[50];
						snprintf(buf6, 50, "the sel_str is %s\n",sel_str.c_str());
						OutputDebugStringA(buf6);
						delete[]buf6;

						if(strcmp("发送消息",sel_str.c_str()) == 0)
						{
							ix = users_chart_information();

							//接收消息
							int msg_len = 0;
							recv(client_server, (char*)&msg_len, sizeof(msg_len), 0);
							std::string msg(msg_len, '\0');
							recv(client_server, &msg[0], msg_len, 0);

							//记录在监控框

							{
								int w_len1 = MultiByteToWideChar(CP_UTF8, 0, user_nickname1.c_str(),user_nickname1.size(), NULL, 0);//计算出转换长度//
								std::wstring w_str1(w_len1, 0);//分配空间//
								MultiByteToWideChar(CP_UTF8, 0, user_nickname1.c_str(), user_nickname1.size(), &w_str1[0], w_len1);//实际转换//

								int w_len2 = MultiByteToWideChar(CP_UTF8, 0, user_nickname2.c_str(),user_nickname2.size(), NULL, 0);//计算出转换长度//
								std::wstring w_str2(w_len2, 0);//分配空间//
								MultiByteToWideChar(CP_UTF8, 0, user_nickname2.c_str(), user_nickname2.size(), &w_str2[0], w_len2);//实际转换//

								int w_len3 = MultiByteToWideChar(CP_UTF8, 0, msg.c_str(), msg_len, NULL, 0);//计算出转换长度//
								std::wstring w_str3(w_len3, 0);//分配空间//
								MultiByteToWideChar(CP_UTF8, 0, msg.c_str(), msg_len, &w_str3[0], w_len3);//实际转换//
								//MessageBox(NULL, L"接收登录界面客户端发送的对话框消息成功", L"QQ", MB_ICONINFORMATION);

								//MessageBox(NULL, w_str.c_str(), L"QQ", MB_ICONINFORMATION);
								//获取当前时间//
								time_t now_one = time(0);//获取当前时间，精确到秒//
								tm tm_one;//声明一个结构体，用于存储本地时间的各个组成部分//
								localtime_s(&tm_one, &now_one);//将now_one（输入）里的内容复制到tm_one里输出//
								wchar_t timeBuffer_one[64];
								wcsftime(timeBuffer_one, sizeof(timeBuffer_one) / sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", &tm_one);
								//MessageBox(NULL, L"已经准备好了时间字段", L"QQ", MB_ICONINFORMATION);

								std::wstring msg_one = L"[";
								msg_one += timeBuffer_one;
								msg_one += L"][";
								//MessageBox(NULL, L"已经成功将昵称末尾的‘0’弹出", L"QQ", MB_ICONINFORMATION);
								msg_one += w_str1.c_str();
								//MessageBox(NULL, L"已经成功拼接昵称", L"QQ", MB_ICONINFORMATION);
								msg_one += L"->";
								//MessageBox(NULL, L"已经成功拼接好->", L"QQ", MB_ICONINFORMATION);
								msg_one += w_str2.c_str();
								msg_one += L"]";
								//MessageBox(NULL, L"已经成功拼接好服务器字段", L"QQ", MB_ICONINFORMATION);
								msg_one += w_str3.c_str();
								
								EnterCriticalSection(&g_csMsg_two_Queue);//加锁//
								g_msg_two_Queue.push(msg_one);//创建副本//
								LeaveCriticalSection(&g_csMsg_two_Queue);//解锁//
								//MessageBox(NULL, L"已经成功将信息拼接", L"QQ", MB_ICONINFORMATION);
								// 记录到服务器监控室//
								PostMessage(g_hInfoDialogmonitor, WM_APP_UPDATEMONITOR_MSG, 0, 0);
								//MessageBox(NULL, L"已经成功通知信息的监控显示", L"QQ", MB_ICONINFORMATION);

							}


							ix.inf_send_account = account_sender;
							ix.inf_recv_account = account_recver;
							ix.inf = msg;

							//将其压入队列g_users_chart_information
							std::lock_guard<std::mutex>lok(g_users_chart_information_mutex);
							g_users_chart_information.push_back(ix);

							//更新消息
							goto door_1314;

						}

						else if (strcmp("更新消息", sel_str.c_str()) == 0)
						{
							goto door_1314;
						}

						else if (strcmp("退出", sel_str.c_str()) == 0)
						{
							//返回到好友框
							goto door_102;
						}

					}
					}
					else
					{
						const char* buf = new char[50];
						buf = "接收好友聊天框初始化请求失败";
						OutputDebugStringA(buf);
						delete[]buf;
						return;
					}

					

				}
				else if (strcmp(oper_str.c_str(), "查看好友信息") == 0)
				{
					//接收好友的账号
					int ac_len = 0;
					recv(client_server, (char*)&ac_len, sizeof(ac_len), 0);
					std::string ac(ac_len,'\0');
					recv(client_server, &ac[0], ac_len, 0);
                    
					char* buf_account = new char[50]();
					snprintf(buf_account, 50, "the fri_ac is %s\n",ac.c_str());
					OutputDebugStringA(buf_account);
					delete[]buf_account;


					//查询好友的个人信息
					MYSQL* conn_421 = mysql_init(NULL);
					int x_x = 0;
					while (!mysql_real_connect(conn_421,"127.0.0.1","myqq_admin","123456","myqq_database",3306,NULL,0))
					{
						x_x++;
						if (x_x > 3)
						{
							MessageBox(NULL,L"连接数据库conn_421失败",L"QQ",MB_ICONERROR);
							return;
						}
					}

					//sql语句
					const char* sql_421 = "SELECT gender,age,signature FROM users WHERE account=? LIMIT 1";
					//获取预处理指针
					MYSQL_STMT* stmt_421 = mysql_stmt_init(conn_421);
					
					if (!stmt_421)
					{
						MessageBox(NULL,L"获取预处理指针失败",L"QQ",MB_ICONERROR);
						mysql_close(conn_421);
						return;
					}

					//将sql语句转换为数据库内部结构
					if (mysql_stmt_prepare(stmt_421, sql_421, strlen(sql_421)))
					{
						MessageBox(NULL, L"数据库预处理语句执行失败", L"QQ", MB_ICONERROR);
						mysql_stmt_close(stmt_421);
						mysql_close(conn_421);
						return;
					}

					//绑定参数
					MYSQL_BIND bind_param[1];
					memset(bind_param, 0, sizeof(bind_param));

					bind_param[0].buffer_type = MYSQL_TYPE_STRING;
					bind_param[0].buffer = (void*)ac.c_str();
					bind_param[0].buffer_length = ac.size();

					if (mysql_stmt_bind_param(stmt_421, bind_param))
					{
						MessageBox(NULL, L"数据库绑定参数失败", L"QQ", MB_ICONERROR);
						mysql_stmt_close(stmt_421);
						mysql_close(conn_421);
						return;
					}

					if (mysql_stmt_execute(stmt_421))
					{
						MessageBox(NULL, L"数据库绑定参数失败", L"QQ", MB_ICONERROR);
						mysql_stmt_close(stmt_421);
						mysql_close(conn_421);
						return;
					}

					char* friend_gender = new char[8]();
					char* friend_age = new char[8]();
					char* friend_signature = new char[256]();

					//绑定结果参数
					MYSQL_BIND bind_result[3];
					memset(bind_result, 0, sizeof(bind_result));

					bind_result[0].buffer_type = MYSQL_TYPE_STRING;
					bind_result[0].buffer = friend_gender;
					bind_result[0].buffer_length = 8;

					bind_result[1].buffer_type = MYSQL_TYPE_STRING;
					bind_result[1].buffer = friend_age;
					bind_result[1].buffer_length = 8;

					bind_result[2].buffer_type = MYSQL_TYPE_STRING;
					bind_result[2].buffer = friend_signature;
					bind_result[2].buffer_length = 256;

					if (mysql_stmt_bind_result(stmt_421, bind_result))
					{
						MessageBox(NULL, L"数据库绑定结果参数失败", L"QQ", MB_ICONERROR);
						mysql_stmt_close(stmt_421);
						mysql_close(conn_421);
						return;
					}


					//将结果缓冲到本地
					if (mysql_stmt_store_result(stmt_421))
					{
						MessageBox(NULL, L"将查询结果缓存到客户端本地失败", L"QQ", MB_ICONERROR);
						mysql_stmt_close(stmt_421);
						mysql_close(conn_421);
						return;
					}

					if (mysql_stmt_fetch(stmt_421))
					{
						MessageBox(NULL, L"无法取出查询结果", L"QQ", MB_ICONERROR);
						mysql_stmt_close(stmt_421);
						mysql_close(conn_421);
						return;
					}
					
					mysql_stmt_close(stmt_421);
					mysql_close(conn_421);


					//发送好友性别
					int fri_gender_len = strlen(friend_gender);
					send(client_server, (char*)&fri_gender_len, sizeof(fri_gender_len), 0);
					send(client_server, friend_gender, fri_gender_len, 0);

					char* buf_gender = new char[50]();
					snprintf(buf_gender, 50, "the fri_gender is %s\n",friend_gender);
					OutputDebugStringA(buf_gender);
					delete[]buf_gender;


					//发送好友年龄
					int fri_age_len = strlen(friend_age);
					send(client_server, (char*)&fri_age_len, sizeof(fri_age_len), 0);
					send(client_server,friend_age,fri_age_len,0);

					char* buf_age = new char[50]();
					snprintf(buf_age, 50, "the fri_age is %s\n", friend_age);
					OutputDebugStringA(buf_age);
					delete[]buf_age;

					//发送好友的个性签名
					int fri_signature_len = strlen(friend_signature);
					send(client_server,(char*)&fri_signature_len,sizeof(fri_signature_len),0);
					send(client_server,friend_signature,fri_signature_len,0);


					char* buf_signature = new char[50]();
					snprintf(buf_signature, 50, "the fri_signature is %s\n", friend_signature);
					OutputDebugStringA(buf_signature);
					delete[]buf_signature;

					delete[]friend_gender;
					delete[]friend_age;
					delete[]friend_signature;

                    //接收客户端在好友信息展示框的消息
					
					int x_len = 0;
					recv(client_server, (char*)&x_len, sizeof(x_len), 0);
					std::string x_str(x_len, '\0');
					recv(client_server, &x_str[0], x_len, 0);

					char* buf_inf = new char[50]();
					snprintf(buf_inf, 50, "the information is %s\n",x_str.c_str());
					OutputDebugStringA(buf_inf);
					delete[]buf_inf;

					if (strcmp("取消", x_str.c_str()) == 0)
					{

						//发送服务器处理标志
						std::string signal_x = "success";
						int str_x_len_x = signal_x.size();
						send(client_server, (char*)&str_x_len_x, sizeof(str_x_len_x), 0);
						send(client_server, signal_x.c_str(), str_x_len_x, 0);
					}

					//返回好友框
					goto door_102;
				}
				else if (strcmp(oper_str.c_str(), "客户端请求添加好友") == 0)
				{

                 door_103:
					//接收客户端发来的确定或取消按钮
					int st_len = 0;
					recv(client_server, (char*)&st_len, sizeof(st_len), 0);
					std::string st(st_len, '\0');
					recv(client_server, &st[0], st_len, 0);

					char* buf_22 = new char[50]();
					snprintf(buf_22, 50, "the st is %s\n",st.c_str());
					OutputDebugStringA(buf_22);
					delete[]buf_22;


					if (strcmp(st.c_str(), "确定") == 0)
					{
						//接收客户端发来的好友账号
						int str_len = 0;
						recv(client_server, (char*)&str_len, sizeof(str_len), 0);
						std::string str_x(str_len, '\0');
						recv(client_server, &str_x[0], str_len, 0);//账号含'\0'

						if (!str_x.empty() && str_x.back() == '\0')
						{
							str_x.pop_back();
						}

						char* buf_23 = new char[50]();
						snprintf(buf_23, 50, "the str_x is %s\n",str_x.c_str());
						OutputDebugStringA(buf_23);
						delete[]buf_23;


						//到users表中查询该用户账号是否存在

						MYSQL* conn_12 = mysql_init(NULL);
						int conn_12_count = 0;
						while (!mysql_real_connect(conn_12, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))
						{
							conn_12_count++;
							if (conn_12_count > 3)
							{
								return;
							}
						}

						char sql[128];
						snprintf(sql, sizeof(sql), "SELECT 1 FROM users WHERE account ='%s' LIMIT 1", str_x.c_str());


						if (mysql_query(conn_12, sql) == 0)
						{
							MYSQL_RES* res = mysql_store_result(conn_12);
							if (res)
							{
								if (mysql_num_rows(res) > 0)
								{
									//账号存在
									mysql_close(conn_12);
									//检查该账号用户是否已经是发起添加好友请求的用户的好友 ，即检查friends表
									MessageBox(NULL, L"服务器查询到客户端发来的所要添加好友的用户账号存在",L"QQ",MB_ICONINFORMATION);

									std::string host_account_y= new_online_user_account;//发起人账号
									if (!host_account_y.empty() && host_account_y.back() == '\0')
									{
										host_account_y.pop_back();
									}
									std::string fri_account_y = str_x;//添加人账号


									MYSQL* conn_1211 = mysql_init(NULL);
									int conn_1211_count = 0;
									while (!mysql_real_connect(conn_1211, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))
									{
										conn_1211_count++;
										if (conn_1211_count > 3)
										{
											mysql_close(conn_1211);
											return;
										}
									}

									//MessageBox(NULL, L"服务器成功链接数据库来验证两人是否已是好友", L"QQ", MB_ICONINFORMATION);

									char sql_y[128];
									snprintf(sql_y, sizeof(sql_y), "SELECT 1 FROM friends WHERE account ='%s' AND friend_account='%s' LIMIT 1", host_account_y.c_str(), fri_account_y.c_str());
									if (mysql_query(conn_1211, sql_y) == 0)
									{
										MYSQL_RES* res = mysql_store_result(conn_1211);
										if (res)
										{
											if (mysql_num_rows(res) > 0)
											{
												//通知客户端账号存在
											   //MessageBox(NULL, L"服务器查询到客户端发来的所要添加的好友，已在用户的好友列表", L"QQ", MB_ICONINFORMATION);

												std::string inf = "你们已经是好友，无需在发送好友请求";
												int inf_len = inf.size();

												send(client_server, (char*)&inf_len, sizeof(inf_len), 0);
												send(client_server, inf.c_str(), inf_len, 0);

												mysql_close(conn_1211);
												goto door_102;
											}

											else 
											{
												//MessageBox(NULL, L"服务器查询到客户端发来的所要添加好友不在用户的好友列表，即将它加入half_friend,等待同意", L"QQ", MB_ICONINFORMATION);

												mysql_close(conn_1211);
												//检查half_friend中是否已存在相关的请求，若有则忽略，若没有，则插入
												//将好友添加发起人账号和添加人账号记录到表half_friend
												std::string host_account_x = new_online_user_account;//发起人账号
												if (!host_account_x.empty() && host_account_x.back() == '\0')
												{
													host_account_x.pop_back();
												}
												std::string fri_account_x = str_x;//添加人账号


												MYSQL* conn_121 = mysql_init(NULL);
												int conn_121_count = 0;
												while (!mysql_real_connect(conn_121, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))
												{
													conn_121_count++;
													if (conn_121_count > 3)
													{
														mysql_close(conn_121);
														return;
													}
												}
												//MessageBox(NULL, L"服务器成功链接服务器来验证是否已有相同的添加好友请求", L"QQ", MB_ICONINFORMATION);

												char sql_x[128];
												snprintf(sql_x, sizeof(sql_x), "SELECT 1 FROM half_friend WHERE account ='%s' AND friend_account='%s' LIMIT 1", host_account_x.c_str(), fri_account_x.c_str());
												if (mysql_query(conn_121, sql_x) == 0)
												{
													MYSQL_RES* res = mysql_store_result(conn_121);
													if (res)
													{
														if (mysql_num_rows(res) > 0)
														{
															//通知客户端账号存在
															//MessageBox(NULL, L"服务器查询到该用户发送过相同的好友添加请求", L"QQ", MB_ICONINFORMATION);

															std::string inf = "已经发送过好友请求，请耐心等待";
															int inf_len = inf.size();

															send(client_server, (char*)&inf_len, sizeof(inf_len), 0);
															send(client_server, inf.c_str(), inf_len, 0);

															mysql_close(conn_121);
															goto door_102;

														}

														else
														{
															mysql_close(conn_121);
															//将好友添加发起人账号和添加人账号记录到表half_friend
															std::string host_account = new_online_user_account;//发起人账号
															if (!host_account.empty() && host_account.back() == '\0')
															{
																host_account.pop_back();
															}
															std::string fri_account = str_x;//添加人账号

															//host_account添加到account字段下
															MYSQL* conn_101 = mysql_init(NULL);
															int conn_101_count = 0;

															while (!mysql_real_connect(conn_101, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))
															{
																conn_101_count++;
																if (conn_101_count > 3)
																{
																	mysql_close(conn_101);
																	return;
																}
															}

															//MessageBox(NULL, L"服务器即将用户账号和所要添加的好友账号添加到half_friend表", L"QQ", MB_ICONINFORMATION);

															const char* sql_222 = "INSERT INTO half_friend (account,friend_account)VALUES(?,?)";
															MYSQL_STMT* stmt = mysql_stmt_init(conn_101);
															if (!stmt)
															{
																mysql_close(conn_101);
																return;
															}
															if (mysql_stmt_prepare(stmt, sql_222, strlen(sql_222)))
															{
																mysql_stmt_close(stmt);
																mysql_close(conn_101);
																return;
															}
															MYSQL_BIND bind[2];
															memset(bind, 0, sizeof(bind));

															bind[0].buffer_type = MYSQL_TYPE_STRING;
															bind[0].buffer = (void*)host_account.c_str();
															bind[0].buffer_length = host_account.length();

															bind[1].buffer_type = MYSQL_TYPE_STRING;
															bind[1].buffer = (void*)fri_account.c_str();
															bind[1].buffer_length = fri_account.length();

															if (mysql_stmt_bind_param(stmt, bind))
															{
																mysql_stmt_close(stmt);
																mysql_close(conn_101);
																return;
															}

															if (mysql_stmt_execute(stmt))
															{
																mysql_stmt_close(stmt);
																mysql_close(conn_101);
																return;

															}

															//成功插入
															mysql_stmt_close(stmt);
															mysql_close(conn_101);
															//fri_account添加到friend_account字段下
															//MessageBox(NULL, L"服务器成功将用户账号和所要添加的好友账号添加到half_friend表", L"QQ", MB_ICONINFORMATION);

														   //通知客户端账号存在
															std::string inf = "添加的好友账号存在";
															int inf_len = inf.size();

															send(client_server, (char*)&inf_len, sizeof(inf_len), 0);
															send(client_server, inf.c_str(), inf_len, 0);

															goto door_102;
														}

													}

												}

												else
												{
													//MessageBox(NULL, L"服务器查询失败3", L"QQ", MB_ICONERROR);
													return;
												}
											}
										}
									}
									else
									{
										//MessageBox(NULL, L"服务器查询失败2", L"QQ", MB_ICONERROR);
										return;
									}

								}

								else
								{
									//账号不存在
									std::string inf = "添加的好友账号不存在";
									int inf_len = inf.size();
									send(client_server, (char*)&inf_len, sizeof(inf_len), 0);
									send(client_server, inf.c_str(), inf_len, 0);
									
									goto door_103;
								}
							}

							else//取结果集失败
							{
								mysql_close(conn_12);
								return;

							}

						}
						else//未查询成功过
						{
							mysql_close(conn_12);
							return;
						}

					}
					else if (strcmp(st.c_str(), "取消") == 0)
					{
						goto door_102;
					}

				}
				else if (strcmp(oper_str.c_str(), "删除好友") == 0)
				{
					//接收服务器处理标志
					int x_len = 0;
					recv(client_server,(char*)&x_len, sizeof(x_len), 0);
					std::string x_str(x_len, '\0');
					recv(client_server, &x_str[0], x_len, 0);
					
					std::string xx = new_online_user_account;
					if (!xx.empty() && xx.back() == '\0')
					{
						xx.pop_back();
					}

					//删除half_friend表中的相关记录
					MYSQL* conn_123 = mysql_init(NULL);
					int x_y = 0;
					while (!mysql_real_connect(conn_123, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))
					{
						x_y++;
						if (x_y > 3)
						{
							MessageBox(NULL, L"连接不上数据库，即将退出1", L"QQ", MB_ICONERROR);
							return;
						}
					}

					const char* sql_123 = "DELETE FROM friends WHERE friend_account=? AND account=?";
					MYSQL_STMT* stmt = mysql_stmt_init(conn_123);

					if (!stmt)
					{
						MessageBox(NULL, L"数据库预处理指针获取失败1", L"QQ", MB_ICONERROR);
						mysql_close(conn_123);
						return;
					}

					if (mysql_stmt_prepare(stmt, sql_123, strlen(sql_123)))
					{
						MessageBox(NULL, L"数据库预处理语句处理失败", L"QQ", MB_ICONERROR);
						mysql_stmt_close(stmt);
						mysql_close(conn_123);
						return;
					}


					
						MYSQL_BIND bind_param[2];
						memset(bind_param, 0, sizeof(bind_param));

						bind_param[0].buffer_type = MYSQL_TYPE_STRING;
						bind_param[0].buffer = (void*)xx.c_str();
						bind_param[0].buffer_length = xx.size();


						bind_param[1].buffer_type = MYSQL_TYPE_STRING;
						bind_param[1].buffer = (void*)x_str.c_str();
						bind_param[1].buffer_length = x_str.size();

						if (mysql_stmt_bind_param(stmt, bind_param))
						{
							MessageBox(NULL, L"数据库绑定参数处理失败", L"QQ", MB_ICONERROR);
							mysql_stmt_close(stmt);
							mysql_close(conn_123);
							return;
						}

						if (mysql_stmt_execute(stmt))
						{
							MessageBox(NULL, L"数据库执行语句执行失败", L"QQ", MB_ICONERROR);
							mysql_stmt_close(stmt);
							mysql_close(conn_123);
							return;
						}

					//将friend_account,account 字段的内容交换

						//MYSQL_BIND bind_param[2];
						memset(bind_param, 0, sizeof(bind_param));

						bind_param[1].buffer_type = MYSQL_TYPE_STRING;
						bind_param[1].buffer = (void*)xx.c_str();
						bind_param[1].buffer_length = xx.size();


						bind_param[0].buffer_type = MYSQL_TYPE_STRING;
						bind_param[0].buffer = (void*)x_str.c_str();
						bind_param[0].buffer_length = x_str.size();

						if (mysql_stmt_bind_param(stmt, bind_param))
						{
							MessageBox(NULL, L"数据库绑定参数处理失败", L"QQ", MB_ICONERROR);
							mysql_stmt_close(stmt);
							mysql_close(conn_123);
							return;
						}

						if (mysql_stmt_execute(stmt))
						{
							MessageBox(NULL, L"数据库执行语句执行失败", L"QQ", MB_ICONERROR);
							mysql_stmt_close(stmt);
							mysql_close(conn_123);
							return;
						}


					mysql_stmt_close(stmt);
					mysql_close(conn_123);

					std::string cc = "success";
					int cc_l = cc.size();
					send(client_server, (char*)&cc_l, sizeof(cc_l), 0);
					send(client_server, cc.c_str(), cc_l, 0);

					goto door_102;

				}
				else if (strcmp(oper_str.c_str(), "新朋友") == 0)
				{
					//接收长度
					int r_len = 0;
					recv(client_server, (char*)&r_len, sizeof(r_len), 0);
					std::string s_str(r_len,'\0');
					recv(client_server, &s_str[0],r_len,0);

					char* buf_x1 = new char[50]();
					snprintf(buf_x1, 50, "the s_str is %s\n",s_str.c_str());
					OutputDebugStringA(buf_x1);
					delete[]buf_x1;


					//比较接受内容
					if (strcmp(s_str.c_str(), "客户端请求加载新朋友列表") == 0)
					{
						std::string send_inf = "success";
						int send_len = send_inf.size();
						send(client_server, (char*)&send_len, sizeof(send_len), 0);
						send(client_server,send_inf.c_str(),send_len,0);

						char* buf_x2 = new char[50]();
						snprintf(buf_x2, 50, "the send_inf is %s\n",send_inf.c_str());
						OutputDebugStringA(buf_x2);
						delete[]buf_x2;

						std::vector<std::string> half_friend_account_list;
						half_friend_account_list.clear();


						//查询表half_friend,查看一共有多少条friend_account为用户账户的记录
						int half_f = 0;

						MYSQL* conn_45 = mysql_init(NULL);
						int conn_45_count = 0;
						while (!mysql_real_connect(conn_45, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))
						{
							conn_45_count++;
							if (conn_45_count > 3)
							{
								mysql_close(conn_45);
								return;
							}
						}

						std::string u_r = new_online_user_account;
						if (!u_r.empty() && u_r.back() == '\0')
						{
							u_r.pop_back();
						}

						char* sql_666 = new char[81];
						snprintf(sql_666,81,"SELECT account FROM half_friend WHERE friend_account=%s",u_r.c_str());
						
						if (mysql_query(conn_45, sql_666) == 0)
						{
							MYSQL_RES* res = mysql_store_result(conn_45);
							if (res)
							{
								MYSQL_ROW row;
								while((row=mysql_fetch_row(res))!=NULL)
								{
									half_friend_account_list.push_back(row[0]);//存储发起添加好友的用户账号
									half_f++;
								}
								mysql_free_result(res);
							}
						}
						mysql_close(conn_45);
						delete[]sql_666;

						//向客户端发送新朋友的人数
						send(client_server,(char*)&half_f,sizeof(half_f),0);

						char* buf_x3 = new char[50]();
						snprintf(buf_x3, 50, "the total half_friend is %d\n",half_f);
						OutputDebugStringA(buf_x3);
						delete[]buf_x3;
						
						if (half_f == 0)
						{
							goto door_102;//返回好友框首页
						}

						//从表users查询添加好友发起人的昵称和头像
						else if (half_f > 0)
						{
							std::vector <std::string> half_friends_nickname;
							std::vector <std::vector<BYTE>> half_friends_image;

							half_friends_nickname.clear();
							half_friends_image.clear();

							//查询表half_friend,查看一共有多少条friend_account为用户账户的记录
							MYSQL* conn_4544 = mysql_init(NULL);
							int conn_4544_count = 0;
							while (!mysql_real_connect(conn_4544, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))
							{
								conn_4544_count++;
								if (conn_4544_count > 3)
								{
									mysql_close(conn_4544);
									return;
								}
							}

								const char* sql_333 = "SELECT nickname,imgData FROM users WHERE account=? LIMIT 1";
								MYSQL_STMT* stmt = mysql_stmt_init(conn_4544);
								if (!stmt)
								{
									return;
								}
								if (mysql_stmt_prepare(stmt, sql_333, strlen(sql_333)))
								{
									mysql_stmt_close(stmt);
									return;
								}

							for (auto it = half_friend_account_list.begin(); it != half_friend_account_list.end();)
							{

								MYSQL_BIND bind[1]; // 1个参数
								memset(bind, 0, sizeof(bind));

								bind[0].buffer_type = MYSQL_TYPE_STRING;
								bind[0].buffer = (void*)(*it).c_str();
								bind[0].buffer_length = (*it).length();


								if (mysql_stmt_bind_param(stmt, bind))
								{
									mysql_stmt_close(stmt);
									return;
								}

								if (mysql_stmt_execute(stmt))
								{
									mysql_stmt_close(stmt);
									return;
								}


								//绑定输出结果
								char* friend_nickname = new char[65]();
								BYTE* friend_image = new BYTE[3 * 1024 * 1024];

								unsigned long friend_nickname_len = 0;
								unsigned long friend_image_len = 0;


								MYSQL_BIND bind_result[2];
								memset(bind_result, 0, sizeof(bind_result));

								bind_result[0].buffer_type = MYSQL_TYPE_STRING;
								bind_result[0].buffer = (void*)friend_nickname;
								bind_result[0].buffer_length = 65;
								bind_result[0].length = &friend_nickname_len;

								bind_result[1].buffer_type = MYSQL_TYPE_BLOB;
								bind_result[1].buffer = (void*)friend_image;
								bind_result[1].buffer_length = 3 * 1024 * 1024;
								bind_result[1].length = &friend_image_len;

								if (mysql_stmt_bind_result(stmt, bind_result))
								{
									mysql_stmt_close(stmt);
									return;
								}

								if (mysql_stmt_fetch(stmt) == 0)
								{

									half_friends_nickname.push_back(friend_nickname);

									char* buf_5_5 = new char[50]();
									snprintf(buf_5_5, 50, "the friend_nickname is %s\n", friend_nickname);
									OutputDebugStringA(buf_5_5);
									delete[]buf_5_5;


									if (friend_image_len < 3 * 1024 * 1024)
									{
										//储存好友头像列表
										std::vector<BYTE>one_image(friend_image, friend_image + friend_image_len);
										//压入好友队列
										half_friends_image.push_back(std::move(one_image));
									}
									else
									{
										std::vector<BYTE>one_image(friend_image, friend_image + 3 * 1024 * 1024);
										//压入好友队列
										half_friends_image.push_back(std::move(one_image));
									}

								}

								delete[]friend_nickname;
								delete[]friend_image;
								it++;
							}

							mysql_stmt_close(stmt);
							mysql_close(conn_4544);


							//向客户端发送新朋友的消息
							int i = 0;
							while (i < half_f)
							{
								std::string account_x = half_friend_account_list[i];//好友账号列表队列
								std::string nickname_x = half_friends_nickname[i];//好友昵称列表队列
								std::vector<BYTE>image_x = half_friends_image[i];//好友头像列表
								

								//发送好友账号
								int account_x_len = account_x.size();
								send(client_server, (char*)&account_x_len, sizeof(account_x_len), 0);
								send(client_server, account_x.c_str(), account_x_len, 0);

								char* buf_4 = new char[50]();
								snprintf(buf_4, 50, "the send_account is %s\n", account_x.c_str());
								OutputDebugStringA(buf_4);
								delete[]buf_4;


								//发送好友昵称
								int nickname_x_len = nickname_x.size();
								send(client_server, (char*)&nickname_x_len, sizeof(nickname_x_len), 0);
								send(client_server, nickname_x.c_str(), nickname_x_len, 0);

								char* buf_5 = new char[50]();
								snprintf(buf_5, 50, "the send_nickname is %s\n", nickname_x.c_str());
								OutputDebugStringA(buf_5);
								delete[]buf_5;


								//发送好友头像
								int image_x_len = image_x.size();
								send(client_server, (char*)&image_x_len, sizeof(image_x_len), 0);

								char* buf_6 = new char[50]();
								snprintf(buf_6, 50, "the image_len is %d\n", image_x_len);
								OutputDebugStringA(buf_6);
								delete[]buf_6;

								int x_r = 0;
								int x_sum = 0;
								while (x_sum < image_x_len)
								{
									x_r = send(client_server, (char*)image_x.data() + x_sum, image_x_len - x_sum, 0);
									if (x_r > 0)
									{
										x_sum += x_r;
									}
								}

								i++;
							}


							//响应客户端新朋友框的选择
							
							//接收长度
							int r_len_x= 0;
							recv(client_server, (char*)&r_len_x, sizeof(r_len_x), 0);
							std::string s_str_x(r_len_x, '\0');
							recv(client_server, &s_str_x[0], r_len_x, 0);
							
							char* buf_113 = new char[50];
							snprintf(buf_113, strlen(buf_113), "the s_str_x is %s\n",s_str_x.c_str());
							OutputDebugStringA(buf_113);
							delete[]buf_113;

							if (strcmp("同意",s_str_x.c_str()) == 0)
							{
								//接收新朋友数目
								int f_len = 0;
								recv(client_server, (char*)&f_len, sizeof(f_len), 0);

								char* buf_114 = new char[50];
								snprintf(buf_114, strlen(buf_114), "the f_len is %d\n",f_len);
								OutputDebugStringA(buf_114);
								delete[]buf_114;
								//接收客户端发来的同意的新朋友账号
								//用容器存储客户端发来的账号
								std::vector<std::string> new_friends;

								for (int i = 0; i < f_len; i++)
								{
									int acc_len = 0;
									recv(client_server,(char*)&acc_len,sizeof(acc_len),0);
									std::string acc(acc_len,'\0');
									recv(client_server, &acc[0], acc_len, 0);

									char* buf_116 = new char[50];
									snprintf(buf_116, strlen(buf_116), "the acc is %s\n",acc.c_str());
									OutputDebugStringA(buf_116);
									delete[]buf_116;

									new_friends.push_back(acc);
								}

								std::string xx = new_online_user_account;
								if (!xx.empty() && xx.back() == '\0')
								{
									xx.pop_back();
								}

								//删除half_friend表中的相关记录
								MYSQL* conn_123 = mysql_init(NULL);
								int x_y = 0;
								while (!mysql_real_connect(conn_123, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))
								{
									x_y++;
									if (x_y > 3)
									{
										MessageBox(NULL, L"连接不上数据库，即将退出1", L"QQ", MB_ICONERROR);
										return;
									}
								}

								const char* sql_123 = "DELETE FROM half_friend WHERE friend_account=? AND account=?";
								MYSQL_STMT* stmt = mysql_stmt_init(conn_123);

								if (!stmt)
								{
									MessageBox(NULL, L"数据库预处理指针获取失败1", L"QQ", MB_ICONERROR);
									mysql_close(conn_123);
									return;
								}

								if (mysql_stmt_prepare(stmt, sql_123, strlen(sql_123)))
								{
									MessageBox(NULL, L"数据库预处理语句处理失败", L"QQ", MB_ICONERROR);
									mysql_stmt_close(stmt);
									mysql_close(conn_123);
									return;
								}

								for (auto it=new_friends.begin();it!=new_friends.end();)
								{
									
									MYSQL_BIND bind_param[2];
									memset(bind_param, 0, sizeof(bind_param));

									bind_param[0].buffer_type = MYSQL_TYPE_STRING;
									bind_param[0].buffer = (void*)xx.c_str();
									bind_param[0].buffer_length = xx.size();


									bind_param[1].buffer_type = MYSQL_TYPE_STRING;
									bind_param[1].buffer = (void*)(*it).c_str();
									bind_param[1].buffer_length = (*it).size();

									if (mysql_stmt_bind_param(stmt, bind_param))
									{
										MessageBox(NULL, L"数据库绑定参数处理失败", L"QQ", MB_ICONERROR);
										mysql_stmt_close(stmt);
										mysql_close(conn_123);
										return;
									}

									if (mysql_stmt_execute(stmt))
									{
										MessageBox(NULL, L"数据库执行语句执行失败", L"QQ", MB_ICONERROR);
										mysql_stmt_close(stmt);
										mysql_close(conn_123);
										return;
									}

									it++;

								}

								mysql_stmt_close(stmt);
								mysql_close(conn_123);

								//并在friends表中增加新的朋友记录


								//删除half_friend表中的相关记录
								MYSQL* conn_1234 = mysql_init(NULL);
								int x_y_3 = 0;
								while (!mysql_real_connect(conn_1234, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))
								{
									x_y_3++;
									if (x_y_3 > 3)
									{
										MessageBox(NULL, L"连接不上数据库2", L"QQ", MB_ICONERROR);
										return;
									}
								}

								const char* sql_1234 = "INSERT INTO friends (account,friend_account)VALUES(?,?)";
								MYSQL_STMT* stmt_1234= mysql_stmt_init(conn_1234);

								if (!stmt_1234)
								{
									MessageBox(NULL, L"数据库预处理指针获取失败", L"QQ", MB_ICONERROR);
									mysql_close(conn_1234);
									return;
								}

								if (mysql_stmt_prepare(stmt_1234, sql_1234, strlen(sql_1234)))
								{
									MessageBox(NULL, L"数据库预处理语句执行失败", L"QQ", MB_ICONERROR);
									mysql_stmt_close(stmt_1234);
									mysql_close(conn_1234);
									return;
								}

								//添加朋友是双向的

								for (auto it = new_friends.begin(); it != new_friends.end();)
								{

									MYSQL_BIND bind_param[2];
									memset(bind_param, 0, sizeof(bind_param));

									bind_param[0].buffer_type = MYSQL_TYPE_STRING;
									bind_param[0].buffer = (void*)xx.c_str();
									bind_param[0].buffer_length = xx.size();

									bind_param[1].buffer_type = MYSQL_TYPE_STRING;
									bind_param[1].buffer = (void*)(*it).c_str();
									bind_param[1].buffer_length = (*it).size();

									if (mysql_stmt_bind_param(stmt_1234, bind_param))
									{
										MessageBox(NULL, L"数据库绑定参数失败2", L"QQ", MB_ICONERROR);
										mysql_stmt_close(stmt_1234);
										mysql_close(conn_1234);
										return;
									}

									if (mysql_stmt_execute(stmt_1234))
									{
										MessageBox(NULL, L"数据库执行语句执行失败2", L"QQ", MB_ICONERROR);
										mysql_stmt_close(stmt_1234);
										mysql_close(conn_1234);
										return;
									}

									it++;
								}

								//account 和 friend_account 字段内容交换
								for (auto it = new_friends.begin(); it != new_friends.end();)
								{
									MYSQL_BIND bind_param[2];
									memset(bind_param, 0, sizeof(bind_param));

									bind_param[1].buffer_type = MYSQL_TYPE_STRING;
									bind_param[1].buffer = (void*)xx.c_str();
									bind_param[1].buffer_length = xx.size();

									bind_param[0].buffer_type = MYSQL_TYPE_STRING;
									bind_param[0].buffer = (void*)(*it).c_str();
									bind_param[0].buffer_length = (*it).size();

									if (mysql_stmt_bind_param(stmt_1234, bind_param))
									{
										MessageBox(NULL, L"数据库绑定参数失败3", L"QQ", MB_ICONERROR);
										mysql_stmt_close(stmt_1234);
										mysql_close(conn_1234);
										return;
									}

									if (mysql_stmt_execute(stmt_1234))
									{
										MessageBox(NULL, L"数据库执行语句执行失败3", L"QQ", MB_ICONERROR);
										mysql_stmt_close(stmt_1234);
										mysql_close(conn_1234);
										return;
									}

									it++;
								}



								mysql_stmt_close(stmt_1234);
								mysql_close(conn_1234);


								std::string cc = "success";
								int cc_l = cc.size();
								send(client_server, (char*)&cc_l, sizeof(cc_l), 0);
								send(client_server, cc.c_str(), cc_l, 0);

								goto door_102;

							}
							else if (strcmp("拒绝",s_str_x.c_str()) == 0)
							{

								//接收新朋友数目
								int f_len = 0;
								recv(client_server, (char*)&f_len, sizeof(f_len), 0);

								char* buf_114 = new char[50];
								snprintf(buf_114, strlen(buf_114), "the f_len is %d\n", f_len);
								OutputDebugStringA(buf_114);
								delete[]buf_114;
								//接收客户端发来的同意的新朋友账号
								//用容器存储客户端发来的账号
								std::vector<std::string> new_friends;

								for (int i = 0; i < f_len; i++)
								{
									int acc_len = 0;
									recv(client_server, (char*)&acc_len, sizeof(acc_len), 0);
									std::string acc(acc_len, '\0');
									recv(client_server, &acc[0], acc_len, 0);

									char* buf_116 = new char[50];
									snprintf(buf_116, strlen(buf_116), "the acc is %s\n", acc.c_str());
									OutputDebugStringA(buf_116);
									delete[]buf_116;

									new_friends.push_back(acc);
								}

								std::string xx = new_online_user_account;
								if (!xx.empty() && xx.back() == '\0')
								{
									xx.pop_back();
								}

								//删除half_friend表中的相关记录
								MYSQL* conn_123 = mysql_init(NULL);
								int x_y = 0;
								while (!mysql_real_connect(conn_123, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))
								{
									x_y++;
									if (x_y > 3)
									{
										MessageBox(NULL, L"连接不上数据库，即将退出1", L"QQ", MB_ICONERROR);
										return;
									}
								}

								const char* sql_123 = "DELETE FROM half_friend WHERE friend_account=? AND account=?";
								MYSQL_STMT* stmt = mysql_stmt_init(conn_123);

								if (!stmt)
								{
									MessageBox(NULL, L"数据库预处理指针获取失败1", L"QQ", MB_ICONERROR);
									mysql_close(conn_123);
									return;
								}

								if (mysql_stmt_prepare(stmt, sql_123, strlen(sql_123)))
								{
									MessageBox(NULL, L"数据库预处理语句处理失败", L"QQ", MB_ICONERROR);
									mysql_stmt_close(stmt);
									mysql_close(conn_123);
									return;
								}

								for (auto it = new_friends.begin(); it != new_friends.end();)
								{

									MYSQL_BIND bind_param[2];
									memset(bind_param, 0, sizeof(bind_param));

									bind_param[0].buffer_type = MYSQL_TYPE_STRING;
									bind_param[0].buffer = (void*)xx.c_str();
									bind_param[0].buffer_length = xx.size();


									bind_param[1].buffer_type = MYSQL_TYPE_STRING;
									bind_param[1].buffer = (void*)(*it).c_str();
									bind_param[1].buffer_length = (*it).size();

									if (mysql_stmt_bind_param(stmt, bind_param))
									{
										MessageBox(NULL, L"数据库绑定参数处理失败", L"QQ", MB_ICONERROR);
										mysql_stmt_close(stmt);
										mysql_close(conn_123);
										return;
									}

									if (mysql_stmt_execute(stmt))
									{
										MessageBox(NULL, L"数据库执行语句执行失败", L"QQ", MB_ICONERROR);
										mysql_stmt_close(stmt);
										mysql_close(conn_123);
										return;
									}

									it++;

								}

								mysql_stmt_close(stmt);
								mysql_close(conn_123);


								std::string cc = "success";
								int cc_l=cc.size();
								send(client_server, (char*)&cc_l, sizeof(cc_l), 0);
								send(client_server, cc.c_str(), cc_l, 0);

								goto door_102;
							}

							else if (strcmp("取消", s_str_x.c_str()) == 0)
							{
								goto door_102;
							}

						}

					}

					else
					{
						std::string send_inf = "failure";
						int send_len = send_inf.size();
						send(client_server, (char*)&send_len, sizeof(send_len), 0);
						send(client_server, send_inf.c_str(), send_len, 0);

					}
				}


			}
		}
		
	}
	else if (wcscmp(wstr.c_str(),L"个人信息")==0)
	{
		send_personal_information_to_client(client_server,new_online_user_account);//加载用户原本的用户数据发给客户端

		int recv_utf8_len = 0;
		int x = recv(client_server, (char*)&recv_utf8_len, sizeof(recv_utf8_len), 0);
		if (x != sizeof(recv_utf8_len))
		{
			return;
		}
		std::string recv_utf8_str(recv_utf8_len, '\0');
		recv(client_server, &recv_utf8_str[0], recv_utf8_len, 0);
		int w_len = MultiByteToWideChar(CP_UTF8, 0, recv_utf8_str.c_str(), recv_utf8_len, NULL, 0);
		std::wstring w_str(w_len, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, recv_utf8_str.c_str(), recv_utf8_len, &w_str[0], w_len);
		if (!w_str.empty() && w_str.back() == L'\0')
		{
			w_str.pop_back();
		}
		if (wcscmp(w_str.c_str(), L"确定") == 0)
		{
			my_user_information new_infor;
			//接收除用户账号之外的用户信息，并更新数据库里的用户消息
			recv_all_information(client_server,new_infor.password);
			recv_all_information(client_server, new_infor.nickname);
			recv_all_information(client_server, new_infor.gender);
			recv_all_information(client_server, new_infor.age);
			recv_all_information(client_server, new_infor.signature);

			//依据用户账号，更新用户数据库信息
			MYSQL* conn_4 = mysql_init(NULL);
			std::string my_acc = new_online_user_account;
			if (!my_acc.empty()&&my_acc.back() == '\0')
			{
				my_acc.pop_back();
			}
			if (update_user_information((const std::string)my_acc, conn_4, new_infor))//成功更新用户信息
			{
				//通知广播框进行更新

				//更新用户列表的在线状态//
				{
					std::lock_guard<std::mutex> lk(users_mutex);//上锁
					users_update_signal = true;      // 设置条件
					users_cv.notify_one();           // 唤醒一个等待线程
				}

				MessageBox(NULL, L"服务器已经成功依据客户端发来的消息更新用户信息", L"QQ", MB_ICONINFORMATION);
				std::string inf = "update_success";
				int inf_len = inf.size();
				send(client_server,(char*)&inf_len,sizeof(inf_len),0);
				send(client_server,inf.c_str(),inf_len,0);
				mysql_close(conn_4);
			}
			else
			{
				MessageBox(NULL, L"服务器无法更新用户信息", L"QQ", MB_ICONERROR);
				mysql_close(conn_4);
				return;
			}
			//返回到登录界面首页
			std::thread(Handlelogin_pro, client_server, my_data,new_online_user_account).detach();
		}
		else if (wcscmp(w_str.c_str(), L"取消") == 0)
		{
			//返回到登录界面首页
			std::thread(Handlelogin_pro, client_server, my_data,new_online_user_account).detach();
		}
	}
	else if (wcscmp(wstr.c_str(),L"更换头像")==0)
	{
		int recv_utf8_len = 0;
		int x=recv(client_server, (char*)&recv_utf8_len, sizeof(recv_utf8_len), 0);
		if (x != sizeof(recv_utf8_len))
		{
			return;
		}
		std::string recv_utf8_str(recv_utf8_len,'\0');
		recv(client_server,&recv_utf8_str[0], recv_utf8_len, 0);
        int w_len =MultiByteToWideChar(CP_UTF8,0,recv_utf8_str.c_str(),recv_utf8_len,NULL,0);
		std::wstring w_str(w_len,L'\0');
		MultiByteToWideChar(CP_UTF8, 0, recv_utf8_str.c_str(), recv_utf8_len,&w_str[0], w_len);
		if (!w_str.empty()&&w_str.back()==L'\0')
		{
			w_str.pop_back();
		}
		if (wcscmp(w_str.c_str(), L"确定") == 0)
		{
			//用户账号
			std::string user_account_new = new_online_user_account;//含'\0'
			if (!new_online_user_account.empty()&&new_online_user_account.back() == '\0')
			{
				new_online_user_account.pop_back();
			}
		    //接收新的头像数据
			int my_new_img_len = 0;
			recv(client_server,(char*)&my_new_img_len,sizeof(my_new_img_len),0);
			BYTE* my_new_img = new BYTE[my_new_img_len];
			int r = 0;//单次接收量
			int sum = 0;//总接收量
			while (sum<my_new_img_len)
			{
				r = recv(client_server, (char*)my_new_img + sum, my_new_img_len - sum, 0);
				if (r > 0)
				{
					sum += r;
				}
				if (sum == my_new_img_len)
				{
					break;
				}
			}

            
			//recv(client_server,(char*)my_new_img,my_new_img_len,0);
			//依据用户账号查找MySQL数据库的用户头像并进行更新
			if (update_user_img(new_online_user_account, my_new_img, my_new_img_len, conn))
			{
				//服务器已经成功更新头像
				//通知服务器更新用户列表
				std::lock_guard<std::mutex> lk(users_mutex);//上锁
				users_update_signal = true;      // 设置条件
				users_cv.notify_one();           // 唤醒一个等待线程
			   //发送成功消息给客户端
				std::wstring img_update_success = L"服务器已经成功更新用户头像";
				int utf8_img_update_success = WideCharToMultiByte(CP_UTF8,0,img_update_success.c_str(),img_update_success.size()+1,NULL,0,NULL,NULL);
				std::string  utf8_img_update_success_str(utf8_img_update_success,'\0');
				WideCharToMultiByte(CP_UTF8, 0, img_update_success.c_str(), img_update_success.size() + 1, &utf8_img_update_success_str[0], utf8_img_update_success, NULL, NULL);
				send(client_server,(char*)&utf8_img_update_success,sizeof(utf8_img_update_success),0);
				send(client_server, utf8_img_update_success_str.c_str(), utf8_img_update_success, 0);

			}
			else
			{
				//发送失败消息给客户端
				std::wstring img_update_success = L"服务器无法更新用户头像";
				int utf8_img_update_success = WideCharToMultiByte(CP_UTF8, 0, img_update_success.c_str(), img_update_success.size() + 1, NULL, 0, NULL, NULL);
				std::string  utf8_img_update_success_str(utf8_img_update_success, '\0');
				WideCharToMultiByte(CP_UTF8, 0, img_update_success.c_str(), img_update_success.size() + 1, &utf8_img_update_success_str[0], utf8_img_update_success, NULL, NULL);
				send(client_server, (char*)&utf8_img_update_success, sizeof(utf8_img_update_success), 0);
				send(client_server, utf8_img_update_success_str.c_str(), utf8_img_update_success, 0);
			}
			//返回到登录界面首页
			std::thread(Handlelogin_pro, client_server, my_data,new_online_user_account).detach();
		}
		else if (wcscmp(w_str.c_str(), L"取消") == 0)
		{
			//返回到登录界面首页
            std::thread(Handlelogin_pro, client_server, my_data,new_online_user_account).detach();
		}
		else 
		{
			return;
		}
	}
	else if (wcscmp(wstr.c_str(),L"返回")==0)
	{
		//找到退出登录的用户账号//
		std::string del_account = my_data.account;//my_data不含'\0'终止符//
		    //将该用户从在线用户列表中删除//
			std::lock_guard<std::mutex>lock(g_onlineUsersMutex);
			//返回符合条件的account的位置,并将account移动到末尾//
			auto it = std::remove_if(g_onlineUsers.begin(), g_onlineUsers.end(), [&del_account](const User_account& user)
				{
					if (user.account.back() == '\0')//如果account末尾有'\0'终止符//
					{
						std::string x = user.account;
						x.pop_back();//去除'\0'终止符//
					    return x == del_account;
				    }
				});
			//删除it到end()之间的元素，包括it，不包括end()//
			if (it != g_onlineUsers.end()) 
			{
				g_onlineUsers.erase(it, g_onlineUsers.end());
			}

			//更新用户列表的在线状态//
			{
				std::lock_guard<std::mutex> lk(users_mutex);
				users_update_signal = true;      // 设置条件
				users_cv.notify_one();           // 唤醒一个等待线程
			}

			//刷新广播框
			std::thread(LoadUsersFromDB, conn).detach();//广播框线程//

			//返回登录和注册判断选择框
			//判断具体执行哪种线程,登录或注册//
			int recv_len = 0;
			recv(client_server, (char*)&recv_len, sizeof(recv_len), 0);//接收消息长度//
			if (recv_len <= 0)
			{
				//MessageBox(NULL, L"接收线程类型的消息长度失败", L"QQ", MB_ICONERROR);
				return ;
			}
			//MessageBox(NULL, L"接收线程类型的消息长度成功", L"QQ", MB_ICONINFORMATION);
			std::string recvchar(recv_len, 0);
			int r = 0;
			r = recv(client_server, &recvchar[0], recv_len, 0);
			{
				if (r <= 0)
				{
					//MessageBox(NULL, L"接收线程类型的消息失败", L"QQ", MB_ICONERROR);
					return ;
				}
			}
			//MessageBox(NULL, L"接收线程类型的消息成功", L"QQ", MB_ICONINFORMATION);
			int wlen = MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, NULL, 0);//转换为宽字节所需要的字节数//
			std::wstring wrecvchar(wlen, 0);//分配空间//
			MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, &wrecvchar[0], wlen);//实际转换//
			//注册处理//
			if (wcscmp(wrecvchar.c_str(), L"注册") == 0)//注册线程//
			{
				//MessageBox(NULL, L"接收到注册消息，即将启用注册线程", L"QQ", MB_ICONINFORMATION);
				std::thread(HandleClient_register, client_server).detach();//创建注册线程//
			}
			//登录处理//
			else if (wcscmp(wrecvchar.c_str(), L"登录") == 0)//登录线程//
			{
				//MessageBox(NULL, L"接收到登录消息，即将启用登录线程", L"QQ", MB_ICONINFORMATION);
				std::thread(HandleClient_login, client_server,new_online_user_account).detach();//创建登录线程//
			}
			else if (wcscmp(wrecvchar.c_str(), L"退出") == 0)//用户退出客户端程序//
			{
				//MessageBox(NULL, L"接收不到线程消息，即将退出", L"QQ", MB_ICONINFORMATION);
				return ;
			}

	}
	else if (wcscmp(wstr.c_str(),L"退出")==0)
	{
		//找到退出登录的用户账号//
		std::string del_account = my_data.account;//my_data不含'\0'终止符//
		//将该用户从在线用户列表中删除//
		std::lock_guard<std::mutex>lock(g_onlineUsersMutex);
		//返回符合条件的account的位置,并将account移动到末尾//
		auto it = std::remove_if(g_onlineUsers.begin(), g_onlineUsers.end(), [&del_account](const User_account& user)
			{
				if (user.account.back() == '\0')//如果account末尾有'\0'终止符//
				{
					std::string x = user.account;
					x.pop_back();//去除'\0'终止符//
					return x == del_account;
				}
			});
		//删除it到end()之间的元素，包括it，不包括end()//
		if (it != g_onlineUsers.end())
		{
			g_onlineUsers.erase(it, g_onlineUsers.end());
		}
		//更新用户列表的在线状态//
		{
			std::lock_guard<std::mutex> lk(users_mutex);
			users_update_signal = true;      // 设置条件
			users_cv.notify_one();           // 唤醒一个等待线程
		}
		return;//退出该线程//
	}
	else if (wcscmp(wstr.c_str(), L"注销账号") == 0)
	{
		int len_utf8 = 0;
		recvAll(client_server, (char*)&len_utf8, sizeof(len_utf8), 0);//先接收长度//
		//MessageBox(NULL, L"接收注销账号界面客户端发送的长度消息成功", L"QQ", MB_ICONINFORMATION);
		if (len_utf8 <= 0)
		{
			MessageBox(NULL, L"接收注销账号界面按钮信息长度失败", L"QQ", MB_ICONERROR);
			return;
		}
		std::string str_utf8(len_utf8, 0);//分配空间//
		recvAll(client_server, &str_utf8[0], len_utf8, 0);//接收内容//
		//MessageBox(NULL, L"接收登录界面客户端发送的按钮内容消息成功", L"QQ", MB_ICONINFORMATION);
		int wchar_len = MultiByteToWideChar(CP_UTF8, 0, str_utf8.c_str(), len_utf8, NULL, 0);//计算出转换长度//
		std::wstring wchar_str(wchar_len, 0);//分配空间//
		MultiByteToWideChar(CP_UTF8, 0, str_utf8.c_str(), len_utf8, &wchar_str[0], wchar_len);//实际转换//
		//MessageBox(NULL,wchar_str.c_str(), L"QQ", MB_ICONINFORMATION);

		if (wcscmp(wchar_str.c_str(),L"确定") == 0)//用户确认注销//
		{
			if (delete_user_by_account_password((const char*)my_data.account.c_str(),(const char*)my_data.password.c_str(), conn))
			{
				//更新用户列表的在线状态//
				
					//std::lock_guard<std::mutex> lk(users_mutex);
					//users_update_signal = true;      // 设置条件
					//users_cv.notify_one();           // 唤醒一个等待线程

				{
					std::vector<Userdata>temp_users;
					temp_users.clear();
					const char* sql = "SELECT nickname,imgData,account FROM users";//SQL命令语句//
					if (mysql_query(conn, sql))//如果没有符合的查询结果，返回非零值//
					{
						//MessageBox(NULL, L"没有符合的昵称和头像", L"QQ", MB_ICONERROR);
						return;
					}

					MYSQL_RES* res = mysql_store_result(conn);//一个结果集//
					if (!res)
					{
						return;
					}
					MYSQL_ROW  row;//指向字符串数组的指针//
					unsigned long* lengths;
					while ((row = mysql_fetch_row(res)))//取某一行查询结果集//
					{
						lengths = mysql_fetch_lengths(res);//取某一行查询结果长度集//
						Userdata user;
						if (row[0])
						{
							user.nickname = row[0];
						}
						if (row[1] && lengths[1] > 0)
						{
							user.imgData.assign((BYTE*)row[1], (BYTE*)row[1] + lengths[1]);
						}
						if (row[2])
						{
							user.account = row[2];
						}
						temp_users.push_back(std::move(user));//将填充好的用户对象移动到全局用户列表里，防止不必要的拷贝//
					}
					mysql_free_result(res);
					std::lock_guard<std::mutex>lock(g_usersMutex);//加锁//
					g_users.swap(temp_users);

					if (g_hInfoDialogbroadcast && IsWindow(g_hInfoDialogbroadcast))//等待句柄有效//
						PostMessage(g_hInfoDialogbroadcast, WM_APP_UPDATEBROADCAST_MSG, 0, 0); // 自定义消息
				}

				//std::lock_guard<std::mutex>lk(users_mutex);
                //users_update_signal =true;//更新用户列表标志//
				//users_cv.notify_one();//通知函数执行更新用户列表//

				//返回登录注册选择处理线程//
				//判断具体执行哪种线程,登录或注册//
				int recv_len = 0;
				recv(client_server, (char*)&recv_len, sizeof(recv_len), 0);//接收消息长度//
				if (recv_len <= 0)
				{
					//MessageBox(NULL, L"接收线程类型的消息长度失败", L"QQ", MB_ICONERROR);
					return ;
				}
				//MessageBox(NULL, L"接收线程类型的消息长度成功", L"QQ", MB_ICONINFORMATION);
				std::string recvchar(recv_len, 0);
				int r = 0;
				r = recv(client_server, &recvchar[0], recv_len, 0);
				{
					if (r <= 0)
					{
						//MessageBox(NULL, L"接收线程类型的消息失败", L"QQ", MB_ICONERROR);
						return ;
					}
				}
				//MessageBox(NULL, L"接收线程类型的消息成功", L"QQ", MB_ICONINFORMATION);
				int wlen = MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, NULL, 0);//转换为宽字节所需要的字节数//
				std::wstring wrecvchar(wlen, 0);//分配空间//
				MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, &wrecvchar[0], wlen);//实际转换//
				//注册处理//
				if (wcscmp(wrecvchar.c_str(), L"注册") == 0)//注册线程//
				{
					//MessageBox(NULL, L"接收到注册消息，即将启用注册线程", L"QQ", MB_ICONINFORMATION);
					std::thread(HandleClient_register, client_server).detach();//创建注册线程//
				}
				//登录处理//
				else if (wcscmp(wrecvchar.c_str(), L"登录") == 0)//登录线程//
				{
					//MessageBox(NULL, L"接收到登录消息，即将启用登录线程", L"QQ", MB_ICONINFORMATION);
					std::thread(HandleClient_login, client_server,new_online_user_account).detach();//创建登录线程//
				}
				else if(wcscmp(wrecvchar.c_str(), L"退出") == 0)//如果用户退出客户端程序//
				{
					//MessageBox(NULL, L"接收不到线程消息，即将退出", L"QQ", MB_ICONINFORMATION);
					
					return ;
				}

			}
			else
			{
				MessageBox(NULL, L"没成功注销该用户", L"QQ", MB_ICONERROR);
				return;
			}
		}
		else if (wcscmp(wchar_str.c_str(),L"取消") == 0)//用户取消了注销//
		{
			std::thread(Handlelogin_pro,client_server,my_data,new_online_user_account).detach();//返回登陆界面处理线程//
		}

	}
}


DWORD WINAPI StartServer(LPVOID lpParam)//服务器线程函数//
{
	SOCKET server_socket = INVALID_SOCKET;
	server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//创建套接字//
	if (server_socket == INVALID_SOCKET)
	{
		MessageBox(NULL, L"服务器套接字创建失败", L"QQ", MB_ICONERROR);
		return 1;
	}
	else
	{
		MessageBox(NULL, L"服务器套接字创建成功", L"QQ", MB_ICONINFORMATION);
	}
	//设置服务器地址//
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;//设置为IPv4//
	serverAddr.sin_port = htons(8080);//端口号//
	serverAddr.sin_addr.s_addr = INADDR_ANY;//监听所有网卡，即本机的所有IP地址//
	bind(server_socket, (sockaddr*)&serverAddr, sizeof(serverAddr));//将套接字与指定的IP地址和端口号关联//
	listen(server_socket, 50);//开始监听//
	int y = 0;

	std::thread(LoadUsersFromDB,conn).detach();//广播框线程//

	while (true && y <= 3)
	{
		SOCKET client_server = accept(server_socket, NULL, NULL);//创建新的套接字//
		if (client_server == INVALID_SOCKET)
		{
			//MessageBox(NULL, L"无法创建新的套接字与客户端进行通信", L"QQ", MB_ICONINFORMATION);
			y++;
			continue;
		}
		
		//MessageBox(NULL, L"成功创建新的套接字与客户端进行通信", L"QQ", MB_ICONINFORMATION);
		//判断具体执行哪种线程,登录或注册//
		int recv_len = 0;
		recv(client_server, (char*)&recv_len, sizeof(recv_len), 0);//接收消息长度//
		if (recv_len <= 0)
		{
			//MessageBox(NULL, L"接收线程类型的消息长度失败", L"QQ", MB_ICONERROR);
			return 1;
		}
		//MessageBox(NULL, L"接收线程类型的消息长度成功", L"QQ", MB_ICONINFORMATION);
		std::string recvchar(recv_len, 0);
		int r = 0;
		r = recv(client_server, &recvchar[0], recv_len, 0);
		{
			if (r <= 0)
			{
				//MessageBox(NULL, L"接收线程类型的消息失败", L"QQ", MB_ICONERROR);
				return 1;
			}
		}
		//MessageBox(NULL, L"接收线程类型的消息成功", L"QQ", MB_ICONINFORMATION);
		int wlen = MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, NULL, 0);//转换为宽字节所需要的字节数//
		std::wstring wrecvchar(wlen, 0);//分配空间//
		MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, &wrecvchar[0], wlen);//实际转换//
		//注册处理//
		if (wcscmp(wrecvchar.c_str(), L"注册") == 0)//注册线程//
		{
			//MessageBox(NULL, L"接收到注册消息，即将启用注册线程", L"QQ", MB_ICONINFORMATION);
			std::thread(HandleClient_register, client_server).detach();//创建注册线程//
		}
		//登录处理//
		else if (wcscmp(wrecvchar.c_str(), L"登录") == 0)//登录线程//
		{
			//MessageBox(NULL, L"接收到登录消息，即将启用登录线程", L"QQ", MB_ICONINFORMATION);
			std::string new_online_user_account;
			std::thread(HandleClient_login, client_server,new_online_user_account).detach();//创建登录线程//
		}
		else if(wcscmp(wrecvchar.c_str(), L"退出") == 0)//用户退出客户端程序//
		{
			//MessageBox(NULL, L"接收不到线程消息，即将退出", L"QQ", MB_ICONINFORMATION);
			return 1;
		}
	}
	return 1;
}


int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE, LPSTR, int nCmdShow)
{
	int con_num = 0;
	while (!mysql_real_connect(conn, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))//连接数据库//
	{
		MessageBox(NULL, L"连接数据库失败", L"QQ", MB_ICONERROR);
		con_num++;
		if (con_num > 3)
		{
			return 1;
		}
	}
	MessageBox(NULL, L"成功连接数据库", L"QQ", MB_ICONINFORMATION);

	const char* create_table = "CREATE TABLE IF NOT EXISTS users("
		"id INT PRIMARY KEY AUTO_INCREMENT,"
		"account VARCHAR(64),"
		"password VARCHAR(64),"
		"nickname VARCHAR(64),"
		"gender VARCHAR(8),"
		"age VARCHAR(8),"
		"signature VARCHAR(256),"
		"imgData LONGBLOB"
		");";

	mysql_query(conn, create_table);

	int con_num_2 = 0;
	MYSQL* conn_7 =mysql_init(NULL);
	while (!mysql_real_connect(conn_7, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))//连接数据库//
	{
		MessageBox(NULL, L"连接数据库失败", L"QQ", MB_ICONERROR);
		con_num_2++;
		if (con_num_2 > 3)
		{
			return 1;
		}
	}
	MessageBox(NULL, L"成功连接数据库", L"QQ", MB_ICONINFORMATION);

	//建表，表名为users//
	const char* create_table_2 = "CREATE TABLE IF NOT EXISTS friends("
		"id INT PRIMARY KEY AUTO_INCREMENT,"
		"account VARCHAR(64),"
		"friend_account VARCHAR(64)"
		");";
	mysql_query(conn_7, create_table_2);
	mysql_close(conn_7);


	int conn_33_count = 0;
	MYSQL* conn_33 = mysql_init(NULL);
	while (!mysql_real_connect(conn_33, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))//连接数据库//
	{
		MessageBox(NULL, L"连接数据库失败", L"QQ", MB_ICONERROR);
		conn_33_count++;
		if (conn_33_count > 3)
		{
			return 1;
		}
	}
	MessageBox(NULL, L"成功连接数据库", L"QQ", MB_ICONINFORMATION);

	//建表，表名为users//
	const char* create_table_3 = "CREATE TABLE IF NOT EXISTS half_friend("
		"id INT PRIMARY KEY AUTO_INCREMENT,"
		"account VARCHAR(64),"
		"friend_account VARCHAR(64)"
		");";
	mysql_query(conn_33, create_table_3);
	mysql_close(conn_33);


	ULONG_PTR gdiToken = 0;//初始化GDI+//
	if (!gdiToken)
	{
		Gdiplus::GdiplusStartupInput gdiplusStarupInput;
		Gdiplus::GdiplusStartup(&gdiToken, &gdiplusStarupInput, NULL);
	}
	InitializeCriticalSection(&g_csMsgQueue);//初始化//
	InitializeCriticalSection(&g_csMsg_two_Queue);//初始化//

	INT_PTR result_one = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_ONE), NULL, Dialog_one);//开始框//
	if (result_one == IDOK)//一级判断//
	{
		WSADATA wsaData;
		int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (result)
		{
			MessageBox(NULL, L"Winsock初始化失败", L"QQ", MB_ICONERROR);
			WSACleanup();
			return 1;
		}
		if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
		{
			MessageBox(NULL, L"不支持Winsock2.2", L"QQ", MB_ICONERROR);
			WSACleanup();
			return 1;
		}
		MessageBox(NULL, L"Winsock初始化成功", L"QQ", MB_ICONINFORMATION);
		//服务器监控窗口//
		if (g_hInfoDialog == NULL)//创建消息对话框//
		{
			g_hInfoDialog = CreateDialog(
				GetModuleHandle(NULL),
				MAKEINTRESOURCE(IDD_MYDIALOG_INFORMATION),
				NULL,
				Dialog_information
			);
			ShowWindow(g_hInfoDialog, SW_SHOW);
		}
		if (g_hInfoDialogphoto == NULL)//创建头像显示框//
		{
			g_hInfoDialogphoto = CreateDialog(
				GetModuleHandle(NULL),
				MAKEINTRESOURCE(IDD_MYDIALOG_PHOTO),
				NULL,
				Dialog_photo
			);
			ShowWindow(g_hInfoDialogphoto, SW_SHOW);
		}
		if (g_hInfoDialogmonitor== NULL)//创建监控框//
		{
			g_hInfoDialogmonitor = CreateDialog(
				GetModuleHandle(NULL),
				MAKEINTRESOURCE(IDD_MYDIALOG_MONITOR),
				NULL,
				Dialog_monitor
			);
			ShowWindow(g_hInfoDialogmonitor, SW_SHOW);
		}
		if (g_hInfoDialogbroadcast == NULL)//广播框//
		{
			g_hInfoDialogbroadcast = CreateDialog(
				GetModuleHandle(NULL),
				MAKEINTRESOURCE(IDD_MYDIALOG_BROADCAST),
				NULL,
				Dialog_broadcast
			);
			ShowWindow(g_hInfoDialogbroadcast, SW_SHOW);
		}

		hThread = CreateThread(NULL, 0, StartServer, NULL, 0, NULL);//线程函数//
	}
	else if (result_one == IDEND)//一级判断//
	{
		g_exitFlag =true;
		DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_END), NULL, Dialog_end);//结束框//
	}
	// 主消息循环
	MSG msg;
	
	while (GetMessage(&msg, NULL, 0, 0)&&(g_exitFlag==false))
	{
		if (!IsDialogMessage(g_hInfoDialog, &msg)||
			!IsDialogMessage(g_hInfoDialogphoto, &msg)||
			!IsDialogMessage(g_hInfoDialogmonitor, &msg)||
			!IsDialogMessage(g_hInfoDialogbroadcast, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

	}
	WSACleanup();
	CloseHandle(hThread);//释放总线程句柄//
	DeleteCriticalSection(&g_csMsgQueue);
	DeleteCriticalSection(&g_csMsg_two_Queue);
	return 0;
}
