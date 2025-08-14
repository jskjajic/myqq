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
			EndDialog(hwndDlg, IDCANCEL);
			return (INT_PTR)TRUE;
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
		return (INT_PTR)TRUE;
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

INT_PTR CALLBACK Dialog_twelve(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//�����ҶԻ���//
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

INT_PTR CALLBACK  Dialog_thirdteen(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//�����б��//
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

INT_PTR CALLBACK Dialog_fourteen(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//��¼�������Ϣ��//
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, IDC_EDIT31, L"�˺�");
		SetDlgItemText(hwndDlg, IDC_EDIT32, L"����");
		SetDlgItemText(hwndDlg, IDC_EDIT33, L"�ǳ�");
		SetDlgItemText(hwndDlg, IDC_EDIT34, L"�Ա�");
		SetDlgItemText(hwndDlg, IDC_EDIT35, L"����");
		SetDlgItemText(hwndDlg, IDC_EDIT36, L"����ǩ��");
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
							MessageBox(NULL, L"��δ����", L"QQ", MB_ICONINFORMATION);
							break;
						case IDCANCEL:
							goto door_two;//��½������ҳ��//
							break;
						}break;
					case IDC_EDIT23:
						result_thirdteen = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_THIRDTEEN), NULL, Dialog_thirdteen);//���ѿ�//
						switch (result_thirdteen)
						{
						case IDOK:
							MessageBox(NULL, L"��δ����", L"QQ", MB_ICONINFORMATION);
							break;
						case IDCANCEL:
							goto door_two;//��½������ҳ��//
							break;
						}break;
					case IDC_EDIT24:
						result_fourteen = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_FOURTEEN), NULL, Dialog_fourteen);//������Ϣ��//
						switch (result_fourteen)
						{
						case IDOK:
							MessageBox(NULL, L"��δ����", L"QQ", MB_ICONINFORMATION);
							break;
						case IDCANCEL:
							goto door_two;//��½������ҳ��//
							break;
						}break;
					case IDC_EDIT25:
						result_fifteen = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_EIGHT), NULL, Dialog_eight);//����ͷ���//
						switch (result_fifteen)
						{
						case IDOK:
							MessageBox(NULL, L"��δ����", L"QQ", MB_ICONINFORMATION);
							break;
						case IDCANCEL:
							goto door_two;//��½������ҳ��//
							break;
						}break;
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
							MessageBox(NULL, L"��δ����", L"QQ", MB_ICONINFORMATION);
							break;
						case IDCANCEL:
							goto door_two;//��½������ҳ��//
							break;
						}break;
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
				DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_END), NULL, Dialog_end);//�˳���//
			}
		}
	}
	return 0;
}



