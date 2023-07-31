#include <rend/Rendering/Vulkan/Renderer.h>

#include <rend/Rendering/Vulkan/Material.h>
#include <rend/Rendering/Vulkan/Mesh.h>
#include <rend/Rendering/Vulkan/Renderable.h>
#include <rend/Transform.h>

#include <rend/Audio/AudioPlayer.h>
#include <rend/EntityRegistry.h>
#include <rend/Systems/DebugBufferFillSystem.h>
#include <rend/Systems/PhysicsSystem.h>

#include <rend/InputHandler.h>

#include <rend/TimeUtils.h>

namespace rend {
void register_components() {
  rend::ECS::EntityRegistry &registry = rend::ECS::get_entity_registry();
  registry.register_component<Transform>();
  registry.register_component<Renderable>();
  registry.register_component<Rigidbody>();
  registry.register_component<AABB>();
}

enum class Primitive { DINGUS, BOX, SPHERE };

ECS::EID create_primitive(Eigen::Vector3f position, Eigen::Quaternionf rotation,
                          Eigen::Vector3f scale, Texture::Ptr p_texture,
                          Primitive primitive, bool static_body = false,
                          bool reflective = false) {

  rend::ECS::EntityRegistry &registry = rend::ECS::get_entity_registry();
  ECS::EID eid = registry.register_entity();
  Transform &transform = registry.add_component<Transform>(eid);
  Renderable &renderable = registry.add_component<Renderable>(eid);
  Rigidbody &rigidbody = registry.add_component<Rigidbody>(eid);
  renderable.reflective = reflective;
  renderable.p_texture = p_texture;
  renderable.type = RenderableType::Geometry;

  transform.position = position;
  transform.rotation = rotation;
  transform.scale = scale;

  if (primitive == Primitive::DINGUS) {
    // TODO: Replace with capsule
    renderable.p_mesh = Primitives::get_default_dingus_mesh();
    rigidbody.primitive_type = Rigidbody::PrimitiveType::BOX;
  }

  if (primitive == Primitive::BOX) {
    renderable.p_mesh = Primitives::get_default_cube_mesh();
    rigidbody.primitive_type = Rigidbody::PrimitiveType::BOX;
  }

  if (primitive == Primitive::SPHERE) {
    renderable.p_mesh = Primitives::get_default_sphere_mesh();
    rigidbody.primitive_type = Rigidbody::PrimitiveType::SPHERE;
    rigidbody.dimensions = transform.scale;
  }

  if (primitive == Primitive::DINGUS || primitive == Primitive::BOX) {
    AABB &aabb = registry.add_component<AABB>(eid);
    aabb = AABB(*renderable.p_mesh);
    rigidbody.dimensions =
        transform.scale.cwiseProduct(aabb.max_local - aabb.min_local) / 2;
  }

  rigidbody.static_body = static_body;
  return eid;
}

} // namespace rend

int main() {
  rend::Renderer &renderer = rend::get_renderer();
  rend::AudioPlayer audio_player{};
  rend::systems::PhysicsSystem physics_system{};
  rend::systems::DebugBufferFillSystem debug_buffer_fill_system{};

  audio_player.load(Path{ASSET_DIRECTORY} / Path{"audio/dingus.mp3"});
  audio_player.loop = true;

  renderer.init();

  rend::input::InputHandler input_handler;

  rend::register_components();

  Texture::Ptr dingus_texture = std::make_shared<Texture>(
      Path{ASSET_DIRECTORY} / "textures/dingus_nowhiskers.jpg");

  Eigen::Quaternionf initial_dingus_rotation =
      Eigen::Quaternionf{Eigen::AngleAxisf{-M_PI_2, Eigen::Vector3f::UnitX()}};

  rend::ECS::EID dingus1_eid = create_primitive(
      Eigen::Vector3f{-20, 40, 0},
      Eigen::Quaternionf{Eigen::AngleAxisf{M_PI_2, Eigen::Vector3f::UnitY()}} *
          initial_dingus_rotation,
      Eigen::Vector3f::Ones() * 0.5f, dingus_texture, rend::Primitive::DINGUS);

  rend::ECS::EID dingus2_eid = create_primitive(
      Eigen::Vector3f{20, 20, 0},
      Eigen::Quaternionf{
          Eigen::AngleAxisf{M_PI * (3 / 2), Eigen::Vector3f::UnitY()}} *
          initial_dingus_rotation,
      Eigen::Vector3f::Ones() * 0.5f, dingus_texture, rend::Primitive::DINGUS);

  rend::ECS::EID sphere = create_primitive(
      Eigen::Vector3f{0, 20, 30},
      Eigen::Quaternionf{Eigen::AngleAxisf{M_PI_2, Eigen::Vector3f::UnitY()}},
      Eigen::Vector3f::Ones() * 10, dingus_texture, rend::Primitive::SPHERE);

  rend::ECS::EID floor_eid = create_primitive(
      Eigen::Vector3f{0, 0, 0}, Eigen::Quaternionf::Identity(),
      Eigen::Vector3f{50.0f, 1.0f, 50.0f}, Texture::get_error_texture(),
      rend::Primitive::BOX, true, true);

  // audio_player.play();

  int N_LIGHTS = 32;
  int light_speeds[N_LIGHTS];

  for (int i = 0; i < N_LIGHTS; i++) {
    LightSource &light = renderer.lights[i];
    light.enable();
    light.set_color(Eigen::Vector3f{rand_float(0.2, 1), rand_float(0.2, 1),
                                    rand_float(0.2, 1)});
    light.set_position(
        Eigen::Vector3f{rand_float(-50, 50), 10.0f, rand_float(-50, 50)});
    light.set_fov(M_PI * 0.3f);
    light.set_direction(-light.get_position().normalized());
    light_speeds[i] = rand_float(-100, 100);
    light.set_intensity(0.1);
  }

  renderer.camera->position = Eigen::Vector3f{25, 12, -24};

  bool draw_debug = false;
  physics_system.init();
  rend::time::TimePoint t1 = rend::time::now();
  rend::time::TimePoint prev_time = t1;
  float cam_pitch = 0, cam_yaw = -M_PI_4;
  float dt = 0;
  while (input_handler.poll(dt)) {
    rend::time::TimePoint t2 = rend::time::now();
    float time = rend::time::time_difference<rend::time::Seconds>(t2, t1);
    dt = rend::time::time_difference<rend::time::Seconds>(t2, prev_time);
    prev_time = t2;

    physics_system.update(dt);
    debug_buffer_fill_system.update(dt);

    if (input_handler.is_key_pressed(rend::input::KeyCode::F)) {
      renderer.debug_mode = !renderer.debug_mode;
    }

    if (input_handler.is_key_pressed(rend::input::KeyCode::TAB)) {
      renderer.show_gui = !renderer.show_gui;
    }

    for (int i = 0; i < N_LIGHTS; i++) {
      LightSource &light = renderer.lights[i];
      Eigen::Vector3f light_right_vec = light.get_view_mat().col(0).head<3>();
      Eigen::Vector3f light_position = light.get_position();
      light.set_position(light_position +
                         light_speeds[i] * dt * light_right_vec);
      light.set_direction(-light.get_position().normalized());
    }

    if (!renderer.show_gui) {
      renderer.camera->position +=
          input_handler.is_key_held(rend::input::KeyCode::W) *
              renderer.camera->forward() +
          input_handler.is_key_held(rend::input::KeyCode::S) *
              -renderer.camera->forward() +
          input_handler.is_key_held(rend::input::KeyCode::D) *
              renderer.camera->right() +
          input_handler.is_key_held(rend::input::KeyCode::A) *
              -renderer.camera->right();

      cam_pitch += input_handler.m_dy * 0.01f;
      cam_yaw += input_handler.m_dx * 0.01f;
      cam_pitch = CLAMP(cam_pitch, -M_PI_2, M_PI_2);

      renderer.camera->rotation = Eigen::Quaternionf{
          Eigen::AngleAxisf{cam_yaw, Eigen::Vector3f::UnitY()} *
          Eigen::AngleAxisf{cam_pitch, Eigen::Vector3f::UnitX()}};
    }

    renderer.draw();
  }

  renderer.cleanup();
  return 0;
}