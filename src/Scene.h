#pragma once

#include <entt.hpp>
#include <imgui.h>
#include <json.hpp>

struct Position {
    float x, y;
};

struct Velocity {
    float dx, dy;
};

struct EntityData {
    float x, y, dx, dy;
};

struct Scene {

    Scene();
    ~Scene();

private:
    entt::registry m_Registry;
};
