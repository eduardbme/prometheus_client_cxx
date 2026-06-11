#include <iostream>

#include "../../single_include/prometheus.hpp"
#include "httplib.h"

int main() {
  httplib::Server svr;
  auto reg = prometheus::registry::create();

  svr.Get("/metrics", [&](const httplib::Request &, httplib::Response &res) {
    res.set_content(reg->str(),
                    static_cast<std::string>(prometheus::TEXT_FORMAT_V004));
  });

  std::cout << "Start listening on 0.0.0.0:19100" << std::endl;
  if (!svr.listen("0.0.0.0", 19100)) {
    std::cerr << "Failed to start server" << std::endl;
    return 1;
  }

  return 0;
}
