#include "Window.h"

#include <stdexcept>

bool Window::isRunning = false;

Window::Window( const std::string& windowName )
{
	this->isRunning = true;
	HINSTANCE instanceHandle = GetModuleHandleA( 0 );
	WNDCLASS windowClass =
	{
		.lpfnWndProc = this->WindowCallbacks,
		.hInstance = instanceHandle,
		.lpszClassName = this->className
	};

	if( !RegisterClassA( &windowClass ) )
	{
		this->raportError( "Failed to register window class" );
	}

	this->window = CreateWindowExA( WS_EX_APPWINDOW, this->className, windowName.c_str(),
		WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_TILED,
		100, 100, 720, 480, 0, 0, instanceHandle, 0 );

	if( this->window == NULL )
	{
		this->raportError( "Failed to create window" );
	}
	else
	{
		// Else block required for Intellisense warning supression
		ShowWindow( this->window, SW_SHOW );
	}
}

void Window::OnUpdate() const
{
	MSG message;

	while( PeekMessageA( &message, this->window, 0, 0, PM_REMOVE ) )
	{
		TranslateMessage( &message );
		DispatchMessageA( &message );
	}
}

bool Window::isOpen() const
{
	return this->isRunning;
}

LRESULT Window::WindowCallbacks( HWND window, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
	case WM_CLOSE:
		isRunning = false;
		break;
	}

	return DefWindowProcA(window, msg, wParam, lParam);
}

void Window::raportError( const std::string& message ) const
{
	MessageBoxA( window, message.c_str(), "Error", MB_ICONEXCLAMATION | MB_OK);
	throw std::runtime_error( message );
}
