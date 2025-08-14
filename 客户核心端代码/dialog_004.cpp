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
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib,"ws2_32.lib")

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
			EndDialog(hwndDlg, IDCANCEL);
			return (INT_PTR)TRUE;
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
		return (INT_PTR)TRUE;
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

INT_PTR CALLBACK Dialog_twelve(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//聊天室对话框//
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			EndDialog(hwndDlg, IDOK);
			return (INT_PTR)TRUE;
		case IDCANCEL:
			EndDialog(hwndDlg, IDCANCEL);
			return (INT_PTR)TRUE;
		}
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK  Dialog_thirdteen(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//好友列表框//
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			EndDialog(hwndDlg, IDOK);
			return (INT_PTR)TRUE;
		case IDCANCEL:
			EndDialog(hwndDlg, IDCANCEL);
			return (INT_PTR)TRUE;
		}
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK Dialog_fourteen(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//登录后个人信息框//
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, IDC_EDIT31, L"账号");
		SetDlgItemText(hwndDlg, IDC_EDIT32, L"密码");
		SetDlgItemText(hwndDlg, IDC_EDIT33, L"昵称");
		SetDlgItemText(hwndDlg, IDC_EDIT34, L"性别");
		SetDlgItemText(hwndDlg, IDC_EDIT35, L"年龄");
		SetDlgItemText(hwndDlg, IDC_EDIT36, L"个性签名");
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			EndDialog(hwndDlg, IDOK);
			return (INT_PTR)TRUE;
		case IDCANCEL:
			EndDialog(hwndDlg, IDCANCEL);
			return (INT_PTR)TRUE;
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
							MessageBox(NULL, L"还未开发", L"QQ", MB_ICONINFORMATION);
							break;
						case IDCANCEL:
							goto door_two;//登陆界面首页框//
							break;
						}break;
					case IDC_EDIT23:
						result_thirdteen = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_THIRDTEEN), NULL, Dialog_thirdteen);//好友框//
						switch (result_thirdteen)
						{
						case IDOK:
							MessageBox(NULL, L"还未开发", L"QQ", MB_ICONINFORMATION);
							break;
						case IDCANCEL:
							goto door_two;//登陆界面首页框//
							break;
						}break;
					case IDC_EDIT24:
						result_fourteen = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_FOURTEEN), NULL, Dialog_fourteen);//个人信息框//
						switch (result_fourteen)
						{
						case IDOK:
							MessageBox(NULL, L"还未开发", L"QQ", MB_ICONINFORMATION);
							break;
						case IDCANCEL:
							goto door_two;//登陆界面首页框//
							break;
						}break;
					case IDC_EDIT25:
						result_fifteen = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_EIGHT), NULL, Dialog_eight);//更换头像框//
						switch (result_fifteen)
						{
						case IDOK:
							MessageBox(NULL, L"还未开发", L"QQ", MB_ICONINFORMATION);
							break;
						case IDCANCEL:
							goto door_two;//登陆界面首页框//
							break;
						}break;
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
							MessageBox(NULL, L"还未开发", L"QQ", MB_ICONINFORMATION);
							break;
						case IDCANCEL:
							goto door_two;//登陆界面首页框//
							break;
						}break;
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
				DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_END), NULL, Dialog_end);//退出框//
			}
		}
	}
	return 0;
}



