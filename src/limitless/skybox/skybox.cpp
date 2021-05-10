#include <limitless/skybox/skybox.hpp>

#include <limitless/loaders/texture_loader.hpp>
#include <limitless/core/shader_program.hpp>
#include <limitless/models/abstract_mesh.hpp>
#include <limitless/core/context.hpp>
#include <limitless/assets.hpp>
#include <limitless/ms/material_builder.hpp>

using namespace Limitless;
using namespace Limitless::ms;

Skybox::Skybox(const std::shared_ptr<Material>& material)
    : material {std::make_shared<Material>(*material)} {
}

Skybox::Skybox(Context& ctx, Assets& assets, const RenderSettings& settings, const fs::path& path, const TextureLoaderFlags& flags) {
    TextureLoader texture_loader {assets};
    const auto& cube_map_texture = texture_loader.loadCubemap(path, flags);

    MaterialBuilder material_builder {ctx, assets, settings};
    material = material_builder
                    .setName(path.string())
                    .addUniform(std::make_unique<UniformSampler>("skybox", cube_map_texture))
                    .setFragmentSnippet("mat_color *= texture(skybox, uv);\n")
                    .add(Property::Color, glm::vec4{1.0f})
                    .setTwoSided(true)
                    .setShading(Shading::Unlit)
                    .setBlending(Blending::Opaque)
                    .setModelShaders({ModelShader::Model})
                    .addPassShader(ShaderPass::Skybox)
                    .build();
}

void Skybox::draw(Context& context, const Assets& assets) {
    auto& shader = assets.shaders.get(ShaderPass::Skybox, ModelShader::Model, material->getShaderIndex());

    context.enable(Capabilities::DepthTest);
    context.setDepthFunc(DepthFunc::Lequal);
    context.setDepthMask(DepthMask::True);
    context.disable(Capabilities::Blending);

    shader << *material;

    shader.use();

    assets.meshes.at("cube_mesh")->draw();
}