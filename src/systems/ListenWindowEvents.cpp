#include <core/define_system.hpp>
#include <services/SFMLRenderWindow.hpp>
#include <components/RenderWindow.hpp>
#include <components/Input.hpp>
#include <SFML/Window.hpp>

namespace {

class ListenWindowEvents : public core::System {
  std::shared_ptr<service::SFMLRenderWindow> render_window_service;
  
  std::vector<sf::Event> event_buffer; //Он очищается при свапе с буффером компонента

public:
  ListenWindowEvents(std::shared_ptr<service::SFMLRenderWindow>);
  void update(entt::registry& registry) override {
    for (entt::entity entity : registry.view<component::RenderWindow>()) {
      sf::RenderWindow* window = render_window_service->get_window(entity);
      if (!window) {
        continue;
      }
      event_buffer.clear();
      sf::Event event;
      while (window->pollEvent(event)) {
        event_buffer.push_back(event);
      }
      if (registry.has<component::ListenWindowEvents>(entity)) {
        registry.patch<component::ListenWindowEvents>(entity, [this](auto& listen_comp) {
          std::swap(listen_comp.events, event_buffer); //Из-за этого не нужно очищать event_buffer
        });
      }
    }
  }
private:
 

};



ListenWindowEvents::ListenWindowEvents(std::shared_ptr<service::SFMLRenderWindow> render_window_service):
  render_window_service(std::move(render_window_service))
{}

CORE_DEFINE_SYSTEM("system::ListenWindowEvents", [](core::ServiceLocator& locator) {
  return std::make_unique<ListenWindowEvents>(
    std::move(locator.get<service::SFMLRenderWindow>())
  );
});



}