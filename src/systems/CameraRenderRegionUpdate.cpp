#include <core/define_system.hpp>
#include <services/Camera.hpp>
#include <components/Camera.hpp>

namespace {

class CameraRenderRegionUpdate final : public core::System {
  std::shared_ptr<service::Camera> camera_service;
public:
  CameraRenderRegionUpdate(std::shared_ptr<service::Camera> camera): camera_service(std::move(camera)) {}

  void update(entt::registry& registry) override {
    auto view = registry.view<component::Camera>();
    view.each([this, &registry](auto entity, const auto& camera) {
      sf::FloatRect region;
      if (calculate_rect(region, registry, entity, camera)) {
        const auto* p_render_region = registry.try_get<component::CameraRenderRegion>(entity);
        if (p_render_region) {
          if (p_render_region->rect != region) {
            registry.replace<component::CameraRenderRegion>(entity, region);
          }
        } else {
          registry.emplace<component::CameraRenderRegion>(entity, region);
        }
      } else {
        registry.remove_if_exists<component::CameraRenderRegion>(entity);
      }
    });
  }
  
  bool calculate_rect(sf::FloatRect& region,
                      const entt::registry& registry,
                      const entt::entity entity,
                      const component::Camera& camera)
  {
    const sf::RenderTarget* target = camera_service->get_render_target(entity, registry);
    if (!target) {
      return false;
    }
    sf::Vector2u size = target->getSize();
    sf::Vector2f f_size(static_cast<float>(size.x), static_cast<float>(size.y));

    const auto* p_region_override = registry.try_get<component::OverrideRenderRegion>(entity);
    if (p_region_override) {
      region = p_region_override->rect;
      return true;
    }
    region = sf::FloatRect(0.0f,0.0f, f_size.x, f_size.y);
    return true;
  }
};

CORE_DEFINE_SYSTEM("system::CameraRenderRegionUpdate", [](core::ServiceLocator& locator) {
  return std::make_unique<CameraRenderRegionUpdate>(std::move(locator.get<service::Camera>()));
});

}