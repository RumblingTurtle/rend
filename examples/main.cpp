#include <rend/Rendering/Vulkan/Renderer.h>

#include <rend/Object.h>
#include <rend/Rendering/Vulkan/Material.h>
#include <rend/Rendering/Vulkan/Mesh.h>
#include <rend/Rendering/Vulkan/Renderable.h>

#include <rend/Audio/AudioPlayer.h>
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

  std::shared_ptr<Mesh> mesh_dingus = std::make_shared<Mesh>();
  std::shared_ptr<Mesh> mesh_cube = std::make_shared<Mesh>();

  Renderable::Ptr r_dingus_1 = std::make_shared<Renderable>();
  Renderable::Ptr r_dingus_2 = std::make_shared<Renderable>();
  Renderable::Ptr r_cube = std::make_shared<Renderable>();

  mesh_dingus->load(Path{ASSET_DIRECTORY} / Path{"models/dingus.fbx"});
  mesh_cube->load(Path{ASSET_DIRECTORY} / Path{"models/cube.obj"});

  std::shared_ptr<Material> mat_dingus = std::make_shared<Material>(
      Path{}, Path{}, Path{ASSET_DIRECTORY} / "textures/dingus_nowhiskers.jpg");

  std::shared_ptr<Material> light_mat = std::make_shared<Material>(
      Path{Path{ASSET_DIRECTORY} / "shaders/bin/light_vert.spv"},
      Path{Path{ASSET_DIRECTORY} / "shaders/bin/light_frag.spv"}, Path{});

  r_dingus_1->p_material = mat_dingus;
  r_dingus_2->p_material = mat_dingus;
  r_cube->p_material = light_mat;

  r_dingus_1->p_mesh = mesh_dingus;
  r_dingus_2->p_mesh = mesh_dingus;
  r_cube->p_mesh = mesh_cube;

  r_dingus_1->object.scale = Eigen::Vector3f::Ones() * 0.5f;
  r_dingus_2->object.scale = Eigen::Vector3f::Ones() * 0.2f;
  r_cube->object.scale = Eigen::Vector3f::Ones() * 0.2f;

  r_dingus_2->object.rotation =
      Eigen::Quaternionf{Eigen::AngleAxisf{-M_PI_2, Eigen::Vector3f::UnitX()}};
  r_dingus_2->object.position = Eigen::Vector3f{0, 12, 0.0f};

  renderer.load_renderable(r_dingus_1);
  renderer.load_renderable(r_dingus_2);
  renderer.load_renderable(r_cube);

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
    r_cube->object.position = renderer.lights[0].position;

    r_dingus_1->object.position =
        Eigen::Vector3f{20 * std::cos(time), 0.5f + std::cos(time), 0.0f};
    r_dingus_1->object.rotation = Eigen::Quaternionf{
        Eigen::AngleAxisf{2 * time, Eigen::Vector3f::UnitY()} *
        Eigen::AngleAxisf{-M_PI_2, Eigen::Vector3f::UnitX()}};
    r_dingus_2->object.rotation = Eigen::Quaternionf{
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

  return 0;
}