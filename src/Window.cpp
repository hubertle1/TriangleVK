#include "Window.h"

#include <stdexcept>

bool Window::isRunning = false;

Window::Window( const std::string& windowName, int width, int height )
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

void Window::RaportError( const std::string& message ) const
{
	MessageBoxA( window, message.c_str(), "Error", MB_ICONEXCLAMATION | MB_OK);
	throw std::runtime_error( message );
}
