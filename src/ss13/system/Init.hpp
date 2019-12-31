#pragma once

#include <core/ISystem.hpp>
#include <core/service/Window.hpp>
#include <core/service/PerformanceController.hpp>
#include <core/service/TextureLoader.hpp>


namespace ss13::system {

class Init final : public core::ISystem {
public:
  const std::string_view name() const noexcept override;
  void init() override;
  void update() override;
private:
  std::shared_ptr<core::service::TextureLoader> texture_loader;
};


} /* ss13::system */
