#include "BTNodes.h"
#include "cocos2d.h"
#include <cstdlib>

namespace editor {

// ==================== AnimAgent ====================

AnimAgent::AnimAgent() {}
AnimAgent::~AnimAgent() {}

bool AnimAgent::playClip(const std::string& clipName) {
    if (!_animator) return false;
    _animator->play(clipName);
    return true;
}

bool AnimAgent::stopAnim() {
    if (!_animator) return false;
    _animator->stop();
    return true;
}

bool AnimAgent::pauseAnim() {
    if (!_animator) return false;
    _animator->pause();
    return true;
}

bool AnimAgent::resumeAnim() {
    if (!_animator) return false;
    _animator->resume();
    return true;
}

bool AnimAgent::setAnimSpeed(float speed) {
    if (!_animator) return false;
    _animator->setSpeed(speed);
    _animSpeed = speed;
    return true;
}

bool AnimAgent::setWrapMode(int mode) {
    if (!_animator) return false;
    if (mode >= 0 && mode <= 2) {
        _animator->setWrapMode((Animator::WrapMode)mode);
    }
    return true;
}

bool AnimAgent::setProperty(int propertyIndex, float value) {
    if (!_animator || !_animator->getOwner()) return false;

    cocos2d::Node* node = _animator->getOwner();
    AnimProperty prop = (AnimProperty)propertyIndex;
    switch (prop) {
        case AnimProperty::PositionX: node->setPositionX(value); break;
        case AnimProperty::PositionY: node->setPositionY(value); break;
        case AnimProperty::ScaleX:    node->setScaleX(value); break;
        case AnimProperty::ScaleY:    node->setScaleY(value); break;
        case AnimProperty::Rotation:  node->setRotation(value); break;
        case AnimProperty::Opacity:   node->setOpacity((GLubyte)value); break;
        default: return false;
    }
    return true;
}

bool AnimAgent::isAnimRunning() {
    if (!_animator) return false;
    return _animator->getState() == Animator::PlayState::Playing;
}

} // namespace editor
