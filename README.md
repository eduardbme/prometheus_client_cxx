# prometheus_client_cxx

- [Design goals](#design-goals)
- [Examples](#examples)
  - [Stdout](#stdout)
  - [Http](#http)
  - [Run exampes in Docker](#run-examples-in-docker)
- [Tests](#tests)
  - [Run tests](#run-tests)

## Design goals

There are myriads of [Prometheus](https://prometheus.io/) libraries out there, and each may even have its reason to exist. Our library had these design goals:

- **Interface-agnostic**. No http server or any other interface is included. Check out the [stdout example](#stdout) and [http example](#http).
- **Trivial integration**. Our whole code consists of a single header file [`prometheus.hpp`](https://github.com/eduardbme/prometheus_client_cxx/blob/main/single_include/prometheus.hpp). That's it. No library, no subproject, no dependencies, no complex build system. The class is written in vanilla C++17. All in all, everything should require no adjustment of your compiler flags or project settings.

## Examples

### Stdout

The `prometheus::registry` class provides `<<` operator to work with `std::ostream` family of classes:

```cpp
#include <iostream>

#include "single_include/prometheus.hpp"

// ...

auto reg = prometheus::registry::create();
std::cout << *reg << std::endl;
```

### Http

No `HTTP` server is included by default, you should leverage third-party library:

```cpp
#include <iostream>

#include "single_include/prometheus.hpp"
#include "httplib.h"

// ...

httplib::Server svr;
auto reg = prometheus::registry::create();

svr.Get("/metrics", [&](const httplib::Request &, httplib::Response &res) {
  res.set_content(reg->to_string(),
                  static_cast<std::string>(prometheus::TEXT_FORMAT_V004));
});

std::cout << "Start listening on 0.0.0.0:19100" << std::endl;
if (!svr.listen("0.0.0.0", 19100)) {
  std::cerr << "Failed to start server" << std::endl;
  return 1;
}

```

### Run examples in Docker

```bash
make examples
```

## Tests

### Run tests

```bash
make tests
```
