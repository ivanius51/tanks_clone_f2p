#pragma once

#include <memory>

#include "cocos2d.h"

#include "model/ConfigNode.h"

namespace tanks
{

// A parsed JSON config file.
struct ObjectConfig
{
	cocos2d::ValueMap Values;
	// The same data behind a format-free interface; loaders read this one.
	std::shared_ptr<const model::IConfigNode> Node;
};

} // namespace tanks
