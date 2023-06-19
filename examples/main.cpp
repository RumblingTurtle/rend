#include <InputHandler.h>
#include <Renderer.h>

int main() {
  Renderer renderer{};

  if (!renderer.init()) {
    return 1;
  }

  InputHandler input_handler;

  // Move polling to a different thread?
  while (input_handler.poll() && renderer.draw()) {
  }

  return 0;
}