#include "Model.h"

class SceneGraphNode
{
public:
	SceneGraphNode();
	SceneGraphNode(std::string objectModelPath);
	~SceneGraphNode();

	void render(glm::mat4& parentTransform, bool parentTransformIsDirty, Shader& renderingShader);
private:
	std::vector<SceneGraphNode> children;

	glm::mat4 transform;
	bool dirtyTransform;
	Model* objectModel;
};
