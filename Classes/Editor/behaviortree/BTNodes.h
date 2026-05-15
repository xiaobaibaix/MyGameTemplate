#ifndef __EDITOR_BT_NODES_H__
#define __EDITOR_BT_NODES_H__

#include "behaviac/agent/agent.h"
#include "behaviac/behaviortree/behaviortree.h"
#include "../core/Animator.h"
#include "../core/AnimTypes.h"
#include <string>

namespace editor {

// ==================== AnimAgent ====================
// A behaviac::Agent subclass that holds an Animator pointer.
// Animation control methods are exposed as Agent methods,
// which can be bound to behaviac Action nodes via XML or code.
//
// Usage in XML:
//   <Action name="PlayAnimation" method="playClip" param="walk" />
//   <Action name="StopAnimation" method="stopAnim" />
//   <Action name="SetSpeed" method="setAnimSpeed" param="2.0" />
//   <Action name="SetProperty" method="setProperty" param="0,100.0" />

class AnimAgent : public behaviac::Agent {
    BEHAVIAC_DECLARE_AGENTTYPE(AnimAgent, behaviac::Agent);
public:
    AnimAgent();
    virtual ~AnimAgent();

    // Set the target Animator component
    void setAnimator(Animator* animator) { _animator = animator; }
    Animator* getAnimator() const { return _animator; }

    // ---- Methods callable from behavior tree Action nodes ----

    // Play a named animation clip
    // Usage: <Action method="playClip" param="clipName" />
    bool playClip(const std::string& clipName);

    // Stop current animation
    // Usage: <Action method="stopAnim" />
    bool stopAnim();

    // Pause current animation
    // Usage: <Action method="pauseAnim" />
    bool pauseAnim();

    // Resume paused animation
    // Usage: <Action method="resumeAnim" />
    bool resumeAnim();

    // Set playback speed
    // Usage: <Action method="setAnimSpeed" param="2.0" />
    bool setAnimSpeed(float speed);

    // Set wrap mode: 0=Once, 1=Loop, 2=PingPong
    // Usage: <Action method="setWrapMode" param="1" />
    bool setWrapMode(int mode);

    // Directly set a node property (instant, no animation)
    // Usage: <Action method="setProperty" param="propertyIndex,value" />
    // propertyIndex: 0=PosX, 1=PosY, 2=ScaleX, 3=ScaleY, 4=Rotation, 5=Opacity
    bool setProperty(int propertyIndex, float value);

    // Check if animation is still playing (returns true = running, false = stopped)
    // Usage: <Action method="isAnimRunning" />
    bool isAnimRunning();

    // ---- Properties accessible from behavior tree ----
    float getAnimSpeed() const { return _animSpeed; }
    void setAnimSpeedProperty(float speed) { _animSpeed = speed; }

    const std::string& getClipName() const { return _clipName; }
    void setClipName(const std::string& name) { _clipName = name; }

    bool getLoop() const { return _loop; }
    void setLoop(bool loop) { _loop = loop; }

private:
    Animator* _animator = nullptr;
    float _animSpeed = 1.0f;
    std::string _clipName;
    bool _loop = false;
};

} // namespace editor

#endif // __EDITOR_BT_NODES_H__
