#include <chrono>
#include <thread>

#include "../single_include/prometheus.hpp"

void set_handler(std::shared_ptr<prometheus::registry> reg, int n_iters) {
  std::stringstream ss;

  for (int i = 0; i < n_iters; i++) {
    reg->label_set({"key1", "value1"});

    reg->counter("counter0", "help")->inc(i);
    reg->counter("counter1", "help", {{"key1", "value"}})->inc(i);
    reg->counter("counter2", "help", {{"key2", "value"}})->inc(i);

    reg->gauge("gauge0", "help")->set(i);
    reg->gauge("gauge1", "help", {{"key1", "value"}})->set(i);
    reg->gauge("gauge2", "help", {{"key2", "value"}})->set(i);
  }
}

void remove_handler(std::shared_ptr<prometheus::registry> reg, int n_iters) {
  std::stringstream ss;

  for (int i = 0; i < n_iters; i++) {
    reg->remove("counter0");
    reg->remove("counter1", {{"key1", "value"}});

    reg->remove("gauge0");
    reg->remove("gauge1", {{"key1", "value"}});

    reg->remove({{"key2", "value"}});
  }
}

void out_handler(std::shared_ptr<prometheus::registry> reg, int n_iters) {
  std::stringstream ss;

  for (int i = 0; i < n_iters; i++) {
    ss << *reg;
    ss.clear();
  }
}

int main() {
  const int n_iters = 1e3;
  auto reg = prometheus::registry::create();

  std::thread set_thread(set_handler, reg, n_iters);
  std::thread remove_thread(remove_handler, reg, n_iters);
  std::thread out_thread(out_handler, reg, n_iters);

  set_thread.join();
  remove_thread.join();
  out_thread.join();

  return 0;
}
