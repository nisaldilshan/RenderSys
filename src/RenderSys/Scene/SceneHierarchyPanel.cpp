#include "SceneHierarchyPanel.h"
#include <RenderSys/Components/TransformComponent.h>
#include <RenderSys/Components/MeshComponent.h>
#include <RenderSys/Components/TagAndIDComponents.h>
#include <RenderSys/Components/LightComponents.h>
#include <RenderSys/Components/CameraComponents.h>

#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <filesystem>
#include <iostream>

/* The Microsoft C++ compiler is non-compliant with the C++ standard and needs
 * the following definition to disable a security warning on std::strncpy().
 */
#ifdef _MSVC_LANG
  #define _CRT_SECURE_NO_WARNINGS
#endif

namespace RenderSys
{
    extern const std::filesystem::path g_AssetPath;

    SceneHierarchyPanel::SceneHierarchyPanel(const std::shared_ptr<Scene> scene)
    {
        SetContext(scene);
    }

    void SceneHierarchyPanel::SetContext(const std::shared_ptr<Scene> scene)
    {
        m_Context = scene;
        m_SelectionContext = {};
    }

    void SceneHierarchyPanel::OnImGuiRender()
    {
        // Scene Hierarchy Panel
		ImGui::Begin("Scene Hierarchy");

		if (m_Context)
		{
            // Directly iterate through all entities in the registry
            // for (auto entity : m_Context->m_Registry.view<entt::entity>()) {
            //     DrawEntityNode(entity);
            // }
            DrawEntityNodes();

			if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
				m_SelectionContext = {};

			// Right click on blank space
			// if (ImGui::BeginPopupContextWindow(0, 1, false))
			// {
			// 	if (ImGui::MenuItem("Create Empty Entity"))
			// 		m_Context->CreateEntity("Empty Entity");

			// 	ImGui::EndPopup();
			// }
		}
		
        ImGui::End();

		// Properties Panel
        ImGui::Begin("Properties");

        if (m_SelectionContext != entt::null)
        {
            DrawComponents(m_SelectionContext);
        }

        ImGui::End();
    }
    
    void SceneHierarchyPanel::SetSelectedEntity(entt::entity entity)
    {
        m_SelectionContext = entity;
    }

    void SceneHierarchyPanel::DrawEntityNodes()
    {
        DrawEntityNodeRecursive(m_Context->GetSceneGraphTreeNode(m_Context->m_instancedRootNodeIndex));
    }

    void SceneHierarchyPanel::DrawEntityNodeRecursive(SceneGraph::TreeNode& node)
    {
        entt::entity entity = node.GetGameObject();
        if (m_Context->m_Registry.all_of<InstanceTagComponent>(entity))
        {
            return;
        }

        std::string tag = "NULL";
        if (m_Context->m_Registry.all_of<TagComponent>(entity))
        {
            tag = m_Context->m_Registry.get<TagComponent>(entity).Tag;
        }

        ImGuiTreeNodeFlags flags = (m_SelectionContext == entity ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
        flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

        // Check if entity has children
        bool hasChildren = false;
        if (node.GetChildren().size() > 0)
        {
            hasChildren = true;
        }

        if (!hasChildren)
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

        bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, "%s", tag.c_str());

        if (ImGui::IsItemClicked())
        {
            m_SelectionContext = entity;
        }

        bool entityDeleted = false;
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete Entity"))
                entityDeleted = true;

            ImGui::EndPopup();
        }

        if (opened && hasChildren)
        {
            auto& children = node.GetChildren();
            for (auto child : children)
            {
                DrawEntityNodeRecursive(m_Context->GetSceneGraphTreeNode(child));
            }
            ImGui::TreePop();
        }

        if (entityDeleted)
        {
            if (m_SelectionContext == entity)
                m_SelectionContext = {};
            m_Context->DestroyEntity(entity);
        }
    }

    static void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue, float columnWidth)
    {
        ImGuiIO& io = ImGui::GetIO();
        auto boldFont = io.Fonts->Fonts[0];

        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text("%s", label.c_str());
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
        ImGui::PushFont(boldFont);
        if (ImGui::Button("X", buttonSize))
            values.x = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##X", &values.x, 0.1f);
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.3f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.3f, 1.0f });
        ImGui::PushFont(boldFont);
        if (ImGui::Button("Y", buttonSize))
            values.y = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Y", &values.y, 0.1f);
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.2f, 0.8f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.3f, 0.9f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.2f, 0.8f, 1.0f });
        ImGui::PushFont(boldFont);
        if (ImGui::Button("Z", buttonSize))
            values.z = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Z", &values.z, 0.1f);
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        ImGui::Columns(1);

        ImGui::PopID();
    }

    template <typename T, typename UIFunction>
    static void DrawComponent(entt::registry& registry, const std::string& name, entt::entity entity, UIFunction uiFunction)
    {
		constexpr ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen |
		                                             ImGuiTreeNodeFlags_Framed |
                                                     ImGuiTreeNodeFlags_SpanAvailWidth |
		                                             ImGuiTreeNodeFlags_AllowItemOverlap |
                                                     ImGuiTreeNodeFlags_FramePadding;
		if (registry.all_of<T>(entity))
		{
            auto& component = registry.get<T>(entity);
            ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4,4 });
            float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
            ImGui::Separator();
            bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, "%s", name.c_str());
            ImGui::PopStyleVar();

            ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
            if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
            {
                ImGui::OpenPopup("ComponentSettings");
            }

            bool removeComponent = false;
            if (ImGui::BeginPopup("ComponentSettings"))
            {
                if (ImGui::MenuItem("Remove Component"))
                    removeComponent = true;

                ImGui::EndPopup();
            }

            if (open)
            {
                uiFunction(component);
                ImGui::TreePop();
            }

            if (removeComponent)
            {
                registry.remove<T>(entity);
            }
        }
    }

    void SceneHierarchyPanel::DrawComponents(entt::entity entity)
    {
        //if (m_Context->m_Registry.all_of<TagComponent>(entity))
        {
            std::string tag = "NULL";
            if (m_Context->m_Registry.all_of<TagComponent>(entity))
            {
                tag = m_Context->m_Registry.get<TagComponent>(entity).Tag;
            }

            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            assert(tag.size() < sizeof(buffer));
            strcpy(buffer, tag.c_str());

            if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
            {
                tag = std::string(buffer);
            }
        }

        ImGui::SameLine();
        ImGui::PushItemWidth(-1);

        if (ImGui::Button("Add Component"))
            ImGui::OpenPopup("AddComponent");

        if (ImGui::BeginPopup("AddComponent"))
        {
            // if (!m_SelectionContext.HasComponent<CameraComponent>())
            // {
            //     if (ImGui::MenuItem("Camera"))
            //     {
            //         m_SelectionContext.AddComponent<CameraComponent>();
            //         ImGui::CloseCurrentPopup();
            //     }
            // }
            
            // if (!m_SelectionContext.HasComponent<SpriteRendererComponent>())
            // {
            //     if (ImGui::MenuItem("Sprite Renderer"))
            //     {
            //         m_SelectionContext.AddComponent<SpriteRendererComponent>();
            //         ImGui::CloseCurrentPopup();
            //     }
            // }

            // if (!m_SelectionContext.HasComponent<Rigidbody2DComponent>())
            // {
            //     if (ImGui::MenuItem("Rigidbody 2D"))
            //     {
            //         m_SelectionContext.AddComponent<Rigidbody2DComponent>();
            //         ImGui::CloseCurrentPopup();
            //     }
            // }

            // if (!m_SelectionContext.HasComponent<BoxCollider2DComponent>())
            // {
            //     if (ImGui::MenuItem("Box Collider 2D"))
            //     {
            //         m_SelectionContext.AddComponent<BoxCollider2DComponent>();
            //         ImGui::CloseCurrentPopup();
            //     }
            // }
            

            ImGui::EndPopup();
        }

        ImGui::PopItemWidth();

		DrawComponent<TransformComponent>(m_Context->m_Registry, "Transform", entity,
		    [](auto& component)
		    {
                auto position = component.GetTranslation();
			    DrawVec3Control("Position", position, 0.0f, 100.0f);
                component.SetTranslation(position);

			    glm::vec3 rotation = glm::degrees(component.GetRotation());
			    DrawVec3Control("Rotation", rotation, 0.0f, 100.0f);
                component.SetRotation(glm::radians(rotation));

                auto scale = component.GetScale();
			    DrawVec3Control("Scale", scale, 1.0f, 100.0f);
                component.SetScale(scale);
		    });

        DrawComponent<DirectionalLightComponent>(m_Context->m_Registry, "Directional Light", entity,
		    [](auto& component)
		    {
		        ImGui::ColorEdit3("Color", (float*)&component.m_Color);
		    	//DrawVec3Control("Direction", component.m_Direction, 1.0f, 100.0f);
		    });

		DrawComponent<PerspectiveCameraComponent>(m_Context->m_Registry, "Perspective Camera", entity,
		    [](auto& component)
		    {
			    std::shared_ptr<RenderSys::PerspectiveCamera> camera = component.m_Camera;
			    ImGui::Checkbox("Primary", &component.IsPrimary);

                float perspectiveFov = camera->GetFOV();
                if (ImGui::DragFloat("Vertical FOV", &perspectiveFov))
                    camera->SetFOV(perspectiveFov);

                float perspectiveNear = camera->GetNearClip();
                if (ImGui::DragFloat("Near", &perspectiveNear))
                    camera->SetNearClip(perspectiveNear);

                float perspectiveFar = camera->GetFarClip();
                if (ImGui::DragFloat("Far", &perspectiveFar))
                    camera->SetFarClip(perspectiveFar);

			    // if (camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
			    // {
				//     float orthoSize = camera.GetOrthographicSize();
				//     if (ImGui::DragFloat("Size", &orthoSize))
				// 	    camera.SetOrthographicSize(orthoSize);

				//     float orthoNear = camera.GetOrthographicNearClip();
				//     if (ImGui::DragFloat("Near", &orthoNear))
				// 	    camera.SetOrthographicNearClip(orthoNear);

				//     float orthoFar = camera.GetOrthographicFarClip();
				//     if (ImGui::DragFloat("Far", &orthoFar))
				// 	    camera.SetOrthographicFarClip(orthoFar);

				//     ImGui::Checkbox("Fixed Aspect Ratio", &component.FixedAspectRatio);
			    // }
		    });

		// DrawComponent<SpriteRendererComponent>(
		//     "Sprite Renderer", entity,
		//     [](auto& component)
		//     {
        //         ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
                
		// 	    ImGui::Button("Texture", ImVec2(100.0f, 0.0f));
		// 	    if (ImGui::BeginDragDropTarget())
		// 	    {
		// 		    if (const ImGuiPayload* payload =
		// 		            ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
		// 		    {
		// 			    const wchar_t* path = (const wchar_t*)payload->Data;
		// 			    std::filesystem::path texturePath = std::filesystem::path(g_AssetPath) / path;
		// 			    component.Texture = Texture2D::Create(texturePath.string());
		// 		    }
		// 		    ImGui::EndDragDropTarget();
		// 	    }

		// 	    ImGui::DragFloat("Tiling Factor", &component.TilingFactor, 0.1f, 0.0f, 100.0f);
		//     });

        // DrawComponent<Rigidbody2DComponent>(
        //     "Rigidbody 2D", entity,
        //     [](auto& component)
        //     {
        //         const char* bodyTypeStrings[] = { "Static", "Dynamic", "Kinematic" };
        //         const char* currentBodyTypeString = bodyTypeStrings[(int)component.Type];

        //         if (ImGui::BeginCombo("Body Type", currentBodyTypeString))
        //         {
        //             for (int i = 0; i < 3; i++)
        //             {
        //                 bool isSelected = currentBodyTypeString == bodyTypeStrings[i];
        //                 if (ImGui::Selectable(bodyTypeStrings[i], isSelected))
        //                 {
        //                     currentBodyTypeString = bodyTypeStrings[i];
        //                     component.Type = (Rigidbody2DComponent::BodyType)i;
        //                 }

        //                 if (isSelected)
        //                     ImGui::SetItemDefaultFocus();
        //             }

        //             ImGui::EndCombo();
        //         }

        //         ImGui::Checkbox("Fixed Rotation", &component.FixedRotation);
        //     });

        // DrawComponent<BoxCollider2DComponent>(
        //     "Box Collider 2D", entity,
        //     [](auto& component)
		// {
		// 	ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset));
		// 	ImGui::DragFloat2("Size", glm::value_ptr(component.Size));
		// 	ImGui::DragFloat("Density", &component.Density, 0.01f, 0.0f, 1.0f);
		// 	ImGui::DragFloat("Friction", &component.Friction, 0.01f, 0.0f, 1.0f);
		// 	ImGui::DragFloat("Restitution", &component.Restitution, 0.01f, 0.0f, 1.0f);
		// 	ImGui::DragFloat("Restitution Threshold", &component.RestitutionThreshold, 0.01f, 0.0f);
		// });
	}
}