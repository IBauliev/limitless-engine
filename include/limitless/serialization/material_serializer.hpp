#pragma once

#include <memory>
#include <limitless/serialization/asset_deserializer.hpp>

namespace Limitless {
    class Context;
    class Assets;
    class ByteBuffer;
}

namespace Limitless::ms {
    class Material;
    class MaterialBuilder;

    class MaterialSerializer {
    private:
        void deserialize(ByteBuffer& buffer, Context& context, Assets& assets, const RenderSettings& settings, ms::MaterialBuilder& builder);
    public:
        ByteBuffer serialize(const ms::Material& material);
        std::shared_ptr<ms::Material> deserialize(Context& ctx, Assets& assets, const RenderSettings& settings, ByteBuffer& buffer);
    };

    ByteBuffer& operator<<(ByteBuffer& buffer, const ms::Material& material);
    ByteBuffer& operator>>(ByteBuffer& buffer, const AssetDeserializer<std::shared_ptr<ms::Material>>& material);
}
