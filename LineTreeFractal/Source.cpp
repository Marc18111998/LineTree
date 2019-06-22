#include <windows.h>
#include <string>
#include <math.h>
#include <chrono>
#include <thread>

using namespace std;

const float PI = 3.1415926536f;
const float FIRST_LINE_LENGTH = 75.0f;

class Bitmap
{
	public:
		Bitmap() : pen(NULL) {}
		~Bitmap()
		{
			DeleteObject(pen);
			DeleteDC(hdc);
			DeleteObject(bmp);
		}

		bool create(int w, int h)
		{
			BITMAPINFO	bi;
			void		*pBits;
			ZeroMemory(&bi, sizeof(bi));
			bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
			bi.bmiHeader.biBitCount = sizeof(DWORD) * 8;
			bi.bmiHeader.biCompression = BI_RGB;
			bi.bmiHeader.biPlanes = 1;
			bi.bmiHeader.biWidth = w;
			bi.bmiHeader.biHeight = -h;

			HDC dc = GetDC(GetConsoleWindow());
			bmp = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &pBits, NULL, 0);
			if (!bmp) return false;

			hdc = CreateCompatibleDC(dc);
			SelectObject(hdc, bmp);
			ReleaseDC(GetConsoleWindow(), dc);

			width = w; height = h;

			return true;
		}

		void setPenColor(DWORD clr)
		{
			if (pen) DeleteObject(pen);
			pen = CreatePen(PS_SOLID, 1, clr);
			SelectObject(hdc, pen);
		}

		void saveBitmap(string path)
		{
			BITMAPFILEHEADER	fileheader;
			BITMAPINFO			infoheader;
			BITMAP				bitmap;
			DWORD*				dwpBits;
			DWORD				wb;
			HANDLE				file;

			GetObject(bmp, sizeof(bitmap), &bitmap);

			dwpBits = new DWORD[bitmap.bmWidth * bitmap.bmHeight];
			ZeroMemory(dwpBits, bitmap.bmWidth * bitmap.bmHeight * sizeof(DWORD));
			ZeroMemory(&infoheader, sizeof(BITMAPINFO));
			ZeroMemory(&fileheader, sizeof(BITMAPFILEHEADER));

			infoheader.bmiHeader.biBitCount = sizeof(DWORD) * 8;
			infoheader.bmiHeader.biCompression = BI_RGB;
			infoheader.bmiHeader.biPlanes = 1;
			infoheader.bmiHeader.biSize = sizeof(infoheader.bmiHeader);
			infoheader.bmiHeader.biHeight = bitmap.bmHeight;
			infoheader.bmiHeader.biWidth = bitmap.bmWidth;
			infoheader.bmiHeader.biSizeImage = bitmap.bmWidth * bitmap.bmHeight * sizeof(DWORD);

			fileheader.bfType = 0x4D42;
			fileheader.bfOffBits = sizeof(infoheader.bmiHeader) + sizeof(BITMAPFILEHEADER);
			fileheader.bfSize = fileheader.bfOffBits + infoheader.bmiHeader.biSizeImage;

			GetDIBits(hdc, bmp, 0, height, (LPVOID)dwpBits, &infoheader, DIB_RGB_COLORS);

			file = CreateFile(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			WriteFile(file, &fileheader, sizeof(BITMAPFILEHEADER), &wb, NULL);
			WriteFile(file, &infoheader.bmiHeader, sizeof(infoheader.bmiHeader), &wb, NULL);
			WriteFile(file, dwpBits, bitmap.bmWidth * bitmap.bmHeight * 4, &wb, NULL);
			CloseHandle(file);

			delete[] dwpBits;
		}

		HDC getDC() { return hdc; }
		int getWidth() { return width; }
		int getHeight() { return height; }

	private:
		HBITMAP bmp;
		HDC	    hdc;
		HPEN    pen;
		int     width, height;
};

class Vector2D
{
	public:
		Vector2D() { x = y = 0; }
		Vector2D(int a, int b) { x = a; y = b; }
		void Rotate(float angle_r)
		{
			float _x = static_cast<float>(x),
				_y = static_cast<float>(y),
				s = sinf(angle_r),
				c = cosf(angle_r),
				a = _x * c - _y * s,
				b = _x * s + _y * c;
	
			x = static_cast<int>(a);
			y = static_cast<int>(b);
		}
	
		int x, y;
};

class FractalLineTree
{
	public:
		//Winkel zu Bogenmaß
		float DegToRadian(float degree) { return degree * (PI / 180.0f); }
	
		void Create(Bitmap* bitmap, float angle)
		{
			_bitmap = bitmap;
			_angle = DegToRadian(angle);
			float line_len = FIRST_LINE_LENGTH;
	
			Vector2D sp(_bitmap->getWidth() / 2, _bitmap->getHeight() - 1);
			MoveToEx(_bitmap->getDC(), sp.x, sp.y, NULL);
			sp.y -= static_cast<int>(line_len);
			LineTo(_bitmap->getDC(), sp.x, sp.y);
			
			//rechts
			DrawRecursionLine(&sp, line_len, 0, true);
			//links
			DrawRecursionLine(&sp, line_len, 0, false);
		}
	
	private:
		// Rekursionslinie zeichnen (rg: true = rechts, false = links)
		void DrawRecursionLine(Vector2D* sp, float lineLength, float angleDelta, bool rightLine)
		{
			lineLength *= .75f;
			
			//Abbruchbedingung für die Rekursion
			if (lineLength < 5.50f) return;
	
			MoveToEx(_bitmap->getDC(), sp->x, sp->y, NULL);
			Vector2D r(0, static_cast<int>(lineLength));
	
			//Sinus-Funktionen um den Baum hin und her zu schwenken
			if (rightLine) angleDelta -= sin(cos(_angle));
			else angleDelta += sin(_angle);
	
			r.Rotate(angleDelta);
			r.x += sp->x; r.y = sp->y - r.y;
	
			LineTo(_bitmap->getDC(), r.x, r.y);
	
			DrawRecursionLine(&r, lineLength, angleDelta, true);
			DrawRecursionLine(&r, lineLength, angleDelta, false);
		}
	
		Bitmap* _bitmap;
		float     _angle;
};

// ----- MAIN FUNCTION ----- //
int main(int argc, char* argv[])
{
	//Fenster generieren
	ShowWindow(GetConsoleWindow(), SW_MAXIMIZE);
	 
	// Initialwert für Winkelparameter, welcher in der Schleife iteriert wird
	float angle = 0;

	//Iteration (Ein Durchgang = ein Frame)
	while (true)
	{
		//Bitmap Instanz
		Bitmap bmp;

		//BitMap Größe 
		bmp.create(300, 300);

		//Farbe des Line Trees
		bmp.setPenColor(RGB(0, 255, 255));

		//LineTree Instanz
		FractalLineTree tree;

		//Winkel iterieren (ist egal dass er ins unermessliche wächst, da sowieso eine Sinusfunktion angewandt wird und der Baum zu einem Maximalwert ausschwenkt)
		angle += 0.25f;

		//LineTree initialisieren
		tree.Create(&bmp, angle);

		/*Bitmap zeichnen (hiermit kann auch die Größe des Baumes gesteuert werden, 
		allerdings ist es zu empfehlen ebenfalls auch die FIRST_LINE_LENGTH zu ändern 
		und die Abbruchbedingung der Rekursion*/ 
		BitBlt(GetDC(GetConsoleWindow()), 0, 20, 300, 300, bmp.getDC(), 0, 0, SRCCOPY);

		//Bild 3ms für das Auge anhalten
		std::this_thread::sleep_for(std::chrono::milliseconds(3));
	}

	// Ich gebe hier einen Fehlercode zurück, da der Sinn des Programmes ist, die Schleife nicht zu verlassen
	return 1;
}