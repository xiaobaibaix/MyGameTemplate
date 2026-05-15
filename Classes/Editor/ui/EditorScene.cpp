#include "EditorScene.h"
#include <windows.h>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_opengl3.h"
#include "platform/CCGLView.h"
#include "platform/desktop/CCGLViewImpl-desktop.h"

#include <GL/gl.h>

// Forward declare - from imgui_impl_win32.h (wrapped in #if 0)
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace editor {

// ==================== Window Subclassing ====================

static WNDPROC s_OriginalWndProc = nullptr;

static LRESULT WINAPI ImGuiWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ImGui::SetCurrentContext(ImGui::GetCurrentContext());
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    return CallWindowProc(s_OriginalWndProc, hWnd, msg, wParam, lParam);
}

static void subclassWindow(HWND hwnd) {
    s_OriginalWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)ImGuiWndProc);
}

static void unsubclassWindow(HWND hwnd) {
    if (s_OriginalWndProc) {
        SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)s_OriginalWndProc);
        s_OriginalWndProc = nullptr;
    }
}

// ==================== EditorScene ====================

cocos2d::Scene* EditorScene::createScene() {
    return EditorScene::create();
}

bool EditorScene::init() {
    if (!Scene::init()) return false;

    // Background
    cocos2d::LayerColor* bg = cocos2d::LayerColor::create(cocos2d::Color4B(40, 40, 48, 255));
    Node::addChild(bg, -100);

    drawGrid();
    initImGui();
    createTestObject();
    createDemoAnimation();
    scheduleUpdate();

    return true;
}

EditorScene::~EditorScene() {
    shutdownImGui();
}

void EditorScene::drawGrid() {
    cocos2d::DrawNode* grid = cocos2d::DrawNode::create();
    const float gridSize = 50.0f;
    cocos2d::Size vs = cocos2d::Director::getInstance()->getVisibleSize();
    cocos2d::Vec2 origin = cocos2d::Director::getInstance()->getVisibleOrigin();

    for (float x = origin.x; x < origin.x + vs.width; x += gridSize) {
        grid->drawLine(cocos2d::Vec2(x, origin.y), cocos2d::Vec2(x, origin.y + vs.height),
                       cocos2d::Color4F(0.3f, 0.3f, 0.35f, 1.0f));
    }
    for (float y = origin.y; y < origin.y + vs.height; y += gridSize) {
        grid->drawLine(cocos2d::Vec2(origin.x, y), cocos2d::Vec2(origin.x + vs.width, y),
                       cocos2d::Color4F(0.3f, 0.3f, 0.35f, 1.0f));
    }

    cocos2d::Vec2 center = origin + cocos2d::Vec2(vs.width / 2, vs.height / 2);
    grid->drawLine(cocos2d::Vec2(center.x - 20, center.y), cocos2d::Vec2(center.x + 20, center.y),
                   cocos2d::Color4F(0.5f, 0.5f, 0.6f, 1.0f));
    grid->drawLine(cocos2d::Vec2(center.x, center.y - 20), cocos2d::Vec2(center.x, center.y + 20),
                   cocos2d::Color4F(0.5f, 0.5f, 0.6f, 1.0f));

    Node::addChild(grid, -99);
}

// ==================== ImGui Integration ====================

void EditorScene::initImGui() {
    if (_imguiInitialized) return;

    IMGUI_CHECKVERSION();
    _imguiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext((ImGuiContext*)_imguiContext);

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    // Dark theme
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 2.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.14f, 0.95f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.40f, 0.55f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.35f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.40f, 0.55f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.35f, 0.50f, 1.00f);

    // Get HWND
    auto* glview = dynamic_cast<cocos2d::GLViewImpl*>(cocos2d::Director::getInstance()->getOpenGLView());
    if (!glview) return;
    HWND hwnd = glview->getWin32Window();
    if (!hwnd) return;

    ImGui_ImplWin32_InitForOpenGL(hwnd);
    ImGui_ImplOpenGL3_Init("#version 130");

    subclassWindow(hwnd);

    _imguiInitialized = true;
}

void EditorScene::shutdownImGui() {
    if (!_imguiInitialized) return;

    ImGui::SetCurrentContext((ImGuiContext*)_imguiContext);

    auto* glview = dynamic_cast<cocos2d::GLViewImpl*>(cocos2d::Director::getInstance()->getOpenGLView());
    if (glview) {
        HWND hwnd = glview->getWin32Window();
        if (hwnd) unsubclassWindow(hwnd);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext((ImGuiContext*)_imguiContext);
    _imguiContext = nullptr;
    _imguiInitialized = false;
}

// ==================== visit() - inject CustomCommand at end of render pipeline ====================

void EditorScene::visit(cocos2d::Renderer* renderer, const cocos2d::Mat4& parentTransform, uint32_t parentFlags) {
    Scene::visit(renderer, parentTransform, parentFlags);

    // After all cocos2d nodes are rendered, add our ImGui render command
    if (_imguiInitialized) {
        _imguiRenderCmd.init(std::numeric_limits<float>::max());
        _imguiRenderCmd.func = [this]() {
            this->onRenderImGui();
        };
        renderer->addCommand(&_imguiRenderCmd);
    }
}

// ==================== Update - build ImGui UI ====================

void EditorScene::update(float dt) {
    Scene::update(dt);

    if (!_imguiInitialized) return;

    // Update playback
    if (_isPlaying && !_isPaused && _selectedObject && _selectedObject->animator) {
        Animator* anim = _selectedObject->animator;
        if (anim->getState() == Animator::PlayState::Playing) {
            _playbackTime = anim->getCurrentTime();
        } else if (anim->getState() == Animator::PlayState::Stopped) {
            _isPlaying = false;
        }
    }

    // Build ImGui UI (NewFrame + widgets + Render)
    ImGui::SetCurrentContext((ImGuiContext*)_imguiContext);
    ImGui_ImplWin32_NewFrame();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    buildImGuiUI();

    ImGui::Render();
}

// ==================== Actual OpenGL render (called from CustomCommand) ====================

void EditorScene::onRenderImGui() {
    if (!_imguiInitialized) return;
    ImGui::SetCurrentContext((ImGuiContext*)_imguiContext);
    ImDrawData* drawData = ImGui::GetDrawData();
    if (drawData) {
        ImGui_ImplOpenGL3_RenderDrawData(drawData);
    }
}

// ==================== Build all ImGui UI panels ====================

void EditorScene::buildImGuiUI() {
    drawMenuBar();
    drawToolbar();
    drawHierarchyPanel();
    drawPropertiesPanel();
    drawTimeline();
}

// ==================== Menu Bar ====================

void EditorScene::drawMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Animation")) {
                if (_selectedObject && _selectedObject->animator) {
                    _selectedObject->animator->createClip("New Animation");
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                cocos2d::Director::getInstance()->end();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Delete Selected", "Delete")) {
                onDeleteSelected();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Timeline", nullptr, &_showTimeline);
            ImGui::MenuItem("Properties", nullptr, &_showProperties);
            ImGui::MenuItem("Hierarchy", nullptr, &_showHierarchy);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Object")) {
            if (ImGui::MenuItem("Add Test Object")) {
                createTestObject();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

// ==================== Toolbar ====================

void EditorScene::drawToolbar() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y));
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, 36));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                              ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                              ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::Begin("##Toolbar", nullptr, flags);
    ImGui::PopStyleVar();

    if (!_isPlaying) {
        if (ImGui::Button("Play", ImVec2(60, 24))) {
            if (_selectedObject && _selectedObject->animator && _selectedClip) {
                _selectedObject->animator->play(_selectedClip->name);
                _isPlaying = true;
                _isPaused = false;
            }
        }
    } else {
        if (_isPaused) {
            if (ImGui::Button("Resume", ImVec2(60, 24))) {
                if (_selectedObject && _selectedObject->animator) {
                    _selectedObject->animator->resume();
                    _isPaused = false;
                }
            }
        } else {
            if (ImGui::Button("Pause", ImVec2(60, 24))) {
                if (_selectedObject && _selectedObject->animator) {
                    _selectedObject->animator->pause();
                    _isPaused = true;
                }
            }
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Stop", ImVec2(60, 24))) {
        if (_selectedObject && _selectedObject->animator) {
            _selectedObject->animator->stop();
            _isPlaying = false;
            _isPaused = false;
            _playbackTime = 0.0f;
        }
    }

    ImGui::SameLine();
    ImGui::Separator();
    ImGui::SameLine();

    if (_selectedObject && _selectedObject->animator) {
        float speed = _selectedObject->animator->getSpeed();
        ImGui::PushItemWidth(80);
        if (ImGui::DragFloat("Speed", &speed, 0.1f, 0.1f, 5.0f, "%.1fx")) {
            _selectedObject->animator->setSpeed(speed);
        }
        ImGui::PopItemWidth();
    }

    ImGui::SameLine();
    ImGui::Separator();
    ImGui::SameLine();

    if (_selectedObject && _selectedObject->animator) {
        int wrapMode = (int)_selectedObject->animator->getWrapMode();
        const char* wrapModes[] = { "Once", "Loop", "PingPong" };
        ImGui::PushItemWidth(80);
        if (ImGui::Combo("Wrap", &wrapMode, wrapModes, 3)) {
            _selectedObject->animator->setWrapMode((Animator::WrapMode)wrapMode);
        }
        ImGui::PopItemWidth();
    }

    ImGui::SameLine();
    ImGui::Separator();
    ImGui::SameLine();
    ImGui::Text("Time: %.2fs", _playbackTime);

    ImGui::End();
}

// ==================== Hierarchy (left) ====================

void EditorScene::drawHierarchyPanel() {
    if (!_showHierarchy) return;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float toolbarHeight = 36.0f;
    float timelineHeight = _showTimeline ? _timelineHeight : 0.0f;
    float topY = viewport->WorkPos.y + toolbarHeight;
    float availHeight = viewport->WorkSize.y - toolbarHeight - timelineHeight;

    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, topY));
    ImGui::SetNextWindowSize(ImVec2(_panelWidth, availHeight));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove |
                              ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("Hierarchy", &_showHierarchy, flags);

    if (ImGui::Button("+ Add Object")) {
        createTestObject();
    }
    ImGui::Separator();

    for (size_t i = 0; i < _objects.size(); i++) {
        bool selected = (_selectedObject == &_objects[i]);
        ImGui::PushID((int)i);
        if (ImGui::Selectable(_objects[i].name.c_str(), selected)) {
            selectNode(_objects[i].node);
        }
        ImGui::PopID();
    }

    ImGui::End();
}

// ==================== Properties (right) ====================

void EditorScene::drawPropertiesPanel() {
    if (!_showProperties) return;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float toolbarHeight = 36.0f;
    float timelineHeight = _showTimeline ? _timelineHeight : 0.0f;
    float topY = viewport->WorkPos.y + toolbarHeight;
    float availHeight = viewport->WorkSize.y - toolbarHeight - timelineHeight;

    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - _panelWidth, topY));
    ImGui::SetNextWindowSize(ImVec2(_panelWidth, availHeight));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove |
                              ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("Properties", &_showProperties, flags);

    if (!_selectedObject) {
        ImGui::TextDisabled("Select an object to view properties");
        ImGui::End();
        return;
    }

    cocos2d::Node* node = _selectedObject->node;
    if (!node) { ImGui::End(); return; }

    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        float posX = node->getPositionX();
        float posY = node->getPositionY();
        float scaleX = node->getScaleX();
        float scaleY = node->getScaleY();
        float rotation = node->getRotation();
        int opacity = (int)node->getOpacity();

        ImGui::DragFloat("Position X", &posX, 1.0f);
        ImGui::DragFloat("Position Y", &posY, 1.0f);
        ImGui::DragFloat("Scale X", &scaleX, 0.01f);
        ImGui::DragFloat("Scale Y", &scaleY, 0.01f);
        ImGui::DragFloat("Rotation", &rotation, 1.0f, -360.0f, 360.0f);
        ImGui::DragInt("Opacity", &opacity, 1.0f, 0, 255);

        node->setPosition(posX, posY);
        node->setScale(scaleX, scaleY);
        node->setRotation(rotation);
        node->setOpacity((GLubyte)opacity);
    }

    Animator* animator = _selectedObject->animator;
    if (animator && ImGui::CollapsingHeader("Animation Clips")) {
        const auto& clips = animator->getClips();
        for (size_t i = 0; i < clips.size(); i++) {
            bool selected = (_selectedClip == &clips[i]);
            ImGui::PushID((int)i);
            if (ImGui::Selectable(clips[i].name.c_str(), selected)) {
                _selectedClip = animator->getClip(clips[i].name);
                _selectedTrack = nullptr;
                _selectedKeyframeIndex = -1;
            }
            ImGui::PopID();
        }
        if (ImGui::Button("+ Add Clip")) {
            char name[64];
            snprintf(name, sizeof(name), "Clip %d", (int)clips.size() + 1);
            _selectedClip = animator->createClip(name);
            _selectedTrack = nullptr;
            _selectedKeyframeIndex = -1;
        }
    }

    if (_selectedClip && ImGui::CollapsingHeader("Tracks")) {
        for (size_t i = 0; i < _selectedClip->tracks.size(); i++) {
            AnimationTrack& track = _selectedClip->tracks[i];
            bool selected = (_selectedTrack == &track);
            ImGui::PushID((int)i);
            if (ImGui::Selectable(AnimationTrack::propertyShortName(track.property).c_str(), selected)) {
                _selectedTrack = &track;
                _selectedKeyframeIndex = -1;
            }
            ImGui::PopID();
        }
        if (ImGui::Button("+ Add Track")) {
            ImGui::OpenPopup("AddTrackPopup");
        }
        if (ImGui::BeginPopup("AddTrackPopup")) {
            if (ImGui::MenuItem("Position X")) _selectedTrack = _selectedClip->addTrack(AnimProperty::PositionX);
            if (ImGui::MenuItem("Position Y")) _selectedTrack = _selectedClip->addTrack(AnimProperty::PositionY);
            if (ImGui::MenuItem("Scale X"))    _selectedTrack = _selectedClip->addTrack(AnimProperty::ScaleX);
            if (ImGui::MenuItem("Scale Y"))    _selectedTrack = _selectedClip->addTrack(AnimProperty::ScaleY);
            if (ImGui::MenuItem("Rotation"))   _selectedTrack = _selectedClip->addTrack(AnimProperty::Rotation);
            if (ImGui::MenuItem("Opacity"))    _selectedTrack = _selectedClip->addTrack(AnimProperty::Opacity);
            ImGui::EndPopup();
        }
    }

    if (_selectedTrack && ImGui::CollapsingHeader("Keyframes")) {
        for (int i = 0; i < (int)_selectedTrack->keyframes.size(); i++) {
            Keyframe& kf = _selectedTrack->keyframes[i];
            bool selected = (_selectedKeyframeIndex == i);
            ImGui::PushID(i);
            if (ImGui::Selectable("", selected)) {
                _selectedKeyframeIndex = i;
            }
            ImGui::SameLine();
            ImGui::Text("T:%.2f  V:%.2f", kf.time, kf.value);
            ImGui::PopID();
        }

        if (ImGui::Button("+ Add Keyframe")) {
            float time = _playbackTime;
            float value = 0.0f;
            if (_selectedObject && _selectedObject->node) {
                cocos2d::Node* n = _selectedObject->node;
                switch (_selectedTrack->property) {
                    case AnimProperty::PositionX: value = n->getPositionX(); break;
                    case AnimProperty::PositionY: value = n->getPositionY(); break;
                    case AnimProperty::ScaleX:    value = n->getScaleX(); break;
                    case AnimProperty::ScaleY:    value = n->getScaleY(); break;
                    case AnimProperty::Rotation:  value = n->getRotation(); break;
                    case AnimProperty::Opacity:   value = (float)n->getOpacity(); break;
                }
            }
            _selectedTrack->addKeyframe(time, value);
            _selectedKeyframeIndex = (int)_selectedTrack->keyframes.size() - 1;
        }

        ImGui::SameLine();
        if (ImGui::Button("- Remove Keyframe") && _selectedKeyframeIndex >= 0) {
            _selectedTrack->removeKeyframe(_selectedKeyframeIndex);
            _selectedKeyframeIndex = -1;
        }

        if (_selectedKeyframeIndex >= 0 && _selectedKeyframeIndex < (int)_selectedTrack->keyframes.size()) {
            Keyframe& kf = _selectedTrack->keyframes[_selectedKeyframeIndex];
            ImGui::Separator();
            ImGui::Text("Edit Keyframe #%d", _selectedKeyframeIndex);
            ImGui::DragFloat("Time", &kf.time, 0.01f, 0.0f, 100.0f);
            ImGui::DragFloat("Value", &kf.value, 0.1f);

            const char* easeTypes[] = { "Linear", "EaseIn", "EaseOut", "EaseInOut", "BounceIn", "BounceOut", "ElasticIn", "ElasticOut" };
            int easeIdx = (int)kf.ease;
            if (ImGui::Combo("Ease", &easeIdx, easeTypes, 8)) {
                kf.ease = (EaseType)easeIdx;
            }
        }
    }

    ImGui::End();
}

// ==================== Timeline (bottom) ====================

void EditorScene::drawTimeline() {
    if (!_showTimeline) return;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float hierarchyWidth = _showHierarchy ? _panelWidth : 0;
    float propertiesWidth = _showProperties ? _panelWidth : 0;

    float timelineX = viewport->WorkPos.x + hierarchyWidth;
    float timelineY = viewport->WorkPos.y + viewport->WorkSize.y - _timelineHeight;
    float timelineW = viewport->WorkSize.x - hierarchyWidth - propertiesWidth;

    ImGui::SetNextWindowPos(ImVec2(timelineX, timelineY));
    ImGui::SetNextWindowSize(ImVec2(timelineW, _timelineHeight));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove |
                              ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                              ImGuiWindowFlags_NoScrollbar;
    ImGui::Begin("Timeline", &_showTimeline, flags);

    if (!_selectedClip) {
        ImGui::TextDisabled("Select an animation clip to view timeline");
        ImGui::End();
        return;
    }

    float rulerHeight = 24;
    float trackLabelWidth = 80;
    float trackAreaWidth = ImGui::GetContentRegionAvail().x - trackLabelWidth;
    if (trackAreaWidth < 10) trackAreaWidth = 10;
    float trackHeight = 28;

    ImGui::BeginChild("TimelineRuler", ImVec2(trackAreaWidth, rulerHeight), false);
    ImVec2 rulerPos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    float pixelsPerSecond = _timelineZoom;
    float startSecond = (int)(_timelineScroll / pixelsPerSecond);
    float endSecond = startSecond + trackAreaWidth / pixelsPerSecond + 1;

    for (float s = startSecond; s <= endSecond; s += 0.5f) {
        float x = s * pixelsPerSecond - _timelineScroll;
        if (x < 0 || x > trackAreaWidth) continue;
        bool isMajor = (fmod(s, 1.0f) < 0.01f);
        float tickHeight = isMajor ? rulerHeight : rulerHeight * 0.5f;
        drawList->AddLine(ImVec2(rulerPos.x + x, rulerPos.y),
                          ImVec2(rulerPos.x + x, rulerPos.y + tickHeight),
                          isMajor ? IM_COL32(180, 180, 180, 200) : IM_COL32(120, 120, 120, 150));
        if (isMajor) {
            char label[16];
            snprintf(label, sizeof(label), "%.0f", s);
            drawList->AddText(ImVec2(rulerPos.x + x + 3, rulerPos.y + 2), IM_COL32(200, 200, 200, 255), label);
        }
    }

    float playheadX = _playbackTime * pixelsPerSecond - _timelineScroll;
    if (playheadX >= 0 && playheadX <= trackAreaWidth) {
        drawList->AddLine(ImVec2(rulerPos.x + playheadX, rulerPos.y),
                          ImVec2(rulerPos.x + playheadX, rulerPos.y + rulerHeight),
                          IM_COL32(255, 80, 80, 255), 2.0f);
    }

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
        float mouseX = ImGui::GetMousePos().x - rulerPos.x;
        _playbackTime = (mouseX + _timelineScroll) / pixelsPerSecond;
        _playbackTime = std::max(0.0f, _playbackTime);
        if (_selectedObject && _selectedObject->animator && _selectedClip) {
            auto sv = _selectedClip->sample(_playbackTime);
            _selectedObject->node->setPosition(sv.posX, sv.posY);
            _selectedObject->node->setScaleX(sv.scaleX);
            _selectedObject->node->setScaleY(sv.scaleY);
            _selectedObject->node->setRotation(sv.rotation);
            _selectedObject->node->setOpacity((GLubyte)sv.opacity);
        }
    }

    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("TrackLabels", ImVec2(trackLabelWidth, 0), false);
    for (const auto& track : _selectedClip->tracks) {
        bool selected = (_selectedTrack == &track);
        if (selected) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
        ImGui::Text("%s", AnimationTrack::propertyShortName(track.property).c_str());
        if (selected) ImGui::PopStyleColor();
    }
    ImGui::EndChild();

    ImGui::BeginChild("TrackKeys", ImVec2(0, 0), false);

    ImDrawList* trackDrawList = ImGui::GetWindowDrawList();
    ImVec2 trackAreaPos = ImGui::GetCursorScreenPos();
    float totalTrackHeight = (float)_selectedClip->tracks.size() * trackHeight;

    for (size_t i = 0; i < _selectedClip->tracks.size(); i++) {
        float y = (float)i * trackHeight;
        bool selected = (_selectedTrack == &_selectedClip->tracks[i]);
        ImU32 bgColor = selected ? IM_COL32(50, 60, 80, 200) : IM_COL32(35, 38, 45, 200);
        trackDrawList->AddRectFilled(
            ImVec2(trackAreaPos.x, trackAreaPos.y + y),
            ImVec2(trackAreaPos.x + trackAreaWidth, trackAreaPos.y + y + trackHeight), bgColor);
        trackDrawList->AddLine(
            ImVec2(trackAreaPos.x, trackAreaPos.y + y + trackHeight),
            ImVec2(trackAreaPos.x + trackAreaWidth, trackAreaPos.y + y + trackHeight),
            IM_COL32(60, 65, 75, 200));
    }

    for (size_t i = 0; i < _selectedClip->tracks.size(); i++) {
        const AnimationTrack& track = _selectedClip->tracks[i];
        float cy = trackAreaPos.y + (float)i * trackHeight + trackHeight * 0.5f;

        for (size_t j = 0; j < track.keyframes.size(); j++) {
            const Keyframe& kf = track.keyframes[j];
            float kx = trackAreaPos.x + kf.time * pixelsPerSecond - _timelineScroll;
            if (kx < trackAreaPos.x - 10 || kx > trackAreaPos.x + trackAreaWidth + 10) continue;

            bool selected = (_selectedTrack == &track && _selectedKeyframeIndex == (int)j);
            ImU32 color = selected ? IM_COL32(100, 200, 255, 255) : IM_COL32(220, 180, 60, 255);
            float size = selected ? 7.0f : 5.0f;
            trackDrawList->AddQuadFilled(
                ImVec2(kx, cy - size), ImVec2(kx + size, cy),
                ImVec2(kx, cy + size), ImVec2(kx - size, cy), color);
        }

        if (track.keyframes.size() >= 2) {
            for (size_t j = 0; j < track.keyframes.size() - 1; j++) {
                float x1 = trackAreaPos.x + track.keyframes[j].time * pixelsPerSecond - _timelineScroll;
                float x2 = trackAreaPos.x + track.keyframes[j + 1].time * pixelsPerSecond - _timelineScroll;
                if (x2 < trackAreaPos.x || x1 > trackAreaPos.x + trackAreaWidth) continue;
                x1 = std::max(x1, trackAreaPos.x);
                x2 = std::min(x2, trackAreaPos.x + trackAreaWidth);
                trackDrawList->AddLine(ImVec2(x1, cy), ImVec2(x2, cy), IM_COL32(100, 100, 120, 150), 1.0f);
            }
        }
    }

    if (playheadX >= 0 && playheadX <= trackAreaWidth) {
        trackDrawList->AddLine(
            ImVec2(trackAreaPos.x + playheadX, trackAreaPos.y),
            ImVec2(trackAreaPos.x + playheadX, trackAreaPos.y + totalTrackHeight),
            IM_COL32(255, 80, 80, 255), 2.0f);
    }

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
        ImVec2 mousePos = ImGui::GetMousePos();
        float mx = mousePos.x - trackAreaPos.x + _timelineScroll;
        float my = mousePos.y - trackAreaPos.y;
        int trackIdx = (int)(my / trackHeight);
        if (trackIdx >= 0 && trackIdx < (int)_selectedClip->tracks.size()) {
            _selectedTrack = &_selectedClip->tracks[trackIdx];
            float time = mx / pixelsPerSecond;
            int closestIdx = -1;
            float closestDist = 10.0f / pixelsPerSecond;
            for (int j = 0; j < (int)_selectedTrack->keyframes.size(); j++) {
                float dist = fabs(_selectedTrack->keyframes[j].time - time);
                if (dist < closestDist) { closestDist = dist; closestIdx = j; }
            }
            _selectedKeyframeIndex = closestIdx;
        }
    }

    if (ImGui::IsWindowHovered()) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0) {
            if (ImGui::GetIO().KeyCtrl) {
                _timelineZoom *= (1.0f + wheel * 0.1f);
                _timelineZoom = std::max(10.0f, std::min(500.0f, _timelineZoom));
            } else {
                _timelineScroll -= wheel * 30.0f;
                _timelineScroll = std::max(0.0f, _timelineScroll);
            }
        }
    }

    ImGui::EndChild();
    ImGui::End();
}

// ==================== Editor Actions ====================

void EditorScene::createTestObject() {
    cocos2d::Size vs = cocos2d::Director::getInstance()->getVisibleSize();
    cocos2d::Vec2 origin = cocos2d::Director::getInstance()->getVisibleOrigin();
    cocos2d::Vec2 center = origin + cocos2d::Vec2(vs.width / 2, vs.height / 2);

    cocos2d::DrawNode* drawNode = cocos2d::DrawNode::create();
    float size = 80.0f;
    cocos2d::Vec2 bl(-size / 2, -size / 2);
    cocos2d::Vec2 tr(size / 2, size / 2);
    drawNode->drawRect(bl, tr, cocos2d::Color4F(0.3f, 0.6f, 1.0f, 1.0f));
    drawNode->drawLine(bl, cocos2d::Vec2(tr.x, bl.y), cocos2d::Color4F(0.5f, 0.8f, 1.0f, 1.0f));
    drawNode->drawLine(cocos2d::Vec2(tr.x, bl.y), tr, cocos2d::Color4F(0.5f, 0.8f, 1.0f, 1.0f));
    drawNode->drawLine(tr, cocos2d::Vec2(bl.x, tr.y), cocos2d::Color4F(0.5f, 0.8f, 1.0f, 1.0f));
    drawNode->drawLine(cocos2d::Vec2(bl.x, tr.y), bl, cocos2d::Color4F(0.5f, 0.8f, 1.0f, 1.0f));
    drawNode->setPosition(center);
    Node::addChild(drawNode);

    Animator* animator = Animator::create();
    drawNode->addComponent(animator);

    EditorObject obj;
    obj.name = "Object_" + std::to_string(_objects.size() + 1);
    obj.node = drawNode;
    obj.animator = animator;
    _objects.push_back(obj);

    selectNode(drawNode);
}

void EditorScene::createDemoAnimation() {
    if (_objects.empty()) return;
    EditorObject& obj = _objects.back();
    if (!obj.animator) return;

    AnimationClip* clip = obj.animator->createClip("Demo Animation");

    AnimationTrack* trackX = clip->addTrack(AnimProperty::PositionX);
    cocos2d::Vec2 pos = obj.node->getPosition();
    trackX->addKeyframe(0.0f, pos.x);
    trackX->addKeyframe(1.0f, pos.x + 150.0f, EaseType::EaseInOut);
    trackX->addKeyframe(2.0f, pos.x, EaseType::EaseInOut);

    AnimationTrack* trackY = clip->addTrack(AnimProperty::PositionY);
    trackY->addKeyframe(0.0f, pos.y);
    trackY->addKeyframe(1.0f, pos.y + 100.0f, EaseType::EaseOut);
    trackY->addKeyframe(2.0f, pos.y, EaseType::EaseIn);

    AnimationTrack* trackRot = clip->addTrack(AnimProperty::Rotation);
    trackRot->addKeyframe(0.0f, 0.0f);
    trackRot->addKeyframe(2.0f, 360.0f, EaseType::Linear);

    AnimationTrack* trackScaleX = clip->addTrack(AnimProperty::ScaleX);
    trackScaleX->addKeyframe(0.0f, 1.0f);
    trackScaleX->addKeyframe(0.5f, 1.5f, EaseType::EaseOut);
    trackScaleX->addKeyframe(1.5f, 1.5f);
    trackScaleX->addKeyframe(2.0f, 1.0f, EaseType::EaseIn);

    AnimationTrack* trackScaleY = clip->addTrack(AnimProperty::ScaleY);
    trackScaleY->addKeyframe(0.0f, 1.0f);
    trackScaleY->addKeyframe(0.5f, 1.5f, EaseType::EaseOut);
    trackScaleY->addKeyframe(1.5f, 1.5f);
    trackScaleY->addKeyframe(2.0f, 1.0f, EaseType::EaseIn);

    _selectedClip = clip;
    _selectedTrack = nullptr;
    _selectedKeyframeIndex = -1;
}

void EditorScene::onDeleteSelected() {
    if (!_selectedObject) return;
    if (_selectedObject->node) removeChild(_selectedObject->node);
    for (auto it = _objects.begin(); it != _objects.end(); ++it) {
        if (&(*it) == _selectedObject) { _objects.erase(it); break; }
    }
    _selectedObject = nullptr;
    _selectedClip = nullptr;
    _selectedTrack = nullptr;
    _selectedKeyframeIndex = -1;
}

void EditorScene::selectNode(cocos2d::Node* node) {
    _selectedObject = nullptr;
    for (auto& obj : _objects) {
        if (obj.node == node) { _selectedObject = &obj; break; }
    }
    if (_selectedObject && _selectedObject->animator) {
        const auto& clips = _selectedObject->animator->getClips();
        _selectedClip = !clips.empty() ? _selectedObject->animator->getClip(clips[0].name) : nullptr;
    } else {
        _selectedClip = nullptr;
    }
    _selectedTrack = nullptr;
    _selectedKeyframeIndex = -1;
}

} // namespace editor
