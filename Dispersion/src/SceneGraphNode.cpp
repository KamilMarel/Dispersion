#include "SceneGraphNode.h"

SceneGraphNode::SceneGraphNode()
{
	transform = glm::mat4(1.0f);
	objectModel = nullptr;
	dirtyTransform = true;
}

SceneGraphNode::SceneGraphNode(std::string objectModelPath) : SceneGraphNode()
{
	objectModel = new Model(objectModelPath);
}

SceneGraphNode::~SceneGraphNode()
{
	if (objectModel != nullptr)
	{
		delete objectModel;
	}
}

void SceneGraphNode::render(glm::mat4& parentTransform, bool parentTransformIsDirty, Shader& renderingShader)
{
	bool dirtyFlag = parentTransformIsDirty;
	dirtyFlag |= dirtyTransform;
	if (dirtyFlag)
	{
		transform *= parentTransform;
		dirtyTransform = false;
	}

	if (objectModel != nullptr)
	{
		renderingShader.setMat4("model", transform);
		objectModel->Draw(renderingShader);
	}

	for (SceneGraphNode child : children)
	{
		child.render(transform, dirtyFlag, renderingShader);
	}
}
