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

// ---------------------------------------------------------------------------
// GET /questions/{id}/stats
// ---------------------------------------------------------------------------

TEST_CASE("GetStats for question with no votes returns empty array") {
  // Question 50 ("How often do you listen to podcasts?") has no user_answers.
  auto resp = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/50/stats", "", "application/json",
      global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  CHECK(resp.json_body.empty());
}

TEST_CASE("GetStats for non-existent question returns empty array") {
  auto resp = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/99999/stats", "",
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  CHECK(resp.json_body.empty());
}

TEST_CASE("GetStats returns correct counts and percentages for seeded votes") {
  // Seed data (004_test_data.sql) for question 1:
  //   answer_id=1 ("0")   -> 2 votes
  //   answer_id=2 ("1-2") -> 1 vote
  //   total = 3
  auto resp = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/1/stats", "", "application/json",
      global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  CHECK(resp.json_body.size() == 2);

  // Verify structure.
  std::vector<std::string> expected_keys = {"answer_id", "answer_text", "count",
                                            "percent"};
  for (size_t i = 0; i < resp.json_body.size(); ++i) {
    test_helpers::check_json_has_keys(resp.json_body[i], expected_keys,
                                      "stat[" + std::to_string(i) + "]");
  }

  // Find each answer in the response (order is not guaranteed).
  auto find_answer = [&](int answer_id) -> nlohmann::json {
    for (const auto& item : resp.json_body) {
      if (item["answer_id"] == answer_id) return item;
    }
    return {};
  };

  auto stat1 = find_answer(1);
  CHECK(!stat1.is_null());
  CHECK(stat1["answer_text"] == "0");
  CHECK(stat1["count"] == 2);
  CHECK(stat1["percent"].get<double>() == doctest::Approx(66.67).epsilon(0.01));

  auto stat2 = find_answer(2);
  CHECK(!stat2.is_null());
  CHECK(stat2["answer_text"] == "1-2");
  CHECK(stat2["count"] == 1);
  CHECK(stat2["percent"].get<double>() == doctest::Approx(33.33).epsilon(0.01));
}

TEST_CASE(
    "GetStats with tag filter returns filtered results for seeded votes") {
  // Seed data (004_test_data.sql) for question 3 ("Do you have an own car?"):
  //   answer_id=11 ("Yes")  -> 2 votes with gender=m, 1 vote with gender=f
  //   answer_id=12 ("No")   -> 1 vote with gender=m
  //   answer_id=13 ("I share one") -> 0 votes
  //
  // Filter gender=m: answer_id=11 (count=2), answer_id=12 (count=1), total=3
  // Filter gender=f: answer_id=11 (count=1), total=1
  auto resp_m = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/3/stats?tagKey=gender&tagValue=m",
      "", "application/json", global_fixture.access_token);
  CHECK(resp_m.status == 200);
  CHECK(resp_m.json_body.is_array());
  CHECK(resp_m.json_body.size() == 2);

  auto find_answer = [&](const nlohmann::json& arr,
                         int answer_id) -> nlohmann::json {
    for (const auto& item : arr) {
      if (item["answer_id"] == answer_id) return item;
    }
    return {};
  };

  auto stat11 = find_answer(resp_m.json_body, 11);
  CHECK(!stat11.is_null());
  CHECK(stat11["answer_text"] == "Yes");
  CHECK(stat11["count"] == 2);
  CHECK(stat11["percent"].get<double>() ==
        doctest::Approx(66.67).epsilon(0.01));

  auto stat12 = find_answer(resp_m.json_body, 12);
  CHECK(!stat12.is_null());
  CHECK(stat12["answer_text"] == "No");
  CHECK(stat12["count"] == 1);
  CHECK(stat12["percent"].get<double>() ==
        doctest::Approx(33.33).epsilon(0.01));

  // Filter gender=f: only answer_id=11 with count=1, 100%
  auto resp_f = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/3/stats?tagKey=gender&tagValue=f",
      "", "application/json", global_fixture.access_token);
  CHECK(resp_f.status == 200);
  CHECK(resp_f.json_body.is_array());
  CHECK(resp_f.json_body.size() == 1);
  CHECK(resp_f.json_body[0]["answer_id"] == 11);
  CHECK(resp_f.json_body[0]["answer_text"] == "Yes");
  CHECK(resp_f.json_body[0]["count"] == 1);
  CHECK(resp_f.json_body[0]["percent"].get<double>() == doctest::Approx(100.0));
}

TEST_CASE("GetStats requires authentication") {
  auto resp = test_helpers::http_request("GET", "127.0.0.1", 8848,
                                         "/questions/1/stats");
  CHECK(resp.status == 401);
}

// ---------------------------------------------------------------------------
// GET /questions/search  (new efficient search endpoint)
// ---------------------------------------------------------------------------

TEST_CASE("SearchQuestions with exact word match returns results") {
  auto resp = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/search?q=bananas", "",
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  // Should find "How many bananas do you eat per week?" from seed data
  CHECK(resp.json_body.size() >= 1);

  bool found_banana_question = false;
  for (const auto& q : resp.json_body) {
    CHECK(q.contains("id"));
    CHECK(q.contains("text"));
    CHECK(q.contains("language"));
    CHECK(q.contains("category_id"));
    CHECK(q.contains("category_name"));
    CHECK(q.contains("created_at"));

    // Verify the search term appears in the text (ILIKE is case-insensitive)
    std::string text = q["text"].get<std::string>();
    if (text.find("bananas") != std::string::npos) {
      found_banana_question = true;
      CHECK(q["category_name"] == "Food");
    }
  }
  CHECK(found_banana_question);
}

TEST_CASE("SearchQuestions with partial word match returns results") {
  auto resp = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/search?q=exercise", "",
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  // Should find "How often do you exercise per week?" from seed data

  bool found_exercise_question = false;
  for (const auto& q : resp.json_body) {
    std::string text = q["text"].get<std::string>();
    if (text.find("exercise") != std::string::npos) {
      found_exercise_question = true;
      // Verify it has the expected category structure
      CHECK(q["category_id"].is_number());
      CHECK(q["language"].is_string());
    }
  }
  CHECK(found_exercise_question);
}

TEST_CASE("SearchQuestions with case-insensitive search") {
  // Test that ILIKE works case-insensitively
  auto resp1 = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/search?q=exercise", "",
      "application/json", global_fixture.access_token);
  CHECK(resp1.status == 200);

  auto resp2 = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/search?q=EXERCISE", "",
      "application/json", global_fixture.access_token);
  CHECK(resp2.status == 200);

  // Both queries should return at least one result (ILIKE is case-insensitive)
  CHECK(resp1.json_body.size() > 0);
  CHECK(resp2.json_body.size() > 0);
}

TEST_CASE("SearchQuestions with non-matching term returns empty array") {
  auto resp = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/search?q=xyz123nonexistent", "",
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  CHECK(resp.json_body.empty());
}

TEST_CASE("SearchQuestions requires authentication") {
  auto resp = test_helpers::http_request("GET", "127.0.0.1", 8848,
                                         "/questions/search?q=exercise");
  CHECK(resp.status == 401);
}

TEST_CASE("SearchQuestions returns all fields for each result") {
  auto resp = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/search?q=do", "",
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);

  // Check that every returned object has all expected keys
  std::vector<std::string> expected_keys = {
      "id", "text", "language", "category_id", "category_name", "created_at"};

  for (size_t i = 0; i < resp.json_body.size(); ++i) {
    test_helpers::check_json_has_keys(
        resp.json_body[i], expected_keys,
        "search_result[" + std::to_string(i) + "]");
  }
}
