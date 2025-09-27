#include "pch.hpp"
#include "System_Physics.hpp"


// #include "Engine/ECS/Component/TransformComponent.hpp"
// #include "Engine/ECS/Component/PhysicsComponent.hpp"
// #include "Engine/ECS/Component/ColliderComponent.hpp"
// #include "CoreEngine.hpp"

// ---------------- Jolt includes only in .cpp ----------------
#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Physics/PhysicsUpdateContext.h>

#include <cstdarg>
#include <cstdio>
#include <thread>
#include <algorithm>
#include <memory>

// ---------------- Minimal trace/assert routing ----------------
static void JoltTraceImpl(const char* fmt, ...) {
    va_list list; va_start(list, fmt);
    char buffer[1024]; vsnprintf(buffer, sizeof(buffer), fmt, list);
    va_end(list);
    std::fputs("[Jolt] ", stdout);
    std::fputs(buffer, stdout);
    std::fputc('\n', stdout);
}

static bool JoltAssertFailedImpl(const char* expr, const char* msg, const char* file, JPH::uint line) {
    JoltTraceImpl("Assertion failed: %s  (%s:%u)  %s", expr, file, line, msg ? msg : "");
    return true;
}

// ---------------- Filter implementations ----------------
namespace {
    using namespace JPH;

    class BroadPhaseLayerInterfaceImpl final : public BroadPhaseLayerInterface {
    public:
        BroadPhaseLayerInterfaceImpl() {
            mBroadPhaseLayers[(int)BroadPhaseLayerTag::NON_MOVING] = BroadPhaseLayer(0);
            mBroadPhaseLayers[(int)BroadPhaseLayerTag::MOVING] = BroadPhaseLayer(1);
        }

        uint GetNumBroadPhaseLayers() const override { return 2; }

        BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer layer) const override {
            if (layer == (ObjectLayer)PhysicsLayer::NON_MOVING)
                return mBroadPhaseLayers[(int)BroadPhaseLayerTag::NON_MOVING];
            return mBroadPhaseLayers[(int)BroadPhaseLayerTag::MOVING];
        }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE)
        const char* GetBroadPhaseLayerName(BroadPhaseLayer layer) const override {
            return (layer.GetValue() == 0) ? "NON_MOVING" : "MOVING";
        }
#endif
    private:
        BroadPhaseLayer mBroadPhaseLayers[2];
    };

    class ObjectLayerPairFilterImpl final : public ObjectLayerPairFilter {
    public:
        bool ShouldCollide(ObjectLayer a, ObjectLayer b) const override {
            if (a == (ObjectLayer)PhysicsLayer::MOVING) return true;
            if (b == (ObjectLayer)PhysicsLayer::MOVING) return true;
            return a != b;
        }
    };

    class ObjectVsBroadPhaseLayerFilterImpl final : public ObjectVsBroadPhaseLayerFilter {
    public:
        bool ShouldCollide(ObjectLayer layer, BroadPhaseLayer b) const override {
            if (layer == (ObjectLayer)PhysicsLayer::MOVING) return true;
            return b == BroadPhaseLayer(1);
        }
    };
} // anonymous namespace

namespace EngineFramework {

    // -------- Static guard so we register Jolt globals once per process --------
    static bool sJoltGlobalsInitialized = false;

    PhysicsSystem::PhysicsSystem() {
        s_instance = this;
    }

    PhysicsSystem& PhysicsSystem::Get() {
        return *s_instance;
    }

    void PhysicsSystem::Initialize() {
        if (!sJoltGlobalsInitialized) {
            JPH::Trace = JoltTraceImpl;
            JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = JoltAssertFailedImpl;)

                JPH::RegisterDefaultAllocator();
            JPH::Factory::sInstance = new JPH::Factory();
            JPH::RegisterTypes();

            sJoltGlobalsInitialized = true;
        }

        // 2) Per-world allocators & job system
        m_temp_alloc = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024); // 10 MB scratch
        const uint32_t workers = std::max(1u, std::thread::hardware_concurrency() > 0
            ? std::thread::hardware_concurrency() - 1
            : 1u);
        m_job_system = std::make_unique<JPH::JobSystemThreadPool>(
            JPH::cMaxPhysicsJobs,       
            JPH::cMaxPhysicsBarriers,
            static_cast<int>(workers)
        );

        // 3) Filters / layer routing
        m_bp_layers = std::make_unique<BroadPhaseLayerInterfaceImpl>();
        m_obj_vs_bp = std::make_unique<ObjectVsBroadPhaseLayerFilterImpl>();
        m_obj_pair = std::make_unique<ObjectLayerPairFilterImpl>();

        // 4) Create & initialize the Jolt world
        constexpr JPH::uint cMaxBodies = 10000;
        constexpr JPH::uint cNumBodyMutexes = 0;     // let Jolt choose
        constexpr JPH::uint cMaxBodyPairs = 65536; // broadphase pairs
        constexpr JPH::uint cMaxContactConstraints = 65536; // contact constraints

        m_jolt = std::make_unique<JPH::PhysicsSystem>();
        m_jolt->Init(
            cMaxBodies,
            cNumBodyMutexes,
            cMaxBodyPairs,
            cMaxContactConstraints,
            *m_bp_layers,
            *m_obj_vs_bp,
            *m_obj_pair
        );

        // 5) Cache BodyInterface and set gravity
        m_bodies = &m_jolt->GetBodyInterface();
        m_jolt->SetGravity(JPH::Vec3(0.0f, -9.8f, 0.0f));
    }

    void PhysicsSystem::Update(float delta_time) {
        // Fixed-step integration
        m_accumulator += delta_time;
        while (m_accumulator >= m_fixed_dt) {
            if (m_jolt) {
                // 1 collision step is fine to start; you can raise it if tunneling becomes an issue.
                m_jolt->Update(m_fixed_dt, /*collisionSteps*/ 1, m_temp_alloc.get(), m_job_system.get());
            }
            m_accumulator -= m_fixed_dt;
        }
    }

    void PhysicsSystem::PostUpdate(float /*delta_time*/) {
        // TODO: any late sync (e.g., debug draw) later
    }

} // namespace EngineFramework
