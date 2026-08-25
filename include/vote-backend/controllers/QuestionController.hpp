#pragma once

#include <drogon/HttpController.h>

class QuestionController : public drogon::HttpController<QuestionController> {
 public:
  METHOD_LIST_BEGIN
  // Standard REST endpoints for /questions.
  // Statistics metadata describing the tag dimensions/values the backend can
  // resolve (public, like /questions/{id}/stats).
  ADD_METHOD_TO(QuestionController::getStatsMeta, "/stats/meta", drogon::Get,
                drogon::Options);
  ADD_METHOD_TO(QuestionController::get, "/questions", drogon::Get,
                drogon::Options, "JwtAuthFilter");
  ADD_METHOD_TO(QuestionController::getOne, "/questions/{1}", drogon::Get,
                drogon::Options, "JwtAuthFilter");
  ADD_METHOD_TO(QuestionController::submitQuestion, "/questions/submissions",
                drogon::Post, drogon::Options, "JwtAuthFilter");
  ADD_METHOD_TO(QuestionController::getStats, "/questions/{1}/stats",
                drogon::Get, drogon::Options);
  ADD_METHOD_TO(QuestionController::getAnswerOptions, "/questions/{1}/answers",
                drogon::Get, drogon::Options);
  ADD_METHOD_TO(QuestionController::getAnswerOptionsWithAuth,
                "/questions/{1}/answers-with-auth", drogon::Get,
                drogon::Options, "JwtAuthFilter");
  ADD_METHOD_TO(QuestionController::getQuestionsWithCategories,
                "/questions/with-categories", drogon::Get, drogon::Options,
                "JwtAuthFilter");
  ADD_METHOD_TO(QuestionController::getQuestionsByLanguage,
                "/questions/lang/{1}", drogon::Get, drogon::Options,
                "JwtAuthFilter");
  ADD_METHOD_TO(QuestionController::searchQuestions, "/questions/search",
                drogon::Get, drogon::Options, "JwtAuthFilter");
  ADD_METHOD_TO(QuestionController::restSearchQuestions,
                "/questions/restSearch", drogon::Post, drogon::Options,
                "JwtAuthFilter");
  ADD_METHOD_TO(QuestionController::answerQuestion, "/questions/{1}/answer",
                drogon::Post, drogon::Options, "JwtAuthFilter");
  // Submission workflow (Option B): a user sees only their own submissions;
  // an admin sees the full review queue and can approve / reject.
  ADD_METHOD_TO(QuestionController::getMySubmissions, "/questions/mine",
                drogon::Get, drogon::Options, "JwtAuthFilter");
  ADD_METHOD_TO(QuestionController::listSubmissions,
                "/admin/questions/submissions", drogon::Get, drogon::Options,
                "AdminAuthFilter");
  ADD_METHOD_TO(QuestionController::approveQuestion,
                "/admin/questions/{1}/approve", drogon::Post, drogon::Options,
                "AdminAuthFilter");
  ADD_METHOD_TO(QuestionController::rejectQuestion,
                "/admin/questions/{1}/reject", drogon::Post, drogon::Options,
                "AdminAuthFilter");
  ADD_METHOD_TO(QuestionController::getAdminAnswerOptions,
                "/admin/questions/{1}/answers", drogon::Get, drogon::Options,
                "AdminAuthFilter");
  ADD_METHOD_TO(QuestionController::deleteQuestion,
                "/admin/questions/{1}/delete", drogon::Post, drogon::Options,
                "AdminAuthFilter");
  ADD_METHOD_TO(QuestionController::changeQuestionText,
                "/admin/questions/{1}/change", drogon::Patch,
                drogon::Options, "AdminAuthFilter");
  METHOD_LIST_END

    void getStats(const drogon::HttpRequestPtr& req,
                  std::function<void(const drogon::HttpResponsePtr&)>&& cb, int questionId);
    // GET /stats/meta: describes the statistics tag contract — configured age
    // bucket size, privacy threshold, and the allowed values per dimension.
    // Age labels derive from config; nationality values reflect the normalized
    // country codes actually present among users (>= min_answers occurrences).
    void getStatsMeta(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& cb);
  void getAnswerOptions(
      const drogon::HttpRequestPtr& req,
      std::function<void(const drogon::HttpResponsePtr&)>&& cb, int questionId);
  // Authenticated endpoint: returns answer options for approved questions,
  // and also allows the owner to see options for their own pending questions.
  void getAnswerOptionsWithAuth(
      const drogon::HttpRequestPtr& req,
      std::function<void(const drogon::HttpResponsePtr&)>&& cb,
      int questionId);
  void getQuestionsWithCategories(
      const drogon::HttpRequestPtr& req,
      std::function<void(const drogon::HttpResponsePtr&)>&& cb);
  void getQuestionsByLanguage(
      const drogon::HttpRequestPtr& req,
      std::function<void(const drogon::HttpResponsePtr&)>&& cb,
      const std::string& language);
  void searchQuestions(
      const drogon::HttpRequestPtr& req,
      std::function<void(const drogon::HttpResponsePtr&)>&& cb);
  void restSearchQuestions(
      const drogon::HttpRequestPtr& req,
      std::function<void(const drogon::HttpResponsePtr&)>&& cb);
  void answerQuestion(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                      int questionId);

  // --- Submission workflow (Option B) -------------------------------------
  // GET /questions/mine: the authenticated user's own submissions (any status).
  void getMySubmissions(
      const drogon::HttpRequestPtr& req,
      std::function<void(const drogon::HttpResponsePtr&)>&& cb);
  // GET /admin/questions/submissions: review queue of non-approved questions.
  void listSubmissions(
      const drogon::HttpRequestPtr& req,
      std::function<void(const drogon::HttpResponsePtr&)>&& cb);
  // POST /admin/questions/{1}/approve: mark a submission as approved.
  void approveQuestion(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                       int questionId);
  // POST /admin/questions/{1}/reject: mark a submission as rejected.
  void rejectQuestion(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                      int questionId);
  // GET /admin/questions/{1}/answers: answer options for any question, exposed
  // to administrators for submission review (no visibility restriction).
  void getAdminAnswerOptions(
      const drogon::HttpRequestPtr& req,
      std::function<void(const drogon::HttpResponsePtr&)>&& cb,
      int questionId);
  // POST /admin/questions/{1}/delete: permanently remove a question. All
  // dependent rows (answer options, user answers, anonymous answer tracking)
  // are removed automatically via ON DELETE CASCADE. Only admins may call this.
  void deleteQuestion(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                      int questionId);
  // PATCH /admin/questions/{1}/change: update the text of an existing
  // question. Only admins may call this. The request body must be a JSON
  // object with a non-empty "text" field.
  void changeQuestionText(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                          int questionId);

  // --- Standard REST endpoints
  // GET /questions: list approved questions (public; requires a valid token).
  void get(const drogon::HttpRequestPtr& req,
           std::function<void(const drogon::HttpResponsePtr&)>&& cb);
  // GET /questions/{1}: fetch a single approved question by id.
  void getOne(const drogon::HttpRequestPtr& req,
              std::function<void(const drogon::HttpResponsePtr&)>&& cb,
              int questionId);
  // POST /questions/submissions: create a *pending* submission owned by the
  // caller; it is not publicly visible until an admin approves it.
  void submitQuestion(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& cb);
};
