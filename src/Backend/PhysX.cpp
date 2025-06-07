#include "PhysX.h"
#include "BackendLogger.h"

struct UserErrorCallback : public PxErrorCallback
{
    virtual void reportError(PxErrorCode::Enum /*code*/, const char* message, const char* file, int line) {
        /*std::cout << file << " line " << line << ": " << message << "\n";*/
        /*std::cout << "\n";*/
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
    if (s_PhysXData.gFoundation) GABGL_INFO("PxCreateFoundation success!");

    s_PhysXData.gPvd = PxCreatePvd(*s_PhysXData.gFoundation);
    PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
    s_PhysXData.gPvd->connect(*transport,PxPvdInstrumentationFlag::eALL);

    s_PhysXData.gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *s_PhysXData.gFoundation, PxTolerancesScale(), true, s_PhysXData.gPvd);
    if (s_PhysXData.gPhysics) GABGL_INFO("PxCreatePhysics success!");

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

    PxRigidStatic* groundPlane = PxCreatePlane(*s_PhysXData.gPhysics, PxPlane(0,1,0,0), *s_PhysXData.gMaterial);
    s_PhysXData.gScene->addActor(*groundPlane);

    /*for (int i = 0; i < 5; i++) {*/
    /*    float halfExtent = 1.1f; // Half the side length of the cube*/
    /*    float gap = 0.1f; // Small gap between cubes*/
    /*    PxTransform t = PxTransform(PxVec3(i * (4.0f * halfExtent + gap), 0.0f, 13.5f));*/
    /*    PxShape* shape = gPhysics->createShape(PxBoxGeometry(halfExtent, halfExtent, halfExtent), *gMaterial);*/
    /*    PxRigidDynamic* body = gPhysics->createRigidDynamic(t);*/
    /*    body->attachShape(*shape);*/
    /*    PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);*/
    /*    gScene->addActor(*body);*/
    /*    shape->release();*/
    /*}*/
}

void PhysX::RenderActors(unsigned int vao)
{
    /*PxU32 nbActors = gScene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC);*/
    /*if(nbActors)*/
    /*{*/
    /*  std::vector<PxRigidActor*> actors(nbActors);*/
    /*  gScene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC, reinterpret_cast<PxActor**>(&actors[0]), nbActors);*/
    /**/
    /*  for (PxRigidActor* actor : actors){*/
    /*    const PxU32 nbShapes = actor->getNbShapes();*/
    /*    PxShape* shapes[32];*/
    /*    actor->getShapes(shapes,nbShapes);*/
    /**/
    /*    for (PxU32 j = 0; j < nbShapes; j++){*/
    /*      const PxMat44 shapePose(PxShapeExt::getGlobalPose(*shapes[j], *actor));*/
    /*      const PxGeometry& geom = shapes[j]->getGeometry();*/
    /**/
    /*      if(geom.getType() == PxGeometryType::eBOX){*/
    /*        const PxBoxGeometry& boxGeom = static_cast<const PxBoxGeometry&>(geom);*/
    /**/
    /*        shader.Use();*/
    /**/
    /*        Utilities::Transform transform;*/
    /*        transform.scale = glm::vec3(boxGeom.halfExtents.x,boxGeom.halfExtents.y,boxGeom.halfExtents.z);*/
    /*        glm::mat4 model = Utilities::PxMat44ToGlmMat4(actor->getGlobalPose()) * transform.to_mat4();*/
    /**/
    /*        shader.setMat4("model",model);*/
    /*        glBindVertexArray(vao);*/
    /*        glDrawArrays(GL_TRIANGLES, 0, 36);*/
    /*      }*/
    /*    }*/
    /*  }*/
    /*}*/
}

void PhysX::Simulate(float deltatime)
{
    s_PhysXData.gScene->simulate(deltatime);
    s_PhysXData.gScene->fetchResults(true);
}

inline void SetupCommonCookingParams(PxCookingParams& params, bool skipMeshCleanup, bool skipEdgeData) {
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

PxTriangleMesh* PhysX::CreateTriangleMesh(PxU32 numVertices, const PxVec3* vertices, PxU32 numTriangles, const PxU32* indices) {

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
    PxVec3 pxOrigin(origin.x, origin.y, origin.z);
    PxVec3 pxDirection(direction.x, direction.y, direction.z);
    PxReal maxDistance = rayLength;

    PxRaycastBuffer hitBuffer;

    if (scene->raycast(pxOrigin, pxDirection.getNormalized(), maxDistance, hitBuffer)) {
        if (hitBuffer.hasBlock) {
            const PxRaycastHit& hit = hitBuffer.block;
            PxRigidActor* actor = hit.actor;

            if (actor && actor->is<PxRigidDynamic>()) {
                //std::cout << "Hit a dynamic actor, applying force!" << std::endl;
                puts("ADDING FORCE");
                PxVec3 pxForce = PxVec3(direction.x, direction.y, direction.z) * 100000;
                PxRigidDynamic* dynamicActor = static_cast<PxRigidDynamic*>(actor);
                dynamicActor->addForce(pxForce,PxForceMode::eFORCE,false);
            } 
        } 
    } 
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
