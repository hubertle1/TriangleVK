#include "win32/window.h"
#include "context.h"

class Renderer
{
public:
	Renderer( const Window& window );

private:
	const Context& context;
};