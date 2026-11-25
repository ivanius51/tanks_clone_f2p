#pragma once

#include <memory>

#include "cocos2d.h"

#include "model/ConfigNode.h"

namespace game
{
/** A parsed JSON config file. */
struct ObjectConfig
{
	cocos2d::ValueMap Values;
	std::shared_ptr<const model::IConfigNode> Node;
};
} // namespace game
