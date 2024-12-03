#include "Scene.h"
#include "glm/fwd.hpp"

#include <glm/glm.hpp>

Scene::Scene()
{
  
    struct TransformComponent
    {
        glm::mat4 Transform;

        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(const glm::mat4& transform) : Transform(transform){}

        operator const glm::mat4&() const { return Transform; }
        operator glm::mat4&() { return Transform; }
    };
    
    entt::entity entity = this->m_Registry.create();
    this->m_Registry.emplace<TransformComponent>(entity, glm::mat4(1.0f));
}

Scene::~Scene()
{

}
