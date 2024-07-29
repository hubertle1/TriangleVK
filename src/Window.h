#pragma once
#include <windows.h>
#include <string>

class Window
{
public:
	Window(const std::string& windowName);
	void OnUpdate() const;
	bool isOpen() const;

private:
	HWND window;
	static bool isRunning;
	const char* className = "vulkan_triangle";

	static LRESULT CALLBACK WindowCallbacks( HWND window, UINT msg, WPARAM wParam, LPARAM lParam );
	void raportError( const std::string& message ) const;
};