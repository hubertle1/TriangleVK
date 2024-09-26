#include "context.h"

class Renderer
{
public:
	Renderer( const Window& window, const std::vector<Vertex>& vertices, const std::string& texturePath );
	void OnUpdate( const glm::vec3& rotation );

private:
	Window window;
	Context context;
};