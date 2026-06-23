#pragma once

#include <drogon/HttpController.h>

class QuestionController : public drogon::HttpController<QuestionController> {
 public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(QuestionController::getStats, "/questions/{1}/stats",
                drogon::Get);
  ADD_METHOD_TO(QuestionController::getQuestionsWithCategories,
                "/questions/with-categories", drogon::Get);
  METHOD_LIST_END

  void getStats(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                int questionId);
  void getQuestionsWithCategories(
      const drogon::HttpRequestPtr& req,
      std::function<void(const drogon::HttpResponsePtr&)>&& cb);
};
