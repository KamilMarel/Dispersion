#include "Model.h"

class SceneGraphNode
{
public:
	SceneGraphNode();
	SceneGraphNode(std::string objectModelPath);
	~SceneGraphNode();

	void addChild(SceneGraphNode* nodeToAdd);
	const std::vector<SceneGraphNode*>& getChildren() const;
	const std::vector<SceneGraphNode*> getRefractors() const;
	const std::vector<SceneGraphNode*> getOpaqueChildren() const;

	void render(const glm::mat4& parentTransform, bool parentTransformIsDirty, Shader& renderingShader);
	void setRefractor(bool isRefractor);

	void translate(const glm::vec3& translation);
	void rotate(const glm::vec3& rotationAxis, float angle);
	void scale(const glm::vec3& scale);
private:
	std::vector<SceneGraphNode*> children;

	glm::mat4 transform;
	bool dirtyTransform;
	Model* objectModel;
	bool isRefractor;
};
