
// Includes
#define NOMINMAX
#include <windows.h>
#include "glew.h"
#include "wglew.h"
#include "log.h"
#include "winfont.h"
#include "rawinput.h"
#include "fileio.h"
#include "ini.h"
#include <vector>
#include "colordefs.h"
#include "mmtimer.h"
#include "mixer.h"

double millsec = (double)1000 / (double)60;
double gametime = 0;// = TimerGetTimeMS();
double starttime = 0;
//Library Includes
#pragma comment(lib, "opengl32.lib")

//Globals
HWND hWnd;
int SCREEN_W = 800;
int SCREEN_H = 600;

// Function Declarations
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void EnableOpenGL(HWND hWnd, HDC * hDC, HGLRC * hRC);
void DisableOpenGL(HWND hWnd, HDC hDC, HGLRC hRC);

static void throttle_speed(void)
{
	double millsec = millsec = (double)1000 / (double)60;
	gametime = TimerGetTimeMS();

	
		while (((double)(gametime)-(double)starttime) < (double)millsec)
		{
			HANDLE current_thread = GetCurrentThread();
			int old_priority = GetThreadPriority(current_thread);

			if (((double)gametime - (double)starttime) < (double)(millsec - 4))
			{
				SetThreadPriority(current_thread, THREAD_PRIORITY_TIME_CRITICAL);
				//Sleep(1);
				SetThreadPriority(current_thread, old_priority);
			}
			//else Sleep(0);
			gametime = TimerGetTimeMS();
		}
	
	starttime = TimerGetTimeMS();
}

void ViewOrtho(int width, int height)
{
	glViewport(0, 0, width, height);             // Set Up An Ortho View	 
	glMatrixMode(GL_PROJECTION);			  // Select Projection
	glLoadIdentity();						  // Reset The Matrix
	glOrtho(0, width, 0, height, -1, 1);	  // Select Ortho 2D Mode DirectX style(640x480)
	glMatrixMode(GL_MODELVIEW);				  // Select Modelview Matrix
	glLoadIdentity();						  // Reset The Matrix
}

//========================================================================
// Return the Window Handle
//========================================================================
HWND win_get_window()
{
	return hWnd;
}

int KeyCheck(int keynum)
{
	static int keys[256];

	static int hasrun = 0;
	int i;

	if (hasrun == 0)
	{
		for (i = 0; i < 256; i++)
		{
			keys[i] = 0;
		}
		hasrun = 1;
	}
	if (!keys[keynum] && key[keynum]) //Return True if not in que
	{
		keys[keynum] = 1;
		return 1;
	}
	else if (keys[keynum] && !key[keynum]) //Return False if in que
		keys[keynum] = 0;
	return 0;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_  LPSTR lpCmdLine, _In_ int iCmdShow)
{
	WNDCLASS wc;
	HDC hDC;
	HGLRC hRC;
	MSG msg;
	BOOL quit = FALSE;

	// register window class
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "GLSample";
	RegisterClass(&wc);

	// create main window
	hWnd = CreateWindow("GLSample", "OpenGL Sample", WS_CAPTION | WS_POPUPWINDOW | WS_VISIBLE, 0, 0, SCREEN_W, SCREEN_H, NULL, NULL, hInstance, NULL);

	///////////////// Initialize everything here //////////////////////////////
	LogOpen("testlog.txt");
	wrlog("Opening Log");

	// enable OpenGL for the window
	EnableOpenGL(hWnd, &hDC, &hRC);

	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		/* Problem: glewInit failed, something is seriously wrong. */
		wrlog("Error: %s\n", glewGetErrorString(err));

	}
	wrlog("Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

	wglSwapIntervalEXT(1);
	TimerInit();
	Font_Init(20);
	HRESULT i = RawInput_Initialize(hWnd);
	ViewOrtho(SCREEN_W, SCREEN_H);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	mixer_init(44100,60);
	
	load_sample(0, "data\\musicstereo.ogg");
	load_sample(0, "data\\InGame1Loop.ogg");
	load_sample(0, "data\\sfx_zap.wav");
	load_sample(0, "data\\fire.wav");
	load_sample(0, "data\\ssaucer.wav");
	load_sample(0, "data\\ZK13.wav");
	load_sample(0, "data\\Space_Alert1.wav");
	load_sample(0, "data\\strings2mono.wav");

	int channel_status[8] = { 0 };

	/////////////////// END INITIALIZATION ////////////////////////////////////
	
	static int b = 100;

	// program main loop
	while (!quit)
	{
		// check for messages
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{

			// handle or dispatch messages
			if (msg.message == WM_QUIT)
			{
				quit = TRUE;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

		}
		else
		{
			// OpenGL animation code goes here

			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			glColor3f(1.0f, 1.0f, 1.0f);
			StartTextMode();
			Font_Print(10, SCREEN_H - 150, "Sample 0: playing %d", sample_playing(0));
			Font_Print(10, SCREEN_H - 200, "Sample 1: playing %d", sample_playing(1));
			Font_Print(10, SCREEN_H - 250, "Sample 2: playing %d", sample_playing(2));
			Font_Print(10, SCREEN_H - 300, "Sample 3: playing %d", sample_playing(3));
			Font_Print(10, SCREEN_H - 350, "Sample 4: playing %d", sample_playing(4));
			Font_Print(10, SCREEN_H - 400, "Sample 5: playing %d", sample_playing(5));
			Font_Print(10, SCREEN_H - 450, "Sample 6: playing %d", sample_playing(6));
			Font_Print(10, SCREEN_H - 500, "Sample 7: playing %d", sample_playing(7));

			Font_Print(10, SCREEN_H - 550, "Sample 4 VOL: playing %i",b);
			

			if (KeyCheck(KEY_UP)) {
				b++;  if (b > 200) b = 200; 
			}
			if (KeyCheck(KEY_DOWN)) {
				b--;   if (b < 0) b = 0; 
			}

			if (KeyCheck(KEY_0)) { sample_start(0, 0, 0); }
			if (KeyCheck(KEY_1)) { sample_start(1, 1, 0); }
			if (KeyCheck(KEY_2)) { sample_start(2, 2, 0); }
			if (KeyCheck(KEY_3)) { sample_start(3, 3, 0); }
			if (KeyCheck(KEY_4)) { sample_start(4, 4, 0); }
			if (KeyCheck(KEY_5)) { sample_start(5, 5, 0); }
			if (KeyCheck(KEY_6)) { sample_start(6, 6, 0); }
			if (KeyCheck(KEY_7)) { sample_start(7, 7, 0); }
		//	if (KeyCheck(KEY_8)) { sample_start(8, "ZK13", 0); }

			if (KeyCheck(KEY_9))
			{
				sample_start(0, 0, 0);
				sample_start(1, 1, 0);
				sample_start(2, 2, 0);
				sample_start(3, 3, 0);
				sample_start(4, 4, 0);
			}

			if (KeyCheck(KEY_ESC)) { quit = TRUE; }

			EndTextMode();
			mixer_update();
			throttle_speed();
			SwapBuffers(hDC);
		}
	}

	// shutdown OpenGL
	DisableOpenGL(hWnd, hDC, hRC);
		
	wrlog("Closing Log");
	KillFont();
	//stream_stop(11,0);
	mixer_end();
	LogClose();
	// destroy the window explicitly
	DestroyWindow(hWnd);

	return (int) msg.wParam;
}

// Window Procedure

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	switch (message)
	{

	case WM_CREATE:
		return 0;

	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;

	case WM_INPUT: {return RawInput_ProcessInput(hWnd, wParam, lParam); return 0; }

	case WM_DESTROY:
		return 0;


	case WM_SYSCOMMAND:
	{
		switch (wParam & 0xfff0)
		{
		case SC_SCREENSAVE:
		case SC_MONITORPOWER:
		{
			return 0;
		}
		/*
		case SC_CLOSE:
		{
			//I can add a close hook here to trap close button
			quit = 1;
			PostQuitMessage(0);
			break;
		}
		*/
		// User trying to access application menu using ALT?
		case SC_KEYMENU:
			return 0;
		}
		DefWindowProc(hWnd, message, wParam, lParam);
	}
	case WM_KEYDOWN:
		switch (wParam)
		{

		case VK_ESCAPE:
			PostQuitMessage(0);
			return 0;

		}
		return 0;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

// Enable OpenGL

void EnableOpenGL(HWND hWnd, HDC * hDC, HGLRC * hRC)
{
	PIXELFORMATDESCRIPTOR pfd;
	int format;

	// get the device context (DC)
	*hDC = GetDC(hWnd);

	// set the pixel format for the DC
	ZeroMemory(&pfd, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 16;
	pfd.iLayerType = PFD_MAIN_PLANE;
	format = ChoosePixelFormat(*hDC, &pfd);
	SetPixelFormat(*hDC, format, &pfd);

	// create and enable the render context (RC)
	*hRC = wglCreateContext(*hDC);
	wglMakeCurrent(*hDC, *hRC);

}

// Disable OpenGL

void DisableOpenGL(HWND hWnd, HDC hDC, HGLRC hRC)
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(hRC);
	ReleaseDC(hWnd, hDC);
}
