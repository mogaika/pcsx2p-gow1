#include "PrecompiledHeader.h"

#include "gow/resources.h"

using namespace gow;

static IResourceManager *gow::managers[64];

IResourceManager *IResource::GetManager() {
	return managers[serverId];
}
