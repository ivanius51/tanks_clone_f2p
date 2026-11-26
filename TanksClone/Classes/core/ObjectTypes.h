#pragma once

#include <memory>

#include "model/ConfigNode.h"
#include "model/JsonValue.h"
#include "model/RefCounted.h"

namespace game
{
/** A parsed JSON config file. */
struct ObjectConfig
{
	model::JsonValue Values;
	THandle<const model::IConfigNode> Node;
};
} // namespace game
