#include "context.h"

class Renderer
{
public:
	Renderer( const Window& window );
	void OnUpdate();

private:
	Window window;
	Context context;
};