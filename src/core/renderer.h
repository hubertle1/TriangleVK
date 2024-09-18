#include "context.h"

class Renderer
{
public:
	Renderer( const Window& window, const std::vector<Vertex>& vertices );
	void OnUpdate();

private:
	Window window;
	Context context;
};