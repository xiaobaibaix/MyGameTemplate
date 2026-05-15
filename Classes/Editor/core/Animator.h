#ifndef __EDITOR_ANIMATOR_H__
#define __EDITOR_ANIMATOR_H__

#include "cocos2d.h"
#include "AnimTypes.h"
#include <string>
#include <vector>
#include <functional>

namespace editor {

// ==================== Animator Component ====================
// Attach to any cocos2d::Node to play AnimationClips on it.

class Animator : public cocos2d::Component {
public:
    enum class PlayState {
        Stopped,
        Playing,
        Paused
    };

    enum class WrapMode {
        Once,       // Play once then stop
        Loop,       // Loop forever
        PingPong    // Forward then backward
    };

    CREATE_FUNC(Animator);

    virtual bool init() override;
    virtual void update(float dt) override;

    // ---- Animation Clip Management ----
    AnimationClip* createClip(const std::string& name);
    AnimationClip* getClip(const std::string& name);
    AnimationClip* getCurrentClip();
    const std::vector<AnimationClip>& getClips() const { return _clips; }
    void removeClip(const std::string& name);

    // ---- Playback Control ----
    void play(const std::string& clipName);
    void play();
    void pause();
    void resume();
    void stop();

    // ---- State ----
    PlayState getState() const { return _state; }
    float getCurrentTime() const { return _currentTime; }
    float getSpeed() const { return _speed; }
    void setSpeed(float speed) { _speed = speed; }
    WrapMode getWrapMode() const { return _wrapMode; }
    void setWrapMode(WrapMode mode) { _wrapMode = mode; }

    // ---- Callbacks ----
    using AnimCallback = std::function<void()>;
    void setOnFinished(AnimCallback cb) { _onFinished = cb; }

    // ---- Save initial node state (for reset on stop) ----
    void captureInitialState();

private:
    void applySampledValues(const AnimationClip::SampledValues& sv);
    void resetToInitialState();

    std::vector<AnimationClip> _clips;
    AnimationClip* _currentClip = nullptr;
    std::string _currentClipName;

    PlayState _state = PlayState::Stopped;
    WrapMode _wrapMode = WrapMode::Once;
    float _currentTime = 0.0f;
    float _speed = 1.0f;

    // For ping-pong
    bool _pingPongForward = true;

    // Initial state for reset
    struct InitialState {
        float posX = 0, posY = 0;
        float scaleX = 1, scaleY = 1;
        float rotation = 0;
        float opacity = 255;
    };
    InitialState _initialState;
    bool _hasInitialState = false;

    AnimCallback _onFinished;
};

} // namespace editor

#endif // __EDITOR_ANIMATOR_H__
