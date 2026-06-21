#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../single_include/prometheus.hpp"

TEST(CounterTest, DefaultValue) {
  auto reg = prometheus::registry::create();

  reg->counter("counter1", "help1");

  EXPECT_THAT(::testing::PrintToString(*reg), "# HELP counter1 help1\n"
                                              "# TYPE counter1 counter\n"
                                              "counter1 0\n\n");
}

TEST(CounterTest, Inc) {
  auto reg = prometheus::registry::create();

  reg->counter("counter1", "help1")->inc();
  reg->counter("counter2", "help2")->inc(2);

  EXPECT_THAT(::testing::PrintToString(*reg), "# HELP counter1 help1\n"
                                              "# TYPE counter1 counter\n"
                                              "counter1 1\n\n"
                                              "# HELP counter2 help2\n"
                                              "# TYPE counter2 counter\n"
                                              "counter2 2\n\n");
}
