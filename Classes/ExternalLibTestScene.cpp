#include "ExternalLibTestScene.h"

// EnTT
#include "entt/entt.hpp"

// behaviac
#include "behaviac/behaviac.h"

// ImGui
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_opengl3.h"

USING_NS_CC;

// ============================================================================
// Helper: add a label to the scene
// ============================================================================
static int g_testYOffset = 0;

static void addTestLabel(cocos2d::Node* parent, const std::string& text,
                          const cocos2d::Vec2& origin, float x, float& y,
                          const cocos2d::Color3B& color = cocos2d::Color3B::WHITE)
{
    auto label = Label::createWithTTF(text, "fonts/Marker Felt.ttf", 16);
    if (label)
    {
        label->setPosition(Vec2(origin.x + x, origin.y + y));
        label->setColor(color);
        parent->addChild(label);
        y -= 22;
    }
}

// ============================================================================
// EnTT Test
// ============================================================================
struct Position { float x, y; };
struct Velocity { float dx, dy; };
struct Health   { int hp; };

void ExternalLibTestScene::testEnTT()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    entt::registry registry;

    // Create entities with components
    auto e1 = registry.create();
    registry.emplace<Position>(e1, 100.0f, 200.0f);
    registry.emplace<Velocity>(e1, 1.5f, -0.5f);
    registry.emplace<Health>(e1, 100);

    auto e2 = registry.create();
    registry.emplace<Position>(e2, 300.0f, 400.0f);
    registry.emplace<Velocity>(e2, -1.0f, 2.0f);
    registry.emplace<Health>(e2, 80);

    auto e3 = registry.create();
    registry.emplace<Position>(e3, 500.0f, 100.0f);
    registry.emplace<Health>(e3, 50);

    // Iterate entities with Position + Velocity
    int movedCount = 0;
    auto view = registry.view<Position, Velocity>();
    for (auto entity : view)
    {
        auto& pos = view.get<Position>(entity);
        auto& vel = view.get<Velocity>(entity);
        pos.x += vel.dx;
        pos.y += vel.dy;
        movedCount++;
    }

    // Iterate entities with Health
    int healthCount = 0;
    int totalHp = 0;
    auto healthView = registry.view<Health>();
    for (auto entity : healthView)
    {
        auto& hp = healthView.get<Health>(entity);
        totalHp += hp.hp;
        healthCount++;
    }

    // Verify
    bool pass = true;
    if (movedCount != 2) pass = false;
    if (healthCount != 3) pass = false;
    if (totalHp != 230) pass = false;

    auto& pos1 = registry.get<Position>(e1);
    if (pos1.x != 101.5f || pos1.y != 199.5f) pass = false;

    float y = origin.y + visibleSize.height - 60;
    float x = 20;

    addTestLabel(this, "====== EnTT (ECS) Test ======", origin, x, y, Color3B::YELLOW);
    addTestLabel(this, "Created 3 entities with components", origin, x, y);
    addTestLabel(this, StringUtils::format("  Entities with Position+Velocity: %d (expect 2)", movedCount), origin, x, y);
    addTestLabel(this, StringUtils::format("  Entities with Health: %d (expect 3)", healthCount), origin, x, y);
    addTestLabel(this, StringUtils::format("  Total HP: %d (expect 230)", totalHp), origin, x, y);
    addTestLabel(this, StringUtils::format("  Entity1 pos after update: (%.1f, %.1f) (expect 101.5, 199.5)", pos1.x, pos1.y), origin, x, y);

    if (pass)
        addTestLabel(this, "  [PASS] EnTT test passed!", origin, x, y, Color3B::GREEN);
    else
        addTestLabel(this, "  [FAIL] EnTT test failed!", origin, x, y, Color3B::RED);

    g_testYOffset = y;
}

// ============================================================================
// behaviac Test
// ============================================================================

class TestAgent : public behaviac::Agent
{
    BEHAVIAC_DECLARE_AGENTTYPE(TestAgent, behaviac::Agent);

public:
    TestAgent() : behaviac::Agent(), testProperty(0), testAction1Called(false) {}

    int testProperty;
    bool testAction1Called;

    void testAction1() { testAction1Called = true; }
    bool testCondition1() { return true; }
};

void ExternalLibTestScene::testBehaviac()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    float y = g_testYOffset - 10;
    float x = 20;

    addTestLabel(this, "====== behaviac (Behavior Tree) Test ======", origin, x, y, Color3B::YELLOW);

    bool pass = true;

    // Test 1: Create agent using the template API
    TestAgent* agent = (TestAgent*)behaviac::Agent::Create<TestAgent>("TestAgent");
    if (agent)
    {
        agent->testProperty = 42;
        agent->testAction1Called = false;
        int readBack = agent->testProperty;

        addTestLabel(this, "  Agent created successfully", origin, x, y);
        addTestLabel(this, StringUtils::format("  Property set/read: %d (expect 42)", readBack), origin, x, y);

        if (readBack != 42) pass = false;

        // Test 2: Workspace init
        behaviac::Workspace::GetInstance()->SetFilePath("../Resources/");
        addTestLabel(this, "  Workspace initialized", origin, x, y);

        // Test 3: Agent method call
        agent->testAction1();
        if (agent->testAction1Called)
            addTestLabel(this, "  Agent action method called OK", origin, x, y);
        else
        {
            addTestLabel(this, "  Agent action method call FAILED", origin, x, y, Color3B::RED);
            pass = false;
        }

        // Test 4: Condition method
        if (agent->testCondition1())
            addTestLabel(this, "  Agent condition method returned true OK", origin, x, y);
        else
        {
            addTestLabel(this, "  Agent condition method FAILED", origin, x, y, Color3B::RED);
            pass = false;
        }

        behaviac::Agent::Destroy(agent);
    }
    else
    {
        addTestLabel(this, "  [FAIL] Failed to create TestAgent!", origin, x, y, Color3B::RED);
        pass = false;
    }

    if (pass)
        addTestLabel(this, "  [PASS] behaviac test passed!", origin, x, y, Color3B::GREEN);
    else
        addTestLabel(this, "  [FAIL] behaviac test failed!", origin, x, y, Color3B::RED);

    g_testYOffset = y;
}

// ============================================================================
// ImGui Test
// ============================================================================
void ExternalLibTestScene::testImGui()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    float y = g_testYOffset - 10;
    float x = 20;

    addTestLabel(this, "====== ImGui Test ======", origin, x, y, Color3B::YELLOW);

    bool pass = true;

    // Test 1: ImGui version
    const char* version = ImGui::GetVersion();
    addTestLabel(this, StringUtils::format("  ImGui version: %s", version), origin, x, y);

    // Test 2: Create ImGui context (without rendering to avoid font texture issues)
    ImGuiContext* prevContext = ImGui::GetCurrentContext();
    ImGuiContext* newContext = ImGui::CreateContext();
    if (newContext)
    {
        ImGui::SetCurrentContext(newContext);
        addTestLabel(this, "  ImGui context created OK", origin, x, y);

        // Test 3: ImGui IO
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)visibleSize.width, (float)visibleSize.height);
        addTestLabel(this, StringUtils::format("  DisplaySize set: (%.0f, %.0f)", io.DisplaySize.x, io.DisplaySize.y), origin, x, y);

        // Test 4: Style
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 5.0f;
        addTestLabel(this, "  ImGui style modified OK", origin, x, y);

        // Note: Skip NewFrame/Render to avoid font texture assertion
        // These require proper initialization with ImGui_ImplOpenGL3_NewFrame etc.

        // Restore previous context
        ImGui::DestroyContext(newContext);
        if (prevContext)
            ImGui::SetCurrentContext(prevContext);
        addTestLabel(this, "  ImGui context destroyed OK", origin, x, y);
    }
    else
    {
        addTestLabel(this, "  [FAIL] Failed to create ImGui context!", origin, x, y, Color3B::RED);
        pass = false;
    }

    if (pass)
        addTestLabel(this, "  [PASS] ImGui test passed!", origin, x, y, Color3B::GREEN);
    else
        addTestLabel(this, "  [FAIL] ImGui test failed!", origin, x, y, Color3B::RED);

    g_testYOffset = y;
}

// ============================================================================
// Scene entry
// ============================================================================
Scene* ExternalLibTestScene::createScene()
{
    return ExternalLibTestScene::create();
}

bool ExternalLibTestScene::init()
{
    if (!Scene::init())
        return false;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // Title
    auto title = Label::createWithTTF("External Libraries Test", "fonts/Marker Felt.ttf", 28);
    if (title)
    {
        title->setPosition(Vec2(origin.x + visibleSize.width / 2,
                                origin.y + visibleSize.height - 20));
        title->setColor(Color3B(0, 255, 255)); // CYAN
        this->addChild(title);
    }

    // Run all tests
    testEnTT();
    testBehaviac();
    testImGui();

    return true;
}
