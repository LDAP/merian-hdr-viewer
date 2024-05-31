#include "merian-nodes/graph/graph.hpp"
#include "merian-nodes/nodes/exposure/exposure.hpp"
#include "merian-nodes/nodes/glfw_window/glfw_window.hpp"
#include "merian-nodes/nodes/image_read/hdr_image.hpp"
#include "merian-nodes/nodes/image_write/image_write.hpp"
#include "merian-nodes/nodes/tonemap/tonemap.hpp"
#include "merian-nodes/nodes/vkdt_filmcurv/vkdt_filmcurv.hpp"

#include "merian/utils/configuration_imgui.hpp"
#include "merian/vk/context.hpp"
#include "merian/vk/extension/extension_resources.hpp"
#include "merian/vk/extension/extension_vk_debug_utils.hpp"
#include "merian/vk/extension/extension_vk_glfw.hpp"
#include "merian/vk/window/glfw_imgui.hpp"

int main(int argc, char** argv) {
    spdlog::set_level(spdlog::level::debug);

    if (argc != 2) {
        SPDLOG_INFO("Usage: merian-hdr-viewer <path/to/image.hdr>");
        return 0;
    }

    // Setup Vulkan context.
    const auto debug_utils = std::make_shared<merian::ExtensionVkDebugUtils>(false);
    const auto extGLFW = std::make_shared<merian::ExtensionVkGLFW>();
    const auto resources = std::make_shared<merian::ExtensionResources>();
    const std::vector<std::shared_ptr<merian::Extension>> extensions = {extGLFW, resources,
                                                                        debug_utils};
    const merian::SharedContext context =
        merian::Context::make_context(extensions, "merian-hdr-viewer");

    merian::ResourceAllocatorHandle alloc = resources->resource_allocator();
    merian::QueueHandle queue = context->get_queue_GCT();

    // Setup processing graph.
    merian_nodes::Graph graph{context, alloc};
    auto window = std::make_shared<merian_nodes::GLFWWindow>(context);
    auto image_out = std::make_shared<merian_nodes::ImageWrite>(context, alloc);
    auto image_in =
        std::make_shared<merian_nodes::HDRImageRead>(alloc->getStaging(), argv[1], false);
    auto exp = std::make_shared<merian_nodes::AutoExposure>(context);
    auto tonemap = std::make_shared<merian_nodes::Tonemap>(context);
    auto curve = std::make_shared<merian_nodes::VKDTFilmcurv>(context);

    graph.add_node(window, "window");
    graph.add_node(image_in);
    graph.add_node(exp);
    graph.add_node(tonemap);
    graph.add_node(curve);
    graph.add_connection(image_in, exp, "out", "src");
    graph.add_connection(exp, tonemap, "out", "src");
    graph.add_connection(tonemap, curve, "out", "src");
    graph.add_connection(curve, window, "out", "src");

    // Setup IMGUI window, by hooking into the window node.
    merian::ImGuiContextWrapperHandle debug_ctx = std::make_shared<merian::ImGuiContextWrapper>();
    merian::GLFWImGui imgui(context, debug_ctx, true);
    merian::ImGuiConfiguration config;
    window->set_on_blit_completed(
        [&](const vk::CommandBuffer& cmd, merian::SwapchainAcquireResult& acquire_result) {
            imgui.new_frame(queue, cmd, *window->get_window(), acquire_result);

            ImGui::Begin("HDR Viewer", NULL, ImGuiWindowFlags_NoFocusOnAppearing);

            graph.configuration(config);

            ImGui::End();
            imgui.render(cmd);
        });

    // Run the graph
    while (!window->get_window()->should_close()) {
        glfwPollEvents();
        graph.run();
    }

    return 0;
}
