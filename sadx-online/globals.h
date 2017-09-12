#pragma once

#include <memory>
#include "PacketBroker.h"

namespace globals
{
	extern std::unique_ptr<PacketBroker> broker;
}
