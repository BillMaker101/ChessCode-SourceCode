#include "framework.h"
#include "ChessCode.h"

extern HWND hWnd;
extern int BmpWidth;
extern HCURSOR 	hPrevCursor;
extern int		com;	// color on the move: white = 0, black = 1
extern bool		startedmove;
extern bool		endsearch;
extern int		FromSqr, ToSqr, PrevTo;
extern int		win_left, win_top;
extern int ScreenHeight, CharHeight;
extern SQUARE Brd[64];

static HBITMAP hSlideSprite, hSlideMask, hPrevBmp;
static POINT	CurrentPt, PreviousPt;
static HDC	hPrevDC;
static bool	sliding = false;

HGDIOBJ	 hReplacedObj;		// set & release the device context
HGDIOBJ LtPaint, DrkPaint;	// hold colors for the paint brush

WCHAR Swp[8] = L"swp.bmp";
WCHAR Swn[8] = L"swn.bmp";
WCHAR Swb[8] = L"swb.bmp";
WCHAR Swr[8] = L"swr.bmp";
WCHAR Swq[8] = L"swq.bmp";
WCHAR Swk[8] = L"swk.bmp";

WCHAR Sbp[8] = L"sbp.bmp";
WCHAR Sbn[8] = L"sbn.bmp";
WCHAR Sbb[8] = L"sbb.bmp";
WCHAR Sbr[8] = L"sbr.bmp";
WCHAR Sbq[8] = L"sbq.bmp";
WCHAR Sbk[8] = L"sbk.bmp";

WCHAR Spm[8] = L"spm.bmp";
WCHAR Snm[8] = L"snm.bmp";
WCHAR Sbm[8] = L"sbm.bmp";
WCHAR Srm[8] = L"srm.bmp";
WCHAR Sqm[8] = L"sqm.bmp";
WCHAR Skm[8] = L"skm.bmp";

WCHAR Lwp[8] = L"lwp.bmp";
WCHAR Lwn[8] = L"lwn.bmp";
WCHAR Lwb[8] = L"lwb.bmp";
WCHAR Lwr[8] = L"lwr.bmp";
WCHAR Lwq[8] = L"lwq.bmp";
WCHAR Lwk[8] = L"lwk.bmp";

WCHAR Lbp[8] = L"lbp.bmp";
WCHAR Lbn[8] = L"lbn.bmp";
WCHAR Lbb[8] = L"lbb.bmp";
WCHAR Lbr[8] = L"lbr.bmp";
WCHAR Lbq[8] = L"lbq.bmp";
WCHAR Lbk[8] = L"lbk.bmp";

WCHAR Lpm[8] = L"lpm.bmp";
WCHAR Lnm[8] = L"lnm.bmp";
WCHAR Lbm[8] = L"lbm.bmp";
WCHAR Lrm[8] = L"lrm.bmp";
WCHAR Lqm[8] = L"lqm.bmp";
WCHAR Lkm[8] = L"lkm.bmp";

HBITMAP Sprites[6][2];  // 6 piece types, both wht and blk
HBITMAP Masks[6];       // one mask per piece type

WCHAR TouchMv[14] = L"Touch move ? ";
WCHAR Warning[20] = L"Error entering move";
// takes 34 spaces to cover the 19 character message above!
WCHAR Erasing[35] = L"                                  ";


//
// locate the top left corner (point) of a square (0 - 63) or (0x00 – 0x3f)
//
POINT LocatePt(int sqr)
{
	POINT pt{};

	pt.x = (7 - (sqr & 7)) * BmpWidth + HBOARDOFFSET;
	pt.y = (7 - (sqr >> 3)) * BmpWidth;
	return pt;
}

//
// displays an empty square of the chess board
//
void ClrSquare(int sqr)
{
	POINT pt;
	HDC   hDC;   // handle for device context

	hDC = GetDC(hWnd);  // get the device context for the main window

	pt = LocatePt(sqr);

	// to get the color of the square, either dark or light,
	// add file + rank, then look at least significant bit
	if ((((sqr & 7) + (sqr >> 3)) & 1) == 0)
		hReplacedObj = SelectObject(hDC, LtPaint);
	else
		hReplacedObj = SelectObject(hDC, DrkPaint);
	// color the square
	PatBlt(hDC, pt.x, pt.y, BmpWidth, BmpWidth, PATCOPY);
	// select original paint brush
	SelectObject(hDC, hReplacedObj);
	// delete paint brush
	DeleteObject(hReplacedObj);
	ReleaseDC(hWnd, hDC);  	// release the device context
}

//
//  set the pieces on the chess board
//
void InitChessboard(void)
{
	PIECE WPcs[8] = { wr, wn, wb, wk, wq, wb, wn, wr };   // initial setup
	int sq;

	// First clear the board
	for (sq = 0; sq <= 63; sq++)
	{
		Brd[sq].pce = nil;	// clear each square
	}

	// Next place the pieces in initial setup
	for (sq = 0; sq < 8; sq++)
	{
		Brd[sq].pce = WPcs[sq];
		Brd[sq + 8].pce = wp;
		Brd[sq + 48].pce = bp;
		Brd[sq + 56].pce = (PIECE)(WPcs[sq] | 1);
	}
}

//
// displays the input chess piece on the square
//  specified with the input point
//
void DisplayPiece(POINT pt, PIECE pce, DWORD pixels)
{
	HBITMAP hPieceBmp, hReplacedObj, hMaskBmp;
	BITMAP	PieceBmp{};
	HDC hDC, hMemoryDC;
	int StoredBytes;
	COLOR cl;
	PIECE pc;

	if (pce == nil) return;

	cl = (COLOR)(pce & 1);
	pc = (PIECE)(pce ^ 1);

	// get the piece bmp image handle
	hPieceBmp = Sprites[(pc >> 1) - 1][cl];
	// get the mask bmp image handle
	hMaskBmp = Masks[(pc >> 1) - 1];

	hDC = GetDC(hWnd);  // get the device context for the main window

	// create a 2nd display context
	hMemoryDC = CreateCompatibleDC(hDC);
	// initialize the piece bmp structure so you can use width and height
	StoredBytes = GetObject(hPieceBmp, sizeof(BITMAP), (LPSTR)&PieceBmp);
	// select an object (mask) into the specified device context (2nd DC)
	hReplacedObj = (HBITMAP)SelectObject(hMemoryDC, hMaskBmp);
	// Blt the mask which is in hMemoryDC into the device DC (same width
	//  and height as the piece bmp).
	BitBlt(hDC, pt.x, pt.y, PieceBmp.bmWidth, PieceBmp.bmHeight,
		hMemoryDC, 0, 0, SRCAND);
	// select original object into the specified device context (2nd DC)
	SelectObject(hMemoryDC, hPieceBmp);
	// Blt the piece which is in the memoryDC into the device DC
	BitBlt(hDC, pt.x, pt.y, PieceBmp.bmWidth, PieceBmp.bmHeight,
		hMemoryDC, 0, 0, pixels);
	// select original object into the 2nd DC
	SelectObject(hMemoryDC, hReplacedObj);
	// delete display context
	DeleteDC(hMemoryDC);
	// free all system resources associated with the hReplacedObj
	DeleteObject((HBITMAP)hReplacedObj);
	ReleaseDC(hWnd, hDC);		// release the device context
}

//
//  Display board
//
void DispBrd()
{
	// Display an empty Chess board
	for (int i = 0; i < 64; i++) ClrSquare(i);

	// Now, show the pieces and pawns
	for (int sq = 0; sq <= 63; sq++)
	{
		if (Brd[sq].pce != nil)
			DisplayPiece(LocatePt(sq), Brd[sq].pce, SRCINVERT);
	}
}

//
//   This function will be called from ChessCode.cpp to initialize the game
//
bool	InitGraphicsCode()
{
	// specify paint colors for the squares
	LtPaint = CreateSolidBrush(RGB(LIGHTSQR[0], LIGHTSQR[1], LIGHTSQR[2]));
	DrkPaint = CreateSolidBrush(RGB(DARKSQR[0], DARKSQR[1], DARKSQR[2]));

	// initialize the sprites and their masks
	if (ScreenHeight < MINHEIGHT)
	{
		Sprites[(wp >> 1) - 1][wht] = (HBITMAP)LoadImage(0, Swp, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Sprites[(wn >> 1) - 1][wht] = (HBITMAP)LoadImage(0, Swn, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Sprites[(wb >> 1) - 1][wht] = (HBITMAP)LoadImage(0, Swb, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Sprites[(wr >> 1) - 1][wht] = (HBITMAP)LoadImage(0, Swr, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Sprites[(wq >> 1) - 1][wht] = (HBITMAP)LoadImage(0, Swq, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Sprites[(wk >> 1) - 1][wht] = (HBITMAP)LoadImage(0, Swk, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

		Sprites[(bp >> 1) - 1][blk] = (HBITMAP)LoadImage(0, Sbp, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Sprites[(bn >> 1) - 1][blk] = (HBITMAP)LoadImage(0, Sbn, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Sprites[(bb >> 1) - 1][blk] = (HBITMAP)LoadImage(0, Sbb, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Sprites[(br >> 1) - 1][blk] = (HBITMAP)LoadImage(0, Sbr, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Sprites[(bq >> 1) - 1][blk] = (HBITMAP)LoadImage(0, Sbq, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Sprites[(bk >> 1) - 1][blk] = (HBITMAP)LoadImage(0, Sbk, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

		Masks[(wp >> 1) - 1] = (HBITMAP)LoadImage(0, Spm, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Masks[(wn >> 1) - 1] = (HBITMAP)LoadImage(0, Snm, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Masks[(wb >> 1) - 1] = (HBITMAP)LoadImage(0, Sbm, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Masks[(wr >> 1) - 1] = (HBITMAP)LoadImage(0, Srm, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Masks[(wq >> 1) - 1] = (HBITMAP)LoadImage(0, Sqm, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Masks[(wk >> 1) - 1] = (HBITMAP)LoadImage(0, Skm, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	}
	else
	{
		Sprites[(wp >> 1) - 1][wht] = (HBITMAP)LoadImage(0, Lwp, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Sprites[(wn >> 1) - 1][wht] = (HBITMAP)LoadImage(0, Lwn, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Sprites[(wb >> 1) - 1][wht] = (HBITMAP)LoadImage(0, Lwb, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Sprites[(wr >> 1) - 1][wht] = (HBITMAP)LoadImage(0, Lwr, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Sprites[(wq >> 1) - 1][wht] = (HBITMAP)LoadImage(0, Lwq, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Sprites[(wk >> 1) - 1][wht] = (HBITMAP)LoadImage(0, Lwk, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

		Sprites[(bp >> 1) - 1][blk] = (HBITMAP)LoadImage(0, Lbp, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Sprites[(bn >> 1) - 1][blk] = (HBITMAP)LoadImage(0, Lbn, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Sprites[(bb >> 1) - 1][blk] = (HBITMAP)LoadImage(0, Lbb, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Sprites[(br >> 1) - 1][blk] = (HBITMAP)LoadImage(0, Lbr, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Sprites[(bq >> 1) - 1][blk] = (HBITMAP)LoadImage(0, Lbq, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Sprites[(bk >> 1) - 1][blk] = (HBITMAP)LoadImage(0, Lbk, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

		Masks[(wp >> 1) - 1] = (HBITMAP)LoadImage(0, Lpm, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Masks[(wn >> 1) - 1] = (HBITMAP)LoadImage(0, Lnm, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Masks[(wb >> 1) - 1] = (HBITMAP)LoadImage(0, Lbm, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Masks[(wr >> 1) - 1] = (HBITMAP)LoadImage(0, Lrm, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Masks[(wq >> 1) - 1] = (HBITMAP)LoadImage(0, Lqm, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		Masks[(wk >> 1) - 1] = (HBITMAP)LoadImage(0, Lkm, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	}

	DispBrd();
	return true;
}

//
// check for overlap of rectangles, r1 and r2
// return true if they overlap
//
BOOL Overlap(RECT& r1, RECT& r2)
{
	POINT p{};

	p.x = r1.left;
	p.y = r1.top;
	if (PtInRect(&r2, p)) return TRUE;

	p.x = r1.right;
	if (PtInRect(&r2, p)) return TRUE;

	p.y = r1.bottom;
	if (PtInRect(&r2, p)) return TRUE;

	p.x = r1.left;
	if (PtInRect(&r2, p)) return TRUE;

	return FALSE;
}

//
// get the windows top left coordinates
//  in case user has moved the window
//
void GetWinTopLeft(void)
{
	RECT rect{ NULL };

	if (GetWindowRect(hWnd, &rect))
	{
		win_left = rect.left;
		win_top = rect.top;
	}
}

//
// this function restores the background after it was previously saved
//
static void RestoreBackground()
{
	HBITMAP hBitmap;
	BITMAP Bitmap;
	HDC hDC;

	hDC = GetDC(hWnd);  // find device context for the main window
	hBitmap = Sprites[0][wht];
	// get Bitmap structure and size
	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&Bitmap);
	// show original or previous bmp
	BitBlt(hDC, PreviousPt.x, PreviousPt.y, Bitmap.bmWidth, Bitmap.bmHeight,
		hPrevDC, 0, 0, SRCCOPY);
	// restore the previous bmp in the device context
	DeleteObject(SelectObject(hPrevDC, hPrevBmp));
	// delete the device context and bmp
	DeleteDC(hPrevDC);
	ReleaseDC(hWnd, hDC);
}

static void ShowPiece(POINT& p)
{
	HBITMAP hPrevBmp;
	BITMAP Bitmap;
	HDC hMemoryDC, hDC;

	hDC = GetDC(hWnd);  // get device context of main window
	hMemoryDC = CreateCompatibleDC(hDC);
	// get Bitmap of Sliding Sprite
	GetObject(hSlideSprite, sizeof(BITMAP), (LPSTR)&Bitmap);
	// select the mask first
	hPrevBmp = (HBITMAP)SelectObject(hMemoryDC, hSlideMask);
	// show the mask
	BitBlt(hDC, p.x, p.y, Bitmap.bmWidth, Bitmap.bmHeight,
		hMemoryDC, 0, 0, SRCAND);
	// select the sliding piece
	SelectObject(hMemoryDC, hSlideSprite);
	// show the sliding piece
	BitBlt(hDC, p.x, p.y, Bitmap.bmWidth, Bitmap.bmHeight,
		hMemoryDC, 0, 0, SRCINVERT);
	// restore the original bmp
	SelectObject(hMemoryDC, hPrevBmp);
	// delete compatible device context and associated bmp
	DeleteDC(hMemoryDC);
	ReleaseDC(hWnd, hDC);
}

static void SaveBackground(POINT& p)
{
	HDC hDC;
	HBITMAP hBitmap;
	BITMAP Bitmap;

	PreviousPt = p;
	hDC = GetDC(hWnd);  // get device context for main window
	hBitmap = Sprites[0][wht];
	// get bmp structure and size
	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&Bitmap);
	hPrevDC = CreateCompatibleDC(hDC);
	// save the original bmp
	hPrevBmp = (HBITMAP)SelectObject(hPrevDC,
		CreateCompatibleBitmap(hDC, Bitmap.bmWidth, Bitmap.bmHeight));
	// show original bmp unchanged
	BitBlt(hPrevDC, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight, hDC, p.x, p.y, SRCCOPY);
	ReleaseDC(hWnd, hDC);
}

static void RedrawScreen(POINT& p)
{
	HDC hDC, hHiddenDC, hNewPrevDC;
	HBITMAP hHiddenBmp, hNewPrevBmp;
	BITMAP Bitmap;
	RECT rect, rect1, rect2;
	int Width, Height;

	// get bmp structure and size
	GetObject(hSlideSprite, sizeof(BITMAP), (LPSTR)&Bitmap);

	// new rectangle from move of sliding piece
	rect1.left = p.x;
	rect1.top = p.y;
	rect1.right = p.x + Bitmap.bmWidth;
	rect1.bottom = p.y + Bitmap.bmHeight;

	// old rectangle from PreviousPt (global)
	rect2.left = PreviousPt.x;
	rect2.top = PreviousPt.y;
	rect2.right = PreviousPt.x + Bitmap.bmWidth;
	rect2.bottom = PreviousPt.y + Bitmap.bmHeight;

	if (!Overlap(rect1, rect2))
	{
		RestoreBackground();
		SaveBackground(p);
		ShowPiece(p);
		return;
	}
	// get handle for device context of main window
	hDC = GetDC(hWnd);
	// create compatible DC
	hNewPrevDC = CreateCompatibleDC(hDC);
	// create compatible bmp and select it
	hNewPrevBmp = (HBITMAP)SelectObject(hNewPrevDC,
		CreateCompatibleBitmap(hDC, Bitmap.bmWidth, Bitmap.bmHeight));

	// enlarge area to encompass both rectangles
	rect.left = min(rect1.left, rect2.left);
	rect.top = min(rect1.top, rect2.top);
	rect.bottom = max(rect1.bottom, rect2.bottom);
	rect.right = max(rect1.right, rect2.right);

	Width = rect.right - rect.left;
	Height = rect.bottom - rect.top;

	// create yet another DC, for larger rect
	hHiddenDC = CreateCompatibleDC(hDC);
	// create an enlarged bmp and select it
	hHiddenBmp = (HBITMAP)SelectObject(hHiddenDC,
		CreateCompatibleBitmap(hDC, Width, Height));

	// blt enlarged area in hidden device context
	BitBlt(hHiddenDC, 0, 0, Width, Height, hDC, rect.left, rect.top, SRCCOPY);

	// restore previous area (empty sqr) in hidden device context
	BitBlt(hHiddenDC, abs(rect.left - rect2.left), abs(rect.top - rect2.top),
		Bitmap.bmWidth, Bitmap.bmHeight, hPrevDC, 0, 0, SRCCOPY);

	// make background in hidden DC the new previous background
	BitBlt(hNewPrevDC, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight,
		hHiddenDC, abs(rect.left - rect1.left), abs(rect.top - rect1.top),
		SRCCOPY);

	// select the mask of the sliding piece
	DeleteObject(SelectObject(hPrevDC, hSlideMask));
	// Show the mask of the sliding piece in hidden DC
	BitBlt(hHiddenDC, abs(rect.left - rect1.left), abs(rect.top - rect1.top),
		Bitmap.bmWidth, Bitmap.bmHeight, hPrevDC, 0, 0, SRCAND);

	// get the sliding piece
	SelectObject(hPrevDC, hSlideSprite);
	// show it in hidden DC
	BitBlt(hHiddenDC, abs(rect.left - rect1.left), abs(rect.top - rect1.top),
		Bitmap.bmWidth, Bitmap.bmHeight, hPrevDC, 0, 0, SRCINVERT);

	// now show the new DC in the main window's DC
	BitBlt(hDC, rect.left, rect.top, Width, Height, hHiddenDC, 0, 0, SRCCOPY);

	SelectObject(hPrevDC, hPrevBmp); // don't delete current bmp (hSlidePiece) 
	DeleteDC(hPrevDC);
	hPrevDC = hNewPrevDC;
	hPrevBmp = hNewPrevBmp;
	PreviousPt = p;
	DeleteObject(SelectObject(hHiddenDC, hHiddenBmp));
	DeleteDC(hHiddenDC);
	ReleaseDC(hWnd, hDC);
}


int LocateSqr(POINT pt, int colr, BOOL started)
{
	POINT p;
	int s;
	RECT r;

	for (s = 0; s <= 63; s++)
	{
		p = LocatePt(s);
		r.left = p.x;
		r.top = p.y;
		r.right = r.left + BmpWidth;
		r.bottom = r.top + BmpWidth;
		if (PtInRect(&r, pt))
		{
			if (((Brd[s].pce != nil) && ((Brd[s].pce & 1) == colr)) || started)
				return(s);
		}
	}
	return(-1);
}

//
// StartSlide - allows player to drag the piece
//
void StartSlide(int sqr, POINT& pt)
{
	PIECE pc;
	COLOR cl;
	pt.x -= BmpWidth / 2;
	pt.y -= BmpWidth / 2;

	CurrentPt = LocatePt(sqr);
	cl = (COLOR)(Brd[sqr].pce & 1);
	pc = (PIECE)(Brd[sqr].pce ^ 1);
	hSlideSprite = Sprites[(pc >> 1) - 1][cl];
	hSlideMask = Masks[(pc >> 1) - 1];
	hPrevCursor = SetCursor(0);
	SetClassWord(hWnd, GCW_HCURSOR, 0);
	ClrSquare(sqr);
	SaveBackground(pt);
	ShowPiece(pt);
	sliding = TRUE;
}

//
// StopSlide - stops sliding piece
//
void StopSlide(void)
{
	if (!sliding) return;

	RestoreBackground();       // remove piece if off the board or on a bad square

	SetCursor(hPrevCursor);
	SetClassWord(hWnd, GCW_HCURSOR, WORD(hPrevCursor));

	sliding = FALSE;
}

//
// display messages to the user
//
void DispMsg(WCHAR* Err, int len)
{
	RECT Rect;
	HDC  hDC;
	int  x, y;
	int  ScrnSizeOffset;

	ScrnSizeOffset = BmpWidth * 8 + 2;
	// error entering move
	x = 80;
	if (ScreenHeight >= MINHEIGHT) y = ScrnSizeOffset;
	else y = ScrnSizeOffset + 2;
	Rect = { x,y,x + 130,y + CharHeight };
	hDC = GetDC(hWnd);  // get device context for main window
	DrawText(hDC, Err, len, &Rect, DT_LEFT);
	ReleaseDC(hWnd, hDC);
}

// this will allow the user to play a game of chess, but will be replaced later
void TempCastlingPromotes()
{   // if castling, king was just moved on the Brd[]
	if ((FromSqr == 03) && (Brd[ToSqr].pce == wk) &&
		((ToSqr == 01) || (ToSqr == 05)))
	{
		// assume castling is legal by movement of the king
		// if to sqr = 01, move white rook from 00 to 02
		if (ToSqr == 01)
		{
			Brd[0x00].pce = nil;
			Brd[0x02].pce = wr;
		}
		else
		{
			Brd[0x07].pce = nil;
			Brd[0x04].pce = wr;
		}
	}
	if ((FromSqr == 0x3b) && (Brd[ToSqr].pce == bk) &&
		((ToSqr == 0x39) || (ToSqr == 0x3d)))
	{
		// assume castling is legal by movement of the king
		// if to sqr = 0x39, move black rook from 0x38 to 0x3a
		if (ToSqr == 0x39)
		{
			Brd[0x38].pce = nil;
			Brd[0x3a].pce = br;
		}
		else
		{
			Brd[0x3f].pce = nil;
			Brd[0x3c].pce = br;
		}
	}
	// assume white pawn is promoting to a queen
	if (((FromSqr & 0x38) == 0x30) && (Brd[ToSqr].pce == wp))
		Brd[ToSqr].pce = wq;

	// assume black pawn is promoting to a queen
	if (((FromSqr & 0x38) == 8) && (Brd[ToSqr].pce == bp))
		Brd[ToSqr].pce = bq;

	// white pawn capture e.p.?
	if (((FromSqr & 0x38) == 0x20) && (Brd[ToSqr].pce == wp) &&
		((PrevTo & 0x38) == 0x20) && (Brd[PrevTo].pce == bp) &&
		((ToSqr & 0x07) == (PrevTo & 0x07)))
		Brd[PrevTo].pce = nil;

	// black pawn capture e.p.?
	if (((FromSqr & 0x38) == 0x18) && (Brd[ToSqr].pce == bp) &&
		((PrevTo & 0x38) == 0x18) && (Brd[PrevTo].pce == wp) &&
		((ToSqr & 0x07) == (PrevTo & 0x07)))
		Brd[PrevTo].pce = nil;

	//display board
	DispBrd();
}

void EndSearch()
{
	MSG msg;
	POINT Point;
	int px, py;

	if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_LBUTTONDOWN)    //this allows the mouse to work
		{
			GetCursorPos(&Point);
			ScreenToClient(hWnd, &Point);
			if (!startedmove)
			{
				FromSqr = LocateSqr(Point, com, startedmove);
				if (FromSqr == -1) return;
				startedmove = TRUE;
				// cover up previous message on display
				DispMsg(Erasing, 34);
				StartSlide(FromSqr, Point);
			}
		}
		else if (msg.message == WM_LBUTTONUP)
		{
			GetCursorPos(&Point);

			// save location of mouse button release to be able to restore
			// the IDC_ARROWCURSOR for human vs human
			px = Point.x;
			py = Point.y;

			ScreenToClient(hWnd, &Point);
			if (startedmove)
			{
				ToSqr = LocateSqr(Point, com, startedmove);
				if ((ToSqr != FromSqr) && (ToSqr != -1) && (FromSqr != -1))
					startedmove = FALSE;
				if ((ToSqr == -1) || (ToSqr == FromSqr))
				{
					StopSlide();
					if (ToSqr == FromSqr)
					{
						// touch move?
						DispMsg(TouchMv, 11);
					}
					else
					{
						// error entering move
						DispMsg(Warning, 19);
					}
					ClrSquare(FromSqr);
					DisplayPiece(LocatePt(FromSqr), Brd[FromSqr].pce, SRCINVERT);
					startedmove = false;    //begin move again
					return;
				}
				if ((Brd[ToSqr].pce != nil) && ((Brd[ToSqr].pce & 1) == com))
				{
					StopSlide();
					// error entering move
					DispMsg(Warning, 19);
					ClrSquare(FromSqr);
					DisplayPiece(LocatePt(FromSqr), Brd[FromSqr].pce, SRCINVERT);
					startedmove = false;    //begin move again
					return;
				}
				// a player may move its sliding pieces through its own pieces and pawns
				if (ToSqr != FromSqr)
				{
					// remove enemy piece if it was a capture
					Brd[ToSqr].pce = nil;
					// make move on Board
					Brd[ToSqr] = Brd[FromSqr];
					Brd[FromSqr].pce = nil;

					// carefully place moving piece on To square
					StopSlide();
					ClrSquare(ToSqr);
					DisplayPiece(LocatePt(ToSqr), Brd[ToSqr].pce, SRCINVERT);
					TempCastlingPromotes();  // castling, promotes, & en passant
					PrevTo = ToSqr;	// to detect en passant pawn captures
				}
				SetCursorPos(px, py);
				SetClassWord(hWnd, GCW_HCURSOR, WORD(hPrevCursor));
				SetCursor(hPrevCursor);
			}
			if ((ToSqr != FromSqr) && (ToSqr != -1) && (FromSqr != -1))
				endsearch = TRUE;
		}
		else if (msg.message == WM_MOUSEMOVE)
		{
			if (!startedmove) return;
			Point.x = msg.pt.x - BmpWidth / 2 - win_left; //in case window moved
			Point.y = msg.pt.y - LBMPWIDTH - win_top;	 // even for small bmp width
			RedrawScreen(Point);
		}
	}
	return;
}
