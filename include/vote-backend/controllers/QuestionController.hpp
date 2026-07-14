#pragma once

#include <drogon/HttpController.h>

class QuestionController : public drogon::HttpController<QuestionController> {
 public:
  METHOD_LIST_BEGIN
  // Standard REST endpoints for /questions (moved from the generated
  // RestfulQuestionsCtrl so the generated controller can eventually be
  // retired).
  ADD_METHOD_TO(QuestionController::get, "/questions", drogon::Get,
                drogon::Options, "JwtAuthFilter");
  ADD_METHOD_TO(QuestionController::getOne, "/questions/{1}", drogon::Get,
                drogon::Options, "JwtAuthFilter");
  ADD_METHOD_TO(QuestionController::submitQuestion, "/questions/submissions",
                drogon::Post, drogon::Options, "JwtAuthFilter");
  ADD_METHOD_TO(QuestionController::getStats, "/questions/{1}/stats",
                drogon::Get, drogon::Options, "JwtAuthFilter");
  ADD_METHOD_TO(QuestionController::getAnswerOptions, "/questions/{1}/answers",
                drogon::Get, drogon::Options, "JwtAuthFilter");
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
  METHOD_LIST_END

  void getStats(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                int questionId);
  void getAnswerOptions(
      const drogon::HttpRequestPtr& req,
      std::function<void(const drogon::HttpResponsePtr&)>&& cb, int questionId);
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

  // --- Standard REST endpoints (moved from the generated RestfulQuestionsCtrl)
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
