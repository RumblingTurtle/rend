#include <InputHandler.h>
#include <Material.h>
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
  std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();
  mesh->load(Path{ASSET_DIRECTORY} / Path{"models/dingus.fbx"});

  std::shared_ptr<Material> mat = std::make_shared<Material>(
      Path{}, Path{}, Path{ASSET_DIRECTORY} / "textures/dingus_nowhiskers.jpg");

  Renderable::Ptr renderable1 = std::make_shared<Renderable>();
  renderable1->p_material = mat;
  renderable1->p_mesh = mesh;
  renderable1->object.scale = Eigen::Vector3f{0.5f, 0.5f, 0.5f};

  Renderable::Ptr renderable2 = std::make_shared<Renderable>();
  renderable2->p_material = mat;
  renderable2->p_mesh = mesh;
  renderable2->object.rotation =
      Eigen::Quaternionf{Eigen::AngleAxisf{-M_PI_2, Eigen::Vector3f::UnitX()}};
  renderable2->object.scale = Eigen::Vector3f{0.2f, 0.2f, 0.2f};
  renderable2->object.position = Eigen::Vector3f{0, 12, 0.0f};

  renderer.load_renderable(renderable1);
  renderer.load_renderable(renderable2);

  while (input_handler.poll() && renderer.draw()) {
    std::chrono::high_resolution_clock::time_point t2 =
        std::chrono::high_resolution_clock::now();
    float time =
        std::chrono::duration_cast<std::chrono::duration<float>>(t2 - t1)
            .count();
    renderable1->object.position =
        Eigen::Vector3f{20 * std::cos(time), 0.5f + std::cos(time), 0.0f};
    renderable1->object.rotation = Eigen::Quaternionf{
        Eigen::AngleAxisf{2 * time, Eigen::Vector3f::UnitY()} *
        Eigen::AngleAxisf{-M_PI_2, Eigen::Vector3f::UnitX()}};

    renderable2->object.rotation = Eigen::Quaternionf{
        Eigen::AngleAxisf{-3 * time, Eigen::Vector3f::UnitX()} *
        Eigen::AngleAxisf{M_PI, Eigen::Vector3f::UnitY()}};

    renderer.camera->position = Eigen::Vector3f{0.0f, 10.0f, -20.0f};
    renderer.camera->lookat(renderable1->object.position);
  }

  return 0;
}