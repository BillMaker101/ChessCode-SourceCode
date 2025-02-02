// ChessCode.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "ChessCode.h"

// Declarations of external functions
extern bool InitGraphicsCode();
extern void EndSearch();
extern void InitChessboard();

// Global Variables:
HWND hWnd;
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
bool   exiting = false;   // wait until user exits program
bool   gameover = false;   // a flag for ending a game
int	BmpWidth = 84;			// initialize the bmp width to the larger square size
int	BoardDim = BmpWidth * 8;	// chess board is 8 squares * square dimension
int ScreenHeight, CharHeight;

HCURSOR 	hPrevCursor;
int		com = wht;	// color on the move: white = 0, black = 1
bool		startedmove = false;
bool		endsearch = false;
int		FromSqr, ToSqr, PrevTo;
int		win_left, win_top;

SQUARE Brd[64];  //  chessboard

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
// forward declarations of the game functions
void	RunChessCode();
void	EndChessCode();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CHESSCODE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    ScreenHeight = GetSystemMetrics(SM_CYFULLSCREEN);
    CharHeight = GetSystemMetrics(SM_CYCAPTION);
    if (ScreenHeight < MINHEIGHT) BmpWidth = SBMPWIDTH;
    else 			        BmpWidth = LBMPWIDTH;
    BoardDim = BmpWidth * 8;

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }
    // initialize the game
    InitChessboard();
    if (!InitGraphicsCode()) return 0;

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CHESSCODE));

    MSG msg;

    // Main message loop:
    while (!exiting)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        // process game loop
        RunChessCode();
    }
    EndChessCode();
    return 0;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = 0;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CHESSCODE));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_CHESSCODE);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindowW(szWindowClass, szTitle, WS_CAPTION | WS_SYSMENU,
      0, 0, HBOARDOFFSET + BoardDim + BmpWidth, BoardDim + 2 * BmpWidth,
      nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            RECT		Rect;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            // create the chess board area
            //Rect = { 4 + HBOARDOFFSET,
                //0,
                //4 + HBOARDOFFSET + BoardDim,
                //0 + BoardDim };
            //DrawFocusRect(hdc, &Rect);
            // put 20 lines of text in the area for the scoresheet for now
            for (int n = 0; n < 20; n++)
            {
                int	x = 4;
                int	y = n * 20 + 4;
                Rect = { x, y, x + 210, y + 40 };
                DrawText(hdc, szTitle, TITLELENGTH, &Rect, DT_LEFT);
                DrawText(hdc, szTitle, TITLELENGTH, &Rect, DT_RIGHT);
            }
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        gameover = true;
        exiting = true;
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

void	RunChessCode()
{
    if (gameover == true) return;
    while (!gameover)    //  the program loops until a game ends
    {
        // PeekMessage() is called in EndSearch() to respond to input
     // from the user, via the mouse
        EndSearch();     // endsearch is set to true when search ends
        if (!endsearch)  // after human has moved, you come back here
        {
            // SearchTree(); // periodically checks for the end of search,
            // when found, returns from SearchTree()
        }
        else // endsearch == true
        {
            // toggle ‘color on the move’ (com) when move search has ended
         // if white to move, com = 0.  if black to move, com = 1.
            if (endsearch)
            {
                com ^= 1;
                endsearch = false;
            }
        }
    }
    // gets here on gameover = TRUE
}

// shutdown program
void	EndChessCode()
{
    return;
}
