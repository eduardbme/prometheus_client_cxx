#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../single_include/prometheus.hpp"

TEST(GaugeTest, Gauge) {
  auto reg = prometheus::registry::create();

  reg->gauge("gauge1", "help1")->set(1);

  EXPECT_THAT(::testing::PrintToString(*reg), "# HELP gauge1 help1\n"
                                              "# TYPE gauge1 gauge\n"
                                              "gauge1 1\n\n");
}

TEST(GaugeTest, GaugeUpdate) {
  auto reg = prometheus::registry::create();

  reg->gauge("gauge1", "help1")->set(1);
  reg->gauge("gauge1", "help1")->set(2);

  EXPECT_THAT(::testing::PrintToString(*reg), "# HELP gauge1 help1\n"
                                              "# TYPE gauge1 gauge\n"
                                              "gauge1 2\n\n");
}

TEST(GaugeTest, GaugeLabel) {
  auto reg = prometheus::registry::create();

  reg->gauge("gauge1", "help1", {{"key1", "value1"}})->set(1);

  EXPECT_THAT(::testing::PrintToString(*reg),
              "# HELP gauge1 help1\n"
              "# TYPE gauge1 gauge\n"
              "gauge1{\"key1\"=\"value1\"} 1\n\n");
}

TEST(GaugeTest, GaugeLabelUpdate) {
  auto reg = prometheus::registry::create();

  reg->gauge("gauge1", "help1", {{"key1", "value1"}})->set(1);
  reg->gauge("gauge1", "help1", {{"key1", "value1"}})->set(2);

  EXPECT_THAT(::testing::PrintToString(*reg),
              "# HELP gauge1 help1\n"
              "# TYPE gauge1 gauge\n"
              "gauge1{\"key1\"=\"value1\"} 2\n\n");
}

TEST(GaugeTest, GaugeLabelsOrder) {
  auto reg = prometheus::registry::create();

  reg->gauge("gauge1", "help1", {{"key1", "value1"}, {"key2", "value2"}})
      ->set(1);
  reg->gauge("gauge1", "help1", {{"key2", "value2"}, {"key1", "value1"}})
      ->set(2);

  EXPECT_THAT(::testing::PrintToString(*reg),
              "# HELP gauge1 help1\n"
              "# TYPE gauge1 gauge\n"
              "gauge1{\"key1\"=\"value1\",\"key2\"=\"value2\"} 2\n\n");
}

TEST(GaugeTest, GaugeFamily) {
  auto reg = prometheus::registry::create();

  reg->gauge("gauge1", "help1")->set(1);
  reg->gauge("gauge1", "help1", {{"key1", "value1"}})->set(2);
  reg->gauge("gauge1", "help1", {{"key1", "value2"}})->set(3);
  reg->gauge("gauge1", "help1", {{"key2", "value1"}})->set(4);

  EXPECT_THAT(::testing::PrintToString(*reg),
              "# HELP gauge1 help1\n"
              "# TYPE gauge1 gauge\n"
              "gauge1 1\n"
              "gauge1{\"key1\"=\"value1\"} 2\n"
              "gauge1{\"key1\"=\"value2\"} 3\n"
              "gauge1{\"key2\"=\"value1\"} 4\n\n");
}

TEST(GaugeTest, GaugeAnyNumericType) {
  auto reg = prometheus::registry::create();

  reg->gauge("gauge1", "help1", {{"key", "test1"}})->set(1);
  reg->gauge("gauge1", "help1", {{"key", "test2"}})->set(1.1f);
  reg->gauge("gauge1", "help1", {{"key", "test3"}})->set(1.1);
  // TODO:
  // reg->gauge<size_t>("gauge1", "help1", {{"key", "test4"}})
  //     ->set(std::numeric_limits<std::size_t>::max());

  EXPECT_THAT(::testing::PrintToString(*reg),
              "# HELP gauge1 help1\n"
              "# TYPE gauge1 gauge\n"
              "gauge1{\"key\"=\"test1\"} 1\n"
              "gauge1{\"key\"=\"test2\"} 1.1\n"
              "gauge1{\"key\"=\"test3\"} 1.1\n"
              "gauge1{\"key\"=\"test4\"} " +
                  std::to_string(std::numeric_limits<std::size_t>::max()) +
                  "\n\n");
}
