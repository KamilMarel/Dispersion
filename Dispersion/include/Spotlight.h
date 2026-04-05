#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Spotlight
{
	glm::vec3 position;
	glm::vec3 direction;
	glm::vec3 color;
	float innerCutoff;
	float outerCutoff;
};