#pragma once

#include "PxPhysicsAPI.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "DeltaTime.hpp"

using namespace physx;

struct PhysicsRaycastHit
{
  glm::vec3 position{0.0f};
  glm::vec3 normal{0.0f, 1.0f, 0.0f};
  float distance = 0.0f;
};

struct PhysX
{
  static void Init();
  static void Shutdown();
  static void Simulate(DeltaTime& dt);
  static void raycastAndApplyForce(PxScene* scene, const glm::vec3& origin, const glm::vec3& direction, float rayLength);
  static bool Raycast(
  const glm::vec3& origin,
  const glm::vec3& direction,
  float rayLength,
  PhysicsRaycastHit& result,
  const PxRigidActor* ignoredActor = nullptr,
  float impulseStrength = 1000.0f);
  static void DisableRaycast(PxShape* shape);
  static void EnableRaycast(PxShape* shape);

  static PxTriangleMesh* CreateTriangleMesh(PxU32 numVertices, const PxVec3* vertices, PxU32 numTriangles, const PxU32* indices);
  static PxConvexMesh* CreateConvexMesh(PxU32 numVertices, const PxVec3* vertices);
  static PxController* CreateCharacterController(const PxVec3& footPosition, float radius, float height, bool slopeLimit);

  static PxTransform GlmMat4ToPxTransform(const glm::mat4& mat);
  static glm::mat4 PxMat44ToGlmMat4(physx::PxMat44 pxMatrix);
  static PxMat44 GlmMat4ToPxMat44(glm::mat4 glmMatrix);
  static PxVec3 GlmVec3ToPxVec3(const glm::vec3& vec);
  static glm::vec3 PxExtendedVec3toGlmVec3(const PxExtendedVec3& v);

  static PxScene* getScene();
  static PxPhysics* getPhysics();
  static PxMaterial* getMaterial();
};
