#ifndef __EXTERNAL_LIB_TEST_SCENE_H__
#define __EXTERNAL_LIB_TEST_SCENE_H__

#include "cocos2d.h"

class ExternalLibTestScene : public cocos2d::Scene
{
public:
    static cocos2d::Scene* createScene();
    virtual bool init();

    // 测试结果日志
    void testEnTT();
    void testBehaviac();
    void testImGui();

    CREATE_FUNC(ExternalLibTestScene);
};

#endif // __EXTERNAL_LIB_TEST_SCENE_H__
