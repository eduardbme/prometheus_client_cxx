#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../single_include/prometheus.hpp"

using namespace prometheus::internal;

// TODO: rename tests

TEST(LabelTest, Str) {
  label l1 = {"a", "val1"};

  EXPECT_EQ(l1.str(), "\"a\"=\"val1\"");
}

TEST(LabelTest, OperatorEqual) {
  EXPECT_EQ(label({"a", "val1"}), label({"a", "val1"}));
}

TEST(LabelTest, OperatorNotEqual) {
  EXPECT_NE(label({"a", "val1"}), label({"a", "val2"}));
  EXPECT_NE(label({"a", "val1"}), label({"b", "val1"}));
}

TEST(LabelTest, OperatorLess) {
  EXPECT_LT(label({"a", "val1"}), label({"a", "val2"}));
  EXPECT_LT(label({"a", "val1"}), label({"b", "val1"}));
}

TEST(LabelTest, OperatorStreamOutput) {
  label l{"a", "val1"};

  std::stringstream ss;
  ss << l;

  EXPECT_EQ(ss.str(), l.str());
}
