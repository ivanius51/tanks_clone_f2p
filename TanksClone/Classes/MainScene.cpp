#include "MainScene.h"
#include "SimpleAudioEngine.h"
#include "KeyboardController.h"
#include "ObjectsFactory.h"
#include "ConfigReader.h"

USING_NS_CC;

Scene* MainScene::createScene()
{
	return MainScene::create();
}

// Print useful error message instead of segfaulting when files are not there.
static void problemLoading(const char* filename)
{
	printf("Error while loading: %s\n", filename);
	printf("Depending on how you compiled you might have to add 'Resources/' in front of filenames in MainSceneScene.cpp\n");
}

// on "init" you need to initialize your instance
bool MainScene::init()
{
	//////////////////////////////
	// 1. super init first
	if ( !Scene::init() )
	{
		return false;
	}

	auto visibleSize = Director::getInstance()->getVisibleSize();
	Vec2 origin = Director::getInstance()->getVisibleOrigin();

	/////////////////////////////
	// 2. add a menu item with "X" image, which is clicked to quit the program
	//    you may modify it.

	// add a "close" icon to exit the progress. it's an autorelease object
	auto closeItem = MenuItemImage::create(
		"CloseNormal.png",
		"CloseSelected.png",
		CC_CALLBACK_1(MainScene::menuCloseCallback, this));

	if ( closeItem == nullptr ||
		closeItem->getContentSize().width <= 0 ||
		closeItem->getContentSize().height <= 0 )
	{
		problemLoading("'CloseNormal.png' and 'CloseSelected.png'");
	}
	else
	{
		float x = origin.x + visibleSize.width - closeItem->getContentSize().width / 2;
		float y = origin.y + closeItem->getContentSize().height / 2;
		closeItem->setPosition(Vec2(x, y));
	}

	// create menu, it's an autorelease object
	auto menu = Menu::create(closeItem, NULL);
	menu->setPosition(Vec2::ZERO);
	this->addChild(menu, 1);

	/////////////////////////////
	// 3. add your codes below...

	// add a label shows "Hello World"
	// create and initialize a label

	auto label = Label::createWithTTF("Hello World", "fonts/Marker Felt.ttf", 24);
	if ( label == nullptr )
	{
		problemLoading("'fonts/Marker Felt.ttf'");
	}
	else
	{
		// position the label on the center of the screen
		label->setPosition(Vec2(origin.x + visibleSize.width / 2,
			origin.y + visibleSize.height - label->getContentSize().height));

		// add the label as a child to this layer
		this->addChild(label, 1);
	}

	auto spritecache = SpriteFrameCache::getInstance();
	spritecache->addSpriteFramesWithFile("minimalist.plist");

	CONFIGS.readObjectConfigs("configs\\");


	ValueMap testData;
	testData["node_type"] = static_cast<int>(eNodeType::NODE_TYPE_SPRITE);
	testData["type"] = static_cast<int>(eVehicleType::VEHICLE_TYPE_TANK);
	testData["name"] = "PLAYER";
	testData["texture_path"] = "green_tank";
	
	ValueVector posVec;
	posVec.reserve(2);
	posVec.push_back(Value(visibleSize.width / 2 + origin.x));
	posVec.push_back(Value(visibleSize.height / 2 + origin.y));
	testData["position"] = posVec;

	auto tank = FACTORY.getInstance().create(eObjectType::OBJECT_TYPE_VEHICLE, testData, this);
		 
	auto framesAnimation = Animation::create();
	//framesAnimation->addSpriteFrame(newspriteFrame);
	for ( int i = 1; i < 8; ++i )
	{
		auto frame = spritecache->getSpriteFrameByName("green_tank_anim_" + Value(i).asString());
		framesAnimation->addSpriteFrame(frame);
	}
	framesAnimation->setDelayPerUnit(0.15f);
	framesAnimation->setLoops(-1);
	auto action = tank->getVisualNode()->runAction(Speed::create(Animate::create(framesAnimation), 1.0f));
	action->setTag(static_cast<int>(eActionTag::ACTION_TAG_ANIMATION));
	dynamic_cast<Speed*>(action)->setSpeed(0.0f);
	return true;
}


void MainScene::menuCloseCallback(Ref* pSender)
{
	//Close the cocos2d-x game scene and quit the application
	Director::getInstance()->end();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
	exit(0);
#endif

	/*To navigate back to native iOS screen(if present) without quitting the application  ,do not use Director::getInstance()->end() and exit(0) as given above,instead trigger a custom event created in RootViewController.mm as below*/

	//EventCustom customEndEvent("game_scene_close_event");
	//_eventDispatcher->dispatchEvent(&customEndEvent);


}
