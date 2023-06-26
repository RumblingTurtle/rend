#include <ECS/EntityRegistry.h>
#include <Eigen/Dense>
#include <Object.h>
#include <gtest/gtest.h>

namespace {

class RegisterComponent : public ::testing::Test {
protected:
  ECS::EntityRegistry *registry;

  void SetUp() override {
    registry = &ECS::EntityRegistry::get_entity_registry();
    registry->register_component<Object>();
  }
};

TEST_F(RegisterComponent, CounterTest) {
  ASSERT_EQ(registry->registered_component_count, 1);
}

TEST_F(RegisterComponent, EntityFlagsAllocatedTest) {
  ASSERT_EQ(registry->entities.capacity(), ECS::MAX_ENTITIES);
}

class RegisterEntity : public ::testing::Test {
protected:
  ECS::EntityRegistry *registry;

  void SetUp() override {
    registry = &ECS::EntityRegistry::get_entity_registry();
    registry->register_component<Object>();
    ECS::EID eid = registry->register_entity();
  }
};

TEST_F(RegisterEntity, EnabledFlagActiveTest) {
  ECS::EID eid = registry->register_entity();
  ASSERT_TRUE(registry->entities[eid].mask.test(ECS::MAX_COMPONENTS));
}

TEST_F(RegisterEntity, ComponentInitializedTest) {
  registry->register_component<Object>();
  ECS::EID eid = registry->register_entity();
  registry->add_component<Object>(eid);
  Eigen::Vector3f position = registry->get_component<Object>(eid).position;
  ASSERT_TRUE(position.isZero())
      << "Position is not initialized to zero" << position;
}
}; // namespace
