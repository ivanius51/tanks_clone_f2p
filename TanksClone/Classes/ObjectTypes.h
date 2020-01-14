#ifndef _ObjectTypes_H_
#define _ObjectTypes_H_

#include "ObjectEnums.h"
#include "cocos2d.h"

struct sBaseConfig
{
	cocos2d::ValueMap data;
};

struct sNodeConfig  : public sBaseConfig
{

};

struct sObjectConfig : public sNodeConfig
{

};

#endif//Object_Types

