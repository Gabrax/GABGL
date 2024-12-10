#pragma once

#include "PxPhysicsAPI.h"

using namespace physx;
class UserErrorCallback : public PxErrorCallback
{
public:
    virtual void reportError(PxErrorCode::Enum /*code*/, const char* message, const char* file, int line) {
        std::cout << file << " line " << line << ": " << message << "\n";
        std::cout << "\n";
    }
}gErrorCallback;

#define PVD_HOST "127.0.0.1"

static PxDefaultAllocator		gAllocator;
static PxFoundation*			gFoundation = NULL;
static PxPhysics*				gPhysics	= NULL;
static PxDefaultCpuDispatcher*	gDispatcher = NULL;
static PxScene*					gScene		= NULL;
static PxMaterial*				gMaterial	= NULL;
static PxPvd*					gPvd        = NULL;

namespace PhysX {

  inline void Init()
  {
    gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
    gPvd = PxCreatePvd(*gFoundation);
    PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
    gPvd->connect(*transport,PxPvdInstrumentationFlag::eALL);

    gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), true, gPvd);
  }

}
