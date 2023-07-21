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

void register_components(rend::ECS::EntityRegistry &registry) {
  registry.register_component<Transform>();
  registry.register_component<Renderable>();
  registry.register_component<Rigidbody>();
  registry.register_component<AABB>();
}

int main() {
  rend::Renderer &renderer = rend::get_renderer();
  rend::ECS::EntityRegistry &registry = rend::ECS::get_entity_registry();
  rend::AudioPlayer audio_player{};
  rend::systems::PhysicsSystem physics_system{};
  rend::systems::DebugBufferFillSystem debug_buffer_fill_system{};

  audio_player.load(Path{ASSET_DIRECTORY} / Path{"audio/dingus.mp3"});
  audio_player.loop = true;

  if (!renderer.init()) {
    return 1;
  }
  rend::input::InputHandler input_handler;

  register_components(registry);

  // Cat 1
  rend::ECS::EID dingus1 = registry.register_entity();
  Transform &dingus1_transform = registry.add_component<Transform>(dingus1);
  Renderable &dingus1_renderable = registry.add_component<Renderable>(dingus1);
  Rigidbody &dingus1_rigidbody = registry.add_component<Rigidbody>(dingus1);
  AABB &dingus1_aabb = registry.add_component<AABB>(dingus1);

  dingus1_renderable.p_mesh =
      std::make_shared<Mesh>(Path{ASSET_DIRECTORY} / Path{"models/dingus.fbx"});
  dingus1_renderable.type = RenderableType::Geometry;

  dingus1_renderable.p_texture = std::make_shared<Texture>(
      Path{ASSET_DIRECTORY} / "textures/dingus_nowhiskers.jpg");

  dingus1_transform.scale = Eigen::Vector3f::Ones() * 0.5f;

  dingus1_aabb = AABB(*dingus1_renderable.p_mesh);

  dingus1_transform.position = Eigen::Vector3f{-1, 40, 0.0f};
  dingus1_rigidbody.dimensions =
      dingus1_transform.scale.cwiseProduct(dingus1_aabb.max_local -
                                           dingus1_aabb.min_local) /
      2;

  // Cat 2
  rend::ECS::EID dingus2 = registry.register_entity();
  Transform &dingus2_transform = registry.add_component<Transform>(dingus2);
  Renderable &dingus2_renderable = registry.add_component<Renderable>(dingus2);
  Rigidbody &dingus2_rigidbody = registry.add_component<Rigidbody>(dingus2);
  AABB &dingus2_aabb = registry.add_component<AABB>(dingus2);

  dingus2_renderable.p_mesh = dingus1_renderable.p_mesh;
  dingus2_renderable.type = RenderableType::Geometry;
  dingus2_renderable.p_texture = dingus1_renderable.p_texture;

  dingus2_transform.scale = Eigen::Vector3f::Ones() * 0.2f;

  dingus2_aabb = dingus1_aabb;

  dingus2_transform.position = Eigen::Vector3f{0, 20, 0.0f};

  dingus2_rigidbody.primitive_type = Rigidbody::PrimitiveType::BOX;
  dingus2_rigidbody.dimensions =
      dingus2_transform.scale.cwiseProduct(dingus2_aabb.max_local -
                                           dingus2_aabb.min_local) /
      2;

  // Sphere
  rend::ECS::EID sphere = registry.register_entity();
  Transform &sphere_transform = registry.add_component<Transform>(sphere);
  Renderable &sphere_renderable = registry.add_component<Renderable>(sphere);
  Rigidbody &sphere_rigidbody = registry.add_component<Rigidbody>(sphere);

  sphere_renderable.p_mesh =
      std::make_shared<Mesh>(Path{ASSET_DIRECTORY} / Path{"models/sphere.obj"});
  sphere_renderable.type = RenderableType::Geometry;
  sphere_renderable.p_texture = dingus1_renderable.p_texture;

  sphere_transform.scale = Eigen::Vector3f::Ones() * 5.0f;
  sphere_transform.position = Eigen::Vector3f{-20, 5, 0.0f};

  sphere_rigidbody.primitive_type = Rigidbody::PrimitiveType::SPHERE;
  sphere_rigidbody.dimensions = sphere_transform.scale;

  // Light cube
  rend::ECS::EID cube = registry.register_entity();
  Transform &cube_transform = registry.add_component<Transform>(cube);
  Renderable &cube_renderable = registry.add_component<Renderable>(cube);

  cube_renderable.p_mesh = Primitives::get_default_cube_mesh();

  cube_renderable.type = RenderableType::Light;
  cube_renderable.p_texture = Texture::get_error_texture();

  cube_transform.scale = Eigen::Vector3f::Ones() * 0.2f;
  cube_transform.scale.z() *= 2;

  // Floor
  rend::ECS::EID floor_cube = registry.register_entity();
  Transform &floor_transform = registry.add_component<Transform>(floor_cube);
  Renderable &floor_renderable = registry.add_component<Renderable>(floor_cube);
  Rigidbody &floor_rigidbody = registry.add_component<Rigidbody>(floor_cube);
  AABB &floor_aabb = registry.add_component<AABB>(floor_cube);

  floor_renderable.p_mesh = cube_renderable.p_mesh;
  sphere_renderable.type = RenderableType::Geometry;

  floor_aabb = AABB(*floor_renderable.p_mesh);

  floor_transform.scale = Eigen::Vector3f{50.0f, 0.1f, 50.0f};

  floor_transform.position = Eigen::Vector3f{0.0f, -10.0f, 0.0f};
  floor_rigidbody.static_body = true;
  floor_rigidbody.dimensions =
      floor_transform.scale.cwiseProduct(floor_aabb.max_local -
                                         floor_aabb.min_local) /
      2;

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

  renderer.camera->position = Eigen::Vector3f{0.0f, 10.0f, -20.0f};

  bool draw_debug = false;
  physics_system.init();
  rend::time::TimePoint t1 = rend::time::now();
  rend::time::TimePoint prev_time = t1;
  double cam_pitch = 0, cam_yaw = 0;
  float dt = 0;
  while (input_handler.poll(dt)) {
    rend::time::TimePoint t2 = rend::time::now();
    float time = rend::time::time_difference<rend::time::Seconds>(t2, t1);
    dt = rend::time::time_difference<rend::time::Seconds>(t2, prev_time);
    prev_time = t2;

    physics_system.update(dt);

    if (draw_debug) {
      debug_buffer_fill_system.update(dt);
    }
    if (input_handler.is_key_pressed(rend::input::KeyCode::F)) {
      draw_debug = !draw_debug;
    }

    for (int i = 0; i < N_LIGHTS; i++) {
      LightSource &light = renderer.lights[i];
      Eigen::Vector3f light_right_vec = light.get_view_mat().col(0).head<3>();
      Eigen::Vector3f light_position = light.get_position();
      light.set_position(light_position +
                         light_speeds[i] * dt * light_right_vec);
      light.set_direction(-light.get_position().normalized());
      if (i == 0) {
        cube_transform.position = light.get_position();
      }
    }

    cube_transform.lookat(Eigen::Vector3f::Zero());

    // renderer.camera->position = renderer.lights[0].get_position();
    // renderer.camera->lookat(Eigen::Vector3f::Zero());

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

    renderer.draw();
  }

  renderer.cleanup();
  return 0;
}