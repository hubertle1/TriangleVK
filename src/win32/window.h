#pragma once
#include <windows.h>
#include <string>

class Window
{
public:
	Window(const std::string& windowName, int width = 640, int height = 480);
	HINSTANCE GetModule() const;
	HWND GetWindow() const;
	bool IsOpen() const;
	void OnUpdate() const;

private:
	HWND window;
	static bool isRunning;
	const char* className = "vulkan_triangle";

	std::pair<long, long> GetScreenResolution();
	static LRESULT CALLBACK WindowCallbacks( HWND window, UINT msg, WPARAM wParam, LPARAM lParam );
	void RaportError( const std::string& message ) const;
};