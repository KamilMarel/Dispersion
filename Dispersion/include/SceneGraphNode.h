#include "Model.h"

class SceneGraphNode
{
public:
	SceneGraphNode();
	SceneGraphNode(std::string objectModelPath);
	~SceneGraphNode();

	void addChild(SceneGraphNode* nodeToAdd);

	void render(glm::mat4& parentTransform, bool parentTransformIsDirty, Shader& renderingShader);

	void translate(const glm::vec3& translation);
	void rotate(const glm::vec3& rotationAxis, float angle);
	void scale(const glm::vec3& scale);
private:
	std::vector<SceneGraphNode*> children;

	glm::mat4 transform;
	bool dirtyTransform;
	Model* objectModel;
};
