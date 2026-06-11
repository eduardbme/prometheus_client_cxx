#include <iostream>

#include "../../single_include/prometheus.hpp"

int main() {
  auto reg = prometheus::registry::create();

  std::cout << *reg << std::endl;

  return 0;
}
