#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../single_include/prometheus.hpp"

TEST(GaugeTest, DefaultValue) {
  auto reg = prometheus::registry::create();

  reg->gauge("gauge1", "help1");

  EXPECT_THAT(::testing::PrintToString(*reg), "# HELP gauge1 help1\n"
                                              "# TYPE gauge1 gauge\n"
                                              "gauge1 0\n\n");
}

TEST(GaugeTest, GaugeUpdate) {
  auto reg = prometheus::registry::create();

  reg->gauge("gauge1", "help1")->set(1);

  EXPECT_THAT(::testing::PrintToString(*reg), "# HELP gauge1 help1\n"
                                              "# TYPE gauge1 gauge\n"
                                              "gauge1 1\n\n");
}

TEST(GaugeTest, GaugeWithLabel) {
  auto reg = prometheus::registry::create();

  reg->gauge("gauge1", "help1", {{"key1", "value1"}})->set(1);

  EXPECT_THAT(::testing::PrintToString(*reg),
              "# HELP gauge1 help1\n"
              "# TYPE gauge1 gauge\n"
              "gauge1{\"key1\"=\"value1\"} 1\n\n");
}

TEST(GaugeTest, GaugeWithLabelUpdate) {
  auto reg = prometheus::registry::create();

  reg->gauge("gauge1", "help1", {{"key1", "value1"}})->set(1);

  EXPECT_THAT(::testing::PrintToString(*reg),
              "# HELP gauge1 help1\n"
              "# TYPE gauge1 gauge\n"
              "gauge1{\"key1\"=\"value1\"} 1\n\n");
}

TEST(GaugeTest, GaugeWithLabel_LabelsOrder) {
  auto reg = prometheus::registry::create();

  reg->gauge("gauge1", "help1", {{"key1", "value1"}, {"key2", "value2"}});
  reg->gauge("gauge1", "help1", {{"key2", "value2"}, {"key1", "value1"}})
      ->set(1);

  EXPECT_THAT(::testing::PrintToString(*reg),
              "# HELP gauge1 help1\n"
              "# TYPE gauge1 gauge\n"
              "gauge1{\"key1\"=\"value1\",\"key2\"=\"value2\"} 1\n\n");
}

TEST(GaugeTest, GaugeFamily) {
  auto reg = prometheus::registry::create();

  reg->gauge("gauge1", "help1");
  reg->gauge("gauge1", "help1", {{"key1", "value1"}});
  reg->gauge("gauge1", "help1", {{"key1", "value2"}});
  reg->gauge("gauge1", "help1", {{"key2", "value1"}});

  EXPECT_THAT(::testing::PrintToString(*reg),
              "# HELP gauge1 help1\n"
              "# TYPE gauge1 gauge\n"
              "gauge1 0\n"
              "gauge1{\"key1\"=\"value1\"} 0\n"
              "gauge1{\"key1\"=\"value2\"} 0\n"
              "gauge1{\"key2\"=\"value1\"} 0\n\n");
}

TEST(GaugeTest, GaugeNumericTypeOnly) {
  auto reg = prometheus::registry::create();

  // compile time error
  // reg->gauge<std::string>("incorrect1", "help1")->set("");

  // compile time error
  // int != float
  // reg->gauge("gauge1", "help1")->set(std::numeric_limits<float>::max());

  reg->gauge("gauge1", "help1", {{"key", "test1"}})
      ->set(std::numeric_limits<int>::max());
  reg->gauge<float>("gauge1", "help1", {{"key", "test2"}})
      ->set(std::numeric_limits<float>::max());
  reg->gauge<double>("gauge1", "help1", {{"key", "test3"}})
      ->set(std::numeric_limits<double>::max());
  reg->gauge<std::size_t>("gauge1", "help1", {{"key", "test4"}})
      ->set(std::numeric_limits<std::size_t>::max());

  EXPECT_THAT(::testing::PrintToString(*reg),
              "# HELP gauge1 help1\n"
              "# TYPE gauge1 gauge\n"
              "gauge1{\"key\"=\"test1\"} " +
                  std::to_string(std::numeric_limits<int>::max()) +
                  "\n"
                  "gauge1{\"key\"=\"test2\"} " +
                  std::to_string(std::numeric_limits<float>::max()) +
                  "\n"
                  "gauge1{\"key\"=\"test3\"} " +
                  std::to_string(std::numeric_limits<double>::max()) +
                  "\n"
                  "gauge1{\"key\"=\"test4\"} " +
                  std::to_string(std::numeric_limits<std::size_t>::max()) +
                  "\n\n");
}
