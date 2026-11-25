#pragma once

#include <memory>

#include "cocos2d.h"

#include "model/ConfigNode.h"
#include "model/RefCounted.h"

namespace game
{
/** A parsed JSON config file. */
struct ObjectConfig
{
	cocos2d::ValueMap Values;
	THandle<const model::IConfigNode> Node;
};
} // namespace game
