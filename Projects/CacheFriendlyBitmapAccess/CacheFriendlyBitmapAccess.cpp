// CacheFriendlyBitmapAccess.cpp : Defines the entry point for the console application.
//

#include <string>
#include <iomanip>
#include <iostream>
#include <chrono>
#include <Windows.h>
#include <Gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

class timer
{
public:
	timer() = default;
	void start(const std::string& text_)
	{
		text = text_;
		begin = std::chrono::high_resolution_clock::now();
	}
	void stop()
	{
		auto end = std::chrono::high_resolution_clock::now();
		auto dur = end - begin;
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
		std::cout << std::setw(30) << text << ":" << std::setw(5) << ms << "ms" << std::endl;
	}

private:
	std::string text;
	std::chrono::high_resolution_clock::time_point begin;
};

Gdiplus::GdiplusStartupInput m_gdiplusStartupInput;
ULONG_PTR m_gdiplusToken;

void write_bitmap_cache_friendly();
void write_bitmap_cache_unfriendly();

int main()
{
	timer stopwatch;

	Gdiplus::GdiplusStartup(&m_gdiplusToken, &m_gdiplusStartupInput, NULL);

	stopwatch.start("write_bitmap_cache_friendly");
	write_bitmap_cache_friendly();
	stopwatch.stop();

	stopwatch.start("write_bitmap_cache_unfriendly");
	write_bitmap_cache_unfriendly();
	stopwatch.stop();

	Gdiplus::GdiplusShutdown(m_gdiplusToken);

    return 0;
}

void write_bitmap_cache_friendly()
{
	UINT* pixelsDest = NULL;

	using namespace Gdiplus;

	Bitmap bmp(4000, 4000, PixelFormat32bppARGB);

	BitmapData bitmapDataDest;
	Rect rect(0, 0, bmp.GetWidth(), bmp.GetHeight());

	bmp.LockBits(
		&rect,
		ImageLockModeWrite,
		PixelFormat32bppARGB,
		&bitmapDataDest);


	pixelsDest = (UINT*)bitmapDataDest.Scan0;

	if (!pixelsDest)
		return;

	UINT col = 0;
	int stride = bitmapDataDest.Stride >> 2;
	for (UINT row = 0; row < bitmapDataDest.Height; ++row)
	{
		for (col = 0; col < bitmapDataDest.Width; ++col)
		{
			UINT index = row * stride + col;
			pixelsDest[index] = 0xff << 24 | 255 << 16 | 255 << 8; // yellow color
		}
	}

	bmp.UnlockBits(&bitmapDataDest);
}

void write_bitmap_cache_unfriendly()
{
	UINT* pixelsDest = NULL;

	using namespace Gdiplus;

	Bitmap bmp(4000, 4000, PixelFormat32bppARGB);

	BitmapData bitmapDataDest;
	Rect rect(0, 0, bmp.GetWidth(), bmp.GetHeight());

	bmp.LockBits(
		&rect,
		ImageLockModeWrite,
		PixelFormat32bppARGB,
		&bitmapDataDest);


	pixelsDest = (UINT*)bitmapDataDest.Scan0;

	if (!pixelsDest)
		return;

	UINT row = 0;
	int stride = bitmapDataDest.Stride >> 2;
	for (UINT col = 0; col < bitmapDataDest.Width; ++col)
	{
		for (row = 0; row < bitmapDataDest.Height; ++row)
		{
			UINT index = row * stride + col;
			pixelsDest[index] = 0xff << 24 | 255 << 16 | 255 << 8; // yellow color
		}
	}

	bmp.UnlockBits(&bitmapDataDest);
}
