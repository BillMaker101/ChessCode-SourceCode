#pragma once

#include "resource.h"

#define MAX_LOADSTRING 100
#define GCW_HCURSOR 38

const int TITLELENGTH = 10;
const int HBOARDOFFSET = 290;

// fit the board to the screen size
const int LBMPWIDTH = 84;
const int SBMPWIDTH = 42;
const int MINHEIGHT = 743; // minimum height required for large bmp width

// RGB color choice - easy on the eyes
const BYTE DARKSQR[3] = { 0x8b, 0x63, 0x44 };	// brown (to resemble wood)
const BYTE LIGHTSQR[3] = { 0xf2, 0xcf, 0xa3 };    // blonde 

enum COLOR { wht, blk };

// the piece color is in bit 0: 0=white, 1=black, determined by piece ‘AND’ 1.
// remove the color to see the piece type: pn=2, kt=4, bi=6, ro=8, qu=10, ki=12
enum PIECE
{
	nil = 0, unused = 1,
	wp = 2, bp = 3,
	wn = 4, bn = 5,
	wb = 6, bb = 7,
	wr = 8, br = 9,
	wq = 10, bq = 11,
	wk = 12, bk = 13
};

struct SQUARE     // used to track the locations of the chess pieces
{
	PIECE pce;	   // piece
};
