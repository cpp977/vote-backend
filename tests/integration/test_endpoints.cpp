#include <ctime>
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

TEST_CASE("GetAnswerOptions is accessible without authentication") {
  auto resp = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/1/answers", "", "application/json");
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  CHECK(resp.json_body.size() == 5);
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

TEST_CASE("GetStats for question with no votes reports insufficient data") {
  // Question 50 ("How often do you listen to podcasts?") has no user_answers.
  auto resp = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/50/stats", "", "application/json",
      global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_object());
  CHECK(resp.json_body["status"] == "insufficient_data");
  CHECK(resp.json_body["message"] ==
        "Not enough responses to display statistics for this filter.");
  CHECK(resp.json_body["answers"].is_array());
  CHECK(resp.json_body["answers"].empty());
}

TEST_CASE("GetStats for non-existent question reports insufficient data") {
  auto resp = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/99999/stats", "",
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_object());
  CHECK(resp.json_body["status"] == "insufficient_data");
  CHECK(resp.json_body["answers"].is_array());
  CHECK(resp.json_body["answers"].empty());
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
  CHECK(resp.json_body.is_object());
  CHECK(resp.json_body["status"] == "ok");
  CHECK(resp.json_body["message"] == "");
  CHECK(resp.json_body["answers"].is_array());

  const auto& answers = resp.json_body["answers"];
  CHECK(answers.size() == 2);

  // Verify structure.
  std::vector<std::string> expected_keys = {"answer_id", "answer_text", "count",
                                            "percent"};
  for (size_t i = 0; i < answers.size(); ++i) {
    test_helpers::check_json_has_keys(answers[i], expected_keys,
                                      "stat[" + std::to_string(i) + "]");
  }

  // Find each answer in the response (order is not guaranteed).
  auto find_answer = [&](int answer_id) -> nlohmann::json {
    for (const auto& item : answers) {
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
    "GetStats with tag parameters returns filtered results for seeded "
    "votes") {
  // Seed data (004_test_data.sql) for question 3 ("Do you have an own car?"):
  //   answer_id=11 ("Yes")  -> 2 votes with gender=m, 1 vote with gender=w
  //   answer_id=12 ("No")   -> 1 vote with gender=m
  //   answer_id=13 ("I share one") -> 0 votes
  //
  // Filter gender=m: answer_id=11 (count=2), answer_id=12 (count=1), total=3
  // Filter gender=w: answer_id=11 (count=1), total=1
  auto resp_m = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/3/stats?gender=m", "",
      "application/json", global_fixture.access_token);
  CHECK(resp_m.status == 200);
  CHECK(resp_m.json_body.is_object());
  CHECK(resp_m.json_body["status"] == "ok");
  CHECK(resp_m.json_body["answers"].is_array());
  CHECK(resp_m.json_body["answers"].size() == 2);

  auto find_answer = [&](const nlohmann::json& arr,
                         int answer_id) -> nlohmann::json {
    for (const auto& item : arr) {
      if (item["answer_id"] == answer_id) return item;
    }
    return {};
  };

  const auto& answers_m = resp_m.json_body["answers"];
  auto stat11 = find_answer(answers_m, 11);
  CHECK(!stat11.is_null());
  CHECK(stat11["answer_text"] == "Yes");
  CHECK(stat11["count"] == 2);
  CHECK(stat11["percent"].get<double>() ==
        doctest::Approx(66.67).epsilon(0.01));

  auto stat12 = find_answer(answers_m, 12);
  CHECK(!stat12.is_null());
  CHECK(stat12["answer_text"] == "No");
  CHECK(stat12["count"] == 1);
  CHECK(stat12["percent"].get<double>() ==
        doctest::Approx(33.33).epsilon(0.01));

  // Filter gender=w: only answer_id=11 with count=1, 100%. The seed data
  // uses the canonical gender vocabulary (m/w/d) that the backend derives
  // from the users table.
  auto resp_w = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/3/stats?gender=w", "",
      "application/json", global_fixture.access_token);
  CHECK(resp_w.status == 200);
  CHECK(resp_w.json_body["status"] == "ok");
  CHECK(resp_w.json_body["answers"].is_array());
  CHECK(resp_w.json_body["answers"].size() == 1);
  CHECK(resp_w.json_body["answers"][0]["answer_id"] == 11);
  CHECK(resp_w.json_body["answers"][0]["answer_text"] == "Yes");
  CHECK(resp_w.json_body["answers"][0]["count"] == 1);
  CHECK(resp_w.json_body["answers"][0]["percent"].get<double>() ==
        doctest::Approx(100.0));
}

TEST_CASE("GetStats ignores unknown tag parameters") {
  // birth_year is not a filterable tag (it is bucketed into age_bucket), so a
  // birth_year query parameter is simply ignored: the request behaves like an
  // unfiltered query.
  auto resp = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/3/stats?birth_year=1985", "",
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body["status"] == "ok");
  CHECK(resp.json_body["answers"].size() == 2);
}

TEST_CASE("GetStats rejects invalid tag values with 400") {
  // "x" is not a valid gender value.
  auto resp_bad_gender = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/3/stats?gender=x", "",
      "application/json", global_fixture.access_token);
  CHECK(resp_bad_gender.status == 400);
  CHECK(resp_bad_gender.json_body.contains("error"));

  // Malformed age bucket label.
  auto resp_bad_bucket = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/3/stats?age_bucket=abc", "",
      "application/json", global_fixture.access_token);
  CHECK(resp_bad_bucket.status == 400);
  CHECK(resp_bad_bucket.json_body.contains("error"));

  // Empty parameter values are treated as absent (no filtering).
  auto resp_empty = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/3/stats?gender=", "",
      "application/json", global_fixture.access_token);
  CHECK(resp_empty.status == 200);
  CHECK(resp_empty.json_body["status"] == "ok");
}

TEST_CASE("GetStats is accessible without authentication") {
  auto resp = test_helpers::http_request("GET", "127.0.0.1", 8848,
                                         "/questions/1/stats");
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_object());
  CHECK(resp.json_body.contains("status"));
  CHECK(resp.json_body["answers"].is_array());
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
// Admin endpoints
// ---------------------------------------------------------------------------

TEST_CASE("Admin only endpoint lists all usernames") {
  // Login as an admin user (Admin from seed data)
  auto admin_token =
      test_helpers::login_only("127.0.0.1", 8848, "Admin", "12345678");
  auto resp =
      test_helpers::http_request("GET", "127.0.0.1", 8848, "/admin/users", "",
                                 "application/json", admin_token);
  // The response should be a JSON array (will be checked by framework)
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  // The endpoint should return usernames and ids
  for (size_t i = 0; i < resp.json_body.size(); ++i) {
    CHECK(resp.json_body[i].is_object());
    CHECK(resp.json_body[i].contains("id"));
    CHECK(resp.json_body[i].contains("username"));
    CHECK(resp.json_body[i]["id"].is_string());
    CHECK(resp.json_body[i]["username"].is_string());
    CHECK(!resp.json_body[i]["username"].empty());
  }
}

TEST_CASE("Admin endpoint requires admin privileges") {
  // Login as a non-admin user (Jim / 12345678 from seed data)
  auto user_token =
      test_helpers::login_only("127.0.0.1", 8848, "Jim", "12345678");
  // This user should NOT have access to the admin endpoint
  auto resp =
      test_helpers::http_request("GET", "127.0.0.1", 8848, "/admin/users", "",
                                 "application/json", user_token);
  // Should return 403 Forbidden as per AdminAuthFilter
  CHECK(resp.status == 403);
  CHECK(resp.json_body.contains("error"));
}

TEST_CASE("Admin endpoint returns usernames and IDs, no sensitive data") {
  // Login as an admin user
  auto admin_token =
      test_helpers::login_only("127.0.0.1", 8848, "Admin", "12345678");
  auto resp =
      test_helpers::http_request("GET", "127.0.0.1", 8848, "/admin/users", "",
                                 "application/json", admin_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());

  // Verify that both id and username are returned (not just strings)
  for (size_t i = 0; i < resp.json_body.size(); ++i) {
    CHECK(resp.json_body[i].is_object());
    CHECK(resp.json_body[i].contains("id"));
    CHECK(resp.json_body[i].contains("username"));
    CHECK(resp.json_body[i]["id"].is_string());
    CHECK(resp.json_body[i]["username"].is_string());
    CHECK(!resp.json_body[i]["username"].empty());
    // Verify that sensitive fields (email, password_hash, etc.) are not exposed
    CHECK_FALSE(resp.json_body[i].contains("email"));
    CHECK_FALSE(resp.json_body[i].contains("password_hash"));
  }
}

// ---------------------------------------------------------------------------
// GET /admin/users/{id}  (single user endpoint)
// ---------------------------------------------------------------------------

TEST_CASE(
    "Admin endpoint gets user by ID returns all fields except password_hash") {
  // Login as an admin user
  auto admin_token =
      test_helpers::login_only("127.0.0.1", 8848, "Admin", "12345678");
  // Get Jim's UUID from the seed data
  auto jimId = test_helpers::get_userid("Jim");
  auto resp = test_helpers::http_request("GET", "127.0.0.1", 8848,
                                         "/admin/users/" + jimId, "",
                                         "application/json", admin_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_object());

  // Verify all expected fields are present (including the new is_active field)
  std::vector<std::string> expected_fields = {
      "id",          "username",   "email",      "birth_year", "gender",
      "nationality", "created_at", "updated_at", "is_admin",   "is_active"};
  for (const auto& field : expected_fields) {
    CHECK(resp.json_body.contains(field));
  }

  // Verify field types
  CHECK(resp.json_body["id"].is_string());
  CHECK(resp.json_body["username"].is_string());
  CHECK(resp.json_body["email"].is_string());
  CHECK(resp.json_body["birth_year"].is_number());
  CHECK(resp.json_body["gender"].is_string());
  CHECK(resp.json_body["nationality"].is_string());
  CHECK(resp.json_body["created_at"].is_string());
  CHECK(resp.json_body["updated_at"].is_string());
  CHECK(resp.json_body["is_admin"].is_boolean());
  CHECK(resp.json_body["is_active"].is_boolean());

  // Verify password_hash is NOT present (security requirement)
  CHECK_FALSE(resp.json_body.contains("password_hash"));

  // Verify specific Jim user data from seed data
  CHECK(resp.json_body["username"] == "Jim");
  CHECK(resp.json_body["email"] == "jim@example.com");
  CHECK(resp.json_body["gender"] == "m");
  CHECK(resp.json_body["is_admin"] == false);
}

TEST_CASE("Admin endpoint gets non-existent user by ID returns 404") {
  // Login as an admin user
  auto admin_token =
      test_helpers::login_only("127.0.0.1", 8848, "Admin", "12345678");
  auto resp = test_helpers::http_request(
      "GET", "127.0.0.1", 8848,
      "/admin/users/00000000-0000-0000-0000-000000000000", "",
      "application/json", admin_token);
  CHECK(resp.status == 404);
  CHECK(resp.json_body.contains("error"));
}

TEST_CASE("Admin endpoint gets regular user by ID returns complete data") {
  // Login as an admin user
  auto admin_token =
      test_helpers::login_only("127.0.0.1", 8848, "Admin", "12345678");
  // Get Jim's UUID (seed user)
  auto jimId = test_helpers::get_userid("Jim");
  auto resp = test_helpers::http_request("GET", "127.0.0.1", 8848,
                                         "/admin/users/" + jimId, "",
                                         "application/json", admin_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_object());

  // Verify all fields except password_hash are present (including is_active)
  CHECK(resp.json_body.contains("id"));
  CHECK(resp.json_body.contains("username"));
  CHECK(resp.json_body.contains("email"));
  CHECK(resp.json_body.contains("birth_year"));
  CHECK(resp.json_body.contains("gender"));
  CHECK(resp.json_body.contains("nationality"));
  CHECK(resp.json_body.contains("created_at"));
  CHECK(resp.json_body.contains("updated_at"));
  CHECK(resp.json_body.contains("is_admin"));
  CHECK(resp.json_body.contains("is_active"));
  CHECK_FALSE(resp.json_body.contains("password_hash"));

  // Verify Jim's data from seed data (this is a regular user, not admin)
  CHECK(resp.json_body["username"] == "Jim");
  CHECK(resp.json_body["email"] == "jim@example.com");
  CHECK(resp.json_body["gender"] == "m");
  CHECK(resp.json_body["is_admin"] == false);
}

TEST_CASE("Admin endpoint requires admin privileges for single user access") {
  // Login as a non-admin user (Jim / 12345678 from seed data)
  auto user_token =
      test_helpers::login_only("127.0.0.1", 8848, "Jim", "12345678");
  // Get Jim's UUID to use in the URL
  auto jimId = test_helpers::get_userid("Jim");
  // This user should NOT have access to the admin endpoint
  auto resp = test_helpers::http_request("GET", "127.0.0.1", 8848,
                                         "/admin/users/" + jimId, "",
                                         "application/json", user_token);
  // Should return 403 Forbidden as per AdminAuthFilter
  CHECK(resp.status == 403);
  CHECK(resp.json_body.contains("error"));
}

// ---------------------------------------------------------------------------
// Admin endpoints for user status (activate/deactivate)
// ---------------------------------------------------------------------------

TEST_CASE(
    "Admin endpoint sets user inactive (POST /admin/users/{id}/inactive)") {
  // Login as an admin user
  auto admin_token =
      test_helpers::login_only("127.0.0.1", 8848, "Admin", "12345678");
  // Get Jim's UUID - Jim should be active from seed data
  auto jimId = test_helpers::get_userid("Jim");
  auto resp = test_helpers::http_request("POST", "127.0.0.1", 8848,
                                         "/admin/users/" + jimId + "/inactive",
                                         "", "application/json", admin_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.contains("id"));
  CHECK(resp.json_body["id"].is_string());
  CHECK(resp.json_body["id"] == jimId);
  CHECK(resp.json_body.contains("is_active"));
  CHECK(resp.json_body["is_active"] == false);
  CHECK(resp.json_body.contains("message"));
  CHECK(resp.json_body["message"] == "User set to inactive");
}

TEST_CASE("Admin endpoint sets user active (POST /admin/users/{id}/active)") {
  // First, we need to set a user inactive (User 2 / Jim)
  auto admin_token =
      test_helpers::login_only("127.0.0.1", 8848, "Admin", "12345678");
  // Get Jim's UUID
  auto jimId = test_helpers::get_userid("Jim");

  // Now, set the user back to active
  auto resp = test_helpers::http_request("POST", "127.0.0.1", 8848,
                                         "/admin/users/" + jimId + "/active",
                                         "", "application/json", admin_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.contains("id"));
  CHECK(resp.json_body["id"].is_string());
  CHECK(resp.json_body["id"] == jimId);
  CHECK(resp.json_body.contains("is_active"));
  CHECK(resp.json_body["is_active"] == true);
  CHECK(resp.json_body.contains("message"));
  CHECK(resp.json_body["message"] == "User set to active");
}

TEST_CASE("Admin endpoint sets non-existent user inactive returns 404") {
  auto admin_token =
      test_helpers::login_only("127.0.0.1", 8848, "Admin", "12345678");
  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848,
      "/admin/users/00000000-0000-0000-0000-000000000000/inactive", "",
      "application/json", admin_token);
  CHECK(resp.status == 404);
  CHECK(resp.json_body.contains("error"));
}

TEST_CASE("Admin endpoint sets non-existent user active returns 404") {
  auto admin_token =
      test_helpers::login_only("127.0.0.1", 8848, "Admin", "12345678");
  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848,
      "/admin/users/00000000-0000-0000-0000-000000000000/active", "",
      "application/json", admin_token);
  CHECK(resp.status == 404);
  CHECK(resp.json_body.contains("error"));
}

TEST_CASE("Non-admin user cannot set user inactive") {
  // Login as a non-admin user (Jim / 12345678 from seed data)
  auto user_token =
      test_helpers::login_only("127.0.0.1", 8848, "Jim", "12345678");
  // Get another user's UUID to attempt modifying
  auto adminId = test_helpers::get_userid("Admin");
  // This user should NOT have access to the admin endpoint
  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/admin/users/" + adminId + "/inactive", "",
      "application/json", user_token);
  // Should return 403 Forbidden as per AdminAuthFilter
  CHECK(resp.status == 403);
  CHECK(resp.json_body.contains("error"));
}

TEST_CASE("Inactive user cannot set user active") {
  // Login as the inactive user from seed data
  auto inactive_token =
      test_helpers::login_only("127.0.0.1", 8848, "InactiveUser", "12345678");
  // Get Admin's UUID to attempt modifying
  auto adminId = test_helpers::get_userid("Admin");

  // This inactive user should NOT have access to the admin endpoint
  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/admin/users/" + adminId + "/active", "",
      "application/json", inactive_token);
  // Should return 423 Locked due to AdminAuthFilter checking is_active
  CHECK(resp.status == 423);
  CHECK(resp.json_body.contains("error"));
  CHECK(resp.json_body["error"] == "User account is not active");
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
  std::vector<std::string> expected_keys = {
      "id",          "text",          "language",
      "category_id", "category_name", "special_category"};

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

TEST_CASE("RestSearchQuestions is accessible without authentication") {
  nlohmann::json request_body;
  request_body["search"] = "bananas";

  // No bearer token — the request must still succeed because
  // /questions/restSearch is a public endpoint.
  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", request_body.dump(),
      "application/json");
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  CHECK(resp.json_body.size() >= 1);
}

// ---------------------------------------------------------------------------
// POST /questions/{id}/answer  (anonymous, one-answer-per-user enforcement)
//
// The backend inserts an anonymous hash of the user id (never the raw id)
// together with the question_id into the `question_user` table, whose unique
// primary key enforces that a user may answer a question only once.
//
// Tags are NOT accepted in the request body: they are derived server-side
// from the authenticated user's profile in the users table (see the dedicated
// test case below). The answer_option id ranges below come from the seed data
// (003_seed_data.sql):
//   question 1 -> answer ids  1..5
//   question 2 -> answer ids  6..10
//   question 3 -> answer ids 11..13
//   question 4 -> answer ids 14..16
//   question 5 -> answer ids 17..21
//   question 6 -> answer ids 22..26
// ---------------------------------------------------------------------------

TEST_CASE("AnswerQuestion creates a new answer (201 Created)") {
  // answer_id = 6 belongs to question 2. No tags are sent: they are derived
  // from the user profile by the backend.
  nlohmann::json body;
  body["answer_id"] = 6;

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
  // Tags are not part of the request contract anymore; an empty object is
  // enough to trigger the missing-answer_id validation error.
  nlohmann::json body = nlohmann::json::object();

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

TEST_CASE(
    "AnswerQuestion derives tags from the user profile and buckets "
    "birth_year") {
  // Tags are no longer accepted in the request body; they are derived from
  // the users-table row of the authenticated user. The Admin seed user has
  // birth_year=1985 and gender='w' (004_test_data.sql). The expected age
  // bucket is computed at runtime so the test stays valid as time passes.
  std::time_t now_time = std::time(nullptr);
  std::tm tm_buf{};
  localtime_r(&now_time, &tm_buf);
  const int current_year = tm_buf.tm_year + 1900;
  const int birth_year = 1985;
  const int age = current_year - birth_year;
  // Bucket width must mirror `age_bucket_size` from config.json (default 10).
  const int bucket_size = 10;
  const int bucket_start = (age / bucket_size) * bucket_size;
  const std::string expected_bucket =
      fmt::format("{}-{}", bucket_start, bucket_start + bucket_size - 1);

  // Question 7 ("Do you recycle regularly?") is not used by any other answer
  // test; answer_id=27 is its first option ("Always").
  nlohmann::json body;
  body["answer_id"] = 27;

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/7/answer", body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 201);
  CHECK(resp.json_body["question_id"] == 7);
  CHECK(resp.json_body["answer_id"] == 27);

  // A birth_year query parameter is simply ignored (it is not a filterable
  // tag): the request behaves like an unfiltered query, proving the raw birth
  // year is neither stored nor filterable.
  auto resp_raw_year = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/7/stats?birth_year=1985", "",
      "application/json", global_fixture.access_token);
  CHECK(resp_raw_year.status == 200);
  CHECK(resp_raw_year.json_body["status"] == "ok");
  CHECK(resp_raw_year.json_body["answers"].size() == 1);

  // The bucketed age range derived from the profile must be stored instead.
  auto resp_age = test_helpers::http_request(
      "GET", "127.0.0.1", 8848,
      "/questions/7/stats?age_bucket=" + expected_bucket, "",
      "application/json", global_fixture.access_token);
  CHECK(resp_age.status == 200);
  CHECK(resp_age.json_body["status"] == "ok");
  CHECK(resp_age.json_body["answers"].is_array());
  CHECK(resp_age.json_body["answers"].size() == 1);
  CHECK(resp_age.json_body["answers"][0]["answer_id"] == 27);
  CHECK(resp_age.json_body["answers"][0]["count"] == 1);

  // The gender tag must be derived from the profile as well ('w' for Admin).
  auto resp_gender = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/7/stats?gender=w", "",
      "application/json", global_fixture.access_token);
  CHECK(resp_gender.status == 200);
  CHECK(resp_gender.json_body["status"] == "ok");
  CHECK(resp_gender.json_body["answers"].is_array());
  CHECK(resp_gender.json_body["answers"].size() == 1);
  CHECK(resp_gender.json_body["answers"][0]["answer_id"] == 27);
  CHECK(resp_gender.json_body["answers"][0]["count"] == 1);
}

// ---------------------------------------------------------------------------
// POST /questions/{id}/answer — consent flow for special-category questions
//
// Question 8 is flagged with special_category = 'health' in the test data, so
// answering it requires the optional boolean request parameter
// special_category_consent. For regular questions the parameter is ignored.
// ---------------------------------------------------------------------------

TEST_CASE("AnswerQuestion rejects special-category question without consent") {
  // Without the consent parameter.
  nlohmann::json body;
  body["answer_id"] = 32;  // belongs to question 8

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/8/answer", body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 400);
  CHECK(resp.json_body.contains("error"));

  // An explicit false consent flag is equally insufficient.
  nlohmann::json declined;
  declined["answer_id"] = 32;
  declined["special_category_consent"] = false;
  auto resp_declined = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/8/answer", declined.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp_declined.status == 400);
  CHECK(resp_declined.json_body.contains("error"));

  // A non-boolean value is rejected as malformed input.
  nlohmann::json malformed;
  malformed["answer_id"] = 32;
  malformed["special_category_consent"] = "yes";
  auto resp_malformed = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/8/answer", malformed.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp_malformed.status == 400);
  CHECK(resp_malformed.json_body.contains("error"));
}

TEST_CASE("AnswerQuestion accepts special-category question with consent") {
  // With explicit consent the answer succeeds. The refusals above do not
  // consume the per-user answer slot, so this must still be a fresh insert.
  nlohmann::json body;
  body["answer_id"] = 32;  // belongs to question 8
  body["special_category_consent"] = true;

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/8/answer", body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 201);
  CHECK(resp.json_body["question_id"] == 8);
  CHECK(resp.json_body["answer_id"] == 32);
}

TEST_CASE("AnswerQuestion ignores consent parameter for regular questions") {
  // Question 9 is not flagged with a special category: the consent parameter
  // is accepted but ignored entirely (no error, no consent recording).
  nlohmann::json body;
  body["answer_id"] = 37;  // belongs to question 9
  body["special_category_consent"] = true;

  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/9/answer", body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 201);
  CHECK(resp.json_body["question_id"] == 9);
  CHECK(resp.json_body["answer_id"] == 37);
}

// ---------------------------------------------------------------------------
// GET /questions/{id} — surfaces the special_category column so clients can
// ask for consent before answering a flagged question.
// ---------------------------------------------------------------------------

TEST_CASE("GetOne returns the special_category of a question") {
  // Question 8 is flagged with 'health' in the seed data; regular questions
  // carry the default 'none'.
  auto resp_special = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/8", "", "application/json",
      global_fixture.access_token);
  CHECK(resp_special.status == 200);
  CHECK(resp_special.json_body["special_category"] == "health");
  CHECK(resp_special.json_body["text"] ==
        "Was ist Ihr hauptsächliches Verkehrsmittel?");

  auto resp_regular = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/1", "", "application/json",
      global_fixture.access_token);
  CHECK(resp_regular.status == 200);
  CHECK(resp_regular.json_body["special_category"] == "none");
}

TEST_CASE("RestSearchQuestions returns the special_category of questions") {
  // The client list endpoint must expose the flag as well, so apps can ask
  // for consent before answering a flagged question. An explicit high limit
  // is required: without it the default page size of 50 (ordered by
  // created_at DESC) only covers the newest seeded questions, which do not
  // include ids 1 and 8.
  nlohmann::json request_body;
  request_body["limit"] = 1000;
  auto resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/restSearch", request_body.dump(),
      "application/json", global_fixture.access_token);
  CHECK(resp.status == 200);
  REQUIRE(resp.json_body.is_array());

  bool found_special = false;
  bool found_regular = false;
  for (const auto& q : resp.json_body) {
    if (q["id"] == 8) {
      found_special = true;
      CHECK(q["special_category"] == "health");
    }
    if (q["id"] == 1) {
      found_regular = true;
      CHECK(q["special_category"] == "none");
    }
  }
  CHECK(found_special);
  CHECK(found_regular);
}

// ---------------------------------------------------------------------------
// GET /categories  (category language column added, mirroring questions)
// ---------------------------------------------------------------------------

TEST_CASE(
    "GetCategoriesByLanguage surfaces the language field as a 2-char ISO "
    "code") {
  // The categories table carries a `language` column (NOT NULL, FK to
  // languages), mirroring how `questions.language` works. The dedicated
  // /categories/lang/{lang} route must surface it for each returned category.
  auto resp = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/categories/lang/en", "", "application/json",
      global_fixture.access_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  // 10 English seed categories from 003_seed_data.sql.
  CHECK(resp.json_body.size() == 10);

  std::vector<std::string> expected_keys = {"id", "name", "language"};
  for (size_t i = 0; i < resp.json_body.size(); ++i) {
    test_helpers::check_json_has_keys(resp.json_body[i], expected_keys,
                                      "category[" + std::to_string(i) + "]");
    // language must be a 2-char ISO code, e.g. "en".
    CHECK(resp.json_body[i]["language"].is_string());
    CHECK(resp.json_body[i]["language"].get<std::string>() == "en");
    CHECK(resp.json_body[i]["language"].get<std::string>().size() == 2);
  }
}

TEST_CASE("BareCategoryEndpointsAreRemoved") {
  // The generated RestfulCategoriesCtrl (GET/POST /categories and
  // GET/PUT/DELETE /categories/{id}) was removed for security, so the bare
  // endpoints must no longer be registered and return 404.
  auto get_resp = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/categories", "", "application/json",
      global_fixture.access_token);
  CHECK(get_resp.status == 404);

  nlohmann::json body;
  body["name"] = "ShouldNotBeCreated";
  body["language"] = "en";
  auto post_resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/categories", body.dump(), "application/json",
      global_fixture.access_token);
  CHECK(post_resp.status == 404);
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

TEST_CASE("GetCategoriesByLanguage is accessible without authentication") {
  auto resp = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/categories/lang/en", "", "application/json");
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  CHECK(resp.json_body.size() == 10);
}

// ---------------------------------------------------------------------------
// PATCH /me  (and PUT /me) – update the authenticated user's own profile
//
// Only email, gender and password are modifiable. The username is the user's
// identity (from the JWT) and must never be changed.
// ---------------------------------------------------------------------------

TEST_CASE("UpdateMe changes email") {
  // Use a dedicated user so we don't disturb the global fixture user.
  std::string uname = "updateme_email";
  std::string orig_email = "updateme_email@example.com";
  std::string new_email = "updateme_email_new@example.com";
  auto token = test_helpers::authenticate("127.0.0.1", 8848, uname, orig_email,
                                          "password123", 1990, "m", "US");

  nlohmann::json body = {{"email", new_email}};
  auto resp =
      test_helpers::http_request("PATCH", "127.0.0.1", 8848, "/me", body.dump(),
                                 "application/json", token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.contains("username"));
  CHECK(resp.json_body["username"] == uname);
  CHECK(resp.json_body.contains("email"));
  CHECK(resp.json_body["email"] == new_email);
  // The password hash must never be returned in the response.
  CHECK_FALSE(resp.json_body.contains("password_hash"));
}

TEST_CASE("UpdateMe changes gender") {
  auto token = test_helpers::authenticate("127.0.0.1", 8848, "updateme_gender",
                                          "updateme_gender@example.com",
                                          "password123", 1990, "m", "US");

  nlohmann::json body = {{"gender", "w"}};
  auto resp =
      test_helpers::http_request("PATCH", "127.0.0.1", 8848, "/me", body.dump(),
                                 "application/json", token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.contains("gender"));
  CHECK(resp.json_body["gender"] == "w");
}

TEST_CASE("UpdateMe changes password and invalidates the old one") {
  // Use a unique username per run so the test stays isolated against a
  // persistent database: changing the password would otherwise break re-runs
  // that try to log in again with the original password.
  std::string uname = "updateme_pw_" + std::to_string(std::time(nullptr));
  std::string email = uname + "@example.com";
  std::string old_pw = "password123";
  std::string new_pw = "newpassword456";
  auto token = test_helpers::authenticate("127.0.0.1", 8848, uname, email,
                                          old_pw, 1990, "m", "US");

  // Change the password.
  nlohmann::json body = {{"password", new_pw}};
  auto resp =
      test_helpers::http_request("PATCH", "127.0.0.1", 8848, "/me", body.dump(),
                                 "application/json", token);
  CHECK(resp.status == 200);

  // Login with the NEW password now succeeds.
  auto login_new = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/login",
      nlohmann::json{{"username", uname}, {"password", new_pw}}.dump());
  CHECK(login_new.status == 200);

  // Login with the OLD password is rejected.
  auto login_old = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/login",
      nlohmann::json{{"username", uname}, {"password", old_pw}}.dump());
  CHECK(login_old.status == 401);
}

TEST_CASE("UpdateMe with PUT also updates the profile") {
  auto token = test_helpers::authenticate("127.0.0.1", 8848, "updateme_put",
                                          "updateme_put@example.com",
                                          "password123", 1990, "m", "US");

  nlohmann::json body = {{"email", "updateme_put2@example.com"}};
  auto resp = test_helpers::http_request(
      "PUT", "127.0.0.1", 8848, "/me", body.dump(), "application/json", token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body["email"] == "updateme_put2@example.com");
}

TEST_CASE("UpdateMe rejects username modification") {
  auto token = test_helpers::authenticate("127.0.0.1", 8848, "updateme_uname",
                                          "updateme_uname@example.com",
                                          "password123", 1990, "m", "US");

  nlohmann::json body = {{"username", "hacked"}};
  auto resp =
      test_helpers::http_request("PATCH", "127.0.0.1", 8848, "/me", body.dump(),
                                 "application/json", token);
  CHECK(resp.status == 400);
  CHECK(resp.json_body.contains("error"));
}

TEST_CASE("UpdateMe rejects invalid gender") {
  auto token = test_helpers::authenticate(
      "127.0.0.1", 8848, "updateme_badgender", "updateme_badgender@example.com",
      "password123", 1990, "m", "US");

  nlohmann::json body = {{"gender", "x"}};
  auto resp =
      test_helpers::http_request("PATCH", "127.0.0.1", 8848, "/me", body.dump(),
                                 "application/json", token);
  CHECK(resp.status == 400);
  CHECK(resp.json_body.contains("error"));
}

TEST_CASE("UpdateMe rejects email already in use") {
  // The seeded test user "Jim" owns "jim@example.com" (see
  // sql/004_test_data.sql), so reusing that email must be rejected with 409.
  auto token = test_helpers::authenticate("127.0.0.1", 8848, "updateme_dup",
                                          "updateme_dup@example.com",
                                          "password123", 1990, "m", "US");

  nlohmann::json body = {{"email", "jim@example.com"}};
  auto resp =
      test_helpers::http_request("PATCH", "127.0.0.1", 8848, "/me", body.dump(),
                                 "application/json", token);
  CHECK(resp.status == 409);
  CHECK(resp.json_body.contains("error"));
}

TEST_CASE("UpdateMe requires authentication") {
  nlohmann::json body = {{"email", "nobody@example.com"}};
  auto resp = test_helpers::http_request("PATCH", "127.0.0.1", 8848, "/me",
                                         body.dump(), "application/json", "");
  CHECK(resp.status == 401);
}

// ---------------------------------------------------------------------------
// Submission / approval workflow (Option B)
//
// A regular user submits a question, which is stored as 'pending' and is NOT
// visible publicly. Only the submitter (via /questions/mine) and an admin (via
// the review queue) can see it. An admin approves it -> it becomes visible; an
// admin rejects it -> it stays hidden. A non-admin cannot approve.
// ---------------------------------------------------------------------------

TEST_CASE("SubmissionWorkflow submit pending approve reject") {
  auto user_token =
      test_helpers::login_only("127.0.0.1", 8848, "Jim", "12345678");
  auto admin_token =
      test_helpers::login_only("127.0.0.1", 8848, "Admin", "12345678");

  // 1. User submits a new question together with its answer options -> 201,
  // stored as 'pending', and the answer options are returned with it.
  nlohmann::json body = {{"text", "Should voting be mandatory?"},
                         {"category_id", 1},
                         {"language", "en"},
                         {"answer_options", {"Yes", "No", "Abstain"}}};
  auto create = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/submissions", body.dump(),
      "application/json", user_token);
  CHECK(create.status == 201);
  CHECK(create.json_body.contains("id"));
  CHECK(create.json_body["submission_status"] == "pending");
  // The submission must echo back its answer options (id, question_id, text).
  CHECK(create.json_body.contains("answer_options"));
  CHECK(create.json_body["answer_options"].is_array());
  CHECK(create.json_body["answer_options"].size() == 3);
  for (size_t i = 0; i < create.json_body["answer_options"].size(); ++i) {
    CHECK(create.json_body["answer_options"][i].contains("id"));
    CHECK(create.json_body["answer_options"][i].contains("question_id"));
    CHECK(create.json_body["answer_options"][i].contains("text"));
    CHECK(create.json_body["answer_options"][i]["question_id"] ==
          create.json_body["id"]);
  }
  int new_id = create.json_body["id"].get<int>();

  // 2. Not visible via the public single-question endpoint (404).
  auto get_one = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/" + std::to_string(new_id), "",
      "application/json", user_token);
  CHECK(get_one.status == 404);

  // 3. Not visible in the public list of questions.
  auto list = test_helpers::http_request("GET", "127.0.0.1", 8848, "/questions",
                                         "", "application/json", user_token);
  CHECK(list.status == 200);
  bool in_list = false;
  for (const auto& q : list.json_body) {
    if (q["id"].get<int>() == new_id) in_list = true;
  }
  CHECK_FALSE(in_list);

  // 4. Visible to the submitter via /questions/mine.
  auto mine =
      test_helpers::http_request("GET", "127.0.0.1", 8848, "/questions/mine",
                                 "", "application/json", user_token);
  CHECK(mine.status == 200);
  bool in_mine = false;
  for (const auto& q : mine.json_body) {
    if (q["id"].get<int>() == new_id && q["submission_status"] == "pending") {
      in_mine = true;
    }
  }
  CHECK(in_mine);

  // 5. Visible to an admin via the review queue.
  auto queue = test_helpers::http_request("GET", "127.0.0.1", 8848,
                                          "/admin/questions/submissions", "",
                                          "application/json", admin_token);
  CHECK(queue.status == 200);
  bool in_queue = false;
  for (const auto& q : queue.json_body) {
    if (q["id"].get<int>() == new_id && q["submission_status"] == "pending") {
      in_queue = true;
    }
  }
  CHECK(in_queue);

  // 6. A regular user cannot approve (403 from AdminAuthFilter).
  auto bad_approve = test_helpers::http_request(
      "POST", "127.0.0.1", 8848,
      "/admin/questions/" + std::to_string(new_id) + "/approve", "",
      "application/json", user_token);
  CHECK(bad_approve.status == 403);

  // 7. Admin approves -> 200 and the question is now 'approved'.
  auto approve = test_helpers::http_request(
      "POST", "127.0.0.1", 8848,
      "/admin/questions/" + std::to_string(new_id) + "/approve", "",
      "application/json", admin_token);
  CHECK(approve.status == 200);
  CHECK(approve.json_body["submission_status"] == "approved");

  // 8. Now publicly visible.
  auto get_one2 = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/" + std::to_string(new_id), "",
      "application/json", user_token);
  CHECK(get_one2.status == 200);

  // 9. Reject flow: submit another (with answer options), admin rejects,
  // stays hidden.
  nlohmann::json body2 = {{"text", "Should pets be allowed to vote?"},
                          {"category_id", 1},
                          {"language", "en"},
                          {"answer_options", {"Yes", "No"}}};
  auto create2 = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/submissions", body2.dump(),
      "application/json", user_token);
  CHECK(create2.status == 201);
  CHECK(create2.json_body.contains("answer_options"));
  CHECK(create2.json_body["answer_options"].size() == 2);
  int new_id2 = create2.json_body["id"].get<int>();

  auto reject = test_helpers::http_request(
      "POST", "127.0.0.1", 8848,
      "/admin/questions/" + std::to_string(new_id2) + "/reject", "",
      "application/json", admin_token);
  CHECK(reject.status == 200);
  CHECK(reject.json_body["submission_status"] == "rejected");

  auto get_rejected = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/" + std::to_string(new_id2), "",
      "application/json", user_token);
  CHECK(get_rejected.status == 404);
}

TEST_CASE("Submission rejects requests containing min_age") {
  auto user_token =
      test_helpers::login_only("127.0.0.1", 8848, "Jim", "12345678");

  nlohmann::json body = {{"text", "Legacy submission with min_age?"},
                         {"category_id", 1},
                         {"language", "en"},
                         {"min_age", 18},
                         {"answer_options", {"Yes", "No"}}};
  auto resp = test_helpers::http_request("POST", "127.0.0.1", 8848,
                                         "/questions/submissions", body.dump(),
                                         "application/json", user_token);
  CHECK(resp.status == 400);
  CHECK(resp.json_body.contains("error"));
}

TEST_CASE("ApproveQuestion applies min_age and special_category") {
  auto user_token =
      test_helpers::login_only("127.0.0.1", 8848, "Jim", "12345678");
  auto admin_token =
      test_helpers::login_only("127.0.0.1", 8848, "Admin", "12345678");

  nlohmann::json body = {{"text", "How often do you visit a doctor?"},
                         {"category_id", 1},
                         {"language", "en"},
                         {"answer_options", {"Never", "Sometimes"}}};
  auto create = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/submissions", body.dump(),
      "application/json", user_token);
  CHECK(create.status == 201);
  // Submitted without min_age: the defaults apply until approval.
  CHECK(create.json_body["min_age"] == 0);
  CHECK(create.json_body["special_category"] == "none");
  int new_id = create.json_body["id"].get<int>();

  // Invalid special_category label -> 400, submission stays pending.
  nlohmann::json bad_category = {{"min_age", 18},
                                 {"special_category", "not_a_category"}};
  auto bad_cat_resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848,
      "/admin/questions/" + std::to_string(new_id) + "/approve",
      bad_category.dump(), "application/json", admin_token);
  CHECK(bad_cat_resp.status == 400);

  // Invalid min_age values -> 400 as well.
  nlohmann::json negative_age = {{"min_age", -1}};
  auto negative_resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848,
      "/admin/questions/" + std::to_string(new_id) + "/approve",
      negative_age.dump(), "application/json", admin_token);
  CHECK(negative_resp.status == 400);

  nlohmann::json string_age = {{"min_age", "eighteen"}};
  auto string_age_resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848,
      "/admin/questions/" + std::to_string(new_id) + "/approve",
      string_age.dump(), "application/json", admin_token);
  CHECK(string_age_resp.status == 400);

  // Valid body -> approved with the given values, echoed in the response
  // and visible on the public single-question endpoint.
  nlohmann::json approve_body = {{"min_age", 18},
                                 {"special_category", "health"}};
  auto approve = test_helpers::http_request(
      "POST", "127.0.0.1", 8848,
      "/admin/questions/" + std::to_string(new_id) + "/approve",
      approve_body.dump(), "application/json", admin_token);
  CHECK(approve.status == 200);
  CHECK(approve.json_body["submission_status"] == "approved");
  CHECK(approve.json_body["min_age"] == 18);
  CHECK(approve.json_body["special_category"] == "health");

  auto get_one = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/" + std::to_string(new_id), "",
      "application/json", user_token);
  CHECK(get_one.status == 200);
  CHECK(get_one.json_body["min_age"] == 18);
  CHECK(get_one.json_body["special_category"] == "health");

  // Approval without a body remains valid and keeps the defaults.
  nlohmann::json body2 = {{"text", "Plain question without special data?"},
                          {"category_id", 1},
                          {"language", "en"},
                          {"answer_options", {"Yes", "No"}}};
  auto create2 = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/submissions", body2.dump(),
      "application/json", user_token);
  CHECK(create2.status == 201);
  int new_id2 = create2.json_body["id"].get<int>();

  auto plain_approve = test_helpers::http_request(
      "POST", "127.0.0.1", 8848,
      "/admin/questions/" + std::to_string(new_id2) + "/approve", "",
      "application/json", admin_token);
  CHECK(plain_approve.status == 200);
  CHECK(plain_approve.json_body["min_age"] == 0);
  CHECK(plain_approve.json_body["special_category"] == "none");
}

TEST_CASE(
    "SubmissionWorkflow pending question answer options hidden from others") {
  auto user_token =
      test_helpers::login_only("127.0.0.1", 8848, "Jim", "12345678");
  // A different regular user (not the submitter).
  auto other_token = test_helpers::authenticate(
      "127.0.0.1", 8848, "submission_other", "submission_other@example.com",
      "password123", 1990, "m", "US");

  nlohmann::json body = {{"text", "Is pineapple on pizza acceptable?"},
                         {"category_id", 1},
                         {"language", "en"},
                         {"answer_options", {"Yes", "No", "It depends"}}};
  auto create = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/submissions", body.dump(),
      "application/json", user_token);
  CHECK(create.status == 201);
  CHECK(create.json_body["answer_options"].size() == 3);
  int new_id = create.json_body["id"].get<int>();

  // Owner can fetch the answer options for their pending question.
  auto owner_opts = test_helpers::http_request(
      "GET", "127.0.0.1", 8848,
      "/questions/" + std::to_string(new_id) + "/answers-with-auth", "",
      "application/json", user_token);
  CHECK(owner_opts.status == 200);
  CHECK(owner_opts.json_body.is_array());
  CHECK(owner_opts.json_body.size() == 3);
  CHECK(owner_opts.json_body[0]["text"] == "Yes");

  // A non-owner must not see them (404 -> pending content stays hidden).
  auto other_opts = test_helpers::http_request(
      "GET", "127.0.0.1", 8848,
      "/questions/" + std::to_string(new_id) + "/answers-with-auth", "",
      "application/json", other_token);
  CHECK(other_opts.status == 404);
}

TEST_CASE("SubmissionWorkflow requires answer_options") {
  auto user_token =
      test_helpers::login_only("127.0.0.1", 8848, "Jim", "12345678");

  // 1. Missing answer_options -> 400.
  nlohmann::json missing = {{"text", "What is your favorite color?"},
                            {"category_id", 1},
                            {"language", "en"}};
  auto r1 = test_helpers::http_request("POST", "127.0.0.1", 8848,
                                       "/questions/submissions", missing.dump(),
                                       "application/json", user_token);
  CHECK(r1.status == 400);
  CHECK(r1.json_body.contains("error"));

  // 2. Empty answer_options array -> 400.
  nlohmann::json empty = {{"text", "What is your favorite color?"},
                          {"category_id", 1},
                          {"language", "en"},
                          {"answer_options", nlohmann::json::array()}};
  auto r2 = test_helpers::http_request("POST", "127.0.0.1", 8848,
                                       "/questions/submissions", empty.dump(),
                                       "application/json", user_token);
  CHECK(r2.status == 400);
  CHECK(r2.json_body.contains("error"));

  // 3. Answer option without a non-empty text -> 400.
  nlohmann::json bad = {{"text", "What is your favorite color?"},
                        {"category_id", 1},
                        {"language", "en"},
                        {"answer_options", {""}}};
  auto r3 = test_helpers::http_request("POST", "127.0.0.1", 8848,
                                       "/questions/submissions", bad.dump(),
                                       "application/json", user_token);
  CHECK(r3.status == 400);
  CHECK(r3.json_body.contains("error"));
}

TEST_CASE(
    "SubmissionWorkflow answer options persisted and visible after approval") {
  auto user_token =
      test_helpers::login_only("127.0.0.1", 8848, "Jim", "12345678");
  auto admin_token =
      test_helpers::login_only("127.0.0.1", 8848, "Admin", "12345678");

  nlohmann::json body = {{"text", "How often do you drink coffee?"},
                         {"category_id", 1},
                         {"language", "en"},
                         {"answer_options", {"Never", "Daily"}}};
  auto create = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/submissions", body.dump(),
      "application/json", user_token);
  CHECK(create.status == 201);
  int new_id = create.json_body["id"].get<int>();

  // Before approval the owner can still see the answer options.
  auto owner_opts = test_helpers::http_request(
      "GET", "127.0.0.1", 8848,
      "/questions/" + std::to_string(new_id) + "/answers-with-auth", "",
      "application/json", user_token);
  CHECK(owner_opts.status == 200);
  CHECK(owner_opts.json_body.is_array());

  // Admin approves -> answer options become publicly visible.
  auto approve = test_helpers::http_request(
      "POST", "127.0.0.1", 8848,
      "/admin/questions/" + std::to_string(new_id) + "/approve", "",
      "application/json", admin_token);
  CHECK(approve.status == 200);

  auto visible = test_helpers::http_request(
      "GET", "127.0.0.1", 8848,
      "/questions/" + std::to_string(new_id) + "/answers", "",
      "application/json", user_token);
  CHECK(visible.status == 200);
  CHECK(visible.json_body.is_array());
  CHECK(visible.json_body.size() == 2);
  std::set<std::string> texts;
  for (const auto& o : visible.json_body) {
    texts.insert(o["text"].get<std::string>());
    CHECK(o["question_id"] == new_id);
  }
  CHECK(texts.count("Never") == 1);
  CHECK(texts.count("Daily") == 1);
}

// ---------------------------------------------------------------------------
// GET /admin/questions/{id}/answers  (admin review-queue answer options)
//
// Administrators reviewing a submission need to see the submitted answer
// options even when the question is still pending and therefore hidden from the
// public /questions/{id}/answers endpoint. The admin endpoint sits behind
// AdminAuthFilter and returns the options for any existing question regardless
// of submission status; it 404s only when the question itself does not exist.
// ---------------------------------------------------------------------------

TEST_CASE("AdminGetAnswerOptions returns options for a pending submission") {
  auto user_token =
      test_helpers::login_only("127.0.0.1", 8848, "Jim", "12345678");
  auto admin_token =
      test_helpers::login_only("127.0.0.1", 8848, "Admin", "12345678");

  // A regular user submits a pending question together with its answer options.
  nlohmann::json body = {{"text", "Should public transport be free?"},
                         {"category_id", 1},
                         {"language", "en"},
                         {"answer_options", {"Yes", "No", "Depends"}}};
  auto create = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/submissions", body.dump(),
      "application/json", user_token);
  CHECK(create.status == 201);
  int new_id = create.json_body["id"].get<int>();

  // The admin can fetch the answer options of the pending submission through
  // the dedicated admin endpoint (which a non-owner cannot reach publicly).
  auto admin_opts = test_helpers::http_request(
      "GET", "127.0.0.1", 8848,
      "/admin/questions/" + std::to_string(new_id) + "/answers", "",
      "application/json", admin_token);
  CHECK(admin_opts.status == 200);
  CHECK(admin_opts.json_body.is_array());
  CHECK(admin_opts.json_body.size() == 3);

  // Each option carries id, question_id and text; question_id points back to
  // the submission and the texts match what was submitted.
  std::vector<std::string> expected_keys = {"id", "question_id", "text"};
  for (size_t i = 0; i < admin_opts.json_body.size(); ++i) {
    test_helpers::check_json_has_keys(admin_opts.json_body[i], expected_keys,
                                      "admin_opt[" + std::to_string(i) + "]");
    CHECK(admin_opts.json_body[i]["question_id"] == new_id);
  }
  std::set<std::string> texts;
  for (const auto& o : admin_opts.json_body) {
    texts.insert(o["text"].get<std::string>());
  }
  CHECK(texts.count("Yes") == 1);
  CHECK(texts.count("No") == 1);
  CHECK(texts.count("Depends") == 1);
}

TEST_CASE(
    "AdminGetAnswerOptions also returns options for an approved question") {
  auto admin_token =
      test_helpers::login_only("127.0.0.1", 8848, "Admin", "12345678");

  // Seed question 1 ("How many bananas do you eat per week?") is approved and
  // has 5 answer options; the admin endpoint must return them as well.
  auto resp = test_helpers::http_request("GET", "127.0.0.1", 8848,
                                         "/admin/questions/1/answers", "",
                                         "application/json", admin_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
  CHECK(resp.json_body.size() == 5);

  std::vector<std::string> expected_keys = {"id", "question_id", "text"};
  for (size_t i = 0; i < resp.json_body.size(); ++i) {
    test_helpers::check_json_has_keys(
        resp.json_body[i], expected_keys,
        "approved_opt[" + std::to_string(i) + "]");
    CHECK(resp.json_body[i]["question_id"] == 1);
  }
  std::vector<std::string> expected_texts = {"0", "1-2", "3-5", "6-10",
                                             "More than 10"};
  for (size_t i = 0; i < resp.json_body.size(); ++i) {
    CHECK(resp.json_body[i]["text"] == expected_texts[i]);
  }
}

TEST_CASE("AdminGetAnswerOptions rejects non-admin users with 403") {
  // A regular (non-admin) user must be blocked by AdminAuthFilter.
  auto other_token = test_helpers::authenticate(
      "127.0.0.1", 8848, "admin_opts_other", "admin_opts_other@example.com",
      "password123", 1990, "m", "US");

  auto resp = test_helpers::http_request("GET", "127.0.0.1", 8848,
                                         "/admin/questions/1/answers", "",
                                         "application/json", other_token);
  CHECK(resp.status == 403);
  CHECK(resp.json_body.contains("error"));
}

TEST_CASE("AdminGetAnswerOptions requires authentication (401)") {
  // No bearer token at all -> 401 from AdminAuthFilter.
  auto resp = test_helpers::http_request("GET", "127.0.0.1", 8848,
                                         "/admin/questions/1/answers");
  CHECK(resp.status == 401);
}

TEST_CASE("AdminGetAnswerOptions returns 404 for a non-existent question") {
  auto admin_token =
      test_helpers::login_only("127.0.0.1", 8848, "Admin", "12345678");

  // Even an admin cannot read options for a question that does not exist.
  auto resp = test_helpers::http_request("GET", "127.0.0.1", 8848,
                                         "/admin/questions/999999/answers", "",
                                         "application/json", admin_token);
  CHECK(resp.status == 404);
}

// ---------------------------------------------------------------------------
// POST /admin/questions/{id}/delete  (admin-only question deletion)
//
// The endpoint removes a question together with all of its dependent rows
// (answer options, user answers, anonymous answer tracking) via ON DELETE
// CASCADE. It returns the deleted question's id, text and submission_status
// with HTTP 200, 404 when the question does not exist, and is gated by
// AdminAuthFilter (401 for missing tokens, 403 for non-admins).
// ---------------------------------------------------------------------------

TEST_CASE("AdminDeleteQuestion removes a pending submission and its options") {
  auto user_token =
      test_helpers::login_only("127.0.0.1", 8848, "Jim", "12345678");
  auto admin_token =
      test_helpers::login_only("127.0.0.1", 8848, "Admin", "12345678");

  // Create a pending submission with answer options.
  nlohmann::json body = {{"text", "Should socks be worn with sandals?"},
                         {"category_id", 1},
                         {"language", "en"},
                         {"answer_options", {"Yes", "No", "Obviously"}}};
  auto create = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/submissions", body.dump(),
      "application/json", user_token);
  CHECK(create.status == 201);
  int new_id = create.json_body["id"].get<int>();

  // The question's answer options are visible to the owner before deletion.
  auto opts_before = test_helpers::http_request(
      "GET", "127.0.0.1", 8848,
      "/admin/questions/" + std::to_string(new_id) + "/answers", "",
      "application/json", admin_token);
  CHECK(opts_before.status == 200);
  CHECK(opts_before.json_body.size() == 3);

  // Admin deletes the question.
  auto del = test_helpers::http_request(
      "POST", "127.0.0.1", 8848,
      "/admin/questions/" + std::to_string(new_id) + "/delete", "",
      "application/json", admin_token);
  CHECK(del.status == 200);
  CHECK(del.json_body["id"] == new_id);
  CHECK(del.json_body["submission_status"] == "pending");
  CHECK(del.json_body["message"] == "Question deleted");

  // The question no longer appears in the public question list.
  auto list = test_helpers::http_request("GET", "127.0.0.1", 8848, "/questions",
                                         "", "application/json", admin_token);
  CHECK(list.status == 200);
  bool still_in_list = false;
  for (const auto& q : list.json_body) {
    if (q["id"].get<int>() == new_id) still_in_list = true;
  }
  CHECK_FALSE(still_in_list);

  // The question's answer options are gone (cascade delete).
  auto opts_after = test_helpers::http_request(
      "GET", "127.0.0.1", 8848,
      "/admin/questions/" + std::to_string(new_id) + "/answers", "",
      "application/json", admin_token);
  CHECK(opts_after.status == 404);
}

TEST_CASE(
    "AdminDeleteQuestion removes an approved submission and its options") {
  auto user_token =
      test_helpers::login_only("127.0.0.1", 8848, "Jim", "12345678");
  auto admin_token =
      test_helpers::login_only("127.0.0.1", 8848, "Admin", "12345678");

  // Create a pending submission with answer options.
  nlohmann::json body = {{"text", "Should robots be allowed to vote?"},
                         {"category_id", 1},
                         {"language", "en"},
                         {"answer_options", {"Yes", "No"}}};
  auto create = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/submissions", body.dump(),
      "application/json", user_token);
  CHECK(create.status == 201);
  int new_id = create.json_body["id"].get<int>();

  // Admin approves it so it is a fully "approved" question.
  auto approve = test_helpers::http_request(
      "POST", "127.0.0.1", 8848,
      "/admin/questions/" + std::to_string(new_id) + "/approve", "",
      "application/json", admin_token);
  CHECK(approve.status == 200);
  CHECK(approve.json_body["submission_status"] == "approved");

  // The approved question is publicly visible.
  auto get_one = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/" + std::to_string(new_id), "",
      "application/json", admin_token);
  CHECK(get_one.status == 200);

  // Admin deletes the approved question.
  auto del = test_helpers::http_request(
      "POST", "127.0.0.1", 8848,
      "/admin/questions/" + std::to_string(new_id) + "/delete", "",
      "application/json", admin_token);
  CHECK(del.status == 200);
  CHECK(del.json_body["id"] == new_id);
  CHECK(del.json_body["submission_status"] == "approved");
  CHECK(del.json_body["message"] == "Question deleted");

  // The question is no longer publicly visible.
  auto get_after = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/" + std::to_string(new_id), "",
      "application/json", admin_token);
  CHECK(get_after.status == 404);

  // Its answer options are gone too (cascade delete).
  auto opts = test_helpers::http_request(
      "GET", "127.0.0.1", 8848,
      "/admin/questions/" + std::to_string(new_id) + "/answers", "",
      "application/json", admin_token);
  CHECK(opts.status == 404);
}

TEST_CASE("AdminDeleteQuestion for non-existent question returns 404") {
  auto admin_token =
      test_helpers::login_only("127.0.0.1", 8848, "Admin", "12345678");

  auto resp = test_helpers::http_request("POST", "127.0.0.1", 8848,
                                         "/admin/questions/999999/delete", "",
                                         "application/json", admin_token);
  CHECK(resp.status == 404);
  CHECK(resp.json_body.contains("error"));
}

TEST_CASE("AdminDeleteQuestion rejects non-admin users with 403") {
  auto user_token =
      test_helpers::login_only("127.0.0.1", 8848, "Jim", "12345678");

  auto resp = test_helpers::http_request("POST", "127.0.0.1", 8848,
                                         "/admin/questions/1/delete", "",
                                         "application/json", user_token);
  CHECK(resp.status == 403);
  CHECK(resp.json_body.contains("error"));
}

TEST_CASE("AdminDeleteQuestion requires authentication (401)") {
  auto resp = test_helpers::http_request("POST", "127.0.0.1", 8848,
                                         "/admin/questions/1/delete");
  CHECK(resp.status == 401);
}

// ---------------------------------------------------------------------------
// PATCH /admin/questions/{id}/change  (admin-only text update)
//
// Changes the `text` column of an existing question. The request body must be
// a JSON object containing a non-empty "text" field. Protected by
// AdminAuthFilter (401 / 403), returns 404 for unknown questions and 400 for
// invalid or missing bodies. Answer options and other dependent rows are left
// untouched.
// ---------------------------------------------------------------------------

TEST_CASE("AdminChangeQuestionText updates text of a pending submission") {
  auto user_token =
      test_helpers::login_only("127.0.0.1", 8848, "Jim", "12345678");
  auto admin_token =
      test_helpers::login_only("127.0.0.1", 8848, "Admin", "12345678");

  // Create a pending submission.
  nlohmann::json body = {{"text", "Original text for change test"},
                         {"category_id", 1},
                         {"language", "en"},
                         {"answer_options", {"A", "B"}}};
  auto create = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/submissions", body.dump(),
      "application/json", user_token);
  CHECK(create.status == 201);
  int new_id = create.json_body["id"].get<int>();

  // Admin changes the question text.
  nlohmann::json patch = {{"text", "Updated text for change test"}};
  auto resp = test_helpers::http_request(
      "PATCH", "127.0.0.1", 8848,
      "/admin/questions/" + std::to_string(new_id) + "/change", patch.dump(),
      "application/json", admin_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body["id"] == new_id);
  CHECK(resp.json_body["text"] == "Updated text for change test");
  CHECK(resp.json_body["submission_status"] == "pending");
  CHECK(resp.json_body["message"] == "Question text updated");

  // The owner can read the question back with the new text.
  auto get_one = test_helpers::http_request(
      "GET", "127.0.0.1", 8848,
      "/questions/" + std::to_string(new_id) + "/answers-with-auth", "",
      "application/json", user_token);
  CHECK(get_one.status == 200);
}

TEST_CASE("AdminChangeQuestionText updates text of an approved question") {
  auto user_token =
      test_helpers::login_only("127.0.0.1", 8848, "Jim", "12345678");
  auto admin_token =
      test_helpers::login_only("127.0.0.1", 8848, "Admin", "12345678");

  // Create a pending submission and approve it.
  nlohmann::json body = {{"text", "Before approval text"},
                         {"category_id", 1},
                         {"language", "en"},
                         {"answer_options", {"X", "Y"}}};
  auto create = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/submissions", body.dump(),
      "application/json", user_token);
  CHECK(create.status == 201);
  int new_id = create.json_body["id"].get<int>();

  auto approve = test_helpers::http_request(
      "POST", "127.0.0.1", 8848,
      "/admin/questions/" + std::to_string(new_id) + "/approve", "",
      "application/json", admin_token);
  CHECK(approve.status == 200);
  CHECK(approve.json_body["submission_status"] == "approved");

  // Now change the approved question's text.
  nlohmann::json patch = {{"text", "After approval text"}};
  auto resp = test_helpers::http_request(
      "PATCH", "127.0.0.1", 8848,
      "/admin/questions/" + std::to_string(new_id) + "/change", patch.dump(),
      "application/json", admin_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body["id"] == new_id);
  CHECK(resp.json_body["text"] == "After approval text");
  CHECK(resp.json_body["submission_status"] == "approved");

  // The question is still publicly visible (status unchanged).
  auto get_one = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/questions/" + std::to_string(new_id), "",
      "application/json", admin_token);
  CHECK(get_one.status == 200);
  CHECK(get_one.json_body["text"] == "After approval text");
}

TEST_CASE(
    "AdminChangeQuestionText leaves answer options intact after text change") {
  auto user_token =
      test_helpers::login_only("127.0.0.1", 8848, "Jim", "12345678");
  auto admin_token =
      test_helpers::login_only("127.0.0.1", 8848, "Admin", "12345678");

  // Create a pending submission with answer options.
  nlohmann::json body = {
      {"text", "Original question text"},
      {"category_id", 1},
      {"language", "en"},
      {"answer_options", {"Option A", "Option B", "Option C"}}};
  auto create = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/questions/submissions", body.dump(),
      "application/json", user_token);
  CHECK(create.status == 201);
  int new_id = create.json_body["id"].get<int>();

  // Capture the original answer options.
  auto opts_before = test_helpers::http_request(
      "GET", "127.0.0.1", 8848,
      "/admin/questions/" + std::to_string(new_id) + "/answers", "",
      "application/json", admin_token);
  CHECK(opts_before.status == 200);
  CHECK(opts_before.json_body.size() == 3);

  // Change the question text.
  nlohmann::json patch = {{"text", "Changed question text"}};
  auto resp = test_helpers::http_request(
      "PATCH", "127.0.0.1", 8848,
      "/admin/questions/" + std::to_string(new_id) + "/change", patch.dump(),
      "application/json", admin_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body["text"] == "Changed question text");

  // Answer options should be unchanged.
  auto opts_after = test_helpers::http_request(
      "GET", "127.0.0.1", 8848,
      "/admin/questions/" + std::to_string(new_id) + "/answers", "",
      "application/json", admin_token);
  CHECK(opts_after.status == 200);
  CHECK(opts_after.json_body.size() == 3);
  std::set<std::string> texts;
  for (const auto& o : opts_after.json_body) {
    texts.insert(o["text"].get<std::string>());
  }
  CHECK(texts.count("Option A") == 1);
  CHECK(texts.count("Option B") == 1);
  CHECK(texts.count("Option C") == 1);
}

TEST_CASE("AdminChangeQuestionText for non-existent question returns 404") {
  auto admin_token =
      test_helpers::login_only("127.0.0.1", 8848, "Admin", "12345678");

  nlohmann::json patch = {{"text", "Does not matter"}};
  auto resp = test_helpers::http_request(
      "PATCH", "127.0.0.1", 8848, "/admin/questions/999999/change",
      patch.dump(), "application/json", admin_token);
  CHECK(resp.status == 404);
  CHECK(resp.json_body.contains("error"));
}

TEST_CASE("AdminChangeQuestionText with missing text field returns 400") {
  auto admin_token =
      test_helpers::login_only("127.0.0.1", 8848, "Admin", "12345678");

  nlohmann::json patch = {{"category_id", 1}};
  auto resp = test_helpers::http_request(
      "PATCH", "127.0.0.1", 8848, "/admin/questions/1/change", patch.dump(),
      "application/json", admin_token);
  CHECK(resp.status == 400);
  CHECK(resp.json_body.contains("error"));
}

TEST_CASE("AdminChangeQuestionText with empty text returns 400") {
  auto admin_token =
      test_helpers::login_only("127.0.0.1", 8848, "Admin", "12345678");

  nlohmann::json patch = {{"text", ""}};
  auto resp = test_helpers::http_request(
      "PATCH", "127.0.0.1", 8848, "/admin/questions/1/change", patch.dump(),
      "application/json", admin_token);
  CHECK(resp.status == 400);
  CHECK(resp.json_body.contains("error"));
}

TEST_CASE("AdminChangeQuestionText with non-JSON body returns 400") {
  auto admin_token =
      test_helpers::login_only("127.0.0.1", 8848, "Admin", "12345678");

  auto resp = test_helpers::http_request(
      "PATCH", "127.0.0.1", 8848, "/admin/questions/1/change", "not json",
      "application/json", admin_token);
  CHECK(resp.status == 400);
  CHECK(resp.json_body.contains("error"));
}

TEST_CASE("AdminChangeQuestionText rejects non-admin users with 403") {
  auto user_token =
      test_helpers::login_only("127.0.0.1", 8848, "Jim", "12345678");

  nlohmann::json patch = {{"text", "Hacked text"}};
  auto resp = test_helpers::http_request(
      "PATCH", "127.0.0.1", 8848, "/admin/questions/1/change", patch.dump(),
      "application/json", user_token);
  CHECK(resp.status == 403);
  CHECK(resp.json_body.contains("error"));
}

TEST_CASE("AdminChangeQuestionText requires authentication (401)") {
  nlohmann::json patch = {{"text", "Should fail"}};
  auto resp = test_helpers::http_request("PATCH", "127.0.0.1", 8848,
                                         "/admin/questions/1/change",
                                         patch.dump(), "application/json");
  CHECK(resp.status == 401);
}

// ---------------------------------------------------------------------------
// Inactive user tests (HTTP 423 for secured endpoints)
// ---------------------------------------------------------------------------

TEST_CASE("InactiveUser cannot access admin users list (403 status)") {
  // Login as the inactive user from seed data
  auto inactive_token =
      test_helpers::login_only("127.0.0.1", 8848, "InactiveUser", "12345678");

  // This should return 423 Locked due to AdminAuthFilter checking is_active
  auto resp =
      test_helpers::http_request("GET", "127.0.0.1", 8848, "/admin/users", "",
                                 "application/json", inactive_token);
  CHECK(resp.status == 423);
  CHECK(resp.json_body.contains("error"));
  CHECK(resp.json_body["error"] == "User account is not active");
}

TEST_CASE("InactiveUser cannot access admin user details (423 status)") {
  // Login as the inactive user from seed data
  auto inactive_token =
      test_helpers::login_only("127.0.0.1", 8848, "InactiveUser", "12345678");

  // Access a specific user endpoint with admin filter
  auto resp =
      test_helpers::http_request("GET", "127.0.0.1", 8848, "/admin/users/1", "",
                                 "application/json", inactive_token);
  CHECK(resp.status == 423);
  CHECK(resp.json_body.contains("error"));
  CHECK(resp.json_body["error"] == "User account is not active");
}

TEST_CASE("InactiveUser can access categories (public endpoint)") {
  // Login as the inactive user from seed data
  auto inactive_token =
      test_helpers::login_only("127.0.0.1", 8848, "InactiveUser", "12345678");

  // Categories are public; no auth required.
  auto resp = test_helpers::http_request("GET", "127.0.0.1", 8848,
                                         "/categories/lang/en", "",
                                         "application/json", inactive_token);
  CHECK(resp.status == 200);
  CHECK(resp.json_body.is_array());
}

// ---------------------------------------------------------------------------
// DELETE /users/me/delete  (self-service account deletion)
// ---------------------------------------------------------------------------

TEST_CASE("Delete own account requires authentication (401)") {
  auto resp = test_helpers::http_request("DELETE", "127.0.0.1", 8848,
                                         "/users/me/delete");
  CHECK(resp.status == 401);
}

TEST_CASE("Inactive user cannot delete their own account (423 Locked)") {
  auto inactive_token =
      test_helpers::login_only("127.0.0.1", 8848, "InactiveUser", "12345678");
  // InactiveUser's id is 3 from the seed data.
  auto resp = test_helpers::http_request("DELETE", "127.0.0.1", 8848,
                                         "/users/me/delete", "",
                                         "application/json", inactive_token);
  CHECK(resp.status == 423);
  CHECK(resp.json_body.contains("error"));
  CHECK(resp.json_body["error"] == "User account is not active");
}

TEST_CASE("User can delete their own account (204 No Content)") {
  // Register and log in as a dedicated test user.
  auto user_token = test_helpers::authenticate("127.0.0.1", 8848, "deleteuser",
                                               "deleteuser@example.com",
                                               "password123", 1990, "m", "US");

  // The new user's id is not known in advance; look it up via /me.
  auto me_resp = test_helpers::http_request("GET", "127.0.0.1", 8848, "/me", "",
                                            "application/json", user_token);
  CHECK(me_resp.status == 200);
  std::string user_id = me_resp.json_body["id"].get<std::string>();

  // Delete the account.
  auto del_resp = test_helpers::http_request("DELETE", "127.0.0.1", 8848,
                                             "/users/me/delete", "",
                                             "application/json", user_token);
  CHECK(del_resp.status == 204);

  // The user no longer exists in the database, so /me returns 404.
  auto post_del_resp = test_helpers::http_request(
      "GET", "127.0.0.1", 8848, "/me", "", "application/json", user_token);
  CHECK(post_del_resp.status == 404);
}

// ---------------------------------------------------------------------------
// POST /user/password/forgot  (password reset request)
// ---------------------------------------------------------------------------
// The endpoint always returns the same generic success message regardless of
// whether the email exists or was rate-limited, to prevent user enumeration.

TEST_CASE("ForgotPassword returns generic response for existing email") {
  // Jim's email from seed data (sql/004_test_data.sql).
  nlohmann::json body = {{"email", "jim@example.com"}};
  auto resp = test_helpers::http_request("POST", "127.0.0.1", 8848,
                                         "/user/password/forgot", body.dump(),
                                         "application/json");
  CHECK(resp.status == 200);
  CHECK(resp.json_body.contains("message"));
  CHECK(resp.json_body["message"] ==
        "If an account with that email exists, a password reset link has been "
        "sent.");
}

TEST_CASE(
    "ForgotPassword returns same generic response for non-existent email") {
  // A completely made-up email that does not belong to any user.
  nlohmann::json body = {{"email", "nobody@example.com"}};
  auto resp = test_helpers::http_request("POST", "127.0.0.1", 8848,
                                         "/user/password/forgot", body.dump(),
                                         "application/json");
  CHECK(resp.status == 200);
  // The message must be identical to the one returned for existing emails.
  CHECK(resp.json_body.contains("message"));
  CHECK(resp.json_body["message"] ==
        "If an account with that email exists, a password reset link has been "
        "sent.");
}

TEST_CASE("ForgotPassword returns 400 for missing email") {
  nlohmann::json body = {};
  auto resp = test_helpers::http_request("POST", "127.0.0.1", 8848,
                                         "/user/password/forgot", body.dump(),
                                         "application/json");
  CHECK(resp.status == 400);
  CHECK(resp.json_body.contains("error"));
}

// ---------------------------------------------------------------------------
// POST /user/password/reset  (password reset consumption)
//
// Seed-data tokens (SHA-256 hashes stored in sql/004_test_data.sql):
//   "test-reset-token"    -> valid, unused, expires 1h in the future
//   "expired-reset-token" -> expired
//   "used-reset-token"    -> already used
//
// IMPORTANT: These tests change Jim's password. They run at the very end of
// the file so that earlier tests which log in as Jim with "12345678" are
// unaffected. After the valid-reset test runs, Jim's password is changed to
// "newpassword123".
// ---------------------------------------------------------------------------

TEST_CASE(
    "ResetPassword with valid token changes password, invalidates refresh "
    "tokens") {
  // 1. Login as Jim with the original password to obtain a refresh token.
  auto login_resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/login",
      nlohmann::json{{"username", "Jim"}, {"password", "12345678"}}.dump());
  REQUIRE(login_resp.status == 200);
  CHECK(login_resp.json_body.contains("refresh_token"));
  std::string old_refresh_token =
      login_resp.json_body["refresh_token"].get<std::string>();

  // 2. Reset the password using the pre-seeded valid token.
  nlohmann::json reset_body = {{"token", "test-reset-token"},
                               {"password", "newpassword123"}};
  auto reset_resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/user/password/reset", reset_body.dump(),
      "application/json");
  CHECK(reset_resp.status == 200);
  CHECK(reset_resp.json_body["message"] == "Password reset successfully");

  // 3. Login with the NEW password must now succeed and yield a fresh token
  //    pair.
  auto login_new = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/login",
      nlohmann::json{{"username", "Jim"}, {"password", "newpassword123"}}
          .dump());
  CHECK(login_new.status == 200);
  CHECK(login_new.json_body.contains("access_token"));
  CHECK(login_new.json_body.contains("refresh_token"));

  // 4. Login with the OLD password must fail.
  auto login_old = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/login",
      nlohmann::json{{"username", "Jim"}, {"password", "12345678"}}.dump());
  CHECK(login_old.status == 401);
  CHECK(login_old.json_body.contains("error"));

  // 5. The old refresh token must be invalidated — /refresh returns 401.
  nlohmann::json refresh_body = {{"refresh_token", old_refresh_token}};
  auto refresh_resp =
      test_helpers::http_request("POST", "127.0.0.1", 8848, "/refresh",
                                 refresh_body.dump(), "application/json");
  CHECK(refresh_resp.status == 401);
  CHECK(refresh_resp.json_body.contains("error"));

  // 6. The consumed token must not be reusable — a second reset with the
  //    same token returns 400 "Invalid or expired token".
  nlohmann::json reuse_body = {{"token", "test-reset-token"},
                               {"password", "anotherpassword123"}};
  auto reuse_resp = test_helpers::http_request(
      "POST", "127.0.0.1", 8848, "/user/password/reset", reuse_body.dump(),
      "application/json");
  CHECK(reuse_resp.status == 400);
  CHECK(reuse_resp.json_body["error"] == "Invalid or expired token");
}

TEST_CASE("ResetPassword with used token returns 400") {
  // The "used-reset-token" is pre-seeded with used=TRUE for Jim.
  nlohmann::json body = {{"token", "used-reset-token"},
                         {"password", "newpassword456"}};
  auto resp = test_helpers::http_request("POST", "127.0.0.1", 8848,
                                         "/user/password/reset", body.dump(),
                                         "application/json");
  CHECK(resp.status == 400);
  CHECK(resp.json_body.contains("error"));
  CHECK(resp.json_body["error"] == "Invalid or expired token");
}

TEST_CASE("ResetPassword with expired token returns 400") {
  // The "expired-reset-token" is pre-seeded with an expiry in the past.
  nlohmann::json body = {{"token", "expired-reset-token"},
                         {"password", "newpassword456"}};
  auto resp = test_helpers::http_request("POST", "127.0.0.1", 8848,
                                         "/user/password/reset", body.dump(),
                                         "application/json");
  CHECK(resp.status == 400);
  CHECK(resp.json_body.contains("error"));
  CHECK(resp.json_body["error"] == "Invalid or expired token");
}

TEST_CASE("ResetPassword with invalid token returns 400") {
  // A random string that does not match any stored token hash.
  nlohmann::json body = {{"token", "this-is-not-a-valid-token"},
                         {"password", "newpassword456"}};
  auto resp = test_helpers::http_request("POST", "127.0.0.1", 8848,
                                         "/user/password/reset", body.dump(),
                                         "application/json");
  CHECK(resp.status == 400);
  CHECK(resp.json_body.contains("error"));
  CHECK(resp.json_body["error"] == "Invalid or expired token");
}

TEST_CASE("ResetPassword with short password returns 400") {
  // Password shorter than 8 characters should be rejected before token
  // lookup, so the token is not consumed.
  nlohmann::json body = {{"token", "test-reset-token"}, {"password", "short"}};
  auto resp = test_helpers::http_request("POST", "127.0.0.1", 8848,
                                         "/user/password/reset", body.dump(),
                                         "application/json");
  CHECK(resp.status == 400);
  CHECK(resp.json_body.contains("error"));
  CHECK(resp.json_body["error"] == "password must be at least 8 characters");
}

TEST_CASE("ResetPassword with missing token returns 400") {
  nlohmann::json body = {{"password", "validpassword123"}};
  auto resp = test_helpers::http_request("POST", "127.0.0.1", 8848,
                                         "/user/password/reset", body.dump(),
                                         "application/json");
  CHECK(resp.status == 400);
  CHECK(resp.json_body.contains("error"));
}

TEST_CASE("ResetPassword with missing password returns 400") {
  nlohmann::json body = {{"token", "test-reset-token"}};
  auto resp = test_helpers::http_request("POST", "127.0.0.1", 8848,
                                         "/user/password/reset", body.dump(),
                                         "application/json");
  CHECK(resp.status == 400);
  CHECK(resp.json_body.contains("error"));
}
