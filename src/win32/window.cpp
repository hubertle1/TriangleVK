#include "window.h"

#include <stdexcept>

bool Window::isRunning = false;

Window::Window( const std::string& windowName, int width, int height )
{
	this->isRunning = true;
	HINSTANCE instanceHandle = this->GetModule();
	WNDCLASS windowClass =
	{
		.lpfnWndProc = this->WindowCallbacks,
		.hInstance = instanceHandle,
		.lpszClassName = this->className
	};

	if( !RegisterClassA( &windowClass ) )
	{
		this->RaportError( "Failed to register window class" );
	}

	auto resolution = this->GetScreenResolution();
	const auto& vStartingPosition = (resolution.first - width) / 2;
	const auto& hStartingPosition = (resolution.second - height) / 2;

	this->window = CreateWindowExA( WS_EX_APPWINDOW, this->className, windowName.c_str(),
		WS_TILEDWINDOW, vStartingPosition, hStartingPosition, width, height, 0, 0, instanceHandle, 0 );

	if( this->window == NULL )
	{
		this->RaportError( "Failed to create window" );
		return;	// This return suppresses Intellisense C6387 warning
	}
	
	ShowWindow( this->window, SW_SHOW );
}

HINSTANCE Window::GetModule() const
{
	return GetModuleHandleA( 0 );
}

HWND Window::GetWindow() const
{
	return this->window;
}

bool Window::IsOpen() const
{
	return this->isRunning;
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

std::pair<long, long> Window::GetScreenResolution()
{
	RECT desktop;
	const HWND hDesktop = GetDesktopWindow();
	if( GetWindowRect( hDesktop, &desktop ) == NULL )
	{
		this->RaportError( "Failed to obtain screen resolution!" );
	}

	return { desktop.right, desktop.bottom };
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

void Window::RaportError( const std::string& message ) const
{
	MessageBoxA( window, message.c_str(), "Error", MB_ICONEXCLAMATION | MB_OK);
	throw std::runtime_error( message );
}
