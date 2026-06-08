#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../single_include/prometheus.hpp"

TEST(RegistryTest, TEXT_FORMAT_V004) {
  EXPECT_EQ(prometheus::TEXT_FORMAT_V004,
            "text/plain; version=0.0.4; charset=utf-8");
}

TEST(RegistryTest, OPENMETRICS_V100) {
  EXPECT_EQ(prometheus::OPENMETRICS_V100,
            "application/openmetrics-text; version=1.0.0; charset=utf-8");
}

TEST(RegistryTest, Empty) {
  auto reg = prometheus::registry::create();

  EXPECT_THAT(::testing::PrintToString(*reg), "");
}

TEST(RegistryTest, NotEmpty) {
  auto reg = prometheus::registry::create();

  reg->gauge("gauge1", "help1")->set(1);
  reg->gauge("gauge2", "help2")->set(1);

  EXPECT_THAT(::testing::PrintToString(*reg), "# HELP gauge1 help1\n"
                                              "# TYPE gauge1 gauge\n"
                                              "gauge1 1\n\n"
                                              "# HELP gauge2 help2\n"
                                              "# TYPE gauge2 gauge\n"
                                              "gauge2 1\n\n");
}

TEST(RegistryTest, TypeMismatch) {
  auto reg = prometheus::registry::create();

  reg->gauge("gauge1", "help1")->set(1);
  // Type mismatch. Should be ignored.
  reg->counter("gauge1", "help1")->inc();

  reg->counter("counter2", "help2")->inc();
  // Type mismatch. Should be ignored.
  reg->gauge("counter2", "help2")->set(2);

  EXPECT_THAT(::testing::PrintToString(*reg), "# HELP counter2 help2\n"
                                              "# TYPE counter2 counter\n"
                                              "counter2 1\n\n"
                                              "# HELP gauge1 help1\n"
                                              "# TYPE gauge1 gauge\n"
                                              "gauge1 1\n\n");
}

TEST(RegistryTest, RemoveByName) {
  auto reg = prometheus::registry::create();

  reg->gauge("gauge1", "help1")->set(1);
  reg->counter("counter1", "help1")->inc();

  reg->remove("not-exists");
  reg->remove("gauge1");
  reg->remove("counter1");

  EXPECT_THAT(::testing::PrintToString(*reg), "");
}

TEST(RegistryTest, RemoveByLabels) {
  auto reg = prometheus::registry::create();

  reg->gauge("gauge1", "help1", {{"key1", "value1"}})->set(1);
  reg->gauge("gauge2", "help2", {{"key1", "value1"}})->set(2);
  reg->gauge("gauge3", "help3", {{"key1", "value2"}})->set(3);

  reg->remove({{"key1", "value1"}});

  EXPECT_THAT(::testing::PrintToString(*reg),
              "# HELP gauge3 help3\n"
              "# TYPE gauge3 gauge\n"
              "gauge3{\"key1\"=\"value2\"} 3\n\n");
}
