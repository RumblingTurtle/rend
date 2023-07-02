#include <Eigen/Dense>
#include <gtest/gtest.h>
#include <rend/EntityRegistry.h>
#include <rend/Object.h>

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
} // namespace

namespace {
class RegisterEntity : public ::testing::Test {
protected:
  ECS::EntityRegistry *registry;
  ECS::EID eid;
  void SetUp() override {
    registry = &ECS::EntityRegistry::get_entity_registry();
    registry->register_component<Object>();
    eid = registry->register_entity();
  }
};

TEST_F(RegisterEntity, EnabledFlagActiveTest) {
  ASSERT_TRUE(registry->entities[eid].is_entity_enabled());
}

TEST_F(RegisterEntity, ComponentEnabledTest) {
  registry->add_component<Object>(eid);

  ASSERT_TRUE(registry->is_component_enabled<Object>(eid));
}

TEST_F(RegisterEntity, ComponentInitializedTest) {
  registry->add_component<Object>(eid);
  Eigen::Vector3f position = registry->get_component<Object>(eid).position;
  ASSERT_TRUE(position.isZero())
      << "Position is not initialized to zero" << position;
}

TEST_F(RegisterEntity, MaxEntityAmountTest) {
  EXPECT_THROW(
      [&]() {
        for (int i = 0; i < ECS::MAX_ENTITIES; i++) {
          registry->register_entity();
        }
      }(),
      std::runtime_error);
}

}; // namespace
