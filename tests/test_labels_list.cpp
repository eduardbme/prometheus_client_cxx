#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../single_include/prometheus.hpp"

using namespace prometheus::internal;

TEST(LabelsTest, LabelsStr) {
  labels_list l0 = {};
  labels_list l1 = {{"a", "val1"}};
  labels_list l2 = {{"a", "val1"}, {"b", "val2"}};
  labels_list l3 = {{"b", "val2"}, {"a", "val1"}};

  EXPECT_EQ(l0.str(), "");
  EXPECT_EQ(l1.str(), "{\"a\"=\"val1\"}");
  EXPECT_EQ(l2.str(), "{\"a\"=\"val1\",\"b\"=\"val2\"}");
  EXPECT_EQ(l3.str(), "{\"a\"=\"val1\",\"b\"=\"val2\"}");
}

TEST(LabelsTest, LabelsMerge) {
  labels_list l1 = {{"a", "val1"}};
  labels_list l2 = {{"b", "val2"}};
  labels_list l3 = {{"a", "val1"}, {"b", "val2"}};

  EXPECT_EQ(l1 + l2, l3);
}

TEST(LabelsTest, LabelsEquality) {
  labels_list l1 = {};
  labels_list l2 = {};

  labels_list l3 = {{"a", "val1"}, {"b", "val2"}};
  labels_list l4 = {{"b", "val2"}, {"a", "val1"}};

  EXPECT_EQ(l1, l2);
  EXPECT_EQ(l3, l4);
}

TEST(LabelsTest, LabelsInequality) {
  labels_list l1 = {{"a", "val1"}};
  labels_list l2 = {{"a", "val2"}};

  labels_list l3 = {{"a", "val1"}};
  labels_list l4 = {{"b", "val1"}};

  EXPECT_NE(l1, l2);
  EXPECT_NE(l3, l4);
}

TEST(LabelsTest, OperatorLess) {
  labels_list l1 = {{"a", "val1"}};
  labels_list l2 = {{"a", "val2"}};

  labels_list l3 = {{"a", "val1"}};
  labels_list l4 = {{"b", "val1"}};

  EXPECT_LT(l1, l2);
  EXPECT_LT(l3, l4);
}
