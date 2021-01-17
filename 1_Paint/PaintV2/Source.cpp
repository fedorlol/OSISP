#include "resource.h"
#include <Windows.h>
#include <zmouse.h>
#include <tchar.h>
#include <CommCtrl.h>
#include <vector>

#pragma comment(lib, "comctl32.lib")

// IDR - ID for the Resource.
// IDC - ID for a Control.
// IDM - ID for Menu.
// IDI - ID for Icon.
#define IDR_TOOLBAR 10000			// Not used. 
#define IDR_STATUS_BAR 10001		// Not used.
#define IDC_EDIT_PEN_WIDTH2 10002

// Used in Text dialog. The size for the buffer that stores the entered text. 
#define BUFFER_TEXT_SIZE 200

// wParam == new zoom, lParam == not used.
#define MY_MESSAGE_SET_ZOOM WM_APP + 1
// wParam == not used, lParam == not used.
#define MY_MESSAGE_CHANGE_STRETCHING_MODE WM_APP + 2

// WinAPI has POINT struct, but POINT does not have a constructor.
// Because of this we can not do like this: array.push_back(POINT(10, 15));
struct Point
{
	Point(int _x, int _y)
	{
		x = _x; y = _y;
	}

	int x, y;
};

// Used to completely redraw part of the picture painted by mouse.
// Open files are not included, just painted by mouse.
struct DrawPictureData
{
	int figureID;   // What figure to draw.
	int penWidth;
	int x1, y1, x2, y2;
	std::vector<Point> arrayCoord; // Used to draw line painted by Pencil (array of each point of the line). 
	COLORREF colorPen, colorBrush, colorText;
	int isTransparentBrush;
	LOGFONT logicalFont; // Text font.
	TCHAR textBuffer[BUFFER_TEXT_SIZE]; // Buffer containing text. 
};

// gv - global variable.
// "A" - because of button icon. Stores information about the text.
COLORREF gv_AtextColor;
LOGFONT gv_AlogicalFont;
TCHAR gv_AtextBuffer[BUFFER_TEXT_SIZE];

TCHAR gv_openFileNameBuffer[MAX_PATH];
int gv_isFileOpen = 0;
int gv_isFileSaved = 0;

int gv_isTransparentBrush = 1;
int gv_currentZoom = 0;
int gv_isHALFTONE = 0;

TCHAR gv_tempFileNameForPrint[] = _TEXT("qwertyuiopasdfghjklzxcvbnmQQQ.emf");

const int GV_APPLICATION_WIDTH = 640, GV_APPLICATION_HEIGHT = 480;
const TCHAR GV_APPLICATION_CAPTHION[] = _TEXT("Paint");
HWND gv_hWndToolBar, gv_hWndStatusBar, gv_hWndMainWindow, gv_hWndEditPenWidth;

LRESULT CALLBACK WindowProcedure_Main(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void PrintError(const int errorCode, const PTCHAR message);
HWND InitializeToolBar(HWND hWindow);
void On_WM_COMMAND(WPARAM wParam, LPARAM lParam, int* figureToDraw, LPCOLORREF colorPen, LPCOLORREF colorBrush,
	std::vector<DrawPictureData>* drawPictureDataArray, HDC hMemoryDCDraw, HDC hMemoryDCZoom);
void SetStatusBarText(int figureID, int currentZoom);

int GetColorFromChooseColorDialog(LPCOLORREF color, HWND hWndDialogOwner);
int GetFontFromChooseFontDialog(LPLOGFONT font, LPCOLORREF textColor, HWND hWndDialogOwner);

BOOL CALLBACK DialogProcedure_Text(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DialogProcedure_Settings(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Description:
//   Calculates the size of the rectangle for the device context in memory. 
// Parameters:
//   rectangleMemoryDC - pointer to a rectangle.
// Returns:
//   void.
void GetMemoryDCRectangle(RECT* rectangleMemoryDC);

// Description:
//   Creates a context in memory compatible with the context of the display. 
// Parameters:
//   hDC - context of the display.
//   hMemoryDCDraw - pointer to a context in memory.
//   hBitMapMemoryDCDraw - pointer to a bitmap for a context in memory.
// Returns:
//   void.
void CreateMemoryDC(HDC hDC, HDC* hMemoryDCDraw, HBITMAP* hBitMapMemoryDCDraw);

// Description:
//   Calculates the size of the drawing rectangle. 
//   The toolbar and statusbar are included in the client area, 
//   but do not enter the drawing area.
// Parameters:
//   hDC - context of the display.
//   hMemoryDCDraw - pointer to a context in memory where is drawing occurs.
//   hBitMapMemoryDCDraw - pointer to a bitmap for a context in memory.
// Returns:
//   void.
void GetPictureRectangle(LPRECT pictureRectangle, int* toolBarHeight, int* statusBarHeight);

// Description:
//   When zooming, the mouse click coordinates do not coincide with the 
//   coordinates in the context in memory. 
//	 This function converts the coordinates so that they coincide.
// Parameters:
//   xMousePosition - pointer to X coordinate of the "hot point" of the mouse cursor.
//   yMousePosition - pointer to Y coordinate of the "hot point" of the mouse cursor.
//   currentZoom - current image scaling.
// Returns:
//   void.
void ZoomCoordinateTransformation(int* xMousePosition, int* yMousePosition, int currentZoom);

// Description:
//	 Sets the current scale and displays the picture.
// Parameters:
//	 hDC - context of the display.
//   hMemoryDCDraw - context in memory where is drawing occurs.
//   hMemoryDCZoom - auxiliary context in memory where is zooming occurs.
//   xScrollPosition - X current scroll position.
//	 yScrollPosition - Y current scroll position.
//   zoomInc - increment to current scale.
//   setZoom - if this parameter is not NULL, then the transmitted zoom will be set.
//	 isZoomLimitReached - if this parameter is not NULL, then if the zoom limit is reached, 
//			   it returns 1, otherwise 0.
// Returns:
//   void.
void ZoomPictureAndDisplay(HDC hDC, HDC hMemoryDCDraw, HDC hMemoryDCZoom, int xScrollPosition, 
	int yScrollPosition, int zoomInc, int* setZoom, int* isZoomLimitReached);

// Description:
//   Draws a picture whose data is transferred in an array.
// Parameters:
//   hDC - context.
//   drawPictureDataArray - an array containing information for redrawing the DRAWN on the screen.
// Returns:
//   void.
void RedrawAllPaintedPicture(HDC hDC, std::vector<DrawPictureData> drawPictureDataArray);

// Description:
//   Draws all picture.
// Parameters:
//   hDC - context.
//   drawPictureDataArray - an array containing information for redrawing the DRAWN on the screen.
//   fillBackgroundBursh - brush, which will be painted the background of the picture.
// Returns:
//   void.
void RedrawAllPicture(HDC hDC, std::vector<DrawPictureData> drawPictureDataArray, 
	HBRUSH fillBackgroundBursh);

void OpenEnhancedMetafile(HDC hDC, const PTCHAR fileName);
void SaveEnhancedMetafile(const PTCHAR fileName);

void SetScrollBarsOptions(HWND hWnd, int xScrollPosition, int yScrollPosition);

// Description:
//   Converts a string obtained from a window(edit control) to a number.
// Parameters:
//   hWnd - where to get the text.
// Returns:
//   Converted number.
int GetNumberFromWindow(HWND hWnd);

// Description:
//   Calculates the scrolling interval of the mouse wheel.
// Parameters:
//   interval - interval of the mouse wheel received from the WM_MOUSEWHEEL.
//   step - number for interval calculation.
// Returns:
//   Interval of the mouse wheel.
int GetMouseWheelInterval(int interval, int step);
int IsKeyPressed(UINT VirtualKey);

void PrintError(const int errorCode, const PTCHAR message)
{
	/*TCHAR buffer[100] = { 0 };
	wsprintf(buffer, _TEXT(" Ó‰ Ó¯Ë·ÍË: %d\n%s"), errorCode, message);*/

	MessageBox(NULL, message, _TEXT("Where error"), MB_OK | MB_ICONWARNING);

	HLOCAL buffer = NULL;

	BOOL fOk = FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL, errorCode, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		(LPTSTR)&buffer, 0, NULL);

	if (buffer != NULL)
	{
		MessageBox(NULL, (LPTSTR)buffer, _TEXT("Error"), MB_OK | MB_ICONWARNING);
		LocalFree(buffer);
	}
	else
	{
		TCHAR errorError[] = _TEXT("it's not error report.\nIt's error error.");
		MessageBox(NULL, errorError, _TEXT("Error error"), MB_OK | MB_ICONWARNING);
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)
{
	WNDCLASSEX windowsClassMain = { 0 };

	windowsClassMain.cbSize = sizeof(WNDCLASSEX);
	windowsClassMain.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	windowsClassMain.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowsClassMain.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN_APPLICATION_ICON));
	windowsClassMain.hInstance = hInstance;
	windowsClassMain.lpfnWndProc = WindowProcedure_Main;
	windowsClassMain.lpszClassName = _TEXT("main windows class");

	if (!RegisterClassEx(&windowsClassMain))
	{
		PrintError(GetLastError(), _TEXT("RegisterClassEx()"));
		return 1;
	}

	int centerMonitorX, centerMonitorY;
	centerMonitorX = (GetSystemMetrics(SM_CXSCREEN) - GV_APPLICATION_WIDTH) / 2;
	centerMonitorY = (GetSystemMetrics(SM_CYSCREEN) - GV_APPLICATION_HEIGHT) / 2;
	gv_hWndMainWindow = CreateWindow(windowsClassMain.lpszClassName, GV_APPLICATION_CAPTHION,
		WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL, centerMonitorX, centerMonitorY,
		GV_APPLICATION_WIDTH, GV_APPLICATION_HEIGHT, NULL, NULL, hInstance, NULL);

	if (!gv_hWndMainWindow)
	{
		PrintError(GetLastError(), _TEXT("CreateWindow()"));
		return 1;
	}

	ShowWindow(gv_hWndMainWindow, cmdShow);
	if (!UpdateWindow(gv_hWndMainWindow))
	{
		PrintError(GetLastError(), _TEXT("UpdateWindow()"));
		return 1;
	}

	HACCEL hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));
	if (hAccel == NULL)
	{
		PrintError(GetLastError(), _TEXT("LoadAccelerators()"));
		return 1;
	}

	INITCOMMONCONTROLSEX icc;
	icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icc.dwICC = ICC_WIN95_CLASSES;
	if (!InitCommonControlsEx(&icc))
	{
		PrintError(GetLastError(), _TEXT("InitCommonControlsEx()"));
		return 1;
	}

	MSG message;
	while (GetMessage(&message, NULL, 0, 0))
	{
		if (!TranslateAccelerator(gv_hWndMainWindow, hAccel, &message))
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
	}

	return (int)message.wParam;
}

LRESULT CALLBACK WindowProcedure_Main(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HDC hDC;
	static int xMousePosition, yMousePosition;
	static int xLeftRectangleOrEllipse, yTopRectangleOrEllipse;
	static POINT firstPointOfTheLine;
	static int isDrawing = 0, figureToDraw = ID_CURSOR;

	static HPEN pen;
	static HBRUSH brush;
	static COLORREF colorPen = 0, colorBrush = 0;

	static HDC hMemoryDCDraw, hMemoryDCZoom;
	static HBITMAP hBitMapMemoryDCDraw, hBitMapMemoryDCZoom;
	static int toolBarHeight = 0;

	static std::vector<DrawPictureData> drawPictureDataArray;
	static DrawPictureData drawData;

	static int xScrollPosition = 0, yScrollPosition = 0;
	static int xScrollStep = 5, yScrollStep = 5, zoomStep = 5;

	static int oldStretchMode;

	switch (message)
	{
	case WM_CREATE:
	{
		gv_hWndToolBar = InitializeToolBar(hWnd);

		gv_hWndStatusBar = CreateStatusWindow(WS_CHILD | WS_VISIBLE, _TEXT("Cursor.   Zoom: 0"), hWnd, IDR_STATUS_BAR);
		if (gv_hWndStatusBar == NULL)
			PrintError(GetLastError(), _TEXT("CreateStatusWindow()"));

		hDC = GetDC(hWnd);

		break;
	}
	case WM_LBUTTONDOWN:
	{
		static int isFirstClickPolygon = 1;
		if (figureToDraw != ID_POLYGON)
		{
			isFirstClickPolygon = 1;
		}
	
		if (figureToDraw != ID_CURSOR)
		{
			isDrawing = 1;

			gv_isFileSaved = 0;

			int penWidth = GetNumberFromWindow(gv_hWndEditPenWidth);
			pen = CreatePen(PS_SOLID, penWidth, colorPen);
			SelectObject(hMemoryDCDraw, pen);

			if (gv_isTransparentBrush)
			{
				brush = (HBRUSH)GetStockObject(NULL_BRUSH);
			}
			else
			{
				brush = CreateSolidBrush(colorBrush);
			}
			SelectObject(hMemoryDCDraw, brush);

			drawData.figureID = figureToDraw;
			drawData.penWidth = penWidth;
			drawData.colorPen = colorPen;
			drawData.colorBrush = colorBrush;
			drawData.isTransparentBrush = gv_isTransparentBrush;

			switch (figureToDraw)
			{
			case ID_PENCIL:
			{
				// + xScrollPosition, + yScrollPosition - so that the current scroll coincides with the drawing.
				// - toolBarHeight - toolbar is included in the client area, but is not included in the drawing area.
				xMousePosition = LOWORD(lParam) + xScrollPosition;
				yMousePosition = HIWORD(lParam) - toolBarHeight + yScrollPosition;
				ZoomCoordinateTransformation(&xMousePosition, &yMousePosition, gv_currentZoom);
				MoveToEx(hMemoryDCDraw, xMousePosition, yMousePosition, NULL);

				drawData.x1 = xMousePosition;
				drawData.y1 = yMousePosition;

				break;
			}
			case ID_LINE:
			{
				xMousePosition = LOWORD(lParam) + xScrollPosition;
				yMousePosition = HIWORD(lParam) - toolBarHeight + yScrollPosition;
				ZoomCoordinateTransformation(&xMousePosition, &yMousePosition, gv_currentZoom);
				MoveToEx(hMemoryDCDraw, xMousePosition, yMousePosition, NULL);

				firstPointOfTheLine.x = xMousePosition;
				firstPointOfTheLine.y = yMousePosition;

				drawData.x1 = xMousePosition;
				drawData.y1 = yMousePosition;

				break;
			}
			case ID_POLYGON:
			{
				if (isFirstClickPolygon)
				{
					xMousePosition = LOWORD(lParam) + xScrollPosition;
					yMousePosition = HIWORD(lParam) - toolBarHeight + yScrollPosition;
					ZoomCoordinateTransformation(&xMousePosition, &yMousePosition, gv_currentZoom);
					MoveToEx(hMemoryDCDraw, xMousePosition, yMousePosition, NULL);
					isFirstClickPolygon = 0;

					drawData.x1 = xMousePosition;
					drawData.y1 = yMousePosition;
				}
				else
				{
					xMousePosition = LOWORD(lParam) + xScrollPosition;
					yMousePosition = HIWORD(lParam) - toolBarHeight + yScrollPosition;
					ZoomCoordinateTransformation(&xMousePosition, &yMousePosition, gv_currentZoom);
					LineTo(hMemoryDCDraw, xMousePosition, yMousePosition);

					ZoomPictureAndDisplay(hDC, hMemoryDCDraw, hMemoryDCZoom, xScrollPosition, yScrollPosition, 0, NULL, NULL);

					drawData.x2 = xMousePosition;
					drawData.y2 = yMousePosition;

					drawPictureDataArray.push_back(drawData);
					drawData.arrayCoord.clear();
					::memset(&drawData, 0, sizeof(drawData));

					drawData.x1 = xMousePosition;
					drawData.y1 = yMousePosition;
				}

				break;
			}
			case ID_RECTANGLE:
			{
				xLeftRectangleOrEllipse = LOWORD(lParam) + xScrollPosition;
				yTopRectangleOrEllipse = HIWORD(lParam) - toolBarHeight + yScrollPosition;
				ZoomCoordinateTransformation(&xLeftRectangleOrEllipse, &yTopRectangleOrEllipse, gv_currentZoom);

				drawData.x1 = xLeftRectangleOrEllipse;
				drawData.y1 = yTopRectangleOrEllipse;

				break;
			}
			case ID_ELLIPSE:
			{
				xLeftRectangleOrEllipse = LOWORD(lParam) + xScrollPosition;
				yTopRectangleOrEllipse = HIWORD(lParam) - toolBarHeight + yScrollPosition;
				ZoomCoordinateTransformation(&xLeftRectangleOrEllipse, &yTopRectangleOrEllipse, gv_currentZoom);

				drawData.x1 = xLeftRectangleOrEllipse;
				drawData.y1 = yTopRectangleOrEllipse;

				break;
			}
			case ID_TEXT:
			{
				xMousePosition = LOWORD(lParam) + xScrollPosition;
				yMousePosition = HIWORD(lParam) - toolBarHeight + yScrollPosition;
				ZoomCoordinateTransformation(&xMousePosition, &yMousePosition, gv_currentZoom);

				SelectObject(hMemoryDCDraw, CreateFontIndirect(&gv_AlogicalFont));
				SetTextColor(hMemoryDCDraw, gv_AtextColor);
				int oldMode;
				if (gv_isTransparentBrush)
				{
					oldMode = SetBkMode(hMemoryDCDraw, TRANSPARENT);
				}
				else
				{
					SetBkColor(hMemoryDCDraw, colorBrush);
				}
				TextOut(hMemoryDCDraw, xMousePosition, yMousePosition, gv_AtextBuffer, lstrlen(gv_AtextBuffer));
				if (gv_isTransparentBrush)
				{
					SetBkMode(hMemoryDCDraw, oldMode);
				}

				ZoomPictureAndDisplay(hDC, hMemoryDCDraw, hMemoryDCZoom, xScrollPosition, yScrollPosition, 0, NULL, NULL);

				drawData.x1 = xMousePosition;
				drawData.y1 = yMousePosition;
				memcpy(&drawData.logicalFont, &gv_AlogicalFont, sizeof(gv_AlogicalFont));
				drawData.colorText = gv_AtextColor;
				lstrcpy(drawData.textBuffer, gv_AtextBuffer);

				drawPictureDataArray.push_back(drawData);
				drawData.arrayCoord.clear();
				::memset(&drawData, 0, sizeof(drawData));

				break;
			}
			default:
				break;
			}

			DeleteObject(pen);
			if (!gv_isTransparentBrush)
				DeleteObject(brush);
		}
		else
		{
			isDrawing = 0;
			isFirstClickPolygon = 1;
		}

		break;
	}
	case WM_MOUSEMOVE:
	{
		if (isDrawing)
		{
			int penWidth = GetNumberFromWindow(gv_hWndEditPenWidth);
			pen = CreatePen(PS_SOLID, penWidth, colorPen);
			SelectObject(hMemoryDCDraw, pen);

			if (gv_isTransparentBrush)
			{
				brush = (HBRUSH)GetStockObject(NULL_BRUSH);
			}
			else
			{
				brush = CreateSolidBrush(colorBrush);
			}
			SelectObject(hMemoryDCDraw, brush);

			switch (figureToDraw)
			{
			case ID_PENCIL:
			{
				xMousePosition = LOWORD(lParam) + xScrollPosition;
				yMousePosition = HIWORD(lParam) - toolBarHeight + yScrollPosition;
				ZoomCoordinateTransformation(&xMousePosition, &yMousePosition, gv_currentZoom);
				LineTo(hMemoryDCDraw, xMousePosition, yMousePosition);

				ZoomPictureAndDisplay(hDC, hMemoryDCDraw, hMemoryDCZoom, xScrollPosition, yScrollPosition, 0, NULL, NULL);

				drawData.arrayCoord.push_back(Point(xMousePosition, yMousePosition));

				break;
			}
			case ID_LINE:
			{
				MoveToEx(hMemoryDCDraw, firstPointOfTheLine.x, firstPointOfTheLine.y, NULL);
				xMousePosition = LOWORD(lParam) + xScrollPosition;
				yMousePosition = HIWORD(lParam) - toolBarHeight + yScrollPosition;
				ZoomCoordinateTransformation(&xMousePosition, &yMousePosition, gv_currentZoom);
				LineTo(hMemoryDCDraw, xMousePosition, yMousePosition);

				ZoomPictureAndDisplay(hDC, hMemoryDCDraw, hMemoryDCZoom, xScrollPosition, yScrollPosition, 0, NULL, NULL);

				RedrawAllPicture(hMemoryDCDraw, drawPictureDataArray, (HBRUSH)GetStockObject(WHITE_BRUSH));

				break;
			}
			case ID_RECTANGLE:
			{
				xMousePosition = LOWORD(lParam) + xScrollPosition;
				yMousePosition = HIWORD(lParam) - toolBarHeight + yScrollPosition;
				ZoomCoordinateTransformation(&xMousePosition, &yMousePosition, gv_currentZoom);
				if (wParam & MK_SHIFT)
				{
					int squareSideLength = xMousePosition - xLeftRectangleOrEllipse;
					int xRigthSquare = xLeftRectangleOrEllipse + squareSideLength;
					int yBottomSquare = yTopRectangleOrEllipse + squareSideLength;

					Rectangle(hMemoryDCDraw, xLeftRectangleOrEllipse, yTopRectangleOrEllipse, xRigthSquare, yBottomSquare);
				}
				else
				{
					Rectangle(hMemoryDCDraw, xLeftRectangleOrEllipse, yTopRectangleOrEllipse, xMousePosition, yMousePosition);
				}

				ZoomPictureAndDisplay(hDC, hMemoryDCDraw, hMemoryDCZoom, xScrollPosition, yScrollPosition, 0, NULL, NULL);
				RedrawAllPicture(hMemoryDCDraw, drawPictureDataArray, (HBRUSH)GetStockObject(WHITE_BRUSH));

				break;
			}
			case ID_ELLIPSE:
			{
				xMousePosition = LOWORD(lParam) + xScrollPosition;
				yMousePosition = HIWORD(lParam) - toolBarHeight + yScrollPosition;
				ZoomCoordinateTransformation(&xMousePosition, &yMousePosition, gv_currentZoom);
				if (wParam & MK_SHIFT)
				{
					int squareSideLength = xMousePosition - xLeftRectangleOrEllipse;
					int xRigthSquare = xLeftRectangleOrEllipse + squareSideLength;
					int yBottomSquare = yTopRectangleOrEllipse + squareSideLength;

					Ellipse(hMemoryDCDraw, xLeftRectangleOrEllipse, yTopRectangleOrEllipse, xRigthSquare, yBottomSquare);
				}
				else
				{
					Ellipse(hMemoryDCDraw, xLeftRectangleOrEllipse, yTopRectangleOrEllipse, xMousePosition, yMousePosition);
				}

				ZoomPictureAndDisplay(hDC, hMemoryDCDraw, hMemoryDCZoom, xScrollPosition, yScrollPosition, 0, NULL, NULL);
				RedrawAllPicture(hMemoryDCDraw, drawPictureDataArray, (HBRUSH)GetStockObject(WHITE_BRUSH));

				break;
			}
			}

			DeleteObject(pen);
			if (!gv_isTransparentBrush)
				DeleteObject(brush);
		}

		break;
	}
	case WM_LBUTTONUP:
	{
		int penWidth = GetNumberFromWindow(gv_hWndEditPenWidth);
		pen = CreatePen(PS_SOLID, penWidth, colorPen);
		SelectObject(hMemoryDCDraw, pen);
		if (gv_isTransparentBrush)
		{
			brush = (HBRUSH)GetStockObject(NULL_BRUSH);
		}
		else
		{
			brush = CreateSolidBrush(colorBrush);
		}
		SelectObject(hMemoryDCDraw, brush);

		if (isDrawing)
		{
			switch (figureToDraw)
			{
			case ID_PENCIL:
			{
				drawPictureDataArray.push_back(drawData);
				drawData.arrayCoord.clear();
				::memset(&drawData, 0, sizeof(drawData));

				break;
			}
			case ID_LINE:
			{
				MoveToEx(hMemoryDCDraw, firstPointOfTheLine.x, firstPointOfTheLine.y, NULL);
				xMousePosition = LOWORD(lParam) + xScrollPosition;
				yMousePosition = HIWORD(lParam) - toolBarHeight + yScrollPosition;
				ZoomCoordinateTransformation(&xMousePosition, &yMousePosition, gv_currentZoom);
				LineTo(hMemoryDCDraw, xMousePosition, yMousePosition);

				ZoomPictureAndDisplay(hDC, hMemoryDCDraw, hMemoryDCZoom, xScrollPosition, yScrollPosition, 0, NULL, NULL);

				drawData.x2 = xMousePosition;
				drawData.y2 = yMousePosition;

				drawPictureDataArray.push_back(drawData);
				drawData.arrayCoord.clear();
				::memset(&drawData, 0, sizeof(drawData));

				break;
			}
			case ID_RECTANGLE:
			{
				xMousePosition = LOWORD(lParam) + xScrollPosition;
				yMousePosition = HIWORD(lParam) - toolBarHeight + yScrollPosition;
				ZoomCoordinateTransformation(&xMousePosition, &yMousePosition, gv_currentZoom);
				if (wParam == MK_SHIFT)
				{
					int squareSideLength = xMousePosition - xLeftRectangleOrEllipse;
					int xRigthSquare = xLeftRectangleOrEllipse + squareSideLength;
					int yBottomSquare = yTopRectangleOrEllipse + squareSideLength;

					Rectangle(hMemoryDCDraw, xLeftRectangleOrEllipse, yTopRectangleOrEllipse, xRigthSquare, yBottomSquare);

					drawData.x2 = xRigthSquare;
					drawData.y2 = yBottomSquare;
				}
				else
				{
					Rectangle(hMemoryDCDraw, xLeftRectangleOrEllipse, yTopRectangleOrEllipse, xMousePosition, yMousePosition);

					drawData.x2 = xMousePosition;
					drawData.y2 = yMousePosition;
				}

				ZoomPictureAndDisplay(hDC, hMemoryDCDraw, hMemoryDCZoom, xScrollPosition, yScrollPosition, 0, NULL, NULL);

				drawPictureDataArray.push_back(drawData);
				drawData.arrayCoord.clear();
				::memset(&drawData, 0, sizeof(drawData));

				break;
			}
			case ID_ELLIPSE:
			{
				xMousePosition = LOWORD(lParam) + xScrollPosition;
				yMousePosition = HIWORD(lParam) - toolBarHeight + yScrollPosition;
				ZoomCoordinateTransformation(&xMousePosition, &yMousePosition, gv_currentZoom);
				if (wParam == MK_SHIFT)
				{
					int squareSideLength = xMousePosition - xLeftRectangleOrEllipse;
					int xRigthSquare = xLeftRectangleOrEllipse + squareSideLength;
					int yBottomSquare = yTopRectangleOrEllipse + squareSideLength;

					Ellipse(hMemoryDCDraw, xLeftRectangleOrEllipse, yTopRectangleOrEllipse, xRigthSquare, yBottomSquare);

					drawData.x2 = xRigthSquare;
					drawData.y2 = yBottomSquare;
				}
				else
				{
					Ellipse(hMemoryDCDraw, xLeftRectangleOrEllipse, yTopRectangleOrEllipse, xMousePosition, yMousePosition);

					drawData.x2 = xMousePosition;
					drawData.y2 = yMousePosition;
				}

				ZoomPictureAndDisplay(hDC, hMemoryDCDraw, hMemoryDCZoom, xScrollPosition, yScrollPosition, 0, NULL, NULL);

				drawPictureDataArray.push_back(drawData);
				drawData.arrayCoord.clear();
				::memset(&drawData, 0, sizeof(drawData));

				break;
			}
			}
		}

		isDrawing = 0;
		DeleteObject(pen);
		if (!gv_isTransparentBrush)
			DeleteObject(brush);

		break;
	}
	case WM_VSCROLL:
	{
		SCROLLINFO scrollInfo = { sizeof(SCROLLINFO) };
		scrollInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_TRACKPOS;
		GetScrollInfo(hWnd, SB_VERT, &scrollInfo);
		yScrollPosition = scrollInfo.nPos;

		switch (LOWORD(wParam))
		{
		case SB_LINEUP:
		{
			if (scrollInfo.nPos <= 0)
				break;
			yScrollPosition -= yScrollStep;
			break;
		}
		case SB_LINEDOWN:
		{
			if ((scrollInfo.nMax - scrollInfo.nPage) <= scrollInfo.nPos)
				break;
			yScrollPosition += yScrollStep;
			break;
		}
		case SB_TOP:            yScrollPosition = scrollInfo.nMin; break;
		case SB_BOTTOM:         yScrollPosition = scrollInfo.nMax; break;
		case SB_THUMBTRACK:     yScrollPosition = scrollInfo.nTrackPos; break;
		default:
		case SB_THUMBPOSITION:  yScrollPosition = scrollInfo.nPos; break;
		}

		ZoomPictureAndDisplay(hDC, hMemoryDCDraw, hMemoryDCZoom, xScrollPosition, yScrollPosition, 0, NULL, NULL);
		SetScrollPos(hWnd, SB_VERT, yScrollPosition, TRUE);

		break;
	}
	case WM_HSCROLL:
	{
		SCROLLINFO scrollInfo = { sizeof(SCROLLINFO) };
		scrollInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_TRACKPOS;
		GetScrollInfo(hWnd, SB_HORZ, &scrollInfo);
		xScrollPosition = scrollInfo.nPos;

		switch (LOWORD(wParam))
		{
		case SB_LINEUP:
		{
			if (scrollInfo.nPos <= 0)
				break;
			xScrollPosition -= xScrollStep;
			break;
		}
		case SB_LINEDOWN:
		{
			if ((scrollInfo.nMax - scrollInfo.nPage) <= scrollInfo.nPos)
				break;
			xScrollPosition += xScrollStep;
			break;
		}
		case SB_TOP:            xScrollPosition = scrollInfo.nMin; break;
		case SB_BOTTOM:         xScrollPosition = scrollInfo.nMax; break;
		case SB_THUMBTRACK:     xScrollPosition = scrollInfo.nTrackPos; break;
		default:
		case SB_THUMBPOSITION:  xScrollPosition = scrollInfo.nPos; break;
		}

		ZoomPictureAndDisplay(hDC, hMemoryDCDraw, hMemoryDCZoom, xScrollPosition, yScrollPosition, 0, NULL, NULL);
		SetScrollPos(hWnd, SB_HORZ, xScrollPosition, TRUE);

		break;
	}
	case WM_KEYDOWN:
	{
		if (IsIconic(hWnd))	break;

		if (!IsKeyPressed(VK_CONTROL))
		{
			if (IsZoomed(hWnd)) break;

			switch (wParam)
			{
			case VK_UP:     SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEUP, 0), NULL); break;
			case VK_DOWN:   SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), NULL); break;
			case VK_LEFT:   SendMessage(hWnd, WM_HSCROLL, MAKEWPARAM(SB_LINEUP, 0), NULL); break;
			case VK_RIGHT:  SendMessage(hWnd, WM_HSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), NULL); break;
			}
		}
		else 
		{
			switch (wParam)
			{
			case VK_UP: SendMessage(hWnd, WM_MOUSEWHEEL, MAKEWPARAM(MK_CONTROL, 120), 0); break;
			case VK_DOWN: SendMessage(hWnd, WM_MOUSEWHEEL, MAKEWPARAM(MK_CONTROL, -120), 0); break;
			}
		}
		break;
	}
	case WM_MOUSEWHEEL:
	{
		if (IsIconic(hWnd))
			break;

		SCROLLINFO scrollInfo = { sizeof(SCROLLINFO) };
		scrollInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_TRACKPOS;

		if (LOWORD(wParam) & MK_SHIFT)
		{
			if (IsZoomed(hWnd))
				break;

			GetScrollInfo(hWnd, SB_HORZ, &scrollInfo);
			xScrollPosition += GetMouseWheelInterval(HIWORD(wParam), xScrollStep);
			if (xScrollPosition < scrollInfo.nMin)
			{
				xScrollPosition = scrollInfo.nMin;
			}
			if (xScrollPosition >= scrollInfo.nMax - scrollInfo.nPage)
			{
				SendMessage(hWnd, WM_HSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), NULL);
				SendMessage(hWnd, WM_HSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), NULL);
				SendMessage(hWnd, WM_HSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), NULL);
				break;
			}

			ZoomPictureAndDisplay(hDC, hMemoryDCDraw, hMemoryDCZoom, xScrollPosition, yScrollPosition, 0, NULL, NULL);
			SetScrollPos(hWnd, SB_HORZ, xScrollPosition, TRUE);
		}
		else
		{
			if (!(LOWORD(wParam) & MK_CONTROL))
			{
				if (IsZoomed(hWnd))
					break;

				GetScrollInfo(hWnd, SB_VERT, &scrollInfo);
				yScrollPosition += GetMouseWheelInterval(HIWORD(wParam), yScrollStep);
				if (yScrollPosition < scrollInfo.nMin)
				{
					yScrollPosition = scrollInfo.nMin;
				}
				if (yScrollPosition >= (scrollInfo.nMax - scrollInfo.nPage))
				{
					SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), NULL);
					SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), NULL);
					SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), NULL);
					break;
				}

				ZoomPictureAndDisplay(hDC, hMemoryDCDraw, hMemoryDCZoom, xScrollPosition, yScrollPosition, 0, NULL, NULL);
				SetScrollPos(hWnd, SB_VERT, yScrollPosition, TRUE);
			}
		}

		if (LOWORD(wParam) & MK_CONTROL)
		{
			static int isZoomLimitReached = 0;
			int zoomInc = GetMouseWheelInterval(HIWORD(wParam), zoomStep);
			ZoomPictureAndDisplay(hDC, hMemoryDCDraw, hMemoryDCZoom, xScrollPosition, yScrollPosition, zoomInc, NULL, &isZoomLimitReached);

			if (IsZoomed(hWnd))
			{
				RECT rectangleClient;
				GetPictureRectangle(&rectangleClient, NULL, NULL);
				if (!isZoomLimitReached)
					FillRect(hDC, &rectangleClient, (HBRUSH)GetStockObject(GRAY_BRUSH));
				ZoomPictureAndDisplay(hDC, hMemoryDCDraw, hMemoryDCZoom, xScrollPosition, yScrollPosition, zoomInc, NULL, &isZoomLimitReached);
			}

			SetStatusBarText(figureToDraw, gv_currentZoom);
		}

		break;
	}
	case MY_MESSAGE_SET_ZOOM:
	{
		int newZoom = wParam;
		ZoomPictureAndDisplay(hDC, hMemoryDCDraw, hMemoryDCZoom, xScrollPosition, yScrollPosition, 0, &newZoom, NULL);
		SetStatusBarText(figureToDraw, gv_currentZoom);
		break;
	}
	case MY_MESSAGE_CHANGE_STRETCHING_MODE:
	{
		POINT pt;
		if (GetBrushOrgEx(hMemoryDCZoom, &pt) == 0)
			PrintError(GetLastError(), _TEXT("GetBrushOrgEx()"));
		if (gv_isHALFTONE)
		{
			if (SetStretchBltMode(hMemoryDCZoom, HALFTONE) == 0)
				PrintError(GetLastError(), _TEXT("SetStretchBltMode()"));
		}
		else
		{
			if (SetStretchBltMode(hMemoryDCZoom, oldStretchMode) == 0)
				PrintError(GetLastError(), _TEXT("SetStretchBltMode()"));
		}
		if (SetBrushOrgEx(hMemoryDCZoom, pt.x, pt.y, NULL) == 0)
			PrintError(GetLastError(), _TEXT("SetBrushOrgEx()"));

		break;
	}
	case WM_SETCURSOR:
	{
		static HCURSOR cursorARROW = LoadCursor(NULL, IDC_ARROW), cursorIBEAM = LoadCursor(NULL, IDC_IBEAM);
		if ((figureToDraw == ID_TEXT) && ((HWND)wParam == gv_hWndMainWindow) && (LOWORD(lParam) <= HTCLIENT))
		{
			SetCursor(cursorIBEAM);
		}
		else
		{
			SetCursor(cursorARROW);
		}

		break;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);

		static int justOneTime = 1;
		if (justOneTime)
		{
			justOneTime = 0;
			CreateMemoryDC(hDC, &hMemoryDCDraw, &hBitMapMemoryDCDraw);
			CreateMemoryDC(hDC, &hMemoryDCZoom, &hBitMapMemoryDCZoom);
			GetPictureRectangle(NULL, &toolBarHeight, NULL);
			
			oldStretchMode = GetStretchBltMode(hMemoryDCZoom);
			if (oldStretchMode == 0)
				PrintError(GetLastError(), _TEXT("GetStretchBltMode()"));
			if (gv_isHALFTONE)
			{
				POINT pt;
				if (GetBrushOrgEx(hMemoryDCZoom, &pt) == 0)
					PrintError(GetLastError(), _TEXT("GetBrushOrgEx()"));
				if (SetStretchBltMode(hMemoryDCZoom, HALFTONE) == 0)
					PrintError(GetLastError(), _TEXT("SetStretchBltMode()"));
				if (SetBrushOrgEx(hMemoryDCZoom, pt.x, pt.y, NULL) == 0)
					PrintError(GetLastError(), _TEXT("SetBrushOrgEx()"));
			}
		}
		else
		{
			RedrawAllPicture(hMemoryDCDraw, drawPictureDataArray, (HBRUSH)GetStockObject(WHITE_BRUSH));
			if (IsZoomed(hWnd))
			{
				xScrollPosition = 0;
				yScrollPosition = 0;

				RECT rectangleClient;
				GetPictureRectangle(&rectangleClient, NULL, NULL);
				FillRect(hDC, &rectangleClient, (HBRUSH)GetStockObject(GRAY_BRUSH));
			}

			ZoomPictureAndDisplay(hDC, hMemoryDCDraw, hMemoryDCZoom, xScrollPosition, yScrollPosition, 0, NULL, NULL);
			SetStatusBarText(figureToDraw, gv_currentZoom);
		}

		SetScrollBarsOptions(hWnd, xScrollPosition, yScrollPosition);

		EndPaint(hWnd, &ps);
	}
	break;
	case WM_SIZE:
	{
		SendMessage(gv_hWndToolBar, TB_AUTOSIZE, 0, 0);
		SendMessage(gv_hWndStatusBar, WM_SIZE, wParam, lParam);
		SetScrollBarsOptions(hWnd, xScrollPosition, yScrollPosition);
		break;
	}
	case WM_NOTIFY:
	{
		LPTOOLTIPTEXT lpTTT = (LPTOOLTIPTEXT)lParam;
		if (lpTTT->hdr.code == TTN_NEEDTEXT)
		{
			lpTTT->hinst = GetModuleHandle(NULL);
			lpTTT->lpszText = MAKEINTRESOURCE(lpTTT->hdr.idFrom);
		}
		break;
	}
	case WM_COMMAND:
	{
		On_WM_COMMAND(wParam, lParam, &figureToDraw, &colorPen, &colorBrush, &drawPictureDataArray, hMemoryDCDraw, hMemoryDCZoom);
		break;
	}
	case WM_DESTROY:
	{
		DeleteDC(hMemoryDCDraw);
		DeleteObject(hBitMapMemoryDCDraw);
		DeleteDC(hMemoryDCZoom);
		DeleteObject(hBitMapMemoryDCZoom);
		ReleaseDC(hWnd, hDC);

		DeleteFile(gv_tempFileNameForPrint);

		PostQuitMessage(0);
		break;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

HWND InitializeToolBar(HWND hWindow)
{
	// Separator also a button.
	#define COUNT_TOOL_BAR_BUTTONS 12
	const int SEPARATOR_WIDTH = 15;
	const int SEPARATOR_WIDTH_FOR_EDIT = 20;
	HWND hWndToolBar;

	TBBUTTON toolBarButtons[COUNT_TOOL_BAR_BUTTONS] = { 0 };

	int i = 0;
	toolBarButtons[i].iBitmap = i;
	toolBarButtons[i].idCommand = ID_UNDO;
	toolBarButtons[i].fsState = TBSTATE_ENABLED;
	toolBarButtons[i++].fsStyle = TBSTYLE_BUTTON;

	toolBarButtons[i].iBitmap = i;
	toolBarButtons[i].idCommand = ID_CURSOR;
	toolBarButtons[i].fsState = TBSTATE_ENABLED;
	toolBarButtons[i++].fsStyle = TBSTYLE_BUTTON;

	toolBarButtons[i].iBitmap = i;
	toolBarButtons[i].idCommand = ID_PENCIL;
	toolBarButtons[i].fsState = TBSTATE_ENABLED;
	toolBarButtons[i++].fsStyle = TBSTYLE_BUTTON;

	toolBarButtons[i].iBitmap = i;
	toolBarButtons[i].idCommand = ID_LINE;
	toolBarButtons[i].fsState = TBSTATE_ENABLED;
	toolBarButtons[i++].fsStyle = TBSTYLE_BUTTON;

	toolBarButtons[i].iBitmap = i;
	toolBarButtons[i].idCommand = ID_POLYGON;
	toolBarButtons[i].fsState = TBSTATE_ENABLED;
	toolBarButtons[i++].fsStyle = TBSTYLE_BUTTON;

	toolBarButtons[i].iBitmap = i;
	toolBarButtons[i].idCommand = ID_RECTANGLE;
	toolBarButtons[i].fsState = TBSTATE_ENABLED;
	toolBarButtons[i++].fsStyle = TBSTYLE_BUTTON;

	toolBarButtons[i].iBitmap = i;
	toolBarButtons[i].idCommand = ID_ELLIPSE;
	toolBarButtons[i].fsState = TBSTATE_ENABLED;
	toolBarButtons[i++].fsStyle = TBSTYLE_BUTTON;

	toolBarButtons[i].iBitmap = i;
	toolBarButtons[i].idCommand = ID_TEXT;
	toolBarButtons[i].fsState = TBSTATE_ENABLED;
	toolBarButtons[i++].fsStyle = TBSTYLE_BUTTON;

	toolBarButtons[i].iBitmap = SEPARATOR_WIDTH;
	toolBarButtons[i].idCommand = ID_SEPARATOR_1;
	toolBarButtons[i].fsState = TBSTATE_ENABLED;
	toolBarButtons[i++].fsStyle = TBSTYLE_SEP;

	toolBarButtons[i].iBitmap = SEPARATOR_WIDTH_FOR_EDIT;
	toolBarButtons[i].idCommand = ID_SEPARATOR_FOR_EDIT;
	toolBarButtons[i].fsState = TBSTATE_ENABLED;
	toolBarButtons[i++].fsStyle = TBSTYLE_SEP;

	toolBarButtons[i].iBitmap = i;
	toolBarButtons[i].idCommand = ID_CHOOSE_COLOR_PEN;
	toolBarButtons[i].fsState = TBSTATE_ENABLED;
	toolBarButtons[i++].fsStyle = TBSTYLE_BUTTON;

	toolBarButtons[i].iBitmap = i;
	toolBarButtons[i].idCommand = ID_CHOOSE_COLOR_BRUSH;
	if (gv_isTransparentBrush)
	{
		toolBarButtons[i].fsState = TBSTATE_HIDDEN;
	}
	else
	{
		toolBarButtons[i].fsState = TBSTATE_ENABLED;
	}
	toolBarButtons[i++].fsStyle = TBSTYLE_BUTTON;

	hWndToolBar = CreateToolbarEx(hWindow,
		WS_CHILD | WS_VISIBLE | WS_BORDER | TBSTYLE_TOOLTIPS | TBSTYLE_WRAPABLE,
		IDR_TOOLBAR, COUNT_TOOL_BAR_BUTTONS, GetModuleHandle(NULL), IDR_TOOLBAR1,
		toolBarButtons, COUNT_TOOL_BAR_BUTTONS, 0, 0, 0, 0, sizeof(TBBUTTON));

	if (hWndToolBar == NULL)
		PrintError(GetLastError(), _TEXT("CreateToolbarEx()"));
	SendMessage(hWndToolBar, TB_AUTOSIZE, 0, 0);

	const int x = 196, y = 2, width = 22, height = 21;
	const TCHAR EDIT_PEN_WIDTH_START_TEXT[] = _TEXT("1");
	gv_hWndEditPenWidth = CreateWindow(_TEXT("EDIT"), EDIT_PEN_WIDTH_START_TEXT, WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_CENTER,
		x, y, width, height, hWndToolBar, (HMENU)IDC_EDIT_PEN_WIDTH2, GetModuleHandle(NULL), 0);

	if (!gv_hWndEditPenWidth)
		PrintError(GetLastError(), _TEXT("CreateWindow()"));

	SendMessage(gv_hWndEditPenWidth, EM_SETLIMITTEXT, 2, 0);

	return hWndToolBar;
}

void On_WM_COMMAND(WPARAM wParam, LPARAM lParam, int* figureToDraw, LPCOLORREF colorPen, LPCOLORREF colorBrush,
	std::vector<DrawPictureData>* drawPictureDataArray, HDC hMemoryDCDraw, HDC hMemoryDCZoom)
{
	SendMessage(gv_hWndStatusBar, SB_SIMPLE, TRUE, 0);

	static int isSavedAs = 0;

	switch (LOWORD(wParam))
	{
	case IDM_NEW:
	{
		if (!gv_isFileSaved)
		{
			TCHAR text[] = _TEXT("The picture is not saved. Save the picture?");
			TCHAR Òaption[] = _TEXT("Message");
			UINT styles = MB_YESNOCANCEL | MB_ICONQUESTION;
			int userAnser = MessageBox(gv_hWndMainWindow, text, Òaption, styles);

			if (userAnser == IDYES)
			{
				SendMessage(gv_hWndMainWindow, WM_COMMAND, MAKEWPARAM(IDM_FILE_SAVE, 0), (LPARAM)gv_hWndMainWindow);
			}

			if (userAnser == IDCANCEL)
			{
				break;
			}
		}

		RECT rectangleClient, rectangleMemoryDC;
		GetPictureRectangle(&rectangleClient, NULL, NULL);
		GetMemoryDCRectangle(&rectangleMemoryDC);

		HDC hDC = GetDC(gv_hWndMainWindow);
		FillRect(hDC, &rectangleClient, (HBRUSH)GetStockObject(WHITE_BRUSH));
		FillRect(hMemoryDCDraw, &rectangleMemoryDC, (HBRUSH)GetStockObject(WHITE_BRUSH));
		ReleaseDC(gv_hWndMainWindow, hDC);

		int zoom = 0;
		SendMessage(gv_hWndMainWindow, MY_MESSAGE_SET_ZOOM, zoom, 0);

		gv_isFileSaved = 0;
		gv_isFileOpen = 0;
		drawPictureDataArray->clear();

		break;
	}
	case ID_UNDO:
	{
		if (!drawPictureDataArray->empty())
		{
			drawPictureDataArray->pop_back();
			InvalidateRect(gv_hWndMainWindow, NULL, TRUE);
			UpdateWindow(gv_hWndMainWindow);
		}
		break;
	}
	case ID_CURSOR:
	{
		SetStatusBarText(LOWORD(wParam), gv_currentZoom);
		(*figureToDraw) = LOWORD(wParam);
		break;
	}
	case ID_PENCIL:
	{
		SetStatusBarText(LOWORD(wParam), gv_currentZoom);
		(*figureToDraw) = LOWORD(wParam);
		break;
	}
	case ID_LINE:
	{
		SetStatusBarText(LOWORD(wParam), gv_currentZoom);
		(*figureToDraw) = LOWORD(wParam);
		break;
	}
	case ID_POLYGON:
	{
		SetStatusBarText(LOWORD(wParam), gv_currentZoom);
		(*figureToDraw) = LOWORD(wParam);
		break;
	}
	case ID_RECTANGLE:
	{
		SetStatusBarText(LOWORD(wParam), gv_currentZoom);
		(*figureToDraw) = LOWORD(wParam);

		break;
	}
	case ID_ELLIPSE:
	{
		SetStatusBarText(LOWORD(wParam), gv_currentZoom);
		(*figureToDraw) = LOWORD(wParam);
		break;
	}
	case ID_TEXT:
	{
		TCHAR messageAbout[] = _TEXT("ÕÂ Â‡ÎËÁÓ‚‡ÌÓ");
		TCHAR messageBoxCaption[] = _TEXT("Not implemented");
		MessageBox(gv_hWndMainWindow, messageAbout, messageBoxCaption, MB_OK | MB_ICONINFORMATION);
		break;
	}
	case ID_CHOOSE_COLOR_PEN:
	{
		COLORREF color;
		if (GetColorFromChooseColorDialog(&color, gv_hWndMainWindow))
		{
			// ok button pressed. 
		}
		else
		{
			// cancel button pressed.
		}

		(*colorPen) = color;

		break;
	}
	case ID_CHOOSE_COLOR_BRUSH:
	{
		COLORREF color;
		if (GetColorFromChooseColorDialog(&color, gv_hWndMainWindow))
		{
			// ok button pressed. 
		}
		else
		{
			// cancel button pressed.
		}

		(*colorBrush) = color;

		break;
	}
	default:
		break;
	}
}

void SetStatusBarText(int figureID, int currentZoom)
{
	TCHAR statusBarText[100] = { 0 };

	LoadString(GetModuleHandle(NULL), figureID, statusBarText, sizeof(statusBarText));
	wsprintf(statusBarText, _TEXT("%s.   Zoom: %d"), statusBarText, currentZoom);
	SendMessage(gv_hWndStatusBar, SB_SETTEXT, 255, (LPARAM)statusBarText);
}

int GetColorFromChooseColorDialog(LPCOLORREF color, HWND hWndDialogOwner)
{
	CHOOSECOLOR chooseColor = { 0 };
	static COLORREF acrCustClr[16];

	chooseColor.lStructSize = sizeof(chooseColor);
	chooseColor.hwndOwner = hWndDialogOwner;
	chooseColor.lpCustColors = (LPDWORD)acrCustClr;
	chooseColor.rgbResult = 0;
	chooseColor.Flags = CC_FULLOPEN;

	if (ChooseColor(&chooseColor) != 0)
	{
		(*color) = chooseColor.rgbResult;
		return 1;
	}

	return 0;
}

int GetFontFromChooseFontDialog(LPLOGFONT font, LPCOLORREF textColor, HWND hWndDialogOwner)
{
	CHOOSEFONT chooseFont = { 0 };
	LOGFONT tempFont = { 0 };

	chooseFont.lStructSize = sizeof(chooseFont);
	chooseFont.hwndOwner = hWndDialogOwner;
	chooseFont.lpLogFont = &tempFont;
	chooseFont.Flags = CF_SCREENFONTS | CF_EFFECTS;

	if (ChooseFont(&chooseFont) == TRUE)
	{
		memcpy(font, &tempFont, sizeof(tempFont));
		memcpy(textColor, &chooseFont.rgbColors, sizeof(COLORREF));
		return 1;
	}

	return 0;
}

BOOL CALLBACK DialogProcedure_Text(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		/*
		There are 2 ways to set focus on edit:
		1) SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg, IDC_EDIT1), (LPARAM)TRUE);
		2) SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
		*/

		SetFocus(GetDlgItem(hDlg, IDC_EDIT_TEXT));

		// In response to a WM_INITDIALOG message, the dialog box procedure should return zero 
		// if it calls the SetFocus function to set the focus to one of the controls in the dialog box. 
		// Otherwise, it should return nonzero, in which case the system sets the focus to the first control 
		// in the dialog box that can be given the focus.
		return 0;
		//	return TRUE;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			::memset(gv_AtextBuffer, 0, sizeof(gv_AtextBuffer));
			SendMessage(GetDlgItem(hDlg, IDC_EDIT_TEXT), WM_GETTEXT, sizeof(gv_AtextBuffer) / sizeof(TCHAR), (LPARAM)&gv_AtextBuffer);

			EndDialog(hDlg, TRUE);
			return TRUE;
		}
		case IDCANCEL:
		{
			EndDialog(hDlg, FALSE);
			return TRUE;
		}
		case IDC_BUTTON_FONT:
		{
			GetFontFromChooseFontDialog(&gv_AlogicalFont, &gv_AtextColor, hDlg);
			return TRUE;
		}
		}
		break;
	}
	return FALSE;
}

BOOL CALLBACK DialogProcedure_Settings(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#define BUFFER_SIZE_ZOOM 5
#define BUFFER_SIZE_PEN_WIDTH 3
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		SetFocus(GetDlgItem(hDlg, IDC_EDIT_PEN_WIDTH));
		SendMessage(GetDlgItem(hDlg, IDC_EDIT_PEN_WIDTH), EM_SETLIMITTEXT, BUFFER_SIZE_PEN_WIDTH - 1, 0);
		SendMessage(GetDlgItem(hDlg, IDC_EDIT_ZOOM), EM_SETLIMITTEXT, BUFFER_SIZE_ZOOM - 1, 0);

		TCHAR penWidthStr[BUFFER_SIZE_PEN_WIDTH] = { 0 };
		SendMessage(gv_hWndEditPenWidth, WM_GETTEXT, sizeof(penWidthStr) / sizeof(TCHAR), (LPARAM)&penWidthStr);
		SendMessage(GetDlgItem(hDlg, IDC_EDIT_PEN_WIDTH), WM_SETTEXT, 0, (LPARAM)&penWidthStr);

		TCHAR zoomStr[BUFFER_SIZE_ZOOM] = { 0 };
		wsprintf(zoomStr, _TEXT("%d"), gv_currentZoom);
		SendMessage(GetDlgItem(hDlg, IDC_EDIT_ZOOM), WM_SETTEXT, 0, (LPARAM)&zoomStr);

		if (gv_isTransparentBrush)
		{
			CheckDlgButton(hDlg, IDC_CHECKBOX_TRANSTORENT_BRUSH, BST_CHECKED);
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_BRUSH_COLOR), FALSE);
		}
		else
		{
			CheckDlgButton(hDlg, IDC_CHECKBOX_TRANSTORENT_BRUSH, BST_UNCHECKED);
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_BRUSH_COLOR), FALSE);
		}

		if (gv_isHALFTONE)
		{
			CheckDlgButton(hDlg, IDC_CHECKBOX_STRETCHING_MODE, BST_CHECKED);
		}
		else
		{
			CheckDlgButton(hDlg, IDC_CHECKBOX_STRETCHING_MODE, BST_UNCHECKED);
		}

		return 0;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			TCHAR penWidthStr[BUFFER_SIZE_PEN_WIDTH] = { 0 };
			SendMessage(GetDlgItem(hDlg, IDC_EDIT_PEN_WIDTH), WM_GETTEXT, sizeof(penWidthStr) / sizeof(TCHAR), (LPARAM)&penWidthStr);

			SendMessage(gv_hWndEditPenWidth, WM_SETTEXT, 0, (LPARAM)&penWidthStr);

			UINT checked = IsDlgButtonChecked(hDlg, IDC_CHECKBOX_TRANSTORENT_BRUSH);
			if (checked == BST_CHECKED)
			{
				gv_isTransparentBrush = 1;
				SendMessage(gv_hWndToolBar, TB_SETSTATE, ID_CHOOSE_COLOR_BRUSH, (LPARAM)MAKELPARAM(TBSTATE_HIDDEN, 0));
			}
			else
			{
				gv_isTransparentBrush = 0;
				SendMessage(gv_hWndToolBar, TB_SETSTATE, ID_CHOOSE_COLOR_BRUSH, (LPARAM)MAKELPARAM(TBSTATE_ENABLED, 0));
			}

			checked = IsDlgButtonChecked(hDlg, IDC_CHECKBOX_STRETCHING_MODE);
			if (checked == BST_CHECKED)
			{
				gv_isHALFTONE = 1;
				SendMessage(gv_hWndMainWindow, MY_MESSAGE_CHANGE_STRETCHING_MODE, 0, 0);
			}
			else
			{
				gv_isHALFTONE = 0;
				SendMessage(gv_hWndMainWindow, MY_MESSAGE_CHANGE_STRETCHING_MODE, 0, 0);
			}

			int zoom = GetNumberFromWindow(GetDlgItem(hDlg, IDC_EDIT_ZOOM));
			SendMessage(gv_hWndMainWindow, MY_MESSAGE_SET_ZOOM, zoom, 0);

			EndDialog(hDlg, TRUE);
			return TRUE;
		}
		case IDCANCEL:
		{
			EndDialog(hDlg, FALSE);
			return TRUE;
		}
		case IDC_BUTTON_PEN_COLOR:
		{
			SendMessage(gv_hWndMainWindow, WM_COMMAND, MAKEWPARAM(ID_CHOOSE_COLOR_PEN, 1), (LPARAM)gv_hWndMainWindow);
			return TRUE;
		}
		case IDC_BUTTON_BRUSH_COLOR:
		{
			SendMessage(gv_hWndMainWindow, WM_COMMAND, MAKEWPARAM(ID_CHOOSE_COLOR_BRUSH, 1), (LPARAM)gv_hWndMainWindow);
			return TRUE;
		}
		case IDC_CHECKBOX_TRANSTORENT_BRUSH:
		{
			UINT checked = IsDlgButtonChecked(hDlg, IDC_CHECKBOX_TRANSTORENT_BRUSH);
			if (checked == BST_CHECKED)
			{
				EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_BRUSH_COLOR), FALSE);
			}
			else
			{
				EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_BRUSH_COLOR), TRUE);
			}
			break;
		}
		}
		break;
	}
	return FALSE;
}

void GetMemoryDCRectangle(RECT* rectangleMemoryDC)
{
	if (rectangleMemoryDC == NULL) return;

	RECT rectangleToolBar, rectangleStatusBar;
	int statusBarHeight, toolBarHeight;

	GetWindowRect(gv_hWndToolBar, &rectangleToolBar);
	GetWindowRect(gv_hWndStatusBar, &rectangleStatusBar);

	toolBarHeight = rectangleToolBar.bottom - rectangleToolBar.top;
	statusBarHeight = rectangleStatusBar.bottom - rectangleStatusBar.top;

	int fullScreenClientWidth = GetSystemMetrics(SM_CXFULLSCREEN);
	int fullScreenClientHeight = GetSystemMetrics(SM_CYFULLSCREEN) - statusBarHeight - toolBarHeight - 20;

	(*rectangleMemoryDC).top = 0; (*rectangleMemoryDC).left = 0;
	(*rectangleMemoryDC).right = fullScreenClientWidth;
	(*rectangleMemoryDC).bottom = fullScreenClientHeight;
}

void CreateMemoryDC(HDC hDC, HDC* hMemoryDCDraw, HBITMAP* hBitMapMemoryDCDraw)
{
	if ((hMemoryDCDraw == NULL) || (hBitMapMemoryDCDraw == NULL)) return;

	RECT rectanglePicture;
	GetMemoryDCRectangle(&rectanglePicture);

	int fullScreenClientWidth = rectanglePicture.right - rectanglePicture.left;
	int fullScreenClientHeight = rectanglePicture.bottom - rectanglePicture.top;

	if (((*hMemoryDCDraw) = CreateCompatibleDC(hDC)) == NULL)
		PrintError(GetLastError(), _TEXT("CreateCompatibleDC()"));
	if (((*hBitMapMemoryDCDraw) = CreateCompatibleBitmap(hDC, fullScreenClientWidth, fullScreenClientHeight)) == NULL)
		PrintError(GetLastError(), _TEXT("CreateCompatibleBitmap()"));
	SelectObject((*hMemoryDCDraw), (*hBitMapMemoryDCDraw));

	FillRect(*hMemoryDCDraw, &rectanglePicture, (HBRUSH)GetStockObject(WHITE_BRUSH));
}

void GetPictureRectangle(LPRECT pictureRectangle, int* toolBarHeight, int* statusBarHeight)
{
	RECT rectangleToolBar, rectangleStatusBar;
	int _toolBarHeight = 0, _statusBarHeight = 0;

	if (pictureRectangle != NULL)
		GetClientRect(gv_hWndMainWindow, pictureRectangle);
	GetWindowRect(gv_hWndToolBar, &rectangleToolBar);
	GetWindowRect(gv_hWndStatusBar, &rectangleStatusBar);

	_toolBarHeight = rectangleToolBar.bottom - rectangleToolBar.top;
	_statusBarHeight = rectangleStatusBar.bottom - rectangleStatusBar.top;

	if (toolBarHeight != NULL)
		(*toolBarHeight) = _toolBarHeight;
	if (statusBarHeight != NULL)
		(*statusBarHeight) = _statusBarHeight;

	if (pictureRectangle != NULL)
	{
		pictureRectangle->top += _toolBarHeight - 1;
		pictureRectangle->bottom -= _statusBarHeight;
	}
}

void ZoomCoordinateTransformation(int* xMousePosition, int* yMousePosition, int currentZoom)
{
	RECT rectanglePicture;
	GetPictureRectangle(&rectanglePicture, NULL, NULL);

	int clientWidth = rectanglePicture.right - rectanglePicture.left;
	int clientHeight = rectanglePicture.bottom - rectanglePicture.top;
	int clientWidthZoomed = clientWidth + currentZoom;
	int clientHeightZoomed = clientHeight + currentZoom;

	double xCoefficient = (double)clientWidth / clientWidthZoomed;
	double yCoefficient = (double)clientHeight / clientHeightZoomed;

	(*xMousePosition) /= xCoefficient;
	(*yMousePosition) /= yCoefficient;
}

void ZoomPictureAndDisplay(HDC hDC, HDC hMemoryDCDraw, HDC hMemoryDCZoom, int xScrollPosition, int yScrollPosition, 
	int zoomInc, int* setZoom, int* isZoomLimitReached)
{
	RECT rectanglePicture;
	int statusBarHeight, zoomLimit = 200;
	static int zoom = 0;

	if (isZoomLimitReached != NULL) (*isZoomLimitReached) = 0;

	if (setZoom != NULL)
	{
		if (abs((*setZoom)) < zoomLimit)
		{
			zoom = (*setZoom);
		}
		else
		{
			zoom = zoomLimit;
			if (isZoomLimitReached != NULL) (*isZoomLimitReached) = 1;
		}
	}
	else
	{
		if (abs(zoom + zoomInc) < zoomLimit)
			zoom += zoomInc;
		else
			if (isZoomLimitReached != NULL) (*isZoomLimitReached) = 1;
	}

	gv_currentZoom = zoom;
	GetPictureRectangle(&rectanglePicture, NULL, &statusBarHeight);

	int clientWidth = rectanglePicture.right - rectanglePicture.left;
	int clientHeight = rectanglePicture.bottom - rectanglePicture.top;
	int clientWidthZoomed = clientWidth + zoom;
	int clientHeightZoomed = clientHeight + zoom;

	RECT rectangleMemoryDC;
	GetMemoryDCRectangle(&rectangleMemoryDC);
	FillRect(hMemoryDCZoom, &rectangleMemoryDC, (HBRUSH)GetStockObject(GRAY_BRUSH));

	StretchBlt(hMemoryDCZoom, 0, 0, clientWidth, clientHeight, hMemoryDCDraw, xScrollPosition, yScrollPosition, clientWidthZoomed, clientHeightZoomed, SRCCOPY);
	//BitBlt(hDC, 0, rectanglePicture.top, clientWidth, clientHeight, hMemoryDCZoom, 0, 0, SRCCOPY);
	StretchBlt(hDC, 0, rectanglePicture.top, clientWidth, clientHeight, hMemoryDCZoom, 0, 0, clientWidth, clientHeight, SRCCOPY);
}

void RedrawAllPaintedPicture(HDC hDC, std::vector<DrawPictureData> drawPictureDataArray)
{
	std::vector<DrawPictureData>::iterator it;
	HPEN pen;
	HBRUSH brush;

	for (it = drawPictureDataArray.begin(); it != drawPictureDataArray.end(); it++)
	{
		pen = CreatePen(PS_SOLID, it->penWidth, it->colorPen);
		SelectObject(hDC, pen);

		if (it->isTransparentBrush)
		{
			brush = (HBRUSH)GetStockObject(NULL_BRUSH);
		}
		else
		{
			brush = CreateSolidBrush(it->colorBrush);
		}
		SelectObject(hDC, brush);

		switch (it->figureID)
		{
		case ID_PENCIL:
		{
			MoveToEx(hDC, it->x1, it->y1, NULL);
			std::vector<Point> pointData = it->arrayCoord;
			std::vector<Point>::iterator itPoint;
			for (itPoint = pointData.begin(); itPoint != pointData.end(); itPoint++)
				LineTo(hDC, itPoint->x, itPoint->y);
			break;
		}
		case ID_LINE:
		{
			MoveToEx(hDC, it->x1, it->y1, NULL);
			LineTo(hDC, it->x2, it->y2);
			break;
		}
		case ID_POLYGON:
		{
			MoveToEx(hDC, it->x1, it->y1, NULL);
			LineTo(hDC, it->x2, it->y2);
			break;
		}
		case ID_RECTANGLE:
		{
			Rectangle(hDC, it->x1, it->y1, it->x2, it->y2);
			break;
		}
		case ID_ELLIPSE:
		{
			Ellipse(hDC, it->x1, it->y1, it->x2, it->y2);
			break;
		}
		case ID_TEXT:
		{
			SelectObject(hDC, CreateFontIndirect(&(it->logicalFont)));
			SetTextColor(hDC, it->colorText);

			int oldMode;
			if (it->isTransparentBrush)
			{
				oldMode = SetBkMode(hDC, TRANSPARENT);
			}
			else
			{
				SetBkColor(hDC, it->colorBrush);
			}
			TextOut(hDC, it->x1, it->y1, it->textBuffer, lstrlen(it->textBuffer));
			if (it->isTransparentBrush)
			{
				SetBkMode(hDC, oldMode);
			}

			break;
		}
		default:
			break;
		}

		DeleteObject(pen);
		if (!it->isTransparentBrush)
			DeleteObject(brush);
	}
}

void RedrawAllPicture(HDC hDC, std::vector<DrawPictureData> drawPictureDataArray, 
	HBRUSH fillBackgroundBursh)
{
	RECT rectanglePicture;
	GetMemoryDCRectangle(&rectanglePicture);
	FillRect(hDC, &rectanglePicture, fillBackgroundBursh);

	if (gv_isFileOpen)
	{
		OpenEnhancedMetafile(hDC, gv_openFileNameBuffer);
	}

	if (!drawPictureDataArray.empty())
	{
		RedrawAllPaintedPicture(hDC, drawPictureDataArray);
	}
}

void OpenEnhancedMetafile(HDC hDC, const PTCHAR fileName)
{
	HENHMETAFILE hMetafile = GetEnhMetaFile(fileName);
	if (!hMetafile)
	{
		MessageBox(NULL, _TEXT("File not found"), _TEXT("Message"), MB_OK | MB_ICONWARNING);
	}
	else
	{
		RECT rectanglePicture, rect;
		int pictureRectangleWidth, pictureRectangleHeight, toolBarHeight, statusBarHeight;

		GetPictureRectangle(&rectanglePicture, &toolBarHeight, &statusBarHeight);
		pictureRectangleWidth = rectanglePicture.right - rectanglePicture.left;
		pictureRectangleHeight = rectanglePicture.bottom - rectanglePicture.top;

		ENHMETAHEADER emh;
		int x1, y1, x2, y2;

		GetEnhMetaFileHeader(hMetafile, sizeof(ENHMETAHEADER), &emh);
		x1 = emh.rclBounds.left; y1 = emh.rclBounds.top + toolBarHeight;
		x2 = emh.rclBounds.right; y2 = emh.rclBounds.bottom - statusBarHeight;
		SetRect(&rect, x1, y1, x2, y2);

		// You can also transfer a "rectanglePicture" to this function, 
		// but then when you click on the "full screen" button, the picture will be scaled.
		PlayEnhMetaFile(hDC, hMetafile, &rect);
		DeleteEnhMetaFile(hMetafile);
	}
}

void SaveEnhancedMetafile(const PTCHAR fileName)
{
	RECT rectanglePicture;
	int pictureRectangleWidth, pictureRectangleHeight, toolBarHeight, statusBarHeight;

	GetPictureRectangle(&rectanglePicture, &toolBarHeight, &statusBarHeight);
	pictureRectangleWidth = rectanglePicture.right - rectanglePicture.left;
	pictureRectangleHeight = rectanglePicture.bottom - rectanglePicture.top;

	HDC hdcEnhancedMetafile;
	HENHMETAFILE hMetafile;

	HDC hDC = GetDC(gv_hWndMainWindow);

	hdcEnhancedMetafile = CreateEnhMetaFile(NULL, fileName, NULL, _TEXT("PaintAB\0"));

	BitBlt(hdcEnhancedMetafile, 0, 0, pictureRectangleWidth, pictureRectangleHeight, hDC, rectanglePicture.left, rectanglePicture.top, SRCCOPY);

	hMetafile = CloseEnhMetaFile(hdcEnhancedMetafile);
	DeleteEnhMetaFile(hMetafile);

	ReleaseDC(gv_hWndMainWindow, hDC);
}

void SetScrollBarsOptions(HWND hWnd, int xScrollPosition, int yScrollPosition)
{
	RECT rectangleMemoryDC, rectangleClient;
	int toolBarHeight, statusBarHeight;
	GetMemoryDCRectangle(&rectangleMemoryDC);
	GetPictureRectangle(&rectangleClient, &toolBarHeight, &statusBarHeight);

	int widthVerticalScrollbar = 0;
	int heightHorizontalScrollbar = 0;
	if (IsZoomed(hWnd))
	{
		heightHorizontalScrollbar = GetSystemMetrics(SM_CXHSCROLL);
		widthVerticalScrollbar = GetSystemMetrics(SM_CXVSCROLL);
	}

	// Setting options for the vertical scroll bar.
	SCROLLINFO scrollInfo = { 0 };
	scrollInfo.cbSize = sizeof(SCROLLINFO);
	scrollInfo.fMask = SIF_RANGE | SIF_PAGE;
	scrollInfo.nMin = 0;
	scrollInfo.nMax = rectangleMemoryDC.bottom + toolBarHeight + statusBarHeight + 7;
	scrollInfo.nPage = rectangleClient.bottom + toolBarHeight - widthVerticalScrollbar;
	if (IsZoomed(hWnd))
		scrollInfo.nPage = scrollInfo.nMax + 1;
	scrollInfo.nPos = yScrollPosition;
	SetScrollInfo(hWnd, SB_VERT, &scrollInfo, TRUE);

	// Setting options for the horizontal scroll bar.
	scrollInfo.cbSize = sizeof(SCROLLINFO);
	scrollInfo.fMask = SIF_RANGE | SIF_PAGE;
	scrollInfo.nMin = 0;
	scrollInfo.nMax = rectangleMemoryDC.right;
	scrollInfo.nPage = rectangleClient.right - heightHorizontalScrollbar;
	if (IsZoomed(hWnd))
		scrollInfo.nPage = scrollInfo.nMax + 1;
	scrollInfo.nPos = xScrollPosition;
	SetScrollInfo(hWnd, SB_HORZ, &scrollInfo, TRUE);
}

int GetNumberFromWindow(HWND hWnd)
{
#define MAX_DIGITS 5
	TCHAR numberStr[MAX_DIGITS] = { 0 };
	SendMessage(hWnd, WM_GETTEXT, sizeof(numberStr) / sizeof(TCHAR), (LPARAM)&numberStr);
	return _ttoi(numberStr);
}

int GetMouseWheelInterval(int interval, int step)
{
	return ((-3 * (short)interval / WHEEL_DELTA) * step);
}

int IsKeyPressed(UINT VirtualKey)
{
	return GetKeyState(VirtualKey) < 0 ? TRUE : FALSE;
}
