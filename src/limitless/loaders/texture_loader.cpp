#include <limitless/loaders/texture_loader.hpp>

#include <limitless/core/context_initializer.hpp>
#include <limitless/core/texture_builder.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>
#include <limitless/assets.hpp>

using namespace Limitless;

namespace {
    constexpr auto ANIS_EXTENSION = "GL_EXT_texture_filter_anisotropic";
    constexpr auto S3TC_EXTENSION = "GL_EXT_texture_compression_s3tc";
    constexpr auto BPTC_EXTENSION = "GL_ARB_texture_compression_bptc";
    constexpr auto RGTC_EXTENSION = "GL_ARB_texture_compression_rgtc";
}

void TextureLoader::setFormat(TextureBuilder& builder, const TextureLoaderFlags& flags, int channels) {
    Texture::InternalFormat internal;

    switch (flags.compression) {
        case TextureLoaderFlags::Compression::None:
        none:
            switch (channels) {
                case 1: internal = Texture::InternalFormat::R8; break;
                case 2: internal = Texture::InternalFormat::RG8; break;
                case 3: internal = Texture::InternalFormat::RGB8; break;
                case 4: internal = Texture::InternalFormat::RGBA8; break;
                default: throw texture_loader_exception("Bad channels count!");
            }
            break;
        case TextureLoaderFlags::Compression::DXT1:
        dxt1:
            if (channels != 3 && channels != 4) {
                throw texture_loader_exception("Bad Compression S3TC setting for channels count!");
            }

            if (!ContextInitializer::isExtensionSupported(S3TC_EXTENSION)) {
                throw texture_loader_exception("Compression S3TC is not supported!");
            }

            switch (channels) {
                case 3: internal = Texture::InternalFormat::RGB_DXT1; break;
                case 4: internal = Texture::InternalFormat::RGBA_DXT1; break;
            }
            break;
        case TextureLoaderFlags::Compression::DXT5:
        dxt5:
            if (channels != 4) {
                throw texture_loader_exception("Bad Compression S3TC setting for channels count!");
            }

            if (!ContextInitializer::isExtensionSupported(S3TC_EXTENSION)) {
                throw texture_loader_exception("Compression S3TC is not supported!");
            }
            internal = Texture::InternalFormat::RGBA_DXT5;
            break;
        case TextureLoaderFlags::Compression::BC7:
        bc7:
            if (channels != 3 && channels != 4) {
                throw texture_loader_exception("Bad Compression BPTC setting for channels count!");
            }

            if (!ContextInitializer::isExtensionSupported(BPTC_EXTENSION)) {
                throw texture_loader_exception("Compression BPTC is not supported!");
            }

            internal = Texture::InternalFormat::RGBA_BC7;
            break;
        case TextureLoaderFlags::Compression::RGTC:
        rgtc:
            if (channels != 1 && channels != 2) {
                throw texture_loader_exception("Bad Compression RGTC setting for channels count!");
            }

            if (!ContextInitializer::isExtensionSupported(RGTC_EXTENSION)) {
                throw texture_loader_exception("Compression RGTC is not supported!");
            }

            switch (channels) {
                case 1: internal = Texture::InternalFormat::R_RGTC; break;
                case 2: internal = Texture::InternalFormat::RG_RGTC; break;
            }
            break;
        case TextureLoaderFlags::Compression::Default:
            if ((channels == 1 || channels == 2) && ContextInitializer::isExtensionSupported(RGTC_EXTENSION)) {
                goto rgtc;
            }

            if ((channels == 3 || channels == 4) && ContextInitializer::isExtensionSupported(BPTC_EXTENSION)) {
                goto bc7;
            }

            if (channels == 3 && ContextInitializer::isExtensionSupported(S3TC_EXTENSION)) {
                goto dxt1;
            }

            if (channels == 4 && ContextInitializer::isExtensionSupported(S3TC_EXTENSION)) {
                goto dxt5;
            }

            goto none;
    }

    Texture::Format format;
    switch (channels) {
        case 1: format = Texture::Format::Red; break;
        case 2: format = Texture::Format::RG; break;
        case 3: format = Texture::Format::RGB; break;
        case 4: format = Texture::Format::RGBA; break;
        default: throw texture_loader_exception("Bad channels count!");
    }

    builder.setInternalFormat(internal)
           .setFormat(format);
}

void TextureLoader::setAnisotropicFilter(const std::shared_ptr<Texture>& texture, const TextureLoaderFlags& flags) {
    if (flags.anisotropic_filter && ContextInitializer::isExtensionSupported(ANIS_EXTENSION)) {
        if (flags.anisotropic_value == 0.0f) {
            texture->setAnisotropicFilterMax();
        } else {
            texture->setAnisotropicFilter(flags.anisotropic_value);
        }
    }
}

std::shared_ptr<Texture> TextureLoader::load(Assets& assets, const fs::path& _path, const TextureLoaderFlags& flags) {
    auto path = convertPathSeparators(_path);

    if (assets.textures.contains(path.stem().string())) {
        return assets.textures[path.stem().string()];
    }

    stbi_set_flip_vertically_on_load(static_cast<bool>((int)flags.origin));

    int width = 0, height = 0, channels = 0;
    unsigned char* data = stbi_load(path.string().c_str(), &width, &height, &channels, 0);

    setDownScale(width, height, channels, data, flags);

    if (data) {
        TextureBuilder builder;

        builder.setTarget(Texture::Type::Tex2D)
               .setLevels(glm::floor(glm::log2(static_cast<float>(glm::max(width, height)))) + 1)
               .setSize({ width, height })
               .setDataType(Texture::DataType::UnsignedByte)
               .setData(data);

        setFormat(builder, flags, channels);
        setTextureParameters(builder, flags);

        auto texture = builder.build();
        texture->setPath(path);
        setAnisotropicFilter(texture, flags);

        if (flags.downscale != TextureLoaderFlags::DownScale::None) {
            delete data;
        } else {
            stbi_image_free(data);
        }

        assets.textures.add(path.stem().string(), texture);
        return texture;
    } else {
        throw std::runtime_error("Failed to load texture: " + path.string() + " " + stbi_failure_reason());
    }
}

std::shared_ptr<Texture> TextureLoader::loadCubemap(Assets& assets, const fs::path& _path, const TextureLoaderFlags& flags) {
    auto path = convertPathSeparators(_path);

    stbi_set_flip_vertically_on_load(static_cast<bool>((int)flags.origin));

    constexpr std::array ext = { "_right", "_left", "_top", "_bottom", "_front", "_back" };

    std::array<void*, 6> data = { nullptr };
    int width = 0, height = 0, channels = 0;

    for (size_t i = 0; i < data.size(); ++i) {
        std::string p = path.parent_path().string() + PATH_SEPARATOR + path.stem().string() + ext[i] + path.extension().string();
        data[i] = stbi_load(p.c_str(), &width, &height, &channels, 0);

        if (!data[i]) {
            throw std::runtime_error("Failed to load texture: " + path.string() + " " + stbi_failure_reason());
        }
    }

    TextureBuilder builder;
    builder .setTarget(Texture::Type::CubeMap)
            .setSize(glm::uvec2{ width, height })
            .setDataType(Texture::DataType::UnsignedByte)
            .setData(data);

    setFormat(builder, flags, channels);
    setTextureParameters(builder, flags);

    builder.setWrapS(Texture::Wrap::ClampToEdge)
           .setWrapT(Texture::Wrap::ClampToEdge)
           .setWrapR(Texture::Wrap::ClampToEdge);

    auto texture = builder.buildMutable();
    texture->setPath(path);
    setAnisotropicFilter(texture, flags);

    std::for_each(data.begin(), data.end(), [] (auto* ptr) { stbi_image_free(ptr); });

    return texture;
}

GLFWimage TextureLoader::loadGLFWImage(Assets& assets, const fs::path& _path, const TextureLoaderFlags& flags) {
    auto path = convertPathSeparators(_path);

    stbi_set_flip_vertically_on_load(static_cast<bool>(flags.origin));

    int width = 0, height = 0, channels = 0;
    unsigned char* data = stbi_load(path.string().c_str(), &width, &height, &channels, 0);

    if (data) {
        return GLFWimage{ width, height, data };
    } else {
        throw std::runtime_error("Failed to load texture: " + path.string() + " " + stbi_failure_reason());
    }
}

void TextureLoader::setDownScale(int& width, int& height, int channels, unsigned char*& data, const TextureLoaderFlags& flags) {
    int scaled_width, scaled_height;
    unsigned char* resized_data;

    switch (flags.downscale) {
        case TextureLoaderFlags::DownScale::None:
            return;
        case TextureLoaderFlags::DownScale::x2:
            scaled_width = width / 2;
            scaled_height = height / 2;
            break;
        case TextureLoaderFlags::DownScale::x4:
            scaled_width = width / 4;
            scaled_height = height / 4;
            break;
        case TextureLoaderFlags::DownScale::x8:
            scaled_width = width / 8;
            scaled_height = height / 8;
            break;
    }

    resized_data = new unsigned char[channels * scaled_height * scaled_width];
    stbir_resize_uint8(data, width, height, 0, resized_data, scaled_width, scaled_height, 0, channels);

    width = scaled_width;
    height = scaled_height;

    stbi_image_free(data);
    data = resized_data;
}

void TextureLoader::setTextureParameters(TextureBuilder& builder, const TextureLoaderFlags& flags) {
    builder.setMipMap(flags.mipmap)
           .setBorder(flags.border)
           .setBorderColor(flags.border_color)
           .setWrapS(flags.wrapping)
           .setWrapT(flags.wrapping)
           .setWrapR(flags.wrapping);

    switch (flags.filter) {
        case TextureLoaderFlags::Filter::Linear:
            builder.setMagFilter(Texture::Filter::Linear);
            if (flags.mipmap) {
                builder.setMinFilter(Texture::Filter::LinearMipmapLinear);
            }
            break;
        case TextureLoaderFlags::Filter::Nearest:
            builder.setMagFilter(Texture::Filter::Nearest);
            if (flags.mipmap) {
                builder.setMinFilter(Texture::Filter::NearestMipmapNearest);
            }
            break;
    }
}
