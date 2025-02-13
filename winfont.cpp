#include "winfont.h"

GLuint textBase = 0;
GLuint startTextModeList = 0;

#pragma warning ( disable:4996 )

int GetCharFontWidth(const char cCharacter)
{
	HWND hWnd = win_get_window();
	HDC hDC = GetDC(hWnd);
	SIZE kSize;
	GetTextExtentPoint32(hDC, &cCharacter, 1, &kSize);
	ReleaseDC(hWnd, hDC);
	return (int)kSize.cx;
}

int Font_Init(int sizept)
{
	TEXTMETRIC kMetric;
	HWND hwnd;
	HFONT font;
	HFONT oldfont;
	HDC    hdc;
	long lfHeight;
	hwnd = win_get_window();
	hdc = GetDC(hwnd);

	//Create 96 display lists
	textBase = glGenLists(96);
	if (textBase == 0)
	{
		wrlog("Unable to create 96 display lists for font");
		return 0;
	}

	lfHeight = -MulDiv(sizept, GetDeviceCaps(hdc, LOGPIXELSY), 72);

	//Create font
	font = CreateFont(lfHeight,			//height -18
		0,				//default width,
		0, 0,			//angles
		FW_BOLD,		//bold
		0,			//italic
		0,			//underline
		0,			//strikeout
		ANSI_CHARSET,	//character set
		OUT_TT_PRECIS,	//precision
		CLIP_DEFAULT_PRECIS,
		ANTIALIASED_QUALITY,	//quality
		FF_DONTCARE | DEFAULT_PITCH,
		"Arial");

	//Select font
	oldfont = (HFONT)SelectObject(hdc, font);

	//Fill in the 96 display lists, starting with character 32
	wglUseFontBitmaps(hdc, 32, 96, textBase);
	GetTextMetrics(hdc, &kMetric);

	SelectObject(hdc, oldfont);				// Selects The Previous Font
	DeleteObject(font);                     //Cleanup
	wrlog("Font created successfully");

	return 1;
}

//Start text mode
void StartTextMode(void)
{
	//Create a display list if not already done
	if (!startTextModeList)
	{
		startTextModeList = glGenLists(1);
		glNewList(startTextModeList, GL_COMPILE);
		{
			glListBase(textBase - 32);

			ViewOrtho(SCREEN_W, SCREEN_H);

			//Set states
			glDisable(GL_DEPTH_TEST);
		}
		glEndList();
	}

	//Call the list
	//	glPushAttrib(GL_LIST_BIT);				// Pushes The Display List Bits
	glCallList(startTextModeList);
	//	glPopAttrib();
}

//Print some text
void Font_Print(int x, int y, const char* string, ...)
{
	//Convert to text
	static char text[256];

	va_list va;

	if (string == NULL)
		return;

	va_start(va, string);
	vsprintf(text, string, va);
	va_end(va);

	//Print the text
	glRasterPos2i(x, y);
	glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
}

//End text mode
void EndTextMode(void)
{
	//restore matrices
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	//reset other states
	glListBase(0);
	//glEnable(GL_DEPTH_TEST);
}

void KillFont()						// Delete The Font List
{
	glDeleteLists(textBase, 96);				// Delete All 96 Characters ( NEW )
}