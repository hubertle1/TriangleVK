#include "context.h"

class Renderer
{
public:
	Renderer( const Window& window );
	void OnUpdate();

private:
	Context context;
};