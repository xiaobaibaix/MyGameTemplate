#include "Animator.h"

namespace editor {

bool Animator::init() {
    if (!Component::init()) return false;
    setName("Animator");
    return true;
}

void Animator::update(float dt) {
    if (_state != PlayState::Playing || !_currentClip) return;

    float duration = _currentClip->getDuration();
    if (duration <= 0.0f) {
        _state = PlayState::Stopped;
        if (_onFinished) _onFinished();
        return;
    }

    _currentTime += dt * _speed;

    if (_wrapMode == WrapMode::Loop) {
        while (_currentTime >= duration) {
            _currentTime -= duration;
        }
    } else if (_wrapMode == WrapMode::PingPong) {
        if (_pingPongForward) {
            if (_currentTime >= duration) {
                _currentTime = duration - (_currentTime - duration);
                _pingPongForward = false;
                if (_currentTime < 0) _currentTime = 0;
            }
        } else {
            if (_currentTime <= 0) {
                _currentTime = -_currentTime;
                _pingPongForward = true;
                if (_currentTime > duration) _currentTime = duration;
            }
        }
    } else { // Once
        if (_currentTime >= duration) {
            _currentTime = duration;
            applySampledValues(_currentClip->sample(_currentTime));
            _state = PlayState::Stopped;
            if (_onFinished) _onFinished();
            return;
        }
    }

    applySampledValues(_currentClip->sample(_currentTime));
}

// ---- Clip Management ----

AnimationClip* Animator::createClip(const std::string& name) {
    // Remove existing clip with same name
    removeClip(name);
    _clips.push_back(AnimationClip());
    _clips.back().name = name;
    return &_clips.back();
}

AnimationClip* Animator::getClip(const std::string& name) {
    for (auto& clip : _clips) {
        if (clip.name == name) return &clip;
    }
    return nullptr;
}

AnimationClip* Animator::getCurrentClip() {
    return _currentClip;
}

void Animator::removeClip(const std::string& name) {
    _clips.erase(
        std::remove_if(_clips.begin(), _clips.end(),
            [&name](const AnimationClip& c) { return c.name == name; }),
        _clips.end()
    );
    if (_currentClipName == name) {
        _currentClip = nullptr;
        _currentClipName.clear();
        _state = PlayState::Stopped;
    }
}

// ---- Playback Control ----

void Animator::play(const std::string& clipName) {
    AnimationClip* clip = getClip(clipName);
    if (!clip) return;

    _currentClip = clip;
    _currentClipName = clipName;
    _currentTime = 0.0f;
    _pingPongForward = true;
    _state = PlayState::Playing;

    // Capture initial state on first play
    if (!_hasInitialState) {
        captureInitialState();
    }

    // Apply first frame immediately
    applySampledValues(clip->sample(0.0f));
}

void Animator::play() {
    if (_currentClip) {
        _currentTime = 0.0f;
        _pingPongForward = true;
        _state = PlayState::Playing;
        applySampledValues(_currentClip->sample(0.0f));
    }
}

void Animator::pause() {
    if (_state == PlayState::Playing) {
        _state = PlayState::Paused;
    }
}

void Animator::resume() {
    if (_state == PlayState::Paused) {
        _state = PlayState::Playing;
    }
}

void Animator::stop() {
    _state = PlayState::Stopped;
    if (_hasInitialState) {
        resetToInitialState();
    }
}

// ---- Internal ----

void Animator::applySampledValues(const AnimationClip::SampledValues& sv) {
    cocos2d::Node* node = _owner;
    if (!node) return;

    node->setPosition(sv.posX, sv.posY);
    node->setScaleX(sv.scaleX);
    node->setScaleY(sv.scaleY);
    node->setRotation(sv.rotation);
    node->setOpacity((GLubyte)sv.opacity);
}

void Animator::captureInitialState() {
    cocos2d::Node* node = _owner;
    if (!node) return;

    _initialState.posX = node->getPositionX();
    _initialState.posY = node->getPositionY();
    _initialState.scaleX = node->getScaleX();
    _initialState.scaleY = node->getScaleY();
    _initialState.rotation = node->getRotation();
    _initialState.opacity = (float)node->getOpacity();
    _hasInitialState = true;
}

void Animator::resetToInitialState() {
    cocos2d::Node* node = _owner;
    if (!node) return;

    node->setPosition(_initialState.posX, _initialState.posY);
    node->setScaleX(_initialState.scaleX);
    node->setScaleY(_initialState.scaleY);
    node->setRotation(_initialState.rotation);
    node->setOpacity((GLubyte)_initialState.opacity);
}

} // namespace editor
