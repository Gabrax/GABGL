#include "ParticleRenderer.h"

#include "Camera.h"
#include "RandomGen.hpp"
#include "Shader.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <vector>

#include <glad/glad.h>
#include <glm/ext/scalar_constants.hpp>
#include <glm/glm.hpp>

namespace
{
  struct Particle
  {
    glm::vec3 Position{0.0f};
    glm::vec3 Velocity{0.0f};
    glm::vec3 Acceleration{0.0f};
    glm::vec4 ColorStart{1.0f};
    glm::vec4 ColorEnd{1.0f};
    float Rotation = 0.0f;
    float AngularVelocity = 0.0f;
    float SizeBegin = 1.0f;
    float SizeEnd = 0.0f;
    float ConeAngle = 0.0f;
    float LifeTime = 1.0f;
    float LifeRemaining = 0.0f;
  };

  struct ParticleInstance
  {
    glm::vec4 PositionAndSize;
    glm::vec4 Color;
    float Rotation;
  };

  struct ParticleRendererData
  {
    static constexpr size_t MaxParticles = 512;
    static constexpr float ParticlesPerSecond = 60.0f;

    GLuint VertexArray = 0;
    GLuint QuadVertexBuffer = 0;
    GLuint InstanceBuffer = 0;
    std::shared_ptr<Shader> ParticleShader;

    Particle Prototype;
    std::vector<Particle> Pool;
    std::vector<ParticleInstance> Instances;
    size_t ActiveCount = 0;
    float EmissionAccumulator = 0.0f;
  } s_Data;

  glm::vec3 RandomDirectionInCone(const glm::vec3& direction, float coneAngle)
  {
    const float u = RandomGen::Float();
    const float v = RandomGen::Float();
    const float cosTheta = glm::mix(std::cos(coneAngle), 1.0f, u);
    const float sinTheta = std::sqrt(std::max(0.0f, 1.0f - cosTheta * cosTheta));
    const float phi = 2.0f * glm::pi<float>() * v;

    const glm::vec3 localDirection(
      std::cos(phi) * sinTheta,
      std::sin(phi) * sinTheta,
      cosTheta);

    const glm::vec3 up = std::abs(direction.z) < 0.999f
      ? glm::vec3(0.0f, 0.0f, 1.0f)
      : glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 tangent = glm::normalize(glm::cross(up, direction));
    const glm::vec3 bitangent = glm::cross(direction, tangent);

    return tangent * localDirection.x + bitangent * localDirection.y + direction * localDirection.z;
  }

  void AdvanceParticle(Particle& particle, float delta)
  {
    particle.Position += particle.Velocity * delta + 0.5f * particle.Acceleration * delta * delta;
    particle.Velocity += particle.Acceleration * delta;
    particle.Rotation += particle.AngularVelocity * delta;
    particle.LifeRemaining -= delta;
  }

  void RenderInstances()
  {
    if (s_Data.Instances.empty())
      return;

    glNamedBufferSubData(
      s_Data.InstanceBuffer,
      0,
      static_cast<GLsizeiptr>(s_Data.Instances.size() * sizeof(ParticleInstance)),
      s_Data.Instances.data());

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    s_Data.ParticleShader->Bind();
    s_Data.ParticleShader->SetVec3("u_CameraRight", Camera::GetRightDirection());
    s_Data.ParticleShader->SetVec3("u_CameraUp", Camera::GetUpDirection());

    glBindVertexArray(s_Data.VertexArray);
    glDrawArraysInstanced(
      GL_TRIANGLE_STRIP,
      0,
      4,
      static_cast<GLsizei>(s_Data.Instances.size()));
    glBindVertexArray(0);

    s_Data.ParticleShader->UnBind();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
  }
}

void ParticleRenderer::Init()
{
  RandomGen::Init();
  Shader::Create(s_Data.ParticleShader, "../res/shaders/particle.glsl");

  constexpr glm::vec2 quadVertices[4] = {
    {-0.5f, -0.5f},
    { 0.5f, -0.5f},
    {-0.5f,  0.5f},
    { 0.5f,  0.5f}
  };

  glCreateVertexArrays(1, &s_Data.VertexArray);
  glCreateBuffers(1, &s_Data.QuadVertexBuffer);
  glCreateBuffers(1, &s_Data.InstanceBuffer);

  glNamedBufferData(s_Data.QuadVertexBuffer, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
  glNamedBufferData(
    s_Data.InstanceBuffer,
    static_cast<GLsizeiptr>(ParticleRendererData::MaxParticles * sizeof(ParticleInstance)),
    nullptr,
    GL_DYNAMIC_DRAW);

  glVertexArrayVertexBuffer(s_Data.VertexArray, 0, s_Data.QuadVertexBuffer, 0, sizeof(glm::vec2));
  glEnableVertexArrayAttrib(s_Data.VertexArray, 0);
  glVertexArrayAttribFormat(s_Data.VertexArray, 0, 2, GL_FLOAT, GL_FALSE, 0);
  glVertexArrayAttribBinding(s_Data.VertexArray, 0, 0);

  glVertexArrayVertexBuffer(s_Data.VertexArray, 1, s_Data.InstanceBuffer, 0, sizeof(ParticleInstance));
  glVertexArrayBindingDivisor(s_Data.VertexArray, 1, 1);

  glEnableVertexArrayAttrib(s_Data.VertexArray, 1);
  glVertexArrayAttribFormat(
    s_Data.VertexArray, 1, 4, GL_FLOAT, GL_FALSE, static_cast<GLuint>(offsetof(ParticleInstance, PositionAndSize)));
  glVertexArrayAttribBinding(s_Data.VertexArray, 1, 1);

  glEnableVertexArrayAttrib(s_Data.VertexArray, 2);
  glVertexArrayAttribFormat(
    s_Data.VertexArray, 2, 4, GL_FLOAT, GL_FALSE, static_cast<GLuint>(offsetof(ParticleInstance, Color)));
  glVertexArrayAttribBinding(s_Data.VertexArray, 2, 1);

  glEnableVertexArrayAttrib(s_Data.VertexArray, 3);
  glVertexArrayAttribFormat(
    s_Data.VertexArray, 3, 1, GL_FLOAT, GL_FALSE, static_cast<GLuint>(offsetof(ParticleInstance, Rotation)));
  glVertexArrayAttribBinding(s_Data.VertexArray, 3, 1);

  s_Data.Pool.resize(ParticleRendererData::MaxParticles);
  s_Data.Instances.reserve(ParticleRendererData::MaxParticles);

  s_Data.Prototype.Position = glm::vec3(0.0f);
  s_Data.Prototype.Velocity = glm::vec3(0.0f, 5.0f, 0.0f);
  s_Data.Prototype.Acceleration = glm::vec3(0.0f, 0.75f, 0.0f);
  s_Data.Prototype.ColorStart = glm::vec4(1.0f, 0.9f, 0.3f, 1.0f);
  s_Data.Prototype.ColorEnd = glm::vec4(0.6f, 0.1f, 0.0f, 0.0f);
  s_Data.Prototype.LifeTime = 3.0f;
  s_Data.Prototype.SizeBegin = 1.0f;
  s_Data.Prototype.SizeEnd = 0.0f;
  s_Data.Prototype.ConeAngle = glm::radians(30.0f);
}

void ParticleRenderer::Shutdown()
{
  glDeleteBuffers(1, &s_Data.InstanceBuffer);
  glDeleteBuffers(1, &s_Data.QuadVertexBuffer);
  glDeleteVertexArrays(1, &s_Data.VertexArray);
  s_Data.ParticleShader.reset();
  s_Data.Pool.clear();
  s_Data.Instances.clear();
  s_Data.ActiveCount = 0;
}

void ParticleRenderer::Emit()
{
  if (s_Data.ActiveCount >= s_Data.Pool.size())
    return;

  Particle& particle = s_Data.Pool[s_Data.ActiveCount++];
  particle.Position = s_Data.Prototype.Position;
  particle.Rotation = RandomGen::Float() * 2.0f * glm::pi<float>();
  particle.AngularVelocity = RandomGen::RandomRange(-1.5f, 1.5f);

  const float baseSpeed = glm::length(s_Data.Prototype.Velocity);
  const glm::vec3 baseDirection = baseSpeed > 0.0001f
    ? s_Data.Prototype.Velocity / baseSpeed
    : glm::vec3(0.0f, 1.0f, 0.0f);
  const glm::vec3 direction = RandomDirectionInCone(baseDirection, s_Data.Prototype.ConeAngle * 0.5f);

  particle.Velocity = direction * baseSpeed * RandomGen::RandomRange(0.8f, 1.2f);
  particle.Acceleration = s_Data.Prototype.Acceleration;
  particle.ColorStart = s_Data.Prototype.ColorStart;
  particle.ColorEnd = s_Data.Prototype.ColorEnd;
  particle.LifeTime = s_Data.Prototype.LifeTime * RandomGen::RandomRange(0.85f, 1.15f);
  particle.LifeRemaining = particle.LifeTime;
  particle.SizeBegin = s_Data.Prototype.SizeBegin * RandomGen::RandomRange(0.8f, 1.2f);
  particle.SizeEnd = s_Data.Prototype.SizeEnd;
}

void ParticleRenderer::UpdateAndRender(const DeltaTime& dt)
{
  const float delta = glm::clamp(static_cast<float>(dt), 0.0f, 0.1f);

  size_t particleIndex = 0;
  while (particleIndex < s_Data.ActiveCount)
  {
    Particle& particle = s_Data.Pool[particleIndex];
    AdvanceParticle(particle, delta);
    if (particle.LifeRemaining <= 0.0f)
    {
      --s_Data.ActiveCount;
      if (particleIndex != s_Data.ActiveCount)
        particle = s_Data.Pool[s_Data.ActiveCount];
      continue;
    }
    ++particleIndex;
  }

  s_Data.EmissionAccumulator += delta * ParticleRendererData::ParticlesPerSecond;
  const uint32_t particlesToEmit = static_cast<uint32_t>(s_Data.EmissionAccumulator);
  s_Data.EmissionAccumulator -= static_cast<float>(particlesToEmit);

  for (uint32_t i = 0; i < particlesToEmit && s_Data.ActiveCount < s_Data.Pool.size(); ++i)
  {
    Emit();
    Particle& particle = s_Data.Pool[s_Data.ActiveCount - 1];
    const float initialAge = delta * static_cast<float>(particlesToEmit - i) /
      static_cast<float>(particlesToEmit + 1);
    AdvanceParticle(particle, initialAge);
  }

  s_Data.Instances.clear();
  for (size_t i = 0; i < s_Data.ActiveCount; ++i)
  {
    const Particle& particle = s_Data.Pool[i];
    const float life = glm::clamp(particle.LifeRemaining / particle.LifeTime, 0.0f, 1.0f);
    const float age = 1.0f - life;
    glm::vec4 color = glm::mix(particle.ColorStart, particle.ColorEnd, age);
    color.a *= glm::smoothstep(0.0f, 0.08f, age);
    const float size = glm::mix(particle.SizeBegin, particle.SizeEnd, age);

    s_Data.Instances.push_back({glm::vec4(particle.Position, size), color, particle.Rotation});
  }

  RenderInstances();
}
