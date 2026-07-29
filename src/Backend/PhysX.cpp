#include "PhysX.h"
#include "Logger.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <algorithm>
#include <cmath>

struct UserErrorCallback : public PxErrorCallback
{
  virtual void reportError(PxErrorCode::Enum /*code*/, const char* message, const char* file, int line)
  {
    GABGL_ERROR("file: {0}, line: {1}, message: {2}",file,line,message);
  }

}gErrorCallback;

#define PVD_HOST "127.0.0.1"

struct PhysXData
{
  PxDefaultAllocator		gAllocator;
  PxFoundation*			gFoundation = nullptr;
  PxPhysics*				gPhysics	= nullptr;
  PxDefaultCpuDispatcher*	gDispatcher = nullptr;
  PxScene*				gScene		= nullptr;
  PxMaterial*				gMaterial	= nullptr;
  PxPvd*					gPvd        = nullptr;
  PxPvdTransport* gPvdTransport = nullptr;
  PxControllerManager* controllerManager = nullptr;
} s_PhysXData;

class TriggerRender
{
    public:
    virtual	bool	isTrigger(physx::PxShape*)	const	= 0;
};

enum RaycastGroup { RAYCAST_DISABLED = 0, RAYCAST_ENABLED = 1 };

void PhysX::Init()
{
  s_PhysXData.gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, s_PhysXData.gAllocator, gErrorCallback);
  if (!s_PhysXData.gFoundation) GABGL_ERROR("PxCreateFoundation init failed!");

  s_PhysXData.gPvd = PxCreatePvd(*s_PhysXData.gFoundation);
  s_PhysXData.gPvdTransport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
  if (s_PhysXData.gPvd && s_PhysXData.gPvdTransport)
    s_PhysXData.gPvd->connect(*s_PhysXData.gPvdTransport, PxPvdInstrumentationFlag::eALL);

  s_PhysXData.gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *s_PhysXData.gFoundation, PxTolerancesScale(), true, s_PhysXData.gPvd);
  if (!s_PhysXData.gPhysics) GABGL_ERROR("PxCreatePhysics init failed!");

  PxSceneDesc sceneDesc(s_PhysXData.gPhysics->getTolerancesScale());
  sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
  s_PhysXData.gDispatcher = PxDefaultCpuDispatcherCreate(2);
  sceneDesc.cpuDispatcher	= s_PhysXData.gDispatcher;
  sceneDesc.filterShader	= PxDefaultSimulationFilterShader;
  s_PhysXData.gScene = s_PhysXData.gPhysics->createScene(sceneDesc);

  PxPvdSceneClient* pvdClient = s_PhysXData.gScene->getScenePvdClient();
  if(pvdClient)
  {
    pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
    pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
    pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
  }
  s_PhysXData.gMaterial = s_PhysXData.gPhysics->createMaterial(0.5f, 0.5f, 0.6f);

  s_PhysXData.controllerManager = PxCreateControllerManager(*s_PhysXData.gScene);

  GABGL_WARN("PhysX init success!");
}

void PhysX::Shutdown()
{
  if (s_PhysXData.controllerManager)
  {
    s_PhysXData.controllerManager->release();
    s_PhysXData.controllerManager = nullptr;
  }
  if (s_PhysXData.gMaterial)
  {
    s_PhysXData.gMaterial->release();
    s_PhysXData.gMaterial = nullptr;
  }
  if (s_PhysXData.gScene)
  {
    s_PhysXData.gScene->release();
    s_PhysXData.gScene = nullptr;
  }
  if (s_PhysXData.gDispatcher)
  {
    s_PhysXData.gDispatcher->release();
    s_PhysXData.gDispatcher = nullptr;
  }
  if (s_PhysXData.gPhysics)
  {
    s_PhysXData.gPhysics->release();
    s_PhysXData.gPhysics = nullptr;
  }
  if (s_PhysXData.gPvd)
  {
    s_PhysXData.gPvd->disconnect();
    s_PhysXData.gPvd->release();
    s_PhysXData.gPvd = nullptr;
  }
  if (s_PhysXData.gPvdTransport)
  {
    s_PhysXData.gPvdTransport->release();
    s_PhysXData.gPvdTransport = nullptr;
  }
  if (s_PhysXData.gFoundation)
  {
    s_PhysXData.gFoundation->release();
    s_PhysXData.gFoundation = nullptr;
  }
}

void PhysX::Simulate(DeltaTime& dt)
{
  if (!s_PhysXData.gScene)
    return;

  constexpr float fixedTimeStep = 1.0f / 60.0f;
  constexpr float maxFrameTime = 0.25f;
  constexpr int maxSubsteps = 8;
  static float accumulator = 0.0f;

  accumulator += std::clamp(dt.GetSeconds(), 0.0f, maxFrameTime);

  int substeps = 0;
  while (accumulator >= fixedTimeStep && substeps < maxSubsteps)
  {
    s_PhysXData.gScene->simulate(fixedTimeStep);
    s_PhysXData.gScene->fetchResults(true);
    accumulator -= fixedTimeStep;
    ++substeps;
  }

  // Do not try to catch up forever after a breakpoint or a very slow frame.
  if (substeps == maxSubsteps && accumulator >= fixedTimeStep)
    accumulator = std::fmod(accumulator, fixedTimeStep);
}

inline void SetupCommonCookingParams(PxCookingParams& params, bool skipMeshCleanup, bool skipEdgeData)
{
  // we suppress the triangle mesh remap table computation to gain some speed, as we will not need it
  // in this snippet
  params.suppressTriangleMeshRemapTable = true;

  // If DISABLE_CLEAN_MESH is set, the mesh is not cleaned during the cooking. The input mesh must be valid.
  // The following conditions are true for a valid triangle mesh :
  //  1. There are no duplicate vertices(within specified vertexWeldTolerance.See PxCookingParams::meshWeldTolerance)
  //  2. There are no large triangles(within specified PxTolerancesScale.)
  // It is recommended to run a separate validation check in debug/checked builds, see below.

  if (!skipMeshCleanup)
      params.meshPreprocessParams &= ~static_cast<PxMeshPreprocessingFlags>(PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH);
  else
      params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH;

  // If eDISABLE_ACTIVE_EDGES_PRECOMPUTE is set, the cooking does not compute the active (convex) edges, and instead
  // marks all edges as active. This makes cooking faster but can slow down contact generation. This flag may change
  // the collision behavior, as all edges of the triangle mesh will now be considered active.
  if (!skipEdgeData)
      params.meshPreprocessParams &= ~static_cast<PxMeshPreprocessingFlags>(PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE);
  else
      params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;
}

PxTriangleMesh* PhysX::CreateTriangleMesh(PxU32 numVertices, const PxVec3* vertices, PxU32 numTriangles, const PxU32* indices)
{
  PxTriangleMeshDesc meshDesc;
  meshDesc.points.count = numVertices;
  meshDesc.points.data = vertices;
  meshDesc.points.stride = sizeof(PxVec3);
  meshDesc.triangles.count = numTriangles;
  meshDesc.triangles.data = indices;
  meshDesc.triangles.stride = 3 * sizeof(PxU32);

  PxTolerancesScale scale;
  PxCookingParams params(scale);

  // Create BVH33 midphase
  params.midphaseDesc = PxMeshMidPhase::eBVH33;

  // setup common cooking params
  bool skipMeshCleanup = false;
  bool skipEdgeData = false;
  bool cookingPerformance = false;
  bool meshSizePerfTradeoff = true;
  SetupCommonCookingParams(params, skipMeshCleanup, skipEdgeData);

  // The COOKING_PERFORMANCE flag for BVH33 midphase enables a fast cooking path at the expense of somewhat lower quality BVH construction.
  if (cookingPerformance) {
      params.midphaseDesc.mBVH33Desc.meshCookingHint = PxMeshCookingHint::eCOOKING_PERFORMANCE;
  }
  else {
      params.midphaseDesc.mBVH33Desc.meshCookingHint = PxMeshCookingHint::eSIM_PERFORMANCE;
  }

  // If meshSizePerfTradeoff is set to true, smaller mesh cooked mesh is produced. The mesh size/performance trade-off
  // is controlled by setting the meshSizePerformanceTradeOff from 0.0f (smaller mesh) to 1.0f (larger mesh).
  if (meshSizePerfTradeoff) {
      params.midphaseDesc.mBVH33Desc.meshSizePerformanceTradeOff = 0.0f;
  }
  else {
      // using the default value
      params.midphaseDesc.mBVH33Desc.meshSizePerformanceTradeOff = 0.55f;
  }
  if (skipMeshCleanup) {
      PX_ASSERT(PxValidateTriangleMesh(params, meshDesc));
  }

  PxTriangleMesh* triMesh = NULL;
  //PxU32 meshSize = 0;

  triMesh = PxCreateTriangleMesh(params, meshDesc, s_PhysXData.gPhysics->getPhysicsInsertionCallback());
  return triMesh;
  //triMesh->release();
}

PxConvexMesh* PhysX::CreateConvexMesh(PxU32 numVertices, const PxVec3* vertices)
{
  PxConvexMeshDesc convexDesc;
  convexDesc.points.count     = numVertices;
  convexDesc.points.stride    = sizeof(PxVec3);
  convexDesc.points.data      = vertices;
  convexDesc.flags            = PxConvexFlag::eCOMPUTE_CONVEX;

  PxTolerancesScale scale;
  PxCookingParams params(scale);
  SetupCommonCookingParams(params,false,false); // Optional: reuse your cooking param setup

  PxConvexMesh* convexMesh = nullptr;

  // Create the mesh
  convexMesh = PxCreateConvexMesh(params, convexDesc, s_PhysXData.gPhysics->getPhysicsInsertionCallback());

  return convexMesh;
}

PxController* PhysX::CreateCharacterController(const PxVec3& position, float radius, float height, bool slopeLimit)
{
  PxCapsuleControllerDesc desc;
  desc.radius = radius;
  desc.height = height;
  desc.position = PxExtendedVec3(position.x, position.y, position.z);
  desc.material = s_PhysXData.gMaterial;
  desc.stepOffset = 0.3f; // how high it can "step"
  desc.contactOffset = 0.05f; // tolerance buffer
  desc.slopeLimit = slopeLimit ? cosf(PxPi / 4.0f) : 0.0f; // 45° max slope
  desc.upDirection = PxVec3(0.0f, 1.0f, 0.0f);
  desc.climbingMode = PxCapsuleClimbingMode::eCONSTRAINED;

  if (!desc.isValid())
  {
      GABGL_ERROR("CharacterController description is invalid.");
      return nullptr;
  }

  PxController* controller = s_PhysXData.controllerManager->createController(desc);
  if (!controller)
  {
      GABGL_ERROR("Failed to create character controller.");
      return nullptr;
  }

  return controller;
}

void PhysX::EnableRaycast(PxShape* shape) {
    PxFilterData filterData = shape->getQueryFilterData();
    filterData.word0 = RaycastGroup::RAYCAST_ENABLED;
    shape->setQueryFilterData(filterData);
}

void PhysX::DisableRaycast(PxShape* shape) {
    PxFilterData filterData = shape->getQueryFilterData();
    filterData.word0 = RaycastGroup::RAYCAST_DISABLED;
    shape->setQueryFilterData(filterData);
}

void PhysX::raycastAndApplyForce(PxScene* scene, const glm::vec3& origin, const glm::vec3& direction, float rayLength)
{
  if (scene != s_PhysXData.gScene)
    return;

  PhysicsRaycastHit ignoredResult;
  Raycast(origin, direction, rayLength, ignoredResult);
}

bool PhysX::Raycast(
  const glm::vec3& origin,
  const glm::vec3& direction,
  float rayLength,
  PhysicsRaycastHit& result,
  const PxRigidActor* ignoredActor,
  float impulseStrength)
{
  if (!s_PhysXData.gScene || rayLength <= 0.0f || glm::dot(direction, direction) < 0.000001f)
    return false;

  class IgnoreActorFilter final : public PxQueryFilterCallback
  {
  public:
    explicit IgnoreActorFilter(const PxRigidActor* ignored) : m_Ignored(ignored) {}

    PxQueryHitType::Enum preFilter(
      const PxFilterData&,
      const PxShape*,
      const PxRigidActor* actor,
      PxHitFlags&) override
    {
      return actor == m_Ignored ? PxQueryHitType::eNONE : PxQueryHitType::eBLOCK;
    }

    PxQueryHitType::Enum postFilter(
      const PxFilterData&,
      const PxQueryHit&,
      const PxShape*,
      const PxRigidActor*) override
    {
      return PxQueryHitType::eBLOCK;
    }

  private:
    const PxRigidActor* m_Ignored;
  };

  const glm::vec3 normalizedDirection = glm::normalize(direction);
  const PxVec3 pxDirection = GlmVec3ToPxVec3(normalizedDirection);
  PxRaycastBuffer hitBuffer;
  IgnoreActorFilter filter(ignoredActor);
  PxQueryFilterData filterData(
    PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER);
  const PxHitFlags hitFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;

  if (!s_PhysXData.gScene->raycast(
        GlmVec3ToPxVec3(origin), pxDirection, rayLength, hitBuffer, hitFlags, filterData, &filter)
      || !hitBuffer.hasBlock)
    return false;

  const PxRaycastHit& hit = hitBuffer.block;
  result.position = glm::vec3(hit.position.x, hit.position.y, hit.position.z);
  result.normal = glm::vec3(hit.normal.x, hit.normal.y, hit.normal.z);
  result.distance = hit.distance;

  if (PxRigidDynamic* dynamicActor = hit.actor ? hit.actor->is<PxRigidDynamic>() : nullptr)
  {
    if (!dynamicActor->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
      dynamicActor->addForce(pxDirection * impulseStrength, PxForceMode::eIMPULSE);
  }

  return true;
}

PxTransform PhysX::GlmMat4ToPxTransform(const glm::mat4& mat)
{
  glm::vec3 scale, translation, skew;
  glm::vec4 perspective;
  glm::quat rotation;

  glm::decompose(mat, scale, rotation, translation, skew, perspective);

  return physx::PxTransform(
      physx::PxVec3(translation.x, translation.y, translation.z),
      physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w)
  );
}

glm::mat4 PhysX::PxMat44ToGlmMat4(physx::PxMat44 pxMatrix)
{
  glm::mat4 matrix;
  for (int x = 0; x < 4; x++)
      for (int y = 0; y < 4; y++)
          matrix[x][y] = pxMatrix[x][y];
  return matrix;
}

PxMat44 PhysX::GlmMat4ToPxMat44(glm::mat4 glmMatrix)
{
  PxMat44 matrix;
  for (int x = 0; x < 4; x++)
      for (int y = 0; y < 4; y++)
          matrix[x][y] = glmMatrix[x][y];
  return matrix;
}

PxVec3 PhysX::GlmVec3ToPxVec3(const glm::vec3& vec)
{
    return PxVec3(vec.x, vec.y, vec.z);
}

glm::vec3 PhysX::PxExtendedVec3toGlmVec3(const PxExtendedVec3& v)
{
    return glm::vec3(
        static_cast<float>(v.x),
        static_cast<float>(v.y),
        static_cast<float>(v.z)
    );
}

PxScene* PhysX::getScene()
{
    return s_PhysXData.gScene;
}

PxPhysics* PhysX::getPhysics()
{
    return s_PhysXData.gPhysics;
}

PxMaterial* PhysX::getMaterial()
{
    return s_PhysXData.gMaterial;
}
