#include <rend/Rendering/Vulkan/Renderer.h>

#include <rend/Object.h>
#include <rend/Rendering/Vulkan/Material.h>
#include <rend/Rendering/Vulkan/Mesh.h>
#include <rend/Rendering/Vulkan/Renderable.h>

#include <rend/Audio/AudioPlayer.h>
#include <rend/EntityRegistry.h>
#include <rend/InputHandler.h>

#include <rend/TimeUtils.h>

int main() {
  Renderer renderer{};
  AudioPlayer audio_player{};
  audio_player.load(Path{ASSET_DIRECTORY} / Path{"audio/dingus.mp3"});
  audio_player.loop = true;

  if (!renderer.init()) {
    return 1;
  }

  InputHandler input_handler;

  ECS::EntityRegistry &registry = ECS::EntityRegistry::get_entity_registry();
  registry.register_component<Object>();
  registry.register_component<Renderable>();

  ECS::EID dingus1 = registry.register_entity();
  ECS::EID dingus2 = registry.register_entity();
  ECS::EID cube = registry.register_entity();

  registry.add_component<Object>(dingus1);
  registry.add_component<Object>(dingus2);
  registry.add_component<Object>(cube);

  registry.add_component<Renderable>(dingus1);
  registry.add_component<Renderable>(dingus2);
  registry.add_component<Renderable>(cube);

  Renderable &dingus1_renderable = registry.get_component<Renderable>(dingus1);
  Renderable &dingus2_renderable = registry.get_component<Renderable>(dingus2);
  Renderable &cube_renderable = registry.get_component<Renderable>(cube);

  dingus1_renderable.p_mesh =
      std::make_shared<Mesh>(Path{ASSET_DIRECTORY} / Path{"models/dingus.fbx"});
  dingus1_renderable.p_material = std::make_shared<Material>(
      Path{}, Path{}, Path{ASSET_DIRECTORY} / "textures/dingus_nowhiskers.jpg",
      std::vector<VkFormat>{VK_FORMAT_R32G32B32_SFLOAT,
                            VK_FORMAT_R32G32B32_SFLOAT,
                            VK_FORMAT_R32G32_SFLOAT});

  dingus2_renderable.p_mesh = dingus1_renderable.p_mesh;
  dingus2_renderable.p_material = dingus1_renderable.p_material;

  cube_renderable.p_mesh =
      std::make_shared<Mesh>(Path{ASSET_DIRECTORY} / Path{"models/cube.obj"});
  cube_renderable.p_material = std::make_shared<Material>(
      Path{Path{ASSET_DIRECTORY} / "shaders/bin/light_vert.spv"},
      Path{Path{ASSET_DIRECTORY} / "shaders/bin/light_frag.spv"}, Path{},
      std::vector<VkFormat>{VK_FORMAT_R32G32B32_SFLOAT,
                            VK_FORMAT_R32G32B32_SFLOAT,
                            VK_FORMAT_R32G32_SFLOAT});

  Object &dingus1_object = registry.get_component<Object>(dingus1);
  Object &dingus2_object = registry.get_component<Object>(dingus2);
  Object &cube_object = registry.get_component<Object>(cube);

  dingus1_object.scale = Eigen::Vector3f::Ones() * 0.5f;
  dingus2_object.scale = Eigen::Vector3f::Ones() * 0.2f;
  cube_object.scale = Eigen::Vector3f::Ones() * 0.2f;

  dingus1_object.rotation =
      Eigen::Quaternionf{Eigen::AngleAxisf{-M_PI_2, Eigen::Vector3f::UnitX()}};
  dingus2_object.position = Eigen::Vector3f{0, 12, 0.0f};

  renderer.load_renderable(dingus1);
  renderer.load_renderable(dingus2);
  renderer.load_renderable(cube);

  audio_player.play();

  renderer.lights[0].color = Eigen::Vector4f::Ones();

  renderer.camera->position = Eigen::Vector3f{0.0f, 10.0f, -20.0f};
  Time::TimePoint t1 = Time::now();

  double cam_pitch = 0, cam_yaw = 0;
  while (input_handler.poll() && renderer.draw()) {
    Time::TimePoint t2 = Time::now();
    float time = Time::time_difference<Time::Seconds>(t2, t1);

    renderer.lights[0].position =
        Eigen::Vector3f{10.0f * std::sin(time), 10.0f, 10.0f * std::cos(time)};
    cube_object.position = renderer.lights[0].position;

    dingus1_object.position =
        Eigen::Vector3f{20 * std::cos(time), 0.5f + std::cos(time), 0.0f};
    dingus1_object.rotation = Eigen::Quaternionf{
        Eigen::AngleAxisf{2 * time, Eigen::Vector3f::UnitY()} *
        Eigen::AngleAxisf{-M_PI_2, Eigen::Vector3f::UnitX()}};
    dingus2_object.rotation = Eigen::Quaternionf{
        Eigen::AngleAxisf{-3 * time, Eigen::Vector3f::UnitX()} *
        Eigen::AngleAxisf{M_PI, Eigen::Vector3f::UnitY()}};

    renderer.camera->position +=
        input_handler.is_key_held(KeyCode::W) * renderer.camera->forward() +
        input_handler.is_key_held(KeyCode::S) * -renderer.camera->forward() +
        input_handler.is_key_held(KeyCode::D) * renderer.camera->right() +
        input_handler.is_key_held(KeyCode::A) * -renderer.camera->right();

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