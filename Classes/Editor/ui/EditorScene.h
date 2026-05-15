#ifndef __EDITOR_EDITOR_SCENE_H__
#define __EDITOR_EDITOR_SCENE_H__

#include "cocos2d.h"
#include "../core/AnimTypes.h"
#include "../core/Animator.h"
#include <string>
#include <vector>

namespace editor {

// ==================== EditorScene ====================
// Main scene for the animation editor.
// Uses a CustomCommand to render ImGui AFTER cocos2d finishes its rendering pipeline.

class EditorScene : public cocos2d::Scene {
public:
    static cocos2d::Scene* createScene();
    virtual bool init() override;
    virtual void update(float dt) override;
    virtual void visit(cocos2d::Renderer* renderer, const cocos2d::Mat4& parentTransform, uint32_t parentFlags) override;
    virtual ~EditorScene();

    CREATE_FUNC(EditorScene);

protected:
    // Canvas
    void drawGrid();

    // ImGui integration
    void initImGui();
    void shutdownImGui();

    // Build ImGui draw list (called from update)
    void buildImGuiUI();

    // Actual OpenGL render of ImGui (called from CustomCommand)
    void onRenderImGui();

    // Editor UI panels
    void drawMenuBar();
    void drawToolbar();
    void drawTimeline();
    void drawPropertiesPanel();
    void drawHierarchyPanel();

    // Editor actions
    void createTestObject();
    void createDemoAnimation();
    void onDeleteSelected();

    // Selection
    void selectNode(cocos2d::Node* node);

    // Data
    struct EditorObject {
        std::string name;
        cocos2d::Node* node = nullptr;
        Animator* animator = nullptr;
    };

    std::vector<EditorObject> _objects;
    EditorObject* _selectedObject = nullptr;
    AnimationClip* _selectedClip = nullptr;
    AnimationTrack* _selectedTrack = nullptr;
    int _selectedKeyframeIndex = -1;

    // Timeline state
    float _timelineZoom = 50.0f;
    float _timelineScroll = 0.0f;
    float _playbackTime = 0.0f;
    bool _isPlaying = false;
    bool _isPaused = false;

    // ImGui state
    bool _showTimeline = true;
    bool _showProperties = true;
    bool _showHierarchy = true;

    // Layout
    float _panelWidth = 280.0f;
    float _timelineHeight = 200.0f;

    // ImGui
    void* _imguiContext = nullptr;
    bool _imguiInitialized = false;
    cocos2d::CustomCommand _imguiRenderCmd;
};

} // namespace editor

#endif // __EDITOR_EDITOR_SCENE_H__
