#include <limitless/scene.hpp>
#include <limitless/instances/effect_instance.hpp>

namespace LimitlessDemo {
    class EffectsScene {
    private:
        Limitless::Scene scene;
        std::shared_ptr<Limitless::EffectInstance> hurricane {};

        void addInstances(Limitless::Assets& assets);
    public:
        EffectsScene(Limitless::Context& ctx, Limitless::Assets& assets);

        auto& getScene() { return scene; }

        void update(Limitless::Context& context, const Limitless::Camera& camera);
    };
}