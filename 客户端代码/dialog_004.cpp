#include <winsock2.h>
#include <ws2tcpip.h> 
#include<windows.h>
#include<commCtrl.h>
#include"resource.h"
#include <wincrypt.h>
#include<atlstr.h>
#include<string>
#include <gdiplus.h>
#include <algorithm>
#include <vector>
#include <mutex> 
#include<ctime>
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib,"ws2_32.lib")


//存储用户的头像数据
std::vector<BYTE> user_image_data;
std::string user_nickname_data;

//好友聊天框的好友昵称和头像
std::string account_recver;
std::string nickname_recver;
std::vector<BYTE> image_recver;

//存储新登录用户账号
std::string new_online_user_account;//含'\0'
std::wstring w_new_online_user_account;

//更改用户个人信息时存储新登录用户的个人信息
struct w_new_user_information
{
	std::wstring password;//limit 32
	std::wstring nickname;//limit 32
	std::wstring gender;//limit 男 or 女
	std::wstring age;//limit 8
	std::wstring signature;//limit 128
};


//登录时加载新登录用户的个人信息
struct my_user_information
{
	std::string account;
	std::string password;//limit 32
	std::string nickname;//limit 32
	std::string gender;//limit 男 or 女
	std::string age;//limit 8
	std::string signature;//limit 128
} old_my_user_information,xxy;


//登录时加载新登录用户的个人信息
struct w_my_user_information
{
	std::wstring password;//limit 32
	std::wstring nickname;//limit 32
	std::wstring gender;//limit 男 or 女
	std::wstring age;//limit 8
	std::wstring signature;//limit 128
} w_old_my_user_information;



// 全局变量
SOCKET g_clientSocket = INVALID_SOCKET; // 全局套接字，保持连接
bool g_isConnected = false; // 连接状态标志
WCHAR g_szAccount[32] = { 0 }; // 账号
WCHAR g_szPassword[32] = { 0 }; // 密码
WCHAR g_szNickname[32] = { 0 }; // 昵称
WCHAR g_szGender[16] = { 0 }; // 性别
WCHAR g_szAge[8] = { 0 }; // 年龄
WCHAR g_szSignature[128] = { 0 }; // 个性签名
BYTE* g_pImageData = NULL;//储存头像的二进制数据//
DWORD g_dwImageSize = 0;//数据大小//
int passwd_back_to_account = 0;//账号是否已产生，防止密码设置时，按下取消键，回到账号生成框后，重新生成账号//
INT_PTR CALLBACK Dialog_one(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//初始化对话框//
{
	static UINT_PTR TimeID = 0;//启用定时器//
	static HBRUSH hBackgroundBrush = NULL;//背景画刷//
	static HBITMAP hBmp = NULL;//位图句柄//
	switch (uMsg)
	{
	case WM_INITDIALOG:
		//SetDlgItemText(hwndDlg, IDC_EDIT3, L"Welcome to QQ");//第一个对话框的内容//
		TimeID = SetTimer(hwndDlg, 1, 4000, NULL);//显示第一个对话框的内容，延迟3秒//
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
			DrawText(pdis->hDC, L"Welcome to QQ", -1, &pdis->rcItem,//对话框标题//
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
		EndDialog(hwndDlg, IDCANCEL);//关闭对话框//
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
		exit(0);
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK Dialog_two(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//登录和注册选择框//
{
	passwd_back_to_account = 0;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDLOGIN:
			EndDialog(hwndDlg, IDLOGIN);
			return (INT_PTR)TRUE;
		case IDREGISTER:
			EndDialog(hwndDlg, IDREGISTER);
			return (INT_PTR)TRUE;
		}break;
	case WM_CLOSE:
		EndDialog(hwndDlg, IDCANCEL);
		return (INT_PTR)TRUE;
	}
	return (INT_PTR)FALSE;
}

//将登录框中的账号密码发出//
void my_send_one(SOCKET socket, WCHAR* wstr)
{
	int wlen = wcslen(wstr);//获取宽字符串的字符数,不含'\0'//
	int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wstr, wlen + 1, NULL, 0, NULL, NULL);//获取转换后的utf8长度，包含'\0'//
	std::string utf8str(utf8_len, 0);//分配空间//
	WideCharToMultiByte(CP_UTF8, 0, wstr, wlen + 1, &utf8str[0], utf8_len, NULL, NULL);//实际转换//
	send(socket, (char*)&utf8_len, sizeof(utf8_len), 0);//先发长度//
	send(socket, utf8str.c_str(), utf8_len, 0);//再发内容//
}


INT_PTR CALLBACK Dialog_three(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//登录框//
{
	static bool EDIT1 = FALSE;
	static bool EDIT2 = FALSE;
	HWND hEdit = NULL;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, IDC_EDIT3, L"账号");
		SetDlgItemText(hwndDlg, IDC_EDIT4, L"密码");
		SetDlgItemText(hwndDlg, IDC_EDIT1, L"账号");
		SetDlgItemText(hwndDlg, IDC_EDIT2, L"密码");
		SendMessage(GetDlgItem(hwndDlg, IDC_EDIT1), WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
		SendMessage(GetDlgItem(hwndDlg, IDC_EDIT2), WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
		return (INT_PTR)TRUE;
	case WM_CTLCOLOREDIT://设置水印文字//
		hEdit = (HWND)lParam;
		if (!EDIT1 && hEdit == GetDlgItem(hwndDlg, IDC_EDIT1))
		{
			SetTextColor((HDC)wParam, RGB(200, 200, 200));
			return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
		}
		if (!EDIT2 && hEdit == GetDlgItem(hwndDlg, IDC_EDIT2))
		{
			SetTextColor((HDC)wParam, RGB(200, 200, 200));
			return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
		}break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_EDIT1:
			if (HIWORD(wParam) == EN_SETFOCUS && !EDIT1)
			{
				SetDlgItemText(hwndDlg, IDC_EDIT1, L"");
				EDIT1 = TRUE;
			}
			else if (HIWORD(wParam) == EN_KILLFOCUS)
			{
				if (GetWindowTextLength(GetDlgItem(hwndDlg, IDC_EDIT1)) == 0)
				{
					SetDlgItemText(hwndDlg, IDC_EDIT1, L"账号");
					EDIT1 = FALSE;
				}
			}break;
		case IDC_EDIT2:
			if (HIWORD(wParam) == EN_SETFOCUS && !EDIT2)
			{
				SetDlgItemText(hwndDlg, IDC_EDIT2, L"");
				EDIT2 = TRUE;
			}
			else if (HIWORD(wParam) == EN_KILLFOCUS)
			{
				if (GetWindowTextLength(GetDlgItem(hwndDlg, IDC_EDIT2)) == 0)
				{
					SetDlgItemText(hwndDlg, IDC_EDIT2, L"密码");
					EDIT2 = FALSE;
				}
			}break;
		case IDOK:
			//服务器验证账号密码的有效性//
		{
			std::wstring  wmsg = L"确定";
			int wcharlen = wmsg.size();
			int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, NULL, 0, NULL, NULL);//计算utf8_signlen字节数,含'\0'//
			std::string msg(utf8len, 0);//分配空间//
			WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, &msg[0], utf8len, NULL, NULL);//实际转换//
			send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//向服务器发送信息长度//
			send(g_clientSocket, msg.c_str(), utf8len, 0);//向服务器发送信息内容//

			MessageBox(NULL, L"点击确认，服务器将验证账号密码的有效性", L"QQ", MB_ICONINFORMATION);
			WCHAR account[16];
			WCHAR password[64];
			GetDlgItemText(hwndDlg, IDC_EDIT1, account, 16);//将账号保存,它会在字符串末尾添加一个'\0'//
			
			//将用户账号保存
			int wchar_account_len = wcslen(account);
			w_new_online_user_account.resize(wchar_account_len+1);
			wcscpy_s(&w_new_online_user_account[0],wchar_account_len+1 ,account);

			int utf8_new_user_account_len = WideCharToMultiByte(CP_UTF8,0,account, wchar_account_len + 1, NULL, 0, NULL, NULL);
			std::string utf8_new_user_account_str(utf8_new_user_account_len,'\0');
			WideCharToMultiByte(CP_UTF8, 0, account, wchar_account_len+1, &utf8_new_user_account_str[0], utf8_new_user_account_len, NULL, NULL);
			new_online_user_account = utf8_new_user_account_str;//含'\0'

			GetDlgItemText(hwndDlg, IDC_EDIT2, password, 64);//将密码保存,它会在字符串末尾添加一个'\0'//
			my_send_one(g_clientSocket, account);//发送账号//
			my_send_one(g_clientSocket, password);//发送密码//
			int recv_len = 0;
			recv(g_clientSocket, (char*)&recv_len, sizeof(recv_len), 0);//接收验证消息长度//
			if (recv_len <= 0)
			{
				MessageBox(NULL, L"接受账号和密码的验证信息长度失败", L"QQ", MB_ICONERROR);
				exit(1);
			}
			std::string recvchar(recv_len, 0);
			recv(g_clientSocket, &recvchar[0], recv_len, 0);//接受验证标识//
			int wlen = MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, NULL, 0);
			std::wstring wstr(wlen, 0);//分配空间//
			MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, &wstr[0], wlen);//实际转换//
			if (wcscmp(wstr.c_str(), L"faile") == 0)
			{
				MessageBox(NULL, L"该账号和密码未注册或账号和密码输入错误，请注册或重新输入", L"QQ", MB_ICONERROR);
				//SetDlgItemText(hwndDlg, IDC_EDIT1, L"");//清空账号框//
				//SetDlgItemText(hwndDlg, IDC_EDIT2, L"");//清空密码框//
				EndDialog(hwndDlg, IDC_EDIT_AGAIN);
				return (INT_PTR)TRUE;
			}
			else if (wcscmp(wstr.c_str(), L"sucess") == 0)
			{
				MessageBox(NULL, L"登录成功", L"QQ", MB_ICONINFORMATION);

				EndDialog(hwndDlg, IDOK);
				return (INT_PTR)TRUE;
			}
			else
			{
				MessageBox(NULL, L"未接收到验证消息", L"QQ", MB_ICONINFORMATION);
				exit(1);
			}

		}
		case IDCANCEL:
			std::wstring  wmsg = L"取消";
			int wlen = wmsg.size();
			int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wlen + 1, NULL, 0, NULL, NULL);//计算utf8_signlen字节数,含'\0'//
			std::string msg(utf8len, 0);//分配空间//
			WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wlen + 1, &msg[0], utf8len, NULL, NULL);//实际转换//
			send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//向服务器发送信息长度//
			send(g_clientSocket, msg.c_str(), utf8len, 0);//向服务器发送信息内容//
			EndDialog(hwndDlg, IDCANCEL);
			return (INT_PTR)TRUE;
		}
	}
	return (INT_PTR)FALSE;
}



int Generate_register_number()//产生账号//
{
	HCRYPTPROV hProvider = 0;
	CryptAcquireContext(&hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
	long long randomNum = 0;
	CryptGenRandom(hProvider, sizeof(randomNum), (BYTE*)&randomNum);//randomNum接收产生的随机数//
	CryptReleaseContext(hProvider, 0);
	int num = abs(randomNum % 9000000000) + 1000000000; // 转换为10位数
	if (num > 0)
		return num;
	else if (num <= 0)
		return -num;
}

INT_PTR CALLBACK Dialog_four(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//注册框//
{
	static long long num = 0;
	DRAWITEMSTRUCT* pdis = NULL;
	CString str;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, IDC_EDIT5, L"服务器分配给你的账号");
		if (passwd_back_to_account == 0)
		{
			num = Generate_register_number();
		}
		str.Format(L"%lld", num);
		wcscpy_s(g_szAccount, 32, str);//将账号保存到全局变量g_szAccount//
		SetDlgItemText(hwndDlg, IDC_EDIT6, str);
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDGOON:
		{
			std::wstring  wmsg = L"继续";
			int wcharlen = wmsg.size();
			int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, NULL, 0, NULL, NULL);//计算utf8_signlen字节数,含'\0'//
			std::string msg(utf8len, 0);//分配空间//
			WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, &msg[0], utf8len, NULL, NULL);//实际转换//
			send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//向服务器发送信息长度//
			send(g_clientSocket, msg.c_str(), utf8len, 0);//向服务器发送信息内容//

			EndDialog(hwndDlg, IDGOON);
			return (INT_PTR)TRUE;
		}
		case IDCANCEL:
		{
			std::wstring  wmsg = L"取消";
			int wcharlen = wmsg.size();
			int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, NULL, 0, NULL, NULL);//计算utf8_signlen字节数,含'\0'//
			std::string msg(utf8len, 0);//分配空间//
			WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, &msg[0], utf8len, NULL, NULL);//实际转换//
			send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//向服务器发送信息长度//
			send(g_clientSocket, msg.c_str(), utf8len, 0);//向服务器发送信息内容//

			EndDialog(hwndDlg, IDCANCEL);
			return (INT_PTR)TRUE;
		}
		}
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK Dialog_five(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//密码设置框//
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, IDC_EDIT9, L"密码设置");
		SetDlgItemText(hwndDlg, IDC_EDIT10, L"密码确认");
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDFINISH:
		{
			// 获取密码并验证
			WCHAR szPassword1[32], szPassword2[32];
			GetDlgItemText(hwndDlg, IDC_EDIT7, szPassword1, 32);
			GetDlgItemText(hwndDlg, IDC_EDIT8, szPassword2, 32);
			if (szPassword1[0] == '\0')
			{
				MessageBox(NULL, L"密码不能为空", L"QQ", MB_ICONERROR);
				SetFocus(GetDlgItem(hwndDlg, IDC_EDIT7));
				return (INT_PTR)TRUE;
			}
			if (wcscmp(szPassword1, szPassword2) != 0)//比较前后两次设置的密码是否相同//
			{
				MessageBox(hwndDlg, L"两次输入的密码不一致，请重新输入", L"QQ", MB_ICONERROR);
				SetDlgItemText(hwndDlg, IDC_EDIT8, L"");//清除密码//
				SetFocus(GetDlgItem(hwndDlg, IDC_EDIT8));
				return (INT_PTR)TRUE;
			}

			std::wstring  wmsg = L"完成";
			int wcharlen = wmsg.size();
			int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, NULL, 0, NULL, NULL);//计算utf8_signlen字节数,含'\0'//
			std::string msg(utf8len, 0);//分配空间//
			WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, &msg[0], utf8len, NULL, NULL);//实际转换//
			MessageBox(NULL, L"即将给服务器发送密码框的判断 完成或取消", L"QQ", MB_ICONINFORMATION);
			send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//向服务器发送信息长度//
			send(g_clientSocket, msg.c_str(), utf8len, 0);//向服务器发送信息内容//

			// 保存密码到全局变量
			wcscpy_s(g_szPassword, szPassword1);
			MessageBox(hwndDlg, L"密码已成功设置", L"QQ", MB_ICONINFORMATION);
			EndDialog(hwndDlg, IDFINISH);
			return (INT_PTR)TRUE;
		}
		case IDCANCEL:
		{
			std::wstring  wmsg = L"取消";
			int wcharlen = wmsg.size();
			int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, NULL, 0, NULL, NULL);//计算utf8_signlen字节数,含'\0'//
			std::string msg(utf8len, 0);//分配空间//
			WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, &msg[0], utf8len, NULL, NULL);//实际转换//
			send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//向服务器发送信息长度//
			send(g_clientSocket, msg.c_str(), utf8len, 0);//向服务器发送信息内容//

			passwd_back_to_account = 1;//账号已生成标志//
			EndDialog(hwndDlg, IDCANCEL);
			return (INT_PTR)TRUE;
		}
		}
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK Dialog_six(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//注册成功框//
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, IDC_EDIT11, L"注册成功,按下“确认”即可跳转到到登录界面.");
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{

			std::wstring  wmsg = L"确定";
			int wcharlen = wmsg.size();
			int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, NULL, 0, NULL, NULL);//计算utf8_signlen字节数,含'\0'//
			std::string msg(utf8len, 0);//分配空间//
			WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, &msg[0], utf8len, NULL, NULL);//实际转换//
			send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//向服务器发送信息长度//
			send(g_clientSocket, msg.c_str(), utf8len, 0);//向服务器发送信息内容//

			EndDialog(hwndDlg, IDOK);
			return (INT_PTR)TRUE;
		}
		case IDCANCEL:
		{
			std::wstring  wmsg = L"取消";
			int wcharlen = wmsg.size();
			int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, NULL, 0, NULL, NULL);//计算utf8_signlen字节数,含'\0'//
			std::string msg(utf8len, 0);//分配空间//
			WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, &msg[0], utf8len, NULL, NULL);//实际转换//
			send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//向服务器发送信息长度//
			send(g_clientSocket, msg.c_str(), utf8len, 0);//向服务器发送信息内容//

			EndDialog(hwndDlg, IDCANCEL);
			return (INT_PTR)TRUE;
		}
		}
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK Dialog_seven(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//注册时个人信息框//
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, IDC_EDIT19, L"昵称");
		SetDlgItemText(hwndDlg, IDC_EDIT12, L"性别");
		SetDlgItemText(hwndDlg, IDC_EDIT13, L"年龄");
		SetDlgItemText(hwndDlg, IDC_EDIT14, L"个性签名");
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			// 获取所有个人信息
			GetDlgItemText(hwndDlg, IDC_EDIT15, g_szNickname, 32);//将昵称保存到全局变量g_szNickname它会在字符串末尾添加一个'\0'//
			GetDlgItemText(hwndDlg, IDC_EDIT16, g_szGender, 16);//将性别保存到全局变量g_szGender//
			GetDlgItemText(hwndDlg, IDC_EDIT17, g_szAge, 8);//将年龄保存到全局变量g_szAge//
			GetDlgItemText(hwndDlg, IDC_EDIT18, g_szSignature, 128);//将个性签名保存到全局变量g_szNickname//
			if (g_szNickname[0] == '\0')
			{
				MessageBox(NULL, L"昵称未设置", L"QQ", MB_ICONERROR);
				SetFocus(GetDlgItem(hwndDlg, IDC_EDIT19));
				return (INT_PTR)TRUE;
			}
			if (g_szGender[0] == '\0' || (wcscmp(g_szGender, L"男") != 0) && (wcscmp(g_szGender, L"女") != 0))
			{
				if (g_szGender[0] == '\0')
					MessageBox(NULL, L"性别未设置", L"QQ", MB_ICONERROR);
				else if (wcscmp(g_szGender, L"男") != 0 && wcscmp(g_szGender, L"女") != 0)
					MessageBox(NULL, L"性别设置错误，性别只能是‘男’或者‘女'", L"QQ", MB_ICONERROR);
				SetFocus(GetDlgItem(hwndDlg, IDC_EDIT12));
				return (INT_PTR)TRUE;
			}
			if (g_szAge[0] == '\0')
			{
				MessageBox(NULL, L"年龄未设置", L"QQ", MB_ICONERROR);
				SetFocus(GetDlgItem(hwndDlg, IDC_EDIT13));
				return (INT_PTR)TRUE;
			}
			if (g_szSignature[0] == '\0')
			{
				MessageBox(NULL, L"个性签名未设置", L"QQ", MB_ICONERROR);
				SetFocus(GetDlgItem(hwndDlg, IDC_EDIT14));
				return (INT_PTR)TRUE;
			}
			MessageBox(NULL, L"个人信息准备完毕，即将向服务器发送确定信息", L"QQ", MB_ICONINFORMATION);
			std::wstring  wmsg = L"确定";
			int wcharlen = wmsg.size();
			int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, NULL, 0, NULL, NULL);//计算utf8_signlen字节数,含'\0'//
			std::string msg(utf8len, 0);//分配空间//
			WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, &msg[0], utf8len, NULL, NULL);//实际转换//
			send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//向服务器发送信息长度//
			send(g_clientSocket, msg.c_str(), utf8len, 0);//向服务器发送信息内容//

			// 再次获取所有个人信息
			GetDlgItemText(hwndDlg, IDC_EDIT15, g_szNickname, 32);//将昵称保存到全局变量g_szNickname//
			GetDlgItemText(hwndDlg, IDC_EDIT16, g_szGender, 16);//将性别保存到全局变量g_szGender//
			GetDlgItemText(hwndDlg, IDC_EDIT17, g_szAge, 8);//将年龄保存到全局变量g_szAge//
			GetDlgItemText(hwndDlg, IDC_EDIT18, g_szSignature, 128);//将个性签名保存到全局变量g_szNickname//
			EndDialog(hwndDlg, IDOK);
			return (INT_PTR)TRUE;
		}
		case IDCANCEL:

			std::wstring  wmsg = L"取消";
			int wcharlen = wmsg.size();
			int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, NULL, 0, NULL, NULL);//计算utf8_signlen字节数,含'\0'//
			std::string msg(utf8len, 0);//分配空间//
			WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, &msg[0], utf8len, NULL, NULL);//实际转换//
			send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//向服务器发送信息长度//
			send(g_clientSocket, msg.c_str(), utf8len, 0);//向服务器发送信息内容//
			EndDialog(hwndDlg, IDCANCEL);
			return (INT_PTR)TRUE;
		}
	}
	return (INT_PTR)FALSE;
}

WCHAR g_szPhotoPath[MAX_PATH] = L"";
HBITMAP g_hBitmap = NULL;//HBITMAP 是位图的句柄类型//
HBITMAP LoadImageFileToBitmap(HWND hwnd, LPCWSTR path, int width, int height)//加载头像,LPCWSTR指向常量宽字符字符串的长指针//
{
	
	using namespace Gdiplus;
	static ULONG_PTR gdiplusToken = 0;
	if (!gdiplusToken) //检查GDI+是否初始化//
	{
		GdiplusStartupInput gdiplusStartupInput;
		GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	}
	
	Bitmap bmp(path);//加载图片//
	if (bmp.GetLastStatus() != Ok) //检查bmp最近最后一次的操作状态是否成功，即加载图片是否成功//
		return NULL;
	// 缩放到控件大小
	Bitmap* pThumb = new Bitmap(width, height, bmp.GetPixelFormat());//bmp.GetPixelFormat获取位图的像素格式//
	Graphics g(pThumb);//Graphics是GDI+的核心绘图类，封装了所有绘图操作；g是定义的Graphics的对象名;pThumb:指向Bitmap对象的指针//
	g.DrawImage(&bmp, 0, 0, width, height);//进行图像绘制//
	HBITMAP hBmp = NULL;
	pThumb->GetHBITMAP(Color(200, 230, 255), &hBmp);
	delete pThumb;
	//读取文件//
	HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		MessageBox(NULL, L"读取头像文件失败", L"QQ", MB_ICONERROR);
		return hBmp;
	}
	else
	{
		g_dwImageSize = GetFileSize(hFile, NULL);//获取数据大小//
		g_pImageData = new BYTE[g_dwImageSize];
		DWORD dwREAD;
		ReadFile(hFile, g_pImageData, g_dwImageSize, &dwREAD, NULL);//将图片文件存储在g_pImageData中//
		CloseHandle(hFile);//释放文件句柄//
	}
	return hBmp;
}

INT_PTR CALLBACK Dialog_eight(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//头像上传框//
{
	OPENFILENAME ofn = { 0 };//初始化文件对话框//
	WCHAR szFile[MAX_PATH] = L"";//头文件<windows.h>包含了MAX_PATH的定义//
	switch (uMsg)
	{
	case WM_INITDIALOG:
		g_szPhotoPath[0] = 0;
		if (g_hBitmap)
		{
			DeleteObject(g_hBitmap);
			g_hBitmap = NULL;
		}
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDPHOTO:
		{
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hwndDlg;
			ofn.lpstrFilter = L"图片文件 (*.jpg;*.png)\0*.jpg;*.png\0所有文件 (*.*)\0*.*\0";
			ofn.lpstrFile = szFile;//保存图片路径//
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
			if (GetOpenFileName(&ofn))
			{
				// 检查扩展名
				std::wstring path = szFile;//获取图片了路径//
				auto pos = path.find_last_of(L'.');//在字符串中从后向前查找最后一个.字符的位置//
				if (pos == std::wstring::npos) {
					MessageBox(hwndDlg, L"文件无扩展名，无法识别", L"QQ", MB_OK | MB_ICONERROR);
					break;
				}
				std::wstring ext = path.substr(pos);//获取扩展名//
				std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);//将扩展名的字符转化为小写，towlower所要进行的操作//
				if (ext != L".jpg" && ext != L".png") {
					MessageBox(hwndDlg, L"仅支持jpg和png", L"QQ", MB_OK | MB_ICONWARNING);
					break;
				}
				// 保存路径
				wcsncpy_s(g_szPhotoPath, szFile, _TRUNCATE);//将szFile里的路径保存到g_szPhotoPath,_TRUNCATE允许截断路径，防止溢出目标缓冲区//

				// 加载并显示到静态控件
				if (g_hBitmap)
				{
					DeleteObject(g_hBitmap);
					g_hBitmap = NULL;
				}
				RECT rc;//rc的坐标是屏幕坐标//
				GetWindowRect(GetDlgItem(hwndDlg, IDC_MY_IMAGE_TWO), &rc);//获取图片控件的位置和尺寸，并保存到rc中//
				MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rc, 2);//将rc的坐标转换为窗口坐标//
				int w = rc.right - rc.left, h = rc.bottom - rc.top;//获取图片的宽和高//
				g_hBitmap = LoadImageFileToBitmap(hwndDlg, g_szPhotoPath, w, h);
				if (g_hBitmap)
					SendDlgItemMessage(hwndDlg, IDC_MY_IMAGE_TWO, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)g_hBitmap);//向对话框指定控件发送消息，这里是更新图片控件//
				else
					MessageBox(hwndDlg, L"图片加载失败", L"QQ", MB_OK | MB_ICONERROR);
			}
			return (INT_PTR)TRUE;
		}
		case IDOK:
		{
			if (wcslen(g_szPhotoPath) == 0)
			{
				MessageBox(hwndDlg, L"请先上传头像", L"QQ", MB_OK | MB_ICONWARNING);
				break;
			}
			std::wstring  wmsg = L"确定";
			int wcharlen = wmsg.size();
			int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, NULL, 0, NULL, NULL);//计算utf8_signlen字节数,含'\0'//
			std::string msg(utf8len, 0);//分配空间//
			WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, &msg[0], utf8len, NULL, NULL);//实际转换//
			send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//向服务器发送信息长度//
			send(g_clientSocket, msg.c_str(), utf8len, 0);//向服务器发送信息内容//
			

			
			EndDialog(hwndDlg, IDOK);
			return (INT_PTR)TRUE;
		}
		case IDCANCEL:
		{
			std::wstring  wmsg = L"取消";
			int wcharlen = wmsg.size();
			int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, NULL, 0, NULL, NULL);//计算utf8_signlen字节数,含'\0'//
			std::string msg(utf8len, 0);//分配空间//
			WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, &msg[0], utf8len, NULL, NULL);//实际转换//
			send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//向服务器发送信息长度//
			send(g_clientSocket, msg.c_str(), utf8len, 0);//向服务器发送信息内容//

			EndDialog(hwndDlg, IDCANCEL);
			return (INT_PTR)TRUE;
		}
		}
	case WM_DESTROY:

		if (g_hBitmap)
		{
			DeleteObject(g_hBitmap);
			g_hBitmap = NULL;
		}
		return (INT_PTR)TRUE;

	}
	return (INT_PTR)FALSE;
}

//IP格式验证//
bool IsValidIP(const WCHAR* ip)
{
	char ipAnsi[16];
	// 将宽字符转换为ANSI
	WideCharToMultiByte(CP_ACP, 0, ip, -1, ipAnsi, 16, NULL, NULL);
	struct in_addr addr;
	// 尝试转换IP地址
	return (inet_pton(AF_INET, ipAnsi, &addr) == 1);
}

INT_PTR CALLBACK Dialog_nine(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//IP输入框//
{
	HANDLE hThread = NULL;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, IDC_EDIT20, L"服务器IP");
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			WCHAR IP[16];
			GetDlgItemText(hwndDlg, IDC_EDIT21, IP, 16);//获取IP地址，存储在IP数组里面//
			if (!IsValidIP(IP))//IP格式验证//
			{
				MessageBox(hwndDlg, L"IP格式错误", L"QQ", MB_ICONERROR);
				SetDlgItemText(hwndDlg, IDC_EDIT21, L"");//清除错误IP//
				SetFocus(GetDlgItem(hwndDlg, IDC_EDIT21));
				return (INT_PTR)TRUE;
			}
			MessageBox(hwndDlg, L"IP格式正确", L"QQ", MB_ICONINFORMATION);
			// 禁用确定按钮，防止重复点击
			EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
			//创建工作线程，执行网络连接//
			DWORD threadId;
			hThread = CreateThread(
				NULL, 0,
				[](LPVOID param)->DWORD {
					HWND hwnd = (HWND)param;
					g_clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//创建套接字//
					if (g_clientSocket == INVALID_SOCKET)
					{
						MessageBox(hwnd, L"创建套接字失败", L"QQ", MB_ICONERROR);
						return 1;
					}
					MessageBox(hwnd, L"创建套接字成功", L"QQ", MB_ICONINFORMATION);
					//设置服务器地址//
					sockaddr_in serverAddr;
					serverAddr.sin_family = AF_INET;
					serverAddr.sin_port = htons(8080);//服务器端口//
					WCHAR IP[16];
					GetDlgItemText(hwnd, IDC_EDIT21, IP, 16);//从对话框获取IP地址//
					char ipAnsi[16];
					WideCharToMultiByte(CP_UTF8, 0, IP, -1, ipAnsi, 16, NULL, NULL);//IP地址转化为char类型//
					inet_pton(AF_INET, ipAnsi, &serverAddr.sin_addr);
					if (connect(g_clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) //连接服务器//
					{
						int error = WSAGetLastError();
						switch (error)
						{
						case WSAECONNREFUSED:
							MessageBox(hwnd, L"连接被拒绝", L"QQ", MB_ICONERROR);
							break;
						case WSAENETUNREACH:
							MessageBox(hwnd, L"网络不可达", L"QQ", MB_ICONERROR);
							break;
						case WSAEHOSTDOWN:
							MessageBox(hwnd, L"主机不可达", L"QQ", MB_ICONERROR);
							break;
						case WSAETIMEDOUT:
							MessageBox(hwnd, L"连接超时", L"QQ", MB_ICONERROR);
							break;
						default:
							MessageBox(hwnd, L"连接服务器失败", L"QQ", MB_ICONERROR);
							break;
						}
						closesocket(g_clientSocket);
						g_clientSocket = INVALID_SOCKET;
						PostMessage(hwnd, WM_CONNECT_FAILED, 0, 0);
						return 1;
					}
					g_isConnected = true;
					PostMessage(hwnd, WM_CONNECT_SUCCESS, 0, 0);
					return 0;
				}, hwndDlg, 0, &threadId
			);
			if (hThread)
			{
				CloseHandle(hThread);//释放线程句柄//
			}
			return (INT_PTR)TRUE;
		case IDCANCEL:
			EndDialog(hwndDlg, IDCANCEL);//实际返回操作执行标志//
			return (INT_PTR)TRUE;//不会返回操作执行标志//
		}break;
	case WM_CONNECT_SUCCESS:
	{
		MessageBox(hwndDlg, L"服务器连接成功", L"QQ", MB_ICONINFORMATION);
		//发送字符//
		 /*
		 std::wstring msg = L"你好，服务器。";
		 int slen=WideCharToMultiByte(CP_UTF8,0,msg.c_str(),msg.size(), NULL, 0, NULL, NULL);//计算转换字节数//
		 std::string s(slen, 0);//分配空间//
		 WideCharToMultiByte(CP_UTF8, 0, msg.c_str(),msg.size(), &s[0], slen, NULL, NULL);//计算转换字节数//
		 send(g_clientSocket, (char*)&slen,sizeof(slen),0);//先发送长度//
		 send(g_clientSocket, (const char*)s.c_str(),slen, 0);//再发送内容//
		 */
		EndDialog(hwndDlg, IDOK);
		return (INT_PTR)TRUE;
	}
	case WM_CONNECT_FAILED:
		EnableWindow(GetDlgItem(hwndDlg, IDOK), TRUE); // 重新启用确定按钮
		return (INT_PTR)TRUE;
	}
	return (INT_PTR)FALSE;
}


INT_PTR CALLBACK Dialog_ten(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//登录界面首页框//
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{

		//向服务器发送加载用户头像的请求
		std::wstring Img_info = L"请求加载登录用户头像";
		int utf8_len = WideCharToMultiByte(CP_UTF8, 0, Img_info.c_str(), Img_info.size() + 1, NULL, 0, NULL, NULL);
		std::string  utf8_request(utf8_len,'\0');
		WideCharToMultiByte(CP_UTF8,0, Img_info.c_str(), Img_info.size() + 1, &utf8_request[0], utf8_len, NULL, NULL);
		send(g_clientSocket, (char*)&utf8_len, sizeof(utf8_len), 0);
		send(g_clientSocket, utf8_request.c_str(), utf8_len, 0);
		MessageBox(NULL, L"已经成功向服务器发送加载头像数据的请求", L"QQ", MB_ICONINFORMATION);
		//接收服务器发来的用户头像数据
		int recv_img_len = 0;
		int size_x=recv(g_clientSocket,(char*)&recv_img_len,sizeof(recv_img_len),0);
		if (size_x != sizeof(recv_img_len))
		{
			MessageBox(NULL, L"接收头像数据长度失败", L"QQ", MB_ICONERROR);
			return (INT_PTR)TRUE;
		}
		MessageBox(NULL, L"接收头像数据长度成功", L"QQ", MB_ICONINFORMATION);
		BYTE* user_img = new BYTE[recv_img_len];//接收图片数据大小
		int r = 0;//每次接收的数据
		int sum = 0;//总接收数据
		while (sum< recv_img_len)
		{
			r=recv(g_clientSocket, (char*)user_img+sum, recv_img_len-sum, 0);
			if(r>0)
			{
			  sum += r;
		    }
			if (sum == recv_img_len)
			{
				MessageBox(NULL, L"请求用户图像数据成功", L"QQ", MB_ICONINFORMATION);
			}
			else if (sum < recv_img_len)
			{
				MessageBox(NULL,L"请求用户数据失败",L"QQ",MB_ICONERROR);
				return (INT_PTR)TRUE;
			}
		}

		//存储图片数据
		user_image_data.assign(user_img, user_img + recv_img_len);
        
		/*
		int x=recv(g_clientSocket, (char*)user_img, recv_img_len, 0);
		if(x==recv_img_len)
		MessageBox(NULL, L"请求用户图像数据成功", L"QQ", MB_ICONINFORMATION);
		else
		{
			MessageBox(NULL, L"请求用户图像数据失败", L"QQ", MB_ICONERROR);
		}
		*/

		//获取控件大小
		HWND hImg = GetDlgItem(hwndDlg, IDC_MY_IMAGE_THREE);
		RECT rc;
		GetClientRect(hImg, &rc);
		int w = rc.right - rc.left;
		int h = rc.bottom - rc.top;

       //将图片绘制在控件IDC_MY_IMAGE_THREE上
		using namespace Gdiplus;
		static ULONG_PTR gdiplusToken = 0;
		if (!gdiplusToken) //检查GDI+是否初始化//
		{
			GdiplusStartupInput gdiplusStartupInput;
			GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
		}

		//将图片绘制在控件IDC_MY_IMAGE_THREE上
		if (user_img && recv_img_len > 0)
		{
			// 创建IStream包装内存数据
			HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, recv_img_len);
			if (hMem)
			{
				void* pMem = GlobalLock(hMem);
				memcpy(pMem, user_img, recv_img_len);
				GlobalUnlock(hMem);

				IStream* pStream = NULL;
				if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) == S_OK)
				{
					Bitmap* bmp = Gdiplus::Bitmap::FromStream(pStream);
					if (bmp && bmp->GetLastStatus() == Ok)
					{
						// 创建目标尺寸的Bitmap，并DrawImage拉伸原图
						Bitmap* scaledBmp = new Bitmap(w, h, bmp->GetPixelFormat());
						Graphics g(scaledBmp);
						g.DrawImage(bmp, 0, 0, w, h);

						HBITMAP hBmp = NULL;
						scaledBmp->GetHBITMAP(Color(200, 230, 255), &hBmp);
						//HWND hImg = GetDlgItem(hwndDlg, IDC_MY_IMAGE_THREE);
						// 释放控件原有图片
						HBITMAP hOldBmp = (HBITMAP)SendMessage(hImg, STM_GETIMAGE, IMAGE_BITMAP, 0);
						if (hOldBmp) DeleteObject(hOldBmp);
						// 设置新图片
						SendMessage(hImg, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmp);
						delete scaledBmp;
					}
					delete bmp;
					pStream->Release();
				}
				// hMem会被pStream释放
			}
			delete[] user_img;
			GlobalFree(hMem);
		}
		return (INT_PTR)TRUE;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_EDIT22:
		{
			std::wstring wstr = L"聊天室";
			int utf8_len = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1, NULL, 0, NULL, NULL);//计算转换为utf8的字节长度，含'\0'//
			std::string str(utf8_len, 0);
			WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1,&str[0],utf8_len, NULL, NULL);//实际转换//
			send(g_clientSocket, (char*)&utf8_len, sizeof(utf8_len), 0);//先发长度//
			send(g_clientSocket, str.c_str(), utf8_len, 0);//后发内容//
			MessageBox(NULL, L"已经成功发送登陆界面的按钮信息", L"QQ", MB_ICONINFORMATION);

			EndDialog(hwndDlg, IDC_EDIT22);//聊天室按钮//
			return (INT_PTR)TRUE;
		}
		case IDC_EDIT23:

		{
			std::wstring wstr = L"好友";
			int utf8_len = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1, NULL, 0, NULL, NULL);//计算转换为utf8的字节长度，含'\0'//
			std::string str(utf8_len, 0);
			WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1, &str[0], utf8_len, NULL, NULL);//实际转换//
			send(g_clientSocket, (char*)&utf8_len, sizeof(utf8_len), 0);//先发长度//
			send(g_clientSocket, str.c_str(), utf8_len, 0);//后发内容//
			MessageBox(NULL, L"已经成功发送登陆界面的按钮信息", L"QQ", MB_ICONINFORMATION);

			EndDialog(hwndDlg, IDC_EDIT23);//好友按钮//
			return (INT_PTR)TRUE;
		}
		case IDC_EDIT24:
		{
			std::wstring wstr = L"个人信息";
			int utf8_len = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1, NULL, 0, NULL, NULL);//计算转换为utf8的字节长度，含'\0'//
			std::string str(utf8_len, 0);
			WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1, &str[0], utf8_len, NULL, NULL);//实际转换//
			send(g_clientSocket, (char*)&utf8_len, sizeof(utf8_len), 0);//先发长度//
			send(g_clientSocket, str.c_str(), utf8_len, 0);//后发内容//
			MessageBox(NULL, L"已经成功发送登陆界面的按钮信息", L"QQ", MB_ICONINFORMATION);

			EndDialog(hwndDlg, IDC_EDIT24);//个人信息按钮//
			return (INT_PTR)TRUE;
		}
		case IDC_EDIT25:

		{
			std::wstring wstr = L"更换头像";
			int utf8_len = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1, NULL, 0, NULL, NULL);//计算转换为utf8的字节长度，含'\0'//
			std::string str(utf8_len, 0);
			WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1, &str[0], utf8_len, NULL, NULL);//实际转换//
			send(g_clientSocket, (char*)&utf8_len, sizeof(utf8_len), 0);//先发长度//
			send(g_clientSocket, str.c_str(), utf8_len, 0);//后发内容//
			MessageBox(NULL, L"已经成功发送登陆界面的按钮信息", L"QQ", MB_ICONINFORMATION);

			EndDialog(hwndDlg, IDC_EDIT25);//更换头像按钮//
			return (INT_PTR)TRUE;
		}
		case IDC_EDIT26:

		{
			std::wstring wstr = L"服务器";
			int wstr_len=wcslen(wstr.c_str());//宽字符串的字符数，不含'\0'//
			int utf8_len = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr_len+1, NULL, 0, NULL, NULL);//计算转换为utf8的字节长度，含'\0'//
			std::string str(utf8_len, 0);
			WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr_len + 1, &str[0], utf8_len, NULL, NULL);//实际转换//
			send(g_clientSocket, (char*)&utf8_len, sizeof(utf8_len), 0);//先发长度//
			send(g_clientSocket,str.c_str(),utf8_len, 0);//后发内容//
			MessageBox(NULL,L"已经成功发送登陆界面的“服务器”按钮信息",L"QQ",MB_ICONINFORMATION);

			EndDialog(hwndDlg, IDC_EDIT26);//服务器按钮//
			return (INT_PTR)TRUE;
		}
		case IDCANCEL:

		{
			std::wstring wstr = L"退出";
			int utf8_len = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1, NULL, 0, NULL, NULL);//计算转换为utf8的字节长度，含'\0'//
			std::string str(utf8_len, 0);
			WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1, &str[0], utf8_len, NULL, NULL);//实际转换//
			send(g_clientSocket, (char*)&utf8_len, sizeof(utf8_len), 0);//先发长度//
			send(g_clientSocket, str.c_str(), utf8_len, 0);//后发内容//
			MessageBox(NULL, L"已经成功发送登陆界面的按钮信息", L"QQ", MB_ICONINFORMATION);

			EndDialog(hwndDlg, IDCANCEL);//退出按钮//
			return (INT_PTR)TRUE;
		}
		case IDC_EDIT43:

		{
			std::wstring wstr = L"返回";
			int utf8_len = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1, NULL, 0, NULL, NULL);//计算转换为utf8的字节长度，含'\0'//
			std::string str(utf8_len, 0);
			WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1, &str[0], utf8_len, NULL, NULL);//实际转换//
			send(g_clientSocket, (char*)&utf8_len, sizeof(utf8_len), 0);//先发长度//
			send(g_clientSocket, str.c_str(), utf8_len, 0);//后发内容//
			MessageBox(NULL, L"已经成功发送登陆界面的按钮信息", L"QQ", MB_ICONINFORMATION);



			EndDialog(hwndDlg, IDC_EDIT43);//返回按钮//
			return (INT_PTR)TRUE;
		}
		case IDC_EDIT44:
		{
			std::wstring wstr = L"注销账号";
			int utf8_len = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1, NULL, 0, NULL, NULL);//计算转换为utf8的字节长度，含'\0'//
			std::string str(utf8_len, 0);
			WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1, &str[0], utf8_len, NULL, NULL);//实际转换//
			send(g_clientSocket, (char*)&utf8_len, sizeof(utf8_len), 0);//先发长度//
			send(g_clientSocket, str.c_str(), utf8_len, 0);//后发内容//
			MessageBox(NULL, L"已经成功发送登陆界面的按钮信息", L"QQ", MB_ICONINFORMATION);
		
			EndDialog(hwndDlg, IDC_EDIT44);//退出按钮//
			return (INT_PTR)TRUE;
		}

		}
	}
	return (INT_PTR)FALSE;
}



INT_PTR CALLBACK Dialog_eleven(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//服务器对话框//
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BROADCAST:
		{
			//发送执行操作标识
			std::wstring w_char = L"查看广播";
			int send_len = WideCharToMultiByte(CP_UTF8, 0, w_char.c_str(), w_char.size() + 1, NULL, 0, NULL, NULL);//先计算转换长度，含‘\0'
			std::string send_str(send_len, 0);//分配空间//
			WideCharToMultiByte(CP_UTF8, 0, w_char.c_str(), w_char.size() + 1, &send_str[0], send_len, NULL, NULL);//实际转换//
			send(g_clientSocket, (char*)&send_len, sizeof(send_len), 0);//先发长度//
			send(g_clientSocket, send_str.c_str(), send_len, 0);//再发内容//

			std::string str_account = new_online_user_account;
			if (!str_account.empty() && str_account.back() == '\0')
			{
				str_account.pop_back();
			}

			char buf2[256];
			sprintf_s(buf2, "str_account: [%s] len=%d\n", str_account.c_str(), (int)str_account.size());
			OutputDebugStringA(buf2);

			//发送用户账号，以供查询
			int str_account_len = str_account.size();
			send(g_clientSocket, (char*)&str_account_len,sizeof(str_account_len),0);
			send(g_clientSocket,str_account.c_str(),str_account_len,0);
		    
			//接收用户昵称
			int str_nickname_l = 0;
			recv(g_clientSocket,(char*)&str_nickname_l,sizeof(str_nickname_l),0);
			std::string str_nickname(str_nickname_l,'\0');
			recv(g_clientSocket,&str_nickname[0],str_nickname_l,0);
			
			//将用户昵称转化为宽字符
			int w_nick_len = 0;
			w_nick_len = MultiByteToWideChar(CP_UTF8, 0, str_nickname.c_str(), str_nickname.size(), NULL, 0);
			std::wstring w_nickname_s(w_nick_len, L'\0');
			MultiByteToWideChar(CP_UTF8, 0, str_nickname.c_str(),str_nickname.size(), &w_nickname_s[0], w_nick_len);


	       //接收广播消息条数
			int sum_broadcast = 0;
			recv(g_clientSocket, (char*)&sum_broadcast,sizeof(sum_broadcast),0);

			if (sum_broadcast <= 0)
			{
				//提示暂无新消息
				std::wstring my_notice = L"服务器暂时未向您发送广播消息";
				int wlen_two = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_EDIT27));//获取聊天框原有的文本,不包含'\0'//
				std::wstring wstr_two(wlen_two + 1, 0);//分配空间//
				GetWindowText(GetDlgItem(hwndDlg, IDC_EDIT27), &wstr_two[0], wlen_two + 1);
				std::wstring fullmsg = L"";
				if (wlen_two > 0)//如果不是第一条消息就进行//
				{
					if (wstr_two.back() == L'\0')
					{
						wstr_two.pop_back();
					}
					fullmsg = wstr_two;

					fullmsg += L"\r\n";
				}
				fullmsg += my_notice;
				SetDlgItemText(hwndDlg, IDC_EDIT27, fullmsg.c_str());//将文本显示在对话框上//
				return (INT_PTR)TRUE;
			}
			//接收广播消息
			int i = 0;
		
			while (i < sum_broadcast)
			{
				int l = 0;
				recv(g_clientSocket,(char*)&l,sizeof(l),0);
				std::string s(l, '\0');
				int r = 0;
				int sum = 0;
				while (sum < l)
				{
					r=recv(g_clientSocket, &s[0]+sum, l-sum, 0);
					if (r > 0)
					{
						sum += r;
					}
					if (sum == l)
					{
						//将消息转化宽字符串并去除末尾的'\0'
						int w_l = 0;
						w_l=MultiByteToWideChar(CP_UTF8,0,s.c_str(),s.size(),NULL,0);
						std::wstring w_s(w_l,L'\0');
						MultiByteToWideChar(CP_UTF8, 0, s.c_str(), s.size(),&w_s[0],w_l);
						if (!w_s.empty() && w_s.back() == '\0')
						{
							w_s.pop_back();
						}
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
						msg_one += w_nickname_s;
						msg_one += L"]";
						//MessageBox(NULL, L"已经成功拼接好服务器字段", L"QQ", MB_ICONINFORMATION);
						msg_one += w_s.c_str();

						//以一定的格式显示广播信息
						int wlen_two = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_EDIT27));//获取聊天框原有的文本,不包含'\0'//
						std::wstring wstr_two(wlen_two + 1, 0);//分配空间//
						GetWindowText(GetDlgItem(hwndDlg, IDC_EDIT27), &wstr_two[0], wlen_two + 1);
						std::wstring fullmsg = L"";
						if (wlen_two > 0)//如果不是第一条消息就进行//
						{
							if (wstr_two.back() == L'\0')
							{
								wstr_two.pop_back();
							}
							fullmsg = wstr_two;

							fullmsg += L"\r\n";
						}
						fullmsg +=msg_one;
						SetDlgItemText(hwndDlg, IDC_EDIT27, fullmsg.c_str());//将文本显示在对话框上//
						break;
					}
				}
				i++;
			}
		}
		case IDOK:
		{
			//获取文本发送区的文本//
			HWND hwnd = GetDlgItem(hwndDlg, IDC_EDIT28);//获取聊天框控件句柄//
			int wlen = GetWindowTextLength(hwnd);//获取控件文本长度，不含'\0'//
			if (wlen <= 0)
			{
				MessageBox(NULL,L"请键入内容",L"QQ",MB_ICONINFORMATION);
				return(INT_PTR)TRUE;
			}
			//确保有内容的消息才有效//

			std::wstring w_char = L"发送";
			int send_len = WideCharToMultiByte(CP_UTF8, 0, w_char.c_str(), w_char.size() + 1, NULL, 0, NULL, NULL);//先计算转换长度//
			std::string send_str(send_len, 0);//分配空间//
			WideCharToMultiByte(CP_UTF8, 0, w_char.c_str(), w_char.size() + 1, &send_str[0], send_len, NULL, NULL);//实际转换//
			send(g_clientSocket, (char*)&send_len, sizeof(send_len), 0);//先发长度//
			send(g_clientSocket, send_str.c_str(), send_len, 0);//再发内容//


			std::wstring wstr(wlen + 1, 0);//分配空间//
			GetWindowText(hwnd, &wstr[0], wlen+1);//获取控件文本//
			int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wlen + 1, NULL, 0, NULL, NULL);//计算utf8字节长度//
			
			//std::wstring n = std::to_wstring(utf8_len);
			//MessageBox(NULL, n.c_str(), L"QQ", MB_ICONINFORMATION);

			std::string utf8_str(utf8_len, 0);//分配空间//
			//MessageBox(NULL,L"客户端已成功分配空间来存储数据", L"QQ", MB_ICONINFORMATION);
			WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wlen + 1, &utf8_str[0], utf8_len, NULL, NULL);
			//进行发送和退出按钮判断//
	
			// 发送给服务器//
			send(g_clientSocket,(char*)&utf8_len,sizeof(utf8_len),0);//先发长度//
			send(g_clientSocket, &utf8_str[0], utf8_len, 0);//后发内容//
			//MessageBox(NULL, L"已经成功向服务器发送客户端的对话", L"QQ", MB_ICONINFORMATION);
			
			//接收服务器返回的消息//
			int recv_len = 0;
			recv(g_clientSocket, (char*)&recv_len, sizeof(recv_len), 0);//先接收长度//
			if (recv_len <= 0)
			{
				MessageBox(NULL, L"接收服务器发送的信息长度失败", L"QQ", MB_ICONERROR);
				return (INT_PTR) TRUE;
			}
			std::string recv_char(recv_len, 0);//分配空间//
			recv(g_clientSocket, &recv_char[0], recv_len, 0);//接收内容//
			MessageBox(NULL, L"已经成功接收服务器返回的消息", L"QQ", MB_ICONINFORMATION);
			int wrecv_len = MultiByteToWideChar(CP_UTF8, 0, recv_char.c_str(), recv_len, NULL, 0);//计算出转换长度//
			std::wstring wrecv_str(wrecv_len, 0);//分配空间//
			MultiByteToWideChar(CP_UTF8, 0, recv_char.c_str(), recv_len, &wrecv_str[0], wrecv_len);//实际转换,已经将接受的内容保存到wrecv_str//
		
			int wlen_two= GetWindowTextLength(GetDlgItem(hwndDlg,IDC_EDIT27));//获取聊天框原有的文本,不包含'\0'//
			std::wstring wstr_two(wlen_two+1,0);//分配空间//
			GetWindowText(GetDlgItem(hwndDlg, IDC_EDIT27), &wstr_two[0], wlen_two+1);
			std::wstring fullmsg=L"";
			if (wlen_two>0)//如果不是第一条消息就进行//
			{
				if (wstr_two.back() == L'\0')
				{
					wstr_two.pop_back();
				}
				fullmsg = wstr_two;

				fullmsg += L"\r\n";
			}
			fullmsg += wrecv_str;
			SetDlgItemText(hwndDlg,IDC_EDIT27,fullmsg.c_str());//将文本显示在对话框上//
			//清空文本发送区//
			//MessageBox(NULL, L"即将清空文本框", L"QQ", MB_ICONINFORMATION);
			SetDlgItemText(hwndDlg,IDC_EDIT28,L"");
			//EndDialog(hwndDlg, IDOK);//发送按钮//
			return (INT_PTR)TRUE;
		}
		case IDCANCEL:
		{
			std::wstring w_char = L"退出";
			int send_len = WideCharToMultiByte(CP_UTF8, 0, w_char.c_str(), w_char.size() + 1, NULL, 0, NULL, NULL);//先计算转换长度//
			std::string send_str(send_len, 0);//分配空间//
			WideCharToMultiByte(CP_UTF8, 0, w_char.c_str(), w_char.size() + 1, &send_str[0], send_len, NULL, NULL);//实际转换//
			send(g_clientSocket, (char*)&send_len, sizeof(send_len), 0);//先发长度//
			send(g_clientSocket, send_str.c_str(), send_len, 0);//再发内容//

			EndDialog(hwndDlg, IDCANCEL);//退出按钮//
			return (INT_PTR)TRUE;
		}
		}
	}
	return (INT_PTR)FALSE;
}

struct u
{
	std::vector <BYTE> img_data;
	std::string nickname;
	std::string inf;
};
std::vector<u> g_chartroom_inf;
std::mutex g_chartroom_mutex;



INT_PTR CALLBACK Dialog_twelve(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//聊天室对话框//
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{

		//禁用发送按钮
		HWND hSendBtn = GetDlgItem(hwndDlg, IDOK);
		EnableWindow(hSendBtn, false);

		// 清空原有消息
		{
			std::lock_guard<std::mutex> lk(g_chartroom_mutex);
			g_chartroom_inf.clear();
		}

		//请求加载聊天室已有的所有消息

		std::string u_str = "请求加载聊天室的消息";
		//发送请求
		int u_len = u_str.size();
		send(g_clientSocket, (char*)&u_len, sizeof(u_len), 0);
		send(g_clientSocket, u_str.c_str(), u_len, 0);

		//OutputDebugStringA(("u_str is " + u_str + ", and its len is " + std::to_string(u_len) + "\n").c_str());

		MessageBox(NULL, L"成功发送加载聊天室消息的请求", L"QQ", MB_ICONINFORMATION);

		int num = 0;
		recv(g_clientSocket, (char*)&num, sizeof(num), 0);
		MessageBox(NULL, L"成功接收聊天室消息的总条数", L"QQ", MB_ICONINFORMATION);

		if (num <= 0)
		{
			MessageBox(NULL, L"服务器的聊天室聊天记录为空", L"QQ", MB_ICONINFORMATION);
			return(INT_PTR)TRUE;
		}

		//接收聊天室原有聊天记录数据
		int i = 0;
		while (i < num)
		{
			i++;
			u u_inf;
			//接收用户图像数据
			int img_len = 0;
			recv(g_clientSocket, (char*)&img_len, sizeof(img_len), 0);
			int r = 0;
			int sum = 0;
			BYTE* img_data = new BYTE[img_len];
			while (sum < img_len)
			{
				r = recv(g_clientSocket, (char*)img_data + sum, img_len - sum, 0);
				if (r > 0)
				{
					sum += r;
				}
			}
			u_inf.img_data.insert(u_inf.img_data.end(), img_data, img_data + img_len);//存储图片数据
			delete[]img_data;//释放内存；

			//接收用户昵称
			int nickname_len = 0;
			recv(g_clientSocket, (char*)&nickname_len, sizeof(nickname_len), 0);
			std::string u_nickname(nickname_len, '\0');
			recv(g_clientSocket, &u_nickname[0], nickname_len, 0);
			u_inf.nickname = u_nickname;//存储用户昵称

			//接收用户在聊天室发送的消息
			int chart_inf_len = 0;
			recv(g_clientSocket, (char*)&chart_inf_len, sizeof(chart_inf_len), 0);
			std::string chart_inf_str(chart_inf_len, '\0');
			int x_r = 0;
			int x_sum = 0;
			while (x_sum < chart_inf_len)
			{
				x_r = recv(g_clientSocket, &chart_inf_str[0] + x_sum, chart_inf_len - x_sum, 0);
				if (x_r > 0)
				{
					x_sum += x_r;
				}
			}
			//recv(g_clientSocket, &chart_inf_str[0], chart_inf_len, 0);
			u_inf.inf = chart_inf_str;//存储用户消息

			//将消息压入队列
			std::lock_guard <std::mutex>lk(g_chartroom_mutex);
			g_chartroom_inf.push_back(u_inf);
		}

		//将接收的数据按照一定的格式显示在控件 IDC_EDIT29上
		//格式为头像图片数据+昵称+换行+消息

		// 在 Dialog_twelve 的 WM_INITDIALOG 末尾添加
		//SendDlgItemMessage(hwndDlg, IDC_EDIT29, LB_SETITEMHEIGHT, 0,18*100); // 每项48像素
		// 加载完g_chartroom_inf后，插入ListBox项
		HWND hList1 = GetDlgItem(hwndDlg, IDC_EDIT29);
		SendMessage(hList1, LB_RESETCONTENT, 0, 0);
		for (size_t i = 0; i < g_chartroom_inf.size(); ++i)
		{
			SendDlgItemMessage(hwndDlg, IDC_EDIT29, LB_ADDSTRING, 0, (LPARAM)L"");
		}

		return (INT_PTR)TRUE;
	}break;

	
	case WM_MEASUREITEM:
	{
		// 让每项高度自适应内容
		LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lParam;
		if (lpmis->CtlID == IDC_EDIT29)
		{
			int idx = lpmis->itemID;

			std::lock_guard<std::mutex> lk(g_chartroom_mutex);

			if (idx >= 0 && idx < (int)g_chartroom_inf.size())
			{
				HWND hList2 = GetDlgItem(hwndDlg, IDC_EDIT29);
				RECT rcListBox;
				GetClientRect(hList2, &rcListBox);
				int listBoxWidth = rcListBox.right - rcListBox.left;
				int avatarSize = 40;
				int leftPadding = 10;
				int rightPadding = 8;
				int contentWidth = listBoxWidth - (avatarSize + leftPadding + rightPadding);

				// 计算消息内容的高度
				const u& item = g_chartroom_inf[idx];
				std::wstring wmsg;
				int wlen = MultiByteToWideChar(CP_UTF8, 0, item.inf.c_str(), (int)item.inf.size(), NULL, 0);
				wmsg.resize(wlen);
				MultiByteToWideChar(CP_UTF8, 0, item.inf.c_str(), (int)item.inf.size(), &wmsg[0], wlen);

				HDC hdc = GetDC(hwndDlg);
				RECT rcMsg = { 0, 0,contentWidth, 0 }; // 留出头像和边距
				DrawTextW(hdc, wmsg.c_str(), (int)wmsg.size(), &rcMsg, DT_WORDBREAK | DT_CALCRECT);
				ReleaseDC(hwndDlg, hdc);

				int nicknameHeight = 18;
				int padding = 8;
				int minHeight = avatarSize +8;
				int contentHeight = rcMsg.bottom - rcMsg.top + nicknameHeight + padding;
				lpmis->itemHeight = max(minHeight, contentHeight); // 至少头像高度
			}
			else
			{
				lpmis->itemHeight = 48;
			}
			return TRUE;
		}
		break;
	}


	case WM_DRAWITEM:
	{
		LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
		if (lpdis->CtlID == IDC_EDIT29) 
		{
			int idx = lpdis->itemID;
			std::lock_guard<std::mutex> lk(g_chartroom_mutex);
			if (idx >= 0 && idx < (int)g_chartroom_inf.size())
			{
				HWND hList3 = GetDlgItem(hwndDlg, IDC_EDIT29);
				RECT rcListBox;
				GetClientRect(hList3, &rcListBox);
				int listBoxWidth = rcListBox.right - rcListBox.left;
				int avatarSize = 40;
				int leftPadding = 10;
				int rightPadding = 8;
				int contentWidth = listBoxWidth - (avatarSize + leftPadding + rightPadding);

				const u& item = g_chartroom_inf[idx];

				// 背景
				SetBkColor(lpdis->hDC, (lpdis->itemState & ODS_SELECTED) ? RGB(220, 240, 255) : RGB(255, 255, 255));
				ExtTextOut(lpdis->hDC, 0, 0, ETO_OPAQUE, &lpdis->rcItem, NULL, 0, NULL);

				// 头像
				if (!item.img_data.empty())
				{
					HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, item.img_data.size());
					void* pMem = GlobalLock(hMem);
					memcpy(pMem, item.img_data.data(), item.img_data.size());
					GlobalUnlock(hMem);
					IStream* pStream = NULL;
					if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) == S_OK)
					{
						Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromStream(pStream);
						if (bmp && bmp->GetLastStatus() == Gdiplus::Ok)
						{
							Gdiplus::Graphics g(lpdis->hDC);
							Gdiplus::Rect dest(lpdis->rcItem.left + 4, lpdis->rcItem.top + 4, avatarSize, avatarSize);
							g.DrawImage(bmp, dest);
							delete bmp;
						}
						pStream->Release();
					}
					GlobalFree(hMem);
				}

				// 昵称
				std::wstring wnickname;
				int wlen = MultiByteToWideChar(CP_UTF8, 0, item.nickname.c_str(), (int)item.nickname.size(), NULL, 0);
				wnickname.resize(wlen);
				MultiByteToWideChar(CP_UTF8, 0, item.nickname.c_str(), (int)item.nickname.size(), &wnickname[0], wlen);

				RECT rcNick = lpdis->rcItem;
				rcNick.left += avatarSize + 10;
				rcNick.top += 4;
				rcNick.bottom = rcNick.top + 18;
				SetTextColor(lpdis->hDC, RGB(0, 102, 204));
				DrawText(lpdis->hDC, wnickname.c_str(), (int)wnickname.size(), &rcNick, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

				// 消息内容
				std::wstring wmsg;
				wlen = MultiByteToWideChar(CP_UTF8, 0, item.inf.c_str(), (int)item.inf.size(), NULL, 0);
				wmsg.resize(wlen);
				MultiByteToWideChar(CP_UTF8, 0, item.inf.c_str(), (int)item.inf.size(), &wmsg[0], wlen);

				RECT rcMsg = lpdis->rcItem;
				rcMsg.left += avatarSize + 10;
				rcMsg.top += 24;
				rcMsg.right = rcMsg.left+contentWidth;
				SetTextColor(lpdis->hDC, RGB(0, 0, 0));
				DrawText(lpdis->hDC, wmsg.c_str(), (int)wmsg.size(), &rcMsg, DT_LEFT | DT_WORDBREAK);

				// 选中边框
				if (lpdis->itemState & ODS_SELECTED)
					DrawFocusRect(lpdis->hDC, &lpdis->rcItem);
			}
			return (INT_PTR)TRUE;
		}
	}break;

	case WM_COMMAND:
	{
		if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_EDIT30)
		{
			HWND hEdit = GetDlgItem(hwndDlg, IDC_EDIT30);
			HWND hSendBtn = GetDlgItem(hwndDlg, IDOK);
			int len = GetWindowTextLength(hEdit);
			EnableWindow(hSendBtn, len > 0);
		}
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			// 清空原有消息
			{
				std::lock_guard<std::mutex> lk(g_chartroom_mutex);
				g_chartroom_inf.clear();
			}

			std::string inf = "发送";
			int utf8_string_len = inf.size();
			send(g_clientSocket, (char*)&utf8_string_len, sizeof(utf8_string_len), 0);
			send(g_clientSocket, inf.c_str(), inf.size(), 0);

			//获取控件句柄
			HWND h = GetDlgItem(hwndDlg, IDC_EDIT30);
			//获取控件文本长度
			int text_len = GetWindowTextLength(h);
			if (text_len <= 0)
			{
				MessageBox(NULL, L"发送的消息不能为空", L"QQ", MB_ICONERROR);
				return (INT_PTR)TRUE;
			}
			std::wstring w_text(text_len + 1, L'\0');
			GetWindowText(h, &w_text[0], text_len + 1);//消息含'\0'
			//转换宽字符串
			int utf8_text_len = WideCharToMultiByte(CP_UTF8, 0, w_text.c_str(), text_len + 1, NULL, 0, NULL, NULL);
			std::string utf8_text_str(utf8_text_len, '\0');
			WideCharToMultiByte(CP_UTF8, 0, w_text.c_str(), text_len + 1, &utf8_text_str[0], utf8_text_len, NULL, NULL);
			//将文本消息发送
			send(g_clientSocket, (char*)&utf8_text_len, sizeof(utf8_text_len), 0);
			send(g_clientSocket, utf8_text_str.c_str(), utf8_text_len, 0);


			//将消息编辑框清空
			SetDlgItemText(hwndDlg, IDC_EDIT30, L"");
			//检测控件IDC_EDIT30是否有文本，若无则禁用发送按钮
			HWND hSendBtn = GetDlgItem(hwndDlg, IDOK);
			EnableWindow(hSendBtn, false);



			//接收服务器的更新通知
			int len = 0;
			recv(g_clientSocket, (char*)&len, sizeof(len), 0);
			std::string str(len, '\0');
			recv(g_clientSocket, &str[0], len, 0);
			if (strcmp(str.c_str(), "请更新聊天室消息") != 0)
			{
				MessageBox(NULL, L"未能根据服务器指令正确更新聊天室的消息", L"QQ", MB_ICONERROR);
			}

			//通知初始化消息再执行一次
			// 清空原有消息
			{
				std::lock_guard<std::mutex> lk(g_chartroom_mutex);
				g_chartroom_inf.clear();
			}

			//请求加载聊天室已有的所有消息

			std::string u_str = "请求加载聊天室的消息";
			//发送请求
			int u_len = u_str.size();
			send(g_clientSocket, (char*)&u_len, sizeof(u_len), 0);
			send(g_clientSocket, u_str.c_str(), u_len, 0);

			//OutputDebugStringA(("u_str is " + u_str + ", and its len is " + std::to_string(u_len) + "\n").c_str());

			MessageBox(NULL, L"成功发送加载聊天室消息的请求", L"QQ", MB_ICONINFORMATION);

			int num = 0;
			recv(g_clientSocket, (char*)&num, sizeof(num), 0);
			MessageBox(NULL, L"成功接收聊天室消息的总条数", L"QQ", MB_ICONINFORMATION);

			if (num <= 0)
			{
				MessageBox(NULL, L"服务器的聊天室聊天记录为空", L"QQ", MB_ICONINFORMATION);
				return(INT_PTR)TRUE;
			}

			//接收聊天室原有聊天记录数据
			int i = 0;
			while (i < num)
			{
				i++;
				u u_inf;
				//接收用户图像数据
				int img_len = 0;
				recv(g_clientSocket, (char*)&img_len, sizeof(img_len), 0);
				int r = 0;
				int sum = 0;
				BYTE* img_data = new BYTE[img_len];
				while (sum < img_len)
				{
					r = recv(g_clientSocket, (char*)img_data + sum, img_len - sum, 0);
					if (r > 0)
					{
						sum += r;
					}
				}
				u_inf.img_data.insert(u_inf.img_data.end(), img_data, img_data + img_len);//存储图片数据
				delete[]img_data;//释放内存；

				//接收用户昵称
				int nickname_len = 0;
				recv(g_clientSocket, (char*)&nickname_len, sizeof(nickname_len), 0);
				std::string u_nickname(nickname_len, '\0');
				recv(g_clientSocket, &u_nickname[0], nickname_len, 0);
				u_inf.nickname = u_nickname;//存储用户昵称

				//接收用户在聊天室发送的消息
				int chart_inf_len = 0;
				recv(g_clientSocket, (char*)&chart_inf_len, sizeof(chart_inf_len), 0);
				std::string chart_inf_str(chart_inf_len, '\0');
				recv(g_clientSocket, &chart_inf_str[0], chart_inf_len, 0);
				u_inf.inf = chart_inf_str;//存储用户消息

				OutputDebugStringA(("the information user send in chartroom is"+chart_inf_str+"\n").c_str());

				//将消息压入队列
				std::lock_guard <std::mutex>lk(g_chartroom_mutex);
				g_chartroom_inf.push_back(u_inf);
			}

			//将接收的数据按照一定的格式显示在控件 IDC_EDIT29上
			//格式为头像图片数据+昵称+换行+消息

			// 在 Dialog_twelve 的 WM_INITDIALOG 末尾添加
			//SendDlgItemMessage(hwndDlg, IDC_EDIT29, LB_SETITEMHEIGHT, 0, 18*100); // 每项48像素
			// 加载完g_chartroom_inf后，插入ListBox项
			HWND hList4 = GetDlgItem(hwndDlg, IDC_EDIT29);
			SendMessage(hList4, LB_RESETCONTENT, 0, 0);
			for (size_t i = 0; i < g_chartroom_inf.size(); ++i)
			{
				SendDlgItemMessage(hwndDlg, IDC_EDIT29, LB_ADDSTRING, 0, (LPARAM)L"");
			}

			return (INT_PTR)TRUE;
		}


		case IDC_UPDATEINFORMATION:
		{
			// 清空原有消息
			{
				std::lock_guard<std::mutex> lk(g_chartroom_mutex);
				g_chartroom_inf.clear();
			}

			std::string inf = "更新消息";
			int utf8_string_len = inf.size();
			send(g_clientSocket, (char*)&utf8_string_len, sizeof(utf8_string_len), 0);
			send(g_clientSocket, inf.c_str(), inf.size(), 0);

			//请求加载聊天室已有的所有消息

			std::string u_str = "请求加载聊天室的消息";
			//发送请求
			int u_len = u_str.size();
			send(g_clientSocket, (char*)&u_len, sizeof(u_len), 0);
			send(g_clientSocket, u_str.c_str(), u_len, 0);

			//OutputDebugStringA(("u_str is " + u_str + ", and its len is " + std::to_string(u_len) + "\n").c_str());

			MessageBox(NULL, L"成功发送加载聊天室消息的请求", L"QQ", MB_ICONINFORMATION);

			int num = 0;
			recv(g_clientSocket, (char*)&num, sizeof(num), 0);
			MessageBox(NULL, L"成功接收聊天室消息的总条数", L"QQ", MB_ICONINFORMATION);

			if (num <= 0)
			{
				MessageBox(NULL, L"服务器的聊天室聊天记录为空", L"QQ", MB_ICONINFORMATION);
				return(INT_PTR)TRUE;
			}

			//接收聊天室原有聊天记录数据
			int i = 0;
			while (i < num)
			{
				i++;
				u u_inf;
				//接收用户图像数据
				int img_len = 0;
				recv(g_clientSocket, (char*)&img_len, sizeof(img_len), 0);
				int r = 0;
				int sum = 0;
				BYTE* img_data = new BYTE[img_len];
				while (sum < img_len)
				{
					r = recv(g_clientSocket, (char*)img_data + sum, img_len - sum, 0);
					if (r > 0)
					{
						sum += r;
					}
				}
				u_inf.img_data.insert(u_inf.img_data.end(), img_data, img_data + img_len);//存储图片数据
				delete[]img_data;//释放内存；

				//接收用户昵称
				int nickname_len = 0;
				recv(g_clientSocket, (char*)&nickname_len, sizeof(nickname_len), 0);
				std::string u_nickname(nickname_len, '\0');
				recv(g_clientSocket, &u_nickname[0], nickname_len, 0);
				u_inf.nickname = u_nickname;//存储用户昵称

				//接收用户在聊天室发送的消息
				int chart_inf_len = 0;
				recv(g_clientSocket, (char*)&chart_inf_len, sizeof(chart_inf_len), 0);
				std::string chart_inf_str(chart_inf_len, '\0');
				recv(g_clientSocket, &chart_inf_str[0], chart_inf_len, 0);
				u_inf.inf = chart_inf_str;//存储用户消息

				OutputDebugStringA(("the information user send in chartroom is" + chart_inf_str + "\n").c_str());

				//将消息压入队列
				std::lock_guard <std::mutex>lk(g_chartroom_mutex);
				g_chartroom_inf.push_back(u_inf);
			}

			//将接收的数据按照一定的格式显示在控件 IDC_EDIT29上
			//格式为头像图片数据+昵称+换行+消息

			// 在 Dialog_twelve 的 WM_INITDIALOG 末尾添加
			//SendDlgItemMessage(hwndDlg, IDC_EDIT29, LB_SETITEMHEIGHT, 0, 18*100); // 每项48像素
			// 加载完g_chartroom_inf后，插入ListBox项
			HWND hList5 = GetDlgItem(hwndDlg, IDC_EDIT29);
			SendMessage(hList5, LB_RESETCONTENT, 0, 0);
			for (size_t i = 0; i < g_chartroom_inf.size(); ++i)
			{
				SendDlgItemMessage(hwndDlg, IDC_EDIT29, LB_ADDSTRING, 0, (LPARAM)L"");
			}

			return (INT_PTR)TRUE;
		}
		case IDCANCEL:
		{
			std::string inf = "退出";
			int utf8_string_len = inf.size();
			send(g_clientSocket, (char*)&utf8_string_len, sizeof(utf8_string_len), 0);
			send(g_clientSocket, inf.c_str(), inf.size(), 0);

			MessageBox(NULL, L"成功发送退出按钮（WM_COMMAND）", L"QQ", MB_ICONINFORMATION);
			EndDialog(hwndDlg, IDCANCEL);
			return (INT_PTR)TRUE;
		}
		}
	}
	/*
	case WM_CLOSE:
	{
		std::string inf = "退出";
		int utf8_string_len = inf.size();
		send(g_clientSocket, (char*)&utf8_string_len, sizeof(utf8_string_len), 0);
		send(g_clientSocket, inf.c_str(), inf.size(), 0);

		MessageBox(NULL, L"成功发送退出按钮（WM_CLOSE）", L"QQ", MB_ICONERROR);
		EndDialog(hwndDlg, IDCANCEL);
		return (INT_PTR)TRUE;
	}
	*/
	}
	
	return (INT_PTR)FALSE;
}

//记录用户好友的账号、昵称和头像
struct user_friend
{
	int signal;
	std::string account;
	std::string nickname;
	std::vector<BYTE> image;
};
std::vector <user_friend> g_user_friends;


//记录用户好友的账号、昵称和头像
struct user_half_friend
{
	std::string account;
	std::string nickname;
	std::vector<BYTE> image;
};
std::vector <user_half_friend> g_user_half_friends;


INT_PTR CALLBACK Dialog_seventeen(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//新朋友列表框
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		//向服务器发送加载新朋友列表的请求
		std::string request_to_server = "客户端请求加载新朋友列表";
		int request_len = request_to_server.size();
		send(g_clientSocket, (char*)&request_len,sizeof(request_len),0);
		send(g_clientSocket, request_to_server.c_str(), request_len, 0);

		char* buf_x1 = new char[50]();
		snprintf(buf_x1, 50, "the request_to_server is %s\n",request_to_server.c_str());
		OutputDebugStringA(buf_x1);
		delete[]buf_x1;

		//接收服务器返回的消息

		int r_len = 0;
		recv(g_clientSocket, (char*)&r_len, sizeof(r_len), 0);
		std::string s_str(r_len, '\0');
		recv(g_clientSocket, &s_str[0], r_len, 0);

		char* buf_x2 = new char[50]();
		snprintf(buf_x2, 50, "the s_str is %s\n", s_str.c_str());
		OutputDebugStringA(buf_x2);
		delete[]buf_x2;

		//比较接受的字符串
		if (strcmp("success",s_str.c_str()) == 0)
		{
			MessageBox(NULL,L"服务器已经成功加载新朋友列表",L"QQ",MB_ICONINFORMATION);
		}
		else if(strcmp("failure", s_str.c_str()) == 0)
		{
			MessageBox(NULL, L"服务器无法加载新朋友列表,即将返回好友框", L"QQ", MB_ICONERROR);
			//初始化失败，关闭对话框，返回好友框
			EndDialog(hwndDlg,IDCANCEL);
			return (INT_PTR)TRUE;
		}

		//接收新朋友条数
		int half_len = 0;
		recv(g_clientSocket, (char*)&half_len, sizeof(half_len), 0);

		//条数为0，初始化结束
		if (half_len == 0)
		{
			MessageBox(NULL, L"您暂时还没有新朋友，即将返回好友框", L"QQ", MB_ICONINFORMATION);
			EndDialog(hwndDlg, IDCANCEL);
			return(INT_PTR)TRUE;
		}

		//条数不为0，向系统发送列表项数请求
		else if(half_len>0)
		{
			MessageBox(NULL, L"即将接受来自服务器的新朋友数据", L"QQ", MB_ICONINFORMATION);


			//接收好友列表的消息集
			int i = 0;
			g_user_half_friends.clear();//确保好友消息队列清空
			user_half_friend u_x;
			while (i < half_len)
			{
				u_x= user_half_friend();//清空结构体

				//接收好友账号
				int account_x_len = 0;
				recv(g_clientSocket, (char*)&account_x_len, sizeof(account_x_len), 0);
				u_x.account.resize(account_x_len);//重新分配空间
				recv(g_clientSocket, &u_x.account[0], account_x_len, 0);

				char* buf = new char[50]();
				snprintf(buf, 50, "the friend_account is %s\n", u_x.account.c_str());
				OutputDebugStringA(buf);
				delete[]buf;

				//接收好友昵称
				int nickname_x_len = 0;
				recv(g_clientSocket, (char*)&nickname_x_len, sizeof(nickname_x_len), 0);
				u_x.nickname.resize(nickname_x_len);//重新分配空间
				recv(g_clientSocket, &u_x.nickname[0], nickname_x_len, 0);

				char* buf_2 = new char[50]();
				snprintf(buf_2, 50, "the friend_nickname is %s\n", u_x.nickname.c_str());
				OutputDebugStringA(buf_2);
				delete[]buf_2;

				//接收好友头像
				int image_x_len = 0;
				recv(g_clientSocket, (char*)&image_x_len, sizeof(image_x_len), 0);


				char* buf_3 = new char[50]();
				snprintf(buf_3, 50, "the friend_photo_len is %d\n", image_x_len);
				OutputDebugStringA(buf_3);
				delete[]buf_3;


				u_x.image.resize(image_x_len);//重新分配空间
				int  u_r = 0;
				int u_sum = 0;
				while (u_sum < image_x_len)
				{
					u_r = recv(g_clientSocket, (char*)u_x.image.data() + u_sum, image_x_len - u_sum, 0);
					if (u_r > 0)
					{
						u_sum += u_r;
					}
				}
				//接收完毕，将好友的信息压入弹夹
				g_user_half_friends.push_back(u_x);

				i++;
			}

			MessageBox(NULL, L"已经成功接收用户的好友消息", L"QQ", MB_ICONINFORMATION);

			HWND hList6 = GetDlgItem(hwndDlg, IDC_EDIT49);
			SendMessage(hList6, LB_RESETCONTENT, 0, 0);//清空列表框
			for (size_t i = 0; i < g_user_half_friends.size(); ++i)
			{
				SendDlgItemMessage(hwndDlg, IDC_EDIT49, LB_ADDSTRING, 0, (LPARAM)L"");
				SendDlgItemMessage(hwndDlg, IDC_EDIT49, LB_SETITEMHEIGHT, i, 48); // 每项48像素
			}
			
			SendMessage(hList6, LB_SETSEL, FALSE, -1);//取消所有选中项//
			LONG style = GetWindowLong(hList6, GWL_STYLE);//获取当前控件的样式//
			style &= ~(LBS_SINGLESEL);//清除LBS_OWNERDRAWFIXED样式//
			style |= LBS_MULTIPLESEL;  // 增加多选，支持Ctrl/Shift
			SetWindowLong(hList6, GWL_STYLE, style); // 应用新样式

			HWND hBtn_y = GetDlgItem(hwndDlg, IDOK); //播按钮句柄//
			EnableWindow(hBtn_y, FALSE); // 初始禁用同意按钮

			HWND hBtn_x = GetDlgItem(hwndDlg, IDC_REJECT); //播按钮句柄//
			EnableWindow(hBtn_x, FALSE); // 初始禁用拒绝按钮

			return (INT_PTR)TRUE;
		}

	}break;

	case WM_DRAWITEM:
	{
		LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
		if (lpdis->CtlID == IDC_EDIT49)
		{
			int idx = lpdis->itemID;
			if (idx >= 0 && idx < (int)g_user_half_friends.size())
			{
				HWND hList7 = GetDlgItem(hwndDlg, IDC_EDIT49);
				RECT rcListBox;
				GetClientRect(hList7, &rcListBox);
				int listBoxWidth = rcListBox.right - rcListBox.left;
				int avatarSize = 40;
				int leftPadding = 10;
				int rightPadding = 8;
				int contentWidth = listBoxWidth - (avatarSize + leftPadding + rightPadding);

				const user_half_friend& item = g_user_half_friends[idx];

				// 背景
				SetBkColor(lpdis->hDC, (lpdis->itemState & ODS_SELECTED) ? RGB(230, 230, 250) : RGB(255, 255, 255));
				ExtTextOut(lpdis->hDC, 0, 0, ETO_OPAQUE, &lpdis->rcItem, NULL, 0, NULL);

				// 头像
				if (!item.image.empty())
				{
					HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, item.image.size());
					void* pMem = GlobalLock(hMem);
					memcpy(pMem, item.image.data(), item.image.size());
					GlobalUnlock(hMem);
					IStream* pStream = NULL;
					if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) == S_OK)
					{
						Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromStream(pStream);
						if (bmp && bmp->GetLastStatus() == Gdiplus::Ok)
						{
							Gdiplus::Graphics g(lpdis->hDC);
							Gdiplus::Rect dest(lpdis->rcItem.left + 4, lpdis->rcItem.top + 4, avatarSize, avatarSize);
							g.DrawImage(bmp, dest);
							delete bmp;
						}
						pStream->Release();
					}
					GlobalFree(hMem);
				}

				// 昵称
				std::wstring wnickname;
				int wlen = MultiByteToWideChar(CP_UTF8, 0, item.nickname.c_str(), (int)item.nickname.size(), NULL, 0);
				wnickname.resize(wlen);
				MultiByteToWideChar(CP_UTF8, 0, item.nickname.c_str(), (int)item.nickname.size(), &wnickname[0], wlen);

				RECT rcNick = lpdis->rcItem;
				rcNick.left += avatarSize + 10;
				rcNick.top += 4;
				rcNick.bottom = rcNick.top + 18;
				SetTextColor(lpdis->hDC, RGB(0, 102, 204));
				//DrawText(lpdis->hDC, wnickname.c_str(), (int)wnickname.size(), &rcNick, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_CALCRECT);
				DrawText(lpdis->hDC, wnickname.c_str(), (int)wnickname.size(), &rcNick, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
			


				// 账号内容
				std::wstring waccount;
				wlen = MultiByteToWideChar(CP_UTF8, 0, item.account.c_str(), (int)item.account.size(), NULL, 0);
				waccount.resize(wlen);
				MultiByteToWideChar(CP_UTF8, 0, item.account.c_str(), (int)item.account.size(), &waccount[0], wlen);

				RECT rcAcc = lpdis->rcItem;
				rcAcc.left += avatarSize + 10;
				rcAcc.top += 24;
				rcAcc.bottom = rcAcc.top + 18;
				SetTextColor(lpdis->hDC, RGB(0, 0, 0));
				DrawText(lpdis->hDC, waccount.c_str(), (int)waccount.size(), &rcAcc, DT_LEFT | DT_SINGLELINE);

				// 选中边框
				if (lpdis->itemState & ODS_FOCUS)
					DrawFocusRect(lpdis->hDC, &lpdis->rcItem);
			}

			return (INT_PTR)TRUE;
		}
	}break;


	case WM_COMMAND:
	{
		HWND hList8 = GetDlgItem(hwndDlg, IDC_EDIT49);

		HWND hBtn1 = GetDlgItem(hwndDlg, IDOK);
		HWND hBtn2 = GetDlgItem(hwndDlg, IDC_REJECT);
		if (HIWORD(wParam) == LBN_SELCHANGE && LOWORD(wParam) == IDC_EDIT49)
		{
			int selCount = (int)SendMessage(hList8, LB_GETSELCOUNT, 0, 0);
			EnableWindow(hBtn1, selCount > 0 ? TRUE : FALSE);
			EnableWindow(hBtn2, selCount > 0 ? TRUE : FALSE);
		}

		switch (LOWORD(wParam))
		{

		case IDOK:
		{
			std::vector <std::string> new_friend_account;
			new_friend_account.clear();
			
			int selCount_x = (int)SendMessage(hList8, LB_GETSELCOUNT, 0, 0);

			char* buf_111 = new char[50];
			snprintf(buf_111,strlen(buf_111),"the selCount_x is %d\n",selCount_x);
			OutputDebugStringA(buf_111);
			delete[]buf_111;

			if (selCount_x > 0)
			{
				std::vector<int> selItems(selCount_x, 0);
				SendMessage(hList8, LB_GETSELITEMS, selCount_x, (LPARAM)selItems.data());

				for (int i = 0; i < selCount_x; ++i)
				{
					int itemIdx = selItems[i];
					if (itemIdx < 0 || itemIdx >= (int)g_user_half_friends.size())
					{
						continue;
					}
					const std::string& account = g_user_half_friends[itemIdx].account;
					
					char* buf_112 = new char[50];
					snprintf(buf_112, strlen(buf_112), "the g_user_half_friends[%d].account is %s\n",i,account.c_str());
					OutputDebugStringA(buf_112);
					delete[]buf_112;

					new_friend_account.push_back(account);//将全部选中的用户账号存储

				}
			}

			//SendMessage(hList, LB_SETSEL, FALSE, -1);//取消所有选中项//
			
			//向服务器发送确定消息
			std::string buf = "同意";
			int buf_len = buf.size();
			send(g_clientSocket, (char*)&buf_len, sizeof(buf_len), 0);
			send(g_clientSocket, buf.c_str(), buf_len, 0);

			//先发送同意的新朋友数目
			send(g_clientSocket, (char*)&selCount_x, sizeof(selCount_x), 0);

			//向服务器发送经用户同意的新朋友账号
			for (auto it=new_friend_account.begin();it!=new_friend_account.end();)
			{
				std::string x = *it;

				int s_l = x.size();
				send(g_clientSocket, (char*)&s_l, sizeof(s_l), 0);
                send(g_clientSocket, x.c_str(), s_l, 0);
				
				char* buf_115 = new char[50];
				snprintf(buf_115, strlen(buf_115), "the x is %s\n",x.c_str());
				OutputDebugStringA(buf_115);
				delete[]buf_115;

				it++;
			}

			int cc_l = 0;
			recv(g_clientSocket, (char*)&cc_l, sizeof(cc_l), 0);
			std::string cc(cc_l, '\0');
			recv(g_clientSocket, &cc[0], cc_l, 0);

			if (strcmp("success", cc.c_str()) == 0)
			{
				MessageBox(NULL, L"服务器已经更新您的好友列表和新朋友列表，即将回到好友框", L"QQ", MB_ICONINFORMATION);
				EndDialog(hwndDlg,IDOK);
				return(INT_PTR)TRUE;
			}
			return (INT_PTR)TRUE;
		}

		case IDC_REJECT:
		{
			
			std::vector <std::string> new_friend_account;
			new_friend_account.clear();

			int selCount_x = (int)SendMessage(hList8, LB_GETSELCOUNT, 0, 0);

			char* buf_111 = new char[50];
			snprintf(buf_111, strlen(buf_111), "the selCount_x is %d\n", selCount_x);
			OutputDebugStringA(buf_111);
			delete[]buf_111;

			if (selCount_x > 0)
			{
				std::vector<int> selItems(selCount_x, 0);
				SendMessage(hList8, LB_GETSELITEMS, selCount_x, (LPARAM)selItems.data());

				for (int i = 0; i < selCount_x; ++i)
				{
					int itemIdx = selItems[i];
					if (itemIdx < 0 || itemIdx >= (int)g_user_half_friends.size())
					{
						continue;
					}
					const std::string& account = g_user_half_friends[itemIdx].account;

					char* buf_112 = new char[50];
					snprintf(buf_112, strlen(buf_112), "the g_user_half_friends[%d].account is %s\n", i, account.c_str());
					OutputDebugStringA(buf_112);
					delete[]buf_112;

					new_friend_account.push_back(account);//将全部选中的用户账号存储

				}
			}

			//SendMessage(hList, LB_SETSEL, FALSE, -1);//取消所有选中项//

			//向服务器发送确定消息
			std::string buf = "拒绝";
			int buf_len = buf.size();
			send(g_clientSocket, (char*)&buf_len, sizeof(buf_len), 0);
			send(g_clientSocket, buf.c_str(), buf_len, 0);

			//先发送同意的新朋友数目
			send(g_clientSocket, (char*)&selCount_x, sizeof(selCount_x), 0);

			//向服务器发送经用户同意的新朋友账号
			for (auto it = new_friend_account.begin(); it != new_friend_account.end();)
			{
				std::string x = *it;

				int s_l = x.size();
				send(g_clientSocket, (char*)&s_l, sizeof(s_l), 0);
				send(g_clientSocket, x.c_str(), s_l, 0);

				char* buf_115 = new char[50];
				snprintf(buf_115, strlen(buf_115), "the x is %s\n", x.c_str());
				OutputDebugStringA(buf_115);
				delete[]buf_115;

				it++;
			}

			int cc_l = 0;
			recv(g_clientSocket, (char*)&cc_l, sizeof(cc_l), 0);
			std::string cc(cc_l, '\0');
			recv(g_clientSocket, &cc[0], cc_l, 0);

			if (strcmp("success", cc.c_str()) == 0)
			{
				MessageBox(NULL, L"服务器已经更新您的好友列表和新朋友列表，即将回到好友框", L"QQ", MB_ICONINFORMATION);
				EndDialog(hwndDlg, IDOK);
				return(INT_PTR)TRUE;
			}
			return (INT_PTR)TRUE;

		}

		case IDCANCEL:
		{
			//向服务器发送确定消息
			std::string buf = "取消";
			int buf_len = buf.size();
			send(g_clientSocket, (char*)&buf_len, sizeof(buf_len), 0);
			send(g_clientSocket, buf.c_str(), buf_len, 0);

			EndDialog(hwndDlg, IDCANCEL);
			return(INT_PTR)TRUE;

		}
		}
	}break;


    }

	return (INT_PTR)FALSE;
}


INT_PTR CALLBACK Dialog_sixteen(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//添加好友账号输入框
{
	switch (uMsg)
	{

	case WM_INITDIALOG:
	{
		SetDlgItemText(hwndDlg, IDC_EDIT47, L"请输入好友的账号");
		//禁用确定按钮
		HWND hSendBtn = GetDlgItem(hwndDlg, IDOK);
		EnableWindow(hSendBtn, false);
		return (INT_PTR)TRUE;

	}break;

	case WM_COMMAND:
	{
		if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_EDIT48)
		{
			HWND hEdit = GetDlgItem(hwndDlg, IDC_EDIT48);
			HWND hSendBtn = GetDlgItem(hwndDlg, IDOK);
			int len = GetWindowTextLength(hEdit);
			EnableWindow(hSendBtn, len > 0);
		}

		switch (LOWORD(wParam))
		{

		case IDOK:
		{
			//获取控件IDC_EDIT48的文本
			HWND h = GetDlgItem(hwndDlg, IDC_EDIT48);
			//获取文本长度
			int w_text_len = GetWindowTextLength(h);
			if (w_text_len < 0)
			{
				return (INT_PTR)TRUE;
			}

			//向服务器发送确定消息
			std::string buf = "确定";
			int buf_len = buf.size();
			send(g_clientSocket, (char*)&buf_len, sizeof(buf_len), 0);
			send(g_clientSocket, buf.c_str(), buf_len, 0);

		
			std::wstring w_text(w_text_len+1,L'\0');
			GetWindowText(h, &w_text[0], w_text_len+1);

			if (!w_text.empty() && w_text.back() == L'\0')
			{
				w_text.pop_back();
			}

			//将宽字符串转换
			int utf8_len = 0;
			utf8_len=WideCharToMultiByte(CP_UTF8,0,w_text.c_str(),w_text_len,NULL,0,NULL,NULL);
			std::string utf8_str(utf8_len,'\0');
			WideCharToMultiByte(CP_UTF8, 0, w_text.c_str(), w_text_len, &utf8_str[0],utf8_len, NULL, NULL);

			//向服务器发送好友账号进行验证
			send(g_clientSocket, (char*)&utf8_len, sizeof(utf8_len), 0);
			send(g_clientSocket, utf8_str.c_str(), utf8_len, 0);

			//清空账号输入框
			SetDlgItemText(hwndDlg,IDC_EDIT48,L"");

			//接收服务器该好友账号是否存在的消息
			
			int x_len = 0;

			recv(g_clientSocket,(char*)&x_len, sizeof(x_len), 0);
			std::string x_str(x_len, '\0');
			recv(g_clientSocket, &x_str[0], x_len, 0);

			if(strcmp("添加的好友账号存在",x_str.c_str()) == 0)
			//若存在
			{
				MessageBox(NULL, L"添加的好友账号存在,已成功发送添加好友的请求", L"QQ", MB_ICONINFORMATION);
				//关闭对话框，返回好友框
				EndDialog(hwndDlg,IDOK);
				return (INT_PTR)TRUE;
			}

			else if (strcmp("你们已经是好友，无需在发送好友请求", x_str.c_str()) == 0)
				//若存在
			{
				MessageBox(NULL, L"你们已经是好友，无需在发送好友请求", L"QQ", MB_ICONINFORMATION);
				//关闭对话框，返回好友框
				EndDialog(hwndDlg, IDOK);
				return (INT_PTR)TRUE;
			}

			else if (strcmp("已经发送过好友请求，请耐心等待", x_str.c_str()) == 0)
				//若存在
			{
				MessageBox(NULL, L"已经发送过好友请求，请耐心等待", L"QQ", MB_ICONINFORMATION);
				//关闭对话框，返回好友框
				EndDialog(hwndDlg, IDOK);
				return (INT_PTR)TRUE;
			}

			else if (strcmp("添加的好友账号不存在", x_str.c_str()) == 0)
			//若不存在
			{
				MessageBox(NULL, L"添加的好友账号不存在", L"QQ", MB_ICONERROR);
				MessageBox(NULL, L"请重新输入好友的账号", L"QQ", MB_ICONINFORMATION);
				return (INT_PTR)TRUE;
			}

		}break;

		case IDCANCEL:
		{
			//向服务器发送确定消息
			std::string buf = "取消";
			int buf_len = buf.size();
			send(g_clientSocket, (char*)&buf_len, sizeof(buf_len), 0);
			send(g_clientSocket, buf.c_str(), buf_len, 0);

		   //退出添加好友账号输入框
			EndDialog(hwndDlg,IDCANCEL);
			return (INT_PTR)TRUE;

		}break;

		}
	}break;

	}

	return (INT_PTR)FALSE;
}

//转换字符串为宽字符串
std::wstring strTowstr(std::string str)
{
	int w_len = MultiByteToWideChar(CP_UTF8,0,str.c_str(),str.size(),NULL,0);
	std::wstring wstr(w_len, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.size(), &wstr[0], w_len);
	return wstr;
}


INT_PTR CALLBACK  Dialog_eighteen(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//好友信息框//
{

	switch (uMsg)
	{

	case WM_INITDIALOG:
	{

		SetDlgItemText(hwndDlg, IDC_EDIT50,L"账号");
		SetDlgItemText(hwndDlg, IDC_EDIT51, L"昵称");
		SetDlgItemText(hwndDlg, IDC_EDIT52, L"性别");
		SetDlgItemText(hwndDlg, IDC_EDIT53, L"年龄");
		SetDlgItemText(hwndDlg, IDC_EDIT54, L"个性签名");

		SetDlgItemText(hwndDlg, IDC_EDIT55,strTowstr(xxy.account).c_str());
		SetDlgItemText(hwndDlg, IDC_EDIT56, strTowstr(xxy.nickname).c_str());
		SetDlgItemText(hwndDlg, IDC_EDIT57, strTowstr(xxy.gender).c_str());
		SetDlgItemText(hwndDlg, IDC_EDIT58, strTowstr(xxy.age).c_str());
		SetDlgItemText(hwndDlg, IDC_EDIT59, strTowstr(xxy.signature).c_str());

		return (INT_PTR)TRUE;

	}break;

	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{

		case IDCANCEL:
		{
		   //向服务器发送取消按钮
			std::string sd = "取消";
			int sd_len = sd.size();
			send(g_clientSocket,(char*)&sd_len,sizeof(sd_len),0);
			send(g_clientSocket,sd.c_str(),sd_len,0);

			//关闭好友消息展示框
			EndDialog(hwndDlg,IDCANCEL);
			return (INT_PTR)TRUE;

		}break;

		}

	}break;

	}

	return (INT_PTR)FALSE;
}

struct users_chart_information
{
	std::string inf_send_account;
	std::string inf_recv_account;
	std::string inf;
};
std::vector<users_chart_information>g_users_chart_information;
users_chart_information ss;


INT_PTR CALLBACK Dialog_nineteen(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//好友聊天对话框//
{
	
	switch (uMsg)
	{
	
	case WM_INITDIALOG:
	{
		//禁用发送按钮
		HWND hBton1 = GetDlgItem(hwndDlg,IDOK);
		EnableWindow(hBton1,FALSE);
		//清空文本框
		SetDlgItemText(hwndDlg,IDC_EDIT61,L"");

		//需要加载先前的聊天记录
		
		//向服务器发送加载先前聊天记录的请求
		std::string r_str = "请求更新聊天框消息";
		int r_str_len = r_str.size();
		send(g_clientSocket, (char*)&r_str_len, sizeof(r_str_len), 0);
		send(g_clientSocket, r_str.c_str(), r_str_len, 0);

		char* buf = new char[50];
		snprintf(buf, 50, "the r_str is %s\n", r_str.c_str());
		OutputDebugStringA(buf);
		delete[]buf;

		//接收总的聊天框消息条数
		int msg_len = 0;
		recv(g_clientSocket, (char*)&msg_len, sizeof(msg_len), 0);

		char* buf2 = new char[50];
		snprintf(buf2, 50, "the msg_len is %d\n", msg_len);
		OutputDebugStringA(buf2);
		delete[]buf2;

		//如果条数为0
		if (msg_len <= 0)
		{
			//初始化结束
			return (INT_PTR)TRUE;
		}
		else
		{
			int i = 0;
			g_users_chart_information.clear();

			//接收发送的消息
			while (i < msg_len)
			{
				ss = users_chart_information();//清空结构体

				//接收消息发送方账号
				int account_send_len = 0;
				recv(g_clientSocket, (char*)&account_send_len, sizeof(account_send_len), 0);

				char* buf13 = new char[50];
				snprintf(buf13, 50, "the  account_send_len is %d\n", account_send_len);
				OutputDebugStringA(buf13);
				delete[]buf13;

				ss.inf_send_account.resize(account_send_len);
				recv(g_clientSocket,&ss.inf_send_account[0],account_send_len,0);

				char* buf6 = new char[50];
				snprintf(buf6, 50, "the  ss.inf_send_account is %s\n", ss.inf_send_account.c_str());
				OutputDebugStringA(buf6);
				delete[]buf6;

				//接收消息接收方账号
				int account_recv_len = 0;
				recv(g_clientSocket, (char*)&account_recv_len, sizeof(account_recv_len), 0);

				char* buf14 = new char[50];
				snprintf(buf14, 50, "the  account_recv_len is %d\n", account_recv_len);
				OutputDebugStringA(buf14);
				delete[]buf14;

				ss.inf_recv_account.resize(account_recv_len);
				recv(g_clientSocket, &ss.inf_recv_account[0], account_recv_len, 0);

				char* buf7 = new char[50];
				snprintf(buf7, 50, "the  ss.inf_recv_account is %s\n", ss.inf_recv_account.c_str());
				OutputDebugStringA(buf7);
				delete[]buf7;


				//消息
				int inf_len = 0;
				recv(g_clientSocket, (char*)&inf_len, sizeof(inf_len), 0);
				
				char* buf45 = new char[50];
				snprintf(buf45, 50, "the  inf_len is %d\n", inf_len);
				OutputDebugStringA(buf45);
				delete[]buf45;

				ss.inf.resize(inf_len);
				recv(g_clientSocket, &ss.inf[0], inf_len, 0);

				char* buf8 = new char[50];
				snprintf(buf8, 50, "the  ss.inf is %s\n", ss.inf.c_str());
				OutputDebugStringA(buf8);
				delete[]buf8;

				//压入队列
				g_users_chart_information.push_back(ss);

				i++;
			}


			//以一定的格式显示聊天记录
			
			
			//清空聊天框
			HWND n = GetDlgItem(hwndDlg,IDC_EDIT60);
			SendMessage(n, LB_RESETCONTENT, 0, 0);
			for (int k = 0; k < msg_len;k++)
			{
				SendDlgItemMessage(hwndDlg, IDC_EDIT60, LB_ADDSTRING, 0, (LPARAM)L"");
			}
			
		}

	}break;

	
	case WM_MEASUREITEM:
	{
		// 让每项高度自适应内容
		LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lParam;
		if (lpmis->CtlID == IDC_EDIT60)
		{
			int idx = lpmis->itemID;

			if (idx >= 0 && idx < (int)g_users_chart_information.size())
			{
				HWND hList9 = GetDlgItem(hwndDlg, IDC_EDIT60);
				RECT rcListBox;
				GetClientRect(hList9, &rcListBox);
				int listBoxWidth = rcListBox.right - rcListBox.left;
				int avatarSize = 40;
				int leftPadding = 10;
				int rightPadding = 8;
				int contentWidth = listBoxWidth - (avatarSize + leftPadding + rightPadding);

				// 计算消息内容的高度
				const users_chart_information& item = g_users_chart_information[idx];
				std::wstring wmsg;
				int wlen = MultiByteToWideChar(CP_UTF8, 0, item.inf.c_str(), (int)item.inf.size(), NULL, 0);
				wmsg.resize(wlen);
				MultiByteToWideChar(CP_UTF8, 0, item.inf.c_str(), (int)item.inf.size(), &wmsg[0], wlen);

				HDC hdc = GetDC(hwndDlg);
				RECT rcMsg = { 0, 0,contentWidth, 0 }; // 留出头像和边距
				DrawTextW(hdc, wmsg.c_str(), (int)wmsg.size(), &rcMsg, DT_WORDBREAK | DT_CALCRECT);
				ReleaseDC(hwndDlg, hdc);

				int nicknameHeight = 18;
				int padding = 8;
				int minHeight = avatarSize + 8;
				int contentHeight = rcMsg.bottom - rcMsg.top + nicknameHeight + padding;
				lpmis->itemHeight = max(minHeight, contentHeight); // 至少头像高度
			}
			else
			{
				lpmis->itemHeight = 48;
			}

			return (INT_PTR)TRUE;
		}
		return (INT_PTR)TRUE;
		
	}break;

	
	case WM_DRAWITEM:
	{
		LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
		if (lpdis->CtlID == IDC_EDIT60)
		{
			int idx = lpdis->itemID;
			
			if (idx >= 0 && idx < (int)g_users_chart_information.size())
			{
				HWND hList10 = GetDlgItem(hwndDlg, IDC_EDIT60);
				RECT rcListBox;
				GetClientRect(hList10, &rcListBox);
				int listBoxWidth = rcListBox.right - rcListBox.left;
				int avatarSize = 40;
				int leftPadding = 10;
				int rightPadding = 8;
				int contentWidth = listBoxWidth - (avatarSize + leftPadding + rightPadding);

				const users_chart_information& item = g_users_chart_information[idx];

				// 背景
				SetBkColor(lpdis->hDC, (lpdis->itemState & ODS_SELECTED) ? RGB(220, 240, 255) : RGB(255, 255, 255));
				ExtTextOut(lpdis->hDC, 0, 0, ETO_OPAQUE, &lpdis->rcItem, NULL, 0, NULL);

				
				std::string host_account = new_online_user_account;
				if (!host_account.empty() && host_account.back() == '\0')
				{
					host_account.pop_back();
				}

				char* buf9 = new char[50];
				snprintf(buf9, 50, "the  item.inf_send_account is %s\n", item.inf_send_account.c_str());
				OutputDebugStringA(buf9);
				delete[]buf9;

				char* buf10 = new char[50];
				snprintf(buf10, 50, "the  host_account is %s\n", host_account.c_str());
				OutputDebugStringA(buf10);
				delete[]buf10;

				char* buf11 = new char[50];
				snprintf(buf11, 50, "the  account_recver is %s\n", account_recver.c_str());
				OutputDebugStringA(buf11);
				delete[]buf11;

				if (strcmp(account_recver.c_str(),item.inf_send_account.c_str()) == 0)//该消息是好友发送的
				{
					char* buf13 = new char[50];
					snprintf(buf13, 50, "the image_recver is %d\n", image_recver.size());
					OutputDebugStringA(buf13);
					delete[]buf13;

					// 头像
					if (!image_recver.empty())
					{
						char* buf12 = new char[50];
						snprintf(buf12, 50, "the  image_recver.size() is %d\n", (unsigned long long)image_recver.size());
						OutputDebugStringA(buf12);
						delete[]buf12;

						HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE,image_recver.size());
						void* pMem = GlobalLock(hMem);
						memcpy(pMem,image_recver.data(),image_recver.size());
						GlobalUnlock(hMem);
						IStream* pStream = NULL;
						if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) == S_OK)
						{
							Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromStream(pStream);
							if (bmp && bmp->GetLastStatus() == Gdiplus::Ok)
							{
								Gdiplus::Graphics g(lpdis->hDC);
								Gdiplus::Rect dest(lpdis->rcItem.left + 4, lpdis->rcItem.top + 4, avatarSize, avatarSize);
								g.DrawImage(bmp, dest);
								delete bmp;
							}
							pStream->Release();
						}
						GlobalFree(hMem);
					}

					// 昵称
					std::wstring wnickname;
					int wlen = MultiByteToWideChar(CP_UTF8, 0,nickname_recver.c_str(),nickname_recver.size(), NULL, 0);
					wnickname.resize(wlen);
					MultiByteToWideChar(CP_UTF8, 0,nickname_recver.c_str(),nickname_recver.size(), &wnickname[0], wlen);

					RECT rcNick = lpdis->rcItem;
					rcNick.left += avatarSize + 10;
					rcNick.top += 4;
					rcNick.bottom = rcNick.top + 18;
					SetTextColor(lpdis->hDC, RGB(0, 102, 204));
					DrawText(lpdis->hDC, wnickname.c_str(), (int)wnickname.size(), &rcNick, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
				}

				else if (strcmp(host_account.c_str(), item.inf_send_account.c_str()) == 0)
				{
					char* buf13 = new char[50];
					snprintf(buf13, 50, "the user_image_data is %d\n",user_image_data.size());
					OutputDebugStringA(buf13);
					delete[]buf13;

					// 头像
					if (user_image_data.size())
					{
						HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE,user_image_data.size());
						void* pMem = GlobalLock(hMem);
						memcpy(pMem,user_image_data.data(), user_image_data.size());
						GlobalUnlock(hMem);
						IStream* pStream = NULL;
						if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) == S_OK)
						{
							Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromStream(pStream);
							if (bmp && bmp->GetLastStatus() == Gdiplus::Ok)
							{
								Gdiplus::Graphics g(lpdis->hDC);
								Gdiplus::Rect dest(lpdis->rcItem.left + 4, lpdis->rcItem.top + 4, avatarSize, avatarSize);
								g.DrawImage(bmp, dest);
								delete bmp;
							}
							pStream->Release();
						}
						GlobalFree(hMem);
					}

					std::wstring wnickname;
					int wlen_x = MultiByteToWideChar(CP_UTF8, 0,user_nickname_data.c_str(),user_nickname_data.size(), NULL, 0);
					wnickname.resize(wlen_x);
					MultiByteToWideChar(CP_UTF8, 0,user_nickname_data.c_str(),user_nickname_data.size(), &wnickname[0],wlen_x);

					// 昵称
					RECT rcNick = lpdis->rcItem;
					rcNick.left += avatarSize + 10;
					rcNick.top += 4;
					rcNick.bottom = rcNick.top + 18;
					SetTextColor(lpdis->hDC, RGB(0, 102, 204));
					DrawText(lpdis->hDC,wnickname.c_str(),wlen_x, &rcNick, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

				}

				// 消息内容
				std::wstring wmsg;
				int wlen = MultiByteToWideChar(CP_UTF8, 0, item.inf.c_str(), (int)item.inf.size(), NULL, 0);
				wmsg.resize(wlen);
				MultiByteToWideChar(CP_UTF8, 0, item.inf.c_str(), (int)item.inf.size(), &wmsg[0], wlen);

				RECT rcMsg = lpdis->rcItem;
				rcMsg.left += avatarSize + 10;
				rcMsg.top += 24;
				rcMsg.right = rcMsg.left + contentWidth;
				SetTextColor(lpdis->hDC, RGB(0, 0, 0));
				DrawText(lpdis->hDC, wmsg.c_str(), (int)wmsg.size(), &rcMsg, DT_LEFT | DT_WORDBREAK);

				// 选中边框
				if (lpdis->itemState & ODS_SELECTED)
					DrawFocusRect(lpdis->hDC, &lpdis->rcItem);

			}
			return (INT_PTR)TRUE;
		}
	}break;

	
	case WM_COMMAND:
	{
		//检测文本框是否有内容，若有启用发送按钮，若没有禁用发送按钮

		 HWND hList11 = GetDlgItem(hwndDlg,IDC_EDIT61);
		 HWND hBton_ok= GetDlgItem(hwndDlg, IDOK);

		if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_EDIT61)
		{
			int len =GetWindowTextLength(hList11);
			EnableWindow(hBton_ok,len>0);
		}

		switch (LOWORD(wParam))
		{

		case IDC_UPDATEINFORMATION://更新消息
		{

			//向服务器发送更新消息的请求
			std::string r_str = "更新消息";
			int r_str_len = r_str.size();
			send(g_clientSocket, (char*)&r_str_len, sizeof(r_str_len), 0);
			send(g_clientSocket, r_str.c_str(), r_str_len, 0);


			//向服务器发送加载先前聊天记录的请求
			std::string r_str_x = "请求更新聊天框消息";
			int r_str_len_x = r_str_x.size();
			send(g_clientSocket, (char*)&r_str_len_x, sizeof(r_str_len_x), 0);
			send(g_clientSocket, r_str_x.c_str(), r_str_len_x, 0);

			//接收总的聊天框消息条数
			int msg_len = 0;
			recv(g_clientSocket, (char*)&msg_len, sizeof(msg_len), 0);

			//如果条数为0
			if (msg_len <= 0)
			{
				//初始化结束
				return (INT_PTR)TRUE;
			}
			else
			{
				int i = 0;
				g_users_chart_information.clear();

				//接收发送的消息
				while (i < msg_len)
				{
					ss = users_chart_information();//清空结构体
				
					//接收消息发送方账号
					int account_send_len = 0;
					recv(g_clientSocket, (char*)&account_send_len, sizeof(account_send_len), 0);
					ss.inf_send_account.resize(account_send_len);
					recv(g_clientSocket, &ss.inf_send_account[0], account_send_len, 0);

					char* buf3 = new char[50];
					snprintf(buf3, 50, "the  ss.inf_send_account is %s\n", ss.inf_send_account.c_str());
					OutputDebugStringA(buf3);
					delete[]buf3;

					//接收消息接收方账号
					int account_recv_len = 0;
					recv(g_clientSocket, (char*)&account_recv_len, sizeof(account_recv_len), 0);
					ss.inf_recv_account.resize(account_recv_len);
					recv(g_clientSocket, &ss.inf_recv_account[0], account_recv_len, 0);

					char* buf4 = new char[50];
					snprintf(buf4, 50, "the  ss.inf_recv_account is %s\n", ss.inf_recv_account.c_str());
					OutputDebugStringA(buf4);
					delete[]buf4;

					//消息
					int inf_len = 0;
					recv(g_clientSocket, (char*)&inf_len, sizeof(inf_len), 0);
					ss.inf.resize(inf_len);
					recv(g_clientSocket, &ss.inf[0], inf_len, 0);

					char* buf5 = new char[50];
					snprintf(buf5, 50, "the  ss.inf is %s\n", ss.inf.c_str());
					OutputDebugStringA(buf5);
					delete[]buf5;

					//压入队列
					g_users_chart_information.push_back(ss);

					i++;
				}

				//以一定的格式显示聊天记录

				//清空聊天框
				HWND n_x = GetDlgItem(hwndDlg, IDC_EDIT60);
				SendMessage(n_x, LB_RESETCONTENT, 0, 0);

				for (int k = 0; k < msg_len; k++)
				{
					SendDlgItemMessage(hwndDlg, IDC_EDIT60, LB_ADDSTRING, 0, (LPARAM)L"");
				}

			}


		}break;

		case IDOK://发送消息
		{
			//向服务器发送更新消息的请求
			std::string r_str = "发送消息";
			int r_str_len = r_str.size();
			send(g_clientSocket, (char*)&r_str_len, sizeof(r_str_len), 0);
			send(g_clientSocket, r_str.c_str(), r_str_len, 0);

			int text_len = GetWindowTextLength(hList11);
			std::wstring text_str(text_len+1, L'\0');
			GetWindowText(hList11,&text_str[0],text_len+1);
            
			if (!text_str.empty() && text_str.back() == L'\0')
			{
				text_str.pop_back();
			}

			//将宽字符串转换为字符串
			int utf8_len = WideCharToMultiByte(CP_UTF8,0,text_str.c_str(),text_str.size(),NULL,0,NULL,NULL);
			std::string utf8_str(utf8_len,'\0');
			WideCharToMultiByte(CP_UTF8, 0, text_str.c_str(), text_str.size(),&utf8_str[0],utf8_len,NULL,NULL);
		
			send(g_clientSocket, (char*)&utf8_len, sizeof(utf8_len), 0);
			send(g_clientSocket,utf8_str.c_str(),utf8_len,0);

			//清空文本框
			SetDlgItemText(hwndDlg,IDC_EDIT61,L"");

			//更新消息

			
			//向服务器发送加载先前聊天记录的请求
			std::string r_str_x_12 = "请求更新聊天框消息";
			int r_str_len_x_12 = r_str_x_12.size();
			send(g_clientSocket, (char*)&r_str_len_x_12, sizeof(r_str_len_x_12), 0);
			send(g_clientSocket, r_str_x_12.c_str(), r_str_len_x_12, 0);

			//接收总的聊天框消息条数
			int msg_len = 0;
			recv(g_clientSocket, (char*)&msg_len, sizeof(msg_len), 0);

			//如果条数为0
			if (msg_len <= 0)
			{
				//初始化结束
				return (INT_PTR)TRUE;
			}
			else
			{
				int i = 0;
				g_users_chart_information.clear();

				//接收发送的消息
				while (i < msg_len)
				{
					ss = users_chart_information();//清空结构体

					//接收消息发送方账号
					int account_send_len = 0;
					recv(g_clientSocket, (char*)&account_send_len, sizeof(account_send_len), 0);
					ss.inf_send_account.resize(account_send_len);
					recv(g_clientSocket, &ss.inf_send_account[0], account_send_len, 0);

					char* buf3 = new char[50];
					snprintf(buf3, 50, "the  ss.inf_send_account is %s\n", ss.inf_send_account.c_str());
					OutputDebugStringA(buf3);
					delete[]buf3;

					//接收消息接收方账号
					int account_recv_len = 0;
					recv(g_clientSocket, (char*)&account_recv_len, sizeof(account_recv_len), 0);
					ss.inf_recv_account.resize(account_recv_len);
					recv(g_clientSocket, &ss.inf_recv_account[0], account_recv_len, 0);

					char* buf4 = new char[50];
					snprintf(buf4, 50, "the  ss.inf_recv_account is %s\n", ss.inf_recv_account.c_str());
					OutputDebugStringA(buf4);
					delete[]buf4;

					//消息
					int inf_len = 0;
					recv(g_clientSocket, (char*)&inf_len, sizeof(inf_len), 0);
					ss.inf.resize(inf_len);
					recv(g_clientSocket, &ss.inf[0], inf_len, 0);

					char* buf5 = new char[50];
					snprintf(buf5, 50, "the  ss.inf is %s\n", ss.inf.c_str());
					OutputDebugStringA(buf5);
					delete[]buf5;

					//压入队列
					g_users_chart_information.push_back(ss);

					i++;
				}

				//以一定的格式显示聊天记录

				//清空聊天框
				HWND n_x = GetDlgItem(hwndDlg, IDC_EDIT60);
				SendMessage(n_x, LB_RESETCONTENT, 0, 0);

				for (int k = 0; k < msg_len; k++)
				{
					SendDlgItemMessage(hwndDlg, IDC_EDIT60, LB_ADDSTRING, 0, (LPARAM)L"");
				}

			}

			return (INT_PTR)TRUE;

		}break;

		case IDCANCEL://退出该框
		{
			//向服务器发送更新消息的请求
			std::string r_str = "退出";
			int r_str_len = r_str.size();
			send(g_clientSocket, (char*)&r_str_len, sizeof(r_str_len), 0);
			send(g_clientSocket, r_str.c_str(), r_str_len, 0);

			//将聊天对话框关闭
			EndDialog(hwndDlg, IDCANCEL);
			return (INT_PTR)TRUE;

		}break;

		}


	}break;

	}

    return (INT_PTR)FALSE;
}



INT_PTR CALLBACK  Dialog_thirdteen(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//好友列表框//
{
	switch (uMsg)
	{
	
	case WM_INITDIALOG:
	{

		HWND hBtn1 = GetDlgItem(hwndDlg, IDC_DELETE_FRIEND);
	    EnableWindow(hBtn1,FALSE);
		HWND hBtn2 = GetDlgItem(hwndDlg, IDC_FRIEND_INFORMATION);
		EnableWindow(hBtn2, FALSE);
		HWND hBtn3 = GetDlgItem(hwndDlg, IDC_CHART);
		EnableWindow(hBtn3, FALSE);
		
		//请求加载好友列表
		std::string msg = "请求加载好友列表";
		int msg_len = msg.size();
		send(g_clientSocket, (char*)&msg_len, sizeof(msg_len), 0);
		send(g_clientSocket, msg.c_str(), msg_len, 0);
		//接收服务器是否成功接收客户端请求
		int recv_len = 0;
		recv(g_clientSocket, (char*)&recv_len, sizeof(recv_len), 0);
		std::string recv_str(recv_len, '\0');
		recv(g_clientSocket, &recv_str[0], recv_len, 0);
		//比较接收内容
		if (strcmp("success", recv_str.c_str()) != 0)
		{
			MessageBox(NULL, L"服务器未成功接收加载客户端的好友列表请求", L"QQ", MB_ICONERROR);
			//关闭窗口，返回登录界面首页框
			EndDialog(hwndDlg, IDCANCEL);
			return (INT_PTR)TRUE;
		}
		else
		{
			MessageBox(NULL, L"服务器已成功接收加载客户端的好友列表请求", L"QQ", MB_ICONINFORMATION);
		}
		//接收好友列表的条数
		int total_count = 0;
		recv(g_clientSocket, (char*)&total_count, sizeof(total_count), 0);

		if (total_count > 0)
		{
			MessageBox(NULL, L"即将加载好友列表", L"QQ", MB_ICONINFORMATION);
			//接收好友列表的消息集
			int i = 0;
			g_user_friends.clear();//确保好友消息队列清空
			user_friend u;
			while (i < total_count)
			{
				u = user_friend();//清空结构体

				//接收好友在线标志
				int signal_x = 0;
				recv(g_clientSocket,(char*)&signal_x,sizeof(signal_x),0);
				if (signal_x)
				{
					u.signal = 1;
				}
				else
				{
					u.signal = 0;
				}


				//接收好友账号
				int account_x_len = 0;
				recv(g_clientSocket, (char*)&account_x_len, sizeof(account_x_len), 0);
				u.account.resize(account_x_len);//重新分配空间
				recv(g_clientSocket, &u.account[0], account_x_len, 0);

				char* buf = new char[50]();
				snprintf(buf,50,"the friend_account is %s\n",u.account.c_str());
				OutputDebugStringA(buf);
				delete[]buf;

				//接收好友昵称
				int nickname_x_len = 0;
				recv(g_clientSocket, (char*)&nickname_x_len, sizeof(nickname_x_len), 0);
				u.nickname.resize(nickname_x_len);//重新分配空间
				recv(g_clientSocket, &u.nickname[0], nickname_x_len, 0);

				char* buf_2 = new char[50]();
				snprintf(buf_2, 50, "the friend_nickname is %s\n", u.nickname.c_str());
				OutputDebugStringA(buf_2);
				delete[]buf_2;

				//接收好友头像
				int image_x_len = 0;
				recv(g_clientSocket, (char*)&image_x_len, sizeof(image_x_len), 0);


				char* buf_3 = new char[50]();
				snprintf(buf_3, 50, "the friend_photo_len is %d\n",image_x_len);
				OutputDebugStringA(buf_3);
				delete[]buf_3;


                u.image.resize(image_x_len);//重新分配空间
				int  u_r = 0;
				int u_sum = 0;
				while (u_sum < image_x_len)
				{
					u_r = recv(g_clientSocket, (char*)u.image.data() + u_sum, image_x_len - u_sum, 0);
					if (u_r > 0)
					{
						u_sum += u_r;
					}
				}
				//接收完毕，将好友的信息压入弹夹
				g_user_friends.push_back(u);

				i++;
			}

			MessageBox(NULL, L"已经成功接收用户的好友消息", L"QQ", MB_ICONINFORMATION);
			//已完成用户好友信息列表的接收
			//将用户好友列表以一定的格式显示在控件IDC_EDIT46上
			HWND hList12 = GetDlgItem(hwndDlg, IDC_EDIT46);
			SendMessage(hList12, LB_RESETCONTENT, 0, 0);//清空列表框
			for (size_t i = 0; i < g_user_friends.size(); ++i)
			{
				SendDlgItemMessage(hwndDlg, IDC_EDIT46, LB_ADDSTRING, 0, (LPARAM)L"");
				SendDlgItemMessage(hwndDlg, IDC_EDIT46, LB_SETITEMHEIGHT, i, 48); // 每项48像素
			}
			return (INT_PTR)TRUE;
		}

		{
			MessageBox(NULL, L"好友列表为空", L"QQ", MB_ICONINFORMATION);
			//初始化结束
			return (INT_PTR)TRUE;
		}

	}break;

	
	case WM_DRAWITEM:
	{
		LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
		if (lpdis->CtlID == IDC_EDIT46)
		{
			int idx = lpdis->itemID;
			if (idx >= 0 && idx < (int)g_user_friends.size())
			{
				HWND hList13 = GetDlgItem(hwndDlg, IDC_EDIT46);
				RECT rcListBox;
				GetClientRect(hList13, &rcListBox);
				int listBoxWidth = rcListBox.right - rcListBox.left;
				int avatarSize = 40;
				int leftPadding = 10;
				int rightPadding = 8;
				int contentWidth = listBoxWidth - (avatarSize + leftPadding + rightPadding);

				const user_friend& item = g_user_friends[idx];

				// 背景
				SetBkColor(lpdis->hDC, (lpdis->itemState & ODS_SELECTED) ? RGB(230,230, 250) : RGB(255, 255, 255));
				ExtTextOut(lpdis->hDC, 0, 0, ETO_OPAQUE, &lpdis->rcItem, NULL, 0, NULL);

				// 头像
				if (!item.image.empty())
				{
					HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, item.image.size());
					void* pMem = GlobalLock(hMem);
					memcpy(pMem, item.image.data(), item.image.size());
					GlobalUnlock(hMem);
					IStream* pStream = NULL;
					if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) == S_OK)
					{
						Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromStream(pStream);
						if (bmp && bmp->GetLastStatus() == Gdiplus::Ok)
						{
							Gdiplus::Graphics g(lpdis->hDC);
							Gdiplus::Rect dest(lpdis->rcItem.left + 4, lpdis->rcItem.top + 4, avatarSize, avatarSize);
							g.DrawImage(bmp, dest);
							delete bmp;
						}
						pStream->Release();
					}
					GlobalFree(hMem);
				}

				// 昵称
				std::wstring wnickname;
				int wlen = MultiByteToWideChar(CP_UTF8, 0, item.nickname.c_str(), (int)item.nickname.size(), NULL, 0);
				wnickname.resize(wlen);
				MultiByteToWideChar(CP_UTF8, 0, item.nickname.c_str(), (int)item.nickname.size(), &wnickname[0], wlen);

				RECT rcNick = lpdis->rcItem;
				rcNick.left += avatarSize + 10;
				rcNick.top += 4;
				rcNick.bottom = rcNick.top + 18;
				SetTextColor(lpdis->hDC, RGB(0, 102, 204));
				DrawText(lpdis->hDC, wnickname.c_str(), (int)wnickname.size(), &rcNick, DT_LEFT | DT_SINGLELINE | DT_VCENTER |DT_CALCRECT);
				DrawText(lpdis->hDC, wnickname.c_str(), (int)wnickname.size(), &rcNick, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
				int t_wideth = rcNick.right - rcNick.left;

				std::wstring wsignal = L"离线";
				// 在线标志
				if (item.signal)
				{
					wsignal = L"在线";
				}
				else
				{
					wsignal = L"离线";
				}

				RECT rcSignal = lpdis->rcItem;
				rcSignal.left += avatarSize + 10 + t_wideth + 30;
				rcSignal.top += 4;
				rcSignal.bottom = rcNick.top + 18;
				SetTextColor(lpdis->hDC, RGB(232, 46, 115));
				DrawText(lpdis->hDC, wsignal.c_str(), (int)wsignal.size(), &rcSignal, DT_LEFT | DT_SINGLELINE | DT_VCENTER);


				// 账号内容
				std::wstring waccount;
				wlen = MultiByteToWideChar(CP_UTF8, 0, item.account.c_str(), (int)item.account.size(), NULL, 0);
				waccount.resize(wlen);
				MultiByteToWideChar(CP_UTF8, 0, item.account.c_str(), (int)item.account.size(), &waccount[0], wlen);

				RECT rcAcc = lpdis->rcItem;
				rcAcc.left += avatarSize + 10;
				rcAcc.top += 24;
				rcAcc.bottom = rcAcc.top + 18;
				SetTextColor(lpdis->hDC, RGB(0, 0, 0));
				DrawText(lpdis->hDC, waccount.c_str(), (int)waccount.size(), &rcAcc, DT_LEFT | DT_SINGLELINE);


				// 选中边框
				if (lpdis->itemState & ODS_FOCUS)
					DrawFocusRect(lpdis->hDC, &lpdis->rcItem);
			}

		return (INT_PTR)TRUE;
		}
	}break;


	case WM_COMMAND:
	{
		
		if (HIWORD(wParam) == LBN_SELCHANGE && LOWORD(wParam) == IDC_EDIT46)
		{
			HWND hList14 = GetDlgItem(hwndDlg, IDC_EDIT46);
			HWND hBtn1 = GetDlgItem(hwndDlg, IDC_DELETE_FRIEND);
			HWND hBtn2 = GetDlgItem(hwndDlg, IDC_FRIEND_INFORMATION);
			HWND hBtn3 = GetDlgItem(hwndDlg, IDC_CHART);

			int selIdx = (int)SendMessage(hList14, LB_GETCURSEL, 0, 0);
			EnableWindow(hBtn1, selIdx != LB_ERR); // 有选中项时启用按钮，否则禁用
			EnableWindow(hBtn2, selIdx != LB_ERR); // 有选中项时启用按钮，否则禁用
			EnableWindow(hBtn3, selIdx != LB_ERR); // 有选中项时启用按钮，否则禁用

			char* buf_k = new char[50];
			snprintf(buf_k,strlen(buf_k),"the selIdex is %d\n",selIdx);
			OutputDebugStringA(buf_k);
			delete[]buf_k;
		}
		
		switch (LOWORD(wParam))
		{

		case IDC_UPDATE_LIST:
		{
			std::string s = "刷新列表";
			int s_l = s.size();
			send(g_clientSocket, (char*)&s_l, sizeof(s_l), 0);
			send(g_clientSocket, s.c_str(), s_l, 0);

			//重新加载好友列表

			//请求加载好友列表
			std::string msg = "请求加载好友列表";
			int msg_len = msg.size();
			send(g_clientSocket, (char*)&msg_len, sizeof(msg_len), 0);
			send(g_clientSocket, msg.c_str(), msg_len, 0);
			//接收服务器是否成功接收客户端请求
			int recv_len = 0;
			recv(g_clientSocket, (char*)&recv_len, sizeof(recv_len), 0);
			std::string recv_str(recv_len, '\0');
			recv(g_clientSocket, &recv_str[0], recv_len, 0);
			//比较接收内容
			if (strcmp("success", recv_str.c_str()) != 0)
			{
				MessageBox(NULL, L"服务器未成功接收加载客户端的好友列表请求", L"QQ", MB_ICONERROR);
				//关闭窗口，返回登录界面首页框
				EndDialog(hwndDlg, IDCANCEL);
				return (INT_PTR)TRUE;
			}
			else
			{
				MessageBox(NULL, L"服务器已成功接收加载客户端的好友列表请求", L"QQ", MB_ICONINFORMATION);
			}
			//接收好友列表的条数
			int total_count = 0;
			recv(g_clientSocket, (char*)&total_count, sizeof(total_count), 0);

			if (total_count > 0)
			{
				MessageBox(NULL, L"即将加载好友列表", L"QQ", MB_ICONINFORMATION);
				//接收好友列表的消息集
				int i = 0;
				g_user_friends.clear();//确保好友消息队列清空
				user_friend u;
				while (i < total_count)
				{
					u = user_friend();//清空结构体

					//接收好友在线标志
					int signal_x = 0;
					recv(g_clientSocket, (char*)&signal_x, sizeof(signal_x), 0);
					if (signal_x)
					{
						u.signal = 1;
					}
					else
					{
						u.signal = 0;
					}


					//接收好友账号
					int account_x_len = 0;
					recv(g_clientSocket, (char*)&account_x_len, sizeof(account_x_len), 0);
					u.account.resize(account_x_len);//重新分配空间
					recv(g_clientSocket, &u.account[0], account_x_len, 0);

					char* buf = new char[50]();
					snprintf(buf, 50, "the friend_account is %s\n", u.account.c_str());
					OutputDebugStringA(buf);
					delete[]buf;

					//接收好友昵称
					int nickname_x_len = 0;
					recv(g_clientSocket, (char*)&nickname_x_len, sizeof(nickname_x_len), 0);
					u.nickname.resize(nickname_x_len);//重新分配空间
					recv(g_clientSocket, &u.nickname[0], nickname_x_len, 0);

					char* buf_2 = new char[50]();
					snprintf(buf_2, 50, "the friend_nickname is %s\n", u.nickname.c_str());
					OutputDebugStringA(buf_2);
					delete[]buf_2;

					//接收好友头像
					int image_x_len = 0;
					recv(g_clientSocket, (char*)&image_x_len, sizeof(image_x_len), 0);


					char* buf_3 = new char[50]();
					snprintf(buf_3, 50, "the friend_photo_len is %d\n", image_x_len);
					OutputDebugStringA(buf_3);
					delete[]buf_3;


					u.image.resize(image_x_len);//重新分配空间
					int  u_r = 0;
					int u_sum = 0;
					while (u_sum < image_x_len)
					{
						u_r = recv(g_clientSocket, (char*)u.image.data() + u_sum, image_x_len - u_sum, 0);
						if (u_r > 0)
						{
							u_sum += u_r;
						}
					}
					//接收完毕，将好友的信息压入弹夹
					g_user_friends.push_back(u);

					i++;
				}

				MessageBox(NULL, L"已经成功接收用户的好友消息", L"QQ", MB_ICONINFORMATION);
				//已完成用户好友信息列表的接收
				//将用户好友列表以一定的格式显示在控件IDC_EDIT46上
				HWND hList12 = GetDlgItem(hwndDlg, IDC_EDIT46);
				SendMessage(hList12, LB_RESETCONTENT, 0, 0);//清空列表框
				for (size_t i = 0; i < g_user_friends.size(); ++i)
				{
					SendDlgItemMessage(hwndDlg, IDC_EDIT46, LB_ADDSTRING, 0, (LPARAM)L"");
					SendDlgItemMessage(hwndDlg, IDC_EDIT46, LB_SETITEMHEIGHT, i, 48); // 每项48像素
				}
				return (INT_PTR)TRUE;
			}

			{
				MessageBox(NULL, L"好友列表为空", L"QQ", MB_ICONINFORMATION);
				//初始化结束
				return (INT_PTR)TRUE;
			}


		}break;

		case IDC_CHART://聊天
		{
			//聊天框逻辑
			//向服务器发送退出消息
			std::string s = "聊天";
			int s_l = s.size();
			send(g_clientSocket, (char*)&s_l, sizeof(s_l), 0);
			send(g_clientSocket, s.c_str(), s_l, 0);

			//获取聊天对象的账号、昵称和头像
			HWND hList15 = GetDlgItem(hwndDlg, IDC_EDIT46);
			int selIdx = (int)SendMessage(hList15, LB_GETCURSEL, 0, 0);
			std::string& account_recver_x = g_user_friends[selIdx].account;

			account_recver.clear();
			account_recver = account_recver_x;

			nickname_recver.clear();
			image_recver.clear();

			nickname_recver = g_user_friends[selIdx].nickname;
			image_recver= g_user_friends[selIdx].image;

			//发送聊天对象的账号
			int recver_account_len = 0;
			recver_account_len = account_recver_x.size();
			send(g_clientSocket, (char*)&recver_account_len, sizeof(recver_account_len), 0);
			send(g_clientSocket, account_recver_x.c_str(), recver_account_len, 0);

			//接收用户昵称
			int nickname_len = 0;
			recv(g_clientSocket, (char*)&nickname_len, sizeof(nickname_len), 0);
			user_nickname_data.resize(nickname_len);
			recv(g_clientSocket,&user_nickname_data[0],nickname_len,0);

			EndDialog(hwndDlg, IDC_CHART);
			return (INT_PTR)TRUE;
		}break;

		case IDC_FRIEND_INFORMATION://好友消息
		{
			//向服务器发送退出消息
			std::string s = "查看好友信息";
			int s_l = s.size();
			send(g_clientSocket, (char*)&s_l, sizeof(s_l), 0);
			send(g_clientSocket, s.c_str(), s_l, 0);

			HWND hList16 = GetDlgItem(hwndDlg, IDC_EDIT46);
			int selIdx = (int)SendMessage(hList16, LB_GETCURSEL, 0, 0);

			std::string& account = g_user_friends[selIdx].account;

			//发送选中项的账号
			int acc_len = account.size();
			send(g_clientSocket, (char*)&acc_len, sizeof(acc_len), 0);
			send(g_clientSocket, account.c_str(), acc_len, 0);

			char* buf_account = new char[50]();
			snprintf(buf_account, 50, "the fri_account is %s\n",account.c_str());
			OutputDebugStringA(buf_account);
			delete[]buf_account;


			//接收该好友的个性信息
			xxy = my_user_information();//清空结构体
			xxy.account = g_user_friends[selIdx].account;
			xxy.nickname = g_user_friends[selIdx].nickname;

			//接收好友性别
			int fri_gender_len = 0;
			recv(g_clientSocket, (char*)&fri_gender_len, sizeof(fri_gender_len), 0);
			std::string fri_gender(fri_gender_len, '\0');
			recv(g_clientSocket,&fri_gender[0],fri_gender_len,0);
			xxy.gender = fri_gender;

			char* buf_gender = new char[50]();
			snprintf(buf_gender,50,"the fri_gender is %s\n",fri_gender.c_str());
			OutputDebugStringA(buf_gender);
			delete[]buf_gender;

			//接收好友的年龄
			int fri_age_len = 0;
			recv(g_clientSocket, (char*)&fri_age_len, sizeof(fri_age_len), 0);
			std::string fri_age(fri_age_len, '\0');
			recv(g_clientSocket, &fri_age[0], fri_age_len, 0);
			xxy.age = fri_age;

			char* buf_age = new char[50]();
			snprintf(buf_age, 50, "the fri_age is %s\n", fri_age.c_str());
			OutputDebugStringA(buf_age);
			delete[]buf_age;

			//接收好友的个性签名
			int fri_signature_len = 0;
			recv(g_clientSocket, (char*)&fri_signature_len, sizeof(fri_signature_len), 0);
			std::string fri_signature(fri_signature_len, '\0');
			recv(g_clientSocket, &fri_signature[0], fri_signature_len, 0);
			xxy.signature = fri_signature;

			char* buf_signature = new char[50]();
			snprintf(buf_signature, 50, "the fri_gender is %s\n", fri_signature.c_str());
			OutputDebugStringA(buf_signature);
			delete[]buf_signature;


			//跳转到好友信息展示框
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MYDIALOG_EIGHTEEN), NULL, Dialog_eighteen);

			//接收服务器处理标志
			int x_len = 0;
			recv(g_clientSocket, (char*)&x_len, sizeof(x_len), 0);
			std::string x_str(x_len, '\0');
			recv(g_clientSocket, &x_str[0], x_len, 0);

			if (strcmp("success", x_str.c_str()) == 0)
			{
				MessageBox(NULL, L"服务器已经成功处理您的查看好友消息的请求", L"QQ", MB_ICONINFORMATION);
			}

			EndDialog(hwndDlg, IDC_FRIEND_INFORMATION);
			return (INT_PTR)TRUE;
		}break;

		case IDC_ADD_FRIEND://添加好友
		{
			//向服务器发送添加好友的请求
			std::string buf = "客户端请求添加好友";
			int buf_len = buf.size();
			send(g_clientSocket, (char*)&buf_len, sizeof(buf_len), 0);
			send(g_clientSocket,buf.c_str(),buf_len,0);


			EndDialog(hwndDlg,IDC_ADD_FRIEND);
			return (INT_PTR)TRUE;
		}break;

		case IDC_DELETE_FRIEND://删除好友
		{
			//向服务器发送退出消息
			std::string s = "删除好友";
			int s_l = s.size();
			send(g_clientSocket, (char*)&s_l, sizeof(s_l), 0);
			send(g_clientSocket, s.c_str(), s_l, 0);

			HWND hList17 = GetDlgItem(hwndDlg,IDC_EDIT46);
			int selIdx = (int)SendMessage(hList17, LB_GETCURSEL, 0, 0);

			std::string& account = g_user_friends[selIdx].account;
		    
			//发送选中项的账号
			int acc_len = account.size();
			send(g_clientSocket,(char*)&acc_len,sizeof(acc_len),0);
			send(g_clientSocket, account.c_str(), acc_len, 0);


			//接收服务器处理标志
			int x_len = 0;
			recv(g_clientSocket, (char*)&x_len, sizeof(x_len), 0);
			std::string x_str(x_len,'\0');
			recv(g_clientSocket, &x_str[0], x_len, 0);

			if (strcmp("success", x_str.c_str()) == 0)
			{
				MessageBox(NULL,L"服务器已经成功处理您的删除好友的请求",L"QQ",MB_ICONINFORMATION);
			}

			EndDialog(hwndDlg, IDC_DELETE_FRIEND);
			return (INT_PTR)TRUE;
		}break;

		case IDC_NEW_FRIEND://新朋友
		{
			//向服务器发送退出消息
			std::string s = "新朋友";
			int s_l = s.size();
			send(g_clientSocket, (char*)&s_l, sizeof(s_l), 0);
			send(g_clientSocket, s.c_str(), s_l, 0);

			EndDialog(hwndDlg, IDC_NEW_FRIEND);
			return (INT_PTR)TRUE;
		}break;

		case IDCANCEL://退出
		{
			//向服务器发送退出消息
			std::string s = "退出";
			int s_l = s.size();
			send(g_clientSocket, (char*)&s_l, sizeof(s_l), 0);
			send(g_clientSocket, s.c_str(), s_l, 0);

			char* buf_102 = new char[50];
			snprintf(buf_102, 50, "the s is %s\n",s.c_str());
			OutputDebugStringA(buf_102);
			delete[]buf_102;


			EndDialog(hwndDlg, IDCANCEL);
			return (INT_PTR)TRUE;
		}break;

		}
	}

	}
	return (INT_PTR)FALSE;
}

int recv_all(SOCKET socket,char* buf, int buf_size, int signal = 0)
{
	int r = 0;
	int sum = 0;
	while (sum < buf_size)
	{
		int r = recv(socket, buf + sum, buf_size - sum, 0);//单次接收量
		if (r > 0)
		{
			sum += r;
		}
		if (sum == buf_size)
		{
			return sum;
		}
	}
}

void recv_personal_information_from_server()//接收用户个人信息
{
	//接收密码
	int password_len = 0;
	int len1=recv_all(g_clientSocket, (char*)&password_len, sizeof(password_len), 0);
	if (len1!= sizeof(password_len))
	{
		MessageBox(NULL, L"接收个人信息的密码长度失败", L"QQ", MB_ICONERROR);
		return;
	}
	std::string password_buf(password_len, '\0');
	recv_all(g_clientSocket, &password_buf[0], password_len, 0);
	old_my_user_information.password = password_buf;
	//转换为宽字符串 
	int w_password_len = MultiByteToWideChar(CP_UTF8, 0, password_buf.c_str(), password_buf.size(), NULL, 0);
	std::wstring w_password_str(w_password_len, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, password_buf.c_str(), password_buf.size(), &w_password_str[0], w_password_len);
	w_old_my_user_information.password = w_password_str;

	//接收昵称
	int nickname_len = 0;
	int len2= recv_all(g_clientSocket, (char*)&nickname_len, sizeof(nickname_len), 0);
	if (len2!= sizeof(nickname_len))
	{
		MessageBox(NULL, L"接收个人信息的昵称长度失败", L"QQ", MB_ICONERROR);
		return;
	}
	std::string nickname_buf(nickname_len, '\0');
	recv_all(g_clientSocket, &nickname_buf[0], nickname_len, 0);
	old_my_user_information.nickname = nickname_buf;
	//转换为宽字符串 
	int w_nickname_len = MultiByteToWideChar(CP_UTF8, 0, nickname_buf.c_str(), nickname_buf.size(), NULL, 0);
	std::wstring w_nickname_str(w_nickname_len, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, nickname_buf.c_str(), nickname_buf.size(), &w_nickname_str[0], w_nickname_len);
	w_old_my_user_information.nickname = w_nickname_str;

	//接收性别
	int gender_len = 0;
	int len3= recv_all(g_clientSocket, (char*)&gender_len, sizeof(gender_len), 0);
	if (len3!= sizeof(gender_len))
	{
		MessageBox(NULL, L"接收个人信息的性别长度失败", L"QQ", MB_ICONERROR);
		return;
	}
	std::string gender_buf(gender_len, '\0');
	recv_all(g_clientSocket, &gender_buf[0], gender_len, 0);
	old_my_user_information.gender = gender_buf;
	//转换为宽字符串 
	int w_gender_len = MultiByteToWideChar(CP_UTF8, 0, gender_buf.c_str(), gender_buf.size(), NULL, 0);
	std::wstring w_gender_str(w_gender_len, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, gender_buf.c_str(), gender_buf.size(), &w_gender_str[0], w_gender_len);
	w_old_my_user_information.gender = w_gender_str;

	//接收年龄
	int age_len = 0;
	int len4 = recv_all(g_clientSocket, (char*)&age_len, sizeof(age_len), 0);
	if (len4 != sizeof(age_len))
	{
		MessageBox(NULL, L"接收个人信息的年龄长度失败", L"QQ", MB_ICONERROR);
		return;
	}
	std::string age_buf(age_len, '\0');
	recv_all(g_clientSocket, &age_buf[0], age_len, 0);
	old_my_user_information.age = age_buf;
	//转换为宽字符串 
	int w_age_len = MultiByteToWideChar(CP_UTF8, 0, age_buf.c_str(), age_buf.size(), NULL, 0);
	std::wstring w_age_str(w_age_len, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, age_buf.c_str(), age_buf.size(), &w_age_str[0], w_age_len);
	w_old_my_user_information.age = w_age_str;

	//接收个性签名
	int signature_len = 0;
	int len5 = recv_all(g_clientSocket, (char*)&signature_len, sizeof(signature_len), 0);
	if (len5 != sizeof(signature_len))
	{
		MessageBox(NULL, L"接收个人信息的个性签名长度失败", L"QQ", MB_ICONERROR);
		return;
	}
	std::string signature_buf(signature_len, '\0');
	recv_all(g_clientSocket, &signature_buf[0], signature_len, 0);
	old_my_user_information.signature = signature_buf;
	//转换为宽字符串 
	int w_signature_len = MultiByteToWideChar(CP_UTF8, 0, signature_buf.c_str(), signature_buf.size(), NULL, 0);
	std::wstring w_signature_str(w_signature_len, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, signature_buf.c_str(), signature_buf.size(), &w_signature_str[0], w_signature_len);
	w_old_my_user_information.signature= w_signature_str;
	MessageBox(NULL, L"已经成功接受所有用户信息数据", L"QQ", MB_ICONINFORMATION);
}


bool get_infor_from_id(HWND hwndDlg,int id,std::wstring &my_infor)//获取控件内容
{
	HWND hwndEdit = GetDlgItem(hwndDlg,id); // 获取控件句柄
	int len = GetWindowTextLength(hwndEdit);        // 获取文本长度（不含'\0'）
	if (len > 0)
	{
		my_infor.resize(len+1);           // 预分配空间
		GetWindowText(hwndEdit, &my_infor[0], len+1); // 获取文本内容
		if (!my_infor.empty()&&my_infor.back()=='\0')
		{
			my_infor.pop_back();
			return true;
		}
	}
	else
	{
		MessageBox(NULL, L"不允许修改内容为空", L"QQ", MB_ICONERROR);
		return false;
	}
}

bool get_infor_gender_from_id(HWND hwndDlg, int id, std::wstring& my_infor)//获取控件内容
{
	HWND hwndEdit = GetDlgItem(hwndDlg, id); // 获取控件句柄
	int len = GetWindowTextLength(hwndEdit);        // 获取文本长度（不含'\0'）
	if (len > 0)
	{
		my_infor.resize(len + 1);           // 预分配空间
		GetWindowText(hwndEdit, &my_infor[0], len + 1); // 获取文本内容
		if (!my_infor.empty() && my_infor.back() == '\0')
		{
			my_infor.pop_back();
			if (!((wcscmp(my_infor.c_str(), L"男") == 0) || (wcscmp(my_infor.c_str(), L"女") == 0)))
			{
				MessageBox(NULL, L"获取控件性别失败,只能是’男‘ 或’女‘", L"QQ", MB_ICONINFORMATION);
				return false;
			}
			else
			{
				return true;
			}
		}
	}
	else
	{
		MessageBox(NULL, L"不允许修改内容为空", L"QQ", MB_ICONERROR);
		return false;
	}
}

void send_new_user_infor(SOCKET socket, std::wstring& w_str)//将用户的新信息发出
{
	int new_password_len = WideCharToMultiByte(CP_UTF8, 0, w_str.c_str(), w_str.size(), NULL, 0, NULL, NULL);
	std::string new_password_str(new_password_len, '\0');
	WideCharToMultiByte(CP_UTF8, 0, w_str.c_str(), w_str.size(), &new_password_str[0], new_password_len, NULL, NULL);
	send(g_clientSocket, (char*)&new_password_len, sizeof(new_password_len), 0);
	send(g_clientSocket, new_password_str.c_str(), new_password_len, 0);
}

INT_PTR CALLBACK Dialog_fourteen(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//登录后个人信息框//
{
	switch (uMsg)
	{
	case WM_INITDIALOG:

		recv_personal_information_from_server();//接收用户个人数据

		SetDlgItemText(hwndDlg, IDC_EDIT31, L"账号");
		SetDlgItemText(hwndDlg, IDC_EDIT32, L"密码");
		SetDlgItemText(hwndDlg, IDC_EDIT33, L"昵称");
		SetDlgItemText(hwndDlg, IDC_EDIT34, L"性别");
		SetDlgItemText(hwndDlg, IDC_EDIT35, L"年龄");
		SetDlgItemText(hwndDlg, IDC_EDIT36, L"个性签名");
		SetDlgItemText(hwndDlg, IDC_EDIT37,w_new_online_user_account.c_str());//账号

		SetDlgItemText(hwndDlg, IDC_EDIT38,w_old_my_user_information.password.c_str());//密码
		SetDlgItemText(hwndDlg, IDC_EDIT39, w_old_my_user_information.nickname.c_str());//昵称
		SetDlgItemText(hwndDlg, IDC_EDIT40, w_old_my_user_information.gender.c_str());//性别
		SetDlgItemText(hwndDlg, IDC_EDIT41, w_old_my_user_information.age.c_str());//年龄
		SetDlgItemText(hwndDlg, IDC_EDIT42, w_old_my_user_information.signature.c_str());//个性签名
        return (INT_PTR)TRUE;

	case WM_COMMAND:

		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			
			w_new_user_information my_user_new_information;
			//获取新的用户信息存储在my_user_new_information结构体
			
			if (!get_infor_from_id(hwndDlg, IDC_EDIT38, my_user_new_information.password))//获取控件密码
			{
				MessageBox(NULL, L"获取控件密码失败", L"QQ", MB_ICONINFORMATION);
				return (INT_PTR)TRUE;
			}
			if (!get_infor_from_id(hwndDlg, IDC_EDIT39, my_user_new_information.nickname))//获取控件昵称
			{
				MessageBox(NULL, L"获取控件账号失败", L"QQ", MB_ICONINFORMATION);
				return (INT_PTR)TRUE;
			}
			if (!get_infor_gender_from_id(hwndDlg, IDC_EDIT40, my_user_new_information.gender))//获取控件性别
			{
				//MessageBox(NULL, L"获取控件密码失败，只能填’男‘或’女‘", L"QQ", MB_ICONINFORMATION);
				return (INT_PTR)TRUE;
			}
			if (!get_infor_from_id(hwndDlg, IDC_EDIT41, my_user_new_information.age))//获取控件年龄
			{
				MessageBox(NULL, L"获取控件年龄失败", L"QQ", MB_ICONINFORMATION);
				return (INT_PTR)TRUE;
			}
			if (!get_infor_from_id(hwndDlg, IDC_EDIT42, my_user_new_information.signature))//获取控件个性签名
			{
				MessageBox(NULL, L"获取控件个性签名失败", L"QQ", MB_ICONINFORMATION);
				return (INT_PTR)TRUE;
			}
			//MessageBox(NULL, L"已经成功获取新的个人信息", L"QQ", MB_ICONINFORMATION);

			//向服务器发送确定选择
			std::wstring my_sel = L"确定";
			int utf8_my_sel = WideCharToMultiByte(CP_UTF8, 0, my_sel.c_str(), my_sel.size() + 1, NULL, 0, NULL, NULL);
			std::string utf8_my_sel_str(utf8_my_sel, '\0');
			WideCharToMultiByte(CP_UTF8, 0, my_sel.c_str(), my_sel.size() + 1, &utf8_my_sel_str[0], utf8_my_sel, NULL, NULL);
			send(g_clientSocket, (char*)&utf8_my_sel, sizeof(utf8_my_sel), 0);
			send(g_clientSocket, utf8_my_sel_str.c_str(), utf8_my_sel, 0);

			//将结构体my_user_new_information里存储的消息发出
			send_new_user_infor(g_clientSocket, my_user_new_information.password);//将用户的新密码发出
			send_new_user_infor(g_clientSocket, my_user_new_information.nickname);//将用户的新昵称发出
			send_new_user_infor(g_clientSocket, my_user_new_information.gender);//将用户的新性别发出
			send_new_user_infor(g_clientSocket, my_user_new_information.age);//将用户的新年龄发出
			send_new_user_infor(g_clientSocket, my_user_new_information.signature);//将用户的新个性签名发出
			MessageBox(NULL, L"已经成功将个人信息更改的请求发往服务器", L"QQ", MB_ICONINFORMATION);

			int recv_len= 0;
			recv(g_clientSocket,(char*)&recv_len,sizeof(recv_len),0);
			std::string recv_str(recv_len,'\0');
			recv(g_clientSocket,&recv_str[0],recv_len,0);
			if (strcmp(recv_str.c_str(), "update_success") == 0)
			{
				MessageBox(NULL, L"服务器已经成功更新用户个人信息", L"QQ", MB_ICONINFORMATION);
				EndDialog(hwndDlg, IDOK);
				return (INT_PTR)TRUE;
			}
			else
			{
				return (INT_PTR)TRUE;
			}
		}
		case IDCANCEL:
		{
			//向服务器发送确定选择
			std::wstring my_sel = L"取消";
			int utf8_my_sel = WideCharToMultiByte(CP_UTF8, 0, my_sel.c_str(), my_sel.size() + 1, NULL, 0, NULL, NULL);
			std::string utf8_my_sel_str(utf8_my_sel, '\0');
			WideCharToMultiByte(CP_UTF8, 0, my_sel.c_str(), my_sel.size() + 1, &utf8_my_sel_str[0], utf8_my_sel, NULL, NULL);
			send(g_clientSocket, (char*)&utf8_my_sel, sizeof(utf8_my_sel), 0);
			send(g_clientSocket, utf8_my_sel_str.c_str(), utf8_my_sel, 0);
			EndDialog(hwndDlg, IDCANCEL);
			return (INT_PTR)TRUE;
		}
		}
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK Dialog_fifteen(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//注销框//
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg,IDC_EDIT45,L"确定注销账号？");
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			std::wstring w_char = L"确定";
			int send_len = WideCharToMultiByte(CP_UTF8, 0, w_char.c_str(), w_char.size() + 1, NULL, 0, NULL, NULL);//先计算转换长度//
			std::string send_str(send_len, 0);//分配空间//
			WideCharToMultiByte(CP_UTF8, 0, w_char.c_str(), w_char.size() + 1, &send_str[0], send_len, NULL, NULL);//实际转换//
			send(g_clientSocket, (char*)&send_len, sizeof(send_len), 0);//先发长度//
			send(g_clientSocket, send_str.c_str(), send_len, 0);//再发内容//
			
			EndDialog(hwndDlg, IDOK);
			return (INT_PTR)TRUE;
		}
		case IDCANCEL:
		{
			std::wstring w_char = L"取消";
			int send_len = WideCharToMultiByte(CP_UTF8, 0, w_char.c_str(), w_char.size() + 1, NULL, 0, NULL, NULL);//先计算转换长度//
			std::string send_str(send_len, 0);//分配空间//
			WideCharToMultiByte(CP_UTF8, 0, w_char.c_str(), w_char.size() + 1, &send_str[0], send_len, NULL, NULL);//实际转换//
			send(g_clientSocket, (char*)&send_len, sizeof(send_len), 0);//先发长度//
			send(g_clientSocket, send_str.c_str(), send_len, 0);//再发内容//

			EndDialog(hwndDlg, IDCANCEL);
			return (INT_PTR)TRUE;
		}

		}
	}
	return (INT_PTR)FALSE;
}


//宽字符串文本发送函数//
void send_wchar(WCHAR* wstr, SOCKET socket, WCHAR* sign)
{
	int sign_len = wcslen(sign);//获取标识的字符长度//
	int utf8_signlen = WideCharToMultiByte(CP_UTF8, 0, sign, sign_len + 1, NULL, 0, NULL, NULL);//计算utf8_signlen字节数,含'\0'//
	std::string sign_str(utf8_signlen, 0);//分配空间，含'\0'//
	WideCharToMultiByte(CP_UTF8, 0, sign, sign_len + 1, &sign_str[0], utf8_signlen, NULL, NULL);//实际转换//
	int wchar_len = wcslen(wstr);//获取实际字符串长度，不包括'\0'//
	int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wstr, wchar_len + 1, NULL, 0, NULL, NULL);//计算utf-8字节数，包括'\0'//
	std::string utf8str(utf8_len, 0);//分配空间//
	//实际转换//
	WideCharToMultiByte(CP_UTF8, 0, wstr, wchar_len + 1, (char*)&utf8str[0], utf8_len, NULL, NULL);
	send(socket, (const char*)&utf8_signlen, sizeof(utf8_signlen), 0);//标识字节长度//
	send(socket, (const char*)sign_str.c_str(), utf8_signlen, 0);//先发标识//
	//先发长度，再发内容//
	send(socket, (const char*)&utf8_len, sizeof(utf8_len), 0);//内容字节长度//
	send(socket, (const char*)utf8str.c_str(), utf8_len, 0);//内容//
}

//头像发送函数//
void send_byte(SOCKET socket, char* g_pImageData, int g_dwImageSize, WCHAR* sign)
{
	int sign_len = wcslen(sign);//获取标识的字符长度//
	int utf8_signlen = WideCharToMultiByte(CP_UTF8, 0, sign, sign_len + 1, NULL, 0, NULL, NULL);//计算utf8_signlen字节数,含'\0'//
	std::string sign_str(utf8_signlen, 0);//分配空间，含'\0'//
	WideCharToMultiByte(CP_UTF8, 0, sign, sign_len + 1, (char*)&sign_str[0], utf8_signlen, NULL, NULL);//实际转换//
	send(socket, (const char*)&utf8_signlen, sizeof(utf8_signlen), 0);//标识字节长度//
	send(socket, (const char*)sign_str.c_str(), utf8_signlen, 0);//先发标识//
	send(socket, (char*)&g_dwImageSize, sizeof(g_dwImageSize), 0);//再发数据字节长度//
	int r = 0;//单次发送的字节数//
	int sum = 0;//累计发送的字节数//
	while (sum < g_dwImageSize)
	{
		r = send(socket, g_pImageData + sum, g_dwImageSize - sum, 0);//再发数据//
		if (r <= 0)
		{
			break;
		}
		sum += r;
	}
	if (sum < g_dwImageSize)
	{
		MessageBox(NULL, L"头像数据发送失败", L"QQ", MB_ICONERROR);
	}
	else if (sum == g_dwImageSize)
	{
		MessageBox(NULL, L"头像数据发送成功", L"QQ", MB_ICONINFORMATION);
	}
}

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE, LPSTR, int nCmdShow)//Winmain主函数//
{
	int result_one = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_ONE), NULL, Dialog_one);
	if (result_one == IDEND)//退出//  //一级判断//
	{
		DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_END), NULL, Dialog_end);//退出框//
	}
	else if (result_one == IDOK)//一级判断//
	{
		WSADATA wsaData;
		int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (result)
		{
			MessageBox(NULL, L"Winsock初始化失败", L"QQ", MB_ICONERROR);
			WSACleanup();
		}
		if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
		{
			MessageBox(NULL, L"不支持Winsock2.2", L"QQ", MB_ICONERROR);
			WSACleanup();
		}
		MessageBox(NULL, L"Winsock初始化成功", L"QQ", MB_ICONINFORMATION);
		INT_PTR result_nine = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_NINE), NULL, Dialog_nine);//IP输入框//
		if (result_nine == IDCANCEL)//退出//  //二级判断//
		{
			DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_END), NULL, Dialog_end);//退出框//
		}
		else if (result_nine == IDOK)//二级判断//
		{
		door_one:
			INT_PTR result_two = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_TWO), NULL, Dialog_two);//登录和注册选择框//
			if (result_two == IDLOGIN)//登录//     //三级判断//
			{
				std::wstring  wmsg = L"登录";
				int wlen = wmsg.size();
				int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wlen + 1, NULL, 0, NULL, NULL);//计算utf8_signlen字节数,含'\0'//
				std::string msg(utf8len, 0);//分配空间//
				WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wlen + 1, &msg[0], utf8len, NULL, NULL);//实际转换//
				send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//向服务器发送信息长度//
				send(g_clientSocket, msg.c_str(), utf8len, 0);//向服务器发送信息内容//
				MessageBox(NULL, L"已向服务器发送登录信息", L"QQ", MB_ICONINFORMATION);
			door_seven:
				INT_PTR result_three = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_THREE), NULL, Dialog_three);//登录对话框（需账号和密码验证）//
				if (result_three == IDOK) //四级判断//
				{
				door_two:
					//没进入登入首页面前，加载用户个人信息
					int result_ten = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_TEN), NULL, Dialog_ten);//登陆界面首页框//
					int result_fifteen = 0;
					int result_eleven = 0;
					int result_twelve = 0;
					int result_thirdteen = 0;
					int result_fourteen = 0;
					switch (result_ten)
					{
					case IDC_EDIT22:
						result_twelve = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_TWELVE), NULL, Dialog_twelve);//聊天室对话框//
						switch (result_twelve)
						{
						case IDOK:
						{
							//MessageBox(NULL, L"还未开发", L"QQ", MB_ICONINFORMATION);
							break;
						}
						case IDCANCEL:
							goto door_two;//登陆界面首页框//
							break;
						}


					case IDC_EDIT23:

						door_11:
						result_thirdteen = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_THIRDTEEN), NULL, Dialog_thirdteen);//好友框//
						switch (result_thirdteen)
						{

						case IDC_CHART://聊天框
						{
							//进入好友对话聊天框
							int xx=DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MYDIALOG_NINETEEN), NULL, Dialog_nineteen);//个人信息框//

							switch (xx)
							{

							case IDCANCEL:
							{
								goto door_11;
							}break;

							}
							break;
						}

						case IDCANCEL://退出框
						{
							goto door_two;//登陆界面首页框//
							break;
						}

						case IDC_FRIEND_INFORMATION://查看好友信息框
						{
							//返回好友框
							goto door_11;
							break;
						}

						case IDC_ADD_FRIEND://添加朋友
						{

							INT_PTR result_20 = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_SIXTEEN), NULL, Dialog_sixteen);

							switch (result_20)
							{
							case IDOK:
							{
								//返回好友框首页
								MessageBox(NULL,L"即将返回好友首页框",L"QQ",MB_ICONINFORMATION);

								goto door_11;
								break;
							}
							case IDCANCEL:
							{
								//返回好友框首页
								MessageBox(NULL, L"即将返回好友首页框", L"QQ",MB_ICONINFORMATION);

								goto door_11;
								break;
							}
							}

							break;
						}

						case IDC_DELETE_FRIEND://删除朋友
						{
							//返回好友框首页
							MessageBox(NULL, L"已经成功将您所选择的好友删除", L"QQ", MB_ICONINFORMATION);
							goto door_11;
							break;
						}

						case IDC_NEW_FRIEND://新朋友
						{
							//显示新朋友框
							INT_PTR result_201 = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_SEVENTEEN), NULL, Dialog_seventeen);
							switch (result_201)
							{
							case IDOK:
							{
								goto door_11;
								break;
							}
							case IDC_REJECT:
							{
								goto door_11;
								break;
							}
							case IDCANCEL:
							{
								//返回好友框首页
								MessageBox(NULL, L"即将返回好友首页框", L"QQ", MB_ICONINFORMATION);
								goto door_11;
								break;
							}
							}

							break;
						}

						}break;


					case IDC_EDIT24:
						result_fourteen = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_FOURTEEN), NULL, Dialog_fourteen);//个人信息框//
						switch (result_fourteen)
						{
						case IDOK:
							//MessageBox(NULL, L"还未开发", L"QQ", MB_ICONINFORMATION);
							goto door_two;//登陆界面首页框//
							break;
						case IDCANCEL:
							goto door_two;//登陆界面首页框//
							break;
						}
					case IDC_EDIT25:
						result_fifteen = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_EIGHT), NULL, Dialog_eight);//更换头像框//
						switch (result_fifteen)
						{
						case IDOK:
						{
							//获取新的头像数据并发送
							int new_img_len = g_dwImageSize;
							send(g_clientSocket, (char*)&new_img_len, sizeof(new_img_len), 0);
							send(g_clientSocket, (char*)g_pImageData,new_img_len,0);
							//接收消息，判断客户端是否已经成功更新用户头像
							int user_new_img_len = 0;
							int user_x=recv(g_clientSocket, (char*)&user_new_img_len,sizeof(user_new_img_len),0);
							if (user_x != sizeof(user_new_img_len))
							{
								MessageBox(NULL,L"接收服务器返回的更新头像长度消息失败",L"QQ",MB_ICONERROR);
							}
							else
							{
								MessageBox(NULL, L"接收服务器返回的更新头像消息长度成功", L"QQ", MB_ICONINFORMATION);
							}
							std::string user_new_img_str(user_new_img_len,'\0');
							recv(g_clientSocket, &user_new_img_str[0], user_new_img_len, 0);
							int my_wchar_len = 0;
							my_wchar_len=MultiByteToWideChar(CP_UTF8,0,user_new_img_str.c_str(),user_new_img_str.size()+1,NULL,0);
							std::wstring my_wchar_str(my_wchar_len,L'\0');
							MultiByteToWideChar(CP_UTF8, 0, user_new_img_str.c_str(), user_new_img_str.size() + 1,&my_wchar_str[0],my_wchar_len);
							if (!my_wchar_str.empty()&&my_wchar_str.back()==L'\0')
							{
								my_wchar_str.pop_back();
							}
							if (wcscmp(my_wchar_str.c_str(),L"服务器已经成功更新用户头像") == 0)
							{
								MessageBox(NULL, L"服务器已经成功更新用户头像", L"QQ", MB_ICONINFORMATION);
							}
							else
							{
								MessageBox(NULL, L"接收服务器返回的更新头像消息失败", L"QQ", MB_ICONINFORMATION);
							}
							goto door_two;//登陆界面首页框//
							break;
						}
						case IDCANCEL:
						{
							goto door_two;//登陆界面首页框//
							break;
						}
						}
					case IDC_EDIT44:
					{
						int result_sixteen = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_FIFTEEN), NULL, Dialog_fifteen);//注销账号框//
						switch (result_sixteen)
						{
						case IDOK:
							MessageBox(NULL, L"已通知数据库进行账号注销", L"QQ", MB_ICONINFORMATION);
							goto door_one;//登陆界面首页框//
							break;
						case IDCANCEL:
							goto door_two;//登陆界面首页框//
							break;
						}break;
					}
					case IDC_EDIT26:
						result_eleven = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_ELEVEN), NULL, Dialog_eleven);//服务器框//
						switch (result_eleven)
						{
						case IDOK:
							//MessageBox(NULL, L"还未开发", L"QQ", MB_ICONINFORMATION);
							break;
						case IDCANCEL:
							goto door_two;//登陆界面首页框//
							break;
						}
					case IDCANCEL:
						DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_END), NULL, Dialog_end);//退出框//
						break;
					case IDC_EDIT43:
						goto door_one;//登陆界面首页框//
						break;
					}

				}
				else if (result_three == IDCANCEL)//四级判断//
				{
					goto door_one;//登录和注册选择框//
				}
				else if (result_three == IDC_EDIT_AGAIN)//四级判断，重新输入账号和密码//
				{
					goto door_seven;
				}
			}
			else if (result_two == IDREGISTER)//注册//   //三级判断//
			{
				std::wstring  wmsg = L"注册";
				int wlen = wmsg.size();
				int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wlen + 1, NULL, 0, NULL, NULL);//计算utf8_signlen字节数,含'\0'//
				std::string msg(utf8len, 0);//分配空间//
				WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wlen + 1, &msg[0], utf8len, NULL, NULL);//实际转换//
				send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//向服务器发送信息长度//
				send(g_clientSocket, msg.c_str(), utf8len, 0);//向服务器发送信息内容//
				MessageBox(NULL, L"已向服务器发送注册信息", L"QQ", MB_ICONINFORMATION);
			door_three:
				INT_PTR result_four = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_FOUR), NULL, Dialog_four);//注册对话框//
				if (result_four == IDGOON)//四级判断//
				{
				door_four:
					INT_PTR result_five = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_FIVE), NULL, Dialog_five);//密码设置框//
					{
						if (result_five == IDFINISH)//五级判断//
						{
						door_five:
							int result_seven = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_SEVEN), NULL, Dialog_seven);//个人信息框//
							if (result_seven == IDCANCEL)//六级判断//
							{
								goto door_four;
							}
							else if (result_seven == IDOK)//六级判断//
							{
							door_six:
								int result_eight = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_EIGHT), NULL, Dialog_eight);//头像上传框//
								if (result_eight == IDCANCEL)//七级判断//
								{
									goto door_five;
								}
								else if (result_eight == IDOK)//七级判断//
								{
									INT_PTR result_six = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_SIX), NULL, Dialog_six);//注册成功框//
									if (result_six == IDOK)//八级判断//
									{
										//开始发送//
										send_wchar(g_szAccount, g_clientSocket, (WCHAR*)L"账号");//账号//
										send_wchar(g_szPassword, g_clientSocket, (WCHAR*)L"密码");//密码//
										send_wchar(g_szNickname, g_clientSocket, (WCHAR*)L"昵称");//昵称//
										send_wchar(g_szGender, g_clientSocket, (WCHAR*)L"性别");//性别//
										send_wchar(g_szAge, g_clientSocket, (WCHAR*)L"年龄");//年龄//
										send_wchar(g_szSignature, g_clientSocket, (WCHAR*)L"个性签名");//个性签名//
										send_byte(g_clientSocket, (char*)g_pImageData, g_dwImageSize, (WCHAR*)L"头像");//头像//
										MessageBox(NULL, L"已将您的注册数据保存到服务器", L"QQ", MB_ICONINFORMATION);
										goto door_one;//返回登录和注册选择框//
									}
									else if (result_six == IDCANCEL)//八级判断//
									{
										goto door_six;
									}
								}
							}
						}
						else if (result_five == IDCANCEL)//五级判断//
						{
							goto door_three;//注册对话框//
						}
					}
				}
				else if (result_four == IDCANCEL)//四级判断//
				{
					goto door_one;//返回登录和注册选择框//
				}
			}
			else if (result_two == IDCANCEL)//退出//   //三级判断//
			{
				std::wstring  wmsg = L"退出";
				int wlen = wmsg.size();
				int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wlen + 1, NULL, 0, NULL, NULL);//计算utf8_signlen字节数,含'\0'//
				std::string msg(utf8len, 0);//分配空间//
				WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wlen + 1, &msg[0], utf8len, NULL, NULL);//实际转换//
				send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//向服务器发送信息长度//
				send(g_clientSocket, msg.c_str(), utf8len, 0);//向服务器发送信息内容//
				MessageBox(NULL, L"已向服务器发送用户退出客户端信息", L"QQ", MB_ICONINFORMATION);
				DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_END), NULL, Dialog_end);//退出框//
			}
		}
	}
	return 0;
}


