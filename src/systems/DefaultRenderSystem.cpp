#include <core/define_system.hpp>
#include <services/Camera.hpp>
#include <services/World.hpp>
#include <services/Time.hpp>
#include <components/Camera.hpp>
#include <components/Position.hpp>
#include <components/RenderMode.hpp>
#include <components/Sprite.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <algorithm>

namespace {

struct RenderElement {
  component::DefaultRenderMode mode;
  component::Position position;
  component::Body body;
  const sf::Texture* p_texture = nullptr;
  sf::FloatRect texture_region;
  
  //Что бы работал emplace_back
  RenderElement(const component::DefaultRenderMode& mode,
                const component::Position& position,
                const component::Body& body,
                const sf::Texture* p_texture,
                const sf::FloatRect& texture_region):
    mode(mode),
    position(position),
    body(body),
    p_texture(p_texture),
    texture_region(texture_region)
  {}
};

inline bool render_queue_sort_func(const RenderElement& lhs, const RenderElement& rhs) {
  return lhs.position.layer < rhs.position.layer;
}

sf::Transform calculate_render_transform(const sf::FloatRect& region,
                                         const component::Camera& camera,
                                         const component::Position& camera_pos)
{
  sf::Transform transform;
  const sf::Vector2f region_size = region.getSize();
  const sf::Vector2f step_size(region_size.x / camera.size_x, region_size.y / camera.size_y);

  float translate_x = region.left + region.width * 0.5 - step_size.x * camera_pos.x - step_size.x * 0.5;
  float translate_y = region.top + region.height * 0.5 + step_size.y * camera_pos.y + step_size.y * 0.5;
  transform.translate(translate_x, translate_y);
  transform.scale(step_size.x,-step_size.y);
  return transform;
}

class DefaultRenderSystem : public core::System {
  std::shared_ptr<service::Camera> camera_service;
  std::shared_ptr<service::World> world;
  std::shared_ptr<service::Time> time;
  
  std::vector<RenderElement> render_queue;
  std::vector<entt::entity> world_query_buffer;
  
  std::vector<sf::Vertex> vertices;

  //Хранятся фреймы анимаций у текстур, очищается каждый кадр
  std::map<const sf::Texture*, size_t> texture_frame_cache;
  float current_time = 0;
public:
  DefaultRenderSystem(std::shared_ptr<service::Camera> camera_service,
                      std::shared_ptr<service::World> world,
                      std::shared_ptr<service::Time> time):
    camera_service(std::move(camera_service)),
    world(std::move(world)),
    time(std::move(time))
  {}

  void update(entt::registry& registry) override {
    texture_frame_cache.clear();
    current_time = time->get_time();
    auto view = registry.view<component::Camera, component::Position, component::CameraRenderRegion>();
    view.each([this, &registry](auto entity, const auto& camera, const auto& position, const auto& render_region) {
      render_queue.clear();
      world_query_buffer.clear();
      sf::RenderTarget* r_target = camera_service->get_render_target(entity, registry);
      if (!r_target) {
        return;
      }
      world->query_intersects_region(world_query_buffer, {position.x, position.y, camera.size_x, camera.size_y});
      fill_render_queue(registry);
      render(*r_target, camera, position, render_region);
    });
  }
  
  void fill_render_queue(entt::registry& registry) {
    for (entt::entity entity : world_query_buffer) {
      const auto [p_render_mode, p_position, p_body, p_texture] = 
        registry.try_get<component::DefaultRenderMode,
                         component::Position,
                         component::Body,
                         component::Texture>(entity);
      if (!p_position || !p_body || !p_render_mode) {
        continue;
      }
      
      if (p_texture && p_texture->texture) {
        const auto [p_texture_region, p_texture_auto_animation] = 
          registry.try_get<component::TextureRegion, component::TextureAutoAnimation>(entity);
        sf::IntRect int_rect;
        if (p_texture_region) {
          int_rect = p_texture_region->region;
        } else if (p_texture_auto_animation && p_texture_auto_animation->frames.size()) {
          auto it = texture_frame_cache.find(p_texture->texture.get());
          size_t frame;
          if (it != texture_frame_cache.end()) {
            frame = it->second;
          } else {
            float modulo = std::fmod(
              current_time,
              p_texture_auto_animation->full_time > 0.0f ? p_texture_auto_animation->full_time : 1.0f
            );
            frame = 0;
            for (; frame < p_texture_auto_animation->frames.size(); ++frame) {
              modulo -= p_texture_auto_animation->frames[frame].time;
              if (modulo <= 0) {
                break;
              }
            }
          }
          int_rect = p_texture_auto_animation->frames[frame].region;
        } else {
          sf::Vector2u texture_size = p_texture->texture->getSize();
          int_rect.width = texture_size.x;
          int_rect.height = texture_size.y;
        };

        render_queue.emplace_back(
          *p_render_mode,
          *p_position,
          *p_body,
          p_texture->texture.get(),
          sf::FloatRect(static_cast<float>(int_rect.left),
                        static_cast<float>(int_rect.top),
                        static_cast<float>(int_rect.width),
                        static_cast<float>(int_rect.height))
        );
      } else {
        render_queue.emplace_back(*p_render_mode, *p_position, *p_body, nullptr, sf::FloatRect());
      }      
    }
    std::sort(render_queue.begin(), render_queue.end(), render_queue_sort_func);
  }
  
  void render(sf::RenderTarget& target,
              const component::Camera& camera,
              const component::Position& position,
              const component::CameraRenderRegion& render_region)
  {
    vertices.clear();
    
    sf::Transform render_transform = calculate_render_transform(render_region.rect, camera, position);
    
    sf::RenderStates states;
    const size_t queue_size = render_queue.size();
    for (size_t i = 0; i < queue_size; ++i) {
      const RenderElement& elem = render_queue[i];
      sf::FloatRect rect(
        elem.position.x + (0.5f - elem.body.size_x / 2.0f),
        elem.position.y + (0.5f - elem.body.size_y / 2.0f),
        elem.body.size_x,
        elem.body.size_y
      );
      rect = render_transform.transformRect(rect);
      
      if (states.blendMode != elem.mode.blend_mode || states.texture != elem.p_texture) {
        if (vertices.size()) {
          target.draw(vertices.data(), vertices.size(), sf::Quads, states);
        }
        vertices.clear();
        states.blendMode = elem.mode.blend_mode;
        states.texture = elem.p_texture;
      }
      
      apply_rect(rect, elem.texture_region, elem.mode.color);
    }
    if (vertices.size()) {
      target.draw(vertices.data(), vertices.size(), sf::Quads, states);
    }
  }
  
  inline void apply_rect(const sf::FloatRect& position_rect, const sf::FloatRect& texture_rect, const sf::Color& color) {
    vertices.emplace_back(sf::Vector2f{position_rect.left, position_rect.top},
                          color,
                          sf::Vector2f{texture_rect.left, texture_rect.top});
                          
    vertices.emplace_back(sf::Vector2f{position_rect.left + position_rect.width, position_rect.top},
                          color,
                          sf::Vector2f{texture_rect.left + texture_rect.width, texture_rect.top}); 
                          
    vertices.emplace_back(sf::Vector2f{position_rect.left + position_rect.width, position_rect.top + position_rect.height},
                          color,
                          sf::Vector2f{texture_rect.left + texture_rect.width, texture_rect.top + texture_rect.height});
                          
    vertices.emplace_back(sf::Vector2f{position_rect.left, position_rect.top + position_rect.height},
                          color,
                          sf::Vector2f{texture_rect.left, texture_rect.top + texture_rect.height});
  }
};

CORE_DEFINE_SYSTEM("system::DefaultRenderSystem", [](core::ServiceLocator& locator){
  return std::make_unique<DefaultRenderSystem>(
    locator.get<service::Camera>(),
    locator.get<service::World>(),
    locator.get<service::Time>()
  );
});

}
