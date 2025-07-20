
#include "hazardPointerGuard.h"

inline thread_local std::vector<HazardPointerManager::RetiredNode> HazardPointerManager::retired_nodes;