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
void HandleClient_login(SOCKET client_server);//��¼�����߳�//
void HandleClient_passwordset(SOCKET client_server);
void HandleClient_photoset(SOCKET client_server);
void Handlelogin_pro(SOCKET client_server, receivedData my_data);
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
std::vector<Userdata> g_users;

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
			return (INT_PTR)TRUE;
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

		return (INT_PTR)TRUE;
	}
	case  WM_APP_UPDATEBROADCAST_MSG:          //�����û��б���Ϣ//
	{
		HWND hList = GetDlgItem(hwndDlg, IDC_USERS);//��ȡ�û��б���
		SendMessage(hList, LB_RESETCONTENT, 0, 0);//����ı���//
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
			mis->itemHeight = 72;//����IDC_USERS����߶�Ϊ72//
		}
		return (INT_PTR)TRUE;
	}
	case WM_DRAWITEM:
	{
		LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;//ָ��һ��DRAWITEMSTRUCT�ṹ���ָ��//
		if (dis->CtlID == IDC_USERS && dis->itemID < g_users.size())
		{
			std::lock_guard<std::mutex> lock(g_usersMutex);
			HDC hdc = dis->hDC;//��ͼ�豸����//
			RECT rc = dis->rcItem;//����Ƶ���ľ�������//
			size_t idx = dis->itemData;
			const Userdata& user = g_users[idx];

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

			/*
			//���ǳ�//
			RECT rcText = rc;
			rcText.left += imgsize + 8;
			rcText.top += 12;
			int wlen = MultiByteToWideChar(CP_UTF8,0,user.nickname.c_str(),user.nickname.size()+1,NULL,0);
			std::wstring wnick(wlen, 0);
			MultiByteToWideChar(CP_UTF8, 0, user.nickname.c_str(), user.nickname.size() + 1,&wnick[0], wlen);//���ı�ת��Ϊ���ַ���//
			//DrawTextW(hdc,wnick.c_str(),wlen,&rcText,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
			DrawTextW(hdc, wnick.c_str(), wlen, &rcText, DT_LEFT | DT_TOP|DT_SINGLELINE);

			
			// �����ǳƸ߶�
			DrawTextW(hdc, wnick.c_str(), wlen, &rcText, DT_LEFT | DT_TOP|DT_SINGLELINE| DT_CALCRECT);

			//���˺�//
			RECT rcAccount = rcText;
			rcAccount.top =rcText.bottom+8;
			rcAccount.bottom = rcAccount.top + (rcText.bottom - rcText.top); //����rcAccount.bottom//
			int wlen2 = MultiByteToWideChar(CP_UTF8, 0, user.nickname.c_str(), user.nickname.size() + 1, NULL, 0);
			std::wstring wnick_2(wlen2, 0);
			MultiByteToWideChar(CP_UTF8, 0, user.nickname.c_str(), user.nickname.size() + 1, &wnick_2[0], wlen2);//���ı�ת��Ϊ���ַ���//
			DrawTextW(hdc, wnick_2.c_str(), wlen2, &rcAccount, DT_LEFT | DT_TOP|DT_SINGLELINE);
			*/

			//���˺�//
			RECT rcAccount = rc;
			rcAccount.left += imgsize + 8;
			rcAccount.top += 12;
			int wlen = MultiByteToWideChar(CP_UTF8, 0, user.account.c_str(), user.account.size() + 1, NULL, 0);
			std::wstring wnick(wlen, 0);
			MultiByteToWideChar(CP_UTF8, 0, user.account.c_str(), user.account.size() + 1, &wnick[0], wlen);//���ı�ת��Ϊ���ַ���//
			//DrawTextW(hdc,wnick.c_str(),wlen,&rcText,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
			DrawTextW(hdc, wnick.c_str(), wlen, &rcAccount, DT_LEFT | DT_TOP | DT_SINGLELINE);


			// �����˺Ÿ߶�
			DrawTextW(hdc, wnick.c_str(), wlen, &rcAccount, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_CALCRECT);

			//���ǳ�//
			RECT rcText = rcAccount;
			rcText.top = rcAccount.bottom + 8;
			rcText.bottom = rcText.top + (rcAccount.bottom - rcAccount.top); //����rcText.bottom//
			int wlen2 = MultiByteToWideChar(CP_UTF8, 0, user.nickname.c_str(), user.nickname.size() + 1, NULL, 0);
			std::wstring wnick_2(wlen2, 0);
			MultiByteToWideChar(CP_UTF8, 0, user.nickname.c_str(), user.nickname.size() + 1, &wnick_2[0], wlen2);//���ı�ת��Ϊ���ַ���//
			DrawTextW(hdc, wnick_2.c_str(), wlen2, &rcText, DT_LEFT | DT_TOP | DT_SINGLELINE);


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


void HandleClient_register(SOCKET client_server)//ע�ᴦ���߳�//
{
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
			std::thread(HandleClient_login, client_server).detach();//������¼�߳�//
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
						std::lock_guard<std::mutex>lk(users_mutex);
						users_update_signal = true;
						users_cv.notify_one();//֪ͨ�������¼������ݿ�����û��ǳƺ�ͷ��//

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
							std::thread(HandleClient_login, client_server).detach();//������¼�߳�//
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
					std::lock_guard<std::mutex>lk(users_mutex);
					users_update_signal = true;
					users_cv.notify_one();//֪ͨ�������¼������ݿ�����û��ǳƺ�ͷ��//

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
						std::thread(HandleClient_login, client_server).detach();//������¼�߳�//
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
				std::lock_guard<std::mutex>lk(users_mutex);
				users_update_signal = true;
				users_cv.notify_one();//֪ͨ�������¼������ݿ�����û��ǳƺ�ͷ��//

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
					std::thread(HandleClient_login, client_server).detach();//������¼�߳�//
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
			std::lock_guard<std::mutex>lk(users_mutex);
			users_update_signal = true;
			users_cv.notify_one();//֪ͨ�������¼������ݿ�����û��ǳƺ�ͷ��//

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
				std::thread(HandleClient_login, client_server).detach();//������¼�߳�//
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

















//�û��ǳƺ�ͷ���ѯ���//
void LoadUsersFromDB(MYSQL* conn)
{
	while (true)
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











void HandleClient_login(SOCKET client_server)//��¼�����߳�//
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
			MessageBox(NULL, L"��ѯ�����˺������Ѿ�ע��", L"QQ", MB_ICONINFORMATION);
			std::wstring recv_back = L"sucess";
			int utf8_len = WideCharToMultiByte(CP_UTF8, 0, recv_back.c_str(), recv_back.size() + 1, NULL, 0, NULL, NULL);//ת��Ϊutf8����Ŀռ�//
			std::string utf8str(utf8_len, 0);//����ռ�//
			WideCharToMultiByte(CP_UTF8, 0, recv_back.c_str(), recv_back.size() + 1, &utf8str[0], utf8_len, NULL, NULL);//ʵ��ת��//
			send(client_server, (char*)&utf8_len, sizeof(utf8_len), 0);//�ȷ�����//
			send(client_server, utf8str.c_str(), utf8_len, 0);//������//
			//�����¼��ҳ��֮����߳�//
			MessageBox(NULL, L"���������¼����Ĵ����߳�", L"QQ", MB_ICONINFORMATION);
			std::thread(Handlelogin_pro,client_server,my_data).detach();
			





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
			std::thread(HandleClient_login, client_server).detach();//��¼�����߳�//
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
			std::thread(HandleClient_login, client_server).detach();//������¼�߳�//
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


//��¼���洦���߳�//
void Handlelogin_pro(SOCKET client_server,receivedData my_data)//��¼���洦���߳�//
{
	MessageBox(NULL, L"�ɹ������¼����Ĵ����߳�", L"QQ", MB_ICONINFORMATION);

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
			if (len_utf8<= 0)
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
				
				std::wstring msg_one=L"[";
				msg_one+=timeBuffer_one;
				msg_one+= L"][";
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
				int u_len = WideCharToMultiByte(CP_UTF8,0,msg_one.c_str(),msg_one.size()+1, NULL, 0, NULL, NULL);//����ת������//
				//std::wstring num = std::to_wstring(u_len);//������ת��Ϊ���ַ���//
				//MessageBox(NULL,num.c_str(), L"QQ", MB_ICONINFORMATION);
				std::string u_str(u_len, 0);//����ռ�//
				WideCharToMultiByte(CP_UTF8, 0, msg_one.c_str(), msg_one.size() + 1, &u_str[0], u_len, NULL, NULL);//ʵ��ת��//
				
				send(client_server,(char*)&u_len,sizeof(u_len),0);//�ȷ�����//
				send(client_server, u_str.c_str(), u_len, 0);//������//

				EnterCriticalSection(&g_csMsg_two_Queue);//����//
				g_msg_two_Queue.push(msg_one);//��������//
				LeaveCriticalSection(&g_csMsg_two_Queue);//����//
				//MessageBox(NULL, L"�Ѿ��ɹ�����Ϣƴ��", L"QQ", MB_ICONINFORMATION);
				// ��¼�������������//
				PostMessage(g_hInfoDialogmonitor, WM_APP_UPDATEMONITOR_MSG, 0, 0);
				//MessageBox(NULL, L"�Ѿ��ɹ�֪ͨ��Ϣ�ļ����ʾ", L"QQ", MB_ICONINFORMATION);
				
			}
		}
		
		std::thread(Handlelogin_pro, client_server, my_data).detach();//���ص�¼�׽��洦���߳�//
	}
	else if (wcscmp(wstr.c_str(),L"������")==0)
	{

	}
	else if (wcscmp(wstr.c_str(),L"����")==0)
	{

	}
	else if (wcscmp(wstr.c_str(),L"������Ϣ")==0)
	{

	}
	else if (wcscmp(wstr.c_str(),L"����ͷ��")==0)
	{

	}
	else if (wcscmp(wstr.c_str(),L"����")==0)
	{

	}
	else if (wcscmp(wstr.c_str(),L"�˳�")==0)
	{

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
				//���ص�¼ע��ѡ�����߳�//
				//�жϾ���ִ�������߳�,��¼��ע��//
				std::lock_guard<std::mutex>lk(users_mutex);
                users_update_signal =true;//�����û��б��־//
				users_cv.notify_one();//֪ͨ����ִ�и����û��б�//

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
					std::thread(HandleClient_login, client_server).detach();//������¼�߳�//
				}
				else
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
			std::thread(Handlelogin_pro,client_server,my_data).detach();//���ص�½���洦���߳�//
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
			std::thread(HandleClient_login, client_server).detach();//������¼�߳�//
		}
		else
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
	//��������Ϊusers//
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
		DialogBox(hinstance, MAKEINTRESOURCE(IDD_MYDIALOG_END), NULL, Dialog_end);//������//
	}
	// ����Ϣѭ��
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
	CloseHandle(hThread);//�ͷ����߳̾��//
	DeleteCriticalSection(&g_csMsgQueue);
	DeleteCriticalSection(&g_csMsg_two_Queue);
	return 0;
}
