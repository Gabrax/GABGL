#pragma once

#include "PxPhysicsAPI.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace physx;

struct PhysX
{
  static void Init();
  static void RenderActors(unsigned int vao);
  static void Simulate(float deltatime);
  static void raycastAndApplyForce(PxScene* scene, const glm::vec3& origin, const glm::vec3& direction, float rayLength);
  static void DisableRaycast(PxShape* shape);
  static void EnableRaycast(PxShape* shape);

  static PxTriangleMesh* CreateTriangleMesh(PxU32 numVertices, const PxVec3* vertices, PxU32 numTriangles, const PxU32* indices);
  static PxConvexMesh* CreateConvexMesh(PxU32 numVertices, const PxVec3* vertices);

  static glm::mat4 PxMat44ToGlmMat4(physx::PxMat44 pxMatrix);
  static PxMat44 GlmMat4ToPxMat44(glm::mat4 glmMatrix);

  static PxScene* getScene();
  static PxPhysics* getPhysics();
  static PxMaterial* getMaterial();
};
