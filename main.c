#include <windows.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <process.h>
#include "pope.h"
#include "resource.h"

#define PROGRAM_NAME "PopEvents"
#define TIMER_ID     (100) 
#define TIMER_ELAPSE (60*1000)
#define WM_SYSTRAY (WM_USER + 1)
#define WM_SYSTRAY2 (WM_USER + 2)
#define IDR_MAINFRAME 1
#define ID_EXIT 0x20
#define ID_ABOUT 0x30

typedef struct {
  char *s1; /* shorter string */
  char *s2; /* longer string */
} two_strings;



HINSTANCE g_hinst;
static HMENU systray_menu;
static HWND aboutbox;
static HWND g_hwnd;

int program_running(void)
{
  HWND hwnd;
  hwnd = FindWindow(PROGRAM_NAME, PROGRAM_NAME);
  
  return (hwnd == NULL) ? 0 : 1;
}

int compare(const void *a, const void *b)
{
  return ((tuple *)a)->val - ((tuple *)b)->val;
}


#define TITLE_SIZE 250

DWORD WINAPI thread(LPVOID lpParam)
{
  char title[TITLE_SIZE];
  char message[MSG_SIZE];


  /*
  title = ((two_strings *) lpParam)->s1;
  message = ((two_strings *) lpParam)->s2; */

  strcpy(title, ((two_strings *) lpParam)->s1);
  strcpy(message, ((two_strings *) lpParam)->s2);

  if (strstr(title, "MISSED"))
    MessageBox(NULL, TEXT(message), TEXT(title), MB_OK | MB_ICONQUESTION);   
  else
    MessageBox(NULL, TEXT(message), TEXT(title), MB_OK | MB_ICONINFORMATION);   

  free(((two_strings *) lpParam)->s1);
  free(((two_strings *) lpParam)->s2);

  return 0;
}


void show_message(char *title, char *msg)
{
  two_strings message;
  HANDLE hthread = 0;

  message.s1 = (char *) xmalloc(TITLE_SIZE);
  message.s2 = (char *) xmalloc(MSG_SIZE);
  strcpy(message.s1, title);
  strcpy(message.s2, msg);

  hthread = CreateThread(NULL, 0, thread, &message, 0, NULL);

  if (hthread == NULL)
    ExitProcess(1);
  Sleep(1000);
  CloseHandle(hthread); 
}


/* 
 * r -- points to a list of records
 * n -- number of records
 *
 * Go through the record list, notify (by showing a message box) whenever
 * applicable, update record data structure accordingly
 */
void notify_windows(record *r, int n)
{
  int i, j;
  int *p;
  time_t curtime, futtime;
  struct tm now;
  double diff, low, high;
  char date[DATE_SIZE];  /* to hold the ascii description of date */
  char title[TITLE_SIZE];
  char message[MSG_SIZE];
  tuple notify[NOTIFY_SIZE];

  curtime = time(NULL);
  now = *localtime(&curtime);

  for (i = 0; i < n; i++) {
    /* printf("\nChecking record %d\n", i); */
    futtime = mktime(&(r[i].time));
    strcpy(date, asctime(&(r[i].time)));
    date[strlen(date)-1] = '\0'; 
    printf("Now: %s", asctime(&now));
    /* printf("Fut: %s", ctime(&futtime));  */
    diff = difftime(futtime, curtime); /* future time - current time (in secs) */
    if (diff < 0) { /* future time is less than current time, possibly
		       hapening when you mistakenly set a future time,
		       or the program was not running at that future time. */
      printf("Missed: %s [%s]\n", date, r[i].msg);    
      sprintf(title, "%s [MISSED]", r[i].msg);  
      sprintf(message, "%s\n\n\n\n%s\n\n\n", date, r[i].msg);
      /* MessageBox(NULL, TEXT(message), "Missed", MB_OK | MB_ICONWARNING);  */
      show_message(title , message);
      for (j = 0; j < r[i].num_pending; j++) { /* set all notified flags to 1 after showing the message box */
	p = &(r[i].notified[j]);
	*p = 1;	  
      }
    } else {
      for (j = 0; j < r[i].num_pending; j++) {
	notify[j].val = r[i].notify[j];
	notify[j].ord = j;
      }
      qsort(notify, r[i].num_pending, sizeof(tuple), compare);
      for (j = 0; j < r[i].num_pending; j++) {
	if (j > 0)
	  low = r[i].notify[notify[j-1].ord];
	else
	  low = 0;
	high = r[i].notify[notify[j].ord];
	/* if time diff is in the interval and it has not been notified yet */
	if (diff < high && diff >= low && r[i].notified[notify[j].ord] == 0) { 
	  sprintf(title, "%s", r[i].msg);  
	  sprintf(message, "%s\n\n\n\n%s\n\n\n", date, r[i].msg);
	  /* MessageBox(NULL, TEXT(message), "Remember", MB_OK | MB_ICONINFORMATION);  */
	  show_message(title, message); 
	  p = &(r[i].notified[notify[j].ord]);
	  *p = 1; /* set the notified flag to 1 */
	}
      }
    }
  }
}


/* int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow ) */
/* { */

/*   record *r; */
/*   int num; */

/*   g_hinst = hInstance;  */

/*   while (1) { */
/*     r = parse_file(EVENT_FILE, &num); */
/*     notify_windows(r, num); */
/*     update_file(EVENT_FILE, r, num); */
/*     free(r); */
/*     Sleep(10000); */
/*   } */
/*   return 0; */
/* } */



static int CALLBACK AboutProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
      case WM_INITDIALOG:
        { /* centre the window */
	  RECT rs, rd;
	  HWND hw;
	  
	  hw = GetDesktopWindow();
	  if (GetWindowRect(hw, &rs) && GetWindowRect(hwnd, &rd))
	    MoveWindow(hwnd,
		       (rs.right + rs.left + rd.left - rd.right) / 2,
		       (rs.bottom + rs.top + rd.top - rd.bottom) / 2,
		       rd.right - rd.left, rd.bottom - rd.top, TRUE);
        }
        return 1;
      case WM_COMMAND:
        switch (LOWORD(wParam)) {
          case IDOK:
          case IDCANCEL:
	    DestroyWindow(hwnd);
            return 0;
        }
        return 0;
      case WM_CLOSE:
	DestroyWindow(hwnd);
        return 0;
    }
    return 0;
}



LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  record *r;
  int num;
  static int menuinprogress;

  switch (msg) {
  case WM_CREATE:
    {
      systray_menu = CreatePopupMenu();
      AppendMenu(systray_menu, MF_STRING, ID_ABOUT, "U&sage");
      AppendMenu(systray_menu, MF_STRING, ID_EXIT, "E&xit");
      SetMenu(hwnd, systray_menu);
    }
    break;
  case WM_COMMAND:
    switch(LOWORD(wParam)) {
    case ID_EXIT:
      SendMessage(hwnd, WM_DESTROY, 0, 0);
      break;
    case ID_ABOUT:
      {
	aboutbox = CreateDialog(g_hinst, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, AboutProc);
	ShowWindow(aboutbox, SW_SHOW);
	SetForegroundWindow(aboutbox);
	SetWindowPos(aboutbox, HWND_TOP, 0, 0, 0, 0,
		     SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
	
      }
      break;
    default:
      break;
    } 
    break;
  case WM_SYSCOMMAND:
    break;
  case WM_TIMER:
    r = parse_file(EVENT_FILE, &num);
    notify_windows(r, num);
    update_file(EVENT_FILE, r, num);
    free(r);
    break;
  case WM_SYSTRAY:
    switch (LOWORD(lParam)) {
    case WM_LBUTTONDOWN:
      if (ShellExecute(NULL, "open", "file.txt", NULL, NULL, SW_SHOWNORMAL) <= (HINSTANCE) 32) {
	MessageBox(NULL, "Failed to open file.txt.  Creating an empty one for your.", PROGRAM_NAME, MB_OK| MB_ICONEXCLAMATION);
	{ /* create a file */
	  HANDLE hFile;
	  DWORD dwBytesWritten = 0;
	  const char format[] = "% Remove % from the following lines to take effect:\r\n"
	    "% 2012..01.30 10am; class\r\n"
	    "% 1.30 7:23pm; test\r\n"
	    "% 7:30pm; dinner\r\n";
	  DWORD dwBytesToWrite = (DWORD) strlen(format);

	  hFile = CreateFile("file.txt", GENERIC_WRITE, 0, NULL, CREATE_NEW, 
			     FILE_ATTRIBUTE_NORMAL, NULL);

	  if (hFile == INVALID_HANDLE_VALUE) 
	    MessageBox(NULL, "Failed to create file.txt.  You can create it manually.", PROGRAM_NAME, 
		       MB_OK | MB_ICONERROR);
	  WriteFile(hFile, format, dwBytesToWrite, &dwBytesWritten, NULL);
	  CloseHandle(hFile);
	}
      }
      break;
    case WM_RBUTTONDOWN:
      /* PostQuitMessage(0); */
      /* SendMessage(hwnd, WM_DESTROY, 0, 0); */
      {
	POINT cursor_pos;
	GetCursorPos(&cursor_pos);
	PostMessage(hwnd, WM_SYSTRAY2, cursor_pos.x, cursor_pos.y);
      }
      break;
    default:
      break;
    }
    break;
  case WM_SYSTRAY2:
    if (!menuinprogress) {
      menuinprogress = 1;
      SetForegroundWindow(hwnd);
      TrackPopupMenu(systray_menu,
		     TPM_RIGHTALIGN | TPM_BOTTOMALIGN |
		     TPM_RIGHTBUTTON,
		     wParam, lParam, 0, hwnd, NULL);
      menuinprogress = 0;
    }
    break;
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  }
  return DefWindowProc(hwnd, msg, wParam, lParam);
}


/*
 * Main function
 */
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
		    LPSTR lpCmdLine, int nCmdShow )
{
    MSG  msg ;
    WNDCLASS wc = {0};
    HWND hwnd;
    NOTIFYICONDATA tnd;

    wc.style         = 0;
    wc.lpfnWndProc   = WndProc ;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance ;
    wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MYICON));
    wc.hCursor       = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = PROGRAM_NAME;

    if (program_running()) {
      MessageBox(NULL, "The program is already running.\n"
		 "Right click the icon on the system tray to kill it.\n", "PopE Error",
		 MB_ICONERROR | MB_OK);
      return 0;
    }

    g_hinst = hInstance;
        
    if (!RegisterClass(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error!",
                   MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    hwnd = CreateWindow( wc.lpszClassName, PROGRAM_NAME,
			 WS_OVERLAPPEDWINDOW | WS_VSCROLL,
			 CW_USEDEFAULT, CW_USEDEFAULT, 
			 100, 100, NULL, NULL, hInstance, NULL);

    g_hwnd = hwnd;

    /* move stuff to system tray */
    tnd.cbSize = sizeof(NOTIFYICONDATA);
    tnd.hWnd = hwnd;
    tnd.uID = IDR_MAINFRAME; 
    tnd.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    tnd.uCallbackMessage = WM_SYSTRAY;
    /* tnd.hIcon = (HICON) LoadImage(NULL, "myicon.ico", IMAGE_ICON, 32, 32, LR_LOADFROMFILE); */
    tnd.hIcon = (HICON) LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MYICON));
    strcpy(tnd.szTip,"PopE (Pop Events)");
    Shell_NotifyIcon(NIM_ADD,&tnd);
    if (tnd.hIcon) DestroyIcon(tnd.hIcon);


    ShowWindow(hwnd, SW_HIDE);

    SetTimer(hwnd, TIMER_ID, TIMER_ELAPSE, NULL);
    
    while(GetMessage(&msg, NULL, 0, 0)) {
      if (!IsDialogMessage(aboutbox, &msg)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }

    KillTimer(hwnd, TIMER_ID);
    Shell_NotifyIcon(NIM_DELETE,&tnd); /* remove system tray icon */

    return (int) msg.wParam;
}
