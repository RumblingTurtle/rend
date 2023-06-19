#include <InputHandler.h>
#include <Mesh.h>
#include <Object.h>
#include <Renderer.h>
#include <chrono>

int main() {
  Renderer renderer{};

  if (!renderer.init()) {
    return 1;
  }
  std::chrono::high_resolution_clock::time_point t1 =
      std::chrono::high_resolution_clock::now();

  InputHandler input_handler;
  std::shared_ptr<Mesh> m = std::make_shared<Mesh>();
  std::shared_ptr<Object> o = std::make_shared<Object>();
  m->load(Path{ASSET_DIRECTORY} / Path{"models/dingus.fbx"});
  renderer.load_mesh(m, o);
  o->_scale = Eigen::Vector3f{0.5f, 0.5f, 0.5f};

  std::shared_ptr<Mesh> m2 = std::make_shared<Mesh>();
  m2->load(Path{ASSET_DIRECTORY} / Path{"models/dingus.fbx"});
  std::shared_ptr<Object> o2 = std::make_shared<Object>();
  renderer.load_mesh(m2, o2);
  o2->_rotation =
      Eigen::Quaternionf{Eigen::AngleAxisf{-M_PI_2, Eigen::Vector3f::UnitX()}};
  o2->_scale = Eigen::Vector3f{0.2f, 0.2f, 0.2f};

  while (input_handler.poll() && renderer.draw()) {
    std::chrono::high_resolution_clock::time_point t2 =
        std::chrono::high_resolution_clock::now();
    float time =
        std::chrono::duration_cast<std::chrono::duration<float>>(t2 - t1)
            .count();
    o->_rotation = Eigen::Quaternionf{
        Eigen::AngleAxisf{time, Eigen::Vector3f::UnitY()} *
        Eigen::AngleAxisf{-M_PI_2, Eigen::Vector3f::UnitX()}};

    o->_position =
        Eigen::Vector3f{20 * std::cos(time), 0.5f + std::cos(time), 0.0f};

    renderer._camera->_position = Eigen::Vector3f{0.0f, 5.0f, -20.0f};
    renderer._camera->lookat(o->_position);
  }

  return 0;
}