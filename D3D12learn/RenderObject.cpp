#include "RenderObject.h"

void RenderObject::SetDrawArgs(std::string name)
{
	IndexCount = Geo->DrawArgs[name].IndexCount;
	StartIndexLocation = Geo->DrawArgs[name].StartIndexLocation;
	BaseVertexLocation = Geo->DrawArgs[name].BaseVertexLocation;
}
