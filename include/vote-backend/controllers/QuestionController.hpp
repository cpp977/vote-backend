#pragma once

#include <drogon/HttpController.h>

class QuestionController : public drogon::HttpController<QuestionController> {
 public:
  METHOD_LIST_BEGIN
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
};
