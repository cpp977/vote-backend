#include <set>

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
  CHECK(resp.json_body[0]["id"] == 2);
  CHECK(resp.json_body[0]["text"] == "Wie viele Bananen essen Sie pro Woche?");
  CHECK(resp.json_body[0]["category_name"] == "Essen");
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

// ---------------------------------------------------------------------------
// POST /questions/restSearch  (RESTful search endpoint)
// ---------------------------------------------------------------------------

TEST_CASE("RestSearchQuestions with search filter returns results") {
  nlohmann::json request_body;
  request_body["search"] = "bananas";

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", request_body.dump(),
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

    // Verify the search term appears in the text
    std::string text = q["text"].get<std::string>();
    if (text.find("bananas") != std::string::npos) {
      found_banana_question = true;
      CHECK(q["category_name"] == "Food");
    }
  }
  CHECK(found_banana_question);
}

TEST_CASE("RestSearchQuestions with language filter returns results") {
  nlohmann::json request_body;
  request_body["language"] = "en";

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", request_body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  // Should return all 50 English questions from seed data
  CHECK(resp.json_body.size() == 50);

  for (const auto& q : resp.json_body) {
    CHECK(q["language"] == "en");
  }
}

TEST_CASE("RestSearchQuestions with categoryIds filter returns results") {
  nlohmann::json request_body;
  request_body["categoryIds"] = nlohmann::json::array({1, 2, 3, 11, 12, 13});
  // Use a high limit so the full filtered set is returned (the default limit
  // of 50 would otherwise truncate it). Query both language variants of
  // Food/Mobility/Lifestyle: EN categories 1,2,3 and DE categories 11,12,13
  // -> 27 questions per language, 54 total.
  request_body["limit"] = 1000;

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", request_body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  CHECK(resp.json_body.size() == 54);

  for (const auto& q : resp.json_body) {
    int category_id = q["category_id"].get<int>();
    // Returned questions must belong to the queried categories (EN 1-3 or DE
    // 11-13).
    bool in_filter = (category_id >= 1 && category_id <= 3) ||
                     (category_id >= 11 && category_id <= 13);
    CHECK(in_filter);
  }
}

TEST_CASE(
    "RestSearchQuestions with age filter returns questions at or above the "
    "minimum age") {
  nlohmann::json request_body;
  request_body["age"] = 18;
  request_body["limit"] = 1000;

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", request_body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  CHECK(resp.json_body.size() == 98);
}

TEST_CASE(
    "RestSearchQuestions with age filter excludes questions above the minimum "
    "age") {
  nlohmann::json request_body;
  request_body["age"] = 7;
  request_body["limit"] = 1000;

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", request_body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  CHECK(resp.json_body.size() == 37);

  const std::set<int> expected = {7, 9};
  for (const auto& q : resp.json_body) {
    CHECK(expected.count(q["id"].get<int>()) == 0);
  }
}

TEST_CASE(
    "RestSearchQuestions with age above all minimum ages returns nothing") {
  // No seeded question has min_age >= 22, so the filter must match nothing.
  nlohmann::json request_body;
  request_body["age"] = -1;
  request_body["limit"] = 1000;

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", request_body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  bool no_results = resp.json_body.is_null() ||
                    (resp.json_body.is_array() && resp.json_body.empty());
  CHECK(no_results);
}

TEST_CASE("RestSearchQuestions with age = 0 returns all questions (default)") {
  // age = 0 equals the filter's nullValue, so the WHERE clause is skipped and
  // the result is identical to sending no age at all.
  nlohmann::json body_with_age;
  body_with_age["age"] = 100;
  body_with_age["limit"] = 0;  // limit = 0 means "no limit" -> all questions

  auto resp_age = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", body_with_age.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp_age.status == 200);
  CHECK(resp_age.json_body.is_array());

  nlohmann::json body_no_age;
  body_no_age["limit"] = 0;

  auto resp_no_age = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", body_no_age.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp_no_age.status == 200);
  CHECK(resp_no_age.json_body.is_array());

  // The two requests must return the exact same set of questions.
  CHECK(resp_age.json_body.size() == resp_no_age.json_body.size());
  std::set<int> ids_age;
  for (const auto& q : resp_age.json_body) ids_age.insert(q["id"].get<int>());
  for (const auto& q : resp_no_age.json_body) {
    CHECK(ids_age.count(q["id"].get<int>()) == 1);
  }
}

TEST_CASE("RestSearchQuestions with age filter combined with language filter") {
  // All questions carrying a non-zero min_age in the test data are English, so
  // combining age = 21 with language = "en" still yields questions 7 and 9,
  // while language = "de" yields nothing (cross-filter AND semantics).
  nlohmann::json en_body;
  en_body["age"] = 11;
  en_body["language"] = "en";
  en_body["limit"] = 1000;

  auto resp_en = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", en_body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp_en.status == 200);
  CHECK(resp_en.json_body.is_array());
  CHECK(resp_en.json_body.size() == 22);
  const std::set<int> expected_en = {7, 9};
  for (const auto& q : resp_en.json_body) {
    CHECK(expected_en.count(q["id"].get<int>()) == 0);
  }

  nlohmann::json de_body;
  de_body["age"] = -1;
  de_body["language"] = "de";
  de_body["limit"] = 1000;

  auto resp_de = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", de_body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp_de.status == 200);
  bool no_results = resp_de.json_body.is_null() ||
                    (resp_de.json_body.is_array() && resp_de.json_body.empty());
  CHECK(no_results);
}

TEST_CASE("RestSearchQuestions with no filters returns all questions") {
  nlohmann::json request_body;

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", request_body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  // With default pagination (limit=50), we get first 50 questions
  CHECK(resp.json_body.size() == 50);
}

TEST_CASE("RestSearchQuestions with empty search returns all questions") {
  nlohmann::json request_body;
  request_body["search"] = "";

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", request_body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  // Empty search returns all questions
  // With default limit=50, we get first 50
  CHECK(resp.json_body.size() == 50);
}

TEST_CASE("RestSearchQuestions requires JSON body") {
  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", "",
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 400);
}

TEST_CASE("RestSearchQuestions with malformed JSON returns 400") {
  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch",
      "{\"search\": \"bananas\"", "application/json",
      global_fixture.access_token);
  CHECK(resp.status == 400);
}

TEST_CASE("RestSearchQuestions returns correct fields in response") {
  nlohmann::json request_body;
  request_body["search"] = "car";

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", request_body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);

  // Check that every returned object has all expected keys
  std::vector<std::string> expected_keys = {"id", "text", "language",
                                            "category_id", "category_name"};

  for (size_t i = 0; i < resp.json_body.size(); ++i) {
    test_helpers::check_json_has_keys(
        resp.json_body[i], expected_keys,
        "rest_search_result[" + std::to_string(i) + "]");
  }
}

// ---------------------------------------------------------------------------
// POST /questions/restSearch  pagination tests
// ---------------------------------------------------------------------------

TEST_CASE("RestSearchQuestions with offset parameter paginates correctly") {
  nlohmann::json request_body;
  request_body["offset"] = 10;

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", request_body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  // 100 questions total; with offset=10 and the default limit=50 we get the
  // next 50 questions (ids 90 down to 41, ordered by created_at DESC / id
  // DESC).
  CHECK(resp.json_body.size() == 50);

  // All returned questions must lie in the expected id window.
  for (size_t i = 0; i < resp.json_body.size(); ++i) {
    int id = resp.json_body[i]["id"].get<int>();
    CHECK(id >= 41);
    CHECK(id <= 90);
  }
}

TEST_CASE("RestSearchQuestions with limit parameter restricts results count") {
  nlohmann::json request_body;
  request_body["limit"] = 5;

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", request_body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  // Should return only 5 questions
  CHECK(resp.json_body.size() == 5);
}

TEST_CASE("RestSearchQuestions with limit=0 returns all questions") {
  nlohmann::json request_body;
  request_body["limit"] = 0;

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", request_body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  // limit=0 is treated as "no limit" by the backend (the LIMIT clause is only
  // added when limit > 0), so all 100 seeded questions are returned.
  CHECK(resp.json_body.is_array());
  CHECK(resp.json_body.size() == 100);
}

TEST_CASE("RestSearchQuestions with limit > 1000 respects maximum limit") {
  nlohmann::json request_body;
  request_body["limit"] = 1500;

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", request_body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  // The backend caps the limit at 1000; with only 100 seeded questions it
  // returns all of them.
  CHECK(resp.json_body.size() == 100);
}

TEST_CASE(
    "RestSearchQuestions with offset > total records returns no results") {
  nlohmann::json request_body;
  request_body["offset"] = 200;

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", request_body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  // When the offset is past the end of the result set the backend returns no
  // rows. The response body is `null` rather than an empty JSON array, but we
  // also accept an empty array for robustness.
  bool no_results = resp.json_body.is_null() ||
                    (resp.json_body.is_array() && resp.json_body.empty());
  CHECK(no_results);
}

TEST_CASE(
    "RestSearchQuestions with offset and limit together paginates correctly") {
  nlohmann::json request_body;
  request_body["offset"] = 20;
  request_body["limit"] = 10;

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", request_body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  // Should return exactly 10 questions starting from offset 20
  CHECK(resp.json_body.size() == 10);
}

TEST_CASE("RestSearchQuestions offset and limit override default values") {
  nlohmann::json request_body;
  request_body["offset"] = 30;
  request_body["limit"] = 15;

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", request_body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  // Should return exactly 15 questions
  CHECK(resp.json_body.size() == 15);

  // Should NOT return default of 50 (limit 50 as described in comment)
  CHECK(resp.json_body.size() != 50);
}

TEST_CASE("RestSearchQuestions with offset=0 returns first N results") {
  nlohmann::json request_body;
  request_body["offset"] = 0;
  request_body["limit"] = 25;

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", request_body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  // Should return exactly 25 questions. Results are ordered by created_at DESC
  // (newest first), which with the seed data corresponds to id DESC, so the
  // first element is the highest id (100) and the last is 76.
  CHECK(resp.json_body.size() == 25);
  CHECK(resp.json_body[0]["id"].get<int>() == 100);
  CHECK(resp.json_body[24]["id"].get<int>() == 76);

  // Verify the slice is strictly ordered by id descending.
  for (size_t i = 1; i < resp.json_body.size(); ++i) {
    CHECK(resp.json_body[i - 1]["id"].get<int>() >
          resp.json_body[i]["id"].get<int>());
  }
}

TEST_CASE("RestSearchQuestions with negative offset defaults to 0") {
  nlohmann::json request_body;
  request_body["offset"] = -5;

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", request_body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  // A negative offset is clamped to 0, so the default limit=50 applies and we
  // get the 50 most recent questions (ids 100 down to 51).
  CHECK(resp.json_body.size() == 50);
  CHECK(resp.json_body[0]["id"].get<int>() == 100);
}

TEST_CASE("RestSearchQuestions with negative limit defaults to 50") {
  nlohmann::json request_body;
  request_body["limit"] = -10;

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", request_body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  // Should default to limit=50, returning only 50 questions
  CHECK(resp.json_body.size() == 50);
}

TEST_CASE(
    "RestSearchQuestions preserves ORDER BY created_at DESC with pagination") {
  nlohmann::json request_body;
  request_body["limit"] = 5;

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", request_body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());

  // Verify the returned questions are in descending order by created_at
  // (most recent first). We check that the IDs are in descending order
  // Since created_at ordering should match the id ordering in our seed data
  for (size_t i = 1; i < resp.json_body.size(); ++i) {
    CHECK(resp.json_body[i - 1]["id"].get<int>() >
          resp.json_body[i]["id"].get<int>());
  }
}

TEST_CASE(
    "RestSearchQuestions with pagination and filters combines correctly") {
  nlohmann::json request_body;
  request_body["language"] = "en";
  request_body["offset"] = 10;
  request_body["limit"] = 5;

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", request_body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  // Should return 5 English questions starting from offset 10
  CHECK(resp.json_body.size() == 5);

  for (const auto& q : resp.json_body) {
    CHECK(q["language"] == "en");
  }
}

TEST_CASE(
    "RestSearchQuestions large offset with small limit returns correct slice") {
  nlohmann::json request_body;
  request_body["offset"] = 75;
  request_body["limit"] = 3;

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", request_body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  // Should return 3 questions. Ordered by created_at DESC (id DESC), skipping
  // the first 75 rows lands on ids 25, 24, 23.
  CHECK(resp.json_body.size() == 3);

  // Verify IDs are in the expected range (23-25)
  for (const auto& q : resp.json_body) {
    int id = q["id"].get<int>();
    CHECK(id >= 23);
    CHECK(id <= 25);
  }
}

TEST_CASE("RestSearchQuestions zero offset with large limit works correctly") {
  nlohmann::json request_body;
  request_body["offset"] = 0;
  request_body["limit"] = 150;

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", request_body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  // Should respect the 1000 limit but return at most 100 questions
  CHECK(resp.json_body.size() == 100);
}

TEST_CASE("RestSearchQuestions without explicit offset defaults to 0") {
  nlohmann::json request_body;
  request_body["limit"] = 10;

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", request_body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  // Should default to offset=0, returning the first 10 questions in
  // created_at DESC order (ids 100 down to 91).
  CHECK(resp.json_body.size() == 10);
  // The most recent question (highest id) is returned first.
  CHECK(resp.json_body[0]["id"].get<int>() == 100);
  CHECK(resp.json_body[9]["id"].get<int>() == 91);
}

TEST_CASE("RestSearchQuestions without explicit limit defaults to 50") {
  nlohmann::json request_body;
  request_body["offset"] = 20;

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", request_body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  // Should default to limit=50, returning 50 questions.
  CHECK(resp.json_body.size() == 50);

  // Ordered by created_at DESC (id DESC), offset=20 skips ids 100..81 and
  // returns ids 80 down to 31.
  for (const auto& q : resp.json_body) {
    int id = q["id"].get<int>();
    CHECK(id >= 31);
    CHECK(id <= 80);
  }
}

// ---------------------------------------------------------------------------
// POST /questions/{id}/answer  (anonymous, one-answer-per-user enforcement)
//
// The backend inserts an anonymous hash of the user id (never the raw id)
// together with the question_id into the `question_user` table, whose unique
// primary key enforces that a user may answer a question only once. The
// answer_option id ranges below come from the seed data (003_seed_data.sql):
//   question 1 -> answer ids  1..5
//   question 2 -> answer ids  6..10
//   question 3 -> answer ids 11..13
//   question 4 -> answer ids 14..16
//   question 5 -> answer ids 17..21
//   question 6 -> answer ids 22..26
// ---------------------------------------------------------------------------

TEST_CASE("AnswerQuestion creates a new answer (201 Created)") {
  // answer_id = 6 belongs to question 2.
  nlohmann::json body;
  body["answer_id"] = 6;
  body["tags"] = {{"gender", "m"}, {"age", 30}};

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/2/answer", body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 201);
  CHECK(resp.json_body.contains("id"));
  CHECK(resp.json_body["id"].is_number());
  CHECK(resp.json_body["question_id"] == 2);
  CHECK(resp.json_body["answer_id"] == 6);
}

TEST_CASE("AnswerQuestion rejects a second answer with 409 Conflict") {
  // First answer for question 4 must succeed.
  nlohmann::json first;
  first["answer_id"] = 14;  // belongs to question 4
  auto r1 = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/4/answer", first.dump(),
      "application/json", global_fixture.access_token);
  CHECK(r1.status == 201);

  // A second answer for the same question (even a different option) must be
  // rejected because the user already answered it.
  nlohmann::json second;
  second["answer_id"] = 15;  // also belongs to question 4
  auto r2 = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/4/answer", second.dump(),
      "application/json", global_fixture.access_token);
  CHECK(r2.status == 409);
  CHECK(r2.json_body.contains("error"));
}

TEST_CASE(
    "AnswerQuestion with answer_id not belonging to the question is 400") {
  // answer_id = 1 belongs to question 1, not question 5.
  nlohmann::json body;
  body["answer_id"] = 1;

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/5/answer", body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 400);
  CHECK(resp.json_body.contains("error"));
}

TEST_CASE("AnswerQuestion without answer_id is 400") {
  nlohmann::json body = {{"tags", {{"gender", "m"}}}};

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/6/answer", body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 400);
  CHECK(resp.json_body.contains("error"));
}

TEST_CASE("AnswerQuestion requires authentication (401)") {
  nlohmann::json body;
  body["answer_id"] = 6;

  // No bearer token -> 401.
  auto resp = test_helpers::http_request("POST", "127.0.0.1", 8848,
                                         "/questions/2/answer", body.dump(),
                                         "application/json", "");
  CHECK(resp.status == 401);
}

// ---------------------------------------------------------------------------
// GET /categories  (category language column added, mirroring questions)
// ---------------------------------------------------------------------------

TEST_CASE("GetCategories returns the language field for every row") {
  // The categories table now carries a `language` column (NOT NULL, FK to
  // languages), mirroring how `questions.language` works. GET /categories
  // must surface it for each seeded category.
  auto resp = test_helpers::http_request("GET", "127.0.0.1", 8848,
                                         "/categories", "", "application/json",
                                         global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  // 20 seed categories (10 EN + 10 DE) from 003_seed_data.sql.
  CHECK(resp.json_body.size() == 20);

  std::vector<std::string> expected_keys = {"id", "name", "language"};
  for (size_t i = 0; i < resp.json_body.size(); ++i) {
    test_helpers::check_json_has_keys(resp.json_body[i], expected_keys,
                                      "category[" + std::to_string(i) + "]");
    // language must be a 2-char ISO code, e.g. "en".
    CHECK(resp.json_body[i]["language"].is_string());
    CHECK(resp.json_body[i]["language"].get<std::string>().size() == 2);
  }
}

TEST_CASE("CreateCategory without language is rejected (400)") {
  // `language` is now a NOT NULL column, so a category created without it
  // must be rejected by the model validation before any DB write happens
  // (no row is inserted, so this test leaves no residue).
  nlohmann::json body;
  body["name"] = "TestCategoryWithoutLanguage";

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/categories", body.dump(), "application/json",
      global_fixture.access_token);
  CHECK(resp.status == 400);
  CHECK(resp.json_body.contains("error"));
}

TEST_CASE("GetCategoriesByLanguage returns only English categories") {
  // Mirrors GetQuestionsByLanguage: the dedicated /categories/lang/{lang}
  // route must return only categories whose `language` matches the path param.
  auto resp = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/categories/lang/en", "", "application/json",
      global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  // Seed data provides 10 English categories.
  CHECK(resp.json_body.size() == 10);
  for (size_t i = 0; i < resp.json_body.size(); ++i) {
    CHECK(resp.json_body[i]["language"] == "en");
    CHECK(resp.json_body[i].contains("id"));
    CHECK(resp.json_body[i].contains("name"));
  }
}

TEST_CASE("GetCategoriesByLanguage returns only German categories") {
  auto resp = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/categories/lang/de", "", "application/json",
      global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  CHECK(resp.json_body.size() == 10);
  for (size_t i = 0; i < resp.json_body.size(); ++i) {
    CHECK(resp.json_body[i]["language"] == "de");
    CHECK(resp.json_body[i].contains("id"));
    CHECK(resp.json_body[i].contains("name"));
  }
}

TEST_CASE("GetCategoriesByLanguage for unknown language returns empty array") {
  auto resp = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/categories/lang/zz", "", "application/json",
      global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  CHECK(resp.json_body.empty());
}

TEST_CASE("CreateCategory with language succeeds (200)") {
  // A well-formed category carrying a valid language code must be accepted.
  // Uses a unique name so it does not collide with the UNIQUE constraint on
  // the seed categories. Note: the generated REST controller's create()
  // returns 200 (not 201) for a successful insertion.
  nlohmann::json body;
  body["name"] = "TestCategoryWithLanguage";
  body["language"] = "en";

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/categories", body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.contains("id"));
  CHECK(resp.json_body.contains("name"));
  CHECK(resp.json_body.contains("language"));
  CHECK(resp.json_body["name"] == "TestCategoryWithLanguage");
  CHECK(resp.json_body["language"] == "en");
}
