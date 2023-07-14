#include <rend/Rendering/Vulkan/Renderer.h>

#include <rend/Rendering/Vulkan/Material.h>
#include <rend/Rendering/Vulkan/Mesh.h>
#include <rend/Rendering/Vulkan/Renderable.h>
#include <rend/Transform.h>

#include <rend/Audio/AudioPlayer.h>
#include <rend/EntityRegistry.h>
#include <rend/Physics/PhysicsSystem.h>
#include <rend/Rendering/Vulkan/DebugBufferFillSystem.h>

#include <rend/InputHandler.h>

#include <rend/TimeUtils.h>

int main() {
  rend::Renderer &renderer = rend::get_renderer();
  rend::AudioPlayer audio_player{};
  rend::systems::PhysicsSystem physics_system{};
  rend::systems::DebugBufferFillSystem debug_buffer_fill_system{};

  audio_player.load(Path{ASSET_DIRECTORY} / Path{"audio/dingus.mp3"});
  audio_player.loop = true;

  if (!renderer.init()) {
    return 1;
  }

  rend::input::InputHandler input_handler;

  rend::ECS::EntityRegistry &registry = rend::ECS::get_entity_registry();
  registry.register_component<Transform>();
  registry.register_component<Renderable>();
  registry.register_component<AABB>();
  registry.register_component<Rigidbody>();

  rend::ECS::EID dingus1 = registry.register_entity();
  rend::ECS::EID dingus2 = registry.register_entity();
  rend::ECS::EID cube = registry.register_entity();

  rend::ECS::EID floor_cube = registry.register_entity();

  Transform &dingus1_transform = registry.add_component<Transform>(dingus1);
  Transform &dingus2_transform = registry.add_component<Transform>(dingus2);
  Transform &cube_transform = registry.add_component<Transform>(cube);
  Transform &floor_transform = registry.add_component<Transform>(floor_cube);

  Renderable &dingus1_renderable = registry.add_component<Renderable>(dingus1);
  Renderable &dingus2_renderable = registry.add_component<Renderable>(dingus2);
  Renderable &cube_renderable = registry.add_component<Renderable>(cube);
  Renderable &floor_renderable = registry.add_component<Renderable>(floor_cube);

  Rigidbody &dingus1_rigidbody = registry.add_component<Rigidbody>(dingus1);
  Rigidbody &dingus2_rigidbody = registry.add_component<Rigidbody>(dingus2);
  Rigidbody &floor_rigidbody = registry.add_component<Rigidbody>(floor_cube);

  dingus1_rigidbody.mass = 1;
  dingus2_rigidbody.mass = 100000;

  dingus1_renderable.p_mesh =
      std::make_shared<Mesh>(Path{ASSET_DIRECTORY} / Path{"models/dingus.fbx"});
  dingus1_renderable.p_material = std::make_shared<Material>(
      Path{}, Path{}, Path{ASSET_DIRECTORY} / "textures/dingus_nowhiskers.jpg",
      std::vector<VkFormat>{VK_FORMAT_R32G32B32_SFLOAT,
                            VK_FORMAT_R32G32B32_SFLOAT,
                            VK_FORMAT_R32G32_SFLOAT});

  dingus2_renderable.p_mesh = dingus1_renderable.p_mesh;
  dingus2_renderable.p_material = dingus1_renderable.p_material;

  cube_renderable.p_mesh = Primitives::get_default_cube_mesh();
  cube_renderable.p_material = std::make_shared<Material>(
      Path{Path{ASSET_DIRECTORY} / "shaders/bin/light_vert.spv"},
      Path{Path{ASSET_DIRECTORY} / "shaders/bin/light_frag.spv"}, Path{},
      std::vector<VkFormat>{VK_FORMAT_R32G32B32_SFLOAT,
                            VK_FORMAT_R32G32B32_SFLOAT,
                            VK_FORMAT_R32G32_SFLOAT});

  floor_renderable.p_mesh = Primitives::get_default_cube_mesh();
  floor_renderable.p_material = dingus1_renderable.p_material;

  dingus1_transform.scale = Eigen::Vector3f::Ones() * 0.5f;
  dingus2_transform.scale = Eigen::Vector3f::Ones() * 0.2f;
  cube_transform.scale = Eigen::Vector3f::Ones() * 0.2f;
  floor_transform.scale = Eigen::Vector3f{50.0f, 0.1f, 50.0f};

  floor_transform.position = Eigen::Vector3f{0.0f, -10.0f, 0.0f};
  floor_rigidbody.static_body = true;

  AABB &aabb1 = registry.add_component<AABB>(dingus1);
  aabb1 = AABB(*dingus1_renderable.p_mesh);

  AABB &aabb2 = registry.add_component<AABB>(dingus2);
  aabb2 = AABB(*dingus2_renderable.p_mesh);

  AABB &floor_aabb = registry.add_component<AABB>(floor_cube);
  floor_aabb = AABB(*floor_renderable.p_mesh);

  dingus1_transform.position = Eigen::Vector3f{0, 40, 0.0f};
  dingus2_transform.position = Eigen::Vector3f{0, 20, 0.0f};

  renderer.load_renderable(dingus1);
  renderer.load_renderable(dingus2);
  renderer.load_renderable(cube);
  renderer.load_renderable(floor_cube);

  // audio_player.play();

  renderer.lights[0].color = Eigen::Vector4f::Ones();
  renderer.camera->position = Eigen::Vector3f{0.0f, 10.0f, -20.0f};

  physics_system.init();
  rend::time::TimePoint t1 = rend::time::now();
  rend::time::TimePoint prev_time = t1;
  double cam_pitch = 0, cam_yaw = 0;
  while (input_handler.poll() && renderer.draw()) {
    rend::time::TimePoint t2 = rend::time::now();
    float time = rend::time::time_difference<rend::time::Seconds>(t2, t1);
    float dt = rend::time::time_difference<rend::time::Seconds>(t2, prev_time);
    prev_time = t2;

    physics_system.update(dt);
    debug_buffer_fill_system.update(dt);

    renderer.lights[0].position =
        Eigen::Vector3f{10.0f * std::sin(time), 10.0f, 10.0f * std::cos(time)};
    cube_transform.position = renderer.lights[0].position;

    if (false) {
      dingus1_transform.position =
          Eigen::Vector3f{20 * std::cos(time), 0.5f + std::cos(time), 0.0f};
      dingus1_transform.rotation = Eigen::Quaternionf{
          Eigen::AngleAxisf{2 * time, Eigen::Vector3f::UnitY()} *
          Eigen::AngleAxisf{-M_PI_2, Eigen::Vector3f::UnitX()}};
      dingus2_transform.rotation = Eigen::Quaternionf{
          Eigen::AngleAxisf{-3 * time, Eigen::Vector3f::UnitX()} *
          Eigen::AngleAxisf{M_PI, Eigen::Vector3f::UnitY()}};
    }

    renderer.camera->position +=
        input_handler.is_key_held(rend::input::KeyCode::W) *
            renderer.camera->forward() +
        input_handler.is_key_held(rend::input::KeyCode::S) *
            -renderer.camera->forward() +
        input_handler.is_key_held(rend::input::KeyCode::D) *
            renderer.camera->right() +
        input_handler.is_key_held(rend::input::KeyCode::A) *
            -renderer.camera->right();

    if (input_handler.mouse_moved) {
      cam_pitch += input_handler.m_dy * 0.01f;
      cam_yaw += input_handler.m_dx * 0.01f;
      cam_pitch = CLAMP(cam_pitch, -M_PI_2, M_PI_2);

      renderer.camera->rotation = Eigen::Quaternionf{
          Eigen::AngleAxisf{cam_yaw, Eigen::Vector3f::UnitY()} *
          Eigen::AngleAxisf{cam_pitch, Eigen::Vector3f::UnitX()}};
    }
  }

  renderer.cleanup();
  return 0;
}