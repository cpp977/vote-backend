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
  CHECK(resp.json_body.size() == 100);

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

TEST_CASE("GetAnswerOptions for question 1") {
  auto resp = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/1/answers", "", "application/json",
      global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  // Question 1 ("How many bananas do you eat per week?") has 5 answer options.
  CHECK(resp.json_body.size() == 5);

  // Each element must have id, question_id, and text.
  std::vector<std::string> expected_keys = {"id", "question_id", "text"};
  for (size_t i = 0; i < resp.json_body.size(); ++i) {
    test_helpers::check_json_has_keys(resp.json_body[i], expected_keys,
                                      "answer[" + std::to_string(i) + "]");
  }

  // Verify the answer option texts match the seed data.
  std::vector<std::string> expected_texts = {"0", "1-2", "3-5", "6-10",
                                             "More than 10"};
  for (size_t i = 0; i < resp.json_body.size(); ++i) {
    CHECK(resp.json_body[i]["text"] == expected_texts[i]);
  }
}

TEST_CASE("GetAnswerOptions for non-existent question returns 404") {
  auto resp = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/99999/answers", "",
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 404);
}

TEST_CASE("GetQuestionsByLanguage returns only English questions") {
  auto resp = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/lang/en", "", "application/json",
      global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  // The seed data has 50 English questions (5 per category × 10 categories).
  CHECK(resp.json_body.size() == 50);

  for (size_t i = 0; i < resp.json_body.size(); ++i) {
    CHECK(resp.json_body[i]["language"] == "en");
  }
}

TEST_CASE("GetQuestionsByLanguage returns only German questions") {
  auto resp = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/lang/de", "", "application/json",
      global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  // The seed data has 50 German questions (5 per category × 10 categories).
  CHECK(resp.json_body.size() == 50);

  for (size_t i = 0; i < resp.json_body.size(); ++i) {
    CHECK(resp.json_body[i]["language"] == "de");
  }
}

TEST_CASE("GetQuestionsByLanguage for unknown language returns empty array") {
  auto resp = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/lang/fr", "", "application/json",
      global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  CHECK(resp.json_body.empty());
}

TEST_CASE("GetQuestionsByLanguage returns well-known English question texts") {
  auto resp = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/lang/en", "", "application/json",
      global_fixture.access_token);
  CHECK(resp.status == 200);

  // Collect all returned texts.
  std::vector<std::string> texts;
  for (const auto& q : resp.json_body) {
    texts.push_back(q["text"].get<std::string>());
  }

  // Spot-check that known English seed questions are present.
  CHECK(std::find(texts.begin(), texts.end(),
                  "How many bananas do you eat per week?") != texts.end());
  CHECK(std::find(texts.begin(), texts.end(), "Do you have an own car?") !=
        texts.end());
  CHECK(std::find(texts.begin(), texts.end(),
                  "How often do you exercise per week?") != texts.end());
}
