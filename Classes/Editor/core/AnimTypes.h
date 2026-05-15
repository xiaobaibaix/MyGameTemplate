#ifndef __EDITOR_ANIM_TYPES_H__
#define __EDITOR_ANIM_TYPES_H__

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

namespace editor {

// ==================== Enums ====================

enum class AnimProperty {
    PositionX,
    PositionY,
    ScaleX,
    ScaleY,
    Rotation,
    Opacity
};

enum class EaseType {
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut,
    BounceIn,
    BounceOut,
    ElasticIn,
    ElasticOut
};

// ==================== Keyframe ====================

struct Keyframe {
    float time = 0.0f;
    float value = 0.0f;
    EaseType ease = EaseType::Linear;

    Keyframe() = default;
    Keyframe(float t, float v, EaseType e = EaseType::Linear)
        : time(t), value(v), ease(e) {}
};

// ==================== Animation Track ====================

struct AnimationTrack {
    AnimProperty property = AnimProperty::PositionX;
    std::vector<Keyframe> keyframes;

    void addKeyframe(float time, float value, EaseType ease = EaseType::Linear) {
        keyframes.push_back({time, value, ease});
        sortKeyframes();
    }

    void sortKeyframes() {
        std::sort(keyframes.begin(), keyframes.end(),
            [](const Keyframe& a, const Keyframe& b) { return a.time < b.time; });
    }

    void removeKeyframe(int index) {
        if (index >= 0 && index < (int)keyframes.size()) {
            keyframes.erase(keyframes.begin() + index);
        }
    }

    // Sample value at given time
    float sample(float time) const {
        if (keyframes.empty()) return 0.0f;
        if (keyframes.size() == 1) return keyframes[0].value;
        if (time <= keyframes.front().time) return keyframes.front().value;
        if (time >= keyframes.back().time) return keyframes.back().value;

        // Find the two keyframes to interpolate between
        for (size_t i = 0; i < keyframes.size() - 1; i++) {
            if (time >= keyframes[i].time && time <= keyframes[i + 1].time) {
                float t = (time - keyframes[i].time) / (keyframes[i + 1].time - keyframes[i].time);
                t = applyEase(t, keyframes[i + 1].ease);
                return keyframes[i].value + (keyframes[i + 1].value - keyframes[i].value) * t;
            }
        }
        return keyframes.back().value;
    }

    float getDuration() const {
        if (keyframes.empty()) return 0.0f;
        return keyframes.back().time;
    }

    static std::string propertyName(AnimProperty prop) {
        switch (prop) {
            case AnimProperty::PositionX: return "Position X";
            case AnimProperty::PositionY: return "Position Y";
            case AnimProperty::ScaleX:    return "Scale X";
            case AnimProperty::ScaleY:    return "Scale Y";
            case AnimProperty::Rotation:  return "Rotation";
            case AnimProperty::Opacity:   return "Opacity";
        }
        return "Unknown";
    }

    static std::string propertyShortName(AnimProperty prop) {
        switch (prop) {
            case AnimProperty::PositionX: return "PosX";
            case AnimProperty::PositionY: return "PosY";
            case AnimProperty::ScaleX:    return "ScaleX";
            case AnimProperty::ScaleY:    return "ScaleY";
            case AnimProperty::Rotation:  return "Rot";
            case AnimProperty::Opacity:   return "Alpha";
        }
        return "?";
    }

private:
    static float applyEase(float t, EaseType ease) {
        switch (ease) {
            case EaseType::Linear:     return t;
            case EaseType::EaseIn:     return t * t * t;
            case EaseType::EaseOut:    return 1.0f - std::pow(1.0f - t, 3);
            case EaseType::EaseInOut: {
                if (t < 0.5f) return 4.0f * t * t * t;
                return 1.0f - std::pow(-2.0f * t + 2.0f, 3) / 2.0f;
            }
            case EaseType::BounceIn:  return 1.0f - bounceOut(1.0f - t);
            case EaseType::BounceOut: return bounceOut(t);
            case EaseType::ElasticIn:  return elastic(t, 1.0f);
            case EaseType::ElasticOut: return elasticOut(t, 1.0f);
        }
        return t;
    }

    static float bounceOut(float t) {
        if (t < 1.0f / 2.75f) {
            return 7.5625f * t * t;
        } else if (t < 2.0f / 2.75f) {
            t -= 1.5f / 2.75f;
            return 7.5625f * t * t + 0.75f;
        } else if (t < 2.5f / 2.75f) {
            t -= 2.25f / 2.75f;
            return 7.5625f * t * t + 0.9375f;
        } else {
            t -= 2.625f / 2.75f;
            return 7.5625f * t * t + 0.984375f;
        }
    }

    static float elastic(float t, float s) {
        if (t == 0 || t == 1) return t;
        return -std::pow(2.0f, 10.0f * (t - 1.0f)) * std::sin((t - 1.1f) * 5.0f * 3.14159265f / s);
    }

    static float elasticOut(float t, float s) {
        if (t == 0 || t == 1) return t;
        return std::pow(2.0f, -10.0f * t) * std::sin((t - 0.1f) * 5.0f * 3.14159265f / s) + 1.0f;
    }
};

// ==================== Animation Clip ====================

struct AnimationClip {
    std::string name = "New Animation";
    std::vector<AnimationTrack> tracks;

    AnimationTrack* getTrack(AnimProperty prop) {
        for (auto& track : tracks) {
            if (track.property == prop) return &track;
        }
        return nullptr;
    }

    AnimationTrack* addTrack(AnimProperty prop) {
        tracks.push_back(AnimationTrack());
        tracks.back().property = prop;
        return &tracks.back();
    }

    void removeTrack(AnimProperty prop) {
        tracks.erase(
            std::remove_if(tracks.begin(), tracks.end(),
                [prop](const AnimationTrack& t) { return t.property == prop; }),
            tracks.end()
        );
    }

    float getDuration() const {
        float maxDur = 0.0f;
        for (const auto& track : tracks) {
            maxDur = std::max(maxDur, track.getDuration());
        }
        return maxDur;
    }

    // Sample all tracks at given time, return property values
    struct SampledValues {
        float posX = 0, posY = 0;
        float scaleX = 1, scaleY = 1;
        float rotation = 0;
        float opacity = 255;
    };

    SampledValues sample(float time) const {
        SampledValues sv;
        for (const auto& track : tracks) {
            float v = track.sample(time);
            switch (track.property) {
                case AnimProperty::PositionX: sv.posX = v; break;
                case AnimProperty::PositionY: sv.posY = v; break;
                case AnimProperty::ScaleX:    sv.scaleX = v; break;
                case AnimProperty::ScaleY:    sv.scaleY = v; break;
                case AnimProperty::Rotation:  sv.rotation = v; break;
                case AnimProperty::Opacity:   sv.opacity = v; break;
            }
        }
        return sv;
    }
};

} // namespace editor

#endif // __EDITOR_ANIM_TYPES_H__
