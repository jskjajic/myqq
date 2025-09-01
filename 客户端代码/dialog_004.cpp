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


//�洢�û���ͷ������
std::vector<BYTE> user_image_data;
std::string user_nickname_data;

//���������ĺ����ǳƺ�ͷ��
std::string account_recver;
std::string nickname_recver;
std::vector<BYTE> image_recver;

//�洢�µ�¼�û��˺�
std::string new_online_user_account;//��'\0'
std::wstring w_new_online_user_account;

//�����û�������Ϣʱ�洢�µ�¼�û��ĸ�����Ϣ
struct w_new_user_information
{
	std::wstring password;//limit 32
	std::wstring nickname;//limit 32
	std::wstring gender;//limit �� or Ů
	std::wstring age;//limit 8
	std::wstring signature;//limit 128
};


//��¼ʱ�����µ�¼�û��ĸ�����Ϣ
struct my_user_information
{
	std::string account;
	std::string password;//limit 32
	std::string nickname;//limit 32
	std::string gender;//limit �� or Ů
	std::string age;//limit 8
	std::string signature;//limit 128
} old_my_user_information,xxy;


//��¼ʱ�����µ�¼�û��ĸ�����Ϣ
struct w_my_user_information
{
	std::wstring password;//limit 32
	std::wstring nickname;//limit 32
	std::wstring gender;//limit �� or Ů
	std::wstring age;//limit 8
	std::wstring signature;//limit 128
} w_old_my_user_information;



// ȫ�ֱ���
SOCKET g_clientSocket = INVALID_SOCKET; // ȫ���׽��֣���������
bool g_isConnected = false; // ����״̬��־
WCHAR g_szAccount[32] = { 0 }; // �˺�
WCHAR g_szPassword[32] = { 0 }; // ����
WCHAR g_szNickname[32] = { 0 }; // �ǳ�
WCHAR g_szGender[16] = { 0 }; // �Ա�
WCHAR g_szAge[8] = { 0 }; // ����
WCHAR g_szSignature[128] = { 0 }; // ����ǩ��
BYTE* g_pImageData = NULL;//����ͷ��Ķ���������//
DWORD g_dwImageSize = 0;//���ݴ�С//
int passwd_back_to_account = 0;//�˺��Ƿ��Ѳ�������ֹ��������ʱ������ȡ�������ص��˺����ɿ�����������˺�//
INT_PTR CALLBACK Dialog_one(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//��ʼ���Ի���//
{
	static UINT_PTR TimeID = 0;//���ö�ʱ��//
	static HBRUSH hBackgroundBrush = NULL;//������ˢ//
	static HBITMAP hBmp = NULL;//λͼ���//
	switch (uMsg)
	{
	case WM_INITDIALOG:
		//SetDlgItemText(hwndDlg, IDC_EDIT3, L"Welcome to QQ");//��һ���Ի��������//
		TimeID = SetTimer(hwndDlg, 1, 4000, NULL);//��ʾ��һ���Ի�������ݣ��ӳ�3��//
		hBackgroundBrush = CreateSolidBrush((RGB(200, 230, 255)));//���ñ���Ϊ����ɫ//
		hBmp = (HBITMAP)LoadImage(NULL, L"image_one.bmp", IMAGE_BITMAP, 200, 170, LR_LOADFROMFILE);//����ͼƬλ��//
		if (hBmp)
		{
			SendDlgItemMessage(hwndDlg, IDC_MY_IMAGE_ONE, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmp);
		}
		else
		{
			MessageBox(hwndDlg, L"image lost", L"QQ", MB_ICONERROR);
		}
		return (INT_PTR)TRUE;//���ý���//
	case WM_DRAWITEM: {
		DRAWITEMSTRUCT* pdis = (DRAWITEMSTRUCT*)lParam;
		if (pdis->CtlID == IDC_CUSTOM_TEXT) {
			SetTextColor(pdis->hDC, RGB(255, 0, 0));
			DrawText(pdis->hDC, L"Welcome to QQ", -1, &pdis->rcItem,//�Ի������//
				DT_CENTER | DT_VCENTER);
		}
		return (INT_PTR)TRUE;
	}
	case WM_CTLCOLORDLG:
		return (INT_PTR)hBackgroundBrush;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDSTART)
		{
			EndDialog(hwndDlg, IDOK);//�رնԻ���//
		}
		return (INT_PTR)TRUE;
		/*
			case WM_TIMER:
				if (wParam == 1)
				{
					EndDialog(hwndDlg, IDOK);//�رնԻ���//
				}
				return (INT_PTR)TRUE;
		*/
	case WM_DESTROY:
		KillTimer(hwndDlg, TimeID);//���ٶ�ʱ��//
		DeleteObject(hBackgroundBrush);//�ͷŻ�ˢ//
		DeleteObject(hBmp);//�ͷ�λͼ��Դ//
		EndDialog(hwndDlg, IDCANCEL);//�رնԻ���//
		break;
	case WM_CLOSE:
		EndDialog(hwndDlg, IDEND);//�رնԻ���//
		break;
	}

	return (INT_PTR)FALSE;
}


INT_PTR CALLBACK Dialog_end(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//�����Ի���//
{
	static UINT_PTR TimeID = 0;//���ö�ʱ��//
	static HBRUSH hBackgroundBrush = NULL;//������ˢ//
	static HBITMAP hBmp = NULL;//λͼ���//
	switch (uMsg)
	{
	case WM_INITDIALOG:
		TimeID = SetTimer(hwndDlg, 1, 4000, NULL);//��ʾ��һ���Ի�������ݣ��ӳ�3��//
		hBackgroundBrush = CreateSolidBrush((RGB(200, 230, 255)));//���ñ���Ϊ����ɫ//
		return (INT_PTR)TRUE;//���ý���//
	case WM_DRAWITEM: {
		DRAWITEMSTRUCT* pdis = (DRAWITEMSTRUCT*)lParam;
		if (pdis->CtlID == IDC_CUSTOM_TEXT_TWO) {
			SetTextColor(pdis->hDC, RGB(255, 0, 0));
			DrawText(pdis->hDC, L"Goodbye", -1, &pdis->rcItem,//�Ի������//
				DT_CENTER | DT_VCENTER);
		}
		return (INT_PTR)TRUE;
	}
	case WM_CTLCOLORDLG:
		return (INT_PTR)hBackgroundBrush;
	case WM_TIMER:
		if (wParam == 1)
		{
			KillTimer(hwndDlg, TimeID);//���ٶ�ʱ��//
			DeleteObject(hBackgroundBrush);//�ͷŻ�ˢ//
			EndDialog(hwndDlg, IDOK);//�رնԻ���//
		}
		return (INT_PTR)TRUE;
	case WM_CLOSE:
		EndDialog(hwndDlg, IDEND);//�رնԻ���//
		exit(0);
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK Dialog_two(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//��¼��ע��ѡ���//
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

//����¼���е��˺����뷢��//
void my_send_one(SOCKET socket, WCHAR* wstr)
{
	int wlen = wcslen(wstr);//��ȡ���ַ������ַ���,����'\0'//
	int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wstr, wlen + 1, NULL, 0, NULL, NULL);//��ȡת�����utf8���ȣ�����'\0'//
	std::string utf8str(utf8_len, 0);//����ռ�//
	WideCharToMultiByte(CP_UTF8, 0, wstr, wlen + 1, &utf8str[0], utf8_len, NULL, NULL);//ʵ��ת��//
	send(socket, (char*)&utf8_len, sizeof(utf8_len), 0);//�ȷ�����//
	send(socket, utf8str.c_str(), utf8_len, 0);//�ٷ�����//
}


INT_PTR CALLBACK Dialog_three(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//��¼��//
{
	static bool EDIT1 = FALSE;
	static bool EDIT2 = FALSE;
	HWND hEdit = NULL;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, IDC_EDIT3, L"�˺�");
		SetDlgItemText(hwndDlg, IDC_EDIT4, L"����");
		SetDlgItemText(hwndDlg, IDC_EDIT1, L"�˺�");
		SetDlgItemText(hwndDlg, IDC_EDIT2, L"����");
		SendMessage(GetDlgItem(hwndDlg, IDC_EDIT1), WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
		SendMessage(GetDlgItem(hwndDlg, IDC_EDIT2), WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
		return (INT_PTR)TRUE;
	case WM_CTLCOLOREDIT://����ˮӡ����//
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
					SetDlgItemText(hwndDlg, IDC_EDIT1, L"�˺�");
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
					SetDlgItemText(hwndDlg, IDC_EDIT2, L"����");
					EDIT2 = FALSE;
				}
			}break;
		case IDOK:
			//��������֤�˺��������Ч��//
		{
			std::wstring  wmsg = L"ȷ��";
			int wcharlen = wmsg.size();
			int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, NULL, 0, NULL, NULL);//����utf8_signlen�ֽ���,��'\0'//
			std::string msg(utf8len, 0);//����ռ�//
			WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, &msg[0], utf8len, NULL, NULL);//ʵ��ת��//
			send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//�������������Ϣ����//
			send(g_clientSocket, msg.c_str(), utf8len, 0);//�������������Ϣ����//

			MessageBox(NULL, L"���ȷ�ϣ�����������֤�˺��������Ч��", L"QQ", MB_ICONINFORMATION);
			WCHAR account[16];
			WCHAR password[64];
			GetDlgItemText(hwndDlg, IDC_EDIT1, account, 16);//���˺ű���,�������ַ���ĩβ���һ��'\0'//
			
			//���û��˺ű���
			int wchar_account_len = wcslen(account);
			w_new_online_user_account.resize(wchar_account_len+1);
			wcscpy_s(&w_new_online_user_account[0],wchar_account_len+1 ,account);

			int utf8_new_user_account_len = WideCharToMultiByte(CP_UTF8,0,account, wchar_account_len + 1, NULL, 0, NULL, NULL);
			std::string utf8_new_user_account_str(utf8_new_user_account_len,'\0');
			WideCharToMultiByte(CP_UTF8, 0, account, wchar_account_len+1, &utf8_new_user_account_str[0], utf8_new_user_account_len, NULL, NULL);
			new_online_user_account = utf8_new_user_account_str;//��'\0'

			GetDlgItemText(hwndDlg, IDC_EDIT2, password, 64);//�����뱣��,�������ַ���ĩβ���һ��'\0'//
			my_send_one(g_clientSocket, account);//�����˺�//
			my_send_one(g_clientSocket, password);//��������//
			int recv_len = 0;
			recv(g_clientSocket, (char*)&recv_len, sizeof(recv_len), 0);//������֤��Ϣ����//
			if (recv_len <= 0)
			{
				MessageBox(NULL, L"�����˺ź��������֤��Ϣ����ʧ��", L"QQ", MB_ICONERROR);
				exit(1);
			}
			std::string recvchar(recv_len, 0);
			recv(g_clientSocket, &recvchar[0], recv_len, 0);//������֤��ʶ//
			int wlen = MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, NULL, 0);
			std::wstring wstr(wlen, 0);//����ռ�//
			MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, &wstr[0], wlen);//ʵ��ת��//
			if (wcscmp(wstr.c_str(), L"faile") == 0)
			{
				MessageBox(NULL, L"���˺ź�����δע����˺ź��������������ע�����������", L"QQ", MB_ICONERROR);
				//SetDlgItemText(hwndDlg, IDC_EDIT1, L"");//����˺ſ�//
				//SetDlgItemText(hwndDlg, IDC_EDIT2, L"");//��������//
				EndDialog(hwndDlg, IDC_EDIT_AGAIN);
				return (INT_PTR)TRUE;
			}
			else if (wcscmp(wstr.c_str(), L"sucess") == 0)
			{
				MessageBox(NULL, L"��¼�ɹ�", L"QQ", MB_ICONINFORMATION);

				EndDialog(hwndDlg, IDOK);
				return (INT_PTR)TRUE;
			}
			else
			{
				MessageBox(NULL, L"δ���յ���֤��Ϣ", L"QQ", MB_ICONINFORMATION);
				exit(1);
			}

		}
		case IDCANCEL:
			std::wstring  wmsg = L"ȡ��";
			int wlen = wmsg.size();
			int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wlen + 1, NULL, 0, NULL, NULL);//����utf8_signlen�ֽ���,��'\0'//
			std::string msg(utf8len, 0);//����ռ�//
			WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wlen + 1, &msg[0], utf8len, NULL, NULL);//ʵ��ת��//
			send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//�������������Ϣ����//
			send(g_clientSocket, msg.c_str(), utf8len, 0);//�������������Ϣ����//
			EndDialog(hwndDlg, IDCANCEL);
			return (INT_PTR)TRUE;
		}
	}
	return (INT_PTR)FALSE;
}



int Generate_register_number()//�����˺�//
{
	HCRYPTPROV hProvider = 0;
	CryptAcquireContext(&hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
	long long randomNum = 0;
	CryptGenRandom(hProvider, sizeof(randomNum), (BYTE*)&randomNum);//randomNum���ղ����������//
	CryptReleaseContext(hProvider, 0);
	int num = abs(randomNum % 9000000000) + 1000000000; // ת��Ϊ10λ��
	if (num > 0)
		return num;
	else if (num <= 0)
		return -num;
}

INT_PTR CALLBACK Dialog_four(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//ע���//
{
	static long long num = 0;
	DRAWITEMSTRUCT* pdis = NULL;
	CString str;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, IDC_EDIT5, L"���������������˺�");
		if (passwd_back_to_account == 0)
		{
			num = Generate_register_number();
		}
		str.Format(L"%lld", num);
		wcscpy_s(g_szAccount, 32, str);//���˺ű��浽ȫ�ֱ���g_szAccount//
		SetDlgItemText(hwndDlg, IDC_EDIT6, str);
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDGOON:
		{
			std::wstring  wmsg = L"����";
			int wcharlen = wmsg.size();
			int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, NULL, 0, NULL, NULL);//����utf8_signlen�ֽ���,��'\0'//
			std::string msg(utf8len, 0);//����ռ�//
			WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, &msg[0], utf8len, NULL, NULL);//ʵ��ת��//
			send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//�������������Ϣ����//
			send(g_clientSocket, msg.c_str(), utf8len, 0);//�������������Ϣ����//

			EndDialog(hwndDlg, IDGOON);
			return (INT_PTR)TRUE;
		}
		case IDCANCEL:
		{
			std::wstring  wmsg = L"ȡ��";
			int wcharlen = wmsg.size();
			int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, NULL, 0, NULL, NULL);//����utf8_signlen�ֽ���,��'\0'//
			std::string msg(utf8len, 0);//����ռ�//
			WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, &msg[0], utf8len, NULL, NULL);//ʵ��ת��//
			send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//�������������Ϣ����//
			send(g_clientSocket, msg.c_str(), utf8len, 0);//�������������Ϣ����//

			EndDialog(hwndDlg, IDCANCEL);
			return (INT_PTR)TRUE;
		}
		}
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK Dialog_five(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//�������ÿ�//
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, IDC_EDIT9, L"��������");
		SetDlgItemText(hwndDlg, IDC_EDIT10, L"����ȷ��");
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDFINISH:
		{
			// ��ȡ���벢��֤
			WCHAR szPassword1[32], szPassword2[32];
			GetDlgItemText(hwndDlg, IDC_EDIT7, szPassword1, 32);
			GetDlgItemText(hwndDlg, IDC_EDIT8, szPassword2, 32);
			if (szPassword1[0] == '\0')
			{
				MessageBox(NULL, L"���벻��Ϊ��", L"QQ", MB_ICONERROR);
				SetFocus(GetDlgItem(hwndDlg, IDC_EDIT7));
				return (INT_PTR)TRUE;
			}
			if (wcscmp(szPassword1, szPassword2) != 0)//�Ƚ�ǰ���������õ������Ƿ���ͬ//
			{
				MessageBox(hwndDlg, L"������������벻һ�£�����������", L"QQ", MB_ICONERROR);
				SetDlgItemText(hwndDlg, IDC_EDIT8, L"");//�������//
				SetFocus(GetDlgItem(hwndDlg, IDC_EDIT8));
				return (INT_PTR)TRUE;
			}

			std::wstring  wmsg = L"���";
			int wcharlen = wmsg.size();
			int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, NULL, 0, NULL, NULL);//����utf8_signlen�ֽ���,��'\0'//
			std::string msg(utf8len, 0);//����ռ�//
			WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, &msg[0], utf8len, NULL, NULL);//ʵ��ת��//
			MessageBox(NULL, L"�����������������������ж� ��ɻ�ȡ��", L"QQ", MB_ICONINFORMATION);
			send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//�������������Ϣ����//
			send(g_clientSocket, msg.c_str(), utf8len, 0);//�������������Ϣ����//

			// �������뵽ȫ�ֱ���
			wcscpy_s(g_szPassword, szPassword1);
			MessageBox(hwndDlg, L"�����ѳɹ�����", L"QQ", MB_ICONINFORMATION);
			EndDialog(hwndDlg, IDFINISH);
			return (INT_PTR)TRUE;
		}
		case IDCANCEL:
		{
			std::wstring  wmsg = L"ȡ��";
			int wcharlen = wmsg.size();
			int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, NULL, 0, NULL, NULL);//����utf8_signlen�ֽ���,��'\0'//
			std::string msg(utf8len, 0);//����ռ�//
			WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, &msg[0], utf8len, NULL, NULL);//ʵ��ת��//
			send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//�������������Ϣ����//
			send(g_clientSocket, msg.c_str(), utf8len, 0);//�������������Ϣ����//

			passwd_back_to_account = 1;//�˺������ɱ�־//
			EndDialog(hwndDlg, IDCANCEL);
			return (INT_PTR)TRUE;
		}
		}
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK Dialog_six(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//ע��ɹ���//
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, IDC_EDIT11, L"ע��ɹ�,���¡�ȷ�ϡ�������ת������¼����.");
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{

			std::wstring  wmsg = L"ȷ��";
			int wcharlen = wmsg.size();
			int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, NULL, 0, NULL, NULL);//����utf8_signlen�ֽ���,��'\0'//
			std::string msg(utf8len, 0);//����ռ�//
			WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, &msg[0], utf8len, NULL, NULL);//ʵ��ת��//
			send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//�������������Ϣ����//
			send(g_clientSocket, msg.c_str(), utf8len, 0);//�������������Ϣ����//

			EndDialog(hwndDlg, IDOK);
			return (INT_PTR)TRUE;
		}
		case IDCANCEL:
		{
			std::wstring  wmsg = L"ȡ��";
			int wcharlen = wmsg.size();
			int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, NULL, 0, NULL, NULL);//����utf8_signlen�ֽ���,��'\0'//
			std::string msg(utf8len, 0);//����ռ�//
			WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, &msg[0], utf8len, NULL, NULL);//ʵ��ת��//
			send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//�������������Ϣ����//
			send(g_clientSocket, msg.c_str(), utf8len, 0);//�������������Ϣ����//

			EndDialog(hwndDlg, IDCANCEL);
			return (INT_PTR)TRUE;
		}
		}
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK Dialog_seven(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//ע��ʱ������Ϣ��//
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, IDC_EDIT19, L"�ǳ�");
		SetDlgItemText(hwndDlg, IDC_EDIT12, L"�Ա�");
		SetDlgItemText(hwndDlg, IDC_EDIT13, L"����");
		SetDlgItemText(hwndDlg, IDC_EDIT14, L"����ǩ��");
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			// ��ȡ���и�����Ϣ
			GetDlgItemText(hwndDlg, IDC_EDIT15, g_szNickname, 32);//���ǳƱ��浽ȫ�ֱ���g_szNickname�������ַ���ĩβ���һ��'\0'//
			GetDlgItemText(hwndDlg, IDC_EDIT16, g_szGender, 16);//���Ա𱣴浽ȫ�ֱ���g_szGender//
			GetDlgItemText(hwndDlg, IDC_EDIT17, g_szAge, 8);//�����䱣�浽ȫ�ֱ���g_szAge//
			GetDlgItemText(hwndDlg, IDC_EDIT18, g_szSignature, 128);//������ǩ�����浽ȫ�ֱ���g_szNickname//
			if (g_szNickname[0] == '\0')
			{
				MessageBox(NULL, L"�ǳ�δ����", L"QQ", MB_ICONERROR);
				SetFocus(GetDlgItem(hwndDlg, IDC_EDIT19));
				return (INT_PTR)TRUE;
			}
			if (g_szGender[0] == '\0' || (wcscmp(g_szGender, L"��") != 0) && (wcscmp(g_szGender, L"Ů") != 0))
			{
				if (g_szGender[0] == '\0')
					MessageBox(NULL, L"�Ա�δ����", L"QQ", MB_ICONERROR);
				else if (wcscmp(g_szGender, L"��") != 0 && wcscmp(g_szGender, L"Ů") != 0)
					MessageBox(NULL, L"�Ա����ô����Ա�ֻ���ǡ��С����ߡ�Ů'", L"QQ", MB_ICONERROR);
				SetFocus(GetDlgItem(hwndDlg, IDC_EDIT12));
				return (INT_PTR)TRUE;
			}
			if (g_szAge[0] == '\0')
			{
				MessageBox(NULL, L"����δ����", L"QQ", MB_ICONERROR);
				SetFocus(GetDlgItem(hwndDlg, IDC_EDIT13));
				return (INT_PTR)TRUE;
			}
			if (g_szSignature[0] == '\0')
			{
				MessageBox(NULL, L"����ǩ��δ����", L"QQ", MB_ICONERROR);
				SetFocus(GetDlgItem(hwndDlg, IDC_EDIT14));
				return (INT_PTR)TRUE;
			}
			MessageBox(NULL, L"������Ϣ׼����ϣ����������������ȷ����Ϣ", L"QQ", MB_ICONINFORMATION);
			std::wstring  wmsg = L"ȷ��";
			int wcharlen = wmsg.size();
			int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, NULL, 0, NULL, NULL);//����utf8_signlen�ֽ���,��'\0'//
			std::string msg(utf8len, 0);//����ռ�//
			WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, &msg[0], utf8len, NULL, NULL);//ʵ��ת��//
			send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//�������������Ϣ����//
			send(g_clientSocket, msg.c_str(), utf8len, 0);//�������������Ϣ����//

			// �ٴλ�ȡ���и�����Ϣ
			GetDlgItemText(hwndDlg, IDC_EDIT15, g_szNickname, 32);//���ǳƱ��浽ȫ�ֱ���g_szNickname//
			GetDlgItemText(hwndDlg, IDC_EDIT16, g_szGender, 16);//���Ա𱣴浽ȫ�ֱ���g_szGender//
			GetDlgItemText(hwndDlg, IDC_EDIT17, g_szAge, 8);//�����䱣�浽ȫ�ֱ���g_szAge//
			GetDlgItemText(hwndDlg, IDC_EDIT18, g_szSignature, 128);//������ǩ�����浽ȫ�ֱ���g_szNickname//
			EndDialog(hwndDlg, IDOK);
			return (INT_PTR)TRUE;
		}
		case IDCANCEL:

			std::wstring  wmsg = L"ȡ��";
			int wcharlen = wmsg.size();
			int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, NULL, 0, NULL, NULL);//����utf8_signlen�ֽ���,��'\0'//
			std::string msg(utf8len, 0);//����ռ�//
			WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, &msg[0], utf8len, NULL, NULL);//ʵ��ת��//
			send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//�������������Ϣ����//
			send(g_clientSocket, msg.c_str(), utf8len, 0);//�������������Ϣ����//
			EndDialog(hwndDlg, IDCANCEL);
			return (INT_PTR)TRUE;
		}
	}
	return (INT_PTR)FALSE;
}

WCHAR g_szPhotoPath[MAX_PATH] = L"";
HBITMAP g_hBitmap = NULL;//HBITMAP ��λͼ�ľ������//
HBITMAP LoadImageFileToBitmap(HWND hwnd, LPCWSTR path, int width, int height)//����ͷ��,LPCWSTRָ�������ַ��ַ����ĳ�ָ��//
{
	
	using namespace Gdiplus;
	static ULONG_PTR gdiplusToken = 0;
	if (!gdiplusToken) //���GDI+�Ƿ��ʼ��//
	{
		GdiplusStartupInput gdiplusStartupInput;
		GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	}
	
	Bitmap bmp(path);//����ͼƬ//
	if (bmp.GetLastStatus() != Ok) //���bmp������һ�εĲ���״̬�Ƿ�ɹ���������ͼƬ�Ƿ�ɹ�//
		return NULL;
	// ���ŵ��ؼ���С
	Bitmap* pThumb = new Bitmap(width, height, bmp.GetPixelFormat());//bmp.GetPixelFormat��ȡλͼ�����ظ�ʽ//
	Graphics g(pThumb);//Graphics��GDI+�ĺ��Ļ�ͼ�࣬��װ�����л�ͼ������g�Ƕ����Graphics�Ķ�����;pThumb:ָ��Bitmap�����ָ��//
	g.DrawImage(&bmp, 0, 0, width, height);//����ͼ�����//
	HBITMAP hBmp = NULL;
	pThumb->GetHBITMAP(Color(200, 230, 255), &hBmp);
	delete pThumb;
	//��ȡ�ļ�//
	HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		MessageBox(NULL, L"��ȡͷ���ļ�ʧ��", L"QQ", MB_ICONERROR);
		return hBmp;
	}
	else
	{
		g_dwImageSize = GetFileSize(hFile, NULL);//��ȡ���ݴ�С//
		g_pImageData = new BYTE[g_dwImageSize];
		DWORD dwREAD;
		ReadFile(hFile, g_pImageData, g_dwImageSize, &dwREAD, NULL);//��ͼƬ�ļ��洢��g_pImageData��//
		CloseHandle(hFile);//�ͷ��ļ����//
	}
	return hBmp;
}

INT_PTR CALLBACK Dialog_eight(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//ͷ���ϴ���//
{
	OPENFILENAME ofn = { 0 };//��ʼ���ļ��Ի���//
	WCHAR szFile[MAX_PATH] = L"";//ͷ�ļ�<windows.h>������MAX_PATH�Ķ���//
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
			ofn.lpstrFilter = L"ͼƬ�ļ� (*.jpg;*.png)\0*.jpg;*.png\0�����ļ� (*.*)\0*.*\0";
			ofn.lpstrFile = szFile;//����ͼƬ·��//
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
			if (GetOpenFileName(&ofn))
			{
				// �����չ��
				std::wstring path = szFile;//��ȡͼƬ��·��//
				auto pos = path.find_last_of(L'.');//���ַ����дӺ���ǰ�������һ��.�ַ���λ��//
				if (pos == std::wstring::npos) {
					MessageBox(hwndDlg, L"�ļ�����չ�����޷�ʶ��", L"QQ", MB_OK | MB_ICONERROR);
					break;
				}
				std::wstring ext = path.substr(pos);//��ȡ��չ��//
				std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);//����չ�����ַ�ת��ΪСд��towlower��Ҫ���еĲ���//
				if (ext != L".jpg" && ext != L".png") {
					MessageBox(hwndDlg, L"��֧��jpg��png", L"QQ", MB_OK | MB_ICONWARNING);
					break;
				}
				// ����·��
				wcsncpy_s(g_szPhotoPath, szFile, _TRUNCATE);//��szFile���·�����浽g_szPhotoPath,_TRUNCATE����ض�·������ֹ���Ŀ�껺����//

				// ���ز���ʾ����̬�ؼ�
				if (g_hBitmap)
				{
					DeleteObject(g_hBitmap);
					g_hBitmap = NULL;
				}
				RECT rc;//rc����������Ļ����//
				GetWindowRect(GetDlgItem(hwndDlg, IDC_MY_IMAGE_TWO), &rc);//��ȡͼƬ�ؼ���λ�úͳߴ磬�����浽rc��//
				MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rc, 2);//��rc������ת��Ϊ��������//
				int w = rc.right - rc.left, h = rc.bottom - rc.top;//��ȡͼƬ�Ŀ�͸�//
				g_hBitmap = LoadImageFileToBitmap(hwndDlg, g_szPhotoPath, w, h);
				if (g_hBitmap)
					SendDlgItemMessage(hwndDlg, IDC_MY_IMAGE_TWO, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)g_hBitmap);//��Ի���ָ���ؼ�������Ϣ�������Ǹ���ͼƬ�ؼ�//
				else
					MessageBox(hwndDlg, L"ͼƬ����ʧ��", L"QQ", MB_OK | MB_ICONERROR);
			}
			return (INT_PTR)TRUE;
		}
		case IDOK:
		{
			if (wcslen(g_szPhotoPath) == 0)
			{
				MessageBox(hwndDlg, L"�����ϴ�ͷ��", L"QQ", MB_OK | MB_ICONWARNING);
				break;
			}
			std::wstring  wmsg = L"ȷ��";
			int wcharlen = wmsg.size();
			int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, NULL, 0, NULL, NULL);//����utf8_signlen�ֽ���,��'\0'//
			std::string msg(utf8len, 0);//����ռ�//
			WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, &msg[0], utf8len, NULL, NULL);//ʵ��ת��//
			send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//�������������Ϣ����//
			send(g_clientSocket, msg.c_str(), utf8len, 0);//�������������Ϣ����//
			

			
			EndDialog(hwndDlg, IDOK);
			return (INT_PTR)TRUE;
		}
		case IDCANCEL:
		{
			std::wstring  wmsg = L"ȡ��";
			int wcharlen = wmsg.size();
			int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, NULL, 0, NULL, NULL);//����utf8_signlen�ֽ���,��'\0'//
			std::string msg(utf8len, 0);//����ռ�//
			WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wcharlen + 1, &msg[0], utf8len, NULL, NULL);//ʵ��ת��//
			send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//�������������Ϣ����//
			send(g_clientSocket, msg.c_str(), utf8len, 0);//�������������Ϣ����//

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

//IP��ʽ��֤//
bool IsValidIP(const WCHAR* ip)
{
	char ipAnsi[16];
	// �����ַ�ת��ΪANSI
	WideCharToMultiByte(CP_ACP, 0, ip, -1, ipAnsi, 16, NULL, NULL);
	struct in_addr addr;
	// ����ת��IP��ַ
	return (inet_pton(AF_INET, ipAnsi, &addr) == 1);
}

INT_PTR CALLBACK Dialog_nine(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//IP�����//
{
	HANDLE hThread = NULL;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, IDC_EDIT20, L"������IP");
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			WCHAR IP[16];
			GetDlgItemText(hwndDlg, IDC_EDIT21, IP, 16);//��ȡIP��ַ���洢��IP��������//
			if (!IsValidIP(IP))//IP��ʽ��֤//
			{
				MessageBox(hwndDlg, L"IP��ʽ����", L"QQ", MB_ICONERROR);
				SetDlgItemText(hwndDlg, IDC_EDIT21, L"");//�������IP//
				SetFocus(GetDlgItem(hwndDlg, IDC_EDIT21));
				return (INT_PTR)TRUE;
			}
			MessageBox(hwndDlg, L"IP��ʽ��ȷ", L"QQ", MB_ICONINFORMATION);
			// ����ȷ����ť����ֹ�ظ����
			EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
			//���������̣߳�ִ����������//
			DWORD threadId;
			hThread = CreateThread(
				NULL, 0,
				[](LPVOID param)->DWORD {
					HWND hwnd = (HWND)param;
					g_clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//�����׽���//
					if (g_clientSocket == INVALID_SOCKET)
					{
						MessageBox(hwnd, L"�����׽���ʧ��", L"QQ", MB_ICONERROR);
						return 1;
					}
					MessageBox(hwnd, L"�����׽��ֳɹ�", L"QQ", MB_ICONINFORMATION);
					//���÷�������ַ//
					sockaddr_in serverAddr;
					serverAddr.sin_family = AF_INET;
					serverAddr.sin_port = htons(8080);//�������˿�//
					WCHAR IP[16];
					GetDlgItemText(hwnd, IDC_EDIT21, IP, 16);//�ӶԻ����ȡIP��ַ//
					char ipAnsi[16];
					WideCharToMultiByte(CP_UTF8, 0, IP, -1, ipAnsi, 16, NULL, NULL);//IP��ַת��Ϊchar����//
					inet_pton(AF_INET, ipAnsi, &serverAddr.sin_addr);
					if (connect(g_clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) //���ӷ�����//
					{
						int error = WSAGetLastError();
						switch (error)
						{
						case WSAECONNREFUSED:
							MessageBox(hwnd, L"���ӱ��ܾ�", L"QQ", MB_ICONERROR);
							break;
						case WSAENETUNREACH:
							MessageBox(hwnd, L"���粻�ɴ�", L"QQ", MB_ICONERROR);
							break;
						case WSAEHOSTDOWN:
							MessageBox(hwnd, L"�������ɴ�", L"QQ", MB_ICONERROR);
							break;
						case WSAETIMEDOUT:
							MessageBox(hwnd, L"���ӳ�ʱ", L"QQ", MB_ICONERROR);
							break;
						default:
							MessageBox(hwnd, L"���ӷ�����ʧ��", L"QQ", MB_ICONERROR);
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
				CloseHandle(hThread);//�ͷ��߳̾��//
			}
			return (INT_PTR)TRUE;
		case IDCANCEL:
			EndDialog(hwndDlg, IDCANCEL);//ʵ�ʷ��ز���ִ�б�־//
			return (INT_PTR)TRUE;//���᷵�ز���ִ�б�־//
		}break;
	case WM_CONNECT_SUCCESS:
	{
		MessageBox(hwndDlg, L"���������ӳɹ�", L"QQ", MB_ICONINFORMATION);
		//�����ַ�//
		 /*
		 std::wstring msg = L"��ã���������";
		 int slen=WideCharToMultiByte(CP_UTF8,0,msg.c_str(),msg.size(), NULL, 0, NULL, NULL);//����ת���ֽ���//
		 std::string s(slen, 0);//����ռ�//
		 WideCharToMultiByte(CP_UTF8, 0, msg.c_str(),msg.size(), &s[0], slen, NULL, NULL);//����ת���ֽ���//
		 send(g_clientSocket, (char*)&slen,sizeof(slen),0);//�ȷ��ͳ���//
		 send(g_clientSocket, (const char*)s.c_str(),slen, 0);//�ٷ�������//
		 */
		EndDialog(hwndDlg, IDOK);
		return (INT_PTR)TRUE;
	}
	case WM_CONNECT_FAILED:
		EnableWindow(GetDlgItem(hwndDlg, IDOK), TRUE); // ��������ȷ����ť
		return (INT_PTR)TRUE;
	}
	return (INT_PTR)FALSE;
}


INT_PTR CALLBACK Dialog_ten(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//��¼������ҳ��//
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{

		//����������ͼ����û�ͷ�������
		std::wstring Img_info = L"������ص�¼�û�ͷ��";
		int utf8_len = WideCharToMultiByte(CP_UTF8, 0, Img_info.c_str(), Img_info.size() + 1, NULL, 0, NULL, NULL);
		std::string  utf8_request(utf8_len,'\0');
		WideCharToMultiByte(CP_UTF8,0, Img_info.c_str(), Img_info.size() + 1, &utf8_request[0], utf8_len, NULL, NULL);
		send(g_clientSocket, (char*)&utf8_len, sizeof(utf8_len), 0);
		send(g_clientSocket, utf8_request.c_str(), utf8_len, 0);
		MessageBox(NULL, L"�Ѿ��ɹ�����������ͼ���ͷ�����ݵ�����", L"QQ", MB_ICONINFORMATION);
		//���շ������������û�ͷ������
		int recv_img_len = 0;
		int size_x=recv(g_clientSocket,(char*)&recv_img_len,sizeof(recv_img_len),0);
		if (size_x != sizeof(recv_img_len))
		{
			MessageBox(NULL, L"����ͷ�����ݳ���ʧ��", L"QQ", MB_ICONERROR);
			return (INT_PTR)TRUE;
		}
		MessageBox(NULL, L"����ͷ�����ݳ��ȳɹ�", L"QQ", MB_ICONINFORMATION);
		BYTE* user_img = new BYTE[recv_img_len];//����ͼƬ���ݴ�С
		int r = 0;//ÿ�ν��յ�����
		int sum = 0;//�ܽ�������
		while (sum< recv_img_len)
		{
			r=recv(g_clientSocket, (char*)user_img+sum, recv_img_len-sum, 0);
			if(r>0)
			{
			  sum += r;
		    }
			if (sum == recv_img_len)
			{
				MessageBox(NULL, L"�����û�ͼ�����ݳɹ�", L"QQ", MB_ICONINFORMATION);
			}
			else if (sum < recv_img_len)
			{
				MessageBox(NULL,L"�����û�����ʧ��",L"QQ",MB_ICONERROR);
				return (INT_PTR)TRUE;
			}
		}

		//�洢ͼƬ����
		user_image_data.assign(user_img, user_img + recv_img_len);
        
		/*
		int x=recv(g_clientSocket, (char*)user_img, recv_img_len, 0);
		if(x==recv_img_len)
		MessageBox(NULL, L"�����û�ͼ�����ݳɹ�", L"QQ", MB_ICONINFORMATION);
		else
		{
			MessageBox(NULL, L"�����û�ͼ������ʧ��", L"QQ", MB_ICONERROR);
		}
		*/

		//��ȡ�ؼ���С
		HWND hImg = GetDlgItem(hwndDlg, IDC_MY_IMAGE_THREE);
		RECT rc;
		GetClientRect(hImg, &rc);
		int w = rc.right - rc.left;
		int h = rc.bottom - rc.top;

       //��ͼƬ�����ڿؼ�IDC_MY_IMAGE_THREE��
		using namespace Gdiplus;
		static ULONG_PTR gdiplusToken = 0;
		if (!gdiplusToken) //���GDI+�Ƿ��ʼ��//
		{
			GdiplusStartupInput gdiplusStartupInput;
			GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
		}

		//��ͼƬ�����ڿؼ�IDC_MY_IMAGE_THREE��
		if (user_img && recv_img_len > 0)
		{
			// ����IStream��װ�ڴ�����
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
						// ����Ŀ��ߴ��Bitmap����DrawImage����ԭͼ
						Bitmap* scaledBmp = new Bitmap(w, h, bmp->GetPixelFormat());
						Graphics g(scaledBmp);
						g.DrawImage(bmp, 0, 0, w, h);

						HBITMAP hBmp = NULL;
						scaledBmp->GetHBITMAP(Color(200, 230, 255), &hBmp);
						//HWND hImg = GetDlgItem(hwndDlg, IDC_MY_IMAGE_THREE);
						// �ͷſؼ�ԭ��ͼƬ
						HBITMAP hOldBmp = (HBITMAP)SendMessage(hImg, STM_GETIMAGE, IMAGE_BITMAP, 0);
						if (hOldBmp) DeleteObject(hOldBmp);
						// ������ͼƬ
						SendMessage(hImg, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmp);
						delete scaledBmp;
					}
					delete bmp;
					pStream->Release();
				}
				// hMem�ᱻpStream�ͷ�
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
			std::wstring wstr = L"������";
			int utf8_len = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1, NULL, 0, NULL, NULL);//����ת��Ϊutf8���ֽڳ��ȣ���'\0'//
			std::string str(utf8_len, 0);
			WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1,&str[0],utf8_len, NULL, NULL);//ʵ��ת��//
			send(g_clientSocket, (char*)&utf8_len, sizeof(utf8_len), 0);//�ȷ�����//
			send(g_clientSocket, str.c_str(), utf8_len, 0);//������//
			MessageBox(NULL, L"�Ѿ��ɹ����͵�½����İ�ť��Ϣ", L"QQ", MB_ICONINFORMATION);

			EndDialog(hwndDlg, IDC_EDIT22);//�����Ұ�ť//
			return (INT_PTR)TRUE;
		}
		case IDC_EDIT23:

		{
			std::wstring wstr = L"����";
			int utf8_len = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1, NULL, 0, NULL, NULL);//����ת��Ϊutf8���ֽڳ��ȣ���'\0'//
			std::string str(utf8_len, 0);
			WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1, &str[0], utf8_len, NULL, NULL);//ʵ��ת��//
			send(g_clientSocket, (char*)&utf8_len, sizeof(utf8_len), 0);//�ȷ�����//
			send(g_clientSocket, str.c_str(), utf8_len, 0);//������//
			MessageBox(NULL, L"�Ѿ��ɹ����͵�½����İ�ť��Ϣ", L"QQ", MB_ICONINFORMATION);

			EndDialog(hwndDlg, IDC_EDIT23);//���Ѱ�ť//
			return (INT_PTR)TRUE;
		}
		case IDC_EDIT24:
		{
			std::wstring wstr = L"������Ϣ";
			int utf8_len = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1, NULL, 0, NULL, NULL);//����ת��Ϊutf8���ֽڳ��ȣ���'\0'//
			std::string str(utf8_len, 0);
			WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1, &str[0], utf8_len, NULL, NULL);//ʵ��ת��//
			send(g_clientSocket, (char*)&utf8_len, sizeof(utf8_len), 0);//�ȷ�����//
			send(g_clientSocket, str.c_str(), utf8_len, 0);//������//
			MessageBox(NULL, L"�Ѿ��ɹ����͵�½����İ�ť��Ϣ", L"QQ", MB_ICONINFORMATION);

			EndDialog(hwndDlg, IDC_EDIT24);//������Ϣ��ť//
			return (INT_PTR)TRUE;
		}
		case IDC_EDIT25:

		{
			std::wstring wstr = L"����ͷ��";
			int utf8_len = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1, NULL, 0, NULL, NULL);//����ת��Ϊutf8���ֽڳ��ȣ���'\0'//
			std::string str(utf8_len, 0);
			WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1, &str[0], utf8_len, NULL, NULL);//ʵ��ת��//
			send(g_clientSocket, (char*)&utf8_len, sizeof(utf8_len), 0);//�ȷ�����//
			send(g_clientSocket, str.c_str(), utf8_len, 0);//������//
			MessageBox(NULL, L"�Ѿ��ɹ����͵�½����İ�ť��Ϣ", L"QQ", MB_ICONINFORMATION);

			EndDialog(hwndDlg, IDC_EDIT25);//����ͷ��ť//
			return (INT_PTR)TRUE;
		}
		case IDC_EDIT26:

		{
			std::wstring wstr = L"������";
			int wstr_len=wcslen(wstr.c_str());//���ַ������ַ���������'\0'//
			int utf8_len = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr_len+1, NULL, 0, NULL, NULL);//����ת��Ϊutf8���ֽڳ��ȣ���'\0'//
			std::string str(utf8_len, 0);
			WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr_len + 1, &str[0], utf8_len, NULL, NULL);//ʵ��ת��//
			send(g_clientSocket, (char*)&utf8_len, sizeof(utf8_len), 0);//�ȷ�����//
			send(g_clientSocket,str.c_str(),utf8_len, 0);//������//
			MessageBox(NULL,L"�Ѿ��ɹ����͵�½����ġ�����������ť��Ϣ",L"QQ",MB_ICONINFORMATION);

			EndDialog(hwndDlg, IDC_EDIT26);//��������ť//
			return (INT_PTR)TRUE;
		}
		case IDCANCEL:

		{
			std::wstring wstr = L"�˳�";
			int utf8_len = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1, NULL, 0, NULL, NULL);//����ת��Ϊutf8���ֽڳ��ȣ���'\0'//
			std::string str(utf8_len, 0);
			WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1, &str[0], utf8_len, NULL, NULL);//ʵ��ת��//
			send(g_clientSocket, (char*)&utf8_len, sizeof(utf8_len), 0);//�ȷ�����//
			send(g_clientSocket, str.c_str(), utf8_len, 0);//������//
			MessageBox(NULL, L"�Ѿ��ɹ����͵�½����İ�ť��Ϣ", L"QQ", MB_ICONINFORMATION);

			EndDialog(hwndDlg, IDCANCEL);//�˳���ť//
			return (INT_PTR)TRUE;
		}
		case IDC_EDIT43:

		{
			std::wstring wstr = L"����";
			int utf8_len = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1, NULL, 0, NULL, NULL);//����ת��Ϊutf8���ֽڳ��ȣ���'\0'//
			std::string str(utf8_len, 0);
			WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1, &str[0], utf8_len, NULL, NULL);//ʵ��ת��//
			send(g_clientSocket, (char*)&utf8_len, sizeof(utf8_len), 0);//�ȷ�����//
			send(g_clientSocket, str.c_str(), utf8_len, 0);//������//
			MessageBox(NULL, L"�Ѿ��ɹ����͵�½����İ�ť��Ϣ", L"QQ", MB_ICONINFORMATION);



			EndDialog(hwndDlg, IDC_EDIT43);//���ذ�ť//
			return (INT_PTR)TRUE;
		}
		case IDC_EDIT44:
		{
			std::wstring wstr = L"ע���˺�";
			int utf8_len = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1, NULL, 0, NULL, NULL);//����ת��Ϊutf8���ֽڳ��ȣ���'\0'//
			std::string str(utf8_len, 0);
			WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size() + 1, &str[0], utf8_len, NULL, NULL);//ʵ��ת��//
			send(g_clientSocket, (char*)&utf8_len, sizeof(utf8_len), 0);//�ȷ�����//
			send(g_clientSocket, str.c_str(), utf8_len, 0);//������//
			MessageBox(NULL, L"�Ѿ��ɹ����͵�½����İ�ť��Ϣ", L"QQ", MB_ICONINFORMATION);
		
			EndDialog(hwndDlg, IDC_EDIT44);//�˳���ť//
			return (INT_PTR)TRUE;
		}

		}
	}
	return (INT_PTR)FALSE;
}



INT_PTR CALLBACK Dialog_eleven(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//�������Ի���//
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
			//����ִ�в�����ʶ
			std::wstring w_char = L"�鿴�㲥";
			int send_len = WideCharToMultiByte(CP_UTF8, 0, w_char.c_str(), w_char.size() + 1, NULL, 0, NULL, NULL);//�ȼ���ת�����ȣ�����\0'
			std::string send_str(send_len, 0);//����ռ�//
			WideCharToMultiByte(CP_UTF8, 0, w_char.c_str(), w_char.size() + 1, &send_str[0], send_len, NULL, NULL);//ʵ��ת��//
			send(g_clientSocket, (char*)&send_len, sizeof(send_len), 0);//�ȷ�����//
			send(g_clientSocket, send_str.c_str(), send_len, 0);//�ٷ�����//

			std::string str_account = new_online_user_account;
			if (!str_account.empty() && str_account.back() == '\0')
			{
				str_account.pop_back();
			}

			char buf2[256];
			sprintf_s(buf2, "str_account: [%s] len=%d\n", str_account.c_str(), (int)str_account.size());
			OutputDebugStringA(buf2);

			//�����û��˺ţ��Թ���ѯ
			int str_account_len = str_account.size();
			send(g_clientSocket, (char*)&str_account_len,sizeof(str_account_len),0);
			send(g_clientSocket,str_account.c_str(),str_account_len,0);
		    
			//�����û��ǳ�
			int str_nickname_l = 0;
			recv(g_clientSocket,(char*)&str_nickname_l,sizeof(str_nickname_l),0);
			std::string str_nickname(str_nickname_l,'\0');
			recv(g_clientSocket,&str_nickname[0],str_nickname_l,0);
			
			//���û��ǳ�ת��Ϊ���ַ�
			int w_nick_len = 0;
			w_nick_len = MultiByteToWideChar(CP_UTF8, 0, str_nickname.c_str(), str_nickname.size(), NULL, 0);
			std::wstring w_nickname_s(w_nick_len, L'\0');
			MultiByteToWideChar(CP_UTF8, 0, str_nickname.c_str(),str_nickname.size(), &w_nickname_s[0], w_nick_len);


	       //���չ㲥��Ϣ����
			int sum_broadcast = 0;
			recv(g_clientSocket, (char*)&sum_broadcast,sizeof(sum_broadcast),0);

			if (sum_broadcast <= 0)
			{
				//��ʾ��������Ϣ
				std::wstring my_notice = L"��������ʱδ�������͹㲥��Ϣ";
				int wlen_two = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_EDIT27));//��ȡ�����ԭ�е��ı�,������'\0'//
				std::wstring wstr_two(wlen_two + 1, 0);//����ռ�//
				GetWindowText(GetDlgItem(hwndDlg, IDC_EDIT27), &wstr_two[0], wlen_two + 1);
				std::wstring fullmsg = L"";
				if (wlen_two > 0)//������ǵ�һ����Ϣ�ͽ���//
				{
					if (wstr_two.back() == L'\0')
					{
						wstr_two.pop_back();
					}
					fullmsg = wstr_two;

					fullmsg += L"\r\n";
				}
				fullmsg += my_notice;
				SetDlgItemText(hwndDlg, IDC_EDIT27, fullmsg.c_str());//���ı���ʾ�ڶԻ�����//
				return (INT_PTR)TRUE;
			}
			//���չ㲥��Ϣ
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
						//����Ϣת�����ַ�����ȥ��ĩβ��'\0'
						int w_l = 0;
						w_l=MultiByteToWideChar(CP_UTF8,0,s.c_str(),s.size(),NULL,0);
						std::wstring w_s(w_l,L'\0');
						MultiByteToWideChar(CP_UTF8, 0, s.c_str(), s.size(),&w_s[0],w_l);
						if (!w_s.empty() && w_s.back() == '\0')
						{
							w_s.pop_back();
						}
						//��ȡ��ǰʱ��//
						time_t now_one = time(0);//��ȡ��ǰʱ�䣬��ȷ����//
						tm tm_one;//����һ���ṹ�壬���ڴ洢����ʱ��ĸ�����ɲ���//
						localtime_s(&tm_one, &now_one);//��now_one�����룩������ݸ��Ƶ�tm_one�����//
						wchar_t timeBuffer_one[64];
						wcsftime(timeBuffer_one, sizeof(timeBuffer_one) / sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", &tm_one);
						//MessageBox(NULL, L"�Ѿ�׼������ʱ���ֶ�", L"QQ", MB_ICONINFORMATION);

						std::wstring msg_one = L"[";
						msg_one += timeBuffer_one;
						msg_one += L"][";
						msg_one += L"������";
						msg_one += L"->";
						//�û��ǳ�
						msg_one += w_nickname_s;
						msg_one += L"]";
						//MessageBox(NULL, L"�Ѿ��ɹ�ƴ�Ӻ÷������ֶ�", L"QQ", MB_ICONINFORMATION);
						msg_one += w_s.c_str();

						//��һ���ĸ�ʽ��ʾ�㲥��Ϣ
						int wlen_two = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_EDIT27));//��ȡ�����ԭ�е��ı�,������'\0'//
						std::wstring wstr_two(wlen_two + 1, 0);//����ռ�//
						GetWindowText(GetDlgItem(hwndDlg, IDC_EDIT27), &wstr_two[0], wlen_two + 1);
						std::wstring fullmsg = L"";
						if (wlen_two > 0)//������ǵ�һ����Ϣ�ͽ���//
						{
							if (wstr_two.back() == L'\0')
							{
								wstr_two.pop_back();
							}
							fullmsg = wstr_two;

							fullmsg += L"\r\n";
						}
						fullmsg +=msg_one;
						SetDlgItemText(hwndDlg, IDC_EDIT27, fullmsg.c_str());//���ı���ʾ�ڶԻ�����//
						break;
					}
				}
				i++;
			}
		}
		case IDOK:
		{
			//��ȡ�ı����������ı�//
			HWND hwnd = GetDlgItem(hwndDlg, IDC_EDIT28);//��ȡ�����ؼ����//
			int wlen = GetWindowTextLength(hwnd);//��ȡ�ؼ��ı����ȣ�����'\0'//
			if (wlen <= 0)
			{
				MessageBox(NULL,L"���������",L"QQ",MB_ICONINFORMATION);
				return(INT_PTR)TRUE;
			}
			//ȷ�������ݵ���Ϣ����Ч//

			std::wstring w_char = L"����";
			int send_len = WideCharToMultiByte(CP_UTF8, 0, w_char.c_str(), w_char.size() + 1, NULL, 0, NULL, NULL);//�ȼ���ת������//
			std::string send_str(send_len, 0);//����ռ�//
			WideCharToMultiByte(CP_UTF8, 0, w_char.c_str(), w_char.size() + 1, &send_str[0], send_len, NULL, NULL);//ʵ��ת��//
			send(g_clientSocket, (char*)&send_len, sizeof(send_len), 0);//�ȷ�����//
			send(g_clientSocket, send_str.c_str(), send_len, 0);//�ٷ�����//


			std::wstring wstr(wlen + 1, 0);//����ռ�//
			GetWindowText(hwnd, &wstr[0], wlen+1);//��ȡ�ؼ��ı�//
			int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wlen + 1, NULL, 0, NULL, NULL);//����utf8�ֽڳ���//
			
			//std::wstring n = std::to_wstring(utf8_len);
			//MessageBox(NULL, n.c_str(), L"QQ", MB_ICONINFORMATION);

			std::string utf8_str(utf8_len, 0);//����ռ�//
			//MessageBox(NULL,L"�ͻ����ѳɹ�����ռ����洢����", L"QQ", MB_ICONINFORMATION);
			WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wlen + 1, &utf8_str[0], utf8_len, NULL, NULL);
			//���з��ͺ��˳���ť�ж�//
	
			// ���͸�������//
			send(g_clientSocket,(char*)&utf8_len,sizeof(utf8_len),0);//�ȷ�����//
			send(g_clientSocket, &utf8_str[0], utf8_len, 0);//������//
			//MessageBox(NULL, L"�Ѿ��ɹ�����������Ϳͻ��˵ĶԻ�", L"QQ", MB_ICONINFORMATION);
			
			//���շ��������ص���Ϣ//
			int recv_len = 0;
			recv(g_clientSocket, (char*)&recv_len, sizeof(recv_len), 0);//�Ƚ��ճ���//
			if (recv_len <= 0)
			{
				MessageBox(NULL, L"���շ��������͵���Ϣ����ʧ��", L"QQ", MB_ICONERROR);
				return (INT_PTR) TRUE;
			}
			std::string recv_char(recv_len, 0);//����ռ�//
			recv(g_clientSocket, &recv_char[0], recv_len, 0);//��������//
			MessageBox(NULL, L"�Ѿ��ɹ����շ��������ص���Ϣ", L"QQ", MB_ICONINFORMATION);
			int wrecv_len = MultiByteToWideChar(CP_UTF8, 0, recv_char.c_str(), recv_len, NULL, 0);//�����ת������//
			std::wstring wrecv_str(wrecv_len, 0);//����ռ�//
			MultiByteToWideChar(CP_UTF8, 0, recv_char.c_str(), recv_len, &wrecv_str[0], wrecv_len);//ʵ��ת��,�Ѿ������ܵ����ݱ��浽wrecv_str//
		
			int wlen_two= GetWindowTextLength(GetDlgItem(hwndDlg,IDC_EDIT27));//��ȡ�����ԭ�е��ı�,������'\0'//
			std::wstring wstr_two(wlen_two+1,0);//����ռ�//
			GetWindowText(GetDlgItem(hwndDlg, IDC_EDIT27), &wstr_two[0], wlen_two+1);
			std::wstring fullmsg=L"";
			if (wlen_two>0)//������ǵ�һ����Ϣ�ͽ���//
			{
				if (wstr_two.back() == L'\0')
				{
					wstr_two.pop_back();
				}
				fullmsg = wstr_two;

				fullmsg += L"\r\n";
			}
			fullmsg += wrecv_str;
			SetDlgItemText(hwndDlg,IDC_EDIT27,fullmsg.c_str());//���ı���ʾ�ڶԻ�����//
			//����ı�������//
			//MessageBox(NULL, L"��������ı���", L"QQ", MB_ICONINFORMATION);
			SetDlgItemText(hwndDlg,IDC_EDIT28,L"");
			//EndDialog(hwndDlg, IDOK);//���Ͱ�ť//
			return (INT_PTR)TRUE;
		}
		case IDCANCEL:
		{
			std::wstring w_char = L"�˳�";
			int send_len = WideCharToMultiByte(CP_UTF8, 0, w_char.c_str(), w_char.size() + 1, NULL, 0, NULL, NULL);//�ȼ���ת������//
			std::string send_str(send_len, 0);//����ռ�//
			WideCharToMultiByte(CP_UTF8, 0, w_char.c_str(), w_char.size() + 1, &send_str[0], send_len, NULL, NULL);//ʵ��ת��//
			send(g_clientSocket, (char*)&send_len, sizeof(send_len), 0);//�ȷ�����//
			send(g_clientSocket, send_str.c_str(), send_len, 0);//�ٷ�����//

			EndDialog(hwndDlg, IDCANCEL);//�˳���ť//
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



INT_PTR CALLBACK Dialog_twelve(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//�����ҶԻ���//
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{

		//���÷��Ͱ�ť
		HWND hSendBtn = GetDlgItem(hwndDlg, IDOK);
		EnableWindow(hSendBtn, false);

		// ���ԭ����Ϣ
		{
			std::lock_guard<std::mutex> lk(g_chartroom_mutex);
			g_chartroom_inf.clear();
		}

		//����������������е�������Ϣ

		std::string u_str = "������������ҵ���Ϣ";
		//��������
		int u_len = u_str.size();
		send(g_clientSocket, (char*)&u_len, sizeof(u_len), 0);
		send(g_clientSocket, u_str.c_str(), u_len, 0);

		//OutputDebugStringA(("u_str is " + u_str + ", and its len is " + std::to_string(u_len) + "\n").c_str());

		MessageBox(NULL, L"�ɹ����ͼ�����������Ϣ������", L"QQ", MB_ICONINFORMATION);

		int num = 0;
		recv(g_clientSocket, (char*)&num, sizeof(num), 0);
		MessageBox(NULL, L"�ɹ�������������Ϣ��������", L"QQ", MB_ICONINFORMATION);

		if (num <= 0)
		{
			MessageBox(NULL, L"�������������������¼Ϊ��", L"QQ", MB_ICONINFORMATION);
			return(INT_PTR)TRUE;
		}

		//����������ԭ�������¼����
		int i = 0;
		while (i < num)
		{
			i++;
			u u_inf;
			//�����û�ͼ������
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
			u_inf.img_data.insert(u_inf.img_data.end(), img_data, img_data + img_len);//�洢ͼƬ����
			delete[]img_data;//�ͷ��ڴ棻

			//�����û��ǳ�
			int nickname_len = 0;
			recv(g_clientSocket, (char*)&nickname_len, sizeof(nickname_len), 0);
			std::string u_nickname(nickname_len, '\0');
			recv(g_clientSocket, &u_nickname[0], nickname_len, 0);
			u_inf.nickname = u_nickname;//�洢�û��ǳ�

			//�����û��������ҷ��͵���Ϣ
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
			u_inf.inf = chart_inf_str;//�洢�û���Ϣ

			//����Ϣѹ�����
			std::lock_guard <std::mutex>lk(g_chartroom_mutex);
			g_chartroom_inf.push_back(u_inf);
		}

		//�����յ����ݰ���һ���ĸ�ʽ��ʾ�ڿؼ� IDC_EDIT29��
		//��ʽΪͷ��ͼƬ����+�ǳ�+����+��Ϣ

		// �� Dialog_twelve �� WM_INITDIALOG ĩβ���
		//SendDlgItemMessage(hwndDlg, IDC_EDIT29, LB_SETITEMHEIGHT, 0,18*100); // ÿ��48����
		// ������g_chartroom_inf�󣬲���ListBox��
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
		// ��ÿ��߶�����Ӧ����
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

				// ������Ϣ���ݵĸ߶�
				const u& item = g_chartroom_inf[idx];
				std::wstring wmsg;
				int wlen = MultiByteToWideChar(CP_UTF8, 0, item.inf.c_str(), (int)item.inf.size(), NULL, 0);
				wmsg.resize(wlen);
				MultiByteToWideChar(CP_UTF8, 0, item.inf.c_str(), (int)item.inf.size(), &wmsg[0], wlen);

				HDC hdc = GetDC(hwndDlg);
				RECT rcMsg = { 0, 0,contentWidth, 0 }; // ����ͷ��ͱ߾�
				DrawTextW(hdc, wmsg.c_str(), (int)wmsg.size(), &rcMsg, DT_WORDBREAK | DT_CALCRECT);
				ReleaseDC(hwndDlg, hdc);

				int nicknameHeight = 18;
				int padding = 8;
				int minHeight = avatarSize +8;
				int contentHeight = rcMsg.bottom - rcMsg.top + nicknameHeight + padding;
				lpmis->itemHeight = max(minHeight, contentHeight); // ����ͷ��߶�
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

				// ����
				SetBkColor(lpdis->hDC, (lpdis->itemState & ODS_SELECTED) ? RGB(220, 240, 255) : RGB(255, 255, 255));
				ExtTextOut(lpdis->hDC, 0, 0, ETO_OPAQUE, &lpdis->rcItem, NULL, 0, NULL);

				// ͷ��
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

				// �ǳ�
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

				// ��Ϣ����
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

				// ѡ�б߿�
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
			// ���ԭ����Ϣ
			{
				std::lock_guard<std::mutex> lk(g_chartroom_mutex);
				g_chartroom_inf.clear();
			}

			std::string inf = "����";
			int utf8_string_len = inf.size();
			send(g_clientSocket, (char*)&utf8_string_len, sizeof(utf8_string_len), 0);
			send(g_clientSocket, inf.c_str(), inf.size(), 0);

			//��ȡ�ؼ����
			HWND h = GetDlgItem(hwndDlg, IDC_EDIT30);
			//��ȡ�ؼ��ı�����
			int text_len = GetWindowTextLength(h);
			if (text_len <= 0)
			{
				MessageBox(NULL, L"���͵���Ϣ����Ϊ��", L"QQ", MB_ICONERROR);
				return (INT_PTR)TRUE;
			}
			std::wstring w_text(text_len + 1, L'\0');
			GetWindowText(h, &w_text[0], text_len + 1);//��Ϣ��'\0'
			//ת�����ַ���
			int utf8_text_len = WideCharToMultiByte(CP_UTF8, 0, w_text.c_str(), text_len + 1, NULL, 0, NULL, NULL);
			std::string utf8_text_str(utf8_text_len, '\0');
			WideCharToMultiByte(CP_UTF8, 0, w_text.c_str(), text_len + 1, &utf8_text_str[0], utf8_text_len, NULL, NULL);
			//���ı���Ϣ����
			send(g_clientSocket, (char*)&utf8_text_len, sizeof(utf8_text_len), 0);
			send(g_clientSocket, utf8_text_str.c_str(), utf8_text_len, 0);


			//����Ϣ�༭�����
			SetDlgItemText(hwndDlg, IDC_EDIT30, L"");
			//���ؼ�IDC_EDIT30�Ƿ����ı�����������÷��Ͱ�ť
			HWND hSendBtn = GetDlgItem(hwndDlg, IDOK);
			EnableWindow(hSendBtn, false);



			//���շ������ĸ���֪ͨ
			int len = 0;
			recv(g_clientSocket, (char*)&len, sizeof(len), 0);
			std::string str(len, '\0');
			recv(g_clientSocket, &str[0], len, 0);
			if (strcmp(str.c_str(), "�������������Ϣ") != 0)
			{
				MessageBox(NULL, L"δ�ܸ��ݷ�����ָ����ȷ���������ҵ���Ϣ", L"QQ", MB_ICONERROR);
			}

			//֪ͨ��ʼ����Ϣ��ִ��һ��
			// ���ԭ����Ϣ
			{
				std::lock_guard<std::mutex> lk(g_chartroom_mutex);
				g_chartroom_inf.clear();
			}

			//����������������е�������Ϣ

			std::string u_str = "������������ҵ���Ϣ";
			//��������
			int u_len = u_str.size();
			send(g_clientSocket, (char*)&u_len, sizeof(u_len), 0);
			send(g_clientSocket, u_str.c_str(), u_len, 0);

			//OutputDebugStringA(("u_str is " + u_str + ", and its len is " + std::to_string(u_len) + "\n").c_str());

			MessageBox(NULL, L"�ɹ����ͼ�����������Ϣ������", L"QQ", MB_ICONINFORMATION);

			int num = 0;
			recv(g_clientSocket, (char*)&num, sizeof(num), 0);
			MessageBox(NULL, L"�ɹ�������������Ϣ��������", L"QQ", MB_ICONINFORMATION);

			if (num <= 0)
			{
				MessageBox(NULL, L"�������������������¼Ϊ��", L"QQ", MB_ICONINFORMATION);
				return(INT_PTR)TRUE;
			}

			//����������ԭ�������¼����
			int i = 0;
			while (i < num)
			{
				i++;
				u u_inf;
				//�����û�ͼ������
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
				u_inf.img_data.insert(u_inf.img_data.end(), img_data, img_data + img_len);//�洢ͼƬ����
				delete[]img_data;//�ͷ��ڴ棻

				//�����û��ǳ�
				int nickname_len = 0;
				recv(g_clientSocket, (char*)&nickname_len, sizeof(nickname_len), 0);
				std::string u_nickname(nickname_len, '\0');
				recv(g_clientSocket, &u_nickname[0], nickname_len, 0);
				u_inf.nickname = u_nickname;//�洢�û��ǳ�

				//�����û��������ҷ��͵���Ϣ
				int chart_inf_len = 0;
				recv(g_clientSocket, (char*)&chart_inf_len, sizeof(chart_inf_len), 0);
				std::string chart_inf_str(chart_inf_len, '\0');
				recv(g_clientSocket, &chart_inf_str[0], chart_inf_len, 0);
				u_inf.inf = chart_inf_str;//�洢�û���Ϣ

				OutputDebugStringA(("the information user send in chartroom is"+chart_inf_str+"\n").c_str());

				//����Ϣѹ�����
				std::lock_guard <std::mutex>lk(g_chartroom_mutex);
				g_chartroom_inf.push_back(u_inf);
			}

			//�����յ����ݰ���һ���ĸ�ʽ��ʾ�ڿؼ� IDC_EDIT29��
			//��ʽΪͷ��ͼƬ����+�ǳ�+����+��Ϣ

			// �� Dialog_twelve �� WM_INITDIALOG ĩβ���
			//SendDlgItemMessage(hwndDlg, IDC_EDIT29, LB_SETITEMHEIGHT, 0, 18*100); // ÿ��48����
			// ������g_chartroom_inf�󣬲���ListBox��
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
			// ���ԭ����Ϣ
			{
				std::lock_guard<std::mutex> lk(g_chartroom_mutex);
				g_chartroom_inf.clear();
			}

			std::string inf = "������Ϣ";
			int utf8_string_len = inf.size();
			send(g_clientSocket, (char*)&utf8_string_len, sizeof(utf8_string_len), 0);
			send(g_clientSocket, inf.c_str(), inf.size(), 0);

			//����������������е�������Ϣ

			std::string u_str = "������������ҵ���Ϣ";
			//��������
			int u_len = u_str.size();
			send(g_clientSocket, (char*)&u_len, sizeof(u_len), 0);
			send(g_clientSocket, u_str.c_str(), u_len, 0);

			//OutputDebugStringA(("u_str is " + u_str + ", and its len is " + std::to_string(u_len) + "\n").c_str());

			MessageBox(NULL, L"�ɹ����ͼ�����������Ϣ������", L"QQ", MB_ICONINFORMATION);

			int num = 0;
			recv(g_clientSocket, (char*)&num, sizeof(num), 0);
			MessageBox(NULL, L"�ɹ�������������Ϣ��������", L"QQ", MB_ICONINFORMATION);

			if (num <= 0)
			{
				MessageBox(NULL, L"�������������������¼Ϊ��", L"QQ", MB_ICONINFORMATION);
				return(INT_PTR)TRUE;
			}

			//����������ԭ�������¼����
			int i = 0;
			while (i < num)
			{
				i++;
				u u_inf;
				//�����û�ͼ������
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
				u_inf.img_data.insert(u_inf.img_data.end(), img_data, img_data + img_len);//�洢ͼƬ����
				delete[]img_data;//�ͷ��ڴ棻

				//�����û��ǳ�
				int nickname_len = 0;
				recv(g_clientSocket, (char*)&nickname_len, sizeof(nickname_len), 0);
				std::string u_nickname(nickname_len, '\0');
				recv(g_clientSocket, &u_nickname[0], nickname_len, 0);
				u_inf.nickname = u_nickname;//�洢�û��ǳ�

				//�����û��������ҷ��͵���Ϣ
				int chart_inf_len = 0;
				recv(g_clientSocket, (char*)&chart_inf_len, sizeof(chart_inf_len), 0);
				std::string chart_inf_str(chart_inf_len, '\0');
				recv(g_clientSocket, &chart_inf_str[0], chart_inf_len, 0);
				u_inf.inf = chart_inf_str;//�洢�û���Ϣ

				OutputDebugStringA(("the information user send in chartroom is" + chart_inf_str + "\n").c_str());

				//����Ϣѹ�����
				std::lock_guard <std::mutex>lk(g_chartroom_mutex);
				g_chartroom_inf.push_back(u_inf);
			}

			//�����յ����ݰ���һ���ĸ�ʽ��ʾ�ڿؼ� IDC_EDIT29��
			//��ʽΪͷ��ͼƬ����+�ǳ�+����+��Ϣ

			// �� Dialog_twelve �� WM_INITDIALOG ĩβ���
			//SendDlgItemMessage(hwndDlg, IDC_EDIT29, LB_SETITEMHEIGHT, 0, 18*100); // ÿ��48����
			// ������g_chartroom_inf�󣬲���ListBox��
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
			std::string inf = "�˳�";
			int utf8_string_len = inf.size();
			send(g_clientSocket, (char*)&utf8_string_len, sizeof(utf8_string_len), 0);
			send(g_clientSocket, inf.c_str(), inf.size(), 0);

			MessageBox(NULL, L"�ɹ������˳���ť��WM_COMMAND��", L"QQ", MB_ICONINFORMATION);
			EndDialog(hwndDlg, IDCANCEL);
			return (INT_PTR)TRUE;
		}
		}
	}
	/*
	case WM_CLOSE:
	{
		std::string inf = "�˳�";
		int utf8_string_len = inf.size();
		send(g_clientSocket, (char*)&utf8_string_len, sizeof(utf8_string_len), 0);
		send(g_clientSocket, inf.c_str(), inf.size(), 0);

		MessageBox(NULL, L"�ɹ������˳���ť��WM_CLOSE��", L"QQ", MB_ICONERROR);
		EndDialog(hwndDlg, IDCANCEL);
		return (INT_PTR)TRUE;
	}
	*/
	}
	
	return (INT_PTR)FALSE;
}

//��¼�û����ѵ��˺š��ǳƺ�ͷ��
struct user_friend
{
	int signal;
	std::string account;
	std::string nickname;
	std::vector<BYTE> image;
};
std::vector <user_friend> g_user_friends;


//��¼�û����ѵ��˺š��ǳƺ�ͷ��
struct user_half_friend
{
	std::string account;
	std::string nickname;
	std::vector<BYTE> image;
};
std::vector <user_half_friend> g_user_half_friends;


INT_PTR CALLBACK Dialog_seventeen(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//�������б��
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		//����������ͼ����������б������
		std::string request_to_server = "�ͻ�����������������б�";
		int request_len = request_to_server.size();
		send(g_clientSocket, (char*)&request_len,sizeof(request_len),0);
		send(g_clientSocket, request_to_server.c_str(), request_len, 0);

		char* buf_x1 = new char[50]();
		snprintf(buf_x1, 50, "the request_to_server is %s\n",request_to_server.c_str());
		OutputDebugStringA(buf_x1);
		delete[]buf_x1;

		//���շ��������ص���Ϣ

		int r_len = 0;
		recv(g_clientSocket, (char*)&r_len, sizeof(r_len), 0);
		std::string s_str(r_len, '\0');
		recv(g_clientSocket, &s_str[0], r_len, 0);

		char* buf_x2 = new char[50]();
		snprintf(buf_x2, 50, "the s_str is %s\n", s_str.c_str());
		OutputDebugStringA(buf_x2);
		delete[]buf_x2;

		//�ȽϽ��ܵ��ַ���
		if (strcmp("success",s_str.c_str()) == 0)
		{
			MessageBox(NULL,L"�������Ѿ��ɹ������������б�",L"QQ",MB_ICONINFORMATION);
		}
		else if(strcmp("failure", s_str.c_str()) == 0)
		{
			MessageBox(NULL, L"�������޷������������б�,�������غ��ѿ�", L"QQ", MB_ICONERROR);
			//��ʼ��ʧ�ܣ��رնԻ��򣬷��غ��ѿ�
			EndDialog(hwndDlg,IDCANCEL);
			return (INT_PTR)TRUE;
		}

		//��������������
		int half_len = 0;
		recv(g_clientSocket, (char*)&half_len, sizeof(half_len), 0);

		//����Ϊ0����ʼ������
		if (half_len == 0)
		{
			MessageBox(NULL, L"����ʱ��û�������ѣ��������غ��ѿ�", L"QQ", MB_ICONINFORMATION);
			EndDialog(hwndDlg, IDCANCEL);
			return(INT_PTR)TRUE;
		}

		//������Ϊ0����ϵͳ�����б���������
		else if(half_len>0)
		{
			MessageBox(NULL, L"�����������Է�����������������", L"QQ", MB_ICONINFORMATION);


			//���պ����б����Ϣ��
			int i = 0;
			g_user_half_friends.clear();//ȷ��������Ϣ�������
			user_half_friend u_x;
			while (i < half_len)
			{
				u_x= user_half_friend();//��սṹ��

				//���պ����˺�
				int account_x_len = 0;
				recv(g_clientSocket, (char*)&account_x_len, sizeof(account_x_len), 0);
				u_x.account.resize(account_x_len);//���·���ռ�
				recv(g_clientSocket, &u_x.account[0], account_x_len, 0);

				char* buf = new char[50]();
				snprintf(buf, 50, "the friend_account is %s\n", u_x.account.c_str());
				OutputDebugStringA(buf);
				delete[]buf;

				//���պ����ǳ�
				int nickname_x_len = 0;
				recv(g_clientSocket, (char*)&nickname_x_len, sizeof(nickname_x_len), 0);
				u_x.nickname.resize(nickname_x_len);//���·���ռ�
				recv(g_clientSocket, &u_x.nickname[0], nickname_x_len, 0);

				char* buf_2 = new char[50]();
				snprintf(buf_2, 50, "the friend_nickname is %s\n", u_x.nickname.c_str());
				OutputDebugStringA(buf_2);
				delete[]buf_2;

				//���պ���ͷ��
				int image_x_len = 0;
				recv(g_clientSocket, (char*)&image_x_len, sizeof(image_x_len), 0);


				char* buf_3 = new char[50]();
				snprintf(buf_3, 50, "the friend_photo_len is %d\n", image_x_len);
				OutputDebugStringA(buf_3);
				delete[]buf_3;


				u_x.image.resize(image_x_len);//���·���ռ�
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
				//������ϣ������ѵ���Ϣѹ�뵯��
				g_user_half_friends.push_back(u_x);

				i++;
			}

			MessageBox(NULL, L"�Ѿ��ɹ������û��ĺ�����Ϣ", L"QQ", MB_ICONINFORMATION);

			HWND hList6 = GetDlgItem(hwndDlg, IDC_EDIT49);
			SendMessage(hList6, LB_RESETCONTENT, 0, 0);//����б��
			for (size_t i = 0; i < g_user_half_friends.size(); ++i)
			{
				SendDlgItemMessage(hwndDlg, IDC_EDIT49, LB_ADDSTRING, 0, (LPARAM)L"");
				SendDlgItemMessage(hwndDlg, IDC_EDIT49, LB_SETITEMHEIGHT, i, 48); // ÿ��48����
			}
			
			SendMessage(hList6, LB_SETSEL, FALSE, -1);//ȡ������ѡ����//
			LONG style = GetWindowLong(hList6, GWL_STYLE);//��ȡ��ǰ�ؼ�����ʽ//
			style &= ~(LBS_SINGLESEL);//���LBS_OWNERDRAWFIXED��ʽ//
			style |= LBS_MULTIPLESEL;  // ���Ӷ�ѡ��֧��Ctrl/Shift
			SetWindowLong(hList6, GWL_STYLE, style); // Ӧ������ʽ

			HWND hBtn_y = GetDlgItem(hwndDlg, IDOK); //����ť���//
			EnableWindow(hBtn_y, FALSE); // ��ʼ����ͬ�ⰴť

			HWND hBtn_x = GetDlgItem(hwndDlg, IDC_REJECT); //����ť���//
			EnableWindow(hBtn_x, FALSE); // ��ʼ���þܾ���ť

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

				// ����
				SetBkColor(lpdis->hDC, (lpdis->itemState & ODS_SELECTED) ? RGB(230, 230, 250) : RGB(255, 255, 255));
				ExtTextOut(lpdis->hDC, 0, 0, ETO_OPAQUE, &lpdis->rcItem, NULL, 0, NULL);

				// ͷ��
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

				// �ǳ�
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
			


				// �˺�����
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

				// ѡ�б߿�
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

					new_friend_account.push_back(account);//��ȫ��ѡ�е��û��˺Ŵ洢

				}
			}

			//SendMessage(hList, LB_SETSEL, FALSE, -1);//ȡ������ѡ����//
			
			//�����������ȷ����Ϣ
			std::string buf = "ͬ��";
			int buf_len = buf.size();
			send(g_clientSocket, (char*)&buf_len, sizeof(buf_len), 0);
			send(g_clientSocket, buf.c_str(), buf_len, 0);

			//�ȷ���ͬ�����������Ŀ
			send(g_clientSocket, (char*)&selCount_x, sizeof(selCount_x), 0);

			//����������;��û�ͬ����������˺�
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
				MessageBox(NULL, L"�������Ѿ��������ĺ����б���������б������ص����ѿ�", L"QQ", MB_ICONINFORMATION);
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

					new_friend_account.push_back(account);//��ȫ��ѡ�е��û��˺Ŵ洢

				}
			}

			//SendMessage(hList, LB_SETSEL, FALSE, -1);//ȡ������ѡ����//

			//�����������ȷ����Ϣ
			std::string buf = "�ܾ�";
			int buf_len = buf.size();
			send(g_clientSocket, (char*)&buf_len, sizeof(buf_len), 0);
			send(g_clientSocket, buf.c_str(), buf_len, 0);

			//�ȷ���ͬ�����������Ŀ
			send(g_clientSocket, (char*)&selCount_x, sizeof(selCount_x), 0);

			//����������;��û�ͬ����������˺�
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
				MessageBox(NULL, L"�������Ѿ��������ĺ����б���������б������ص����ѿ�", L"QQ", MB_ICONINFORMATION);
				EndDialog(hwndDlg, IDOK);
				return(INT_PTR)TRUE;
			}
			return (INT_PTR)TRUE;

		}

		case IDCANCEL:
		{
			//�����������ȷ����Ϣ
			std::string buf = "ȡ��";
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


INT_PTR CALLBACK Dialog_sixteen(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//��Ӻ����˺������
{
	switch (uMsg)
	{

	case WM_INITDIALOG:
	{
		SetDlgItemText(hwndDlg, IDC_EDIT47, L"��������ѵ��˺�");
		//����ȷ����ť
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
			//��ȡ�ؼ�IDC_EDIT48���ı�
			HWND h = GetDlgItem(hwndDlg, IDC_EDIT48);
			//��ȡ�ı�����
			int w_text_len = GetWindowTextLength(h);
			if (w_text_len < 0)
			{
				return (INT_PTR)TRUE;
			}

			//�����������ȷ����Ϣ
			std::string buf = "ȷ��";
			int buf_len = buf.size();
			send(g_clientSocket, (char*)&buf_len, sizeof(buf_len), 0);
			send(g_clientSocket, buf.c_str(), buf_len, 0);

		
			std::wstring w_text(w_text_len+1,L'\0');
			GetWindowText(h, &w_text[0], w_text_len+1);

			if (!w_text.empty() && w_text.back() == L'\0')
			{
				w_text.pop_back();
			}

			//�����ַ���ת��
			int utf8_len = 0;
			utf8_len=WideCharToMultiByte(CP_UTF8,0,w_text.c_str(),w_text_len,NULL,0,NULL,NULL);
			std::string utf8_str(utf8_len,'\0');
			WideCharToMultiByte(CP_UTF8, 0, w_text.c_str(), w_text_len, &utf8_str[0],utf8_len, NULL, NULL);

			//����������ͺ����˺Ž�����֤
			send(g_clientSocket, (char*)&utf8_len, sizeof(utf8_len), 0);
			send(g_clientSocket, utf8_str.c_str(), utf8_len, 0);

			//����˺������
			SetDlgItemText(hwndDlg,IDC_EDIT48,L"");

			//���շ������ú����˺��Ƿ���ڵ���Ϣ
			
			int x_len = 0;

			recv(g_clientSocket,(char*)&x_len, sizeof(x_len), 0);
			std::string x_str(x_len, '\0');
			recv(g_clientSocket, &x_str[0], x_len, 0);

			if(strcmp("��ӵĺ����˺Ŵ���",x_str.c_str()) == 0)
			//������
			{
				MessageBox(NULL, L"��ӵĺ����˺Ŵ���,�ѳɹ�������Ӻ��ѵ�����", L"QQ", MB_ICONINFORMATION);
				//�رնԻ��򣬷��غ��ѿ�
				EndDialog(hwndDlg,IDOK);
				return (INT_PTR)TRUE;
			}

			else if (strcmp("�����Ѿ��Ǻ��ѣ������ڷ��ͺ�������", x_str.c_str()) == 0)
				//������
			{
				MessageBox(NULL, L"�����Ѿ��Ǻ��ѣ������ڷ��ͺ�������", L"QQ", MB_ICONINFORMATION);
				//�رնԻ��򣬷��غ��ѿ�
				EndDialog(hwndDlg, IDOK);
				return (INT_PTR)TRUE;
			}

			else if (strcmp("�Ѿ����͹��������������ĵȴ�", x_str.c_str()) == 0)
				//������
			{
				MessageBox(NULL, L"�Ѿ����͹��������������ĵȴ�", L"QQ", MB_ICONINFORMATION);
				//�رնԻ��򣬷��غ��ѿ�
				EndDialog(hwndDlg, IDOK);
				return (INT_PTR)TRUE;
			}

			else if (strcmp("��ӵĺ����˺Ų�����", x_str.c_str()) == 0)
			//��������
			{
				MessageBox(NULL, L"��ӵĺ����˺Ų�����", L"QQ", MB_ICONERROR);
				MessageBox(NULL, L"������������ѵ��˺�", L"QQ", MB_ICONINFORMATION);
				return (INT_PTR)TRUE;
			}

		}break;

		case IDCANCEL:
		{
			//�����������ȷ����Ϣ
			std::string buf = "ȡ��";
			int buf_len = buf.size();
			send(g_clientSocket, (char*)&buf_len, sizeof(buf_len), 0);
			send(g_clientSocket, buf.c_str(), buf_len, 0);

		   //�˳���Ӻ����˺������
			EndDialog(hwndDlg,IDCANCEL);
			return (INT_PTR)TRUE;

		}break;

		}
	}break;

	}

	return (INT_PTR)FALSE;
}

//ת���ַ���Ϊ���ַ���
std::wstring strTowstr(std::string str)
{
	int w_len = MultiByteToWideChar(CP_UTF8,0,str.c_str(),str.size(),NULL,0);
	std::wstring wstr(w_len, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.size(), &wstr[0], w_len);
	return wstr;
}


INT_PTR CALLBACK  Dialog_eighteen(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//������Ϣ��//
{

	switch (uMsg)
	{

	case WM_INITDIALOG:
	{

		SetDlgItemText(hwndDlg, IDC_EDIT50,L"�˺�");
		SetDlgItemText(hwndDlg, IDC_EDIT51, L"�ǳ�");
		SetDlgItemText(hwndDlg, IDC_EDIT52, L"�Ա�");
		SetDlgItemText(hwndDlg, IDC_EDIT53, L"����");
		SetDlgItemText(hwndDlg, IDC_EDIT54, L"����ǩ��");

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
		   //�����������ȡ����ť
			std::string sd = "ȡ��";
			int sd_len = sd.size();
			send(g_clientSocket,(char*)&sd_len,sizeof(sd_len),0);
			send(g_clientSocket,sd.c_str(),sd_len,0);

			//�رպ�����Ϣչʾ��
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


INT_PTR CALLBACK Dialog_nineteen(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//��������Ի���//
{
	
	switch (uMsg)
	{
	
	case WM_INITDIALOG:
	{
		//���÷��Ͱ�ť
		HWND hBton1 = GetDlgItem(hwndDlg,IDOK);
		EnableWindow(hBton1,FALSE);
		//����ı���
		SetDlgItemText(hwndDlg,IDC_EDIT61,L"");

		//��Ҫ������ǰ�������¼
		
		//����������ͼ�����ǰ�����¼������
		std::string r_str = "��������������Ϣ";
		int r_str_len = r_str.size();
		send(g_clientSocket, (char*)&r_str_len, sizeof(r_str_len), 0);
		send(g_clientSocket, r_str.c_str(), r_str_len, 0);

		char* buf = new char[50];
		snprintf(buf, 50, "the r_str is %s\n", r_str.c_str());
		OutputDebugStringA(buf);
		delete[]buf;

		//�����ܵ��������Ϣ����
		int msg_len = 0;
		recv(g_clientSocket, (char*)&msg_len, sizeof(msg_len), 0);

		char* buf2 = new char[50];
		snprintf(buf2, 50, "the msg_len is %d\n", msg_len);
		OutputDebugStringA(buf2);
		delete[]buf2;

		//�������Ϊ0
		if (msg_len <= 0)
		{
			//��ʼ������
			return (INT_PTR)TRUE;
		}
		else
		{
			int i = 0;
			g_users_chart_information.clear();

			//���շ��͵���Ϣ
			while (i < msg_len)
			{
				ss = users_chart_information();//��սṹ��

				//������Ϣ���ͷ��˺�
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

				//������Ϣ���շ��˺�
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


				//��Ϣ
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

				//ѹ�����
				g_users_chart_information.push_back(ss);

				i++;
			}


			//��һ���ĸ�ʽ��ʾ�����¼
			
			
			//��������
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
		// ��ÿ��߶�����Ӧ����
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

				// ������Ϣ���ݵĸ߶�
				const users_chart_information& item = g_users_chart_information[idx];
				std::wstring wmsg;
				int wlen = MultiByteToWideChar(CP_UTF8, 0, item.inf.c_str(), (int)item.inf.size(), NULL, 0);
				wmsg.resize(wlen);
				MultiByteToWideChar(CP_UTF8, 0, item.inf.c_str(), (int)item.inf.size(), &wmsg[0], wlen);

				HDC hdc = GetDC(hwndDlg);
				RECT rcMsg = { 0, 0,contentWidth, 0 }; // ����ͷ��ͱ߾�
				DrawTextW(hdc, wmsg.c_str(), (int)wmsg.size(), &rcMsg, DT_WORDBREAK | DT_CALCRECT);
				ReleaseDC(hwndDlg, hdc);

				int nicknameHeight = 18;
				int padding = 8;
				int minHeight = avatarSize + 8;
				int contentHeight = rcMsg.bottom - rcMsg.top + nicknameHeight + padding;
				lpmis->itemHeight = max(minHeight, contentHeight); // ����ͷ��߶�
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

				// ����
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

				if (strcmp(account_recver.c_str(),item.inf_send_account.c_str()) == 0)//����Ϣ�Ǻ��ѷ��͵�
				{
					char* buf13 = new char[50];
					snprintf(buf13, 50, "the image_recver is %d\n", image_recver.size());
					OutputDebugStringA(buf13);
					delete[]buf13;

					// ͷ��
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

					// �ǳ�
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

					// ͷ��
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

					// �ǳ�
					RECT rcNick = lpdis->rcItem;
					rcNick.left += avatarSize + 10;
					rcNick.top += 4;
					rcNick.bottom = rcNick.top + 18;
					SetTextColor(lpdis->hDC, RGB(0, 102, 204));
					DrawText(lpdis->hDC,wnickname.c_str(),wlen_x, &rcNick, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

				}

				// ��Ϣ����
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

				// ѡ�б߿�
				if (lpdis->itemState & ODS_SELECTED)
					DrawFocusRect(lpdis->hDC, &lpdis->rcItem);

			}
			return (INT_PTR)TRUE;
		}
	}break;

	
	case WM_COMMAND:
	{
		//����ı����Ƿ������ݣ��������÷��Ͱ�ť����û�н��÷��Ͱ�ť

		 HWND hList11 = GetDlgItem(hwndDlg,IDC_EDIT61);
		 HWND hBton_ok= GetDlgItem(hwndDlg, IDOK);

		if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_EDIT61)
		{
			int len =GetWindowTextLength(hList11);
			EnableWindow(hBton_ok,len>0);
		}

		switch (LOWORD(wParam))
		{

		case IDC_UPDATEINFORMATION://������Ϣ
		{

			//����������͸�����Ϣ������
			std::string r_str = "������Ϣ";
			int r_str_len = r_str.size();
			send(g_clientSocket, (char*)&r_str_len, sizeof(r_str_len), 0);
			send(g_clientSocket, r_str.c_str(), r_str_len, 0);


			//����������ͼ�����ǰ�����¼������
			std::string r_str_x = "��������������Ϣ";
			int r_str_len_x = r_str_x.size();
			send(g_clientSocket, (char*)&r_str_len_x, sizeof(r_str_len_x), 0);
			send(g_clientSocket, r_str_x.c_str(), r_str_len_x, 0);

			//�����ܵ��������Ϣ����
			int msg_len = 0;
			recv(g_clientSocket, (char*)&msg_len, sizeof(msg_len), 0);

			//�������Ϊ0
			if (msg_len <= 0)
			{
				//��ʼ������
				return (INT_PTR)TRUE;
			}
			else
			{
				int i = 0;
				g_users_chart_information.clear();

				//���շ��͵���Ϣ
				while (i < msg_len)
				{
					ss = users_chart_information();//��սṹ��
				
					//������Ϣ���ͷ��˺�
					int account_send_len = 0;
					recv(g_clientSocket, (char*)&account_send_len, sizeof(account_send_len), 0);
					ss.inf_send_account.resize(account_send_len);
					recv(g_clientSocket, &ss.inf_send_account[0], account_send_len, 0);

					char* buf3 = new char[50];
					snprintf(buf3, 50, "the  ss.inf_send_account is %s\n", ss.inf_send_account.c_str());
					OutputDebugStringA(buf3);
					delete[]buf3;

					//������Ϣ���շ��˺�
					int account_recv_len = 0;
					recv(g_clientSocket, (char*)&account_recv_len, sizeof(account_recv_len), 0);
					ss.inf_recv_account.resize(account_recv_len);
					recv(g_clientSocket, &ss.inf_recv_account[0], account_recv_len, 0);

					char* buf4 = new char[50];
					snprintf(buf4, 50, "the  ss.inf_recv_account is %s\n", ss.inf_recv_account.c_str());
					OutputDebugStringA(buf4);
					delete[]buf4;

					//��Ϣ
					int inf_len = 0;
					recv(g_clientSocket, (char*)&inf_len, sizeof(inf_len), 0);
					ss.inf.resize(inf_len);
					recv(g_clientSocket, &ss.inf[0], inf_len, 0);

					char* buf5 = new char[50];
					snprintf(buf5, 50, "the  ss.inf is %s\n", ss.inf.c_str());
					OutputDebugStringA(buf5);
					delete[]buf5;

					//ѹ�����
					g_users_chart_information.push_back(ss);

					i++;
				}

				//��һ���ĸ�ʽ��ʾ�����¼

				//��������
				HWND n_x = GetDlgItem(hwndDlg, IDC_EDIT60);
				SendMessage(n_x, LB_RESETCONTENT, 0, 0);

				for (int k = 0; k < msg_len; k++)
				{
					SendDlgItemMessage(hwndDlg, IDC_EDIT60, LB_ADDSTRING, 0, (LPARAM)L"");
				}

			}


		}break;

		case IDOK://������Ϣ
		{
			//����������͸�����Ϣ������
			std::string r_str = "������Ϣ";
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

			//�����ַ���ת��Ϊ�ַ���
			int utf8_len = WideCharToMultiByte(CP_UTF8,0,text_str.c_str(),text_str.size(),NULL,0,NULL,NULL);
			std::string utf8_str(utf8_len,'\0');
			WideCharToMultiByte(CP_UTF8, 0, text_str.c_str(), text_str.size(),&utf8_str[0],utf8_len,NULL,NULL);
		
			send(g_clientSocket, (char*)&utf8_len, sizeof(utf8_len), 0);
			send(g_clientSocket,utf8_str.c_str(),utf8_len,0);

			//����ı���
			SetDlgItemText(hwndDlg,IDC_EDIT61,L"");

			//������Ϣ

			
			//����������ͼ�����ǰ�����¼������
			std::string r_str_x_12 = "��������������Ϣ";
			int r_str_len_x_12 = r_str_x_12.size();
			send(g_clientSocket, (char*)&r_str_len_x_12, sizeof(r_str_len_x_12), 0);
			send(g_clientSocket, r_str_x_12.c_str(), r_str_len_x_12, 0);

			//�����ܵ��������Ϣ����
			int msg_len = 0;
			recv(g_clientSocket, (char*)&msg_len, sizeof(msg_len), 0);

			//�������Ϊ0
			if (msg_len <= 0)
			{
				//��ʼ������
				return (INT_PTR)TRUE;
			}
			else
			{
				int i = 0;
				g_users_chart_information.clear();

				//���շ��͵���Ϣ
				while (i < msg_len)
				{
					ss = users_chart_information();//��սṹ��

					//������Ϣ���ͷ��˺�
					int account_send_len = 0;
					recv(g_clientSocket, (char*)&account_send_len, sizeof(account_send_len), 0);
					ss.inf_send_account.resize(account_send_len);
					recv(g_clientSocket, &ss.inf_send_account[0], account_send_len, 0);

					char* buf3 = new char[50];
					snprintf(buf3, 50, "the  ss.inf_send_account is %s\n", ss.inf_send_account.c_str());
					OutputDebugStringA(buf3);
					delete[]buf3;

					//������Ϣ���շ��˺�
					int account_recv_len = 0;
					recv(g_clientSocket, (char*)&account_recv_len, sizeof(account_recv_len), 0);
					ss.inf_recv_account.resize(account_recv_len);
					recv(g_clientSocket, &ss.inf_recv_account[0], account_recv_len, 0);

					char* buf4 = new char[50];
					snprintf(buf4, 50, "the  ss.inf_recv_account is %s\n", ss.inf_recv_account.c_str());
					OutputDebugStringA(buf4);
					delete[]buf4;

					//��Ϣ
					int inf_len = 0;
					recv(g_clientSocket, (char*)&inf_len, sizeof(inf_len), 0);
					ss.inf.resize(inf_len);
					recv(g_clientSocket, &ss.inf[0], inf_len, 0);

					char* buf5 = new char[50];
					snprintf(buf5, 50, "the  ss.inf is %s\n", ss.inf.c_str());
					OutputDebugStringA(buf5);
					delete[]buf5;

					//ѹ�����
					g_users_chart_information.push_back(ss);

					i++;
				}

				//��һ���ĸ�ʽ��ʾ�����¼

				//��������
				HWND n_x = GetDlgItem(hwndDlg, IDC_EDIT60);
				SendMessage(n_x, LB_RESETCONTENT, 0, 0);

				for (int k = 0; k < msg_len; k++)
				{
					SendDlgItemMessage(hwndDlg, IDC_EDIT60, LB_ADDSTRING, 0, (LPARAM)L"");
				}

			}

			return (INT_PTR)TRUE;

		}break;

		case IDCANCEL://�˳��ÿ�
		{
			//����������͸�����Ϣ������
			std::string r_str = "�˳�";
			int r_str_len = r_str.size();
			send(g_clientSocket, (char*)&r_str_len, sizeof(r_str_len), 0);
			send(g_clientSocket, r_str.c_str(), r_str_len, 0);

			//������Ի���ر�
			EndDialog(hwndDlg, IDCANCEL);
			return (INT_PTR)TRUE;

		}break;

		}


	}break;

	}

    return (INT_PTR)FALSE;
}



INT_PTR CALLBACK  Dialog_thirdteen(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//�����б��//
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
		
		//������غ����б�
		std::string msg = "������غ����б�";
		int msg_len = msg.size();
		send(g_clientSocket, (char*)&msg_len, sizeof(msg_len), 0);
		send(g_clientSocket, msg.c_str(), msg_len, 0);
		//���շ������Ƿ�ɹ����տͻ�������
		int recv_len = 0;
		recv(g_clientSocket, (char*)&recv_len, sizeof(recv_len), 0);
		std::string recv_str(recv_len, '\0');
		recv(g_clientSocket, &recv_str[0], recv_len, 0);
		//�ȽϽ�������
		if (strcmp("success", recv_str.c_str()) != 0)
		{
			MessageBox(NULL, L"������δ�ɹ����ռ��ؿͻ��˵ĺ����б�����", L"QQ", MB_ICONERROR);
			//�رմ��ڣ����ص�¼������ҳ��
			EndDialog(hwndDlg, IDCANCEL);
			return (INT_PTR)TRUE;
		}
		else
		{
			MessageBox(NULL, L"�������ѳɹ����ռ��ؿͻ��˵ĺ����б�����", L"QQ", MB_ICONINFORMATION);
		}
		//���պ����б������
		int total_count = 0;
		recv(g_clientSocket, (char*)&total_count, sizeof(total_count), 0);

		if (total_count > 0)
		{
			MessageBox(NULL, L"�������غ����б�", L"QQ", MB_ICONINFORMATION);
			//���պ����б����Ϣ��
			int i = 0;
			g_user_friends.clear();//ȷ��������Ϣ�������
			user_friend u;
			while (i < total_count)
			{
				u = user_friend();//��սṹ��

				//���պ������߱�־
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


				//���պ����˺�
				int account_x_len = 0;
				recv(g_clientSocket, (char*)&account_x_len, sizeof(account_x_len), 0);
				u.account.resize(account_x_len);//���·���ռ�
				recv(g_clientSocket, &u.account[0], account_x_len, 0);

				char* buf = new char[50]();
				snprintf(buf,50,"the friend_account is %s\n",u.account.c_str());
				OutputDebugStringA(buf);
				delete[]buf;

				//���պ����ǳ�
				int nickname_x_len = 0;
				recv(g_clientSocket, (char*)&nickname_x_len, sizeof(nickname_x_len), 0);
				u.nickname.resize(nickname_x_len);//���·���ռ�
				recv(g_clientSocket, &u.nickname[0], nickname_x_len, 0);

				char* buf_2 = new char[50]();
				snprintf(buf_2, 50, "the friend_nickname is %s\n", u.nickname.c_str());
				OutputDebugStringA(buf_2);
				delete[]buf_2;

				//���պ���ͷ��
				int image_x_len = 0;
				recv(g_clientSocket, (char*)&image_x_len, sizeof(image_x_len), 0);


				char* buf_3 = new char[50]();
				snprintf(buf_3, 50, "the friend_photo_len is %d\n",image_x_len);
				OutputDebugStringA(buf_3);
				delete[]buf_3;


                u.image.resize(image_x_len);//���·���ռ�
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
				//������ϣ������ѵ���Ϣѹ�뵯��
				g_user_friends.push_back(u);

				i++;
			}

			MessageBox(NULL, L"�Ѿ��ɹ������û��ĺ�����Ϣ", L"QQ", MB_ICONINFORMATION);
			//������û�������Ϣ�б�Ľ���
			//���û������б���һ���ĸ�ʽ��ʾ�ڿؼ�IDC_EDIT46��
			HWND hList12 = GetDlgItem(hwndDlg, IDC_EDIT46);
			SendMessage(hList12, LB_RESETCONTENT, 0, 0);//����б��
			for (size_t i = 0; i < g_user_friends.size(); ++i)
			{
				SendDlgItemMessage(hwndDlg, IDC_EDIT46, LB_ADDSTRING, 0, (LPARAM)L"");
				SendDlgItemMessage(hwndDlg, IDC_EDIT46, LB_SETITEMHEIGHT, i, 48); // ÿ��48����
			}
			return (INT_PTR)TRUE;
		}

		{
			MessageBox(NULL, L"�����б�Ϊ��", L"QQ", MB_ICONINFORMATION);
			//��ʼ������
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

				// ����
				SetBkColor(lpdis->hDC, (lpdis->itemState & ODS_SELECTED) ? RGB(230,230, 250) : RGB(255, 255, 255));
				ExtTextOut(lpdis->hDC, 0, 0, ETO_OPAQUE, &lpdis->rcItem, NULL, 0, NULL);

				// ͷ��
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

				// �ǳ�
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

				std::wstring wsignal = L"����";
				// ���߱�־
				if (item.signal)
				{
					wsignal = L"����";
				}
				else
				{
					wsignal = L"����";
				}

				RECT rcSignal = lpdis->rcItem;
				rcSignal.left += avatarSize + 10 + t_wideth + 30;
				rcSignal.top += 4;
				rcSignal.bottom = rcNick.top + 18;
				SetTextColor(lpdis->hDC, RGB(232, 46, 115));
				DrawText(lpdis->hDC, wsignal.c_str(), (int)wsignal.size(), &rcSignal, DT_LEFT | DT_SINGLELINE | DT_VCENTER);


				// �˺�����
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


				// ѡ�б߿�
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
			EnableWindow(hBtn1, selIdx != LB_ERR); // ��ѡ����ʱ���ð�ť���������
			EnableWindow(hBtn2, selIdx != LB_ERR); // ��ѡ����ʱ���ð�ť���������
			EnableWindow(hBtn3, selIdx != LB_ERR); // ��ѡ����ʱ���ð�ť���������

			char* buf_k = new char[50];
			snprintf(buf_k,strlen(buf_k),"the selIdex is %d\n",selIdx);
			OutputDebugStringA(buf_k);
			delete[]buf_k;
		}
		
		switch (LOWORD(wParam))
		{

		case IDC_UPDATE_LIST:
		{
			std::string s = "ˢ���б�";
			int s_l = s.size();
			send(g_clientSocket, (char*)&s_l, sizeof(s_l), 0);
			send(g_clientSocket, s.c_str(), s_l, 0);

			//���¼��غ����б�

			//������غ����б�
			std::string msg = "������غ����б�";
			int msg_len = msg.size();
			send(g_clientSocket, (char*)&msg_len, sizeof(msg_len), 0);
			send(g_clientSocket, msg.c_str(), msg_len, 0);
			//���շ������Ƿ�ɹ����տͻ�������
			int recv_len = 0;
			recv(g_clientSocket, (char*)&recv_len, sizeof(recv_len), 0);
			std::string recv_str(recv_len, '\0');
			recv(g_clientSocket, &recv_str[0], recv_len, 0);
			//�ȽϽ�������
			if (strcmp("success", recv_str.c_str()) != 0)
			{
				MessageBox(NULL, L"������δ�ɹ����ռ��ؿͻ��˵ĺ����б�����", L"QQ", MB_ICONERROR);
				//�رմ��ڣ����ص�¼������ҳ��
				EndDialog(hwndDlg, IDCANCEL);
				return (INT_PTR)TRUE;
			}
			else
			{
				MessageBox(NULL, L"�������ѳɹ����ռ��ؿͻ��˵ĺ����б�����", L"QQ", MB_ICONINFORMATION);
			}
			//���պ����б������
			int total_count = 0;
			recv(g_clientSocket, (char*)&total_count, sizeof(total_count), 0);

			if (total_count > 0)
			{
				MessageBox(NULL, L"�������غ����б�", L"QQ", MB_ICONINFORMATION);
				//���պ����б����Ϣ��
				int i = 0;
				g_user_friends.clear();//ȷ��������Ϣ�������
				user_friend u;
				while (i < total_count)
				{
					u = user_friend();//��սṹ��

					//���պ������߱�־
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


					//���պ����˺�
					int account_x_len = 0;
					recv(g_clientSocket, (char*)&account_x_len, sizeof(account_x_len), 0);
					u.account.resize(account_x_len);//���·���ռ�
					recv(g_clientSocket, &u.account[0], account_x_len, 0);

					char* buf = new char[50]();
					snprintf(buf, 50, "the friend_account is %s\n", u.account.c_str());
					OutputDebugStringA(buf);
					delete[]buf;

					//���պ����ǳ�
					int nickname_x_len = 0;
					recv(g_clientSocket, (char*)&nickname_x_len, sizeof(nickname_x_len), 0);
					u.nickname.resize(nickname_x_len);//���·���ռ�
					recv(g_clientSocket, &u.nickname[0], nickname_x_len, 0);

					char* buf_2 = new char[50]();
					snprintf(buf_2, 50, "the friend_nickname is %s\n", u.nickname.c_str());
					OutputDebugStringA(buf_2);
					delete[]buf_2;

					//���պ���ͷ��
					int image_x_len = 0;
					recv(g_clientSocket, (char*)&image_x_len, sizeof(image_x_len), 0);


					char* buf_3 = new char[50]();
					snprintf(buf_3, 50, "the friend_photo_len is %d\n", image_x_len);
					OutputDebugStringA(buf_3);
					delete[]buf_3;


					u.image.resize(image_x_len);//���·���ռ�
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
					//������ϣ������ѵ���Ϣѹ�뵯��
					g_user_friends.push_back(u);

					i++;
				}

				MessageBox(NULL, L"�Ѿ��ɹ������û��ĺ�����Ϣ", L"QQ", MB_ICONINFORMATION);
				//������û�������Ϣ�б�Ľ���
				//���û������б���һ���ĸ�ʽ��ʾ�ڿؼ�IDC_EDIT46��
				HWND hList12 = GetDlgItem(hwndDlg, IDC_EDIT46);
				SendMessage(hList12, LB_RESETCONTENT, 0, 0);//����б��
				for (size_t i = 0; i < g_user_friends.size(); ++i)
				{
					SendDlgItemMessage(hwndDlg, IDC_EDIT46, LB_ADDSTRING, 0, (LPARAM)L"");
					SendDlgItemMessage(hwndDlg, IDC_EDIT46, LB_SETITEMHEIGHT, i, 48); // ÿ��48����
				}
				return (INT_PTR)TRUE;
			}

			{
				MessageBox(NULL, L"�����б�Ϊ��", L"QQ", MB_ICONINFORMATION);
				//��ʼ������
				return (INT_PTR)TRUE;
			}


		}break;

		case IDC_CHART://����
		{
			//������߼�
			//������������˳���Ϣ
			std::string s = "����";
			int s_l = s.size();
			send(g_clientSocket, (char*)&s_l, sizeof(s_l), 0);
			send(g_clientSocket, s.c_str(), s_l, 0);

			//��ȡ���������˺š��ǳƺ�ͷ��
			HWND hList15 = GetDlgItem(hwndDlg, IDC_EDIT46);
			int selIdx = (int)SendMessage(hList15, LB_GETCURSEL, 0, 0);
			std::string& account_recver_x = g_user_friends[selIdx].account;

			account_recver.clear();
			account_recver = account_recver_x;

			nickname_recver.clear();
			image_recver.clear();

			nickname_recver = g_user_friends[selIdx].nickname;
			image_recver= g_user_friends[selIdx].image;

			//�������������˺�
			int recver_account_len = 0;
			recver_account_len = account_recver_x.size();
			send(g_clientSocket, (char*)&recver_account_len, sizeof(recver_account_len), 0);
			send(g_clientSocket, account_recver_x.c_str(), recver_account_len, 0);

			//�����û��ǳ�
			int nickname_len = 0;
			recv(g_clientSocket, (char*)&nickname_len, sizeof(nickname_len), 0);
			user_nickname_data.resize(nickname_len);
			recv(g_clientSocket,&user_nickname_data[0],nickname_len,0);

			EndDialog(hwndDlg, IDC_CHART);
			return (INT_PTR)TRUE;
		}break;

		case IDC_FRIEND_INFORMATION://������Ϣ
		{
			//������������˳���Ϣ
			std::string s = "�鿴������Ϣ";
			int s_l = s.size();
			send(g_clientSocket, (char*)&s_l, sizeof(s_l), 0);
			send(g_clientSocket, s.c_str(), s_l, 0);

			HWND hList16 = GetDlgItem(hwndDlg, IDC_EDIT46);
			int selIdx = (int)SendMessage(hList16, LB_GETCURSEL, 0, 0);

			std::string& account = g_user_friends[selIdx].account;

			//����ѡ������˺�
			int acc_len = account.size();
			send(g_clientSocket, (char*)&acc_len, sizeof(acc_len), 0);
			send(g_clientSocket, account.c_str(), acc_len, 0);

			char* buf_account = new char[50]();
			snprintf(buf_account, 50, "the fri_account is %s\n",account.c_str());
			OutputDebugStringA(buf_account);
			delete[]buf_account;


			//���ոú��ѵĸ�����Ϣ
			xxy = my_user_information();//��սṹ��
			xxy.account = g_user_friends[selIdx].account;
			xxy.nickname = g_user_friends[selIdx].nickname;

			//���պ����Ա�
			int fri_gender_len = 0;
			recv(g_clientSocket, (char*)&fri_gender_len, sizeof(fri_gender_len), 0);
			std::string fri_gender(fri_gender_len, '\0');
			recv(g_clientSocket,&fri_gender[0],fri_gender_len,0);
			xxy.gender = fri_gender;

			char* buf_gender = new char[50]();
			snprintf(buf_gender,50,"the fri_gender is %s\n",fri_gender.c_str());
			OutputDebugStringA(buf_gender);
			delete[]buf_gender;

			//���պ��ѵ�����
			int fri_age_len = 0;
			recv(g_clientSocket, (char*)&fri_age_len, sizeof(fri_age_len), 0);
			std::string fri_age(fri_age_len, '\0');
			recv(g_clientSocket, &fri_age[0], fri_age_len, 0);
			xxy.age = fri_age;

			char* buf_age = new char[50]();
			snprintf(buf_age, 50, "the fri_age is %s\n", fri_age.c_str());
			OutputDebugStringA(buf_age);
			delete[]buf_age;

			//���պ��ѵĸ���ǩ��
			int fri_signature_len = 0;
			recv(g_clientSocket, (char*)&fri_signature_len, sizeof(fri_signature_len), 0);
			std::string fri_signature(fri_signature_len, '\0');
			recv(g_clientSocket, &fri_signature[0], fri_signature_len, 0);
			xxy.signature = fri_signature;

			char* buf_signature = new char[50]();
			snprintf(buf_signature, 50, "the fri_gender is %s\n", fri_signature.c_str());
			OutputDebugStringA(buf_signature);
			delete[]buf_signature;


			//��ת��������Ϣչʾ��
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MYDIALOG_EIGHTEEN), NULL, Dialog_eighteen);

			//���շ����������־
			int x_len = 0;
			recv(g_clientSocket, (char*)&x_len, sizeof(x_len), 0);
			std::string x_str(x_len, '\0');
			recv(g_clientSocket, &x_str[0], x_len, 0);

			if (strcmp("success", x_str.c_str()) == 0)
			{
				MessageBox(NULL, L"�������Ѿ��ɹ��������Ĳ鿴������Ϣ������", L"QQ", MB_ICONINFORMATION);
			}

			EndDialog(hwndDlg, IDC_FRIEND_INFORMATION);
			return (INT_PTR)TRUE;
		}break;

		case IDC_ADD_FRIEND://��Ӻ���
		{
			//�������������Ӻ��ѵ�����
			std::string buf = "�ͻ���������Ӻ���";
			int buf_len = buf.size();
			send(g_clientSocket, (char*)&buf_len, sizeof(buf_len), 0);
			send(g_clientSocket,buf.c_str(),buf_len,0);


			EndDialog(hwndDlg,IDC_ADD_FRIEND);
			return (INT_PTR)TRUE;
		}break;

		case IDC_DELETE_FRIEND://ɾ������
		{
			//������������˳���Ϣ
			std::string s = "ɾ������";
			int s_l = s.size();
			send(g_clientSocket, (char*)&s_l, sizeof(s_l), 0);
			send(g_clientSocket, s.c_str(), s_l, 0);

			HWND hList17 = GetDlgItem(hwndDlg,IDC_EDIT46);
			int selIdx = (int)SendMessage(hList17, LB_GETCURSEL, 0, 0);

			std::string& account = g_user_friends[selIdx].account;
		    
			//����ѡ������˺�
			int acc_len = account.size();
			send(g_clientSocket,(char*)&acc_len,sizeof(acc_len),0);
			send(g_clientSocket, account.c_str(), acc_len, 0);


			//���շ����������־
			int x_len = 0;
			recv(g_clientSocket, (char*)&x_len, sizeof(x_len), 0);
			std::string x_str(x_len,'\0');
			recv(g_clientSocket, &x_str[0], x_len, 0);

			if (strcmp("success", x_str.c_str()) == 0)
			{
				MessageBox(NULL,L"�������Ѿ��ɹ���������ɾ�����ѵ�����",L"QQ",MB_ICONINFORMATION);
			}

			EndDialog(hwndDlg, IDC_DELETE_FRIEND);
			return (INT_PTR)TRUE;
		}break;

		case IDC_NEW_FRIEND://������
		{
			//������������˳���Ϣ
			std::string s = "������";
			int s_l = s.size();
			send(g_clientSocket, (char*)&s_l, sizeof(s_l), 0);
			send(g_clientSocket, s.c_str(), s_l, 0);

			EndDialog(hwndDlg, IDC_NEW_FRIEND);
			return (INT_PTR)TRUE;
		}break;

		case IDCANCEL://�˳�
		{
			//������������˳���Ϣ
			std::string s = "�˳�";
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
		int r = recv(socket, buf + sum, buf_size - sum, 0);//���ν�����
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

void recv_personal_information_from_server()//�����û�������Ϣ
{
	//��������
	int password_len = 0;
	int len1=recv_all(g_clientSocket, (char*)&password_len, sizeof(password_len), 0);
	if (len1!= sizeof(password_len))
	{
		MessageBox(NULL, L"���ո�����Ϣ�����볤��ʧ��", L"QQ", MB_ICONERROR);
		return;
	}
	std::string password_buf(password_len, '\0');
	recv_all(g_clientSocket, &password_buf[0], password_len, 0);
	old_my_user_information.password = password_buf;
	//ת��Ϊ���ַ��� 
	int w_password_len = MultiByteToWideChar(CP_UTF8, 0, password_buf.c_str(), password_buf.size(), NULL, 0);
	std::wstring w_password_str(w_password_len, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, password_buf.c_str(), password_buf.size(), &w_password_str[0], w_password_len);
	w_old_my_user_information.password = w_password_str;

	//�����ǳ�
	int nickname_len = 0;
	int len2= recv_all(g_clientSocket, (char*)&nickname_len, sizeof(nickname_len), 0);
	if (len2!= sizeof(nickname_len))
	{
		MessageBox(NULL, L"���ո�����Ϣ���ǳƳ���ʧ��", L"QQ", MB_ICONERROR);
		return;
	}
	std::string nickname_buf(nickname_len, '\0');
	recv_all(g_clientSocket, &nickname_buf[0], nickname_len, 0);
	old_my_user_information.nickname = nickname_buf;
	//ת��Ϊ���ַ��� 
	int w_nickname_len = MultiByteToWideChar(CP_UTF8, 0, nickname_buf.c_str(), nickname_buf.size(), NULL, 0);
	std::wstring w_nickname_str(w_nickname_len, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, nickname_buf.c_str(), nickname_buf.size(), &w_nickname_str[0], w_nickname_len);
	w_old_my_user_information.nickname = w_nickname_str;

	//�����Ա�
	int gender_len = 0;
	int len3= recv_all(g_clientSocket, (char*)&gender_len, sizeof(gender_len), 0);
	if (len3!= sizeof(gender_len))
	{
		MessageBox(NULL, L"���ո�����Ϣ���Ա𳤶�ʧ��", L"QQ", MB_ICONERROR);
		return;
	}
	std::string gender_buf(gender_len, '\0');
	recv_all(g_clientSocket, &gender_buf[0], gender_len, 0);
	old_my_user_information.gender = gender_buf;
	//ת��Ϊ���ַ��� 
	int w_gender_len = MultiByteToWideChar(CP_UTF8, 0, gender_buf.c_str(), gender_buf.size(), NULL, 0);
	std::wstring w_gender_str(w_gender_len, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, gender_buf.c_str(), gender_buf.size(), &w_gender_str[0], w_gender_len);
	w_old_my_user_information.gender = w_gender_str;

	//��������
	int age_len = 0;
	int len4 = recv_all(g_clientSocket, (char*)&age_len, sizeof(age_len), 0);
	if (len4 != sizeof(age_len))
	{
		MessageBox(NULL, L"���ո�����Ϣ�����䳤��ʧ��", L"QQ", MB_ICONERROR);
		return;
	}
	std::string age_buf(age_len, '\0');
	recv_all(g_clientSocket, &age_buf[0], age_len, 0);
	old_my_user_information.age = age_buf;
	//ת��Ϊ���ַ��� 
	int w_age_len = MultiByteToWideChar(CP_UTF8, 0, age_buf.c_str(), age_buf.size(), NULL, 0);
	std::wstring w_age_str(w_age_len, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, age_buf.c_str(), age_buf.size(), &w_age_str[0], w_age_len);
	w_old_my_user_information.age = w_age_str;

	//���ո���ǩ��
	int signature_len = 0;
	int len5 = recv_all(g_clientSocket, (char*)&signature_len, sizeof(signature_len), 0);
	if (len5 != sizeof(signature_len))
	{
		MessageBox(NULL, L"���ո�����Ϣ�ĸ���ǩ������ʧ��", L"QQ", MB_ICONERROR);
		return;
	}
	std::string signature_buf(signature_len, '\0');
	recv_all(g_clientSocket, &signature_buf[0], signature_len, 0);
	old_my_user_information.signature = signature_buf;
	//ת��Ϊ���ַ��� 
	int w_signature_len = MultiByteToWideChar(CP_UTF8, 0, signature_buf.c_str(), signature_buf.size(), NULL, 0);
	std::wstring w_signature_str(w_signature_len, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, signature_buf.c_str(), signature_buf.size(), &w_signature_str[0], w_signature_len);
	w_old_my_user_information.signature= w_signature_str;
	MessageBox(NULL, L"�Ѿ��ɹ����������û���Ϣ����", L"QQ", MB_ICONINFORMATION);
}


bool get_infor_from_id(HWND hwndDlg,int id,std::wstring &my_infor)//��ȡ�ؼ�����
{
	HWND hwndEdit = GetDlgItem(hwndDlg,id); // ��ȡ�ؼ����
	int len = GetWindowTextLength(hwndEdit);        // ��ȡ�ı����ȣ�����'\0'��
	if (len > 0)
	{
		my_infor.resize(len+1);           // Ԥ����ռ�
		GetWindowText(hwndEdit, &my_infor[0], len+1); // ��ȡ�ı�����
		if (!my_infor.empty()&&my_infor.back()=='\0')
		{
			my_infor.pop_back();
			return true;
		}
	}
	else
	{
		MessageBox(NULL, L"�������޸�����Ϊ��", L"QQ", MB_ICONERROR);
		return false;
	}
}

bool get_infor_gender_from_id(HWND hwndDlg, int id, std::wstring& my_infor)//��ȡ�ؼ�����
{
	HWND hwndEdit = GetDlgItem(hwndDlg, id); // ��ȡ�ؼ����
	int len = GetWindowTextLength(hwndEdit);        // ��ȡ�ı����ȣ�����'\0'��
	if (len > 0)
	{
		my_infor.resize(len + 1);           // Ԥ����ռ�
		GetWindowText(hwndEdit, &my_infor[0], len + 1); // ��ȡ�ı�����
		if (!my_infor.empty() && my_infor.back() == '\0')
		{
			my_infor.pop_back();
			if (!((wcscmp(my_infor.c_str(), L"��") == 0) || (wcscmp(my_infor.c_str(), L"Ů") == 0)))
			{
				MessageBox(NULL, L"��ȡ�ؼ��Ա�ʧ��,ֻ���ǡ��С� ��Ů��", L"QQ", MB_ICONINFORMATION);
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
		MessageBox(NULL, L"�������޸�����Ϊ��", L"QQ", MB_ICONERROR);
		return false;
	}
}

void send_new_user_infor(SOCKET socket, std::wstring& w_str)//���û�������Ϣ����
{
	int new_password_len = WideCharToMultiByte(CP_UTF8, 0, w_str.c_str(), w_str.size(), NULL, 0, NULL, NULL);
	std::string new_password_str(new_password_len, '\0');
	WideCharToMultiByte(CP_UTF8, 0, w_str.c_str(), w_str.size(), &new_password_str[0], new_password_len, NULL, NULL);
	send(g_clientSocket, (char*)&new_password_len, sizeof(new_password_len), 0);
	send(g_clientSocket, new_password_str.c_str(), new_password_len, 0);
}

INT_PTR CALLBACK Dialog_fourteen(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//��¼�������Ϣ��//
{
	switch (uMsg)
	{
	case WM_INITDIALOG:

		recv_personal_information_from_server();//�����û���������

		SetDlgItemText(hwndDlg, IDC_EDIT31, L"�˺�");
		SetDlgItemText(hwndDlg, IDC_EDIT32, L"����");
		SetDlgItemText(hwndDlg, IDC_EDIT33, L"�ǳ�");
		SetDlgItemText(hwndDlg, IDC_EDIT34, L"�Ա�");
		SetDlgItemText(hwndDlg, IDC_EDIT35, L"����");
		SetDlgItemText(hwndDlg, IDC_EDIT36, L"����ǩ��");
		SetDlgItemText(hwndDlg, IDC_EDIT37,w_new_online_user_account.c_str());//�˺�

		SetDlgItemText(hwndDlg, IDC_EDIT38,w_old_my_user_information.password.c_str());//����
		SetDlgItemText(hwndDlg, IDC_EDIT39, w_old_my_user_information.nickname.c_str());//�ǳ�
		SetDlgItemText(hwndDlg, IDC_EDIT40, w_old_my_user_information.gender.c_str());//�Ա�
		SetDlgItemText(hwndDlg, IDC_EDIT41, w_old_my_user_information.age.c_str());//����
		SetDlgItemText(hwndDlg, IDC_EDIT42, w_old_my_user_information.signature.c_str());//����ǩ��
        return (INT_PTR)TRUE;

	case WM_COMMAND:

		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			
			w_new_user_information my_user_new_information;
			//��ȡ�µ��û���Ϣ�洢��my_user_new_information�ṹ��
			
			if (!get_infor_from_id(hwndDlg, IDC_EDIT38, my_user_new_information.password))//��ȡ�ؼ�����
			{
				MessageBox(NULL, L"��ȡ�ؼ�����ʧ��", L"QQ", MB_ICONINFORMATION);
				return (INT_PTR)TRUE;
			}
			if (!get_infor_from_id(hwndDlg, IDC_EDIT39, my_user_new_information.nickname))//��ȡ�ؼ��ǳ�
			{
				MessageBox(NULL, L"��ȡ�ؼ��˺�ʧ��", L"QQ", MB_ICONINFORMATION);
				return (INT_PTR)TRUE;
			}
			if (!get_infor_gender_from_id(hwndDlg, IDC_EDIT40, my_user_new_information.gender))//��ȡ�ؼ��Ա�
			{
				//MessageBox(NULL, L"��ȡ�ؼ�����ʧ�ܣ�ֻ����С���Ů��", L"QQ", MB_ICONINFORMATION);
				return (INT_PTR)TRUE;
			}
			if (!get_infor_from_id(hwndDlg, IDC_EDIT41, my_user_new_information.age))//��ȡ�ؼ�����
			{
				MessageBox(NULL, L"��ȡ�ؼ�����ʧ��", L"QQ", MB_ICONINFORMATION);
				return (INT_PTR)TRUE;
			}
			if (!get_infor_from_id(hwndDlg, IDC_EDIT42, my_user_new_information.signature))//��ȡ�ؼ�����ǩ��
			{
				MessageBox(NULL, L"��ȡ�ؼ�����ǩ��ʧ��", L"QQ", MB_ICONINFORMATION);
				return (INT_PTR)TRUE;
			}
			//MessageBox(NULL, L"�Ѿ��ɹ���ȡ�µĸ�����Ϣ", L"QQ", MB_ICONINFORMATION);

			//�����������ȷ��ѡ��
			std::wstring my_sel = L"ȷ��";
			int utf8_my_sel = WideCharToMultiByte(CP_UTF8, 0, my_sel.c_str(), my_sel.size() + 1, NULL, 0, NULL, NULL);
			std::string utf8_my_sel_str(utf8_my_sel, '\0');
			WideCharToMultiByte(CP_UTF8, 0, my_sel.c_str(), my_sel.size() + 1, &utf8_my_sel_str[0], utf8_my_sel, NULL, NULL);
			send(g_clientSocket, (char*)&utf8_my_sel, sizeof(utf8_my_sel), 0);
			send(g_clientSocket, utf8_my_sel_str.c_str(), utf8_my_sel, 0);

			//���ṹ��my_user_new_information��洢����Ϣ����
			send_new_user_infor(g_clientSocket, my_user_new_information.password);//���û��������뷢��
			send_new_user_infor(g_clientSocket, my_user_new_information.nickname);//���û������ǳƷ���
			send_new_user_infor(g_clientSocket, my_user_new_information.gender);//���û������Ա𷢳�
			send_new_user_infor(g_clientSocket, my_user_new_information.age);//���û��������䷢��
			send_new_user_infor(g_clientSocket, my_user_new_information.signature);//���û����¸���ǩ������
			MessageBox(NULL, L"�Ѿ��ɹ���������Ϣ���ĵ�������������", L"QQ", MB_ICONINFORMATION);

			int recv_len= 0;
			recv(g_clientSocket,(char*)&recv_len,sizeof(recv_len),0);
			std::string recv_str(recv_len,'\0');
			recv(g_clientSocket,&recv_str[0],recv_len,0);
			if (strcmp(recv_str.c_str(), "update_success") == 0)
			{
				MessageBox(NULL, L"�������Ѿ��ɹ������û�������Ϣ", L"QQ", MB_ICONINFORMATION);
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
			//�����������ȷ��ѡ��
			std::wstring my_sel = L"ȡ��";
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

INT_PTR CALLBACK Dialog_fifteen(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//ע����//
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg,IDC_EDIT45,L"ȷ��ע���˺ţ�");
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			std::wstring w_char = L"ȷ��";
			int send_len = WideCharToMultiByte(CP_UTF8, 0, w_char.c_str(), w_char.size() + 1, NULL, 0, NULL, NULL);//�ȼ���ת������//
			std::string send_str(send_len, 0);//����ռ�//
			WideCharToMultiByte(CP_UTF8, 0, w_char.c_str(), w_char.size() + 1, &send_str[0], send_len, NULL, NULL);//ʵ��ת��//
			send(g_clientSocket, (char*)&send_len, sizeof(send_len), 0);//�ȷ�����//
			send(g_clientSocket, send_str.c_str(), send_len, 0);//�ٷ�����//
			
			EndDialog(hwndDlg, IDOK);
			return (INT_PTR)TRUE;
		}
		case IDCANCEL:
		{
			std::wstring w_char = L"ȡ��";
			int send_len = WideCharToMultiByte(CP_UTF8, 0, w_char.c_str(), w_char.size() + 1, NULL, 0, NULL, NULL);//�ȼ���ת������//
			std::string send_str(send_len, 0);//����ռ�//
			WideCharToMultiByte(CP_UTF8, 0, w_char.c_str(), w_char.size() + 1, &send_str[0], send_len, NULL, NULL);//ʵ��ת��//
			send(g_clientSocket, (char*)&send_len, sizeof(send_len), 0);//�ȷ�����//
			send(g_clientSocket, send_str.c_str(), send_len, 0);//�ٷ�����//

			EndDialog(hwndDlg, IDCANCEL);
			return (INT_PTR)TRUE;
		}

		}
	}
	return (INT_PTR)FALSE;
}


//���ַ����ı����ͺ���//
void send_wchar(WCHAR* wstr, SOCKET socket, WCHAR* sign)
{
	int sign_len = wcslen(sign);//��ȡ��ʶ���ַ�����//
	int utf8_signlen = WideCharToMultiByte(CP_UTF8, 0, sign, sign_len + 1, NULL, 0, NULL, NULL);//����utf8_signlen�ֽ���,��'\0'//
	std::string sign_str(utf8_signlen, 0);//����ռ䣬��'\0'//
	WideCharToMultiByte(CP_UTF8, 0, sign, sign_len + 1, &sign_str[0], utf8_signlen, NULL, NULL);//ʵ��ת��//
	int wchar_len = wcslen(wstr);//��ȡʵ���ַ������ȣ�������'\0'//
	int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wstr, wchar_len + 1, NULL, 0, NULL, NULL);//����utf-8�ֽ���������'\0'//
	std::string utf8str(utf8_len, 0);//����ռ�//
	//ʵ��ת��//
	WideCharToMultiByte(CP_UTF8, 0, wstr, wchar_len + 1, (char*)&utf8str[0], utf8_len, NULL, NULL);
	send(socket, (const char*)&utf8_signlen, sizeof(utf8_signlen), 0);//��ʶ�ֽڳ���//
	send(socket, (const char*)sign_str.c_str(), utf8_signlen, 0);//�ȷ���ʶ//
	//�ȷ����ȣ��ٷ�����//
	send(socket, (const char*)&utf8_len, sizeof(utf8_len), 0);//�����ֽڳ���//
	send(socket, (const char*)utf8str.c_str(), utf8_len, 0);//����//
}

//ͷ���ͺ���//
void send_byte(SOCKET socket, char* g_pImageData, int g_dwImageSize, WCHAR* sign)
{
	int sign_len = wcslen(sign);//��ȡ��ʶ���ַ�����//
	int utf8_signlen = WideCharToMultiByte(CP_UTF8, 0, sign, sign_len + 1, NULL, 0, NULL, NULL);//����utf8_signlen�ֽ���,��'\0'//
	std::string sign_str(utf8_signlen, 0);//����ռ䣬��'\0'//
	WideCharToMultiByte(CP_UTF8, 0, sign, sign_len + 1, (char*)&sign_str[0], utf8_signlen, NULL, NULL);//ʵ��ת��//
	send(socket, (const char*)&utf8_signlen, sizeof(utf8_signlen), 0);//��ʶ�ֽڳ���//
	send(socket, (const char*)sign_str.c_str(), utf8_signlen, 0);//�ȷ���ʶ//
	send(socket, (char*)&g_dwImageSize, sizeof(g_dwImageSize), 0);//�ٷ������ֽڳ���//
	int r = 0;//���η��͵��ֽ���//
	int sum = 0;//�ۼƷ��͵��ֽ���//
	while (sum < g_dwImageSize)
	{
		r = send(socket, g_pImageData + sum, g_dwImageSize - sum, 0);//�ٷ�����//
		if (r <= 0)
		{
			break;
		}
		sum += r;
	}
	if (sum < g_dwImageSize)
	{
		MessageBox(NULL, L"ͷ�����ݷ���ʧ��", L"QQ", MB_ICONERROR);
	}
	else if (sum == g_dwImageSize)
	{
		MessageBox(NULL, L"ͷ�����ݷ��ͳɹ�", L"QQ", MB_ICONINFORMATION);
	}
}

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE, LPSTR, int nCmdShow)//Winmain������//
{
	int result_one = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_ONE), NULL, Dialog_one);
	if (result_one == IDEND)//�˳�//  //һ���ж�//
	{
		DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_END), NULL, Dialog_end);//�˳���//
	}
	else if (result_one == IDOK)//һ���ж�//
	{
		WSADATA wsaData;
		int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (result)
		{
			MessageBox(NULL, L"Winsock��ʼ��ʧ��", L"QQ", MB_ICONERROR);
			WSACleanup();
		}
		if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
		{
			MessageBox(NULL, L"��֧��Winsock2.2", L"QQ", MB_ICONERROR);
			WSACleanup();
		}
		MessageBox(NULL, L"Winsock��ʼ���ɹ�", L"QQ", MB_ICONINFORMATION);
		INT_PTR result_nine = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_NINE), NULL, Dialog_nine);//IP�����//
		if (result_nine == IDCANCEL)//�˳�//  //�����ж�//
		{
			DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_END), NULL, Dialog_end);//�˳���//
		}
		else if (result_nine == IDOK)//�����ж�//
		{
		door_one:
			INT_PTR result_two = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_TWO), NULL, Dialog_two);//��¼��ע��ѡ���//
			if (result_two == IDLOGIN)//��¼//     //�����ж�//
			{
				std::wstring  wmsg = L"��¼";
				int wlen = wmsg.size();
				int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wlen + 1, NULL, 0, NULL, NULL);//����utf8_signlen�ֽ���,��'\0'//
				std::string msg(utf8len, 0);//����ռ�//
				WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wlen + 1, &msg[0], utf8len, NULL, NULL);//ʵ��ת��//
				send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//�������������Ϣ����//
				send(g_clientSocket, msg.c_str(), utf8len, 0);//�������������Ϣ����//
				MessageBox(NULL, L"������������͵�¼��Ϣ", L"QQ", MB_ICONINFORMATION);
			door_seven:
				INT_PTR result_three = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_THREE), NULL, Dialog_three);//��¼�Ի������˺ź�������֤��//
				if (result_three == IDOK) //�ļ��ж�//
				{
				door_two:
					//û���������ҳ��ǰ�������û�������Ϣ
					int result_ten = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_TEN), NULL, Dialog_ten);//��½������ҳ��//
					int result_fifteen = 0;
					int result_eleven = 0;
					int result_twelve = 0;
					int result_thirdteen = 0;
					int result_fourteen = 0;
					switch (result_ten)
					{
					case IDC_EDIT22:
						result_twelve = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_TWELVE), NULL, Dialog_twelve);//�����ҶԻ���//
						switch (result_twelve)
						{
						case IDOK:
						{
							//MessageBox(NULL, L"��δ����", L"QQ", MB_ICONINFORMATION);
							break;
						}
						case IDCANCEL:
							goto door_two;//��½������ҳ��//
							break;
						}


					case IDC_EDIT23:

						door_11:
						result_thirdteen = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_THIRDTEEN), NULL, Dialog_thirdteen);//���ѿ�//
						switch (result_thirdteen)
						{

						case IDC_CHART://�����
						{
							//������ѶԻ������
							int xx=DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MYDIALOG_NINETEEN), NULL, Dialog_nineteen);//������Ϣ��//

							switch (xx)
							{

							case IDCANCEL:
							{
								goto door_11;
							}break;

							}
							break;
						}

						case IDCANCEL://�˳���
						{
							goto door_two;//��½������ҳ��//
							break;
						}

						case IDC_FRIEND_INFORMATION://�鿴������Ϣ��
						{
							//���غ��ѿ�
							goto door_11;
							break;
						}

						case IDC_ADD_FRIEND://�������
						{

							INT_PTR result_20 = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_SIXTEEN), NULL, Dialog_sixteen);

							switch (result_20)
							{
							case IDOK:
							{
								//���غ��ѿ���ҳ
								MessageBox(NULL,L"�������غ�����ҳ��",L"QQ",MB_ICONINFORMATION);

								goto door_11;
								break;
							}
							case IDCANCEL:
							{
								//���غ��ѿ���ҳ
								MessageBox(NULL, L"�������غ�����ҳ��", L"QQ",MB_ICONINFORMATION);

								goto door_11;
								break;
							}
							}

							break;
						}

						case IDC_DELETE_FRIEND://ɾ������
						{
							//���غ��ѿ���ҳ
							MessageBox(NULL, L"�Ѿ��ɹ�������ѡ��ĺ���ɾ��", L"QQ", MB_ICONINFORMATION);
							goto door_11;
							break;
						}

						case IDC_NEW_FRIEND://������
						{
							//��ʾ�����ѿ�
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
								//���غ��ѿ���ҳ
								MessageBox(NULL, L"�������غ�����ҳ��", L"QQ", MB_ICONINFORMATION);
								goto door_11;
								break;
							}
							}

							break;
						}

						}break;


					case IDC_EDIT24:
						result_fourteen = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_FOURTEEN), NULL, Dialog_fourteen);//������Ϣ��//
						switch (result_fourteen)
						{
						case IDOK:
							//MessageBox(NULL, L"��δ����", L"QQ", MB_ICONINFORMATION);
							goto door_two;//��½������ҳ��//
							break;
						case IDCANCEL:
							goto door_two;//��½������ҳ��//
							break;
						}
					case IDC_EDIT25:
						result_fifteen = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_EIGHT), NULL, Dialog_eight);//����ͷ���//
						switch (result_fifteen)
						{
						case IDOK:
						{
							//��ȡ�µ�ͷ�����ݲ�����
							int new_img_len = g_dwImageSize;
							send(g_clientSocket, (char*)&new_img_len, sizeof(new_img_len), 0);
							send(g_clientSocket, (char*)g_pImageData,new_img_len,0);
							//������Ϣ���жϿͻ����Ƿ��Ѿ��ɹ������û�ͷ��
							int user_new_img_len = 0;
							int user_x=recv(g_clientSocket, (char*)&user_new_img_len,sizeof(user_new_img_len),0);
							if (user_x != sizeof(user_new_img_len))
							{
								MessageBox(NULL,L"���շ��������صĸ���ͷ�񳤶���Ϣʧ��",L"QQ",MB_ICONERROR);
							}
							else
							{
								MessageBox(NULL, L"���շ��������صĸ���ͷ����Ϣ���ȳɹ�", L"QQ", MB_ICONINFORMATION);
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
							if (wcscmp(my_wchar_str.c_str(),L"�������Ѿ��ɹ������û�ͷ��") == 0)
							{
								MessageBox(NULL, L"�������Ѿ��ɹ������û�ͷ��", L"QQ", MB_ICONINFORMATION);
							}
							else
							{
								MessageBox(NULL, L"���շ��������صĸ���ͷ����Ϣʧ��", L"QQ", MB_ICONINFORMATION);
							}
							goto door_two;//��½������ҳ��//
							break;
						}
						case IDCANCEL:
						{
							goto door_two;//��½������ҳ��//
							break;
						}
						}
					case IDC_EDIT44:
					{
						int result_sixteen = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_FIFTEEN), NULL, Dialog_fifteen);//ע���˺ſ�//
						switch (result_sixteen)
						{
						case IDOK:
							MessageBox(NULL, L"��֪ͨ���ݿ�����˺�ע��", L"QQ", MB_ICONINFORMATION);
							goto door_one;//��½������ҳ��//
							break;
						case IDCANCEL:
							goto door_two;//��½������ҳ��//
							break;
						}break;
					}
					case IDC_EDIT26:
						result_eleven = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_ELEVEN), NULL, Dialog_eleven);//��������//
						switch (result_eleven)
						{
						case IDOK:
							//MessageBox(NULL, L"��δ����", L"QQ", MB_ICONINFORMATION);
							break;
						case IDCANCEL:
							goto door_two;//��½������ҳ��//
							break;
						}
					case IDCANCEL:
						DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_END), NULL, Dialog_end);//�˳���//
						break;
					case IDC_EDIT43:
						goto door_one;//��½������ҳ��//
						break;
					}

				}
				else if (result_three == IDCANCEL)//�ļ��ж�//
				{
					goto door_one;//��¼��ע��ѡ���//
				}
				else if (result_three == IDC_EDIT_AGAIN)//�ļ��жϣ����������˺ź�����//
				{
					goto door_seven;
				}
			}
			else if (result_two == IDREGISTER)//ע��//   //�����ж�//
			{
				std::wstring  wmsg = L"ע��";
				int wlen = wmsg.size();
				int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wlen + 1, NULL, 0, NULL, NULL);//����utf8_signlen�ֽ���,��'\0'//
				std::string msg(utf8len, 0);//����ռ�//
				WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wlen + 1, &msg[0], utf8len, NULL, NULL);//ʵ��ת��//
				send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//�������������Ϣ����//
				send(g_clientSocket, msg.c_str(), utf8len, 0);//�������������Ϣ����//
				MessageBox(NULL, L"�������������ע����Ϣ", L"QQ", MB_ICONINFORMATION);
			door_three:
				INT_PTR result_four = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_FOUR), NULL, Dialog_four);//ע��Ի���//
				if (result_four == IDGOON)//�ļ��ж�//
				{
				door_four:
					INT_PTR result_five = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_FIVE), NULL, Dialog_five);//�������ÿ�//
					{
						if (result_five == IDFINISH)//�弶�ж�//
						{
						door_five:
							int result_seven = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_SEVEN), NULL, Dialog_seven);//������Ϣ��//
							if (result_seven == IDCANCEL)//�����ж�//
							{
								goto door_four;
							}
							else if (result_seven == IDOK)//�����ж�//
							{
							door_six:
								int result_eight = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_EIGHT), NULL, Dialog_eight);//ͷ���ϴ���//
								if (result_eight == IDCANCEL)//�߼��ж�//
								{
									goto door_five;
								}
								else if (result_eight == IDOK)//�߼��ж�//
								{
									INT_PTR result_six = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_SIX), NULL, Dialog_six);//ע��ɹ���//
									if (result_six == IDOK)//�˼��ж�//
									{
										//��ʼ����//
										send_wchar(g_szAccount, g_clientSocket, (WCHAR*)L"�˺�");//�˺�//
										send_wchar(g_szPassword, g_clientSocket, (WCHAR*)L"����");//����//
										send_wchar(g_szNickname, g_clientSocket, (WCHAR*)L"�ǳ�");//�ǳ�//
										send_wchar(g_szGender, g_clientSocket, (WCHAR*)L"�Ա�");//�Ա�//
										send_wchar(g_szAge, g_clientSocket, (WCHAR*)L"����");//����//
										send_wchar(g_szSignature, g_clientSocket, (WCHAR*)L"����ǩ��");//����ǩ��//
										send_byte(g_clientSocket, (char*)g_pImageData, g_dwImageSize, (WCHAR*)L"ͷ��");//ͷ��//
										MessageBox(NULL, L"�ѽ�����ע�����ݱ��浽������", L"QQ", MB_ICONINFORMATION);
										goto door_one;//���ص�¼��ע��ѡ���//
									}
									else if (result_six == IDCANCEL)//�˼��ж�//
									{
										goto door_six;
									}
								}
							}
						}
						else if (result_five == IDCANCEL)//�弶�ж�//
						{
							goto door_three;//ע��Ի���//
						}
					}
				}
				else if (result_four == IDCANCEL)//�ļ��ж�//
				{
					goto door_one;//���ص�¼��ע��ѡ���//
				}
			}
			else if (result_two == IDCANCEL)//�˳�//   //�����ж�//
			{
				std::wstring  wmsg = L"�˳�";
				int wlen = wmsg.size();
				int utf8len = WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wlen + 1, NULL, 0, NULL, NULL);//����utf8_signlen�ֽ���,��'\0'//
				std::string msg(utf8len, 0);//����ռ�//
				WideCharToMultiByte(CP_UTF8, 0, wmsg.c_str(), wlen + 1, &msg[0], utf8len, NULL, NULL);//ʵ��ת��//
				send(g_clientSocket, (char*)&utf8len, sizeof(utf8len), 0);//�������������Ϣ����//
				send(g_clientSocket, msg.c_str(), utf8len, 0);//�������������Ϣ����//
				MessageBox(NULL, L"��������������û��˳��ͻ�����Ϣ", L"QQ", MB_ICONINFORMATION);
				DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_END), NULL, Dialog_end);//�˳���//
			}
		}
	}
	return 0;
}


