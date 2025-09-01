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

//��¼ʱ�����µ�¼�û��ĸ�����Ϣ
struct my_user_information
{
	std::string password;//limit 32
	std::string nickname;//limit 32
	std::string gender;//limit �� or Ů
	std::string age;//limit 8
	std::string signature;//limit 128
};

//�����������˳���־//
bool g_exitFlag = false;

//�㲥���û��б����//
std::mutex users_mutex;
std::condition_variable users_cv;
bool users_update_signal = true;

int z = 0;//��Ϣ���һ�ν�����Ϣ��־//
//int z_one = 0;//��Ϣ���һ�ν�����Ϣ��־//
HANDLE hThread = NULL;//���߳̾��//
HWND g_hInfoDialogphoto = NULL;//����ͷ��Ի���//
//��Ϣ����//
HWND g_hInfoDialog = NULL;//�Ի�����//

HWND g_hInfoDialogmonitor = NULL;//��ؿ���//
HWND g_hInfoDialogbroadcast = NULL;//�㲥����//
HWND g_hInfoDialogbroadcast_information = NULL;//�㲥���û���Ϣ�Ի�����//

std::queue<std::wstring>g_msgQueue;//��Ϣ����//
CRITICAL_SECTION g_csMsgQueue;//�߳�ͬ����//

std::queue<std::wstring>g_msg_two_Queue;//�ͻ���+��������Ϣ����//
CRITICAL_SECTION g_csMsg_two_Queue;//�߳�ͬ����//

//ͼ�����//
std::queue<std::vector<BYTE>>g_imgQueue;
std::mutex g_imgQueueMutex;//��ȷ���̰߳�ȫ//

std::mutex g_usersMutex;//�㲥�û��б���//
//�������ݽ��սṹ��//
struct receivedData {
	std::string account;
	std::string password;
	std::string nickname;
	std::string gender;
	std::string age;
	std::string signature;
	std::vector<BYTE> imgData;
	int step = 0; // ��ǰ�յ��˵ڼ���//
};


MYSQL* conn = mysql_init(NULL);//���ݿ��ʼ��//

void HandleClient_informationset(SOCKET client_server);
void HandleClient_login(SOCKET client_server,std::string);//��¼�����߳�//
void HandleClient_passwordset(SOCKET client_server);
void HandleClient_photoset(SOCKET client_server);
void Handlelogin_pro(SOCKET client_server, receivedData my_data,std::string);

//���ݽ��պ���//
int recvAll(SOCKET socket, char* buffer, int len, int y = 0)
{
	int r = 0;//���ν����ֽ���//
	int sum = 0;//�۴ν����ֽ���//
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
	std::string nickname;//�û��ǳ�//
	std::vector<BYTE>imgData;//�û�ͷ������//
	std::string account;//�û��˺�//
};
std::vector<Userdata> g_users;//���ڼ������ݿ���������û���Ϣ���������û��б����//

std::mutex g_onlineUsersMutex;//�����û��б���//
struct User_account
{
	std::string account;//�û��˺�//
};
std::vector<User_account> g_onlineUsers;//������¼���ߵ��û�//


std::mutex g_users_online_Mutex;//�����û���//
//�洢�û��˺ź�socket��ʵ�й㲥����Ϣ�ַ�//
struct User_online
{
	std::string account;
	SOCKET socket;
};
std::vector<User_online> g_users_online;

//�����û��б����//
std::mutex users_online_mutex;
std::condition_variable users_online_cv;
bool users_online_update_signal = false;

std::vector<std::string>g_select_offline_users_account;//�洢�㲥����ѡ�е��û������ߵ��û��˺�//
std::mutex g_users_account_sel_broadcast_Mutex;//��ȷ���̰߳�ȫ//

struct offline_users_and_information//�洢�����û��˺ź͹㲥��Ϣ//
{
	std::string account;
	std::string information;
};
std::vector<offline_users_and_information>g_offline_users_and_information;
std::mutex g_users_account_and_information_sel_broadcast_Mutex;//��ȷ���̰߳�ȫ//

// �洢�����û��ǳ���Ϣ
std::vector<std::string>g_offline_users_nickname;
std::mutex g_users_nickname_sel_broadcast_Mutex;//��ȷ���̰߳�ȫ//

struct users_anii_on_chartroom
{
	std::string user_on_chartroom_account;
	std::string user_on_chartroom_nickname;
	std::vector<BYTE> user_on_chartroom_image;
	std::string user_on_chartroom_inf;
};
std::vector  <users_anii_on_chartroom> users_account_on_chartroom;//�洢�Ѿ����������ҵ��û��˺�
std::mutex users_anii_on_chartroom_mutex;//����


struct users_chart_information
{
	std::string inf_send_account;
	std::string inf_recv_account;
	std::string inf;
};
std::vector<users_chart_information>g_users_chart_information;
std::mutex g_users_chart_information_mutex;


INT_PTR CALLBACK Dialog_one(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//��ʼ�Ի���//
{
	static UINT_PTR TimeID = 0;//���ö�ʱ��//
	static HBRUSH hBackgroundBrush = NULL;//������ˢ//
	static HBITMAP hBmp = NULL;//λͼ���//
	switch (uMsg)
	{
	case WM_INITDIALOG:
		TimeID = SetTimer(hwndDlg, 1, 4000, NULL);//��ʾ��һ���Ի�������ݣ��ӳ�4�룻1Ϊ��ʱ����ID������ʱ�����󣬴��ݸ�wParam�Ĳ���Ϊ��ID����1.//
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
			DrawText(pdis->hDC, L"Welcome to QQ's server", -1, &pdis->rcItem,//�Ի������//
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
		TimeID = SetTimer(hwndDlg, 1, 0, NULL);//��ʾ��һ���Ի�������ݣ��ӳ�4��//
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
			//EndDialog(hwndDlg, IDOK);//�رնԻ���//
			exit(0);//�˳�����//
		}
		return (INT_PTR)TRUE;
	case WM_CLOSE:
		EndDialog(hwndDlg, IDEND);//�رնԻ���//
		break;
	}
	return (INT_PTR)FALSE;
}

//��Ϣ���º���//
void UpdateDialogContent(HWND hwndDlg)
{
	EnterCriticalSection(&g_csMsgQueue);//����//
	if (!g_msgQueue.empty())//���зǿ�//
	{
		std::wstring msg = g_msgQueue.front();//ȡ����Ԫ��//
		g_msgQueue.pop();//������Ԫ�ص���//
		int len = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_INFORMATION));//��ȡ��ǰ�ؼ��ı����ȣ�������'\0'//
		wchar_t* currentText = new wchar_t[len + 1];
		GetWindowText(GetDlgItem(hwndDlg, IDC_INFORMATION), currentText, len + 1);//��ȡ��ǰ�ؼ��ı�,�ڽ�β����'\0'//
		std::wstring combined = L"";
		if (len > 0)
		{
			if (z)//��һ�ν�����Ϣʱ��ȥ���Ի���ġ��ȴ���Ϣ...��//
			{
				combined = currentText;//����ʱ�������\0//
				combined += L"\r\n";
			}
			z = 1;
		}
		combined += msg;
		SetDlgItemText(hwndDlg, IDC_INFORMATION, combined.c_str());//������Ϣ//
		delete[] currentText;
	}
	LeaveCriticalSection(&g_csMsgQueue);//����//
}

//��ȡͼ����ʾ���//
HBITMAP CreateHBITMAPFromBytes(const BYTE* pData, int len, int wide, int high)
{
	HBITMAP hbmp = NULL;//��ʼ��λͼ���//
	IStream* pStream = NULL;
	//�����ڴ���//
	if (CreateStreamOnHGlobal(NULL, TRUE, &pStream) == S_OK)
	{
		ULONG written = 0;//ʵ��д�������ֽ���//
		pStream->Write(pData, len, &written);
		//����ָ�뵽����ͷ//
		LARGE_INTEGER li{};
		pStream->Seek(li, STREAM_SEEK_SET, NULL);
		using namespace Gdiplus;
		Bitmap* bmp = Bitmap::FromStream(pStream);
		if (bmp && bmp->GetLastStatus() == Ok)
		{
			//���ŵ��ؼ���С//
			Bitmap* pThumb = new Bitmap(wide, high, bmp->GetPixelFormat());
			Graphics g(pThumb);
			g.DrawImage(bmp, 0, 0, wide, high);//����ԭͼ�����ػ�//
			pThumb->GetHBITMAP(Color(200, 230, 255), &hbmp);
			delete pThumb;
		}
		delete bmp;
		pStream->Release();
	}
	return hbmp;
}


//ͷ����¿�//
void UpdateDialogphoto(HWND hwndDlg)
{
	std::vector<BYTE>imgData;
	std::lock_guard<std::mutex> lock(g_imgQueueMutex);//�̰߳�ȫ���Զ������ͽ���//
	if (!g_imgQueue.empty())
	{
		imgData = std::move(g_imgQueue.front());//ȡ������Ԫ��//
		g_imgQueue.pop();//��������Ԫ��//
	}
	HWND hwnd = GetDlgItem(hwndDlg, IDC_PHOTO);//��ȡ�ؼ��ߴ�//
	RECT rc;
	GetClientRect(hwnd, &rc);//��ʼ��rc�ṹ��//
	int wide = rc.right - rc.left;
	int high = rc.bottom - rc.top;
	//��ȡͼ����ʾ���//
	HBITMAP hbmp = CreateHBITMAPFromBytes(imgData.data(), imgData.size(), wide, high);
	if (hbmp)
	{
		SendMessage(hwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmp);
		DeleteObject(hbmp);//�ͷž����Դ//
	}
	else
	{
		MessageBox(NULL, L"�޷���ʾͼ����", L"QQ", MB_ICONERROR);
		DeleteObject(hbmp);//�ͷž����Դ//
		exit(1);
	}
}


INT_PTR CALLBACK Dialog_information(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//��Ϣ�Ի���//
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, IDC_INFORMATION, L"�ȴ���Ϣ...");//������Ϣ//
		return (INT_PTR)TRUE;
	case WM_APP_UPDATE_MSG:
		UpdateDialogContent(hwndDlg);
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL)
		{
			DestroyWindow(hwndDlg);
			g_hInfoDialog = NULL;
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MYDIALOG_END), NULL, Dialog_end);//������//
			return (INT_PTR)TRUE;
		}
	}
	return (INT_PTR)FALSE;
}


INT_PTR CALLBACK Dialog_photo(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//ͷ����ʾ��//
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, IDC_PHOTO, L"�ȴ��û��ϴ�ͷ��...");//����ͷ��//
		return (INT_PTR)TRUE;
	case WM_APP_UPDATEPHOTO_MSG:
		UpdateDialogphoto(hwndDlg);
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL)
		{
			DestroyWindow(hwndDlg);
			g_hInfoDialogphoto = NULL;
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MYDIALOG_END), NULL, Dialog_end);//������//
			return (INT_PTR)TRUE;
		}
	}
	return (INT_PTR)FALSE;
}

void UpdateDialogmonitor(HWND hwndDlg)
{
	static int z_one = 0;//��Ϣ���һ�ν�����Ϣ��־//
	EnterCriticalSection(&g_csMsg_two_Queue);//����//
	if (!g_msg_two_Queue.empty())
	{
		std::wstring msg_one = g_msg_two_Queue.front();//ȡ����Ԫ��//
		g_msg_two_Queue.pop();//������Ԫ�ص���//
		HWND hwnd = GetDlgItem(hwndDlg, IDC_MONITOR);
		if (!hwnd)
		{
			MessageBox(NULL, L"��ȡ�������ʧЧ", L"QQ", MB_ICONERROR);
			LeaveCriticalSection(&g_csMsg_two_Queue);//����//
			return;
		}
		int len = GetWindowTextLength(hwnd);//��ȡ��ǰ�ؼ��ı����ȣ�������'\0'//
		wchar_t* currentText = new wchar_t[len + 1];
		GetWindowText(hwnd, currentText, len + 1);//��ȡ��ǰ�ؼ��ı�,�ڽ�β����'\0'//
		std::wstring combined = L"";
		if (len > 0)
		{
			if (z_one)//��һ�ν�����Ϣʱ��ȥ���Ի���ġ��ȴ���Ϣ...��//
			{
				combined = currentText;//����ʱ�������\0//
				combined += L"\r\n";
			}
			z_one = 1;
		}
		combined += msg_one;
		SetDlgItemText(hwndDlg, IDC_MONITOR, combined.c_str());//������Ϣ//
		delete[] currentText;
	}

	LeaveCriticalSection(&g_csMsg_two_Queue);//����//
}


INT_PTR CALLBACK Dialog_monitor(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//��ؿ�//
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, IDC_MONITOR, L"δ��ص���Ϣ����...");//��ؿ��ʼ��//
		return (INT_PTR)TRUE;
	case WM_APP_UPDATEMONITOR_MSG:
		UpdateDialogmonitor(hwndDlg);
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL)
		{
			DestroyWindow(hwndDlg);
			g_hInfoDialogmonitor= NULL;
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MYDIALOG_END), NULL, Dialog_end);//������//
			return (INT_PTR)TRUE;
		}
	}
	return (INT_PTR)FALSE;
}


INT_PTR CALLBACK Dialog_broadcast_information(HWND hndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//�㲥��Ϣ�Ի���//
{
	switch (uMsg)
{
	case WM_INITDIALOG:
	{
		HWND hEdit = GetDlgItem(hndDlg, IDC_BROADCAST_INFORAMATION_EDIT);//��ȡ��Ϣ����
		HWND hBtn = GetDlgItem(hndDlg, IDOK);//��ȡ���Ͱ�ť���
		int len = GetWindowTextLength(hEdit);//��ȡ��Ϣ���ı�����
		EnableWindow(hBtn, len > 0 ? TRUE : FALSE);
		return (INT_PTR)TRUE;
	}
	case WM_COMMAND:
	{
		// ���༭�����ݱ仯
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
			
			//��ȡ�ؼ�IDC_BROADCAST_INFORAMATION_EDIT�ľ��
			HWND hEdit = GetDlgItem(hndDlg, IDC_BROADCAST_INFORAMATION_EDIT);
			//��ȡ�ÿؼ��ı���Ϣ
			int len = GetWindowTextLength(hEdit);//��ȡ�ı�����,����'\0'//
			if (len <= 0)
			{
				MessageBox(NULL,L"������Ϣ������Ϊ��",L"QQ",MB_ICONERROR);
				return (INT_PTR)TRUE;
			}
			std::wstring text(len+1, L'\0');
			GetWindowText(hEdit, &text[0], len + 1);//���ı��Զ����L'\0'
			//תΪUTF-8
			int utf8len = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, NULL, 0, NULL, NULL);//����'\0'����
			std::string msg(utf8len, '\0');
			WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, &msg[0], utf8len, NULL, NULL);//�洢Ҫ���͵Ĺ㲥��Ϣ
			//����ı���
			SetDlgItemText(hndDlg, IDC_BROADCAST_INFORAMATION_EDIT, L"");
			
			HWND hBtn = GetDlgItem(hndDlg, IDOK);
			EnableWindow(hBtn,FALSE);//�ֶ����÷��Ͱ�ť


			//����Ϣ��ʾ		
			for (const auto &acc_nickname:g_offline_users_nickname)
			{
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

				//��ȡԭ�����ı�
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

			//���������û�����Ϣ����//
			for (const auto& offline_account : g_select_offline_users_account)
			{
				offline_users_and_information info;
				info.account = offline_account;//�洢�����˺�
				info.information = msg;//�洢��Ϣstring��ʽ,��'\0'
				std::lock_guard<std::mutex> lock_broadcast_account(g_users_account_and_information_sel_broadcast_Mutex);//��ȷ���̰߳�ȫ//
				g_offline_users_and_information.push_back(info);
			}
			if (g_offline_users_and_information.empty())
			{
				MessageBox(NULL, L"δ�ܽ���Ϣ�Ͷ�Ӧ���û��˺�һ��洢", L"QQ", MB_ICONERROR);
			}
			return (INT_PTR)TRUE;
		}

	  }
	}
  }
	return (INT_PTR)FALSE;
}


INT_PTR CALLBACK Dialog_broadcast(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)//�㲥��//
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		//�㲥���ʼ��//
		HWND hList = GetDlgItem(hwndDlg, IDC_USERS);//��ȡ�û��б���//
		SendMessage(hList, LB_RESETCONTENT, 0, 0);//����ı���//

		// ��ӡ��ȴ���ʼ����ռλ��//
		int idx = SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)L"���ڳ�ʼ��");//���ظ������ݵ�����//
		SendMessage(hList, LB_SETITEMDATA, idx, (LPARAM)-1); // ��-1����//

		SendMessage(hList, LB_SETSEL, FALSE, -1);//ȡ������ѡ����//
		LONG style = GetWindowLong(hList, GWL_STYLE);//��ȡ��ǰ�ؼ�����ʽ//
		style &= ~(LBS_SINGLESEL);//���LBS_OWNERDRAWFIXED��ʽ//
		style |= LBS_MULTIPLESEL;  // ���Ӷ�ѡ��֧��Ctrl/Shift
		SetWindowLong(hList, GWL_STYLE, style); // Ӧ������ʽ
		HWND hBtn = GetDlgItem(hwndDlg, IDOK); //����ť���//
		EnableWindow(hBtn, FALSE); // ��ʼ���ù㲥��ť
		return (INT_PTR)TRUE;
	}

	case  WM_APP_UPDATEBROADCAST_MSG:          //�����û��б���Ϣ//
	{
		HWND hList = GetDlgItem(hwndDlg, IDC_USERS);//��ȡ�û��б���
		SendMessage(hList, LB_RESETCONTENT, 0, 0);//����ı���//
		std::lock_guard<std::mutex> lock(g_usersMutex);
		for (int i = 0; i < g_users.size(); ++i)
		{
			//SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)L"");
			//SendMessage(hList, LB_SETITEMDATA,i, (LPARAM)i);
			 // չʾ�˺�+�ǳ�
			std::wstring wdisplay;
			{
				// �˺�
				int wlen_acc = MultiByteToWideChar(CP_UTF8, 0, g_users[i].account.c_str(), (int)g_users[i].account.size(), NULL, 0);
				std::wstring wacc(wlen_acc, 0);
				MultiByteToWideChar(CP_UTF8, 0, g_users[i].account.c_str(), (int)g_users[i].account.size(), &wacc[0], wlen_acc);

				// �ǳ�
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
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MYDIALOG_END), NULL, Dialog_end);//������//
			return (INT_PTR)TRUE;
		}
		case IDOK:
		{
			// ����ϴε�ѡ��
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
					std::lock_guard<std::mutex> lock_broadcast_account(g_users_account_sel_broadcast_Mutex);//��ȷ���̰߳�ȫ//
					g_select_offline_users_account.push_back(account);//��ȫ��ѡ�е��û��˺Ŵ洢

					// �洢ѡ���û��ǳ���Ϣ
					std::lock_guard<std::mutex>lock_broadcast_nickname(g_users_nickname_sel_broadcast_Mutex);
					g_offline_users_nickname.push_back(g_users[userIdx].nickname);

				}
			}
			if (g_select_offline_users_account.empty())
			{
				MessageBox(NULL,L"δ�ܽ�ѡ�еĹ㲥�����˺Ŵ洢",L"QQ",MB_ICONERROR);
				return (INT_PTR)TRUE;
			}
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MYDIALOG_BROADCAST_INFORMATION), NULL, Dialog_broadcast_information);//�㲥��Ϣ�Ի���//
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
			mis->itemHeight = 72;//����IDC_USERS����߶�Ϊ72//
		}
		return (INT_PTR)TRUE;
	}
	case WM_DRAWITEM:
	{
		LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;//ָ��һ��DRAWITEMSTRUCT�ṹ���ָ��//
		if (dis->CtlID == IDC_USERS)
		{
			size_t idx = dis->itemData;

			std::lock_guard<std::mutex> lock(g_usersMutex);
			if (idx == (size_t)-1 || idx >= g_users.size())
			{
				return (INT_PTR)FALSE;
			}

			const Userdata& user = g_users[idx];
			HDC hdc = dis->hDC;//��ͼ�豸����//
			RECT rc = dis->rcItem;//����Ƶ���ľ�������//

			//������//
			SetBkMode(hdc,OPAQUE);//�����豸�����ı������ģʽΪOPAQUE//
			//SetBkColor(hdc, (dis->itemState & ODS_SELECTED) ? RGB(230, 230, 250) : RGB(255, 255, 255));
			//ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);//���rc�ı���ɫ//
			SetBkColor(hdc, (dis->itemState & ODS_SELECTED) ? RGB(230, 230, 250) : RGB(255, 255, 255));
			ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);//���rc�ı���ɫ//
			//��ͷ��
			int imgsize = 64;//ͷ���С//
			if (!user.imgData.empty())//ͷ�����ݷǿ�//
			{
				HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE,user.imgData.size());//����һ����ƶ��ڴ棬�������ڴ���//
				if (!hMem)
				{
					MessageBox(NULL,L"���ƶ��ڴ����ʧ��",L"QQ",MB_ICONERROR);
					return (INT_PTR)FALSE;
				}
				void* pMem = GlobalLock(hMem);//����//
				memcpy(pMem, user.imgData.data(), user.imgData.size());//��ͷ��ͼƬ���ֽ����ݸ��Ƶ��ղŷ�����ڴ����//
				GlobalUnlock(hMem);//����//
				IStream* pStream = NULL;
				if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) == S_OK)//��ȫ���ڴ��װΪIStream�ڴ����,pStreamָ���������//
				{
					Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromStream(pStream);//����Bitmap����//
					if (bmp && bmp->GetLastStatus() == Gdiplus::Ok)//����ɹ���������//
					{
						Gdiplus::Graphics g(hdc);//����Graphics ����g,���� hdc�豸���л���//
						g.DrawImage(bmp,rc.left+2,rc.top+2,imgsize,imgsize);

					}
					delete bmp;
					pStream->Release();
				}
				GlobalFree(hMem);//�ͷ��ڴ�//
			}

			//��ѯ�����û��и��û����˺��Ƿ����//
			bool isOnline = false;
			{
				std::lock_guard<std::mutex> lock(g_onlineUsersMutex);
				for (const auto& user_account_online : g_onlineUsers)
				{
					std::string x = user_account_online.account;//g_user������\0����β//
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

			// ������ɫ
			COLORREF textColor = isOnline ? RGB(0, 128, 0) : RGB(255, 0, 0);

			//���˺�//
			RECT rcAccount = rc;
			rcAccount.left += imgsize + 8;
			rcAccount.top += 12;
			int wlen = MultiByteToWideChar(CP_UTF8, 0, user.account.c_str(), user.account.size() + 1, NULL, 0);
			std::wstring wnick(wlen, 0);
			MultiByteToWideChar(CP_UTF8, 0, user.account.c_str(), user.account.size() + 1, &wnick[0], wlen);//���ı�ת��Ϊ���ַ���//
			//DrawTextW(hdc,wnick.c_str(),wlen,&rcText,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
			SetTextColor(hdc, textColor); // �����ı���ɫ
			DrawTextW(hdc, wnick.c_str(), wlen, &rcAccount, DT_LEFT | DT_TOP | DT_SINGLELINE);


			// �����˺Ÿ߶�
			DrawTextW(hdc, wnick.c_str(), wlen, &rcAccount, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_CALCRECT);

			//���ǳ�//
			RECT rcText = rcAccount;
			rcText.top = rcAccount.bottom + 8;
			rcText.bottom = rcText.top + (rcAccount.bottom - rcAccount.top); //����rcText.bottom//
			rcText.right +=1000;
			int wlen2 = MultiByteToWideChar(CP_UTF8, 0, user.nickname.c_str(), user.nickname.size() + 1, NULL, 0);
			std::wstring wnick_2(wlen2, 0);
			MultiByteToWideChar(CP_UTF8, 0, user.nickname.c_str(), user.nickname.size() + 1, &wnick_2[0], wlen2);//���ı�ת��Ϊ���ַ���//
			SetTextColor(hdc, textColor);
			DrawTextW(hdc, wnick_2.c_str(), wlen2, &rcText, DT_LEFT | DT_TOP | DT_SINGLELINE);

			//����¼״̬//
			int big_num = (wlen2 > wlen) ? wlen2 : wlen;
			std::wstring big_str = (wnick.size() >wnick_2.size()) ? wnick:wnick_2;
			SIZE sz;
			GetTextExtentPoint32W(hdc, big_str.c_str(), big_num, &sz);//��ȡ�ı��Ŀ�Ⱥ͸߶�//
			RECT rcRegister= rcAccount;
			rcRegister.left +=sz.cx;//�����ı������߿�//
			rcRegister.right += sz.cx;//�����ı�����ұ߿�//
			SetTextColor(hdc, textColor);
			DrawTextW(hdc, isOnline ? L"����" : L"����", -1, & rcRegister, DT_LEFT | DT_VCENTER| DT_SINGLELINE);

			//�������//
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
	// ���ò���(BLOB)
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




void update_new_users_list()//���¹㲥���û��б�
{
	    MYSQL* conn_5 = mysql_init(NULL);
		int con_num = 0;
		while (!mysql_real_connect(conn_5, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))//�������ݿ�//
		{
			MessageBox(NULL, L"�������ݿ�ʧ��", L"QQ", MB_ICONERROR);
			con_num++;
			if (con_num > 3)
			{
				return ;
			}
		}

		std::vector<Userdata>temp_users;
		temp_users.clear();
		const char* sql = "SELECT nickname,imgData,account FROM users";//SQL�������//
		if (mysql_query(conn_5, sql))//���û�з��ϵĲ�ѯ��������ط���ֵ//
		{
			//MessageBox(NULL, L"û�з��ϵ��ǳƺ�ͷ��", L"QQ", MB_ICONERROR);
			return;
		}

		MYSQL_RES* res = mysql_store_result(conn_5);//һ�������//
		if (!res)
		{
			return;
		}
		MYSQL_ROW  row;//ָ���ַ��������ָ��//
		unsigned long* lengths;
		while ((row = mysql_fetch_row(res)))//ȡĳһ�в�ѯ�����//
		{
			lengths = mysql_fetch_lengths(res);//ȡĳһ�в�ѯ������ȼ�//
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
			temp_users.push_back(std::move(user));//�����õ��û������ƶ���ȫ���û��б����ֹ����Ҫ�Ŀ���//
		}
		mysql_free_result(res);
		std::lock_guard<std::mutex>lock(g_usersMutex);//����//
		g_users.swap(temp_users);

		if (g_hInfoDialogbroadcast && IsWindow(g_hInfoDialogbroadcast))//�ȴ������Ч//
			PostMessage(g_hInfoDialogbroadcast, WM_APP_UPDATEBROADCAST_MSG, 0, 0); // �Զ�����Ϣ
}

void HandleClient_register(SOCKET client_server)//ע�ᴦ���߳�//
{
	std::string new_online_user_account;
	//��һ���ж�//
	int len_one = 0;//���տͻ��˵õ��˺ź��ѡ�񣬼��� �� ȡ��// 
	recvAll(client_server, (char*)&len_one, sizeof(len_one), 0);//�Ƚ����ֽڳ���//
	if ((int)len_one <= 0)
	{
		MessageBox(NULL, L"����'�ͻ��˵õ��˺ź��ѡ�񣬼��� �� ȡ��'���ȴ���", L"QQ", MB_ICONERROR);//��ʶ���Ƚ��մ�����ʾ//
		return;
	}
	std::string received_one(len_one, 0);//����ռ�// 
	recvAll(client_server, &received_one[0], len_one, 0);//���ܱ�ʶ����//
	int wlen_one = MultiByteToWideChar(CP_UTF8, 0, &received_one[0], len_one, NULL, 0);//����ת������//
	std::wstring wreceived_one(wlen_one, 0);
	MultiByteToWideChar(CP_UTF8, 0, &received_one[0], len_one, &wreceived_one[0], wlen_one);//ת��Ϊ���ַ�����//
	if (wcscmp(wreceived_one.c_str(), L"ȡ��") == 0)
	{
		//�жϾ���ִ�������߳�,��¼��ע��//
		int recv_len = 0;
		recv(client_server, (char*)&recv_len, sizeof(recv_len), 0);//������Ϣ����//
		if (recv_len <= 0)
		{
			//MessageBox(NULL, L"�����߳����͵���Ϣ����ʧ��", L"QQ", MB_ICONERROR);
			return;
		}
		//MessageBox(NULL, L"�����߳����͵���Ϣ���ȳɹ�", L"QQ", MB_ICONINFORMATION);
		std::string recvchar(recv_len, 0);
		int r = 0;
		r = recv(client_server, &recvchar[0], recv_len, 0);
		{
			if (r <= 0)
			{
				//MessageBox(NULL, L"�����߳����͵���Ϣʧ��", L"QQ", MB_ICONERROR);
				return;
			}
		}
		//MessageBox(NULL, L"�����߳����͵���Ϣ�ɹ�", L"QQ", MB_ICONINFORMATION);
		int wlen = MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, NULL, 0);//ת��Ϊ���ֽ�����Ҫ���ֽ���//
		std::wstring wrecvchar(wlen, 0);//����ռ�//
		MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, &wrecvchar[0], wlen);//ʵ��ת��//
		//ע�ᴦ��//
		if (wcscmp(wrecvchar.c_str(), L"ע��") == 0)//ע���߳�//
		{
			//MessageBox(NULL, L"���յ�ע����Ϣ����������ע���߳�", L"QQ", MB_ICONINFORMATION);
			std::thread(HandleClient_register, client_server).detach();//����ע���߳�//
		}
		//��¼����//
		else if (wcscmp(wrecvchar.c_str(), L"��¼") == 0)//��¼�߳�//
		{
			//MessageBox(NULL, L"���յ���¼��Ϣ���������õ�¼�߳�", L"QQ", MB_ICONINFORMATION);
		
			std::thread(HandleClient_login, client_server,new_online_user_account).detach();//������¼�߳�//
		}
		else
		{
			//MessageBox(NULL, L"���ղ����߳���Ϣ�������˳�", L"QQ", MB_ICONINFORMATION);
			return;
		}
	}
	//sockaddr_in clientAddr;
	//int addrlen = sizeof(clientAddr);
	//��ȡ�ͻ��˵�ַ//
	//getpeername(client_server, (sockaddr*)&clientAddr, &addrlen);
	//char ipStr[INET_ADDRSTRLEN];
	//inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
	//ת��ΪUnicode�ַ�����WCHAR��
	//wchar_t wIP[20];
	//MultiByteToWideChar(CP_UTF8,0,ipStr,-1,wIP,sizeof(wIP)/sizeof(wchar_t));
	else if (wcscmp(wreceived_one.c_str(), L"����") == 0)
	{
		//�ڶ����ж�//
		//�ͻ�����������ѡ���ж� ��ɻ�ȡ�� //
		int len_two = 0;//���տͻ��˵õ��˺ź��ѡ�񣬼��� �� ȡ��//
		recvAll(client_server, (char*)&len_two, sizeof(len_two), 0);//�Ƚ����ֽڳ���//
		if ((int)len_two <= 0)
		{
			MessageBox(NULL, L"����'�ͻ�����������ѡ���ж� ��ɻ�ȡ��'���ȴ���", L"QQ", MB_ICONERROR);//��ʶ���Ƚ��մ�����ʾ//
			return;
		}
		std::string received_two(len_two, 0);//����ռ�//
		recvAll(client_server, &received_two[0], len_two, 0);//���ܱ�ʶ����//
		int wlen_two = MultiByteToWideChar(CP_UTF8, 0, &received_two[0], len_two, NULL, 0);//����ת������//
		std::wstring wreceived_two(wlen_two, 0);
		MultiByteToWideChar(CP_UTF8, 0, &received_two[0], len_two, &wreceived_two[0], wlen_two);//ת��Ϊ���ַ�����//
		if (wcscmp(wreceived_two.c_str(), L"ȡ��") == 0)
		{
			std::thread(HandleClient_register, client_server).detach();//����ͻ��˷��ص�ע���˺����ɴ�//
		}
		else if (wcscmp(wreceived_two.c_str(), L"���") == 0)
		{
			//�������ж�//
			//�ͻ��˸�����Ϣ����ѡ���ж� ȷ����ȡ�� //
			int len_three = 0;//���տͻ��˵õ��˺ź��ѡ�񣬼��� �� ȡ��//
			recvAll(client_server, (char*)&len_three, sizeof(len_three), 0);//�Ƚ����ֽڳ���//
			if (len_three <= 0)
			{
				MessageBox(NULL, L"����'�ͻ�����������ѡ���ж� ��ɻ�ȡ��'���ȴ���", L"QQ", MB_ICONERROR);//��ʶ���Ƚ��մ�����ʾ//
				return;
			}
			std::string received_three(len_three, 0);//����ռ�//
			recvAll(client_server, &received_three[0], len_three, 0);//���ܱ�ʶ����//
			int wlen_three = MultiByteToWideChar(CP_UTF8, 0, &received_three[0], len_three, NULL, 0);//����ת������//
			std::wstring wreceived_three(wlen_three, 0);
			MultiByteToWideChar(CP_UTF8, 0, &received_three[0], len_three, &wreceived_three[0], wlen_three);//ת��Ϊ���ַ�����//
			if (wcscmp(wreceived_three.c_str(), L"ȡ��") == 0)
			{
				MessageBox(NULL, L"�����������ɸ�����Ϣ���̷������������߳�", L"QQ", MB_ICONINFORMATION);
				std::thread(HandleClient_passwordset, client_server).detach();//���ص����������߳�//
			}
			else if (wcscmp(wreceived_three.c_str(), L"ȷ��") == 0)
			{
				//���ĸ��ж�//
			   //�ͻ���ͷ������ѡ���ж� ȷ����ȡ�� //
				MessageBox(NULL, L"�����������ɸ�����Ϣ���̽���ͷ�������߳�", L"QQ", MB_ICONINFORMATION);
				int len_four = 0;//���տͻ���ͷ�����ú��ѡ�񣬼��� �� ȡ��//
				recvAll(client_server, (char*)&len_four, sizeof(len_four), 0);//�Ƚ����ֽڳ���//
				if ((int)len_four <= 0)
				{
					MessageBox(NULL, L"����'�ͻ���ͷ������ѡ���ж� ȷ�ϻ�ȡ��'���ȴ���", L"QQ", MB_ICONERROR);//��ʶ���Ƚ��մ�����ʾ//
					return;
				}
				std::string received_four(len_four, 0);//����ռ�//
				recvAll(client_server, &received_four[0], len_four, 0);//���ܱ�ʶ����//
				int wlen_four = MultiByteToWideChar(CP_UTF8, 0, &received_four[0], len_four, NULL, 0);//����ת������//
				std::wstring wreceived_four(wlen_four, 0);
				MultiByteToWideChar(CP_UTF8, 0, &received_four[0], len_four, &wreceived_four[0], wlen_four);//ת��Ϊ���ַ�����//
				if (wcscmp(wreceived_four.c_str(), L"ȡ��") == 0)
				{
					std::thread(HandleClient_informationset, client_server).detach();//���ص�������Ϣ���ô�//
				}
				else if (wcscmp(wreceived_four.c_str(), L"ȷ��") == 0)
				{
					//������ж�//
					//�ͻ���ע��ɹ�ѡ���ж� ȷ����ȡ�� //
					int len_five = 0;//���տͻ��˵õ��˺ź��ѡ�񣬼��� �� ȡ��//
					recvAll(client_server, (char*)&len_five, sizeof(len_five), 0);//�Ƚ����ֽڳ���//
					if ((int)len_five <= 0)
					{
						MessageBox(NULL, L"����'�ͻ���ע��ɹ�ѡ���ж� ȷ�ϻ�ȡ��'���ȴ���", L"QQ", MB_ICONERROR);//��ʶ���Ƚ��մ�����ʾ//
						return;
					}
					std::string received_five(len_five, 0);//����ռ�//
					recvAll(client_server, &received_five[0], len_five, 0);//���ܱ�ʶ����//
					int wlen_five = MultiByteToWideChar(CP_UTF8, 0, &received_five[0], len_five, NULL, 0);//����ת������//
					std::wstring wreceived_five(wlen_five, 0);
					MultiByteToWideChar(CP_UTF8, 0, &received_five[0], len_five, &wreceived_five[0], wlen_five);//ת��Ϊ���ַ�����//
					if (wcscmp(wreceived_five.c_str(), L"ȡ��") == 0)
					{
						std::thread(HandleClient_photoset, client_server).detach();//����ͷ�������߳�//
					}
					else if (wcscmp(wreceived_five.c_str(), L"ȷ��") == 0)
					{
						//std::lock_guard<std::mutex>lk(users_mutex);
						//users_update_signal = true;
						//users_cv.notify_one();//֪ͨ�������¼������ݿ�����û��ǳƺ�ͷ��//
						
						int x = 0;
						receivedData regData;//�ṹ�壬������������//
						while (true)
						{
							int sign_strlen = 0;//���ձ�ʶ����//
							recvAll(client_server, (char*)&sign_strlen, sizeof(sign_strlen), 0);//�Ƚ��ձ�ʶ�ֽڳ���//
							if ((int)sign_strlen <= 0)
							{
								MessageBox(NULL, L"���ձ�ʶ���ȴ���", L"QQ", MB_ICONERROR);//��ʶ���Ƚ��մ�����ʾ//
								return;
							}
							std::string sign_received(sign_strlen, 0);//����ռ�//
							recvAll(client_server, &sign_received[0], sign_strlen, 0);//���ܱ�ʶ����//
							int sign_wcharlen = MultiByteToWideChar(CP_UTF8, 0, &sign_received[0], sign_strlen, NULL, 0);//����ת������//
							std::wstring sign_wreceived(sign_wcharlen, 0);
							MultiByteToWideChar(CP_UTF8, 0, &sign_received[0], sign_strlen, &sign_wreceived[0], sign_wcharlen);//ת��Ϊ���ַ�����//

							int con_strlen = 0;//�������ݳ���//
							recvAll(client_server, (char*)&con_strlen, sizeof(con_strlen), 0);//�Ƚ��������ֽڳ���//
							if ((int)con_strlen <= 0)
							{
								MessageBox(NULL, L"�������ݳ��ȴ���", L"QQ", MB_ICONERROR);//���ݳ��Ƚ��մ�����ʾ//
								return;
							}

							if (wcscmp(sign_wreceived.c_str(), L"ͷ��") != 0)//��ʶ����ͷ�񣬶����˺š����롢�ǳơ��Ա�����͸���ǩ����ʱ���ʵ��//
							{
								std::string received(con_strlen, 0);//����ռ�//
								int rec = recvAll(client_server, &received[0], con_strlen, 0);//��������//
								int wcharlen = MultiByteToWideChar(CP_UTF8, 0, &received[0], con_strlen, NULL, 0);//����ת������//
								std::wstring wreceived(wcharlen, 0);
								MultiByteToWideChar(CP_UTF8, 0, &received[0], con_strlen, &wreceived[0], wcharlen);//ת��Ϊ���ַ�����//
								if (!sign_wreceived.empty() && sign_wreceived.back() == L'\0')//ȥ����ʶ�ַ�������ֹ��'\0',�ſ���ƴ�Ӳ���ʾ���������//
								{
									sign_wreceived.pop_back();
									//�����ݴ洢���ṹ�壬�Ա�洢�����ݿ�//
									if (wcscmp(sign_wreceived.c_str(), L"�˺�") == 0)
									{
										regData.account = received;
										regData.step++;
									}
									else if (wcscmp(sign_wreceived.c_str(), L"����") == 0)
									{
										regData.password = received;
										regData.step++;
									}
									else if (wcscmp(sign_wreceived.c_str(), L"�ǳ�") == 0)
									{
										regData.nickname = received;
										regData.step++;
									}
									else if (wcscmp(sign_wreceived.c_str(), L"�Ա�") == 0)
									{
										regData.gender = received;
										regData.step++;
									}
									else if (wcscmp(sign_wreceived.c_str(), L"����") == 0)
									{
										regData.age = received;
										regData.step++;
									}
									else if (wcscmp(sign_wreceived.c_str(), L"����ǩ��") == 0)
									{
										regData.signature = received;
										regData.step++;
									}
								}
								//��ȡ��ǰʱ��//
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
								EnterCriticalSection(&g_csMsgQueue);//����//
								g_msgQueue.push(fullMessage);//��������//
								LeaveCriticalSection(&g_csMsgQueue);//����//
								if (x == 0)
								{
									MessageBox(NULL, L"�������ѳɹ��������Կͻ��˵���Ϣ", L"QQ", MB_ICONINFORMATION);//ֻ��ʾһ��//
									x = 1;
								}
								//֪ͨ���жԻ����������//
								PostMessage(g_hInfoDialog, WM_APP_UPDATE_MSG, 0, 0);
							}
							else if (wcscmp(sign_wreceived.c_str(), L"ͷ��") == 0)
							{
								BYTE* received = new BYTE[con_strlen];//����ռ�//
								int r = 0;//���ν��յ��ֽ���//
								int sum = 0;//�ۼƽ��ܵ��ֽ���//
								while (sum < con_strlen)
								{
									r = recv(client_server, (char*)received + sum, con_strlen - sum, 0);//����ͼ�����������//
									if (r <= 0)
									{
										break;//���ս���//
									}
									sum += r;
								}
								//�����ݴ洢���ṹ�壬�Ա�洢�����ݿ�//
								regData.imgData.assign(received, received + sum);
								regData.step++;
								if (sum == con_strlen)
								{
									std::vector<BYTE>imgData(received, received + sum);//ͼƬ���ݴ洢��imgData����//
									std::lock_guard<std::mutex>lock(g_imgQueueMutex);//�̰߳�ȫ//
									g_imgQueue.push(std::move(imgData));//ͼƬ�������//
									delete[]received;
								}
								//֪ͨ����ͼƬ���������//
								PostMessage(g_hInfoDialogphoto, WM_APP_UPDATEPHOTO_MSG, 0, 0);
							}
							if (regData.step == 7)
							{
								SaveToDatabase(regData); // ���ṹ�����ݱ��浽���ݿ�//
								regData = receivedData(); // ��գ�׼����һ���û�//
								MessageBox(NULL, L"�������ѳɹ������յ�ע�����ݴ洢�����ݿ�", L"QQ", MB_ICONINFORMATION);
								break;
							}
						}
						update_new_users_list();//֪ͨ�������¼������ݿ�����û��ǳƺ�ͷ��//
						//����ע���߳�//
					   //�жϾ���ִ�������߳�,��¼��ע��//
						int recv_len = 0;
						recvAll(client_server, (char*)&recv_len, sizeof(recv_len), 0);//������Ϣ����//
						if (recv_len <= 0)
						{
							//MessageBox(NULL, L"�����߳����͵���Ϣ����ʧ��", L"QQ", MB_ICONERROR);
							return;
						}
						//MessageBox(NULL, L"�����߳����͵���Ϣ���ȳɹ�", L"QQ", MB_ICONINFORMATION);
						std::string recvchar(recv_len, 0);
						int r = 0;
						r = recvAll(client_server, &recvchar[0], recv_len, 0);

						if (r <= 0)
						{
							//MessageBox(NULL, L"�����߳����͵���Ϣʧ��", L"QQ", MB_ICONERROR);
							return;
						}

						//MessageBox(NULL, L"�����߳����͵���Ϣ�ɹ�", L"QQ", MB_ICONINFORMATION);
						int wlen = MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, NULL, 0);//ת��Ϊ���ֽ�����Ҫ���ֽ���//
						std::wstring wrecvchar(wlen, 0);//����ռ�//
						MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, &wrecvchar[0], wlen);//ʵ��ת��//
						//ע�ᴦ��//
						if (wcscmp(wrecvchar.c_str(), L"ע��") == 0)//ע���߳�//
						{
							//MessageBox(NULL, L"���յ�ע����Ϣ����������ע���߳�", L"QQ", MB_ICONINFORMATION);
							std::thread(HandleClient_register, client_server).detach();//����ע���߳�//
						}
						//��¼����//
						else if (wcscmp(wrecvchar.c_str(), L"��¼") == 0)//��¼�߳�//
						{
							//MessageBox(NULL, L"���յ���¼��Ϣ���������õ�¼�߳�", L"QQ", MB_ICONINFORMATION);
							std::thread(HandleClient_login, client_server,new_online_user_account).detach();//������¼�߳�//
						}
						else
						{
							//MessageBox(NULL, L"���ղ����߳���Ϣ�������˳�", L"QQ", MB_ICONINFORMATION);
							return;
						}
						//MessageBox(NULL, L"���������޷��������Կͻ��˵���Ϣ", L"QQ", MB_ICONERROR);
						//closesocket(client_server);
					}
				}
			}
		}
	}
}


//�ͻ�����������ѡ���ж��߳�//
void HandleClient_passwordset(SOCKET client_server)
{
	std::string new_online_user_account;
	//�ڶ����ж�//
	//�ͻ�����������ѡ���ж� ��ɻ�ȡ�� //
	int len_two = 0;//���տͻ����������ú��ѡ����� �� ȡ��//
	MessageBox(NULL, L"������������ʼ���������������ж� ��ɻ�ȡ��", L"QQ", MB_ICONINFORMATION);
	recvAll(client_server, (char*)&len_two, sizeof(len_two), 0);//�Ƚ����ֽڳ���//
	if ((int)len_two <= 0)
	{
		MessageBox(NULL, L"����'�ͻ�����������ѡ���ж� ��ɻ�ȡ��'���ȴ���", L"QQ", MB_ICONERROR);//��ʶ���Ƚ��մ�����ʾ//
		return;
	}
	std::string received_two(len_two, 0);//����ռ�//
	recvAll(client_server, &received_two[0], len_two, 0);//���ܱ�ʶ����//
	int wlen_two = MultiByteToWideChar(CP_UTF8, 0, &received_two[0], len_two, NULL, 0);//����ת������//
	std::wstring wreceived_two(wlen_two, 0);
	MultiByteToWideChar(CP_UTF8, 0, &received_two[0], len_two, &wreceived_two[0], wlen_two);//ת��Ϊ���ַ�����//
	if (wcscmp(wreceived_two.c_str(), L"ȡ��") == 0)
	{
		std::thread(HandleClient_register, client_server).detach();//����ͻ��˷��ص�ע���˺����ɴ�//
	}
	else if (wcscmp(wreceived_two.c_str(), L"���") == 0)//�ͻ�����������ѡ���ж� ȷ����ȡ�� //
	{
		//�������ж�//
		//�ͻ��˸�����Ϣ����ѡ���ж� ȷ����ȡ�� //
		int len_three = 0;//���տͻ��˵õ��˺ź��ѡ�񣬼��� �� ȡ��//
		recvAll(client_server, (char*)&len_three, sizeof(len_three), 0);//�Ƚ����ֽڳ���//
		if ((int)len_three <= 0)
		{
			MessageBox(NULL, L"����'�ͻ��˸�����Ϣ����ѡ���ж� ȷ�ϻ�ȡ��'���ȴ���", L"QQ", MB_ICONERROR);//��ʶ���Ƚ��մ�����ʾ//
			return;
		}
		std::string received_three(len_three, 0);//����ռ�//
		recvAll(client_server, &received_three[0], len_three, 0);//���ܱ�ʶ����//
		int wlen_three = MultiByteToWideChar(CP_UTF8, 0, &received_three[0], len_three, NULL, 0);//����ת������//
		std::wstring wreceived_three(wlen_three, 0);
		MultiByteToWideChar(CP_UTF8, 0, &received_three[0], len_three, &wreceived_three[0], wlen_three);//ת��Ϊ���ַ�����//
		if (wcscmp(wreceived_three.c_str(), L"ȡ��") == 0)
		{
			std::thread(HandleClient_passwordset, client_server).detach();//���ص��ͻ����������ô�//
		}
		else if (wcscmp(wreceived_three.c_str(), L"ȷ��") == 0)
		{
			//���ĸ��ж�//
			//�ͻ���ͷ������ѡ���ж� ȷ����ȡ�� //
			int len_four = 0;//���տͻ���ͷ�����ú��ѡ��ȷ�� �� ȡ��//
			recvAll(client_server, (char*)&len_four, sizeof(len_four), 0);//�Ƚ����ֽڳ���//
			if ((int)len_four <= 0)
			{
				MessageBox(NULL, L"����'�ͻ���ͷ������ѡ���ж� ȷ�ϻ�ȡ��'���ȴ���", L"QQ", MB_ICONERROR);//��ʶ���Ƚ��մ�����ʾ//
				return;
			}
			std::string received_four(len_four, 0);//����ռ�//
			recvAll(client_server, &received_four[0], len_four, 0);//���ܱ�ʶ����//
			int wlen_four = MultiByteToWideChar(CP_UTF8, 0, &received_four[0], len_four, NULL, 0);//����ת������//
			std::wstring wreceived_four(wlen_four, 0);
			MultiByteToWideChar(CP_UTF8, 0, &received_four[0], len_four, &wreceived_four[0], wlen_four);//ת��Ϊ���ַ�����//
			if (wcscmp(wreceived_four.c_str(), L"ȡ��") == 0)
			{
				std::thread(HandleClient_informationset, client_server).detach();//���ص�������Ϣ���ô�//
			}
			else if (wcscmp(wreceived_four.c_str(), L"ȷ��") == 0)
			{

				//������ж�//
				//�ͻ���ע��ɹ�ѡ���ж� ȷ����ȡ�� //
				int len_five = 0;//���տͻ��˵õ��˺ź��ѡ�񣬼��� �� ȡ��//
				recvAll(client_server, (char*)&len_five, sizeof(len_five), 0);//�Ƚ����ֽڳ���//
				if ((int)len_five <= 0)
				{
					MessageBox(NULL, L"����'�ͻ���ע��ɹ�ѡ���ж� ȷ�ϻ�ȡ��'���ȴ���", L"QQ", MB_ICONERROR);//��ʶ���Ƚ��մ�����ʾ//
					return;
				}
				std::string received_five(len_five, 0);//����ռ�//
				recvAll(client_server, &received_five[0], len_five, 0);//���ܱ�ʶ����//
				int wlen_five = MultiByteToWideChar(CP_UTF8, 0, &received_five[0], len_five, NULL, 0);//����ת������//
				std::wstring wreceived_five(wlen_five, 0);
				MultiByteToWideChar(CP_UTF8, 0, &received_five[0], len_five, &wreceived_five[0], wlen_five);//ת��Ϊ���ַ�����//
				if (wcscmp(wreceived_five.c_str(), L"ȡ��") == 0)
				{
					std::thread(HandleClient_photoset, client_server).detach();//����ͷ�������߳�//
				}
				else if (wcscmp(wreceived_five.c_str(), L"ȷ��") == 0)
				{
					//std::lock_guard<std::mutex>lk(users_mutex);
					//users_update_signal = true;
					//users_cv.notify_one();//֪ͨ�������¼������ݿ�����û��ǳƺ�ͷ��//
					//update_new_users_list();//֪ͨ�������¼������ݿ�����û��ǳƺ�ͷ��//
					int x = 0;
					receivedData regData;//�ṹ�壬������������//
					while (true)
					{
						int sign_strlen = 0;//���ձ�ʶ����//
						recvAll(client_server, (char*)&sign_strlen, sizeof(sign_strlen), 0);//�Ƚ��ձ�ʶ�ֽڳ���//
						if ((int)sign_strlen <= 0)
						{
							MessageBox(NULL, L"���ձ�ʶ���ȴ���", L"QQ", MB_ICONERROR);//��ʶ���Ƚ��մ�����ʾ//
							return;
						}
						std::string sign_received(sign_strlen, 0);//����ռ�//
						recvAll(client_server, &sign_received[0], sign_strlen, 0);//���ܱ�ʶ����//
						int sign_wcharlen = MultiByteToWideChar(CP_UTF8, 0, &sign_received[0], sign_strlen, NULL, 0);//����ת������//
						std::wstring sign_wreceived(sign_wcharlen, 0);
						MultiByteToWideChar(CP_UTF8, 0, &sign_received[0], sign_strlen, &sign_wreceived[0], sign_wcharlen);//ת��Ϊ���ַ�����//

						int con_strlen = 0;//�������ݳ���//
						recvAll(client_server, (char*)&con_strlen, sizeof(con_strlen), 0);//�Ƚ��������ֽڳ���//
						if ((int)con_strlen <= 0)
						{
							MessageBox(NULL, L"�������ݳ��ȴ���", L"QQ", MB_ICONERROR);//���ݳ��Ƚ��մ�����ʾ//
							return;
						}

						if (wcscmp(sign_wreceived.c_str(), L"ͷ��") != 0)//��ʶ����ͷ�񣬶����˺š����롢�ǳơ��Ա�����͸���ǩ����ʱ���ʵ��//
						{
							std::string received(con_strlen, 0);//����ռ�//
							int rec = recvAll(client_server, &received[0], con_strlen, 0);//��������//
							int wcharlen = MultiByteToWideChar(CP_UTF8, 0, &received[0], con_strlen, NULL, 0);//����ת������//
							std::wstring wreceived(wcharlen, 0);
							MultiByteToWideChar(CP_UTF8, 0, &received[0], con_strlen, &wreceived[0], wcharlen);//ת��Ϊ���ַ�����//
							if (!sign_wreceived.empty() && sign_wreceived.back() == L'\0')//ȥ����ʶ�ַ�������ֹ��'\0',�ſ���ƴ�Ӳ���ʾ���������//
							{
								sign_wreceived.pop_back();
								//�����ݴ洢���ṹ�壬�Ա�洢�����ݿ�//
								if (wcscmp(sign_wreceived.c_str(), L"�˺�") == 0)
								{
									regData.account = received;
									regData.step++;
								}
								else if (wcscmp(sign_wreceived.c_str(), L"����") == 0)
								{
									regData.password = received;
									regData.step++;
								}
								else if (wcscmp(sign_wreceived.c_str(), L"�ǳ�") == 0)
								{
									regData.nickname = received;
									regData.step++;
								}
								else if (wcscmp(sign_wreceived.c_str(), L"�Ա�") == 0)
								{
									regData.gender = received;
									regData.step++;
								}
								else if (wcscmp(sign_wreceived.c_str(), L"����") == 0)
								{
									regData.age = received;
									regData.step++;
								}
								else if (wcscmp(sign_wreceived.c_str(), L"����ǩ��") == 0)
								{
									regData.signature = received;
									regData.step++;
								}
							}
							//��ȡ��ǰʱ��//
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
							EnterCriticalSection(&g_csMsgQueue);//����//
							g_msgQueue.push(fullMessage);//��������//
							LeaveCriticalSection(&g_csMsgQueue);//����//
							if (x == 0)
							{
								MessageBox(NULL, L"�������ѳɹ��������Կͻ��˵���Ϣ", L"QQ", MB_ICONINFORMATION);//ֻ��ʾһ��//
								x = 1;
							}
							//֪ͨ���жԻ����������//
							PostMessage(g_hInfoDialog, WM_APP_UPDATE_MSG, 0, 0);
						}
						else if (wcscmp(sign_wreceived.c_str(), L"ͷ��") == 0)
						{
							BYTE* received = new BYTE[con_strlen];//����ռ�//
							int r = 0;//���ν��յ��ֽ���//
							int sum = 0;//�ۼƽ��ܵ��ֽ���//
							while (sum < con_strlen)
							{
								r = recv(client_server, (char*)received + sum, con_strlen - sum, 0);//����ͼ�����������//
								if (r <= 0)
								{
									break;//���ս���//
								}
								sum += r;
							}
							//�����ݴ洢���ṹ�壬�Ա�洢�����ݿ�//
							regData.imgData.assign(received, received + sum);
							regData.step++;
							if (sum == con_strlen)
							{
								std::vector<BYTE>imgData(received, received + sum);//ͼƬ���ݴ洢��imgData����//
								std::lock_guard<std::mutex>lock(g_imgQueueMutex);//�̰߳�ȫ//
								g_imgQueue.push(std::move(imgData));//ͼƬ�������//
								delete[]received;
							}
							//֪ͨ����ͼƬ���������//
							PostMessage(g_hInfoDialogphoto, WM_APP_UPDATEPHOTO_MSG, 0, 0);
						}
						if (regData.step == 7)
						{
							SaveToDatabase(regData); // ���ṹ�����ݱ��浽���ݿ�//
							regData = receivedData(); // ��գ�׼����һ���û�//
							MessageBox(NULL, L"�������ѳɹ������յ�ע�����ݴ洢�����ݿ�", L"QQ", MB_ICONINFORMATION);
							break;
						}
					}
					update_new_users_list();//֪ͨ�������¼������ݿ�����û��ǳƺ�ͷ��//
					//����ע���߳�//
				   //�жϾ���ִ�������߳�,��¼��ע��//
					int recv_len = 0;
					recv(client_server, (char*)&recv_len, sizeof(recv_len), 0);//������Ϣ����//
					if (recv_len <= 0)
					{
						//MessageBox(NULL, L"�����߳����͵���Ϣ����ʧ��", L"QQ", MB_ICONERROR);
						return;
					}
					//MessageBox(NULL, L"�����߳����͵���Ϣ���ȳɹ�", L"QQ", MB_ICONINFORMATION);
					std::string recvchar(recv_len, 0);
					int r = 0;
					r = recv(client_server, &recvchar[0], recv_len, 0);

					if (r <= 0)
					{
						//MessageBox(NULL, L"�����߳����͵���Ϣʧ��", L"QQ", MB_ICONERROR);
						return;
					}

					//MessageBox(NULL, L"�����߳����͵���Ϣ�ɹ�", L"QQ", MB_ICONINFORMATION);
					int wlen = MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, NULL, 0);//ת��Ϊ���ֽ�����Ҫ���ֽ���//
					std::wstring wrecvchar(wlen, 0);//����ռ�//
					MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, &wrecvchar[0], wlen);//ʵ��ת��//
					//ע�ᴦ��//
					if (wcscmp(wrecvchar.c_str(), L"ע��") == 0)//ע���߳�//
					{
						//MessageBox(NULL, L"���յ�ע����Ϣ����������ע���߳�", L"QQ", MB_ICONINFORMATION);
						std::thread(HandleClient_register, client_server).detach();//����ע���߳�//
					}
					//��¼����//
					else if (wcscmp(wrecvchar.c_str(), L"��¼") == 0)//��¼�߳�//
					{
						//MessageBox(NULL, L"���յ���¼��Ϣ���������õ�¼�߳�", L"QQ", MB_ICONINFORMATION);
						std::thread(HandleClient_login, client_server,new_online_user_account).detach();//������¼�߳�//
					}
					else
					{
						//MessageBox(NULL, L"���ղ����߳���Ϣ�������˳�", L"QQ", MB_ICONINFORMATION);
						return;
					}
					//MessageBox(NULL, L"���������޷��������Կͻ��˵���Ϣ", L"QQ", MB_ICONERROR);
				   //closesocket(client_server);
				}
			}
		}
	}
}


//������Ϣ�������߳�//
void HandleClient_informationset(SOCKET client_server)
{
	std::string new_online_user_account;
	MessageBox(NULL, L"�㼴������������Ϣ�����߳�", L"QQ", MB_ICONINFORMATION);
	//�������ж�//
	//�ͻ��˸�����Ϣ����ѡ���ж� ȷ����ȡ�� //
	int len_three = 0;//���տͻ��˸�����Ϣ���ú��ѡ�񣬼��� �� ȡ��//
	recvAll(client_server, (char*)&len_three, sizeof(len_three), 0);//�Ƚ����ֽڳ���//
	MessageBox(NULL, L"���ѽ���ѡ���ֶγ���", L"QQ", MB_ICONINFORMATION);
	if (len_three <= 0)
	{
		MessageBox(NULL, L"����'�ͻ��˸�����Ϣ����ѡ���ж� ȷ�ϻ�ȡ��'���ȴ���", L"QQ", MB_ICONERROR);//��ʶ���Ƚ��մ�����ʾ//
		return;
	}
	std::string received_three(len_three, 0);//����ռ�//
	recvAll(client_server, &received_three[0], len_three, 0);//���ܱ�ʶ����//
	int wlen_three = MultiByteToWideChar(CP_UTF8, 0, &received_three[0], len_three, NULL, 0);//����ת������//
	std::wstring wreceived_three(wlen_three, 0);
	MultiByteToWideChar(CP_UTF8, 0, &received_three[0], len_three, &wreceived_three[0], wlen_three);//ת��Ϊ���ַ�����//
	if (wcscmp(wreceived_three.c_str(), L"ȡ��") == 0)
	{
		std::thread(HandleClient_passwordset, client_server).detach();//���ص��ͻ����������ô�//
	}
	else if (wcscmp(wreceived_three.c_str(), L"ȷ��") == 0)
	{
		//���ĸ��ж�//
		//�ͻ���ͷ������ѡ���ж� ȷ����ȡ�� //
		int len_four = 0;//���տͻ��˵õ��˺ź��ѡ�񣬼��� �� ȡ��//
		recvAll(client_server, (char*)&len_four, sizeof(len_four), 0);//�Ƚ����ֽڳ���//
		if ((int)len_four <= 0)
		{
			MessageBox(NULL, L"����'�ͻ���ͷ������ѡ���ж� ȷ�ϻ�ȡ��'���ȴ���", L"QQ", MB_ICONERROR);//��ʶ���Ƚ��մ�����ʾ//
			return;
		}
		std::string received_four(len_four, 0);//����ռ�//
		recvAll(client_server, &received_four[0], len_four, 0);//���ܱ�ʶ����//
		int wlen_four = MultiByteToWideChar(CP_UTF8, 0, &received_four[0], len_four, NULL, 0);//����ת������//
		std::wstring wreceived_four(wlen_four, 0);
		MultiByteToWideChar(CP_UTF8, 0, &received_four[0], len_four, &wreceived_four[0], wlen_four);//ת��Ϊ���ַ�����//
		if (wcscmp(wreceived_four.c_str(), L"ȡ��") == 0)
		{
			std::thread(HandleClient_informationset, client_server).detach();//���ص�������Ϣ���ô�//
		}
		else if (wcscmp(wreceived_four.c_str(), L"ȷ��") == 0)
		{
			//������ж�//
		   //�ͻ���ע��ɹ�ѡ���ж� ȷ����ȡ�� //
			int len_five = 0;//���տͻ��˵õ��˺ź��ѡ�񣬼��� �� ȡ��//
			recvAll(client_server, (char*)&len_five, sizeof(len_five), 0);//�Ƚ����ֽڳ���//
			if ((int)len_five <= 0)
			{
				MessageBox(NULL, L"����'�ͻ���ע��ɹ�ѡ���ж� ȷ�ϻ�ȡ��'���ȴ���", L"QQ", MB_ICONERROR);//��ʶ���Ƚ��մ�����ʾ//
				return;
			}
			std::string received_five(len_five, 0);//����ռ�//
			recvAll(client_server, &received_five[0], len_five, 0);//���ܱ�ʶ����//
			int wlen_five = MultiByteToWideChar(CP_UTF8, 0, &received_five[0], len_five, NULL, 0);//����ת������//
			std::wstring wreceived_five(wlen_five, 0);
			MultiByteToWideChar(CP_UTF8, 0, &received_five[0], len_five, &wreceived_five[0], wlen_five);//ת��Ϊ���ַ�����//
			if (wcscmp(wreceived_five.c_str(), L"ȡ��") == 0)
			{
				std::thread(HandleClient_photoset, client_server).detach();//����ͷ�������߳�//
			}
			else if (wcscmp(wreceived_five.c_str(), L"ȷ��") == 0)
			{
				//std::lock_guard<std::mutex>lk(users_mutex);
				//users_update_signal = true;
				//users_cv.notify_one();//֪ͨ�������¼������ݿ�����û��ǳƺ�ͷ��//
				//update_new_users_list();//֪ͨ�������¼������ݿ�����û��ǳƺ�ͷ��//
				int x = 0;
				receivedData regData;//�ṹ�壬������������//
				while (true)
				{
					int sign_strlen = 0;//���ձ�ʶ����//
					recvAll(client_server, (char*)&sign_strlen, sizeof(sign_strlen), 0);//�Ƚ��ձ�ʶ�ֽڳ���//
					if ((int)sign_strlen <= 0)
					{
						MessageBox(NULL, L"���ձ�ʶ���ȴ���", L"QQ", MB_ICONERROR);//��ʶ���Ƚ��մ�����ʾ//
						return;
					}
					std::string sign_received(sign_strlen, 0);//����ռ�//
					recvAll(client_server, &sign_received[0], sign_strlen, 0);//���ܱ�ʶ����//
					int sign_wcharlen = MultiByteToWideChar(CP_UTF8, 0, &sign_received[0], sign_strlen, NULL, 0);//����ת������//
					std::wstring sign_wreceived(sign_wcharlen, 0);
					MultiByteToWideChar(CP_UTF8, 0, &sign_received[0], sign_strlen, &sign_wreceived[0], sign_wcharlen);//ת��Ϊ���ַ�����//

					int con_strlen = 0;//�������ݳ���//
					recvAll(client_server, (char*)&con_strlen, sizeof(con_strlen), 0);//�Ƚ��������ֽڳ���//
					if ((int)con_strlen <= 0)
					{
						MessageBox(NULL, L"�������ݳ��ȴ���", L"QQ", MB_ICONERROR);//���ݳ��Ƚ��մ�����ʾ//
						return;
					}

					if (wcscmp(sign_wreceived.c_str(), L"ͷ��") != 0)//��ʶ����ͷ�񣬶����˺š����롢�ǳơ��Ա�����͸���ǩ����ʱ���ʵ��//
					{
						std::string received(con_strlen, 0);//����ռ�//
						int rec = recvAll(client_server, &received[0], con_strlen, 0);//��������//
						int wcharlen = MultiByteToWideChar(CP_UTF8, 0, &received[0], con_strlen, NULL, 0);//����ת������//
						std::wstring wreceived(wcharlen, 0);
						MultiByteToWideChar(CP_UTF8, 0, &received[0], con_strlen, &wreceived[0], wcharlen);//ת��Ϊ���ַ�����//
						if (!sign_wreceived.empty() && sign_wreceived.back() == L'\0')//ȥ����ʶ�ַ�������ֹ��'\0',�ſ���ƴ�Ӳ���ʾ���������//
						{
							sign_wreceived.pop_back();
							//�����ݴ洢���ṹ�壬�Ա�洢�����ݿ�//
							if (wcscmp(sign_wreceived.c_str(), L"�˺�") == 0)
							{
								regData.account = received;
								regData.step++;
							}
							else if (wcscmp(sign_wreceived.c_str(), L"����") == 0)
							{
								regData.password = received;
								regData.step++;
							}
							else if (wcscmp(sign_wreceived.c_str(), L"�ǳ�") == 0)
							{
								regData.nickname = received;
								regData.step++;
							}
							else if (wcscmp(sign_wreceived.c_str(), L"�Ա�") == 0)
							{
								regData.gender = received;
								regData.step++;
							}
							else if (wcscmp(sign_wreceived.c_str(), L"����") == 0)
							{
								regData.age = received;
								regData.step++;
							}
							else if (wcscmp(sign_wreceived.c_str(), L"����ǩ��") == 0)
							{
								regData.signature = received;
								regData.step++;
							}
						}
						//��ȡ��ǰʱ��//
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
						EnterCriticalSection(&g_csMsgQueue);//����//
						g_msgQueue.push(fullMessage);//��������//
						LeaveCriticalSection(&g_csMsgQueue);//����//
						if (x == 0)
						{
							MessageBox(NULL, L"�������ѳɹ��������Կͻ��˵���Ϣ", L"QQ", MB_ICONINFORMATION);//ֻ��ʾһ��//
							x = 1;
						}
						//֪ͨ���жԻ����������//
						PostMessage(g_hInfoDialog, WM_APP_UPDATE_MSG, 0, 0);
					}
					else if (wcscmp(sign_wreceived.c_str(), L"ͷ��") == 0)
					{
						BYTE* received = new BYTE[con_strlen];//����ռ�//
						int r = 0;//���ν��յ��ֽ���//
						int sum = 0;//�ۼƽ��ܵ��ֽ���//
						while (sum < con_strlen)
						{
							r = recv(client_server, (char*)received + sum, con_strlen - sum, 0);//����ͼ�����������//
							if (r <= 0)
							{
								break;//���ս���//
							}
							sum += r;
						}
						//�����ݴ洢���ṹ�壬�Ա�洢�����ݿ�//
						regData.imgData.assign(received, received + sum);
						regData.step++;
						if (sum == con_strlen)
						{
							std::vector<BYTE>imgData(received, received + sum);//ͼƬ���ݴ洢��imgData����//
							std::lock_guard<std::mutex>lock(g_imgQueueMutex);//�̰߳�ȫ//
							g_imgQueue.push(std::move(imgData));//ͼƬ�������//
							delete[]received;
						}
						//֪ͨ����ͼƬ���������//
						PostMessage(g_hInfoDialogphoto, WM_APP_UPDATEPHOTO_MSG, 0, 0);
					}
					if (regData.step == 7)
					{
						SaveToDatabase(regData); // ���ṹ�����ݱ��浽���ݿ�//
						regData = receivedData(); // ��գ�׼����һ���û�//
						MessageBox(NULL, L"�������ѳɹ������յ�ע�����ݴ洢�����ݿ�", L"QQ", MB_ICONINFORMATION);
						break;
					}
				}
				update_new_users_list();//֪ͨ�������¼������ݿ�����û��ǳƺ�ͷ��//
				//����ע���߳�//
			   //�жϾ���ִ�������߳�,��¼��ע��//
				int recv_len = 0;
				recv(client_server, (char*)&recv_len, sizeof(recv_len), 0);//������Ϣ����//
				if (recv_len <= 0)
				{
					//MessageBox(NULL, L"�����߳����͵���Ϣ����ʧ��", L"QQ", MB_ICONERROR);
					return;
				}
				//MessageBox(NULL, L"�����߳����͵���Ϣ���ȳɹ�", L"QQ", MB_ICONINFORMATION);
				std::string recvchar(recv_len, 0);
				int r = 0;
				r = recv(client_server, &recvchar[0], recv_len, 0);
				{
					if (r <= 0)
					{
						//MessageBox(NULL, L"�����߳����͵���Ϣʧ��", L"QQ", MB_ICONERROR);
						return;
					}
				}
				//MessageBox(NULL, L"�����߳����͵���Ϣ�ɹ�", L"QQ", MB_ICONINFORMATION);
				int wlen = MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, NULL, 0);//ת��Ϊ���ֽ�����Ҫ���ֽ���//
				std::wstring wrecvchar(wlen, 0);//����ռ�//
				MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, &wrecvchar[0], wlen);//ʵ��ת��//
				//ע�ᴦ��//
				if (wcscmp(wrecvchar.c_str(), L"ע��") == 0)//ע���߳�//
				{
					//MessageBox(NULL, L"���յ�ע����Ϣ����������ע���߳�", L"QQ", MB_ICONINFORMATION);
					std::thread(HandleClient_register, client_server).detach();//����ע���߳�//
				}
				//��¼����//
				else if (wcscmp(wrecvchar.c_str(), L"��¼") == 0)//��¼�߳�//
				{
					//MessageBox(NULL, L"���յ���¼��Ϣ���������õ�¼�߳�", L"QQ", MB_ICONINFORMATION);
					std::thread(HandleClient_login, client_server,new_online_user_account).detach();//������¼�߳�//
				}
				else
				{
					//MessageBox(NULL, L"���ղ����߳���Ϣ�������˳�", L"QQ", MB_ICONINFORMATION);
					return;
				}
				//MessageBox(NULL, L"���������޷��������Կͻ��˵���Ϣ", L"QQ", MB_ICONERROR);
				//closesocket(client_server);
			}
		}
	}
}

//ͷ�������߳�//
void HandleClient_photoset(SOCKET client_server)
{
	std::string new_online_user_account;
	//���ĸ��ж�//
	//�ͻ���ͷ������ѡ���ж� ȷ����ȡ�� //
	int len_four = 0;//���տͻ���ͷ�����ú��ѡ�񣬼��� �� ȡ��//
	recvAll(client_server, (char*)&len_four, sizeof(len_four), 0);//�Ƚ����ֽڳ���//
	if ((int)len_four <= 0)
	{
		MessageBox(NULL, L"����'�ͻ���ͷ������ѡ���ж� ȷ�ϻ�ȡ��'���ȴ���", L"QQ", MB_ICONERROR);//��ʶ���Ƚ��մ�����ʾ//
		return;
	}
	std::string received_four(len_four, 0);//����ռ�//
	recvAll(client_server, &received_four[0], len_four, 0);//���ܱ�ʶ����//
	int wlen_four = MultiByteToWideChar(CP_UTF8, 0, &received_four[0], len_four, NULL, 0);//����ת������//
	std::wstring wreceived_four(wlen_four, 0);
	MultiByteToWideChar(CP_UTF8, 0, &received_four[0], len_four, &wreceived_four[0], wlen_four);//ת��Ϊ���ַ�����//
	if (wcscmp(wreceived_four.c_str(), L"ȡ��") == 0)
	{
		std::thread(HandleClient_informationset, client_server).detach();//���ص�������Ϣ���ô�//
	}
	else if (wcscmp(wreceived_four.c_str(), L"ȷ��") == 0)
	{
		//������ж�//
		//�ͻ���ע��ɹ�ѡ���ж� ȷ����ȡ�� //
		int len_five = 0;//���տͻ��˵õ��˺ź��ѡ�񣬼��� �� ȡ��//
		recvAll(client_server, (char*)&len_five, sizeof(len_five), 0);//�Ƚ����ֽڳ���//
		if ((int)len_five <= 0)
		{
			MessageBox(NULL, L"����'�ͻ���ע��ɹ�ѡ���ж� ȷ�ϻ�ȡ��'���ȴ���", L"QQ", MB_ICONERROR);//��ʶ���Ƚ��մ�����ʾ//
			return;
		}
		std::string received_five(len_five, 0);//����ռ�//
		recvAll(client_server, &received_five[0], len_five, 0);//���ܱ�ʶ����//
		int wlen_five = MultiByteToWideChar(CP_UTF8, 0, &received_five[0], len_five, NULL, 0);//����ת������//
		std::wstring wreceived_five(wlen_five, 0);
		MultiByteToWideChar(CP_UTF8, 0, &received_five[0], len_five, &wreceived_five[0], wlen_five);//ת��Ϊ���ַ�����//
		if (wcscmp(wreceived_five.c_str(), L"ȡ��") == 0)
		{
			std::thread(HandleClient_photoset, client_server).detach();//����ͷ�������߳�//
		}
		else if (wcscmp(wreceived_five.c_str(), L"ȷ��") == 0)//ע��ɹ�ȷ���߳�//
		{
			//std::lock_guard<std::mutex>lk(users_mutex);
			//users_update_signal = true;
			//users_cv.notify_one();//֪ͨ�������¼������ݿ�����û��ǳƺ�ͷ��//
			//update_new_users_list();//֪ͨ�������¼������ݿ�����û��ǳƺ�ͷ��//
			int x = 0;
			receivedData regData;//�ṹ�壬������������//
			while (true)
			{
				int sign_strlen = 0;//���ձ�ʶ����//
				recvAll(client_server, (char*)&sign_strlen, sizeof(sign_strlen), 0);//�Ƚ��ձ�ʶ�ֽڳ���//
				if ((int)sign_strlen <= 0)
				{
					MessageBox(NULL, L"���ձ�ʶ���ȴ���", L"QQ", MB_ICONERROR);//��ʶ���Ƚ��մ�����ʾ//
					return;
				}
				std::string sign_received(sign_strlen, 0);//����ռ�//
				recvAll(client_server, &sign_received[0], sign_strlen, 0);//���ܱ�ʶ����//
				int sign_wcharlen = MultiByteToWideChar(CP_UTF8, 0, &sign_received[0], sign_strlen, NULL, 0);//����ת������//
				std::wstring sign_wreceived(sign_wcharlen, 0);
				MultiByteToWideChar(CP_UTF8, 0, &sign_received[0], sign_strlen, &sign_wreceived[0], sign_wcharlen);//ת��Ϊ���ַ�����//

				int con_strlen = 0;//�������ݳ���//
				recvAll(client_server, (char*)&con_strlen, sizeof(con_strlen), 0);//�Ƚ��������ֽڳ���//
				if ((int)con_strlen <= 0)
				{
					MessageBox(NULL, L"�������ݳ��ȴ���", L"QQ", MB_ICONERROR);//���ݳ��Ƚ��մ�����ʾ//
					return;
				}

				if (wcscmp(sign_wreceived.c_str(), L"ͷ��") != 0)//��ʶ����ͷ�񣬶����˺š����롢�ǳơ��Ա�����͸���ǩ����ʱ���ʵ��//
				{
					std::string received(con_strlen, 0);//����ռ�//
					int rec = recvAll(client_server, &received[0], con_strlen, 0);//��������//
					int wcharlen = MultiByteToWideChar(CP_UTF8, 0, &received[0], con_strlen, NULL, 0);//����ת������//
					std::wstring wreceived(wcharlen, 0);
					MultiByteToWideChar(CP_UTF8, 0, &received[0], con_strlen, &wreceived[0], wcharlen);//ת��Ϊ���ַ�����//
					if (!sign_wreceived.empty() && sign_wreceived.back() == L'\0')//ȥ����ʶ�ַ�������ֹ��'\0',�ſ���ƴ�Ӳ���ʾ���������//
					{
						sign_wreceived.pop_back();
						//�����ݴ洢���ṹ�壬�Ա�洢�����ݿ�//
						if (wcscmp(sign_wreceived.c_str(), L"�˺�") == 0)
						{
							regData.account = received;
							regData.step++;
						}
						else if (wcscmp(sign_wreceived.c_str(), L"����") == 0)
						{
							regData.password = received;
							regData.step++;
						}
						else if (wcscmp(sign_wreceived.c_str(), L"�ǳ�") == 0)
						{
							regData.nickname = received;
							regData.step++;
						}
						else if (wcscmp(sign_wreceived.c_str(), L"�Ա�") == 0)
						{
							regData.gender = received;
							regData.step++;
						}
						else if (wcscmp(sign_wreceived.c_str(), L"����") == 0)
						{
							regData.age = received;
							regData.step++;
						}
						else if (wcscmp(sign_wreceived.c_str(), L"����ǩ��") == 0)
						{
							regData.signature = received;
							regData.step++;
						}
					}
					//��ȡ��ǰʱ��//
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
					EnterCriticalSection(&g_csMsgQueue);//����//
					g_msgQueue.push(fullMessage);//��������//
					LeaveCriticalSection(&g_csMsgQueue);//����//
					if (x == 0)
					{
						MessageBox(NULL, L"�������ѳɹ��������Կͻ��˵���Ϣ", L"QQ", MB_ICONINFORMATION);//ֻ��ʾһ��//
						x = 1;
					}
					//֪ͨ���жԻ����������//
					PostMessage(g_hInfoDialog, WM_APP_UPDATE_MSG, 0, 0);
				}
				else if (wcscmp(sign_wreceived.c_str(), L"ͷ��") == 0)
				{
					BYTE* received = new BYTE[con_strlen];//����ռ�//
					int r = 0;//���ν��յ��ֽ���//
					int sum = 0;//�ۼƽ��ܵ��ֽ���//
					while (sum < con_strlen)
					{
						r = recv(client_server, (char*)received + sum, con_strlen - sum, 0);//����ͼ�����������//
						if (r <= 0)
						{
							break;//���ս���//
						}
						sum += r;
					}
					//�����ݴ洢���ṹ�壬�Ա�洢�����ݿ�//
					regData.imgData.assign(received, received + sum);
					regData.step++;
					if (sum == con_strlen)
					{
						std::vector<BYTE>imgData(received, received + sum);//ͼƬ���ݴ洢��imgData����//
						std::lock_guard<std::mutex>lock(g_imgQueueMutex);//�̰߳�ȫ//
						g_imgQueue.push(std::move(imgData));//ͼƬ�������//
						delete[]received;
					}
					//֪ͨ����ͼƬ���������//
					PostMessage(g_hInfoDialogphoto, WM_APP_UPDATEPHOTO_MSG, 0, 0);
				}
				if (regData.step == 7)
				{
					SaveToDatabase(regData); // ���ṹ�����ݱ��浽���ݿ�//
					regData = receivedData(); // ��գ�׼����һ���û�//
					MessageBox(NULL, L"�������ѳɹ������յ�ע�����ݴ洢�����ݿ�", L"QQ", MB_ICONINFORMATION);
					break;
				}
			}
			update_new_users_list();//֪ͨ�������¼������ݿ�����û��ǳƺ�ͷ��//
			//����ע���߳�//
			//�жϾ���ִ�������߳�,��¼��ע��//
			int recv_len = 0;
			recv(client_server, (char*)&recv_len, sizeof(recv_len), 0);//������Ϣ����//
			if (recv_len <= 0)
			{
				//MessageBox(NULL, L"�����߳����͵���Ϣ����ʧ��", L"QQ", MB_ICONERROR);
				return;
			}
			//MessageBox(NULL, L"�����߳����͵���Ϣ���ȳɹ�", L"QQ", MB_ICONINFORMATION);
			std::string recvchar(recv_len, 0);
			int r = 0;
			r = recv(client_server, &recvchar[0], recv_len, 0);
			{
				if (r <= 0)
				{
					//MessageBox(NULL, L"�����߳����͵���Ϣʧ��", L"QQ", MB_ICONERROR);
					return;
				}
			}
			//MessageBox(NULL, L"�����߳����͵���Ϣ�ɹ�", L"QQ", MB_ICONINFORMATION);
			int wlen = MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, NULL, 0);//ת��Ϊ���ֽ�����Ҫ���ֽ���//
			std::wstring wrecvchar(wlen, 0);//����ռ�//
			MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, &wrecvchar[0], wlen);//ʵ��ת��//
			//ע�ᴦ��//
			if (wcscmp(wrecvchar.c_str(), L"ע��") == 0)//ע���߳�//
			{
				//MessageBox(NULL, L"���յ�ע����Ϣ����������ע���߳�", L"QQ", MB_ICONINFORMATION);
				std::thread(HandleClient_register, client_server).detach();//����ע���߳�//
			}
			//��¼����//
			else if (wcscmp(wrecvchar.c_str(), L"��¼") == 0)//��¼�߳�//
			{
				//MessageBox(NULL, L"���յ���¼��Ϣ���������õ�¼�߳�", L"QQ", MB_ICONINFORMATION);
				std::thread(HandleClient_login, client_server,new_online_user_account).detach();//������¼�߳�//
			}
			else
			{
				//MessageBox(NULL, L"���ղ����߳���Ϣ�������˳�", L"QQ", MB_ICONINFORMATION);
				return;
			}
			//MessageBox(NULL, L"���������޷��������Կͻ��˵���Ϣ", L"QQ", MB_ICONERROR);
			//closesocket(client_server);
		}
	}
}


std::string my_recv_one(SOCKET socket)
{
	int recv_len = 0;
	recv(socket, (char*)&recv_len, sizeof(recv_len), 0);//���ճ���//
	if (recv_len <= 0)
	{
		MessageBox(NULL, L"�����˺ų���ʧ��", L"QQ", MB_ICONERROR);
		return "";
	}
	std::string recvchar(recv_len, 0);//����ռ�//
	recv(socket, &recvchar[0], recv_len, 0);//��������//
	return recvchar;
}


//���ݿ�ƥ���˺ź�����ĺ���//
bool is_account_registered(const char* account, const char* password, MYSQL* conn,receivedData &my_data)
{
	// 1. Ԥ���� SQL ��ѯ
	const char* sql = "SELECT account,password,nickname,gender,age,signature,imgData FROM users WHERE account=? AND password=? LIMIT 1";//����һ��SQL��ѯ�����ַ�������෵��һ����¼//
	MYSQL_STMT* stmt = mysql_stmt_init(conn);//��ʼ��һ��Ԥ�����������conn�����ݿ����Ӿ��//
	if (!stmt)
	{
		MessageBox(NULL, L"���ݿ�Ԥ�����������ʼ��ʧ��", L"QQ", MB_ICONERROR);
		return false;
	}

	if (mysql_stmt_prepare(stmt, sql, strlen(sql)))//��SQL�����ַ���ת��Ϊ���ݿ��ڲ��ṹ�����ִ��Ч��// 
	{
		MessageBox(NULL, L"SQL���ת��Ϊ���ݿ��ڲ��ṹʧ��", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}


	// 2. ���������
	MYSQL_BIND bind[2];//��������Ϊ2//
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_STRING;
	bind[0].buffer = (char*)account;
	bind[0].buffer_length = strlen(account);

	bind[1].buffer_type = MYSQL_TYPE_STRING;
	bind[1].buffer = (char*)password;
	bind[1].buffer_length = strlen(password);

	if (mysql_stmt_bind_param(stmt, bind))//�󶨲���//
	{
		MessageBox(NULL, L"�󶨲���ʧ��", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}

	// 3. �󶨽���������������
	char*account_buf = new char[65];     // �����ֶ���󳤶�Ϊ64
	char*password_buf=new char[65];
	char*nickname_buf=new char[65];
	char*gender_buf=new char[9];
	char*age_buf=new char[9];
	char*signature_buf=new char[257];  // �������256
	BYTE*img_buf=new BYTE[10*1024 * 1024]; // 10MB���ͷ��

	unsigned long lens[7] = { 0 };//���ڴ���ÿ���ֶε�ʵ�ʳ���//
	bool is_null[7] = { 0 };//���ڱ�ʶÿ���ֶ��Ƿ�ΪNULL//
	
	// 4. �󶨽��
	MYSQL_BIND result_bind[7];//��������Ϊ1//
	memset(result_bind, 0, sizeof(result_bind));
	result_bind[0].buffer_type = MYSQL_TYPE_STRING;
	result_bind[0].buffer = account_buf;
	result_bind[0].buffer_length =65;//����д���ֽ���//
	result_bind[0].is_null =&is_null[0];//��ʼ��//
	result_bind[0].length =&lens[0];//��ʼ��//

	result_bind[1].buffer_type = MYSQL_TYPE_STRING;
	result_bind[1].buffer = password_buf;
	result_bind[1].buffer_length = 65;
	result_bind[1].is_null =&is_null[1];//��ʼ��//
	result_bind[1].length =&lens[1];//��ʼ��//

	result_bind[2].buffer_type = MYSQL_TYPE_STRING;
	result_bind[2].buffer = nickname_buf;
	result_bind[2].buffer_length= 65;
	result_bind[2].is_null = &is_null[2];//��ʼ��//
	result_bind[2].length = &lens[2];//��ʼ��//

	result_bind[3].buffer_type = MYSQL_TYPE_STRING;
	result_bind[3].buffer = gender_buf;
	result_bind[3].buffer_length = 9;
	result_bind[3].is_null = &is_null[3];//��ʼ��//
	result_bind[3].length = &lens[3];//��ʼ��//

	result_bind[4].buffer_type = MYSQL_TYPE_STRING;
	result_bind[4].buffer = age_buf;
	result_bind[4].buffer_length = 9;
	result_bind[4].is_null =&is_null[4];//��ʼ��//
	result_bind[4].length =&lens[4];//��ʼ��//

	result_bind[5].buffer_type = MYSQL_TYPE_STRING;
	result_bind[5].buffer = signature_buf;
	result_bind[5].buffer_length = 257;
	result_bind[5].is_null =&is_null[5];//��ʼ��//
	result_bind[5].length =&lens[5];//��ʼ��//

	result_bind[6].buffer_type = MYSQL_TYPE_BLOB;
	result_bind[6].buffer = img_buf;
	result_bind[6].buffer_length = 10*1024*1024;
	result_bind[6].is_null = &is_null[6];//��ʼ��//
	result_bind[6].length =&lens[6];//��ʼ��//

	// 3. ִ��
	if (mysql_stmt_execute(stmt))
	{
		MessageBox(NULL, L"ִ��SQL���ʧ��", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}

	if (mysql_stmt_bind_result(stmt, result_bind))//�󶨽������//
	{
		MessageBox(NULL, L"�󶨽������ʧ��", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}
	// 5. ��ȡ���
	bool found = false;
	if (mysql_stmt_store_result(stmt) == 0) //�ѽ�������嵽����//
	{
		MessageBox(NULL, L"�ɹ��ѽ�����嵽����", L"QQ", MB_ICONINFORMATION);
		if (mysql_stmt_fetch(stmt) == 0) // ��һ����¼�������ᱻ����󶨵ı���//
		{      
			MessageBox(NULL, L"�ɹ���ѯ�����", L"QQ", MB_ICONINFORMATION);
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



// ɾ��ָ���˺ź�������û�
bool delete_user_by_account_password(const char* account, const char* password, MYSQL* conn)
{
	// 1. Ԥ���� SQL ɾ�����
	const char* sql = "DELETE FROM users WHERE account=? AND password=? LIMIT 1";
	MYSQL_STMT* stmt = mysql_stmt_init(conn);//��ȡԤ���������//
	if (!stmt)
	{
		MessageBox(NULL, L"���ݿ�Ԥ�����������ʼ��ʧ��", L"QQ", MB_ICONERROR);
		return false;
	}

	if (mysql_stmt_prepare(stmt, sql, strlen(sql)))//��SQL���ת��Ϊ���ݿ��ڲ��ṹ//
	{
		MessageBox(NULL, L"SQL���ת��Ϊ���ݿ��ڲ��ṹʧ��", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);//�ͷ�Ԥ���������//
		return false;
	}

	// 2. ���������
	MYSQL_BIND bind[2];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_STRING;
	bind[0].buffer = (char*)account;
	bind[0].buffer_length = strlen(account);

	bind[1].buffer_type = MYSQL_TYPE_STRING;
	bind[1].buffer = (char*)password;
	bind[1].buffer_length = strlen(password);

	if (mysql_stmt_bind_param(stmt, bind))//�󶨲���//
	{
		MessageBox(NULL, L"�󶨲���ʧ��", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}

	// 3. ִ��
	if (mysql_stmt_execute(stmt))
	{
		MessageBox(NULL, L"ִ��SQL���ʧ��", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}

	// 4. �ж��Ƿ����ɾ��
	my_ulonglong affected_rows = mysql_stmt_affected_rows(stmt);//�鿴���һ��Ԥ������䴦�����Ӱ�������//
	mysql_stmt_close(stmt);

	if (affected_rows > 0)
	{
		MessageBox(NULL, L"�û�ɾ���ɹ�", L"QQ", MB_ICONINFORMATION);
		return true;
	}
	else
	{
		MessageBox(NULL, L"δ�ҵ�ƥ����û���ɾ��ʧ��", L"QQ", MB_ICONERROR);
		return false;
	}
}



// �����û��˺Ų����û�ͼƬ����
bool find_imgdata_according_to_account(const char* account, MYSQL* conn_3,receivedData& mydata)
{
	int con_num = 0;
	while (!mysql_real_connect(conn_3, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))//�������ݿ�//
	{
		//MessageBox(NULL, L"�������ݿ�ʧ��", L"QQ", MB_ICONERROR);
		con_num++;
		if (con_num > 3)
		{
			return false;
		}
	}

	// 1. Ԥ���� SQL ���
	const char* sql = "SELECT ImgData FROM users WHERE account=? LIMIT 1";
	MYSQL_STMT* stmt = mysql_stmt_init(conn_3);//��ȡԤ���������//
	if (!stmt)
	{
		MessageBox(NULL, L"���ݿ�Ԥ�����������ʼ��ʧ��", L"QQ", MB_ICONERROR);
		return false;
	}
	if (mysql_stmt_prepare(stmt, sql, strlen(sql)))//��SQL���ת��Ϊ���ݿ��ڲ��ṹ//
	{
		MessageBox(NULL, L"SQL���ת��Ϊ���ݿ��ڲ��ṹʧ��", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);//�ͷ�Ԥ���������//
		return false;
	}
	// 2. ���������
	MYSQL_BIND bind[1];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_STRING;
	bind[0].buffer = (char*)account;
	bind[0].buffer_length = strlen(account);


	if (mysql_stmt_bind_param(stmt, bind))//�󶨲���//
	{
		MessageBox(NULL, L"�󶨲���ʧ��", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}

	// 3. ִ��
	if (mysql_stmt_execute(stmt))
	{
		MessageBox(NULL, L"ִ��SQL���ʧ��", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}

	//4.�󶨽��
	unsigned long img_length = 0;
	bool is_null = false;
	bool error = false;
	
	BYTE img_buffer[1];
	MYSQL_BIND bind_result[1];
	memset(bind_result,0,sizeof(bind_result));
	bind_result[0].buffer_type = MYSQL_TYPE_BLOB;
	bind_result[0].buffer = img_buffer;
	bind_result[0].buffer_length = sizeof(img_buffer);//����д�������ֽ���
	bind_result[0].length = &img_length;//ʵ��д����ֽ���
	bind_result[0].is_null = &is_null;
	bind_result[0].error = &error;

	if (mysql_stmt_bind_result(stmt, bind_result))
	{
		MessageBox(NULL, L"�󶨽��ʧ��", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}

	//5.��ȡ���
	if (mysql_stmt_store_result(stmt))
	{
		MessageBox(NULL,L"������ʧ��",L"QQ",MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}
	int ret = mysql_stmt_fetch(stmt);
	if (ret == 0 || ret == MYSQL_DATA_TRUNCATED)//�ɹ���ȡ���
	{
		//��ȡͼƬ���ݳ���
		if (!is_null && img_length > 0)
		{
			mydata.imgData.resize(img_length);
			//��������bind_result���mydata.imgData
			bind_result[0].buffer = mydata.imgData.data();
			bind_result[0].buffer_length = img_length;
			//�ٴλ�ȡ��������
			if (mysql_stmt_fetch_column(stmt, &bind_result[0], 0, 0))
			{
				MessageBox(NULL,L"��ȡͼƬ����ʧ��",L"QQ",MB_ICONERROR);
				mysql_stmt_free_result(stmt);//���ͷű��ؽ����������
				mysql_stmt_close(stmt);
				return false;
			}
		}
		else
		{
			//û��ͷ��
			mydata.imgData.clear();
			return false;
		}
	}
	mysql_stmt_free_result(stmt);
	mysql_stmt_close(stmt);
	return true;
}





//�û��ǳƺ�ͷ���ѯ���//
void LoadUsersFromDB(MYSQL* conn)
{
	while (true)//�����û��˺š��ǳƺ�ͷ����Ϣ//
	{
		std::unique_lock<std::mutex>lk(users_mutex);
		//�ȴ��źű�����Ϊtrue//
		users_cv.wait(lk, []{return users_update_signal; });
		users_update_signal = false;
		lk.unlock();

		std::vector<Userdata>temp_users;
		temp_users.clear();
		const char* sql = "SELECT nickname,imgData,account FROM users";//SQL�������//
		if (mysql_query(conn, sql))//���û�з��ϵĲ�ѯ��������ط���ֵ//
		{
			MessageBox(NULL, L"û�з��ϵ��ǳƺ�ͷ��", L"QQ", MB_ICONERROR);
			continue;
		}

		MYSQL_RES* res = mysql_store_result(conn);//һ�������//
		if (!res)
		{
			continue;
		}
		MYSQL_ROW  row;//ָ���ַ��������ָ��//
		unsigned long* lengths;
		while ((row = mysql_fetch_row(res)))//ȡĳһ�в�ѯ�����//
		{
			lengths = mysql_fetch_lengths(res);//ȡĳһ�в�ѯ������ȼ�//
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
			temp_users.push_back(std::move(user));//�����õ��û������ƶ���ȫ���û��б����ֹ����Ҫ�Ŀ���//
		}
		mysql_free_result(res);
		std::lock_guard<std::mutex>lock(g_usersMutex);//����//
		g_users.swap(temp_users);

		if(g_hInfoDialogbroadcast&&IsWindow(g_hInfoDialogbroadcast))//�ȴ������Ч//
		PostMessage(g_hInfoDialogbroadcast, WM_APP_UPDATEBROADCAST_MSG, 0, 0); // �Զ�����Ϣ
	}
}


//�����û�ͷ�����ݵļ����߳�
void load_user_img(SOCKET client_server,std::string str_account)
{
	//���տͻ��˼����û�ͼƬ���ݵ�����
	int request_len = 0;
	recv(client_server, (char*)&request_len, sizeof(request_len), 0);
	std::string request_str(request_len, '\0');
	recv(client_server, &request_str[0], request_len, 0);
	int wrequest_len = MultiByteToWideChar(CP_UTF8, 0, request_str.c_str(), request_str.size(), NULL, 0);
	std::wstring request_wstr(wrequest_len, 0);
	MultiByteToWideChar(CP_UTF8, 0, request_str.c_str(), request_str.size(), &request_wstr[0], wrequest_len);

	if (wcscmp(request_wstr.c_str(), L"������ص�¼�û�ͷ��") == 0)
	{
		int user_img = 0;//ͼƬ���ݵĴ�С
		BYTE* user_imgage;//����ͼƬ���ݵ�����
		MessageBox(NULL, L"�ɹ������û�ͼƬ���ݵ�����", L"QQ", MB_ICONINFORMATION);
		//�ø��û����˺������ݿ����������û���ͼƬ����
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
		send(client_server, (char*)&img_len, sizeof(img_len), 0);//�ȷ�ͼƬ���ݳ���
		send(client_server, (char*)my_data2.imgData.data(), img_len, 0);//��ͼƬ��������
		MessageBox(NULL,L"�Ѿ��ɹ����û���ͷ�����ݷ���",L"QQ",MB_ICONINFORMATION);
	}
	else
	{
		return;
	}
}

//�����ݿ�����û�������Ϣ
bool load_personal_information(std::string new_online_user_account, my_user_information &my_user_infor_edit, MYSQL* conn_2)
{
	int con_num = 0;
	while (!mysql_real_connect(conn_2, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))//�������ݿ�//
	{
		//MessageBox(NULL, L"�������ݿ�ʧ��", L"QQ", MB_ICONERROR);
		con_num++;
		if (con_num > 3)
		{
			return false;
		}
	}
	
	//MessageBox(NULL, L"�������Ѿ��ɹ��������ݿ�", L"QQ", MB_ICONINFORMATION);

	// 1. Ԥ���� SQL ��ѯ
	const char* sql = "SELECT password,nickname,gender,age,signature FROM users WHERE account=? LIMIT 1";//����һ��SQL��ѯ�����ַ�������෵��һ����¼//
	MYSQL_STMT* stmt = mysql_stmt_init(conn_2);//��ʼ��һ��Ԥ�����������conn�����ݿ����Ӿ��//
	if (!stmt)
	{
		MessageBox(NULL, L"���ݿ�Ԥ�����������ʼ��ʧ��", L"QQ", MB_ICONERROR);
		return false;
	}
	//MessageBox(NULL, L"�������Ѿ��ɹ���ʼ��һ��Ԥ���������", L"QQ", MB_ICONINFORMATION);

	if (mysql_stmt_prepare(stmt, sql, strlen(sql)))//��SQL�����ַ���ת��Ϊ���ݿ��ڲ��ṹ�����ִ��Ч��// 
	{
		MessageBox(NULL, L"SQL���ת��Ϊ���ݿ��ڲ��ṹʧ��", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}
	//MessageBox(NULL, L"�������Ѿ��ɹ���SQL���ת��Ϊ���ݿ��ڲ��ṹ", L"QQ", MB_ICONINFORMATION);

	// 2. ���������
	MYSQL_BIND bind[1];//��������Ϊ1//
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_STRING;
	bind[0].buffer =(void*)new_online_user_account.c_str();
	bind[0].buffer_length =(unsigned long)new_online_user_account.size();

	if (mysql_stmt_bind_param(stmt, bind))//�󶨲���//
	{
		MessageBox(NULL, L"�󶨲���ʧ��", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}
	//MessageBox(NULL, L"�������Ѿ��ɹ��󶨲���", L"QQ", MB_ICONINFORMATION);

	// 3. �󶨽���������������
	char* password_buf = new char[65];
	char* nickname_buf = new char[65];
	char* gender_buf = new char[9];
	char* age_buf = new char[9];
	char* signature_buf = new char[257];  // �������256

	unsigned long lens[5] = { 0 };//���ڴ���ÿ���ֶε�ʵ�ʳ���//
	bool is_null[5] = { 0 };//���ڱ�ʶÿ���ֶ��Ƿ�ΪNULL//

	// 4. �󶨽��
	MYSQL_BIND result_bind[5];//��������Ϊ1//
	memset(result_bind, 0, sizeof(result_bind));

	result_bind[0].buffer_type = MYSQL_TYPE_STRING;
	result_bind[0].buffer = password_buf;
	result_bind[0].buffer_length = 65;
	result_bind[0].is_null = &is_null[0];//��ʼ��//
	result_bind[0].length = &lens[0];//��ʼ��//

	result_bind[1].buffer_type = MYSQL_TYPE_STRING;
	result_bind[1].buffer = nickname_buf;
	result_bind[1].buffer_length = 65;
	result_bind[1].is_null = &is_null[1];//��ʼ��//
	result_bind[1].length = &lens[1];//��ʼ��//

	result_bind[2].buffer_type = MYSQL_TYPE_STRING;
	result_bind[2].buffer = gender_buf;
	result_bind[2].buffer_length = 9;
	result_bind[2].is_null = &is_null[2];//��ʼ��//
	result_bind[2].length = &lens[2];//��ʼ��//

	result_bind[3].buffer_type = MYSQL_TYPE_STRING;
	result_bind[3].buffer = age_buf;
	result_bind[3].buffer_length = 9;
	result_bind[3].is_null = &is_null[3];//��ʼ��//
	result_bind[3].length = &lens[3];//��ʼ��//

	result_bind[4].buffer_type = MYSQL_TYPE_STRING;
	result_bind[4].buffer = signature_buf;
	result_bind[4].buffer_length = 257;
	result_bind[4].is_null = &is_null[4];//��ʼ��//
	result_bind[4].length = &lens[4];//��ʼ��//

	// 3. ִ��
	if (mysql_stmt_execute(stmt))
	{
		MessageBox(NULL, L"ִ��SQL���ʧ��", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}
	//MessageBox(NULL, L"�������Ѿ�ִ����SQL���", L"QQ", MB_ICONINFORMATION);

	if (mysql_stmt_bind_result(stmt, result_bind))//�󶨽������//
	{
		MessageBox(NULL, L"�󶨽������ʧ��", L"QQ", MB_ICONERROR);
		mysql_stmt_close(stmt);
		return false;
	}
	//MessageBox(NULL, L"�������Ѿ��ɹ��󶨽������", L"QQ", MB_ICONINFORMATION);

	// 5. ��ȡ���
	bool found = false;
	if (mysql_stmt_store_result(stmt) == 0) //�ѽ�������嵽����//
	{
		//MessageBox(NULL, L"�������ɹ��ѽ�����嵽����", L"QQ", MB_ICONINFORMATION);
		if (mysql_stmt_fetch(stmt) == 0) // ��һ����¼�������ᱻ����󶨵ı���//
		{
			//MessageBox(NULL, L"�������ɹ���ѯ�����", L"QQ", MB_ICONINFORMATION);
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
		MessageBox(NULL, L"�������޷��ѽ�����嵽����", L"QQ", MB_ICONERROR);
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

void HandleClient_login(SOCKET client_server,std::string new_online_user_account)//��¼�����߳�//
{
	//�Ƚ��տͻ�����Ҫִ��ȷ������ȡ������//
	int utf8_len = 0;
	recv(client_server, (char*)&utf8_len, sizeof(utf8_len), 0);//���ճ���//
	if (utf8_len <= 0)
	{
		return;
	}
	std::string utf8_str(utf8_len, 0);//����ռ�//
	recv(client_server, &utf8_str[0], utf8_len, 0);//��������//
	int wchar_len = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), utf8_len, NULL, 0);//����ת������//
	if (wchar_len <= 0)
	{
		return;
	}
	std::wstring wide_str(wchar_len, 0);//����ռ�//
	MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), utf8_len, &wide_str[0], wchar_len);//ʵ��ת��//


	if (wcscmp(wide_str.c_str(), L"ȷ��") == 0)
	{
		//�ȼ�¼���˺ŵ���Ϣ//
		receivedData my_data;
		std::string str_account = my_recv_one(client_server);//�����˺�//
		//��ʾ���յ����˺�//
		int wchar_len_account = MultiByteToWideChar(CP_UTF8, 0, str_account.c_str(), str_account.size() + 1, NULL, 0);
		std::wstring  w_account_pro(wchar_len_account, 0);
		MultiByteToWideChar(CP_UTF8, 0, str_account.c_str(), str_account.size() + 1, &w_account_pro[0], w_account_pro.size());
		MessageBox(NULL, w_account_pro.c_str(), L"QQ", MB_ICONINFORMATION);

		std::string str_password = my_recv_one(client_server);//��������//
		//��ʾ���յ�������//
		int wchar_len_password= MultiByteToWideChar(CP_UTF8, 0, str_password.c_str(), str_password.size() + 1, NULL, 0);
		std::wstring  w_password_pro(wchar_len_password, 0);
		MultiByteToWideChar(CP_UTF8, 0, str_password.c_str(), str_password.size() + 1, &w_password_pro[0], w_password_pro.size());
		MessageBox(NULL, w_password_pro.c_str(), L"QQ", MB_ICONINFORMATION);


		MessageBox(NULL, L"�������ѳɹ�����׼����֤���˺ź�����", L"QQ", MB_ICONINFORMATION);
		//�����ݿ���ң��鿴���û����˺ź������Ƿ��Ѿ�ע��//
		if (is_account_registered(str_account.c_str(), str_password.c_str(), conn,my_data))//��ѯ�����˺������Ѿ�ע��//
		{

			//MessageBox(NULL, L"��ѯ�����˺������Ѿ�ע��", L"QQ", MB_ICONINFORMATION);
			//��¼�µĵ�¼�û��˺�
			new_online_user_account = str_account;//��'\0'

		
			

            //���û���¼Ϊ�����û�//
			User_account users_accounts;
			users_accounts.account = str_account;
			{
				std::lock_guard<std::mutex>lock(g_onlineUsersMutex);
				g_onlineUsers.push_back(users_accounts);
			}

			//�����û��б������״̬//
			{
				std::lock_guard<std::mutex> lk(users_mutex);//����
				users_update_signal = true;      // ��������
				users_cv.notify_one();           // ����һ���ȴ��߳�
			}

			//֪ͨ�����û�����״̬�Ƿ����
			{
				std::lock_guard<std::mutex>lock_users_online(users_online_mutex);//����
				users_online_update_signal = true;//��������
				users_online_cv.notify_one();//����һ���ȴ��߳�
			}

			//����socket�Ͷ�Ӧ���û��˺�//
			User_online user_online;
		    std::lock_guard<std::mutex>lock(g_users_online_Mutex);
			user_online.socket = client_server;
			user_online.account = str_account;//�˺ź�'\0'��ֹ��//
			g_users_online.push_back(user_online);//���û��˺ź�socket���浽�����û��б���//


			std::wstring recv_back = L"sucess";
			int utf8_len = WideCharToMultiByte(CP_UTF8, 0, recv_back.c_str(), recv_back.size() + 1, NULL, 0, NULL, NULL);//ת��Ϊutf8����Ŀռ�//
			std::string utf8str(utf8_len, 0);//����ռ�//
			WideCharToMultiByte(CP_UTF8, 0, recv_back.c_str(), recv_back.size() + 1, &utf8str[0], utf8_len, NULL, NULL);//ʵ��ת��//
			send(client_server, (char*)&utf8_len, sizeof(utf8_len), 0);//�ȷ�����//
			send(client_server, utf8str.c_str(), utf8_len, 0);//������//
			
			

			//�����¼��ҳ��֮����߳�//
			//MessageBox(NULL, L"���������¼����Ĵ����߳�", L"QQ", MB_ICONINFORMATION);
			std::thread(Handlelogin_pro,client_server,my_data,new_online_user_account).detach();
		}
		else
		{
			MessageBox(NULL, L"��ѯ�����˺����뻹û��ע��,�����˺ź����������Ƿ���ȷ", L"QQ", MB_ICONINFORMATION);
			std::wstring recv_back = L"faile";
			int utf8_len = WideCharToMultiByte(CP_UTF8, 0, recv_back.c_str(), recv_back.size() + 1, NULL, 0, NULL, NULL);//ת��Ϊutf8����Ŀռ�//
			std::string utf8str(utf8_len, 0);//����ռ�//
			WideCharToMultiByte(CP_UTF8, 0, recv_back.c_str(), recv_back.size() + 1, &utf8str[0], utf8_len, NULL, NULL);//ʵ��ת��//
			send(client_server, (char*)&utf8_len, sizeof(utf8_len), 0);//�ȷ�����//
			send(client_server, utf8str.c_str(), utf8_len, 0);//������//
			std::thread(HandleClient_login, client_server,new_online_user_account).detach();//��¼�����߳�//
		}
	}
	else if (wcscmp(wide_str.c_str(), L"ȡ��") == 0)
	{
		//�жϾ���ִ�������߳�,��¼��ע��//
		int recv_len = 0;
		recv(client_server, (char*)&recv_len, sizeof(recv_len), 0);//������Ϣ����//
		if (recv_len <= 0)
		{
			MessageBox(NULL, L"�����߳����͵���Ϣ����ʧ��", L"QQ", MB_ICONERROR);
			return;
		}
		MessageBox(NULL, L"�����߳����͵���Ϣ���ȳɹ�", L"QQ", MB_ICONINFORMATION);
		std::string recvchar(recv_len, 0);
		int r = 0;
		r = recv(client_server, &recvchar[0], recv_len, 0);
		{
			if (r <= 0)
			{
				MessageBox(NULL, L"�����߳����͵���Ϣʧ��", L"QQ", MB_ICONERROR);
				return;
			}
		}
		MessageBox(NULL, L"�����߳����͵���Ϣ�ɹ�", L"QQ", MB_ICONINFORMATION);
		int wlen = MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, NULL, 0);//ת��Ϊ���ֽ�����Ҫ���ֽ���//
		std::wstring wrecvchar(wlen, 0);//����ռ�//
		MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, &wrecvchar[0], wlen);//ʵ��ת��//
		//ע�ᴦ��//
		if (wcscmp(wrecvchar.c_str(), L"ע��") == 0)//ע���߳�//
		{
			MessageBox(NULL, L"���յ�ע����Ϣ����������ע���߳�", L"QQ", MB_ICONINFORMATION);
			std::thread(HandleClient_register, client_server).detach();//����ע���߳�//
		}
		//��¼����//
		else if (wcscmp(wrecvchar.c_str(), L"��¼") == 0)//��¼�߳�//
		{
			MessageBox(NULL, L"���յ���¼��Ϣ���������õ�¼�߳�", L"QQ", MB_ICONINFORMATION);
			std::thread(HandleClient_login, client_server,new_online_user_account).detach();//������¼�߳�//
		}
		else
		{
			MessageBox(NULL, L"���ղ����߳���Ϣ�������˳�", L"QQ", MB_ICONINFORMATION);
			return;
		}
	}
}

std::wstring strTowstr(std::string str)
{
	int wlen=MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.size() + 1, NULL, 0);//����ת���ֽ���//
	if (wlen <= 0)
	{
		MessageBox(NULL,L"���ݿ��и��ֶ��б�Ϊ��",L"QQ",MB_ICONERROR);
		return L"";
	}
	std::wstring w_str(wlen, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.size() + 1,&w_str[0], wlen);//ʵ��ת��//
	return w_str;
}


// ����������˺š�ͼƬ���ݺͳ���
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
	// ͷ��BLOB
	bind[0].buffer_type = MYSQL_TYPE_LONG_BLOB;
	bind[0].buffer = (void*)imgData;
	bind[0].buffer_length = imgLen;
	// �˺�
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
	//�����ݿ�����û���Ϣ
	my_user_information my_user_infor_edit;
	std::string account_new = new_online_user_account;
	if (!account_new.empty() && account_new.back() == '\0')
	{
		account_new.pop_back();
	}
	//�����صĸ�����Ϣ���͸��ͻ���
	MYSQL* conn_2 = mysql_init(NULL);
	if (load_personal_information(account_new, my_user_infor_edit, conn_2))
	{
		mysql_close(conn_2);
		//�ṹ���Ѿ��ɹ������û��ĸ�����Ϣ
		MessageBox(NULL, L"�������Ѿ��ɹ��������и�����Ϣ", L"QQ", MB_ICONINFORMATION);
		//��������
		int password_buf_len = my_user_infor_edit.password.size();
		send(client_server, (char*)&password_buf_len, sizeof(password_buf_len), 0);
		send(client_server, my_user_infor_edit.password.c_str(), password_buf_len, 0);//����'\0'

		//�����ǳ�
		int nickname_buf_len = my_user_infor_edit.nickname.size();
		send(client_server, (char*)&nickname_buf_len, sizeof(nickname_buf_len), 0);
		send(client_server, my_user_infor_edit.nickname.c_str(), nickname_buf_len, 0);//����'\0'

		//�����Ա�
		int gender_buf_len = my_user_infor_edit.gender.size();
		send(client_server, (char*)&gender_buf_len, sizeof(gender_buf_len), 0);
		send(client_server, my_user_infor_edit.gender.c_str(), gender_buf_len, 0);//����'\0'

		//��������
		int age_buf_len = my_user_infor_edit.age.size();
		send(client_server, (char*)&age_buf_len, sizeof(age_buf_len), 0);
		send(client_server, my_user_infor_edit.age.c_str(), age_buf_len, 0);//����'\0'

		//���͸���ǩ��
		int signature_buf_len = my_user_infor_edit.signature.size();
		send(client_server, (char*)&signature_buf_len, sizeof(signature_buf_len), 0);
		send(client_server, my_user_infor_edit.signature.c_str(), signature_buf_len, 0);//����'\0'

		MessageBox(NULL, L"�ѳɹ����û�������Ϣ�����ͻ���", L"QQ", MB_ICONINFORMATION);
	}
	else
	{
		mysql_close(conn_2);
		MessageBox(NULL, L"�޷����û�������Ϣ�����ͻ���", L"QQ", MB_ICONERROR);
		return;
	}

}


int recv_all_information(SOCKET socket,std::string &user_infor)
{
	int buf_len = 0;
	recv(socket,(char*)&buf_len,sizeof(buf_len),0);//�Ƚ��ճ���
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
			user_infor = str;//�����Ϣ
			return sum;
		}

	}
}


// �����û���Ϣ
bool update_user_information(const std::string& account,MYSQL* conn_4, my_user_information& new_information)
{
	//�������ݿ�
	int con_num = 0;
	while (!mysql_real_connect(conn_4, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))//�������ݿ�//
	{
		MessageBox(NULL, L"�������ݿ�ʧ��", L"QQ", MB_ICONERROR);
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

	MYSQL_BIND bind[6]; // 6������
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

	// 6. account (WHERE����)
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


//��¼���洦���߳�//
void Handlelogin_pro(SOCKET client_server,receivedData my_data,std::string new_online_user_account)//��¼���洦���߳�//
{
	MessageBox(NULL, L"�ɹ������¼����Ĵ����߳�", L"QQ", MB_ICONINFORMATION);
	 
	//�����û�ͷ�������߳�
	std::string str_account = new_online_user_account;
	//std::thread(load_user_img, client_server, str_account).detach();
	load_user_img(client_server, str_account);

	int len = 0;
	recvAll(client_server, (char*)&len, sizeof(len), 0);//�Ƚ��ճ���//

	MessageBox(NULL, L"���յ�¼����ͻ��˷��͵ĳ�����Ϣ�ɹ�", L"QQ", MB_ICONINFORMATION);
	if (len <= 0)
	{
		MessageBox(NULL, L"���յ�¼���水ť��Ϣ����ʧ��", L"QQ", MB_ICONERROR);
		return;
	}
	std::string utf8str(len, 0);//����ռ�//
	recvAll(client_server, &utf8str[0], len, 0);//��������//
	MessageBox(NULL, L"���յ�¼����ͻ��˷��͵İ�ť������Ϣ�ɹ�", L"QQ", MB_ICONINFORMATION);
	int wlen = MultiByteToWideChar(CP_UTF8,0,utf8str.c_str(),len,NULL,0);//�����ת������//
	std::wstring wstr(wlen, 0);//����ռ�//
	MultiByteToWideChar(CP_UTF8, 0, utf8str.c_str(), len,&wstr[0],wlen);//ʵ��ת��//
	//���н��������ж�//
	if (wcscmp(wstr.c_str(),L"������")==0)
	{

		//���տͻ��˷���������ʱ�İ�ť��Ϣ//
		while (true)
		{
		
			int len_utf8 = 0;
			recvAll(client_server, (char*)&len_utf8, sizeof(len_utf8), 0);//�Ƚ��ճ���//
			//MessageBox(NULL, L"���յ�¼����ͻ��˷��͵ĳ�����Ϣ�ɹ�", L"QQ", MB_ICONINFORMATION);
			if (len_utf8 <= 0)
			{
				MessageBox(NULL, L"���յ�¼���水ť��Ϣ����ʧ��", L"QQ", MB_ICONERROR);
				return;
			}

			std::string str_utf8(len_utf8, 0);//����ռ�//
			recvAll(client_server, &str_utf8[0], len_utf8, 0);//��������//
			//MessageBox(NULL, L"���յ�¼����ͻ��˷��͵İ�ť������Ϣ�ɹ�", L"QQ", MB_ICONINFORMATION);
			int wchar_len = MultiByteToWideChar(CP_UTF8, 0, str_utf8.c_str(), len_utf8, NULL, 0);//�����ת������//
			std::wstring wchar_str(wchar_len, 0);//����ռ�//
			MultiByteToWideChar(CP_UTF8, 0, str_utf8.c_str(), len_utf8, &wchar_str[0], wchar_len);//ʵ��ת��//
			if (wcscmp(wchar_str.c_str(), L"�˳�") == 0)
			{
				MessageBox(NULL, L"��������������ĶԻ��̷߳��ص�½���洦���߳�", L"QQ", MB_ICONINFORMATION);
				break;
			}
			else if (wcscmp(wchar_str.c_str(), L"����") == 0)
			{
				//MessageBox(NULL, L"�ɹ������¼����ķ����������߳�", L"QQ", MB_ICONINFORMATION);
				//���տͻ��˷�������Ϣ//
				int str_len = 0;
				recvAll(client_server, (char*)&str_len, sizeof(str_len), 0);//�Ƚ��ճ���//

				//������ת��Ϊ���ַ���//
				//std::wstring n = std::to_wstring(str_len);
				wchar_t nbuf[32];
				swprintf_s(nbuf, _countof(nbuf), L"%d", str_len);
				std::wstring n = nbuf;
				//MessageBox(NULL, n.c_str(), L"QQ", MB_ICONINFORMATION);

				if (str_len <= 0)
				{
					MessageBox(NULL, L"���յ�¼����ͻ��˷��͵���Ϣ����ʧ��", L"QQ", MB_ICONERROR);
					return;
				}
				std::string str_char(str_len, 0);//����ռ�//

				recvAll(client_server, &str_char[0], str_len, 0);//��������//
				int w_len = MultiByteToWideChar(CP_UTF8, 0, str_char.c_str(), str_len, NULL, 0);//�����ת������//
				std::wstring w_str(w_len, 0);//����ռ�//
				MultiByteToWideChar(CP_UTF8, 0, str_char.c_str(), str_len, &w_str[0], w_len);//ʵ��ת��//
				//MessageBox(NULL, L"���յ�¼����ͻ��˷��͵ĶԻ�����Ϣ�ɹ�", L"QQ", MB_ICONINFORMATION);

				//MessageBox(NULL, w_str.c_str(), L"QQ", MB_ICONINFORMATION);
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
				std::wstring w = strTowstr(my_data.nickname);
				if (!w.empty() && w.back() == L'\0')//ȥ����ʶ�ַ�������ֹ��'\0',�ſ���ƴ�Ӳ���ʾ���������//
				{
					w.pop_back();
				}
				//MessageBox(NULL, L"�Ѿ��ɹ����ǳ�ĩβ�ġ�0������", L"QQ", MB_ICONINFORMATION);
				msg_one += w.c_str();
				//MessageBox(NULL, L"�Ѿ��ɹ�ƴ���ǳ�", L"QQ", MB_ICONINFORMATION);
				msg_one += L"->";
				//MessageBox(NULL, L"�Ѿ��ɹ�ƴ�Ӻ�->", L"QQ", MB_ICONINFORMATION);
				msg_one += L"������";
				msg_one += L"]";
				//MessageBox(NULL, L"�Ѿ��ɹ�ƴ�Ӻ÷������ֶ�", L"QQ", MB_ICONINFORMATION);
				msg_one += w_str.c_str();
				//MessageBox(NULL, w_str.c_str(), L"QQ", MB_ICONINFORMATION);
				//MessageBox(NULL, msg_one.c_str(), L"QQ", MB_ICONINFORMATION);
				//MessageBox(NULL, L"�Ѿ��ɹ�ƴ�Ӻÿͻ��˷�������Ϣ�ֶ�", L"QQ", MB_ICONINFORMATION);
				//MessageBox(NULL, L"����׼������Ϣ�������", L"QQ", MB_ICONINFORMATION);

				//�����յ������ݴ�����ٷ���ȥ//
				int u_len = WideCharToMultiByte(CP_UTF8, 0, msg_one.c_str(), msg_one.size() + 1, NULL, 0, NULL, NULL);//����ת������//
				//std::wstring num = std::to_wstring(u_len);//������ת��Ϊ���ַ���//
				//MessageBox(NULL,num.c_str(), L"QQ", MB_ICONINFORMATION);
				std::string u_str(u_len, 0);//����ռ�//
				WideCharToMultiByte(CP_UTF8, 0, msg_one.c_str(), msg_one.size() + 1, &u_str[0], u_len, NULL, NULL);//ʵ��ת��//

				send(client_server, (char*)&u_len, sizeof(u_len), 0);//�ȷ�����//
				send(client_server, u_str.c_str(), u_len, 0);//������//

				EnterCriticalSection(&g_csMsg_two_Queue);//����//
				g_msg_two_Queue.push(msg_one);//��������//
				LeaveCriticalSection(&g_csMsg_two_Queue);//����//
				//MessageBox(NULL, L"�Ѿ��ɹ�����Ϣƴ��", L"QQ", MB_ICONINFORMATION);
				// ��¼�������������//
				PostMessage(g_hInfoDialogmonitor, WM_APP_UPDATEMONITOR_MSG, 0, 0);
				//MessageBox(NULL, L"�Ѿ��ɹ�֪ͨ��Ϣ�ļ����ʾ", L"QQ", MB_ICONINFORMATION);

			}
			else if (wcscmp(wchar_str.c_str(), L"�鿴�㲥") == 0)
			{
				//�����û��˺�
				int account_user_len = 0;
				recv(client_server,(char*)&account_user_len,sizeof(account_user_len),0);
				std::string account_user_str(account_user_len,'\0');
				recv(client_server,&account_user_str[0],account_user_len,0);//�˺Ų���'\0'
				char buf2[256];
				sprintf_s(buf2, "account_user_str: [%s] len=%d\n", account_user_str.c_str(), (int)account_user_str.size());
				OutputDebugStringA(buf2);
				std::string s_nickname;
				//ƥ���û��ǳ�
				for (auto it=g_users.begin();it!=g_users.end();)
				{
					if (strcmp(it->account.c_str(), account_user_str.c_str()) == 0)
					{
						s_nickname = it->nickname.c_str();//������\0'
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

				//ƥ��g_offline_users_and_information��Ķ�Ӧ�û��˺Ŵ洢����Ϣ
				
				int infor_sum = 0;
			
				//ͳ��һ���������㲥
				for (auto it = g_offline_users_and_information.begin(); it != g_offline_users_and_information.end();)//������������Ϣ���������������Ϣ����Ϣ����˳����ȷ
				{
					std::string acc1 = it->account;
					std::string acc2 = account_user_str;
					while (!acc1.empty() && acc1.back() == '\0') acc1.pop_back();
					while (!acc2.empty() && acc2.back() == '\0') acc2.pop_back();

					// ������ݺͳ���
					char buf[256];
					sprintf_s(buf, "acc1: [%s] len=%d, acc2: [%s] len=%d\n", acc1.c_str(), (int)acc1.size(), acc2.c_str(), (int)acc2.size());
					OutputDebugStringA(buf);

					if (strcmp(acc1.c_str(),acc2.c_str())==0)//ƥ��g_offline_users_and_information��Ķ�Ӧ�û��˺Ŵ洢����Ϣ 
					{
						infor_sum++;
					}
					it++;
				
				}

				if (infor_sum == 0)
				{
					MessageBox(NULL, L"���û�û��δ���յĹ㲥��Ϣ", L"QQ", MB_ICONERROR);
				}
				//�ȷ��͹㲥����
				send(client_server,(char*)&infor_sum,sizeof(infor_sum),0);
				
				if (g_offline_users_and_information.empty())
				{
					MessageBox(NULL, L"������û��δ�����Ĺ㲥��Ϣ", L"QQ", MB_ICONERROR);
					continue;
				}

				if (infor_sum > 0)//��Ҫ���͹㲥��Ϣ
				{
					for (auto it = g_offline_users_and_information.begin(); it != g_offline_users_and_information.end();)//������������Ϣ���������������Ϣ����Ϣ����˳����ȷ
					{
						if (it->account == account_user_str)//ƥ��g_offline_users_and_information��Ķ�Ӧ�û��˺Ŵ洢����Ϣ 
						{

									// ������Ϣ
									int l = it->information.size();
									send(client_server,(char*)&l,sizeof(l),0);
									send(client_server, it->information.c_str(), it->information.size(), 0);//������ǰ���û��洢�Ĺ㲥��Ϣ����'\0'
									
									EnterCriticalSection(&g_csMsg_two_Queue);//����//
									int w_str_len = 0;
									w_str_len = MultiByteToWideChar(CP_UTF8,0,it->information.c_str(),it->information.size(),NULL,0);
									std::wstring w_str_s(w_str_len,L'\0');
									MultiByteToWideChar(CP_UTF8, 0, it->information.c_str(), it->information.size(),&w_str_s[0], w_str_len); 

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
									g_msg_two_Queue.push(msg_one);//��������//
									LeaveCriticalSection(&g_csMsg_two_Queue);//����//
									PostMessage(g_hInfoDialogmonitor, WM_APP_UPDATEMONITOR_MSG, 0, 0);//֪ͨ��ؿ���ʾ������Ϣ

								
							// ɾ��������¼�������µ�����
							std::lock_guard<std::mutex> lock_broadcast_account(g_users_account_and_information_sel_broadcast_Mutex);//��ȷ���̰߳�ȫ
							it = g_offline_users_and_information.erase(it);//����ɾ��Ԫ�ص���һ��
						}
						else
						{
							++it;
						}
					}
				}
	
			}
	      


		}
		//�����˳���Ϣ���˳�while ѭ��
		std::thread(Handlelogin_pro, client_server, my_data,new_online_user_account).detach();//���ص�¼�׽��洦���߳�//
	}
	else if (wcscmp(wstr.c_str(),L"������")==0)
	{
	door_1:
		//���տͻ��˼���������ԭ�е�������Ϣ������
		int r_len = 0;
		recv(client_server, (char*)&r_len, sizeof(r_len), 0);
		std::string r_str(r_len,'\0');
		recv(client_server,&r_str[0],r_len,0);

		//OutputDebugStringA(("r_str is"+r_str+"\n").c_str());
		//�ȽϽ��յ��ַ����Ƿ���ȷ
		if (strcmp(r_str.c_str(),"������������ҵ���Ϣ") != 0)
		{
			//����ʧ��
			MessageBox(NULL, L"������������������Ϣ������ʧ��", L"QQ", MB_ICONERROR);
			return;
		}
		MessageBox(NULL, L"������������������Ϣ������ɹ�", L"QQ", MB_ICONINFORMATION);
		//�ɹ����ռ�������
		//std::lock_guard<std::mutex>lock(users_anii_on_chartroom_mutex);//����
		//�ȷ���Ϣ������
		int num = 0;
		for (auto it = users_account_on_chartroom.begin(); it != users_account_on_chartroom.end();)
		{
			num++;
			it++;
		}
		send(client_server, (char*)&num, sizeof(num), 0);
		MessageBox(NULL, L"�������ɹ���ͻ��˷�����Ϣ������", L"QQ", MB_ICONINFORMATION);

		if(num>0)
		{
			for (auto it = users_account_on_chartroom.begin(); it != users_account_on_chartroom.end();)
			{
				//�����û�ͷ��
				const BYTE* data = it->user_on_chartroom_image.data();
				int img_len = it->user_on_chartroom_image.size();

				int r = 0;
				int sum = 0;

				//������Ƭ����
				send(client_server, (char*)&img_len, sizeof(img_len), 0);
				//������Ƭ����
				while (sum < img_len)
				{
					r = send(client_server, (const char*)data + sum, img_len - sum, 0);
					if (r > 0)
					{
						sum += r;
					}
				}

				//�����û��ǳ�
				int nickname_len = 0;
				nickname_len = it->user_on_chartroom_nickname.size();
				send(client_server, (char*)&nickname_len, sizeof(nickname_len), 0);
				send(client_server, it->user_on_chartroom_nickname.c_str(), nickname_len, 0);

				//�����û���Ϣ
				int inf_len = 0;
				inf_len = it->user_on_chartroom_inf.size();
				send(client_server, (char*)&inf_len, sizeof(inf_len), 0);
				send(client_server, it->user_on_chartroom_inf.c_str(), inf_len, 0);

				OutputDebugStringA(("the information user send in chartroom is" + it->user_on_chartroom_inf + "\n").c_str());
				it++;
			}
			MessageBox(NULL, L"�������ɹ���ͻ��˷��������ҵ������¼", L"QQ", MB_ICONINFORMATION);
		}

		int si = 1;
		//�жϽ��յ�����ʲô��Ϣ
		while (si)
		{
			//���տͻ��˷��ͻ��˳���ť
			int inf_len = 0;
			recv(client_server, (char*)&inf_len, sizeof(inf_len), 0);
			std::string inf_str(inf_len, '\0');
			recv(client_server, &inf_str[0],inf_len, 0);

			if (strcmp(inf_str.c_str(), "����") == 0)
			{
				std::wstring w_nickname;
				std::wstring w_information;

				MessageBox(NULL, L"�Ѿ��ɹ��������Կͻ��˵������ҵ���Ϣ�����͡���ť", L"QQ", MB_ICONINFORMATION);
				//�����û������ı�����
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
				
				MessageBox(NULL, L"�Ѿ��ɹ����տͻ��˵�������Ϣ", L"QQ", MB_ICONINFORMATION);
			   //�����Ѽ�¼��������������Ϣ���û������ҿ򣬰����û�ͷ���û��ǳƺ��û���������Ϣ

		      //��Ҫ�����û��˺Ų�ѯ�û�ͷ����ǳ�
				users_anii_on_chartroom user_i;
				user_i = users_anii_on_chartroom();
				//�����������ҵ��û��˺Ŵ洢
				user_i.user_on_chartroom_account = new_online_user_account;
				if (!user_i.user_on_chartroom_account.empty() && user_i.user_on_chartroom_account.back() == '\0')
				{
					user_i.user_on_chartroom_account.pop_back();
				}
				//���û��ڱ༭�ҷ�����Ϣ�洢
				user_i.user_on_chartroom_inf = text_str;//��'\0'
				
				//ת��Ϊ���ַ���
			    int w_lx= 0;
				w_lx=MultiByteToWideChar(CP_UTF8, 0, user_i.user_on_chartroom_inf.c_str(), user_i.user_on_chartroom_inf.size(), NULL, 0);
				w_information.resize(w_lx);
				MultiByteToWideChar(CP_UTF8, 0, user_i.user_on_chartroom_inf.c_str(), user_i.user_on_chartroom_inf.size(), &w_information[0], w_lx);

				//�����û����ǳƺ�ͷ�񲢴���
				{
					//std::vector  <users_anii_on_chartroom> temp;//��ʱ�洢�û���Ϣ

					MYSQL* conn_6 = mysql_init(NULL);
					int con_num = 0;
					while (!mysql_real_connect(conn_6, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))//�������ݿ�//
					{
						MessageBox(NULL, L"�������ݿ�ʧ��", L"QQ", MB_ICONERROR);
						con_num++;
						if (con_num > 3)
						{
							mysql_close(conn_6);
							return;
						}
					}

					//SQL�������//
					std::string sql = "SELECT nickname,imgData FROM users WHERE account='" + user_i.user_on_chartroom_account + "' LIMIT 1";
					if (mysql_query(conn_6, sql.c_str()))//���û�з��ϵĲ�ѯ��������ط���ֵ//
					{
						MessageBox(NULL, L"û�з��ϵ��ǳƺ�ͷ��", L"QQ", MB_ICONERROR);
						return;
					}

					MYSQL_RES* res = mysql_store_result(conn_6);//һ�������//
					if (!res)
					{
						return;
					}
					MYSQL_ROW  row;//ָ���ַ��������ָ��//
					unsigned long* lengths;
					row = mysql_fetch_row(res);//ȡĳһ�в�ѯ�����//
					{
						lengths = mysql_fetch_lengths(res);//ȡĳһ�в�ѯ������ȼ�//

						if (row[0])
						{
							user_i.user_on_chartroom_nickname = row[0];
							//����ת��Ϊ���ַ�
							int w_l = 0;
							w_l = MultiByteToWideChar(CP_UTF8,0,user_i.user_on_chartroom_nickname.c_str(),user_i.user_on_chartroom_nickname.size(),NULL,0);
							w_nickname.resize(w_l);
							MultiByteToWideChar(CP_UTF8, 0, user_i.user_on_chartroom_nickname.c_str(), user_i.user_on_chartroom_nickname.size(), &w_nickname[0], w_l);
						}
						if (row[1] && lengths[1] > 0)
						{
							user_i.user_on_chartroom_image.assign((BYTE*)row[1], (BYTE*)row[1] + lengths[1]);
						}

						//temp.push_back(std::move(user_i));//�����õ��û������ƶ���ȫ���û��б����ֹ����Ҫ�Ŀ���
						//std::lock_guard<std::mutex>lock(users_anii_on_chartroom_mutex);//����
						users_account_on_chartroom.push_back(std::move(user_i));//�����õ��û������ƶ���ȫ���û��б����ֹ����Ҫ�Ŀ���
					}
					mysql_free_result(res);
					//std::lock_guard<std::mutex>lock(users_anii_on_chartroom_mutex);//����
					//users_account_on_chartroom.swap(temp);
					mysql_close(conn_6);

					MessageBox(NULL, L"�������Ѿ��ɹ�������û���ͷ���ǳƺ���������Ϣ", L"QQ", MB_ICONINFORMATION);
					
					//֪ͨ�ͻ��˸��������Ҹ���������Ϣ
					std::string no_utf8_inf = "�������������Ϣ";
					int no_utf8_string_len = no_utf8_inf.size();
					send(client_server, (char*)&no_utf8_string_len, sizeof(no_utf8_string_len), 0);
					send(client_server, no_utf8_inf.c_str(), no_utf8_inf.size(), 0);


					//��������Ϣ��ʾ�ڷ�������ؿ�
					//MessageBox(NULL, w_str.c_str(), L"QQ", MB_ICONINFORMATION);
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
					
					//MessageBox(NULL, L"�Ѿ��ɹ����ǳ�ĩβ�ġ�0������", L"QQ", MB_ICONINFORMATION);
					msg_one += w_nickname;
					//MessageBox(NULL, L"�Ѿ��ɹ�ƴ���ǳ�", L"QQ", MB_ICONINFORMATION);
					msg_one += L"->";
					//MessageBox(NULL, L"�Ѿ��ɹ�ƴ�Ӻ�->", L"QQ", MB_ICONINFORMATION);
					msg_one += L"������";
					msg_one += L"]";
					//MessageBox(NULL, L"�Ѿ��ɹ�ƴ�Ӻ÷������ֶ�", L"QQ", MB_ICONINFORMATION);
					msg_one += w_information;
					
					EnterCriticalSection(&g_csMsg_two_Queue);//����//
					g_msg_two_Queue.push(msg_one);//��������//
					LeaveCriticalSection(&g_csMsg_two_Queue);//����//
					//MessageBox(NULL, L"�Ѿ��ɹ�����Ϣƴ��", L"QQ", MB_ICONINFORMATION);
					// ��¼�������������//
					PostMessage(g_hInfoDialogmonitor, WM_APP_UPDATEMONITOR_MSG, 0, 0);
					//MessageBox(NULL, L"�Ѿ��ɹ�֪ͨ��Ϣ�ļ����ʾ", L"QQ", MB_ICONINFORMATION);

					MessageBox(NULL, L"�������Ѿ��ɹ���ͻ��˷��͸�����������Ϣ��֪ͨ", L"QQ", MB_ICONINFORMATION);
					goto door_1;
				}

			}
			else if (strcmp(inf_str.c_str(), "������Ϣ") == 0)
			{
				//���տͻ��˼���������ԭ�е�������Ϣ������
				int r_len = 0;
				recv(client_server, (char*)&r_len, sizeof(r_len), 0);
				std::string r_str(r_len, '\0');
				recv(client_server, &r_str[0], r_len, 0);

				//OutputDebugStringA(("r_str is"+r_str+"\n").c_str());
				//�ȽϽ��յ��ַ����Ƿ���ȷ
				if (strcmp(r_str.c_str(), "������������ҵ���Ϣ") != 0)
				{
					//����ʧ��
					MessageBox(NULL, L"������������������Ϣ������ʧ��", L"QQ", MB_ICONERROR);
					return;
				}
				MessageBox(NULL, L"������������������Ϣ������ɹ�", L"QQ", MB_ICONINFORMATION);
				//�ɹ����ռ�������
				//std::lock_guard<std::mutex>lock(users_anii_on_chartroom_mutex);//����
				//�ȷ���Ϣ������
				int num = 0;
				for (auto it = users_account_on_chartroom.begin(); it != users_account_on_chartroom.end();)
				{
					num++;
					it++;
				}
				send(client_server, (char*)&num, sizeof(num), 0);
				MessageBox(NULL, L"�������ɹ���ͻ��˷�����Ϣ������", L"QQ", MB_ICONINFORMATION);

				if (num > 0)
				{
					for (auto it = users_account_on_chartroom.begin(); it != users_account_on_chartroom.end();)
					{
						//�����û�ͷ��
						const BYTE* data = it->user_on_chartroom_image.data();
						int img_len = it->user_on_chartroom_image.size();

						int r = 0;
						int sum = 0;

						//������Ƭ����
						send(client_server, (char*)&img_len, sizeof(img_len), 0);
						//������Ƭ����
						while (sum < img_len)
						{
							r = send(client_server, (const char*)data + sum, img_len - sum, 0);
							if (r > 0)
							{
								sum += r;
							}
						}

						//�����û��ǳ�
						int nickname_len = 0;
						nickname_len = it->user_on_chartroom_nickname.size();
						send(client_server, (char*)&nickname_len, sizeof(nickname_len), 0);
						send(client_server, it->user_on_chartroom_nickname.c_str(), nickname_len, 0);

						//�����û���Ϣ
						int inf_len = 0;
						inf_len = it->user_on_chartroom_inf.size();
						send(client_server, (char*)&inf_len, sizeof(inf_len), 0);
						send(client_server, it->user_on_chartroom_inf.c_str(), inf_len, 0);

						OutputDebugStringA(("the information user send in chartroom is" + it->user_on_chartroom_inf + "\n").c_str());
						it++;
					}
					MessageBox(NULL, L"�������ɹ���ͻ��˷��������ҵ������¼", L"QQ", MB_ICONINFORMATION);
				}
			}
			else if (strcmp(inf_str.c_str(), "�˳�") == 0)
			{
				si = 0;//�˳���Ϣ����ѭ��
				MessageBox(NULL, L"�Ѿ��ɹ��������Կͻ��˵������ҵ���Ϣ���˳�����ť", L"QQ", MB_ICONINFORMATION);

				std::string account = new_online_user_account;
				if (!account.empty() && account.back() == '\0')
				{
					account.pop_back();
				}

				//�����û��˺ŵ�������
				//std::lock_guard<std::mutex>lock(users_anii_on_chartroom_mutex);//����//
				auto it = std::find_if(users_account_on_chartroom.begin(), users_account_on_chartroom.end(), [&account](const users_anii_on_chartroom& user) {return user.user_on_chartroom_account == account; });//�����ҳɹ������ص�һ��ָ��recv_user_account_str��ָ��
				if (it != users_account_on_chartroom.end()) 
				{
					users_account_on_chartroom.erase(it);
				}
				break;
			}
			else
			{
				si = 0;
				MessageBox(NULL, L"�޷��������Կͻ��˵������ҵ���Ϣ����ť", L"QQ", MB_ICONERROR);
				return;
			}
		}
		
		//�˳��󽫽����¼������ҳ�߳�
		std::thread(Handlelogin_pro,client_server,my_data,new_online_user_account).detach();

	}
	else if (wcscmp(wstr.c_str(),L"����")==0)
	{
		door_102:
		{
			//�ͻ��˽�����ѹ���
			//���ռ��ؿͻ����û������б������
			int msg_len = 0;
			recv(client_server, (char*)&msg_len, sizeof(msg_len), 0);
			std::string msg(msg_len, '\0');
			recv(client_server, &msg[0], msg_len, 0);
			//�ȽϽ����ַ�����
			if (strcmp(msg.c_str(), "������غ����б�") != 0)
			{
				std::string msg_x = "failure";
				int msg_x_len = msg_x.size();
				send(client_server, (char*)&msg_x_len, sizeof(msg_x_len), 0);
				send(client_server, msg_x.c_str(), msg_x.size(), 0);
				//���ص�¼�׽��洦���߳�
				std::thread(Handlelogin_pro, client_server, my_data, new_online_user_account).detach();//���ص�¼�׽��洦���߳�
			}
			std::string msg_x = "success";
			int msg_x_len = msg_x.size();
			send(client_server, (char*)&msg_x_len, sizeof(msg_x_len), 0);
			send(client_server, msg_x.c_str(), msg_x.size(), 0);


			//�����û��˺ż����û��ĺ����б�new_online_user_account����\0'
			std::string user_account = new_online_user_account;
			if (!user_account.empty() && user_account.back() == '\0')
			{
				user_account.pop_back();
			}
			char *buf=new char[50]();
			snprintf(buf,50, "the user_account is %s\n",user_account.c_str());
			OutputDebugStringA(buf);
			delete[]buf;

			std::vector <std::string>user_friends_account;//�����˺��б����
			user_friends_account.clear();



			//�����û��˺Ų�ѯ����û��˺�ӵ�е�������
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

			MYSQL_BIND bind[1]; // 1������
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

			//��������
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


			//�������Ѿ��ɹ������û������˻��б�
			//����Ҫ���ݺ����˺ż��غ��ѵ��ǳƺ�ͷ��

			//��ͻ��˷��ͺ��Ѹ���
			send(client_server, (char*)&total_count, sizeof(total_count), 0);
			char *buf_2=new char[50]();
			snprintf(buf_2,50, "the total_count is %d\n", total_count);
			OutputDebugStringA(buf_2);
			delete[]buf_2;


			int* online_user_signal = new int[total_count]();//�����û����߱�־
			int k = 0;


			MessageBox(NULL, L"�Ѿ��ɹ���ͻ��˷��ͺ��Ѹ�����Ϣ", L"QQ",MB_ICONINFORMATION);

			if (total_count > 0 && !user_friends_account.empty())
			{
				std::vector <std::string>user_friends_nickname;//�����ǳ��б����
				std::vector< std::vector <BYTE>>user_friends_image;//����ͷ���б�

				user_friends_nickname.clear();
				user_friends_image.clear();

				//�����û��˺Ų�ѯ����û��˺�ӵ�е�������
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


				//���������˺��б�
				for (auto it = user_friends_account.begin(); it != user_friends_account.end();)
				{
					//��ѯ�����Ƿ�����
					
					std::lock_guard<std::mutex>lok(g_onlineUsersMutex);//����

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

					MYSQL_BIND bind[1]; // 1������
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


					//��������
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
							//�������ͷ���б�
							std::vector<BYTE>one_image(friend_image, friend_image + friend_image_len);
							//ѹ����Ѷ���
							user_friends_image.push_back(std::move(one_image));
						}
						else
						{
							std::vector<BYTE>one_image(friend_image, friend_image + 3 * 1024 * 1024);
							//ѹ����Ѷ���
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

				//MessageBox(NULL,L"�������Ѿ��ɹ������û��ĺ����б�",L"QQ",MB_ICONINFORMATION);

				//���洢���û������˺š��ǳƺ�ͷ�����ݷ��͸��ͻ���
				int i = 0;
				while (i < total_count)
				{
					std::string account_x = user_friends_account[i];//�����˺��б����
					std::string nickname_x = user_friends_nickname[i];//�����ǳ��б����
					std::vector<BYTE>image_x = user_friends_image[i];//����ͷ���б�
					int signal = *(online_user_signal + i);
					
					//���ͺ������߱�־
					send(client_server, (char*)&signal, sizeof(signal), 0);

					//���ͺ����˺�
					int account_x_len = account_x.size();
					send(client_server, (char*)&account_x_len, sizeof(account_x_len), 0);
					send(client_server, account_x.c_str(), account_x_len, 0);

					char* buf_4 = new char[50]();
					snprintf(buf_4, 50, "the send_account is %s\n",account_x.c_str());
					OutputDebugStringA(buf_4);
					delete[]buf_4;


					//���ͺ����ǳ�
					int nickname_x_len = nickname_x.size();
					send(client_server, (char*)&nickname_x_len, sizeof(nickname_x_len), 0);
					send(client_server, nickname_x.c_str(), nickname_x_len, 0);

					char* buf_5= new char[50]();
					snprintf(buf_5, 50, "the send_nickname is %s\n", nickname_x.c_str());
					OutputDebugStringA(buf_5);
					delete[]buf_5;


					//���ͺ���ͷ��
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
				MessageBox(NULL, L"�������Ѿ����û��ĺ����б���Ϣȫ������", L"QQ", MB_ICONINFORMATION);
			}


			//�жϿͻ�����ѡ��Ĳ���

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

				if (strcmp(oper_str.c_str(), "�˳�") == 0)
				{
					//���ص�¼�׽��洦���߳�
					std::thread(Handlelogin_pro, client_server, my_data, new_online_user_account).detach();//���ص�¼�׽��洦���߳�
					return;
				}
				else if (strcmp(oper_str.c_str(), "ˢ���б�") == 0)
				{
					//���¼��غ����б�
					goto door_102;
				}
				else if (strcmp(oper_str.c_str(), "����") == 0)
				{
					
					users_chart_information ix;

					//��Ϣ���ͷ����˺�
					std::string account_sender = new_online_user_account;
					if (!account_sender.empty() && account_sender.back() == '\0')
					{
						account_sender.pop_back();
					}

					//������Ϣ���շ����˺�
					int account_recver_len = 0;
					recv(client_server, (char*)&account_recver_len, sizeof(account_recver_len), 0);
					std::string account_recver(account_recver_len, '\0');
					recv(client_server, &account_recver[0], account_recver_len, 0);

					char* buf = new char[50];
					snprintf(buf, 50, "the account_receiver is %s\n", account_recver.c_str());
					OutputDebugStringA(buf);
					delete[]buf;

					//��ѯ�û����ǳƲ�����
					std::string user_nickname1;
					std::string user_nickname2;

					MYSQL* conn_235 = mysql_init(NULL);
					int xxc = 0;
					while (!mysql_real_connect(conn_235, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))
					{
						xxc++;
						if (xxc > 3)
						{
							MessageBox(NULL, L"conn_235�������ݿ�ʧ��", L"QQ", MB_ICONERROR);
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
					
					//���պ���������ʼ������
					int client_len = 0;
					recv(client_server, (char*)&client_len, sizeof(client_len), 0);
					std::string client_str(client_len, '\0');
					recv(client_server, &client_str[0], client_len, 0);

					char* buf2 = new char[50];
					snprintf(buf2, 50, "the client_str is %s\n", client_str.c_str());
					OutputDebugStringA(buf2);
					delete[]buf2;

					if (strcmp("��������������Ϣ", client_str.c_str()) == 0)
					{
						if (g_users_chart_information.empty())
						{
							//֪ͨ�ͻ�����ʱû�������¼
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
									//��սṹ��
									ss = users_chart_information();

									ss.inf_send_account = acc.inf_send_account;
									ss.inf_recv_account = acc.inf_recv_account;
									ss.inf = acc.inf;

									g_ss.push_back(ss);

									inf_sum++;
								}
							}

							//��ͻ��˷�����Ϣ����
							send(client_server, (char*)&inf_sum, sizeof(inf_sum), 0);

							if (inf_sum>0)
							{
								//�����������������¼����
								int i = 0;
								while (i < inf_sum)
								{
									//��Ϣ���ͷ��˺�
									int inf_send_account_len=g_ss[i].inf_send_account.size();
									send(client_server, (char*)&inf_send_account_len, sizeof(inf_send_account_len), 0);
									send(client_server, g_ss[i].inf_send_account.c_str(), inf_send_account_len, 0);
									
									char* buf3 = new char[50];
									snprintf(buf3, 50, "the inf_send_account is %s\n",g_ss[i].inf_send_account.c_str());
									OutputDebugStringA(buf3);
									delete[]buf3;

									//��Ϣ���շ��˺�
									int inf_recv_account_len = g_ss[i].inf_recv_account.size();
									send(client_server, (char*)&inf_recv_account_len, sizeof(inf_recv_account_len), 0);
									send(client_server, g_ss[i].inf_recv_account.c_str(), inf_recv_account_len, 0);

									char* buf4 = new char[50];
									snprintf(buf4, 50, "the inf_recv_account is %s\n", g_ss[i].inf_recv_account.c_str());
									OutputDebugStringA(buf4);
									delete[]buf4;

									//��Ϣ
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

						
					//�������Կͻ��˵���Ϣ
					while (true)
					{
						//���տͻ��˷������ں��������İ�ťѡ��
						int sel_len = 0;
						recv(client_server, (char*)&sel_len, sizeof(sel_len), 0);
						std::string sel_str(sel_len,'\0');
						recv(client_server,&sel_str[0],sel_len,0);

						char* buf6 = new char[50];
						snprintf(buf6, 50, "the sel_str is %s\n",sel_str.c_str());
						OutputDebugStringA(buf6);
						delete[]buf6;

						if(strcmp("������Ϣ",sel_str.c_str()) == 0)
						{
							ix = users_chart_information();

							//������Ϣ
							int msg_len = 0;
							recv(client_server, (char*)&msg_len, sizeof(msg_len), 0);
							std::string msg(msg_len, '\0');
							recv(client_server, &msg[0], msg_len, 0);

							//��¼�ڼ�ؿ�

							{
								int w_len1 = MultiByteToWideChar(CP_UTF8, 0, user_nickname1.c_str(),user_nickname1.size(), NULL, 0);//�����ת������//
								std::wstring w_str1(w_len1, 0);//����ռ�//
								MultiByteToWideChar(CP_UTF8, 0, user_nickname1.c_str(), user_nickname1.size(), &w_str1[0], w_len1);//ʵ��ת��//

								int w_len2 = MultiByteToWideChar(CP_UTF8, 0, user_nickname2.c_str(),user_nickname2.size(), NULL, 0);//�����ת������//
								std::wstring w_str2(w_len2, 0);//����ռ�//
								MultiByteToWideChar(CP_UTF8, 0, user_nickname2.c_str(), user_nickname2.size(), &w_str2[0], w_len2);//ʵ��ת��//

								int w_len3 = MultiByteToWideChar(CP_UTF8, 0, msg.c_str(), msg_len, NULL, 0);//�����ת������//
								std::wstring w_str3(w_len3, 0);//����ռ�//
								MultiByteToWideChar(CP_UTF8, 0, msg.c_str(), msg_len, &w_str3[0], w_len3);//ʵ��ת��//
								//MessageBox(NULL, L"���յ�¼����ͻ��˷��͵ĶԻ�����Ϣ�ɹ�", L"QQ", MB_ICONINFORMATION);

								//MessageBox(NULL, w_str.c_str(), L"QQ", MB_ICONINFORMATION);
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
								//MessageBox(NULL, L"�Ѿ��ɹ����ǳ�ĩβ�ġ�0������", L"QQ", MB_ICONINFORMATION);
								msg_one += w_str1.c_str();
								//MessageBox(NULL, L"�Ѿ��ɹ�ƴ���ǳ�", L"QQ", MB_ICONINFORMATION);
								msg_one += L"->";
								//MessageBox(NULL, L"�Ѿ��ɹ�ƴ�Ӻ�->", L"QQ", MB_ICONINFORMATION);
								msg_one += w_str2.c_str();
								msg_one += L"]";
								//MessageBox(NULL, L"�Ѿ��ɹ�ƴ�Ӻ÷������ֶ�", L"QQ", MB_ICONINFORMATION);
								msg_one += w_str3.c_str();
								
								EnterCriticalSection(&g_csMsg_two_Queue);//����//
								g_msg_two_Queue.push(msg_one);//��������//
								LeaveCriticalSection(&g_csMsg_two_Queue);//����//
								//MessageBox(NULL, L"�Ѿ��ɹ�����Ϣƴ��", L"QQ", MB_ICONINFORMATION);
								// ��¼�������������//
								PostMessage(g_hInfoDialogmonitor, WM_APP_UPDATEMONITOR_MSG, 0, 0);
								//MessageBox(NULL, L"�Ѿ��ɹ�֪ͨ��Ϣ�ļ����ʾ", L"QQ", MB_ICONINFORMATION);

							}


							ix.inf_send_account = account_sender;
							ix.inf_recv_account = account_recver;
							ix.inf = msg;

							//����ѹ�����g_users_chart_information
							std::lock_guard<std::mutex>lok(g_users_chart_information_mutex);
							g_users_chart_information.push_back(ix);

							//������Ϣ
							goto door_1314;

						}

						else if (strcmp("������Ϣ", sel_str.c_str()) == 0)
						{
							goto door_1314;
						}

						else if (strcmp("�˳�", sel_str.c_str()) == 0)
						{
							//���ص����ѿ�
							goto door_102;
						}

					}
					}
					else
					{
						const char* buf = new char[50];
						buf = "���պ���������ʼ������ʧ��";
						OutputDebugStringA(buf);
						delete[]buf;
						return;
					}

					

				}
				else if (strcmp(oper_str.c_str(), "�鿴������Ϣ") == 0)
				{
					//���պ��ѵ��˺�
					int ac_len = 0;
					recv(client_server, (char*)&ac_len, sizeof(ac_len), 0);
					std::string ac(ac_len,'\0');
					recv(client_server, &ac[0], ac_len, 0);
                    
					char* buf_account = new char[50]();
					snprintf(buf_account, 50, "the fri_ac is %s\n",ac.c_str());
					OutputDebugStringA(buf_account);
					delete[]buf_account;


					//��ѯ���ѵĸ�����Ϣ
					MYSQL* conn_421 = mysql_init(NULL);
					int x_x = 0;
					while (!mysql_real_connect(conn_421,"127.0.0.1","myqq_admin","123456","myqq_database",3306,NULL,0))
					{
						x_x++;
						if (x_x > 3)
						{
							MessageBox(NULL,L"�������ݿ�conn_421ʧ��",L"QQ",MB_ICONERROR);
							return;
						}
					}

					//sql���
					const char* sql_421 = "SELECT gender,age,signature FROM users WHERE account=? LIMIT 1";
					//��ȡԤ����ָ��
					MYSQL_STMT* stmt_421 = mysql_stmt_init(conn_421);
					
					if (!stmt_421)
					{
						MessageBox(NULL,L"��ȡԤ����ָ��ʧ��",L"QQ",MB_ICONERROR);
						mysql_close(conn_421);
						return;
					}

					//��sql���ת��Ϊ���ݿ��ڲ��ṹ
					if (mysql_stmt_prepare(stmt_421, sql_421, strlen(sql_421)))
					{
						MessageBox(NULL, L"���ݿ�Ԥ�������ִ��ʧ��", L"QQ", MB_ICONERROR);
						mysql_stmt_close(stmt_421);
						mysql_close(conn_421);
						return;
					}

					//�󶨲���
					MYSQL_BIND bind_param[1];
					memset(bind_param, 0, sizeof(bind_param));

					bind_param[0].buffer_type = MYSQL_TYPE_STRING;
					bind_param[0].buffer = (void*)ac.c_str();
					bind_param[0].buffer_length = ac.size();

					if (mysql_stmt_bind_param(stmt_421, bind_param))
					{
						MessageBox(NULL, L"���ݿ�󶨲���ʧ��", L"QQ", MB_ICONERROR);
						mysql_stmt_close(stmt_421);
						mysql_close(conn_421);
						return;
					}

					if (mysql_stmt_execute(stmt_421))
					{
						MessageBox(NULL, L"���ݿ�󶨲���ʧ��", L"QQ", MB_ICONERROR);
						mysql_stmt_close(stmt_421);
						mysql_close(conn_421);
						return;
					}

					char* friend_gender = new char[8]();
					char* friend_age = new char[8]();
					char* friend_signature = new char[256]();

					//�󶨽������
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
						MessageBox(NULL, L"���ݿ�󶨽������ʧ��", L"QQ", MB_ICONERROR);
						mysql_stmt_close(stmt_421);
						mysql_close(conn_421);
						return;
					}


					//��������嵽����
					if (mysql_stmt_store_result(stmt_421))
					{
						MessageBox(NULL, L"����ѯ������浽�ͻ��˱���ʧ��", L"QQ", MB_ICONERROR);
						mysql_stmt_close(stmt_421);
						mysql_close(conn_421);
						return;
					}

					if (mysql_stmt_fetch(stmt_421))
					{
						MessageBox(NULL, L"�޷�ȡ����ѯ���", L"QQ", MB_ICONERROR);
						mysql_stmt_close(stmt_421);
						mysql_close(conn_421);
						return;
					}
					
					mysql_stmt_close(stmt_421);
					mysql_close(conn_421);


					//���ͺ����Ա�
					int fri_gender_len = strlen(friend_gender);
					send(client_server, (char*)&fri_gender_len, sizeof(fri_gender_len), 0);
					send(client_server, friend_gender, fri_gender_len, 0);

					char* buf_gender = new char[50]();
					snprintf(buf_gender, 50, "the fri_gender is %s\n",friend_gender);
					OutputDebugStringA(buf_gender);
					delete[]buf_gender;


					//���ͺ�������
					int fri_age_len = strlen(friend_age);
					send(client_server, (char*)&fri_age_len, sizeof(fri_age_len), 0);
					send(client_server,friend_age,fri_age_len,0);

					char* buf_age = new char[50]();
					snprintf(buf_age, 50, "the fri_age is %s\n", friend_age);
					OutputDebugStringA(buf_age);
					delete[]buf_age;

					//���ͺ��ѵĸ���ǩ��
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

                    //���տͻ����ں�����Ϣչʾ�����Ϣ
					
					int x_len = 0;
					recv(client_server, (char*)&x_len, sizeof(x_len), 0);
					std::string x_str(x_len, '\0');
					recv(client_server, &x_str[0], x_len, 0);

					char* buf_inf = new char[50]();
					snprintf(buf_inf, 50, "the information is %s\n",x_str.c_str());
					OutputDebugStringA(buf_inf);
					delete[]buf_inf;

					if (strcmp("ȡ��", x_str.c_str()) == 0)
					{

						//���ͷ����������־
						std::string signal_x = "success";
						int str_x_len_x = signal_x.size();
						send(client_server, (char*)&str_x_len_x, sizeof(str_x_len_x), 0);
						send(client_server, signal_x.c_str(), str_x_len_x, 0);
					}

					//���غ��ѿ�
					goto door_102;
				}
				else if (strcmp(oper_str.c_str(), "�ͻ���������Ӻ���") == 0)
				{

                 door_103:
					//���տͻ��˷�����ȷ����ȡ����ť
					int st_len = 0;
					recv(client_server, (char*)&st_len, sizeof(st_len), 0);
					std::string st(st_len, '\0');
					recv(client_server, &st[0], st_len, 0);

					char* buf_22 = new char[50]();
					snprintf(buf_22, 50, "the st is %s\n",st.c_str());
					OutputDebugStringA(buf_22);
					delete[]buf_22;


					if (strcmp(st.c_str(), "ȷ��") == 0)
					{
						//���տͻ��˷����ĺ����˺�
						int str_len = 0;
						recv(client_server, (char*)&str_len, sizeof(str_len), 0);
						std::string str_x(str_len, '\0');
						recv(client_server, &str_x[0], str_len, 0);//�˺ź�'\0'

						if (!str_x.empty() && str_x.back() == '\0')
						{
							str_x.pop_back();
						}

						char* buf_23 = new char[50]();
						snprintf(buf_23, 50, "the str_x is %s\n",str_x.c_str());
						OutputDebugStringA(buf_23);
						delete[]buf_23;


						//��users���в�ѯ���û��˺��Ƿ����

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
									//�˺Ŵ���
									mysql_close(conn_12);
									//�����˺��û��Ƿ��Ѿ��Ƿ�����Ӻ���������û��ĺ��� �������friends��
									MessageBox(NULL, L"��������ѯ���ͻ��˷�������Ҫ��Ӻ��ѵ��û��˺Ŵ���",L"QQ",MB_ICONINFORMATION);

									std::string host_account_y= new_online_user_account;//�������˺�
									if (!host_account_y.empty() && host_account_y.back() == '\0')
									{
										host_account_y.pop_back();
									}
									std::string fri_account_y = str_x;//������˺�


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

									//MessageBox(NULL, L"�������ɹ��������ݿ�����֤�����Ƿ����Ǻ���", L"QQ", MB_ICONINFORMATION);

									char sql_y[128];
									snprintf(sql_y, sizeof(sql_y), "SELECT 1 FROM friends WHERE account ='%s' AND friend_account='%s' LIMIT 1", host_account_y.c_str(), fri_account_y.c_str());
									if (mysql_query(conn_1211, sql_y) == 0)
									{
										MYSQL_RES* res = mysql_store_result(conn_1211);
										if (res)
										{
											if (mysql_num_rows(res) > 0)
											{
												//֪ͨ�ͻ����˺Ŵ���
											   //MessageBox(NULL, L"��������ѯ���ͻ��˷�������Ҫ��ӵĺ��ѣ������û��ĺ����б�", L"QQ", MB_ICONINFORMATION);

												std::string inf = "�����Ѿ��Ǻ��ѣ������ڷ��ͺ�������";
												int inf_len = inf.size();

												send(client_server, (char*)&inf_len, sizeof(inf_len), 0);
												send(client_server, inf.c_str(), inf_len, 0);

												mysql_close(conn_1211);
												goto door_102;
											}

											else 
											{
												//MessageBox(NULL, L"��������ѯ���ͻ��˷�������Ҫ��Ӻ��Ѳ����û��ĺ����б�����������half_friend,�ȴ�ͬ��", L"QQ", MB_ICONINFORMATION);

												mysql_close(conn_1211);
												//���half_friend���Ƿ��Ѵ�����ص�������������ԣ���û�У������
												//��������ӷ������˺ź�������˺ż�¼����half_friend
												std::string host_account_x = new_online_user_account;//�������˺�
												if (!host_account_x.empty() && host_account_x.back() == '\0')
												{
													host_account_x.pop_back();
												}
												std::string fri_account_x = str_x;//������˺�


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
												//MessageBox(NULL, L"�������ɹ����ӷ���������֤�Ƿ�������ͬ����Ӻ�������", L"QQ", MB_ICONINFORMATION);

												char sql_x[128];
												snprintf(sql_x, sizeof(sql_x), "SELECT 1 FROM half_friend WHERE account ='%s' AND friend_account='%s' LIMIT 1", host_account_x.c_str(), fri_account_x.c_str());
												if (mysql_query(conn_121, sql_x) == 0)
												{
													MYSQL_RES* res = mysql_store_result(conn_121);
													if (res)
													{
														if (mysql_num_rows(res) > 0)
														{
															//֪ͨ�ͻ����˺Ŵ���
															//MessageBox(NULL, L"��������ѯ�����û����͹���ͬ�ĺ����������", L"QQ", MB_ICONINFORMATION);

															std::string inf = "�Ѿ����͹��������������ĵȴ�";
															int inf_len = inf.size();

															send(client_server, (char*)&inf_len, sizeof(inf_len), 0);
															send(client_server, inf.c_str(), inf_len, 0);

															mysql_close(conn_121);
															goto door_102;

														}

														else
														{
															mysql_close(conn_121);
															//��������ӷ������˺ź�������˺ż�¼����half_friend
															std::string host_account = new_online_user_account;//�������˺�
															if (!host_account.empty() && host_account.back() == '\0')
															{
																host_account.pop_back();
															}
															std::string fri_account = str_x;//������˺�

															//host_account��ӵ�account�ֶ���
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

															//MessageBox(NULL, L"�����������û��˺ź���Ҫ��ӵĺ����˺���ӵ�half_friend��", L"QQ", MB_ICONINFORMATION);

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

															//�ɹ�����
															mysql_stmt_close(stmt);
															mysql_close(conn_101);
															//fri_account��ӵ�friend_account�ֶ���
															//MessageBox(NULL, L"�������ɹ����û��˺ź���Ҫ��ӵĺ����˺���ӵ�half_friend��", L"QQ", MB_ICONINFORMATION);

														   //֪ͨ�ͻ����˺Ŵ���
															std::string inf = "��ӵĺ����˺Ŵ���";
															int inf_len = inf.size();

															send(client_server, (char*)&inf_len, sizeof(inf_len), 0);
															send(client_server, inf.c_str(), inf_len, 0);

															goto door_102;
														}

													}

												}

												else
												{
													//MessageBox(NULL, L"��������ѯʧ��3", L"QQ", MB_ICONERROR);
													return;
												}
											}
										}
									}
									else
									{
										//MessageBox(NULL, L"��������ѯʧ��2", L"QQ", MB_ICONERROR);
										return;
									}

								}

								else
								{
									//�˺Ų�����
									std::string inf = "��ӵĺ����˺Ų�����";
									int inf_len = inf.size();
									send(client_server, (char*)&inf_len, sizeof(inf_len), 0);
									send(client_server, inf.c_str(), inf_len, 0);
									
									goto door_103;
								}
							}

							else//ȡ�����ʧ��
							{
								mysql_close(conn_12);
								return;

							}

						}
						else//δ��ѯ�ɹ���
						{
							mysql_close(conn_12);
							return;
						}

					}
					else if (strcmp(st.c_str(), "ȡ��") == 0)
					{
						goto door_102;
					}

				}
				else if (strcmp(oper_str.c_str(), "ɾ������") == 0)
				{
					//���շ����������־
					int x_len = 0;
					recv(client_server,(char*)&x_len, sizeof(x_len), 0);
					std::string x_str(x_len, '\0');
					recv(client_server, &x_str[0], x_len, 0);
					
					std::string xx = new_online_user_account;
					if (!xx.empty() && xx.back() == '\0')
					{
						xx.pop_back();
					}

					//ɾ��half_friend���е���ؼ�¼
					MYSQL* conn_123 = mysql_init(NULL);
					int x_y = 0;
					while (!mysql_real_connect(conn_123, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))
					{
						x_y++;
						if (x_y > 3)
						{
							MessageBox(NULL, L"���Ӳ������ݿ⣬�����˳�1", L"QQ", MB_ICONERROR);
							return;
						}
					}

					const char* sql_123 = "DELETE FROM friends WHERE friend_account=? AND account=?";
					MYSQL_STMT* stmt = mysql_stmt_init(conn_123);

					if (!stmt)
					{
						MessageBox(NULL, L"���ݿ�Ԥ����ָ���ȡʧ��1", L"QQ", MB_ICONERROR);
						mysql_close(conn_123);
						return;
					}

					if (mysql_stmt_prepare(stmt, sql_123, strlen(sql_123)))
					{
						MessageBox(NULL, L"���ݿ�Ԥ������䴦��ʧ��", L"QQ", MB_ICONERROR);
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
							MessageBox(NULL, L"���ݿ�󶨲�������ʧ��", L"QQ", MB_ICONERROR);
							mysql_stmt_close(stmt);
							mysql_close(conn_123);
							return;
						}

						if (mysql_stmt_execute(stmt))
						{
							MessageBox(NULL, L"���ݿ�ִ�����ִ��ʧ��", L"QQ", MB_ICONERROR);
							mysql_stmt_close(stmt);
							mysql_close(conn_123);
							return;
						}

					//��friend_account,account �ֶε����ݽ���

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
							MessageBox(NULL, L"���ݿ�󶨲�������ʧ��", L"QQ", MB_ICONERROR);
							mysql_stmt_close(stmt);
							mysql_close(conn_123);
							return;
						}

						if (mysql_stmt_execute(stmt))
						{
							MessageBox(NULL, L"���ݿ�ִ�����ִ��ʧ��", L"QQ", MB_ICONERROR);
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
				else if (strcmp(oper_str.c_str(), "������") == 0)
				{
					//���ճ���
					int r_len = 0;
					recv(client_server, (char*)&r_len, sizeof(r_len), 0);
					std::string s_str(r_len,'\0');
					recv(client_server, &s_str[0],r_len,0);

					char* buf_x1 = new char[50]();
					snprintf(buf_x1, 50, "the s_str is %s\n",s_str.c_str());
					OutputDebugStringA(buf_x1);
					delete[]buf_x1;


					//�ȽϽ�������
					if (strcmp(s_str.c_str(), "�ͻ�����������������б�") == 0)
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


						//��ѯ��half_friend,�鿴һ���ж�����friend_accountΪ�û��˻��ļ�¼
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
									half_friend_account_list.push_back(row[0]);//�洢������Ӻ��ѵ��û��˺�
									half_f++;
								}
								mysql_free_result(res);
							}
						}
						mysql_close(conn_45);
						delete[]sql_666;

						//��ͻ��˷��������ѵ�����
						send(client_server,(char*)&half_f,sizeof(half_f),0);

						char* buf_x3 = new char[50]();
						snprintf(buf_x3, 50, "the total half_friend is %d\n",half_f);
						OutputDebugStringA(buf_x3);
						delete[]buf_x3;
						
						if (half_f == 0)
						{
							goto door_102;//���غ��ѿ���ҳ
						}

						//�ӱ�users��ѯ��Ӻ��ѷ����˵��ǳƺ�ͷ��
						else if (half_f > 0)
						{
							std::vector <std::string> half_friends_nickname;
							std::vector <std::vector<BYTE>> half_friends_image;

							half_friends_nickname.clear();
							half_friends_image.clear();

							//��ѯ��half_friend,�鿴һ���ж�����friend_accountΪ�û��˻��ļ�¼
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

								MYSQL_BIND bind[1]; // 1������
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


								//��������
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
										//�������ͷ���б�
										std::vector<BYTE>one_image(friend_image, friend_image + friend_image_len);
										//ѹ����Ѷ���
										half_friends_image.push_back(std::move(one_image));
									}
									else
									{
										std::vector<BYTE>one_image(friend_image, friend_image + 3 * 1024 * 1024);
										//ѹ����Ѷ���
										half_friends_image.push_back(std::move(one_image));
									}

								}

								delete[]friend_nickname;
								delete[]friend_image;
								it++;
							}

							mysql_stmt_close(stmt);
							mysql_close(conn_4544);


							//��ͻ��˷��������ѵ���Ϣ
							int i = 0;
							while (i < half_f)
							{
								std::string account_x = half_friend_account_list[i];//�����˺��б����
								std::string nickname_x = half_friends_nickname[i];//�����ǳ��б����
								std::vector<BYTE>image_x = half_friends_image[i];//����ͷ���б�
								

								//���ͺ����˺�
								int account_x_len = account_x.size();
								send(client_server, (char*)&account_x_len, sizeof(account_x_len), 0);
								send(client_server, account_x.c_str(), account_x_len, 0);

								char* buf_4 = new char[50]();
								snprintf(buf_4, 50, "the send_account is %s\n", account_x.c_str());
								OutputDebugStringA(buf_4);
								delete[]buf_4;


								//���ͺ����ǳ�
								int nickname_x_len = nickname_x.size();
								send(client_server, (char*)&nickname_x_len, sizeof(nickname_x_len), 0);
								send(client_server, nickname_x.c_str(), nickname_x_len, 0);

								char* buf_5 = new char[50]();
								snprintf(buf_5, 50, "the send_nickname is %s\n", nickname_x.c_str());
								OutputDebugStringA(buf_5);
								delete[]buf_5;


								//���ͺ���ͷ��
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


							//��Ӧ�ͻ��������ѿ��ѡ��
							
							//���ճ���
							int r_len_x= 0;
							recv(client_server, (char*)&r_len_x, sizeof(r_len_x), 0);
							std::string s_str_x(r_len_x, '\0');
							recv(client_server, &s_str_x[0], r_len_x, 0);
							
							char* buf_113 = new char[50];
							snprintf(buf_113, strlen(buf_113), "the s_str_x is %s\n",s_str_x.c_str());
							OutputDebugStringA(buf_113);
							delete[]buf_113;

							if (strcmp("ͬ��",s_str_x.c_str()) == 0)
							{
								//������������Ŀ
								int f_len = 0;
								recv(client_server, (char*)&f_len, sizeof(f_len), 0);

								char* buf_114 = new char[50];
								snprintf(buf_114, strlen(buf_114), "the f_len is %d\n",f_len);
								OutputDebugStringA(buf_114);
								delete[]buf_114;
								//���տͻ��˷�����ͬ����������˺�
								//�������洢�ͻ��˷������˺�
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

								//ɾ��half_friend���е���ؼ�¼
								MYSQL* conn_123 = mysql_init(NULL);
								int x_y = 0;
								while (!mysql_real_connect(conn_123, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))
								{
									x_y++;
									if (x_y > 3)
									{
										MessageBox(NULL, L"���Ӳ������ݿ⣬�����˳�1", L"QQ", MB_ICONERROR);
										return;
									}
								}

								const char* sql_123 = "DELETE FROM half_friend WHERE friend_account=? AND account=?";
								MYSQL_STMT* stmt = mysql_stmt_init(conn_123);

								if (!stmt)
								{
									MessageBox(NULL, L"���ݿ�Ԥ����ָ���ȡʧ��1", L"QQ", MB_ICONERROR);
									mysql_close(conn_123);
									return;
								}

								if (mysql_stmt_prepare(stmt, sql_123, strlen(sql_123)))
								{
									MessageBox(NULL, L"���ݿ�Ԥ������䴦��ʧ��", L"QQ", MB_ICONERROR);
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
										MessageBox(NULL, L"���ݿ�󶨲�������ʧ��", L"QQ", MB_ICONERROR);
										mysql_stmt_close(stmt);
										mysql_close(conn_123);
										return;
									}

									if (mysql_stmt_execute(stmt))
									{
										MessageBox(NULL, L"���ݿ�ִ�����ִ��ʧ��", L"QQ", MB_ICONERROR);
										mysql_stmt_close(stmt);
										mysql_close(conn_123);
										return;
									}

									it++;

								}

								mysql_stmt_close(stmt);
								mysql_close(conn_123);

								//����friends���������µ����Ѽ�¼


								//ɾ��half_friend���е���ؼ�¼
								MYSQL* conn_1234 = mysql_init(NULL);
								int x_y_3 = 0;
								while (!mysql_real_connect(conn_1234, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))
								{
									x_y_3++;
									if (x_y_3 > 3)
									{
										MessageBox(NULL, L"���Ӳ������ݿ�2", L"QQ", MB_ICONERROR);
										return;
									}
								}

								const char* sql_1234 = "INSERT INTO friends (account,friend_account)VALUES(?,?)";
								MYSQL_STMT* stmt_1234= mysql_stmt_init(conn_1234);

								if (!stmt_1234)
								{
									MessageBox(NULL, L"���ݿ�Ԥ����ָ���ȡʧ��", L"QQ", MB_ICONERROR);
									mysql_close(conn_1234);
									return;
								}

								if (mysql_stmt_prepare(stmt_1234, sql_1234, strlen(sql_1234)))
								{
									MessageBox(NULL, L"���ݿ�Ԥ�������ִ��ʧ��", L"QQ", MB_ICONERROR);
									mysql_stmt_close(stmt_1234);
									mysql_close(conn_1234);
									return;
								}

								//���������˫���

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
										MessageBox(NULL, L"���ݿ�󶨲���ʧ��2", L"QQ", MB_ICONERROR);
										mysql_stmt_close(stmt_1234);
										mysql_close(conn_1234);
										return;
									}

									if (mysql_stmt_execute(stmt_1234))
									{
										MessageBox(NULL, L"���ݿ�ִ�����ִ��ʧ��2", L"QQ", MB_ICONERROR);
										mysql_stmt_close(stmt_1234);
										mysql_close(conn_1234);
										return;
									}

									it++;
								}

								//account �� friend_account �ֶ����ݽ���
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
										MessageBox(NULL, L"���ݿ�󶨲���ʧ��3", L"QQ", MB_ICONERROR);
										mysql_stmt_close(stmt_1234);
										mysql_close(conn_1234);
										return;
									}

									if (mysql_stmt_execute(stmt_1234))
									{
										MessageBox(NULL, L"���ݿ�ִ�����ִ��ʧ��3", L"QQ", MB_ICONERROR);
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
							else if (strcmp("�ܾ�",s_str_x.c_str()) == 0)
							{

								//������������Ŀ
								int f_len = 0;
								recv(client_server, (char*)&f_len, sizeof(f_len), 0);

								char* buf_114 = new char[50];
								snprintf(buf_114, strlen(buf_114), "the f_len is %d\n", f_len);
								OutputDebugStringA(buf_114);
								delete[]buf_114;
								//���տͻ��˷�����ͬ����������˺�
								//�������洢�ͻ��˷������˺�
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

								//ɾ��half_friend���е���ؼ�¼
								MYSQL* conn_123 = mysql_init(NULL);
								int x_y = 0;
								while (!mysql_real_connect(conn_123, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))
								{
									x_y++;
									if (x_y > 3)
									{
										MessageBox(NULL, L"���Ӳ������ݿ⣬�����˳�1", L"QQ", MB_ICONERROR);
										return;
									}
								}

								const char* sql_123 = "DELETE FROM half_friend WHERE friend_account=? AND account=?";
								MYSQL_STMT* stmt = mysql_stmt_init(conn_123);

								if (!stmt)
								{
									MessageBox(NULL, L"���ݿ�Ԥ����ָ���ȡʧ��1", L"QQ", MB_ICONERROR);
									mysql_close(conn_123);
									return;
								}

								if (mysql_stmt_prepare(stmt, sql_123, strlen(sql_123)))
								{
									MessageBox(NULL, L"���ݿ�Ԥ������䴦��ʧ��", L"QQ", MB_ICONERROR);
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
										MessageBox(NULL, L"���ݿ�󶨲�������ʧ��", L"QQ", MB_ICONERROR);
										mysql_stmt_close(stmt);
										mysql_close(conn_123);
										return;
									}

									if (mysql_stmt_execute(stmt))
									{
										MessageBox(NULL, L"���ݿ�ִ�����ִ��ʧ��", L"QQ", MB_ICONERROR);
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

							else if (strcmp("ȡ��", s_str_x.c_str()) == 0)
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
	else if (wcscmp(wstr.c_str(),L"������Ϣ")==0)
	{
		send_personal_information_to_client(client_server,new_online_user_account);//�����û�ԭ�����û����ݷ����ͻ���

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
		if (wcscmp(w_str.c_str(), L"ȷ��") == 0)
		{
			my_user_information new_infor;
			//���ճ��û��˺�֮����û���Ϣ�����������ݿ�����û���Ϣ
			recv_all_information(client_server,new_infor.password);
			recv_all_information(client_server, new_infor.nickname);
			recv_all_information(client_server, new_infor.gender);
			recv_all_information(client_server, new_infor.age);
			recv_all_information(client_server, new_infor.signature);

			//�����û��˺ţ������û����ݿ���Ϣ
			MYSQL* conn_4 = mysql_init(NULL);
			std::string my_acc = new_online_user_account;
			if (!my_acc.empty()&&my_acc.back() == '\0')
			{
				my_acc.pop_back();
			}
			if (update_user_information((const std::string)my_acc, conn_4, new_infor))//�ɹ������û���Ϣ
			{
				//֪ͨ�㲥����и���

				//�����û��б������״̬//
				{
					std::lock_guard<std::mutex> lk(users_mutex);//����
					users_update_signal = true;      // ��������
					users_cv.notify_one();           // ����һ���ȴ��߳�
				}

				MessageBox(NULL, L"�������Ѿ��ɹ����ݿͻ��˷�������Ϣ�����û���Ϣ", L"QQ", MB_ICONINFORMATION);
				std::string inf = "update_success";
				int inf_len = inf.size();
				send(client_server,(char*)&inf_len,sizeof(inf_len),0);
				send(client_server,inf.c_str(),inf_len,0);
				mysql_close(conn_4);
			}
			else
			{
				MessageBox(NULL, L"�������޷������û���Ϣ", L"QQ", MB_ICONERROR);
				mysql_close(conn_4);
				return;
			}
			//���ص���¼������ҳ
			std::thread(Handlelogin_pro, client_server, my_data,new_online_user_account).detach();
		}
		else if (wcscmp(w_str.c_str(), L"ȡ��") == 0)
		{
			//���ص���¼������ҳ
			std::thread(Handlelogin_pro, client_server, my_data,new_online_user_account).detach();
		}
	}
	else if (wcscmp(wstr.c_str(),L"����ͷ��")==0)
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
		if (wcscmp(w_str.c_str(), L"ȷ��") == 0)
		{
			//�û��˺�
			std::string user_account_new = new_online_user_account;//��'\0'
			if (!new_online_user_account.empty()&&new_online_user_account.back() == '\0')
			{
				new_online_user_account.pop_back();
			}
		    //�����µ�ͷ������
			int my_new_img_len = 0;
			recv(client_server,(char*)&my_new_img_len,sizeof(my_new_img_len),0);
			BYTE* my_new_img = new BYTE[my_new_img_len];
			int r = 0;//���ν�����
			int sum = 0;//�ܽ�����
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
			//�����û��˺Ų���MySQL���ݿ���û�ͷ�񲢽��и���
			if (update_user_img(new_online_user_account, my_new_img, my_new_img_len, conn))
			{
				//�������Ѿ��ɹ�����ͷ��
				//֪ͨ�����������û��б�
				std::lock_guard<std::mutex> lk(users_mutex);//����
				users_update_signal = true;      // ��������
				users_cv.notify_one();           // ����һ���ȴ��߳�
			   //���ͳɹ���Ϣ���ͻ���
				std::wstring img_update_success = L"�������Ѿ��ɹ������û�ͷ��";
				int utf8_img_update_success = WideCharToMultiByte(CP_UTF8,0,img_update_success.c_str(),img_update_success.size()+1,NULL,0,NULL,NULL);
				std::string  utf8_img_update_success_str(utf8_img_update_success,'\0');
				WideCharToMultiByte(CP_UTF8, 0, img_update_success.c_str(), img_update_success.size() + 1, &utf8_img_update_success_str[0], utf8_img_update_success, NULL, NULL);
				send(client_server,(char*)&utf8_img_update_success,sizeof(utf8_img_update_success),0);
				send(client_server, utf8_img_update_success_str.c_str(), utf8_img_update_success, 0);

			}
			else
			{
				//����ʧ����Ϣ���ͻ���
				std::wstring img_update_success = L"�������޷������û�ͷ��";
				int utf8_img_update_success = WideCharToMultiByte(CP_UTF8, 0, img_update_success.c_str(), img_update_success.size() + 1, NULL, 0, NULL, NULL);
				std::string  utf8_img_update_success_str(utf8_img_update_success, '\0');
				WideCharToMultiByte(CP_UTF8, 0, img_update_success.c_str(), img_update_success.size() + 1, &utf8_img_update_success_str[0], utf8_img_update_success, NULL, NULL);
				send(client_server, (char*)&utf8_img_update_success, sizeof(utf8_img_update_success), 0);
				send(client_server, utf8_img_update_success_str.c_str(), utf8_img_update_success, 0);
			}
			//���ص���¼������ҳ
			std::thread(Handlelogin_pro, client_server, my_data,new_online_user_account).detach();
		}
		else if (wcscmp(w_str.c_str(), L"ȡ��") == 0)
		{
			//���ص���¼������ҳ
            std::thread(Handlelogin_pro, client_server, my_data,new_online_user_account).detach();
		}
		else 
		{
			return;
		}
	}
	else if (wcscmp(wstr.c_str(),L"����")==0)
	{
		//�ҵ��˳���¼���û��˺�//
		std::string del_account = my_data.account;//my_data����'\0'��ֹ��//
		    //�����û��������û��б���ɾ��//
			std::lock_guard<std::mutex>lock(g_onlineUsersMutex);
			//���ط���������account��λ��,����account�ƶ���ĩβ//
			auto it = std::remove_if(g_onlineUsers.begin(), g_onlineUsers.end(), [&del_account](const User_account& user)
				{
					if (user.account.back() == '\0')//���accountĩβ��'\0'��ֹ��//
					{
						std::string x = user.account;
						x.pop_back();//ȥ��'\0'��ֹ��//
					    return x == del_account;
				    }
				});
			//ɾ��it��end()֮���Ԫ�أ�����it��������end()//
			if (it != g_onlineUsers.end()) 
			{
				g_onlineUsers.erase(it, g_onlineUsers.end());
			}

			//�����û��б������״̬//
			{
				std::lock_guard<std::mutex> lk(users_mutex);
				users_update_signal = true;      // ��������
				users_cv.notify_one();           // ����һ���ȴ��߳�
			}

			//ˢ�¹㲥��
			std::thread(LoadUsersFromDB, conn).detach();//�㲥���߳�//

			//���ص�¼��ע���ж�ѡ���
			//�жϾ���ִ�������߳�,��¼��ע��//
			int recv_len = 0;
			recv(client_server, (char*)&recv_len, sizeof(recv_len), 0);//������Ϣ����//
			if (recv_len <= 0)
			{
				//MessageBox(NULL, L"�����߳����͵���Ϣ����ʧ��", L"QQ", MB_ICONERROR);
				return ;
			}
			//MessageBox(NULL, L"�����߳����͵���Ϣ���ȳɹ�", L"QQ", MB_ICONINFORMATION);
			std::string recvchar(recv_len, 0);
			int r = 0;
			r = recv(client_server, &recvchar[0], recv_len, 0);
			{
				if (r <= 0)
				{
					//MessageBox(NULL, L"�����߳����͵���Ϣʧ��", L"QQ", MB_ICONERROR);
					return ;
				}
			}
			//MessageBox(NULL, L"�����߳����͵���Ϣ�ɹ�", L"QQ", MB_ICONINFORMATION);
			int wlen = MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, NULL, 0);//ת��Ϊ���ֽ�����Ҫ���ֽ���//
			std::wstring wrecvchar(wlen, 0);//����ռ�//
			MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, &wrecvchar[0], wlen);//ʵ��ת��//
			//ע�ᴦ��//
			if (wcscmp(wrecvchar.c_str(), L"ע��") == 0)//ע���߳�//
			{
				//MessageBox(NULL, L"���յ�ע����Ϣ����������ע���߳�", L"QQ", MB_ICONINFORMATION);
				std::thread(HandleClient_register, client_server).detach();//����ע���߳�//
			}
			//��¼����//
			else if (wcscmp(wrecvchar.c_str(), L"��¼") == 0)//��¼�߳�//
			{
				//MessageBox(NULL, L"���յ���¼��Ϣ���������õ�¼�߳�", L"QQ", MB_ICONINFORMATION);
				std::thread(HandleClient_login, client_server,new_online_user_account).detach();//������¼�߳�//
			}
			else if (wcscmp(wrecvchar.c_str(), L"�˳�") == 0)//�û��˳��ͻ��˳���//
			{
				//MessageBox(NULL, L"���ղ����߳���Ϣ�������˳�", L"QQ", MB_ICONINFORMATION);
				return ;
			}

	}
	else if (wcscmp(wstr.c_str(),L"�˳�")==0)
	{
		//�ҵ��˳���¼���û��˺�//
		std::string del_account = my_data.account;//my_data����'\0'��ֹ��//
		//�����û��������û��б���ɾ��//
		std::lock_guard<std::mutex>lock(g_onlineUsersMutex);
		//���ط���������account��λ��,����account�ƶ���ĩβ//
		auto it = std::remove_if(g_onlineUsers.begin(), g_onlineUsers.end(), [&del_account](const User_account& user)
			{
				if (user.account.back() == '\0')//���accountĩβ��'\0'��ֹ��//
				{
					std::string x = user.account;
					x.pop_back();//ȥ��'\0'��ֹ��//
					return x == del_account;
				}
			});
		//ɾ��it��end()֮���Ԫ�أ�����it��������end()//
		if (it != g_onlineUsers.end())
		{
			g_onlineUsers.erase(it, g_onlineUsers.end());
		}
		//�����û��б������״̬//
		{
			std::lock_guard<std::mutex> lk(users_mutex);
			users_update_signal = true;      // ��������
			users_cv.notify_one();           // ����һ���ȴ��߳�
		}
		return;//�˳����߳�//
	}
	else if (wcscmp(wstr.c_str(), L"ע���˺�") == 0)
	{
		int len_utf8 = 0;
		recvAll(client_server, (char*)&len_utf8, sizeof(len_utf8), 0);//�Ƚ��ճ���//
		//MessageBox(NULL, L"����ע���˺Ž���ͻ��˷��͵ĳ�����Ϣ�ɹ�", L"QQ", MB_ICONINFORMATION);
		if (len_utf8 <= 0)
		{
			MessageBox(NULL, L"����ע���˺Ž��水ť��Ϣ����ʧ��", L"QQ", MB_ICONERROR);
			return;
		}
		std::string str_utf8(len_utf8, 0);//����ռ�//
		recvAll(client_server, &str_utf8[0], len_utf8, 0);//��������//
		//MessageBox(NULL, L"���յ�¼����ͻ��˷��͵İ�ť������Ϣ�ɹ�", L"QQ", MB_ICONINFORMATION);
		int wchar_len = MultiByteToWideChar(CP_UTF8, 0, str_utf8.c_str(), len_utf8, NULL, 0);//�����ת������//
		std::wstring wchar_str(wchar_len, 0);//����ռ�//
		MultiByteToWideChar(CP_UTF8, 0, str_utf8.c_str(), len_utf8, &wchar_str[0], wchar_len);//ʵ��ת��//
		//MessageBox(NULL,wchar_str.c_str(), L"QQ", MB_ICONINFORMATION);

		if (wcscmp(wchar_str.c_str(),L"ȷ��") == 0)//�û�ȷ��ע��//
		{
			if (delete_user_by_account_password((const char*)my_data.account.c_str(),(const char*)my_data.password.c_str(), conn))
			{
				//�����û��б������״̬//
				
					//std::lock_guard<std::mutex> lk(users_mutex);
					//users_update_signal = true;      // ��������
					//users_cv.notify_one();           // ����һ���ȴ��߳�

				{
					std::vector<Userdata>temp_users;
					temp_users.clear();
					const char* sql = "SELECT nickname,imgData,account FROM users";//SQL�������//
					if (mysql_query(conn, sql))//���û�з��ϵĲ�ѯ��������ط���ֵ//
					{
						//MessageBox(NULL, L"û�з��ϵ��ǳƺ�ͷ��", L"QQ", MB_ICONERROR);
						return;
					}

					MYSQL_RES* res = mysql_store_result(conn);//һ�������//
					if (!res)
					{
						return;
					}
					MYSQL_ROW  row;//ָ���ַ��������ָ��//
					unsigned long* lengths;
					while ((row = mysql_fetch_row(res)))//ȡĳһ�в�ѯ�����//
					{
						lengths = mysql_fetch_lengths(res);//ȡĳһ�в�ѯ������ȼ�//
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
						temp_users.push_back(std::move(user));//�����õ��û������ƶ���ȫ���û��б����ֹ����Ҫ�Ŀ���//
					}
					mysql_free_result(res);
					std::lock_guard<std::mutex>lock(g_usersMutex);//����//
					g_users.swap(temp_users);

					if (g_hInfoDialogbroadcast && IsWindow(g_hInfoDialogbroadcast))//�ȴ������Ч//
						PostMessage(g_hInfoDialogbroadcast, WM_APP_UPDATEBROADCAST_MSG, 0, 0); // �Զ�����Ϣ
				}

				//std::lock_guard<std::mutex>lk(users_mutex);
                //users_update_signal =true;//�����û��б��־//
				//users_cv.notify_one();//֪ͨ����ִ�и����û��б�//

				//���ص�¼ע��ѡ�����߳�//
				//�жϾ���ִ�������߳�,��¼��ע��//
				int recv_len = 0;
				recv(client_server, (char*)&recv_len, sizeof(recv_len), 0);//������Ϣ����//
				if (recv_len <= 0)
				{
					//MessageBox(NULL, L"�����߳����͵���Ϣ����ʧ��", L"QQ", MB_ICONERROR);
					return ;
				}
				//MessageBox(NULL, L"�����߳����͵���Ϣ���ȳɹ�", L"QQ", MB_ICONINFORMATION);
				std::string recvchar(recv_len, 0);
				int r = 0;
				r = recv(client_server, &recvchar[0], recv_len, 0);
				{
					if (r <= 0)
					{
						//MessageBox(NULL, L"�����߳����͵���Ϣʧ��", L"QQ", MB_ICONERROR);
						return ;
					}
				}
				//MessageBox(NULL, L"�����߳����͵���Ϣ�ɹ�", L"QQ", MB_ICONINFORMATION);
				int wlen = MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, NULL, 0);//ת��Ϊ���ֽ�����Ҫ���ֽ���//
				std::wstring wrecvchar(wlen, 0);//����ռ�//
				MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, &wrecvchar[0], wlen);//ʵ��ת��//
				//ע�ᴦ��//
				if (wcscmp(wrecvchar.c_str(), L"ע��") == 0)//ע���߳�//
				{
					//MessageBox(NULL, L"���յ�ע����Ϣ����������ע���߳�", L"QQ", MB_ICONINFORMATION);
					std::thread(HandleClient_register, client_server).detach();//����ע���߳�//
				}
				//��¼����//
				else if (wcscmp(wrecvchar.c_str(), L"��¼") == 0)//��¼�߳�//
				{
					//MessageBox(NULL, L"���յ���¼��Ϣ���������õ�¼�߳�", L"QQ", MB_ICONINFORMATION);
					std::thread(HandleClient_login, client_server,new_online_user_account).detach();//������¼�߳�//
				}
				else if(wcscmp(wrecvchar.c_str(), L"�˳�") == 0)//����û��˳��ͻ��˳���//
				{
					//MessageBox(NULL, L"���ղ����߳���Ϣ�������˳�", L"QQ", MB_ICONINFORMATION);
					
					return ;
				}

			}
			else
			{
				MessageBox(NULL, L"û�ɹ�ע�����û�", L"QQ", MB_ICONERROR);
				return;
			}
		}
		else if (wcscmp(wchar_str.c_str(),L"ȡ��") == 0)//�û�ȡ����ע��//
		{
			std::thread(Handlelogin_pro,client_server,my_data,new_online_user_account).detach();//���ص�½���洦���߳�//
		}

	}
}


DWORD WINAPI StartServer(LPVOID lpParam)//�������̺߳���//
{
	SOCKET server_socket = INVALID_SOCKET;
	server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//�����׽���//
	if (server_socket == INVALID_SOCKET)
	{
		MessageBox(NULL, L"�������׽��ִ���ʧ��", L"QQ", MB_ICONERROR);
		return 1;
	}
	else
	{
		MessageBox(NULL, L"�������׽��ִ����ɹ�", L"QQ", MB_ICONINFORMATION);
	}
	//���÷�������ַ//
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;//����ΪIPv4//
	serverAddr.sin_port = htons(8080);//�˿ں�//
	serverAddr.sin_addr.s_addr = INADDR_ANY;//��������������������������IP��ַ//
	bind(server_socket, (sockaddr*)&serverAddr, sizeof(serverAddr));//���׽�����ָ����IP��ַ�Ͷ˿ںŹ���//
	listen(server_socket, 50);//��ʼ����//
	int y = 0;

	std::thread(LoadUsersFromDB,conn).detach();//�㲥���߳�//

	while (true && y <= 3)
	{
		SOCKET client_server = accept(server_socket, NULL, NULL);//�����µ��׽���//
		if (client_server == INVALID_SOCKET)
		{
			//MessageBox(NULL, L"�޷������µ��׽�����ͻ��˽���ͨ��", L"QQ", MB_ICONINFORMATION);
			y++;
			continue;
		}
		
		//MessageBox(NULL, L"�ɹ������µ��׽�����ͻ��˽���ͨ��", L"QQ", MB_ICONINFORMATION);
		//�жϾ���ִ�������߳�,��¼��ע��//
		int recv_len = 0;
		recv(client_server, (char*)&recv_len, sizeof(recv_len), 0);//������Ϣ����//
		if (recv_len <= 0)
		{
			//MessageBox(NULL, L"�����߳����͵���Ϣ����ʧ��", L"QQ", MB_ICONERROR);
			return 1;
		}
		//MessageBox(NULL, L"�����߳����͵���Ϣ���ȳɹ�", L"QQ", MB_ICONINFORMATION);
		std::string recvchar(recv_len, 0);
		int r = 0;
		r = recv(client_server, &recvchar[0], recv_len, 0);
		{
			if (r <= 0)
			{
				//MessageBox(NULL, L"�����߳����͵���Ϣʧ��", L"QQ", MB_ICONERROR);
				return 1;
			}
		}
		//MessageBox(NULL, L"�����߳����͵���Ϣ�ɹ�", L"QQ", MB_ICONINFORMATION);
		int wlen = MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, NULL, 0);//ת��Ϊ���ֽ�����Ҫ���ֽ���//
		std::wstring wrecvchar(wlen, 0);//����ռ�//
		MultiByteToWideChar(CP_UTF8, 0, recvchar.c_str(), recv_len, &wrecvchar[0], wlen);//ʵ��ת��//
		//ע�ᴦ��//
		if (wcscmp(wrecvchar.c_str(), L"ע��") == 0)//ע���߳�//
		{
			//MessageBox(NULL, L"���յ�ע����Ϣ����������ע���߳�", L"QQ", MB_ICONINFORMATION);
			std::thread(HandleClient_register, client_server).detach();//����ע���߳�//
		}
		//��¼����//
		else if (wcscmp(wrecvchar.c_str(), L"��¼") == 0)//��¼�߳�//
		{
			//MessageBox(NULL, L"���յ���¼��Ϣ���������õ�¼�߳�", L"QQ", MB_ICONINFORMATION);
			std::string new_online_user_account;
			std::thread(HandleClient_login, client_server,new_online_user_account).detach();//������¼�߳�//
		}
		else if(wcscmp(wrecvchar.c_str(), L"�˳�") == 0)//�û��˳��ͻ��˳���//
		{
			//MessageBox(NULL, L"���ղ����߳���Ϣ�������˳�", L"QQ", MB_ICONINFORMATION);
			return 1;
		}
	}
	return 1;
}


int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE, LPSTR, int nCmdShow)
{
	int con_num = 0;
	while (!mysql_real_connect(conn, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))//�������ݿ�//
	{
		MessageBox(NULL, L"�������ݿ�ʧ��", L"QQ", MB_ICONERROR);
		con_num++;
		if (con_num > 3)
		{
			return 1;
		}
	}
	MessageBox(NULL, L"�ɹ��������ݿ�", L"QQ", MB_ICONINFORMATION);

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
	while (!mysql_real_connect(conn_7, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))//�������ݿ�//
	{
		MessageBox(NULL, L"�������ݿ�ʧ��", L"QQ", MB_ICONERROR);
		con_num_2++;
		if (con_num_2 > 3)
		{
			return 1;
		}
	}
	MessageBox(NULL, L"�ɹ��������ݿ�", L"QQ", MB_ICONINFORMATION);

	//��������Ϊusers//
	const char* create_table_2 = "CREATE TABLE IF NOT EXISTS friends("
		"id INT PRIMARY KEY AUTO_INCREMENT,"
		"account VARCHAR(64),"
		"friend_account VARCHAR(64)"
		");";
	mysql_query(conn_7, create_table_2);
	mysql_close(conn_7);


	int conn_33_count = 0;
	MYSQL* conn_33 = mysql_init(NULL);
	while (!mysql_real_connect(conn_33, "127.0.0.1", "myqq_admin", "123456", "myqq_database", 3306, NULL, 0))//�������ݿ�//
	{
		MessageBox(NULL, L"�������ݿ�ʧ��", L"QQ", MB_ICONERROR);
		conn_33_count++;
		if (conn_33_count > 3)
		{
			return 1;
		}
	}
	MessageBox(NULL, L"�ɹ��������ݿ�", L"QQ", MB_ICONINFORMATION);

	//��������Ϊusers//
	const char* create_table_3 = "CREATE TABLE IF NOT EXISTS half_friend("
		"id INT PRIMARY KEY AUTO_INCREMENT,"
		"account VARCHAR(64),"
		"friend_account VARCHAR(64)"
		");";
	mysql_query(conn_33, create_table_3);
	mysql_close(conn_33);


	ULONG_PTR gdiToken = 0;//��ʼ��GDI+//
	if (!gdiToken)
	{
		Gdiplus::GdiplusStartupInput gdiplusStarupInput;
		Gdiplus::GdiplusStartup(&gdiToken, &gdiplusStarupInput, NULL);
	}
	InitializeCriticalSection(&g_csMsgQueue);//��ʼ��//
	InitializeCriticalSection(&g_csMsg_two_Queue);//��ʼ��//

	INT_PTR result_one = DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_ONE), NULL, Dialog_one);//��ʼ��//
	if (result_one == IDOK)//һ���ж�//
	{
		WSADATA wsaData;
		int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (result)
		{
			MessageBox(NULL, L"Winsock��ʼ��ʧ��", L"QQ", MB_ICONERROR);
			WSACleanup();
			return 1;
		}
		if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
		{
			MessageBox(NULL, L"��֧��Winsock2.2", L"QQ", MB_ICONERROR);
			WSACleanup();
			return 1;
		}
		MessageBox(NULL, L"Winsock��ʼ���ɹ�", L"QQ", MB_ICONINFORMATION);
		//��������ش���//
		if (g_hInfoDialog == NULL)//������Ϣ�Ի���//
		{
			g_hInfoDialog = CreateDialog(
				GetModuleHandle(NULL),
				MAKEINTRESOURCE(IDD_MYDIALOG_INFORMATION),
				NULL,
				Dialog_information
			);
			ShowWindow(g_hInfoDialog, SW_SHOW);
		}
		if (g_hInfoDialogphoto == NULL)//����ͷ����ʾ��//
		{
			g_hInfoDialogphoto = CreateDialog(
				GetModuleHandle(NULL),
				MAKEINTRESOURCE(IDD_MYDIALOG_PHOTO),
				NULL,
				Dialog_photo
			);
			ShowWindow(g_hInfoDialogphoto, SW_SHOW);
		}
		if (g_hInfoDialogmonitor== NULL)//������ؿ�//
		{
			g_hInfoDialogmonitor = CreateDialog(
				GetModuleHandle(NULL),
				MAKEINTRESOURCE(IDD_MYDIALOG_MONITOR),
				NULL,
				Dialog_monitor
			);
			ShowWindow(g_hInfoDialogmonitor, SW_SHOW);
		}
		if (g_hInfoDialogbroadcast == NULL)//�㲥��//
		{
			g_hInfoDialogbroadcast = CreateDialog(
				GetModuleHandle(NULL),
				MAKEINTRESOURCE(IDD_MYDIALOG_BROADCAST),
				NULL,
				Dialog_broadcast
			);
			ShowWindow(g_hInfoDialogbroadcast, SW_SHOW);
		}

		hThread = CreateThread(NULL, 0, StartServer, NULL, 0, NULL);//�̺߳���//
	}
	else if (result_one == IDEND)//һ���ж�//
	{
		g_exitFlag =true;
		DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_END), NULL, Dialog_end);//������//
	}
	// ����Ϣѭ��
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
	CloseHandle(hThread);//�ͷ����߳̾��//
	DeleteCriticalSection(&g_csMsgQueue);
	DeleteCriticalSection(&g_csMsg_two_Queue);
	return 0;
}
