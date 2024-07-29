#pragma once
#include <windows.h>
#include <string>

class Window
{
public:
	Window(const std::string& windowName, int width = 640, int height = 480);
	void OnUpdate() const;
	bool isOpen() const;

private:
	HWND window;
	static bool isRunning;
	const char* className = "vulkan_triangle";

	static LRESULT CALLBACK WindowCallbacks( HWND window, UINT msg, WPARAM wParam, LPARAM lParam );
	std::pair<long, long> GetScreenResolution();
	void RaportError( const std::string& message ) const;
};