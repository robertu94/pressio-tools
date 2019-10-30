#include "gtest/gtest.h"

#include "utils/fuzzy_matcher.h"

namespace {
  struct TestCases {
    std::string test;
    std::optional<size_t> expected;
  };
  std::ostream& operator<<(std::ostream& out, TestCases const& test_case) {
    return out << test_case.test;
  }
  std::ostream& operator<<(std::ostream& out, std::optional<size_t> const& expected) {
    if (expected) { return out << *expected; }
    return out << "nullopt";
  }
  std::vector<TestCases> test_cases{
    {"f", std::nullopt},
    {"foo", std::nullopt},
    {"fooz", 0},
    {"foobar", 2},
    {"b", 1},
    {"ba", 1},
    {"bar", 1},
    {"barz", 1}, //since bar is unambigious, the remainder is ignored
  };
}
class TrieTests: public ::testing::TestWithParam<TestCases> {
  protected:
    std::vector<std::string> candidates{"fooz", "bar", "foobar"};
};

TEST_P(TrieTests, TestMatches) { 
  {
    auto& [test, expected] = GetParam();
    auto result = fuzzy_match(test, std::begin(candidates),std::end(candidates));
    EXPECT_EQ(result, expected);
  }
}

TEST(TrieTests, InvalidCandidates) {
  std::vector<std::string> candidates{"foo", "foobar"};
  EXPECT_THROW(fuzzy_match("ignored", std::begin(candidates), std::end(candidates)), std::logic_error);
  EXPECT_THROW(fuzzy_match("ignored", std::rbegin(candidates), std::rend(candidates)), std::logic_error);
}

INSTANTIATE_TEST_SUITE_P(TrieAllCorrectnessTests,
    TrieTests,
    testing::ValuesIn(test_cases)
    );
