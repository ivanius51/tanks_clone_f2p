#pragma once

#include "cocos2d.h"

/** The application: creates the window, loads nothing itself and starts the first scene (or the tests with --test). */
class AppDelegate : private cocos2d::Application
{
public:
	AppDelegate();
	~AppDelegate();

	void initGLContextAttrs() override;
	bool applicationDidFinishLaunching() override;
	void applicationDidEnterBackground() override;
	void applicationWillEnterForeground() override;
};
