#include "context.h"

#include "transformations.h"

class Renderer
{
public:
	Renderer( const Window& window, const std::vector<Vertex>& vertices, const std::string& texturePath );
	void OnUpdate();

private:
	Window window;
	Context context;

	size_t currentFrame = 0;
};