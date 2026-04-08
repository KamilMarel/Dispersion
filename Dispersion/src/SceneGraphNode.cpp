#include "SceneGraphNode.h"

SceneGraphNode::SceneGraphNode()
{
	transform = glm::mat4(1.0f);
	objectModel = nullptr;
	dirtyTransform = true;
	isRefractor = false;
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

void SceneGraphNode::addChild(SceneGraphNode* nodeToAdd)
{
	children.push_back(nodeToAdd);
}

const std::vector<SceneGraphNode*>& SceneGraphNode::getChildren() const
{
	return children;
}

const std::vector<SceneGraphNode*> SceneGraphNode::getRefractors() const
{
	std::vector<SceneGraphNode*> queryResult;

	for (SceneGraphNode* child : children)
	{
		if (child->isRefractor)
		{
			queryResult.push_back(child);
		}
	}

	return queryResult;
}

const std::vector<SceneGraphNode*> SceneGraphNode::getOpaqueChildren() const
{
	std::vector<SceneGraphNode*> queryResult;

	for (SceneGraphNode* child : children)
	{
		if (!child->isRefractor)
		{
			queryResult.push_back(child);
		}
	}

	return queryResult;
}

void SceneGraphNode::render(const glm::mat4& parentTransform, bool parentTransformIsDirty, Shader& renderingShader)
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

	for (SceneGraphNode* child : children)
	{
		child->render(transform, dirtyFlag, renderingShader);
	}
}

void SceneGraphNode::setRefractor(bool isRefractor)
{
	this->isRefractor = isRefractor;
}

void SceneGraphNode::translate(const glm::vec3& translation)
{
	transform = glm::translate(transform, translation);
}

void SceneGraphNode::rotate(const glm::vec3& rotationAxis, float angle)
{
	transform = glm::rotate(transform, angle, rotationAxis);
}

void SceneGraphNode::scale(const glm::vec3& scale)
{
	transform = glm::scale(transform, scale);
}
