#include "integration_helpers.hpp"

using namespace test_helpers;

// The controller registers /questions/with-categories (hyphens, not
// underscores). The seed data has 50 questions across 10 categories. We verify
// structural properties rather than exact content because created_at varies.
TEST_CASE("QuestionsWithCategories") {
  auto resp = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/with-categories", "",
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  CHECK(resp.json_body.size() == 50);

  // Every element must have the expected keys.
  std::vector<std::string> expected_keys = {
      "id",       "text",    "category_id", "category_name",
      "language", "min_age", "created_at"};
  for (size_t i = 0; i < resp.json_body.size(); ++i) {
    test_helpers::check_json_has_keys(resp.json_body[i], expected_keys,
                                      "element[" + std::to_string(i) + "]");
  }

  // Spot-check the first question (id=1, "How many bananas do you eat per
  // week?").
  CHECK(resp.json_body[0]["id"] == 1);
  CHECK(resp.json_body[0]["text"] == "How many bananas do you eat per week?");
  CHECK(resp.json_body[0]["category_name"] == "Food");
}
