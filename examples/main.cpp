#include <InputHandler.h>
#include <Mesh.h>
#include <Renderer.h>

int main() {
  Renderer renderer{};

  if (!renderer.init()) {
    return 1;
  }

  InputHandler input_handler;
  Mesh m;
  m.load(Path{ASSET_DIRECTORY} / Path{"models/dingus.fbx"});
  renderer.load_mesh(m);
  //  Move polling to a different thread?
  while (input_handler.poll() && renderer.draw()) {
  }

  return 0;
}