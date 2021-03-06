//仅value支持中文输入。


#include "stdafx.h"

#define PI 3.141592653589793
#define CHORD_DLG_MIN_SIZE 500
#define MY_CLOSE 1111
struct DrawLinesParam
{
	HWND hDlg;
	std::vector<ChordNode*> node_route;
};

INT_PTR CALLBACK    DlgMain(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    DlgChord(HWND, UINT, WPARAM, LPARAM);

void ShowChordNode(HDC, ChordNode *, int);
void ShowKeyNode(HDC, KeyNode *, int);
void ShowAllNode(HDC,ChordRing *,int);

void ShowNodeMsg(HWND, ChordNode *, RECT);
void ShowKeyMsg(HWND, KeyNode *, RECT);
void DrawNodeRoute(PVOID);
std::wstring StringToWString(const std::string& str);
std::string  WStringToString(const std::wstring& ws);

double GetNodeAngle(ChordNode);
int Char2Int(char);


HWND g_chord_dlg;
int g_screen_length_x, g_screen_length_y;
std::map<RECT*, ChordNode*> chord_node_position;
std::map<RECT*, KeyNode*> key_node_position;
//static std::vector<ChordNode*> node_route;
TCHAR key[41], value[41],ip[40];

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgMain);
	return 0;
}


INT_PTR CALLBACK DlgMain(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	RECT main_dlg_rect;
	int chord_dlg_length;
	switch (message)
	{
	case WM_INITDIALOG:
		RECT ScreenRect;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &ScreenRect, 0);
		g_screen_length_x = ScreenRect.right - ScreenRect.left;
		g_screen_length_y = ScreenRect.bottom - ScreenRect.top;
		SetWindowPos(hDlg, HWND_TOPMOST, g_screen_length_y / 5, g_screen_length_x / 6, 0, 0, SWP_NOSIZE);

		g_chord_dlg = CreateDialog(0, MAKEINTRESOURCE(IDD_DIALOG2),NULL, DlgChord);	

		GetWindowRect(hDlg, &main_dlg_rect);
		chord_dlg_length = (main_dlg_rect.right - main_dlg_rect.left)*2;
		SetWindowPos(g_chord_dlg, HWND_BOTTOM, main_dlg_rect.right - 5, main_dlg_rect.top / 2,
			chord_dlg_length, chord_dlg_length,
			SWP_HIDEWINDOW);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{

		case IDC_SHOWCHORD:
			ShowWindow(g_chord_dlg, SW_SHOW);
			SetFocus(g_chord_dlg);
		//	SetFocus(hDlg);
			break;
		case IDC_HIDECHORD:
			ShowWindow(g_chord_dlg, SW_HIDE);
			break;
		case IDC_INSERT:
			GetDlgItemText(hDlg, IDC_INSERTKEY,key,40);
			GetDlgItemText(hDlg, IDC_INSERTVALUE,value,40);
			if (value[0] == '\0')
			{
				MessageBox(hDlg, L"The value can't be space!", L"Warning", MB_OK);
				break;
			}
			SendMessage(g_chord_dlg, WM_COMMAND, IDC_INSERT,NULL);

			SetDlgItemText(hDlg, IDC_INSERTKEY,NULL);
			SetDlgItemText(hDlg, IDC_INSERTVALUE, NULL);
			break;

		case IDC_UPDATE:
			GetDlgItemText(hDlg, IDC_UPDATEKEY, key, 40);
			GetDlgItemText(hDlg, IDC_UPDATEVALUE, value, 40);
			if (value[0] == '\0')
			{
				MessageBox(hDlg, L"The value can't be space!", L"Warning", MB_OK);
				break;
			}
			SendMessage(g_chord_dlg, WM_COMMAND, IDC_UPDATE, NULL);

			SetDlgItemText(hDlg, IDC_UPDATEKEY, NULL);
			SetDlgItemText(hDlg, IDC_UPDATEVALUE, NULL);
			break;
		case IDC_JOIN:
			GetDlgItemText(hDlg, IDC_NODEIP,ip, 40);
			SendMessage(g_chord_dlg, WM_COMMAND, IDC_JOIN, NULL);
			SetDlgItemText(hDlg, IDC_NODEIP, NULL);
			break;
		case IDC_LEAVE:
			GetDlgItemText(hDlg, IDC_NODEIP, ip, 40);
			SendMessage(g_chord_dlg, WM_COMMAND, IDC_LEAVE, NULL);
			SetDlgItemText(hDlg, IDC_NODEIP, NULL);
			break;
		case IDC_LOOKUP:
			GetDlgItemText(hDlg, IDC_LOOKUPKEY, key, 40);
			SendMessage(g_chord_dlg, WM_COMMAND, IDC_LOOKUP,(LPARAM)hDlg);
			SetDlgItemText(hDlg, IDC_LOOKUPKEY, NULL);
		case IDC_SETLOOKUPVALUE:
			SetDlgItemText(hDlg, IDC_LOOKUPVALUE,value);
		//	InvalidateRect(hDlg, NULL, true);
			break;
		case IDCANCEL :
			SendMessage(g_chord_dlg, MY_CLOSE, NULL, NULL);
			EndDialog(hDlg, LOWORD(wParam));
			DestroyWindow(g_chord_dlg);
			return (INT_PTR)TRUE;
		default:
			break;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


INT_PTR CALLBACK DlgChord(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static RECT *p_dlg_rect,msg_rect;
	static HDC hdc;
	static int client_length_x, client_length_y,client_length;
	static ChordRing *chord_ring;
	static POINT mouse_position;
	static bool node_or_key = false;
	static struct DrawLinesParam draw_lines_param;
	//HWND hDlgParent;
	//const TCHAR *t_key_value;
	PAINTSTRUCT ps;
	static std::vector<ChordNode*> node_route;
	std::wstring key_value;
	std::map<RECT*, ChordNode*>::iterator chord_node_iterator;
	std::map<RECT*, KeyNode*>::iterator key_node_iterator;
	
	switch (message)
	{
	case WM_INITDIALOG:
		chord_ring = new ChordRing();
		return (INT_PTR)TRUE;
	case WM_SIZING:
		p_dlg_rect = (RECT*)lParam;

		switch (wParam)
		{
		case WMSZ_TOP:
			if (p_dlg_rect->bottom - p_dlg_rect->top <= CHORD_DLG_MIN_SIZE)
				p_dlg_rect->top = p_dlg_rect->bottom - CHORD_DLG_MIN_SIZE;

			p_dlg_rect->right = p_dlg_rect->left + p_dlg_rect->bottom - p_dlg_rect->top;
			break;
		case WMSZ_BOTTOM:
			if (p_dlg_rect->bottom - p_dlg_rect->top <= CHORD_DLG_MIN_SIZE)
				p_dlg_rect->bottom = p_dlg_rect->top + CHORD_DLG_MIN_SIZE;

			p_dlg_rect->right = p_dlg_rect->left + p_dlg_rect->bottom - p_dlg_rect->top;
			break;

		case WMSZ_LEFT:
			if (p_dlg_rect->right - p_dlg_rect->left > g_screen_length_y)
				p_dlg_rect->left = p_dlg_rect->right - g_screen_length_y;
			if (p_dlg_rect->right - p_dlg_rect->left <= CHORD_DLG_MIN_SIZE)
				p_dlg_rect->left = p_dlg_rect->right - CHORD_DLG_MIN_SIZE;
			p_dlg_rect->bottom = p_dlg_rect->top + p_dlg_rect->right - p_dlg_rect->left;
		case WMSZ_RIGHT:
			if (p_dlg_rect->right - p_dlg_rect->left > g_screen_length_y)
				p_dlg_rect->right = p_dlg_rect->left + g_screen_length_y;
			if (p_dlg_rect->right - p_dlg_rect->left <= CHORD_DLG_MIN_SIZE)
				p_dlg_rect->right = p_dlg_rect->left + CHORD_DLG_MIN_SIZE;

			p_dlg_rect->bottom = p_dlg_rect->top + p_dlg_rect->right - p_dlg_rect->left;
			break;
		case WMSZ_BOTTOMLEFT:
			if (p_dlg_rect->bottom - p_dlg_rect->top <= CHORD_DLG_MIN_SIZE)
				p_dlg_rect->bottom = p_dlg_rect->top + CHORD_DLG_MIN_SIZE;
			if (p_dlg_rect->right - p_dlg_rect->left <= CHORD_DLG_MIN_SIZE)
				p_dlg_rect->left = p_dlg_rect->right - CHORD_DLG_MIN_SIZE;
			break;
		case WMSZ_BOTTOMRIGHT:
			if (p_dlg_rect->bottom - p_dlg_rect->top <= CHORD_DLG_MIN_SIZE)
				p_dlg_rect->bottom = p_dlg_rect->top + CHORD_DLG_MIN_SIZE;
			if (p_dlg_rect->right - p_dlg_rect->left <= CHORD_DLG_MIN_SIZE)
				p_dlg_rect->right = p_dlg_rect->left + CHORD_DLG_MIN_SIZE;
			break;
		case WMSZ_TOPLEFT:
			if (p_dlg_rect->bottom - p_dlg_rect->top <= CHORD_DLG_MIN_SIZE)
				p_dlg_rect->top = p_dlg_rect->bottom - CHORD_DLG_MIN_SIZE;
			if (p_dlg_rect->right - p_dlg_rect->left <= CHORD_DLG_MIN_SIZE)
				p_dlg_rect->left = p_dlg_rect->right - CHORD_DLG_MIN_SIZE;

		case WMSZ_TOPRIGHT:
			if (p_dlg_rect->bottom - p_dlg_rect->top <= CHORD_DLG_MIN_SIZE)
				p_dlg_rect->top = p_dlg_rect->bottom - CHORD_DLG_MIN_SIZE;
			if (p_dlg_rect->right - p_dlg_rect->left <= CHORD_DLG_MIN_SIZE)
				p_dlg_rect->right = p_dlg_rect->left + CHORD_DLG_MIN_SIZE;
		default:
			break;
		}
		break;
	case WM_SIZE:
		client_length_x = LOWORD(lParam);
		client_length_y = HIWORD(lParam);

		client_length = client_length_y;

		msg_rect.left = (double)client_length*0.2;
		msg_rect.right = (double)client_length*0.8;
		msg_rect.top = (double)client_length*0.2;
		msg_rect.bottom = (double)client_length*0.8;

		InvalidateRect(hDlg, NULL, TRUE);
		break;
	case WM_MOUSEMOVE:
		 
		mouse_position.x = LOWORD(lParam);
		mouse_position.y = HIWORD(lParam);
		chord_node_iterator = chord_node_position.begin();
		while (chord_node_iterator != chord_node_position.end())
		{
			if (PtInRect(chord_node_iterator->first, mouse_position))
			{
				node_route.erase(node_route.begin(), node_route.end());
				InvalidateRect(hDlg, NULL, true);
				break;
			}
			chord_node_iterator++;
		}

		key_node_iterator = key_node_position.begin();
		while (key_node_iterator != key_node_position.end())
		{
			if (PtInRect(key_node_iterator->first, mouse_position))
			{
				node_route.erase(node_route.begin(), node_route.end());
				InvalidateRect(hDlg, NULL, true);
				break;
			}
			key_node_iterator++;
		}
		//InvalidateRect(hDlg, &msg_rect, TRUE);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{

		case IDC_INSERT:
			//node_route.erase(node_route.begin(), node_route.end());
			if (chord_ring->Insert(WStringToString(key), value))
			{
				chord_ring->Lookup(WStringToString(key), node_route);
				InvalidateRect(hDlg, NULL, TRUE);
			}
			else
				MessageBox(hDlg, L"The Key is already existed!", L"Warning", MB_OK);
			break;
		case IDC_UPDATE:
			//node_route.erase(node_route.begin(), node_route.end());
			if (chord_ring->Update(WStringToString(key), value))
			{
				InvalidateRect(hDlg, NULL, TRUE);
				chord_ring->Lookup(WStringToString(key), node_route);
			}
			else
				MessageBox(hDlg, L"The Key is not existed!", L"Warning", MB_OK);
			break;
		case IDC_JOIN:
			if (chord_ring->Join(WStringToString(ip)))
			{
				InvalidateRect(hDlg, NULL, TRUE);
				chord_ring->Lookup(WStringToString(ip), node_route);
				node_route.erase(----node_route.end());
			}
			else
				MessageBox(hDlg, L"The Node is already existed!", L"Warning", MB_OK);
			break;

		case IDC_LEAVE:
			if (chord_ring->Leave(WStringToString(ip)))
			{
				chord_ring->Lookup(WStringToString(ip), node_route);
				InvalidateRect(hDlg, NULL, TRUE);
			}
			else
				MessageBox(hDlg, L"The Node is not existed!", L"Warning", MB_OK);
			break;
		case IDC_LOOKUP:
			wsprintf(value,L"");
			key_value=chord_ring->Lookup(WStringToString(key), node_route);
			if (key_value.size()!= 0)
			{
				SendMessage((HWND)lParam, WM_COMMAND, IDC_SETLOOKUPVALUE,0);
				wsprintf(value, key_value.data());
				InvalidateRect(hDlg, NULL, TRUE);
			}
			else
				MessageBox(hDlg, L"The Key is not existed!", L"Warning", MB_OK);
			break;
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		default:
			break;
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hDlg, &ps);
	//	SetMapMode(hdc, MM_ISOTROPIC);
	//	Ellipse(hdc, (double)client_length_x / 6, (double)client_length_y / 6,
	//		(double)client_length_x * 5 / 6, (double)client_length_y * 5 / 6);
		ShowAllNode(hdc, chord_ring, client_length);
		if (node_route.size() != 0)
		{
			draw_lines_param.hDlg = hDlg;
			draw_lines_param.node_route = node_route;
			_beginthread(DrawNodeRoute, 0, (PVOID)&draw_lines_param);
		}
		chord_node_iterator = chord_node_position.begin();
		node_or_key = false;
		while (chord_node_iterator != chord_node_position.end())
		{
			if (PtInRect(chord_node_iterator->first, mouse_position))
			{
				
				ShowNodeMsg(hDlg, chord_node_iterator->second, msg_rect);
			
				node_or_key = true;
				break;
			}
			chord_node_iterator++;
		}
		if (!node_or_key)
		{
			key_node_iterator = key_node_position.begin();
			while (key_node_iterator != key_node_position.end())
			{
				if (PtInRect(key_node_iterator->first, mouse_position))
				{
					ShowKeyMsg(hDlg, key_node_iterator->second, msg_rect);
		
					break;
				}
				key_node_iterator++;
			}
		}
		
		EndPaint(hDlg, &ps);
		break;
	case MY_CLOSE:
		delete chord_ring;
	case WM_CLOSE:
		break;
	}
	return (INT_PTR)FALSE;
}

std::wstring StringToWString(const std::string& str)
{
	std::wstring wstr(str.length(), L' ');
	std::copy(str.begin(), str.end(), wstr.begin());
	return wstr;
}
std::string  WStringToString(const std::wstring& wstr)
{
	std::string str(wstr.length(), ' ');
	std::copy(wstr.begin(), wstr.end(), str.begin());
	return str;
}

int Char2Int(char ch)
{
	if (ch >= '0'&&ch <= '9')
		return ch - '0';
	else
		return ch - 'a'+10;

}
double GetNodeAngle(std::string hash_key)
{
	double node_angle;
	long key_num = 0;
	int occupy[4] = { 4096,256,16,1 };
	for (int i = 0; i < 4; i++)
	{
		key_num += Char2Int(hash_key[i])*occupy[i];
	}
	node_angle = (double)key_num / 65536 * 2*PI;
	return PI / 2 - node_angle; //平面直角坐标系x轴正向开始
}


void ShowChordNode(HDC hdc, ChordNode *chord_node,int client_length)
{
	POINT chord_center;
	RECT *node_rect = new RECT;
	int circle_rad = (double)client_length / 2.2;
	double chord_length = 10;
	double node_angle = GetNodeAngle(chord_node->GetHashKey());
	chord_center.x = (double)client_length / 2;
	chord_center.y = (double)client_length / 2;
	node_rect->left = chord_center.x + (double)circle_rad*cos(node_angle) - chord_length;
	node_rect->top = chord_center.y - (double)circle_rad*sin(node_angle) - chord_length;
	node_rect->right = chord_center.x + (double)circle_rad*cos(node_angle) + chord_length;
	node_rect->bottom = chord_center.y - (double)circle_rad*sin(node_angle) + chord_length;
	Rectangle(hdc, node_rect->left, node_rect->top,
		node_rect->right, node_rect->bottom);
	chord_node_position[node_rect] = chord_node;
}

void ShowKeyNode(HDC hdc, KeyNode* key_node, int client_length)
{
	POINT chord_center;
	RECT *key_rect = new RECT;
	int circle_rad = (double)client_length / 2.2;
	double chord_length = 5;
	double node_angle = GetNodeAngle(key_node->GetHashKey());
	chord_center.x = (double)client_length / 2;
	chord_center.y = (double)client_length / 2;
	key_rect->left = chord_center.x + (double)circle_rad*cos(node_angle) - chord_length;
	key_rect->top = chord_center.y - (double)circle_rad*sin(node_angle) - 2*chord_length;
	key_rect->right = chord_center.x + (double)circle_rad*cos(node_angle) + chord_length;
	key_rect->bottom = chord_center.y - (double)circle_rad*sin(node_angle) +2* chord_length;
	Rectangle(hdc, key_rect->left, key_rect->top,
		key_rect->right, key_rect->bottom);
	
	key_node_position[key_rect] = key_node;
	
}

void ShowAllNode(HDC hdc, ChordRing *chord_ring,int client_length)
{
	key_node_position.erase(key_node_position.begin(), key_node_position.end());
	chord_node_position.erase(chord_node_position.begin(), chord_node_position.end());
	ChordNode *chord_node = chord_ring->GetFatherNode()->GetSuccessor();
	std::map<std::string, class KeyNode*>::iterator map_key_node;
	while (chord_node != chord_ring->GetFatherNode())
	{
		std::map<std::string, class KeyNode*> node_list = chord_node->GetHashKeyNodeList();
		std::map<std::string, class KeyNode*>::iterator map_key_node;
		map_key_node = node_list.begin();
		while (map_key_node != node_list.end())
		{
			ShowKeyNode(hdc, map_key_node->second, client_length);
			map_key_node++;
		}
		ShowChordNode(hdc, chord_node, client_length);
		chord_node = chord_node->GetSuccessor();
	}
}
void ShowNodeMsg(HWND hDlg, ChordNode *chord_node,RECT rect)
{
	char str_num[4];
	std::wstring chord_msg;
	ChordNode **finger_table = chord_node->GetFingerTable();
	HDC hdc = GetDC(hDlg);
	
	chord_msg += TEXT("IP:") + 
		StringToWString(chord_node->GetNodeIp());
	if (!chord_node->GetSuccessor()->GetNodeIp().compare("Ancestor"))
	{
		chord_msg += TEXT("   Successor:") +
			StringToWString(chord_node->GetSuccessor()->GetSuccessor()->GetNodeIp());
	}
	else
	{
		chord_msg += TEXT("   Successor:") +
			StringToWString(chord_node->GetSuccessor()->GetNodeIp());
	}
	
	chord_msg += TEXT("   Predecessor:   ") + 
		StringToWString(chord_node->GetPredecessor()->GetNodeIp());
	chord_msg+=TEXT("   Finger Table:   ");
	for (int i = 0; i < HASH_BIT; i++)
	{
		sprintf_s(str_num,4, "%d", i+1);
		chord_msg += TEXT("  ")+StringToWString(str_num)+TEXT(":")+
			StringToWString(finger_table[i]->GetNodeIp());
	}
	//SelectObject(hdc, (HGDIOBJ)GetClassLong(hDlg, GCL_HBRBACKGROUND));
	SetBkMode(hdc, TRANSPARENT);
	//Rectangle(hdc, rect.left, rect.top,rect.right, rect.bottom);
	DrawText(hdc,chord_msg.data(),-1, &rect, DT_LEFT| DT_WORDBREAK);
	ReleaseDC(hDlg, hdc);
}

void ShowKeyMsg(HWND hDlg, KeyNode *key_node, RECT rect)
{
	char str_num[4];
	std::wstring key_msg;
	HDC hdc = GetDC(hDlg);

	key_msg += TEXT("Key:") +StringToWString(key_node->GetKey());
	key_msg += TEXT("\nValue:") + key_node->GetValue();
	//SelectObject(hdc, (HGDIOBJ)GetClassLong(hDlg, GCL_HBRBACKGROUND));
	SetBkMode(hdc, TRANSPARENT);
	//Rectangle(hdc, rect.left, rect.top, rect.left+200, rect.top+33);
	DrawText(hdc, key_msg.data(), -1, &rect, DT_LEFT | DT_WORDBREAK);
	ReleaseDC(hDlg, hdc);
}
void DrawNodeRoute(PVOID draw_lines_param)
{
	HWND hDlg = ((DrawLinesParam*)draw_lines_param)->hDlg;
	std::vector<ChordNode*> node_route = ((DrawLinesParam*)draw_lines_param)->node_route;
	bool first_move = true;
	std::map<RECT*, ChordNode*>::iterator chord_node_iterator;
	RECT rect;
	HDC hdc = GetDC(hDlg);
	
	//chord_node_iterator = chord_node_position.begin();
	for (auto node : node_route)
	{
		for(auto chord_node:chord_node_position)
		{
			if (chord_node.second == node)
			{
				rect = *chord_node.first;
				if (first_move)
				{
					MoveToEx(hdc, (rect.left + rect.right) / 2,
						(rect.top + rect.bottom) / 2, NULL);
					first_move = 0;
				}
				//SelectObject(hdc,getsolid)
				LineTo(hdc, (rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2);
				Sleep(100);
			}
		}
	}
	ReleaseDC(hDlg, hdc);
}
