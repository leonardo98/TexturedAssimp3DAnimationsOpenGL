// ----------------------------------------------------------------------------
// Another Assimp OpenGL sample including texturing.
// Note that it is very basic and will only read and apply the model's diffuse
// textures (by their material ids)
//
// Don't worry about the "Couldn't load Image: ...dwarf2.jpg" Message.
// It's caused by a bad texture reference in the model file (I guess)
//
// If you intend to _use_ this code sample in your app, do yourself a favour
// and replace immediate mode calls with VBOs ...
//
// Thanks to NeHe on whose OpenGL tutorials this one's based on! :)
// http://nehe.gamedev.net/
// ----------------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <IL/il.h>

#include <fstream>

//to map image filenames to textureIds
#include <string.h>
#include <map>


#include "AnimationController.h"

//AnimationController controller("C:/Users/lenya_en/Downloads/ogldev-source/Content/boblampclean.md5mesh");
//AnimationController controller("C:/Dropbox/GAME/Resources/characters/main-hero/mainhero@walk01.fbx");
AnimationController controller("C:/Dropbox/Projects/Moon/pers/walking.dae");

// The default hard-coded path. Can be overridden by supplying a path through the command line.

//static std::string modelpath = "../../test/models/OBJ/spider.obj";
//static std::string modelpath = "C:/Dropbox/GAME/Resources/characters/main-hero/mainhero@run01.fbx";
//static std::string modelpath = "C:/Users/lenya_en/Downloads/ogldev-source/Content/boblampclean.md5mesh";


HGLRC		hRC=NULL;			// Permanent Rendering Context
HDC			hDC=NULL;			// Private GDI Device Context
HWND		hWnd=NULL;			// Holds Window Handle
HINSTANCE	hInstance;	// Holds The Instance Of The Application

bool		keys[256];			// Array used for Keyboard Routine;
bool		active=TRUE;		// Window Active Flag Set To TRUE by Default
bool		fullscreen=TRUE;	// full-screen Flag Set To full-screen By Default


LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	// Declaration For WndProc

const char* windowTitle = "OpenGL Framework";

GLfloat LightAmbient[]= { 0.5f, 0.5f, 0.5f, 1.0f };
GLfloat LightDiffuse[]= { 1.0f, 1.0f, 1.0f, 1.0f };
GLfloat LightPosition[]= { 0.0f, 0.0f, 15.0f, 1.0f };



// the global Assimp scene object
GLuint scene_list = 0;
aiVector3D scene_min, scene_max, scene_center;

// Resize And Initialize The GL Window
void ReSizeGLScene(GLsizei width, GLsizei height)				
{
    // Prevent A Divide By Zero By
	if (height==0)								
	{
        // Making Height Equal One
        height=1;		
	}

	glViewport(0, 0, width, height);					// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();							// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f,(GLfloat)width/(GLfloat)height,0.1f,100.0f);

	glMatrixMode(GL_MODELVIEW);						// Select The Modelview Matrix
	glLoadIdentity();							// Reset The Modelview Matrix
}


// All Setup For OpenGL goes here
int InitGL()
{
	if (!controller.LoadGLTextures())
	{
		return FALSE;
	}
	if (!controller.Add3DAnimFromFile("C:/Dropbox/Projects/Moon/pers/idle.dae")) return 0;


	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);		 // Enables Smooth Shading
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	glClearDepth(1.0f);				// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);		// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);			// The Type Of Depth Test To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculation


	{
		// направленный источник света
		glEnable(GL_LIGHTING);
		GLfloat light0_diffuse[] = {0.7, 0.7, 0.7};
		GLfloat light0_direction[] = {0.0, 1.0, 1.0, 0.0};
		glEnable(GL_LIGHT0);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
		glLightfv(GL_LIGHT0, GL_POSITION, light0_direction);
	}
	//glEnable(GL_LIGHTING);
	//glEnable(GL_LIGHT0);    // Uses default lighting parameters
	//glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	//glEnable(GL_NORMALIZE);

	//glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);
	//glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);
	//glLightfv(GL_LIGHT1, GL_POSITION, LightPosition);
	//glEnable(GL_LIGHT1);

	return TRUE;					// Initialization Went OK
}

void KillGLWindow()			// Properly Kill The Window
{
	if (fullscreen)					// Are We In Fullscreen Mode?
	{
		ChangeDisplaySettings(NULL, 0);		// If So Switch Back To The Desktop
		ShowCursor(TRUE);					// Show Mouse Pointer
	}

	if (hRC)					// Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(NULL, NULL))	// Are We Able To Release The DC And RC Contexts?
		{
			MessageBoxA(NULL, "Release Of DC And RC Failed.", "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))			// Are We Able To Delete The RC?
		{
			MessageBoxA(NULL, "Release Rendering Context Failed.", "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);
		}
		hRC = NULL;
	}

	if (hDC && !ReleaseDC(hWnd, hDC))	// Are We able to Release The DC?
	{
		MessageBoxA(NULL, "Release Device Context Failed.", "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);
		hDC=NULL;
	}

	if (hWnd && !DestroyWindow(hWnd))	// Are We Able To Destroy The Window
	{
		MessageBoxA(NULL, "Could Not Release hWnd.", "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);
		hWnd = NULL;
	}

	if (!UnregisterClassA("OpenGL", hInstance))	// Are We Able To Unregister Class
	{
		MessageBoxA(NULL, "Could Not Unregister Class.", "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);
		hInstance = NULL;
	}
}

BOOL CreateGLWindow(const char* title, int width, int height, int bits, bool fullscreenflag)
{
	GLuint		PixelFormat;		// Hold the result after searching for a match
	WNDCLASS	wc;					// Window Class Structure
	DWORD		dwExStyle;			// Window Extended Style
	DWORD		dwStyle;			// Window Style
	RECT		WindowRect;			// Grabs Rectangle Upper Left / Lower Right Values
	WindowRect.left		= (long)0;
	WindowRect.right	= (long)width;
	WindowRect.top		= (long)0;
	WindowRect.bottom	= (long)height;

	fullscreen = fullscreenflag;

	hInstance = GetModuleHandle(NULL);	// Grab An Instance For Our Window
	wc.style		= CS_HREDRAW | CS_VREDRAW | CS_OWNDC; // Redraw On Move, And Own DC For Window
	wc.lpfnWndProc	= (WNDPROC) WndProc;		// WndProc handles Messages
	wc.cbClsExtra	= 0;	// No Extra Window Data
	wc.cbWndExtra	= 0;	// No Extra Window Data
	wc.hInstance	= hInstance;
	wc.hIcon		= LoadIcon(NULL, IDI_WINLOGO);	// Load The Default Icon
	wc.hCursor		= LoadCursor(NULL, IDC_ARROW);	// Load the default arrow
	wc.hbrBackground= NULL;							// No Background required for OpenGL
	wc.lpszMenuName	= NULL;							// No Menu
	wc.lpszClassName= "OpenGL";						// Class Name

	if (!RegisterClass(&wc))
	{
		MessageBoxA(NULL, "Failed to register the window class", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;		//exit and return false
	}

	if (fullscreen)		// attempt fullscreen mode
	{
		DEVMODE dmScreenSettings;								// Device Mode
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));	// Make Sure Memory's Cleared
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);		// Size Of the devmode structure
		dmScreenSettings.dmPelsWidth	= width;				// Selected Screen Width
		dmScreenSettings.dmPelsHeight	= height;				// Selected Screen Height
		dmScreenSettings.dmBitsPerPel	= bits;					// bits per pixel
		dmScreenSettings.dmFields		= DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

		// Try To Set Selected Mode and Get Results. NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
		if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
		{
			// If The Mode Fails, Offer Two Options.  Quit Or Run In A Window.
			if (MessageBoxA(NULL,"The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Use Windowed Mode Instead?","NeHe GL",MB_YESNO|MB_ICONEXCLAMATION)==IDYES)
			{
				fullscreen = FALSE;		// Select Windowed Mode (Fullscreen = FALSE)
			}
			else
			{
				//Popup Messagebox: Closing
				MessageBoxA(NULL, "Program will close now.", "ERROR", MB_OK|MB_ICONSTOP);
				return FALSE; //exit, return false
			}
		}
	}

	if (fullscreen)		// when mode really succeeded
	{
		dwExStyle=WS_EX_APPWINDOW;		// Window Extended Style
		dwStyle=WS_POPUP;
		ShowCursor(FALSE);
	}
	else
	{
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;	// Window extended style
		dwStyle=WS_OVERLAPPEDWINDOW;					// Windows style
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);		// Adjust Window To True Requestes Size

	if (!(hWnd=CreateWindowExA(	dwExStyle,						// Extended Style For The Window
								"OpenGL",						// Class Name
								title,							// Window Title
								WS_CLIPSIBLINGS |				// Required Window Style
								WS_CLIPCHILDREN |				// Required Window Style
								dwStyle,						// Selected WIndow Style
								0, 0,							// Window Position
								WindowRect.right-WindowRect.left, // Calc adjusted Window Width
								WindowRect.bottom-WindowRect.top, // Calc adjustes Window Height
								NULL,							// No Parent Window
								NULL,							// No Menu
								hInstance,						// Instance
								NULL )))						// Don't pass anything To WM_CREATE
	{
		abortGLInit("Window Creation Error.");
		return FALSE;
	}

	static	PIXELFORMATDESCRIPTOR pfd=					// pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),					// Size Of This Pixel Format Descriptor
		1,												// Version Number
		PFD_DRAW_TO_WINDOW |							// Format Must Support Window
		PFD_SUPPORT_OPENGL |							// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,								// Must Support Double Buffering
		PFD_TYPE_RGBA,									// Request An RGBA Format
		bits,											// Select Our Color Depth
		0, 0, 0, 0, 0, 0,								// Color Bits Ignored
		0,												// No Alpha Buffer
		0,												// Shift Bit Ignored
		0,												// No Accumulation Buffer
		0, 0, 0, 0,										// Accumulation Bits Ignored
		16,												// 16Bit Z-Buffer (Depth Buffer)
		0,												// No Stencil Buffer
		0,												// No Auxiliary Buffer
		PFD_MAIN_PLANE,									// Main Drawing Layer
		0,												// Reserved
		0, 0, 0											// Layer Masks Ignored
	};

	if (!(hDC=GetDC(hWnd)))								// Did we get the Device Context?
	{
		abortGLInit("Can't Create A GL Device Context.");
		return FALSE;
	}

	if (!(PixelFormat=ChoosePixelFormat(hDC, &pfd)))	// Did We Find a matching pixel Format?
	{
		abortGLInit("Can't Find Suitable PixelFormat");
		return FALSE;
	}

	if (!SetPixelFormat(hDC, PixelFormat, &pfd))
	{
		abortGLInit("Can't Set The PixelFormat");
		return FALSE;
	}

	if (!(hRC=wglCreateContext(hDC)))
	{
		abortGLInit("Can't Create A GL Rendering Context.");
		return FALSE;
	}

	if (!(wglMakeCurrent(hDC,hRC)))						// Try to activate the rendering context
	{
		abortGLInit("Can't Activate The Rendering Context");
		return FALSE;
	}

	//// *** everything okay ***

	ShowWindow(hWnd, SW_SHOW);		// Show The Window
	SetForegroundWindow(hWnd);		// Slightly Higher Prio
	SetFocus(hWnd);					// Sets Keyboard Focus To The Window
	ReSizeGLScene(width, height);	// Set Up Our Perspective GL Screen

	if (!InitGL())
	{
		abortGLInit("Initialization failed");
		return FALSE;
	}

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd,				// Handles for this Window
						 UINT uMsg,				// Message for this Window
						 WPARAM wParam,			// additional message Info
						 LPARAM lParam)			// additional message Info
{
	switch (uMsg)				// check for Window Messages
	{
		case WM_ACTIVATE:				// Watch For Window Activate Message
			{
				if (!HIWORD(wParam))	// Check Minimization State
				{
					active=TRUE;
				}
				else
				{
					active=FALSE;
				}

				return 0;				// return To The Message Loop
			}

		case WM_SYSCOMMAND:			// Interrupt System Commands
			{
				switch (wParam)
				{
					case SC_SCREENSAVE:		// Screen-saver trying to start
					case SC_MONITORPOWER:	// Monitor trying to enter power-safe
					return 0;
				}
				break;
			}

		case WM_CLOSE:			// close message received?
			{
				PostQuitMessage(0);	// Send WM_QUIT quit message
				return 0;			// Jump Back
			}

		case WM_KEYDOWN:		// Is a key pressed?
			{
				keys[wParam] = TRUE;	// If so, Mark it as true
				if (wParam == VK_NUMPAD1)
				{
					controller.SetAnimIndex(0);
				}
				else if (wParam == VK_NUMPAD2)
				{
					controller.SetAnimIndex(1);
				}
				return 0;
			}

		case WM_KEYUP:			// Has Key Been released?
			{
				keys[wParam] = FALSE;	// If so, Mark It As FALSE
				return 0;
			}

		case WM_SIZE:			// Resize The OpenGL Window
			{
				ReSizeGLScene(LOWORD(lParam), HIWORD(lParam));	// LoWord-Width, HiWord-Height
				return 0;
			}

	}

	// Pass All unhandled Messaged To DefWindowProc
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int WINAPI WinMain( HINSTANCE hInstance,         // The instance
				   HINSTANCE hPrevInstance,      // Previous instance
				   LPSTR lpCmdLine,              // Command Line Parameters
				   int nShowCmd )                // Window Show State
{
	MSG msg;
	BOOL done=FALSE;

	createAILogger();
	logInfo("App fired!");

	// Check the command line for an override file path. 
	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (argv != NULL && argc > 1)
	{
		std::wstring modelpathW(argv[1]);
		if (!controller.Import3DFromFile(std::string(modelpathW.begin(), modelpathW.end()))) return 0;
	}
	else
	{
		if (!controller.Import3DFromFile()) return 0;
	}

	logInfo("=============== Post Import ====================");

	//if (MessageBoxA(NULL, "Would You Like To Run In Fullscreen Mode?", "Start Fullscreen?", MB_YESNO|MB_ICONEXCLAMATION)==IDNO)
	{
		fullscreen=FALSE;
	}

	if (!CreateGLWindow(windowTitle, 640, 480, 16, fullscreen))
	{
		return 0;
	}

	while(!done)	// Game Loop
	{
		if (PeekMessage(&msg, NULL, 0,0, PM_REMOVE))
		{
			if (msg.message==WM_QUIT)
			{
				done=TRUE;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			// Draw The Scene. Watch For ESC Key And Quit Messaged From DrawGLScene()
			if (active)
			{
				if (keys[VK_ESCAPE])
				{
					done=TRUE;
				}
				else
				{
					controller.Update();
					controller.DrawGLScene();
					SwapBuffers(hDC);
				}
				if (keys[VK_LEFT])
				{
					controller.SetRotation(aiVector3D(controller.GetRotation().x
						, controller.GetRotation().y - 10.f
						, controller.GetRotation().z));
				}
				if (keys[VK_RIGHT])
				{
					controller.SetRotation(aiVector3D(controller.GetRotation().x
						, controller.GetRotation().y + 10.f
						, controller.GetRotation().z));
				}
			}

			if (keys[VK_F1])
			{
				keys[VK_F1]=FALSE;
				KillGLWindow();
				fullscreen=!fullscreen;
				if (!CreateGLWindow(windowTitle, 640, 480, 16, fullscreen))
				{
					return 0;
				}
			}
		}
	}

	// *** cleanup end ***
	controller.Release();
	destroyAILogger();
	KillGLWindow();
	return (msg.wParam);
}
