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
#pragma comment (lib,"libmysql.lib")
#include"resource.h"
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"gdiplus.lib")

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
void HandleClient_login(SOCKET client_server);//登录处理线程//
void HandleClient_passwordset(SOCKET client_server);
void HandleClient_photoset(SOCKET client_server);
void Handlelogin_pro(SOCKET client_server, receivedData my_data);
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
std::vector<Userdata> g_users;

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
		TimeID = SetTimer(hwndDlg, 1, 4000, NULL);//显示第一个对话框的内容，延迟3秒//
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
			EndDialog(hwndDlg, IDOK);//关闭对话框//
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
			return (INT_PTR)TRUE;
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

		return (INT_PTR)TRUE;
	}
	case  WM_APP_UPDATEBROADCAST_MSG:          //更新用户列表消息//
	{
		HWND hList = GetDlgItem(hwndDlg, IDC_USERS);//获取用户列表句柄
		SendMessage(hList, LB_RESETCONTENT, 0, 0);//清空文本框//
		std::lock_guard<std::mutex> lock(g_usersMutex);
		for (int i = 0; i < g_users.size(); ++i)
		{
			SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)L"");
			SendMessage(hList, LB_SETITEMDATA, i, (LPARAM)i);
		}
		return (INT_PTR)TRUE;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
		{
			DestroyWindow(hwndDlg);
			g_hInfoDialogbroadcast = NULL;
			return (INT_PTR)TRUE;
		}
		case IDOK:
		{
			return (INT_PTR)TRUE;
		}
		}
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
		if (dis->CtlID == IDC_USERS && dis->itemID < g_users.size())
		{
			std::lock_guard<std::mutex> lock(g_usersMutex);
			HDC hdc = dis->hDC;//绘图设备环境//
			RECT rc = dis->rcItem;//需绘制的项的矩形区域//
			size_t idx = dis->itemData;
			const Userdata& user = g_users[idx];

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

			/*
			//画昵称//
			RECT rcText = rc;
			rcText.left += imgsize + 8;
			rcText.top += 12;
			int wlen = MultiByteToWideChar(CP_UTF8,0,user.nickname.c_str(),user.nickname.size()+1,NULL,0);
			std::wstring wnick(wlen, 0);
			MultiByteToWideChar(CP_UTF8, 0, user.nickname.c_str(), user.nickname.size() + 1,&wnick[0], wlen);//将文本转化为宽字符串//
			//DrawTextW(hdc,wnick.c_str(),wlen,&rcText,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
			DrawTextW(hdc, wnick.c_str(), wlen, &rcText, DT_LEFT | DT_TOP|DT_SINGLELINE);

			
			// 计算昵称高度
			DrawTextW(hdc, wnick.c_str(), wlen, &rcText, DT_LEFT | DT_TOP|DT_SINGLELINE| DT_CALCRECT);

			//画账号//
			RECT rcAccount = rcText;
			rcAccount.top =rcText.bottom+8;
			rcAccount.bottom = rcAccount.top + (rcText.bottom - rcText.top); //更新rcAccount.bottom//
			int wlen2 = MultiByteToWideChar(CP_UTF8, 0, user.nickname.c_str(), user.nickname.size() + 1, NULL, 0);
			std::wstring wnick_2(wlen2, 0);
			MultiByteToWideChar(CP_UTF8, 0, user.nickname.c_str(), user.nickname.size() + 1, &wnick_2[0], wlen2);//将文本转化为宽字符串//
			DrawTextW(hdc, wnick_2.c_str(), wlen2, &rcAccount, DT_LEFT | DT_TOP|DT_SINGLELINE);
			*/

			//画账号//
			RECT rcAccount = rc;
			rcAccount.left += imgsize + 8;
			rcAccount.top += 12;
			int wlen = MultiByteToWideChar(CP_UTF8, 0, user.account.c_str(), user.account.size() + 1, NULL, 0);
			std::wstring wnick(wlen, 0);
			MultiByteToWideChar(CP_UTF8, 0, user.account.c_str(), user.account.size() + 1, &wnick[0], wlen);//将文本转化为宽字符串//
			//DrawTextW(hdc,wnick.c_str(),wlen,&rcText,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
			DrawTextW(hdc, wnick.c_str(), wlen, &rcAccount, DT_LEFT | DT_TOP | DT_SINGLELINE);


			// 计算账号高度
			DrawTextW(hdc, wnick.c_str(), wlen, &rcAccount, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_CALCRECT);

			//画昵称//
			RECT rcText = rcAccount;
			rcText.top = rcAccount.bottom + 8;
			rcText.bottom = rcText.top + (rcAccount.bottom - rcAccount.top); //更新rcText.bottom//
			int wlen2 = MultiByteToWideChar(CP_UTF8, 0, user.nickname.c_str(), user.nickname.size() + 1, NULL, 0);
			std::wstring wnick_2(wlen2, 0);
			MultiByteToWideChar(CP_UTF8, 0, user.nickname.c_str(), user.nickname.size() + 1, &wnick_2[0], wlen2);//将文本转化为宽字符串//
			DrawTextW(hdc, wnick_2.c_str(), wlen2, &rcText, DT_LEFT | DT_TOP | DT_SINGLELINE);


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


void HandleClient_register(SOCKET client_server)//注册处理线程//
{
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
			std::thread(HandleClient_login, client_server).detach();//创建登录线程//
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
						std::lock_guard<std::mutex>lk(users_mutex);
						users_update_signal = true;
						users_cv.notify_one();//通知函数重新加载数据库里的用户昵称和头像//

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
							std::thread(HandleClient_login, client_server).detach();//创建登录线程//
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
					std::lock_guard<std::mutex>lk(users_mutex);
					users_update_signal = true;
					users_cv.notify_one();//通知函数重新加载数据库里的用户昵称和头像//

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
						std::thread(HandleClient_login, client_server).detach();//创建登录线程//
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
				std::lock_guard<std::mutex>lk(users_mutex);
				users_update_signal = true;
				users_cv.notify_one();//通知函数重新加载数据库里的用户昵称和头像//

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
					std::thread(HandleClient_login, client_server).detach();//创建登录线程//
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
			std::lock_guard<std::mutex>lk(users_mutex);
			users_update_signal = true;
			users_cv.notify_one();//通知函数重新加载数据库里的用户昵称和头像//

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
				std::thread(HandleClient_login, client_server).detach();//创建登录线程//
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

















//用户昵称和头像查询语句//
void LoadUsersFromDB(MYSQL* conn)
{
	while (true)
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











void HandleClient_login(SOCKET client_server)//登录处理线程//
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
			MessageBox(NULL, L"查询到该账号密码已经注册", L"QQ", MB_ICONINFORMATION);
			std::wstring recv_back = L"sucess";
			int utf8_len = WideCharToMultiByte(CP_UTF8, 0, recv_back.c_str(), recv_back.size() + 1, NULL, 0, NULL, NULL);//转换为utf8所需的空间//
			std::string utf8str(utf8_len, 0);//分配空间//
			WideCharToMultiByte(CP_UTF8, 0, recv_back.c_str(), recv_back.size() + 1, &utf8str[0], utf8_len, NULL, NULL);//实际转换//
			send(client_server, (char*)&utf8_len, sizeof(utf8_len), 0);//先发长度//
			send(client_server, utf8str.c_str(), utf8_len, 0);//后发内容//
			//进入登录主页面之后的线程//
			MessageBox(NULL, L"即将进入登录界面的处理线程", L"QQ", MB_ICONINFORMATION);
			std::thread(Handlelogin_pro,client_server,my_data).detach();
			





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
			std::thread(HandleClient_login, client_server).detach();//登录处理线程//
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
			std::thread(HandleClient_login, client_server).detach();//创建登录线程//
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


//登录界面处理线程//
void Handlelogin_pro(SOCKET client_server,receivedData my_data)//登录界面处理线程//
{
	MessageBox(NULL, L"成功进入登录界面的处理线程", L"QQ", MB_ICONINFORMATION);

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
			if (len_utf8<= 0)
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
				
				std::wstring msg_one=L"[";
				msg_one+=timeBuffer_one;
				msg_one+= L"][";
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
				int u_len = WideCharToMultiByte(CP_UTF8,0,msg_one.c_str(),msg_one.size()+1, NULL, 0, NULL, NULL);//计算转换长度//
				//std::wstring num = std::to_wstring(u_len);//将数字转化为宽字符串//
				//MessageBox(NULL,num.c_str(), L"QQ", MB_ICONINFORMATION);
				std::string u_str(u_len, 0);//分配空间//
				WideCharToMultiByte(CP_UTF8, 0, msg_one.c_str(), msg_one.size() + 1, &u_str[0], u_len, NULL, NULL);//实际转换//
				
				send(client_server,(char*)&u_len,sizeof(u_len),0);//先发长度//
				send(client_server, u_str.c_str(), u_len, 0);//后发内容//

				EnterCriticalSection(&g_csMsg_two_Queue);//加锁//
				g_msg_two_Queue.push(msg_one);//创建副本//
				LeaveCriticalSection(&g_csMsg_two_Queue);//解锁//
				//MessageBox(NULL, L"已经成功将信息拼接", L"QQ", MB_ICONINFORMATION);
				// 记录到服务器监控室//
				PostMessage(g_hInfoDialogmonitor, WM_APP_UPDATEMONITOR_MSG, 0, 0);
				//MessageBox(NULL, L"已经成功通知信息的监控显示", L"QQ", MB_ICONINFORMATION);
				
			}
		}
		
		std::thread(Handlelogin_pro, client_server, my_data).detach();//返回登录首界面处理线程//
	}
	else if (wcscmp(wstr.c_str(),L"聊天室")==0)
	{

	}
	else if (wcscmp(wstr.c_str(),L"好友")==0)
	{

	}
	else if (wcscmp(wstr.c_str(),L"个人信息")==0)
	{

	}
	else if (wcscmp(wstr.c_str(),L"更换头像")==0)
	{

	}
	else if (wcscmp(wstr.c_str(),L"返回")==0)
	{

	}
	else if (wcscmp(wstr.c_str(),L"退出")==0)
	{

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
				//返回登录注册选择处理线程//
				//判断具体执行哪种线程,登录或注册//
				std::lock_guard<std::mutex>lk(users_mutex);
                users_update_signal =true;//更新用户列表标志//
				users_cv.notify_one();//通知函数执行更新用户列表//

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
					std::thread(HandleClient_login, client_server).detach();//创建登录线程//
				}
				else
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
			std::thread(Handlelogin_pro,client_server,my_data).detach();//返回登陆界面处理线程//
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
			std::thread(HandleClient_login, client_server).detach();//创建登录线程//
		}
		else
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
	//建表，表名为users//
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
		DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_END), NULL, Dialog_end);//结束框//
	}
	// 主消息循环
	MSG msg;
	
	while (GetMessage(&msg, NULL, 0, 0))
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
