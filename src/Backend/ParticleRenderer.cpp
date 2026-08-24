#include "ParticleRenderer.h"

#include "Camera.h"
#include "RenderBackend.h"
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

  struct ImpactMark
  {
    glm::vec3 Position{0.0f};
    glm::vec3 Right{1.0f, 0.0f, 0.0f};
    glm::vec3 Up{0.0f, 1.0f, 0.0f};
  };

  struct ParticleRendererData
  {
    static constexpr size_t MaxParticles = 512;
    static constexpr size_t MaxImpactMarks = 128;

    GLuint VertexArray = 0;
    GLuint QuadVertexBuffer = 0;
    GLuint InstanceBuffer = 0;
    std::shared_ptr<Shader> ParticleShader;

    Particle Prototype;
    std::vector<Particle> Pool;
    std::vector<ParticleRenderInstance> Instances;
    std::vector<ImpactMark> ImpactMarks;
    size_t ActiveCount = 0;
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

    if (RenderBackend::Get().DrawParticles(s_Data.Instances)) return;

    glNamedBufferSubData(
      s_Data.InstanceBuffer,
      0,
      static_cast<GLsizeiptr>(s_Data.Instances.size() * sizeof(ParticleRenderInstance)),
      s_Data.Instances.data());

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    s_Data.ParticleShader->Bind();

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
  if (!RenderBackend::Capabilities().NativeParticleRenderer)
  {
    Shader::Create(s_Data.ParticleShader, "../res/shaders/particle.slang");

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
    static_cast<GLsizeiptr>(
      (ParticleRendererData::MaxParticles + ParticleRendererData::MaxImpactMarks) * sizeof(ParticleRenderInstance)),
    nullptr,
    GL_DYNAMIC_DRAW);

  glVertexArrayVertexBuffer(s_Data.VertexArray, 0, s_Data.QuadVertexBuffer, 0, sizeof(glm::vec2));
  glEnableVertexArrayAttrib(s_Data.VertexArray, 0);
  glVertexArrayAttribFormat(s_Data.VertexArray, 0, 2, GL_FLOAT, GL_FALSE, 0);
  glVertexArrayAttribBinding(s_Data.VertexArray, 0, 0);

  glVertexArrayVertexBuffer(s_Data.VertexArray, 1, s_Data.InstanceBuffer, 0, sizeof(ParticleRenderInstance));
  glVertexArrayBindingDivisor(s_Data.VertexArray, 1, 1);

  glEnableVertexArrayAttrib(s_Data.VertexArray, 1);
  glVertexArrayAttribFormat(
    s_Data.VertexArray, 1, 4, GL_FLOAT, GL_FALSE, static_cast<GLuint>(offsetof(ParticleRenderInstance, PositionAndSize)));
  glVertexArrayAttribBinding(s_Data.VertexArray, 1, 1);

  glEnableVertexArrayAttrib(s_Data.VertexArray, 2);
  glVertexArrayAttribFormat(
    s_Data.VertexArray, 2, 4, GL_FLOAT, GL_FALSE, static_cast<GLuint>(offsetof(ParticleRenderInstance, Color)));
  glVertexArrayAttribBinding(s_Data.VertexArray, 2, 1);

  glEnableVertexArrayAttrib(s_Data.VertexArray, 3);
  glVertexArrayAttribFormat(
    s_Data.VertexArray, 3, 1, GL_FLOAT, GL_FALSE, static_cast<GLuint>(offsetof(ParticleRenderInstance, Rotation)));
  glVertexArrayAttribBinding(s_Data.VertexArray, 3, 1);

  glEnableVertexArrayAttrib(s_Data.VertexArray, 4);
  glVertexArrayAttribFormat(
    s_Data.VertexArray, 4, 4, GL_FLOAT, GL_FALSE,
    static_cast<GLuint>(offsetof(ParticleRenderInstance, RightAndStyle)));
  glVertexArrayAttribBinding(s_Data.VertexArray, 4, 1);

  glEnableVertexArrayAttrib(s_Data.VertexArray, 5);
  glVertexArrayAttribFormat(
    s_Data.VertexArray, 5, 4, GL_FLOAT, GL_FALSE,
    static_cast<GLuint>(offsetof(ParticleRenderInstance, Up)));
    glVertexArrayAttribBinding(s_Data.VertexArray, 5, 1);
  }

  s_Data.Pool.resize(ParticleRendererData::MaxParticles);
  s_Data.Instances.reserve(
    ParticleRendererData::MaxParticles + ParticleRendererData::MaxImpactMarks);
  s_Data.ImpactMarks.reserve(ParticleRendererData::MaxImpactMarks);

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
  if (!RenderBackend::Capabilities().NativeParticleRenderer)
  {
    glDeleteBuffers(1, &s_Data.InstanceBuffer);
    glDeleteBuffers(1, &s_Data.QuadVertexBuffer);
    glDeleteVertexArrays(1, &s_Data.VertexArray);
  }
  s_Data.InstanceBuffer = 0;
  s_Data.QuadVertexBuffer = 0;
  s_Data.VertexArray = 0;
  s_Data.ParticleShader.reset();
  s_Data.Pool.clear();
  s_Data.Instances.clear();
  s_Data.ImpactMarks.clear();
  s_Data.ActiveCount = 0;
}

void ParticleRenderer::Clear()
{
  s_Data.ActiveCount = 0;
  s_Data.Instances.clear();
  s_Data.ImpactMarks.clear();
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

void ParticleRenderer::EmitImpact(
  const glm::vec3& position,
  const glm::vec3& normal,
  uint32_t particleCount)
{
  const glm::vec3 impactNormal = glm::dot(normal, normal) > 0.000001f
    ? glm::normalize(normal)
    : glm::vec3(0.0f, 1.0f, 0.0f);

  const glm::vec3 reference = std::abs(impactNormal.y) < 0.99f
    ? glm::vec3(0.0f, 1.0f, 0.0f)
    : glm::vec3(1.0f, 0.0f, 0.0f);
  const glm::vec3 markRight = glm::normalize(glm::cross(reference, impactNormal));
  const glm::vec3 markUp = glm::normalize(glm::cross(impactNormal, markRight));

  if (s_Data.ImpactMarks.size() >= ParticleRendererData::MaxImpactMarks)
    s_Data.ImpactMarks.erase(s_Data.ImpactMarks.begin());
  s_Data.ImpactMarks.push_back({
    position + impactNormal * 0.015f,
    markRight,
    markUp
  });

  for (uint32_t i = 0; i < particleCount && s_Data.ActiveCount < s_Data.Pool.size(); ++i)
  {
    Particle& particle = s_Data.Pool[s_Data.ActiveCount++];
    particle.Position = position + impactNormal * 0.025f;
    particle.Rotation = RandomGen::Float() * 2.0f * glm::pi<float>();
    particle.AngularVelocity = RandomGen::RandomRange(-8.0f, 8.0f);

    const glm::vec3 direction = RandomDirectionInCone(impactNormal, glm::radians(75.0f));
    particle.Velocity = direction * RandomGen::RandomRange(2.5f, 6.5f);
    particle.Acceleration = glm::vec3(0.0f, -9.81f, 0.0f);
    particle.ColorStart = glm::vec4(0.95f, 0.9f, 0.8f, 1.0f);
    particle.ColorEnd = glm::vec4(0.25f, 0.22f, 0.18f, 0.0f);
    particle.LifeTime = RandomGen::RandomRange(0.25f, 0.55f);
    particle.LifeRemaining = particle.LifeTime;
    particle.SizeBegin = RandomGen::RandomRange(0.06f, 0.14f);
    particle.SizeEnd = 0.01f;
  }
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

  s_Data.Instances.clear();
  const glm::vec3 cameraRight = Camera::GetRightDirection();
  const glm::vec3 cameraUp = Camera::GetUpDirection();
  for (size_t i = 0; i < s_Data.ActiveCount; ++i)
  {
    const Particle& particle = s_Data.Pool[i];
    const float life = glm::clamp(particle.LifeRemaining / particle.LifeTime, 0.0f, 1.0f);
    const float age = 1.0f - life;
    glm::vec4 color = glm::mix(particle.ColorStart, particle.ColorEnd, age);
    color.a *= glm::smoothstep(0.0f, 0.08f, age);
    const float size = glm::mix(particle.SizeBegin, particle.SizeEnd, age);

    s_Data.Instances.push_back({
      glm::vec4(particle.Position, size),
      color,
      particle.Rotation,
      glm::vec4(cameraRight, 0.0f),
      glm::vec4(cameraUp, 0.0f)
    });
  }

  for (const ImpactMark& mark : s_Data.ImpactMarks)
  {
    s_Data.Instances.push_back({
      glm::vec4(mark.Position, 0.22f),
      glm::vec4(1.0f),
      0.0f,
      glm::vec4(mark.Right, 1.0f),
      glm::vec4(mark.Up, 0.0f)
    });
  }

  RenderInstances();
}
