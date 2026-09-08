#include "Core/PlayerOptionsRules.h"
#include "Core/UIFlowRules.h"
#include <iostream>
#include <limits>
using namespace coastal;
static int checks = 0, failures = 0;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; std::cerr << "line " << __LINE__ << ": " << #x << '\n'; } } while (false)
static bool Near(double a, double b) { return std::abs(a - b) < 1e-9; }
int main()
{
    const PlayerOptions defaults;
    CHECK(ValidOptions(defaults)); CHECK(defaults.fieldOfView == 85); CHECK(defaults.textPercent == 100);
    for (OptionField field : {OptionField::Mouse, OptionField::Stick, OptionField::FieldOfView, OptionField::Text})
    {
        auto p = defaults;
        for (int i = 0; i < 20; ++i) { AdjustOption(p, field, 1); CHECK(ValidOptions(p)); }
        CHECK(!AdjustOption(p, field, 1));
        for (int i = 0; i < 20; ++i) { AdjustOption(p, field, -1); CHECK(ValidOptions(p)); }
        CHECK(!AdjustOption(p, field, -1));
    }
    auto p = defaults;
    CHECK(AdjustOption(p, OptionField::InvertY, 1)); CHECK(p.invertY);
    CHECK(AdjustOption(p, OptionField::InvertY, -1)); CHECK(p == defaults);
    CHECK(!AdjustOption(p, OptionField::Text, 0)); CHECK(!AdjustOption(p, OptionField::Text, 2147483647));
    CHECK(!AdjustOption(p, static_cast<OptionField>(99), 1)); CHECK(p == defaults);
    for (int bad : {-2147483647, -1, 0, 24, 26, 299, 301, 2147483647})
    { p = defaults; p.mousePercent = bad; CHECK(!ValidOptions(p)); CHECK(!AdjustOption(p, OptionField::Text, 1)); }
    p = defaults; p.stickPercent = 99; CHECK(!ValidOptions(p));
    p = defaults; p.fieldOfView = 86; CHECK(!ValidOptions(p));
    p = defaults; p.textPercent = 151; CHECK(!ValidOptions(p));
    for (int percent = 100; percent <= 150; percent += 10)
    {
        CHECK(OptionFontSize(20, percent) >= 20);
        CHECK(OptionFontSize(20, percent) <= 30);
    }
    CHECK(OptionFontSize(19, 150) == 29); CHECK(OptionFontSize(0, 150) == 0);
    CHECK(OptionFontSize(20, 99) == 20); CHECK(OptionFontSize(20, 151) == 20);
    p = defaults; p.mousePercent = 200; p.stickPercent = 150;
    CHECK(Near(MouseLook(p, 2, -3).yaw, 4)); CHECK(Near(MouseLook(p, 2, -3).pitch, -6));
    p.invertY = true; CHECK(Near(MouseLook(p, 2, -3).pitch, 6));
    CHECK(Near(StickLook(p, 1, -1, 1.0 / 60).yaw, 2.25));
    CHECK(Near(StickLook(p, 1, -1, 1.0 / 60).pitch, 2.25));
    for (int fps : {15, 30, 60, 120, 240})
    {
        double sum = 0;
        for (int i = 0; i < fps; ++i) sum += StickLook(p, 1, 0, 1.0 / fps).yaw;
        CHECK(Near(sum, 135.0));
        double mouse = 0;
        for (int i = 0; i < fps; ++i) mouse += MouseLook(p, 60.0 / fps, 0).yaw;
        CHECK(Near(mouse, 120.0));
    }
    CHECK(Near(StickLook(p, 2, 0, 10).yaw, 13.5)); // clamp axis and hitch contribution
    for (double bad : {-1.0, 0.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()})
        CHECK(StickLook(p, 1, 1, bad).yaw == 0);
    CHECK(MouseLook(p, std::numeric_limits<double>::infinity(), 1).yaw == 0);
    CHECK(MouseLook(p, 10001, 1).pitch == 0); CHECK(StickLook(p, 0, std::numeric_limits<double>::quiet_NaN(), .1).yaw == 0);
    LookInputGate gate;
    CHECK(!gate.Mouse(0)); CHECK(!gate.Stick(0, 0, 0));
    gate.Synchronize(true, 1, 0, 10);
    CHECK(!gate.Mouse(10)); CHECK(gate.Mouse(11)); CHECK(!gate.Mouse(11)); CHECK(!gate.Mouse(9));
    CHECK(!gate.Stick(1, 0, 11)); CHECK(!gate.Stick(0, 0, 12)); CHECK(!gate.Stick(1, 0, 12));
    CHECK(gate.Stick(1, 0, 13)); CHECK(!gate.Stick(1, 0, 13));
    gate.Synchronize(false, 1, 1, 14); CHECK(!gate.Mouse(15)); CHECK(!gate.Stick(0, 0, 15));
    gate.Synchronize(true, 1, 2, 16); CHECK(!gate.Stick(1, 0, 17)); CHECK(!gate.Stick(0, 0, 18)); CHECK(gate.Stick(1, 0, 19));
    // A complete menu round trip between ticks is still detected by permission revision.
    gate.Synchronize(true, 1, 4, 20); CHECK(!gate.Mouse(20)); CHECK(!gate.Stick(1, 0, 21));
    CHECK(!gate.Stick(.001, 0, 22)); CHECK(!gate.Stick(0, 0, 23)); CHECK(gate.Stick(0, 1, 24));
    gate.Synchronize(true, 2, 4, 25); CHECK(!gate.Stick(0, 1, 26));
    CHECK(!gate.Stick(std::numeric_limits<double>::quiet_NaN(), 0, 27));
    CHECK(!gate.Stick(0, 0, 28)); CHECK(gate.Stick(1, 0, 29));
    CHECK(VisibleUIIntersection({0, 20, 400, 900}, {0, 0, 400, 500}));
    CHECK(!VisibleUIIntersection({0, 900, 400, 1000}, {0, 0, 400, 500}));
    CHECK(!VisibleUIIntersection({0, 0, 400, 20}, {0, 20, 400, 500}));
    CHECK(!VisibleUIIntersection({0, 0, 0, 20}, {0, 0, 400, 500}));
    CHECK(!VisibleUIIntersection({400, 0, 500, 500}, {0, 0, 400, 500}));
    CHECK(!VisibleUIIntersection({0, 0, std::numeric_limits<double>::infinity(), 100}, {0, 0, 400, 500}));
    UIFlow ui; ui.Reset(1); auto parent = ui.Push(PanelKind::Session, 1);
    ui.RememberFocus(parent, 4); auto settings = ui.Push(PanelKind::Settings, 2);
    CHECK(!ui.ClaimCommand(parent, 3)); CHECK(!ui.ClaimCommand(settings, 2));
    CHECK(ui.ClaimCommand(settings, 3)); CHECK(!ui.ClaimCommand(settings, 3));
    CHECK(ui.Pop(settings, false)); CHECK(ui.Top()->focus == 4); CHECK(!ui.Pop(parent, false));
    auto stale = ui.Push(PanelKind::Settings, 4); ui.Reset(2);
    CHECK(!ui.ClaimCommand(stale, 5)); ui.RequireRecovery(6); CHECK(ui.Push(PanelKind::Settings, 7).id == 0);
    std::cout << "Player options/look core: " << checks << " checks; " << failures << " failures\n";
    return failures ? 1 : 0;
}
