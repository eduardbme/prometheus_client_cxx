#include <iostream>

#include "../../single_include/prometheus.hpp"

int main() {
  auto reg = prometheus::registry::create([](const std::string &error) {
    std::cerr << "Registry error: " << error << std::endl;
    std::abort();
  });

  std::cout << *reg << std::endl;

  return 0;
}
