#include <windows.h>
#include <stdio.h>
#include <math.h>
#include "mapwin.h"
#include "entity.h"
#include "str.h"
#include "comfile.h"
#include <Debug.h>

extern "C" {
#include "utility.h"
}

#define RAD2DEG  57.29577951308
#define DEG2RAD	0.01745329251994

// DK 020504  Uncomment for use with palette code i Load(), Draw()
// Probably should be moved inside the class
static  HPALETTE      hPalette, hOldPalette;

//---------------------------------------------------------------------------------------CMapWin
CMapWin::CMapWin() {
	hMap=0;		// Dimension of the bitmaps used to draw display
	wMap=0;

  // DK CLEANUP hack to set window size
  this->iX = 800;
	this->iY = 0;
	this->nX = 480;
	this->nY = 400;

  hBitmap=0;

}

//---------------------------------------------------------------------------------------~CMapWin
CMapWin::~CMapWin() {
	CEntity *ent;
  // DK Added 020504
  if(hBitmap)
    DeleteObject( hBitmap );

	for(int i=0; i<arrEnt.GetSize(); i++) {
		ent = (CEntity *)arrEnt.GetAt(i);
		delete ent;
	}
}

//---------------------------------------------------------------------------------------Init
void CMapWin::Init(HINSTANCE hinst) {
	CWin::Init(hinst, "Map");
}




//---------------------------------------------------------------------------------------Load
// Load world bit map, need to call Build() to display
int CMapWin::Load(char *file) 
{

  HBITMAP * phBitmap   = &hBitmap;
  HPALETTE * phPalette = &hPalette;


  // This function extracted from MS sample code 
  //   DK 020604
  
  // BITMAP  bm;
  
  *phBitmap = NULL;
  *phPalette = NULL;
  
  // Use LoadImage() to get the image loaded into a DIBSection
  *phBitmap = (HBITMAP)LoadImage( NULL, file, IMAGE_BITMAP, 0, 0,
    LR_CREATEDIBSECTION | LR_DEFAULTSIZE | LR_LOADFROMFILE );
  if( *phBitmap == NULL )
    return -1;




  /*  DK 020504  Uncomment this section, along with the palette
      code in Draw() to enable support for best possible 8-bit (256)
      color representation of the map.  The map should work well 
      without this code in anything 16-bit or higher
  
  // Get the color depth of the DIBSection
  GetObject(*phBitmap, sizeof(BITMAP), &bm );
  // If the DIBSection is 256 color or less, it has a color table
  if( ( bm.bmBitsPixel * bm.bmPlanes ) <= 8 )
  {
    HDC           hMemBMDC;
    HBITMAP       hOldBitmap;
    RGBQUAD       rgb[256];
    LPLOGPALETTE  pLogPal;
    WORD          i;
    
    // Create a memory DC and select the DIBSection into it
    hMemBMDC = CreateCompatibleDC( NULL );
    hOldBitmap = (HBITMAP)SelectObject( hMemBMDC, *phBitmap );
    // Get the DIBSection's color table
    GetDIBColorTable( hMemBMDC, 0, 256, rgb );
    // Create a palette from the color tabl
    pLogPal = (LOGPALETTE *)malloc( sizeof(LOGPALETTE) + (256*sizeof(PALETTEENTRY)) );
    pLogPal->palVersion = 0x300;
    pLogPal->palNumEntries = 256;
    for(i=0;i<256;i++)
    {
      pLogPal->palPalEntry[i].peRed = rgb[i].rgbRed;
      pLogPal->palPalEntry[i].peGreen = rgb[i].rgbGreen;
      pLogPal->palPalEntry[i].peBlue = rgb[i].rgbBlue;
      pLogPal->palPalEntry[i].peFlags = 0;
    }
    *phPalette = CreatePalette( pLogPal );
    // Clean up
    free( pLogPal );
    SelectObject( hMemBMDC, hOldBitmap );
    DeleteDC( hMemBMDC );
  }
  else   // It has no color table, so use a halftone palette
  {
    HDC    hRefDC;
    
    hRefDC = GetDC( NULL );
    *phPalette = CreateHalftonePalette( hRefDC );
    ReleaseDC( NULL, hRefDC );
  }
   *********************************************************/
  return 0;

}

//---------------------------------------------------------------------------------------Build
// Rebuild map base if window resized
void CMapWin::Build(HDC hdc) 
{

  BITMAP        bm;
  HDC           hMemBMDC, hMemSDC;
  HBITMAP hOldBitmap,hOldBitmap2,bmWorld;
  int rc;

  // Retrieve the bitmap information via the hBitmap handle
  rc = GetObject( hBitmap, sizeof(BITMAP), &bm );

  // Get a compatable DC for drawing the original bitmap in memory
  hMemBMDC = CreateCompatibleDC( hdc );

  // Set the bitmap to be the drawing object for the memory DC
  // Save the old drawing object, so that we can restore it later.
  // This effectively copies the original bitmap into the
  // memory device context.
  hOldBitmap = (HBITMAP)SelectObject( hMemBMDC, hBitmap );
  

  // Get the map dimensions  (Fill the client window)
  RECT r;
  GetClientRect(hWnd, &r);
  wMap = r.right-1;
  hMap = r.bottom-1;
  
  // Get a memory device context(DC) for drawing on.  
  // (this DC will be for drawing the Screen image on in memory
  hMemSDC = CreateCompatibleDC(hdc);
  if(!hMemSDC) {
    CDebug::Log(DEBUG_MINOR_ERROR,"Build():Could not create hMemSDC\n");
    return;
  }

  // Allocate a bitmap object the size of our window
  // to allow us to draw on the memory screen DC
  bmWorld = CreateCompatibleBitmap(hdc, wMap, hMap);
  if(!bmWorld) {
    CDebug::Log(DEBUG_MINOR_ERROR,"Build():Could not create bmWorld\n");
    return;
  }

  // Map the bitmap object onto the memory screen-DC
  hOldBitmap2 = (HBITMAP)SelectObject(hMemSDC, bmWorld);

  // Set the stretch translation mode for the memory screen-DC to be COLORONCOLOR.
  // That way if the bitmap has to be shrunk it will choose one of the
  // eligible pixels, instead of trying to combine them all.
  SetStretchBltMode(hMemSDC, COLORONCOLOR);

  // Wanted to use StretchDIBits() here, but I couldn't figure out
  // how to get/fill out a BITMAPINFO struct  DK 020504
  // Copy the bitmap(map) from it's original size(in hMemBMDC) to the window size(in hMemSDC).
  BOOL brc=StretchBlt(hMemSDC, 0,0,wMap,hMap, hMemBMDC,0,0, bm.bmWidth, bm.bmHeight, SRCCOPY);
  
	CEntity *ent;
	int i;

  // Draw the current quake and all of the picks onto the memory  screen-stretched-bitmap DC
  for(i=0; i<arrEnt.GetSize(); i++) {
    ent = (CEntity *)arrEnt.GetAt(i);
    ent->Render(hMemSDC, this);
  }

  // Copy the memory  screen-stretched-bitmap to the screen DC
  BitBlt(hdc, 0, 0, wMap, hMap, hMemSDC, 0, 0, SRCCOPY);

  // The image is now drawn on the screen DC.
  // Clean up

  // De-select our bitmap from the hMemBMDC DC  (reinstate the previous one)
  SelectObject( hMemBMDC, hOldBitmap );

  // De-select our bitmap from the hMemSDC DC  (reinstate the previous one)
  SelectObject( hMemSDC, hOldBitmap2 );

  // Delete the bm object for drawing the stretched bmp
  DeleteObject(bmWorld);

  // Delete the DC for the original-bitmap 
  DeleteDC( hMemBMDC );

  // Delete the DC for the screen-stretched-bitmap 
  DeleteDC( hMemSDC );
}

//---------------------------------------------------------------------------------------Purge
// Remove and "delete" all entities from entity list
void CMapWin::Purge() {
	CEntity *ent;
	int i;

	for(i=0; i<arrEnt.GetSize(); i++) {
		ent = (CEntity *)arrEnt.GetAt(i);
    delete ent;
	}
	arrEnt.RemoveAll();
}

//---------------------------------------------------------------------------------------Entity
// Add entity to entity list
void CMapWin::Entity(CEntity *ent) {
	arrEnt.Add(ent);
}

//---------------------------------------------------------------------------------------X
// Calculate x screen coordinate from longitude (Miller's Cylindrical)
int CMapWin::X(double lonx) {
	double lon1 = -270.0;
	double lon2 = 120.0;
	double lon = lonx;
	if(lon > 90.0)
		lon -= 360.0; 
	int x = (int)((lon-lon1)*wMap/(lon2-lon1));
	return x;
}

//---------------------------------------------------------------------------------------Y
// Calculate y screen coordinate from latitude (Miller's Cylindrical)
int CMapWin::Y(double lat) {
	double lat1 = -80.0;
	double lat2 = 80.0;
	double a = 0.5*hMap/tan(0.8*DEG2RAD*lat2);
	int y = hMap/2 - (int)(a*tan(0.8*DEG2RAD*lat));
	return y;
}

//---------------------------------------------------------------------------------------Refresh
// Redraw map
void CMapWin::Refresh() {
	InvalidateRect(hWnd, NULL, false);
	UpdateWindow(hWnd);
}

//---------------------------------------------------------------------------------------Size
void CMapWin::Size(int w, int h) {
}

//---------------------------------------------------------------------------------------LeftDown
void CMapWin::LeftDown(int x, int y) {
//	InvalidateRect(hWnd, NULL, true);
//	UpdateWindow(hWnd);
}

void CMapWin::RightDown(int x, int y) {
}

//---------------------------------------------------------------------------------------Draw
void CMapWin::Draw(HDC hdc) {

	Build(hdc);


  /*  DK 020504  Uncomment this code, along with the palette
      code in Draw() to enable support for best possible 8-bit (256)
      color representation of the map.  The map should work well 
      without this code in anything 16-bit or higher
  hOldPalette = SelectPalette( hdc, hPalette, FALSE );
  rc = RealizePalette( hdc );

  // I don't think this code belongs here.  The palette needs to
  // stay alive till the end of the program, so if the above code
  // is enable, then the below code should be added to the destructor,
  // or somehow similarly placed.
  
  if(hPalette)
  {
    SelectPalette( hdc, hOldPalette, FALSE );
    DeleteObject( hPalette );
  }
    hOldPalette = SelectPalette( hdc, hPalette, FALSE );
    rc = RealizePalette( hdc );
  ********************************************/


}

//---------------------------------------------------------------------------------------Char
void CMapWin::Char(int key) {
//	InvalidateRect(hWnd, NULL, true);
//	UpdateWindow(hWnd);
}

//---------------------------------------------------------------------------------------KeyDown
void CMapWin::KeyDown(int key) {
}

//---------------------------------------------------------------------------------------KeyUp
void CMapWin::KeyUp(int key) {
}



