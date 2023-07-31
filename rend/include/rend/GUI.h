#pragma once
#include <imgui.h>
#include <rend/EntityRegistry.h>
#include <rend/Rendering/Vulkan/Renderer.h>
#include <rend/components.h>

namespace rend {
struct GUI {
  const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

  GUI() {}
  void draw_entity_browser() {
    Renderer &renderer = get_renderer();
    if (!renderer.show_gui) {
      return;
    }
    ImGui::Begin("Entity browser", &renderer.show_gui,
                 ImGuiWindowFlags_MenuBar);

    static ImGuiTableFlags flags =
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX |
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchSame;

    ECS::EntityRegistry &registry = ECS::get_entity_registry();
    int column_count = 3;
    if (ImGui::BeginTable("Entities", column_count, flags,
                          ImVec2(0.0f, TEXT_BASE_HEIGHT * 7))) {
      ImGui::TableNextRow();

      ImGui::TableNextColumn();
      ImGui::TableHeader("Entity ID");

      ImGui::TableNextColumn();
      ImGui::TableHeader("Position");

      ImGui::TableNextColumn();
      ImGui::TableHeader("Quaternion");

      for (rend::ECS::EntityRegistry::ArchetypeIterator iterator =
               registry.archetype_iterator<Rigidbody, Transform>();
           iterator.valid(); ++iterator) {
        ImGui::TableNextRow();
        Transform transform = registry.get_component<Transform>(*iterator);

        ImGui::TableNextColumn();
        write_decimal(*iterator);

        ImGui::TableNextColumn();
        write_vector(transform.position);

        ImGui::TableNextColumn();
        write_quaternion(transform.rotation);
      }
      ImGui::EndTable();
    }
    ImGui::End();
  }

  void write_decimal(int decimal) { ImGui::Text("%d", decimal); }
  void write_quaternion(Eigen::Quaternionf quaternion) {
    ImGui::Text("%f, %f, %f, %f", quaternion.x(), quaternion.y(),
                quaternion.z(), quaternion.w());
  }

  void write_vector(Eigen::Vector3f vector) {
    ImGui::Text("%f, %f, %f", vector.x(), vector.y(), vector.z());
  }

  void draw() { draw_entity_browser(); }
};

GUI &get_GUI() {
  static GUI gui;
  return gui;
}
} // namespace rend