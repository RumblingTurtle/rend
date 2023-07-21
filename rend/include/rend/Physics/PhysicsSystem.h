#pragma once
#include <rend/EntityRegistry.h>
#include <rend/System.h>
#include <rend/components.h>

#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>

#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

enum BodyType { STATIC, DYNAMIC };

namespace JPH {
namespace Layers {
static constexpr ObjectLayer NON_MOVING = 0;
static constexpr ObjectLayer MOVING = 1;
static constexpr ObjectLayer NUM_LAYERS = 2;
}; // namespace Layers

// Each broadphase layer results in a separate bounding volume tree in the broad
// phase. You at least want to have a layer for non-moving and moving objects to
// avoid having to update a tree full of static objects every frame. You can
// have a 1-on-1 mapping between object layers and broadphase layers (like in
// this case) but if you have many object layers you'll be creating many broad
// phase trees, which is not efficient. If you want to fine tune your broadphase
// layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on
// the TTY.
namespace BroadPhaseLayers {
static constexpr BroadPhaseLayer NON_MOVING(0);
static constexpr BroadPhaseLayer MOVING(1);
static constexpr uint NUM_LAYERS(2);
}; // namespace BroadPhaseLayers

Vec3 eigen_to_jolt(const Eigen::Vector3f &vec) {
  return Vec3{vec.x(), vec.y(), vec.z()};
}

Eigen::Vector3f jolt_to_eigen(const Vec3 &vec) {
  return Eigen::Vector3f{vec.GetX(), vec.GetY(), vec.GetZ()};
}

Quat eigen_to_jolt(const Eigen::Quaternionf &quat) {
  return Quat{quat.x(), quat.y(), quat.z(), quat.w()};
}

Eigen::Quaternionf jolt_to_eigen(const Quat &quat) {
  return Eigen::Quaternionf{quat.GetW(), quat.GetX(), quat.GetY(), quat.GetZ()};
}

struct PhysicsSystemInterface {

  static constexpr uint MAX_BODIES = 10240;
  static constexpr uint MAX_BODY_MUTEXES = 0;
  static constexpr uint MAX_BODY_PAIRS = 65536;
  static constexpr uint MAX_CONTACT_CONSTRAINTS = 20480;

  /// Class that determines if two object layers can collide
  class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter {
  public:
    virtual bool ShouldCollide(ObjectLayer inObject1,
                               ObjectLayer inObject2) const override {
      switch (inObject1) {
      case Layers::NON_MOVING:
        return inObject2 ==
               Layers::MOVING; // Non moving only collides with moving
      case Layers::MOVING:
        return true; // Moving collides with everything
      default:
        JPH_ASSERT(false);
        return false;
      }
    }
  };

  // BroadPhaseLayerInterface implementation
  // This defines a mapping between object and broadphase layers.
  class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface {
  public:
    BPLayerInterfaceImpl() {
      // Create a mapping table from object to broad phase layer
      mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
      mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
    }

    virtual uint GetNumBroadPhaseLayers() const override {
      return BroadPhaseLayers::NUM_LAYERS;
    }

    virtual const char *
    GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override {
      switch ((BroadPhaseLayer::Type)inLayer) {
      case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:
        return "NON_MOVING";
      case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:
        return "MOVING";
      default:
        JPH_ASSERT(false);
        return "INVALID";
      }
    }

    virtual BroadPhaseLayer
    GetBroadPhaseLayer(ObjectLayer inLayer) const override {
      JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
      return mObjectToBroadPhase[inLayer];
    }

  private:
    BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
  };

  /// Class that determines if an object layer can collide with a broadphase
  /// layer
  class ObjectVsBroadPhaseLayerFilterImpl
      : public ObjectVsBroadPhaseLayerFilter {
  public:
    virtual bool ShouldCollide(ObjectLayer inLayer1,
                               BroadPhaseLayer inLayer2) const override {
      switch (inLayer1) {
      case Layers::NON_MOVING:
        return inLayer2 == BroadPhaseLayers::MOVING;
      case Layers::MOVING:
        return true;
      default:
        JPH_ASSERT(false);
        return false;
      }
    }
  };
  // Members

  PhysicsSystem *jph_physics_system; // The physics system
  BPLayerInterfaceImpl
      broad_phase_impl; // The broadphase layer interface that maps
                        // object layers to broadphase layers
  ObjectVsBroadPhaseLayerFilterImpl
      object_vs_broad_impl; // Class that filters object vs broadphase
                            // layers
  ObjectLayerPairFilterImpl
      object_vs_object_impl; // Class that filters object vs object layers

  JobSystem *job_system; // The job system that runs physics jobs

  float fixed_time_step = 1 / 100.0f; // The time step we're simulating
  float step_time_elapsed = 0.0f;     // Time since last update

  TempAllocatorImpl *temp_allocator;

  std::vector<Body *> registered_bodies;
  PhysicsSystemInterface() {
    RegisterDefaultAllocator();
    Factory::sInstance = new Factory();
    RegisterTypes();

    // We need a temp allocator for temporary allocations during the physics
    // update. We're pre-allocating 10 MB to avoid having to do allocations
    // during the physics update. B.t.w. 10 MB is way too much for this
    // example but it is a typical value you can use. If you don't want to
    // pre-allocate you can also use TempAllocatorMalloc to fall back to
    // malloc / free.
    temp_allocator = new TempAllocatorImpl(32 * 1024 * 1024);

    jph_physics_system = new PhysicsSystem{};
    job_system =
        new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, 4);
    jph_physics_system->Init(MAX_BODIES, MAX_BODY_MUTEXES, MAX_BODY_PAIRS,
                             MAX_CONTACT_CONSTRAINTS, broad_phase_impl,
                             object_vs_broad_impl, object_vs_object_impl);
    jph_physics_system->SetPhysicsSettings(PhysicsSettings{});
  }

  PhysicsSystemInterface(const PhysicsSystemInterface &) = delete;

  void add_body(const Transform &transform, Rigidbody &rigidbody, float mass,
                bool static_body) {
    BodyInterface &body_interface = jph_physics_system->GetBodyInterface();

    ShapeSettings::ShapeResult body_shape_result;
    if (rigidbody.primitive_type == Rigidbody::PrimitiveType::SPHERE) {
      SphereShapeSettings body_settings{rigidbody.dimensions[0]};
      body_shape_result = body_settings.Create();
    } else if (rigidbody.primitive_type == Rigidbody::PrimitiveType::BOX) {
      BoxShapeSettings body_settings{Vec3{rigidbody.dimensions[0],
                                          rigidbody.dimensions[1],
                                          rigidbody.dimensions[2]}};
      body_shape_result = body_settings.Create();
    }

    BodyCreationSettings body_physics_settings(
        body_shape_result.Get(), Vec3::sZero(), Quat::sIdentity(),
        static_body ? EMotionType::Static : EMotionType::Dynamic,
        static_body ? Layers::NON_MOVING : Layers::MOVING);

    body_physics_settings.mOverrideMassProperties =
        EOverrideMassProperties::CalculateInertia;
    body_physics_settings.mMassPropertiesOverride.mMass = mass;

    Body *p_body = body_interface.CreateBody(body_physics_settings);
    rigidbody.body_id = p_body->GetID();
    body_interface.AddBody(p_body->GetID(), EActivation::Activate);

    body_interface.SetPositionAndRotation(
        p_body->GetID(), eigen_to_jolt(transform.position),
        eigen_to_jolt(transform.rotation), EActivation::Activate);
    body_interface.SetLinearVelocity(p_body->GetID(), Vec3::sZero());

    registered_bodies.push_back(p_body);
  }

  void update(float dt) {
    step_time_elapsed += dt;
    if (step_time_elapsed < fixed_time_step) {
      return;
    }
    int steps_to_take = step_time_elapsed / fixed_time_step;
    for (int step = 0; step < steps_to_take; ++step) {
      EPhysicsUpdateError err =
          jph_physics_system->Update(dt, 1, temp_allocator, job_system);
      if (err != EPhysicsUpdateError::None) {
        throw std::runtime_error("Physics update error");
      }
    }
    step_time_elapsed -= steps_to_take * fixed_time_step;
  }

  Eigen::Vector3f get_body_position(BodyID body_id) {
    BodyInterface &body_interface = jph_physics_system->GetBodyInterface();
    return jolt_to_eigen(body_interface.GetPosition(body_id));
  }

  Eigen::Vector3f get_body_com_position(BodyID body_id) {
    BodyInterface &body_interface = jph_physics_system->GetBodyInterface();
    return jolt_to_eigen(body_interface.GetCenterOfMassPosition(body_id));
  }

  Eigen::Quaternionf get_body_orientation(BodyID body_id) {
    BodyInterface &body_interface = jph_physics_system->GetBodyInterface();
    return jolt_to_eigen(body_interface.GetRotation(body_id));
  }

  ~PhysicsSystemInterface() {
    BodyInterface &body_interface = jph_physics_system->GetBodyInterface();
    for (Body *body : registered_bodies) {
      BodyID body_id = body->GetID();
      body_interface.RemoveBody(body_id);
      body_interface.DestroyBody(body_id);
    }
    delete job_system;
    delete temp_allocator;
    delete jph_physics_system;
    UnregisterTypes();
    delete Factory::sInstance;
  }
};
} // namespace JPH

JPH::PhysicsSystemInterface &get_jph_physics_interface() {
  static JPH::PhysicsSystemInterface interface {};
  return interface;
}

namespace rend::systems {
struct PhysicsSystem : public System {
  // Go through all the bodies and add them to the JPH physics system
  void init() {
    rend::ECS::EntityRegistry &registry = rend::ECS::get_entity_registry();
    JPH::PhysicsSystemInterface &physics_interface =
        get_jph_physics_interface();

    for (rend::ECS::EntityRegistry::ArchetypeIterator rb_iterator =
             registry.archetype_iterator<Rigidbody, Transform>();
         rb_iterator.valid(); ++rb_iterator) {
      rend::ECS::EID eid = *rb_iterator;
      Rigidbody &rb = registry.get_component<Rigidbody>(eid);
      Transform &transform = registry.get_component<Transform>(eid);
      physics_interface.add_body(transform, rb, rb.mass, rb.static_body);
    }
  }

  virtual void update(float dt) {
    rend::ECS::EntityRegistry &registry = rend::ECS::get_entity_registry();
    // Update JPH physics system
    JPH::PhysicsSystemInterface &physics_interface =
        get_jph_physics_interface();
    JPH::BodyInterface &body_manager =
        physics_interface.jph_physics_system->GetBodyInterface();
    physics_interface.update(dt);
    // Update transforms
    for (rend::ECS::EntityRegistry::ArchetypeIterator rb_iterator =
             registry.archetype_iterator<Rigidbody, Transform>();
         rb_iterator.valid(); ++rb_iterator) {

      rend::ECS::EID eid = *rb_iterator;
      Rigidbody &rb = registry.get_component<Rigidbody>(eid);
      Transform &transform = registry.get_component<Transform>(eid);

      if (registry.is_component_enabled<AABB>(eid)) {
        AABB &aabb = registry.get_component<AABB>(eid);
        { // Update global frame AABBs
          Eigen::Matrix<float, 8, 4> vertices =
              (transform.get_model_matrix() *
               get_local_aabb_vertices(aabb).transpose())
                  .transpose();
          std::pair<Eigen::Vector3f, Eigen::Vector3f> span =
              compute_span(vertices.block<8, 3>(0, 0));
          aabb.min_global = span.first;
          aabb.max_global = span.second;
        }
      }

      Renderer &renderer = get_renderer();

      transform.position = physics_interface.get_body_position(rb.body_id) +
                           transform.rotation * rb.com_offset;
      transform.rotation = physics_interface.get_body_orientation(rb.body_id);

      renderer.draw_debug_sphere(transform.position, 1.0f, 8,
                                 Eigen::Vector3f{1.0f, 0.0f, 0.0f});
    }
  }
};
} // namespace rend::systems