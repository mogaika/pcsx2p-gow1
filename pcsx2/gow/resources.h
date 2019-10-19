#pragma once

#include <map>

namespace gow {

class IResourceManager {
public:
	virtual void Event(int id, void *data) = 0;
};

} // namespace gow
