#pragma once

#include <drogon/HttpController.h>

/**
 * @brief Non-generated controller for category-specific endpoints.
 *
 * Provides the hand-written category endpoints (e.g. filtering by language),
 * kept separate from any generated REST scaffold.
 */
class CategoryController : public drogon::HttpController<CategoryController> {
 public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(CategoryController::getCategoriesByLanguage,
                "/categories/lang/{1}", drogon::Get, drogon::Options,
                "JwtAuthFilter");
  METHOD_LIST_END

  /// @brief Return all categories for a given language code (e.g. "en").
  void getCategoriesByLanguage(
      const drogon::HttpRequestPtr& req,
      std::function<void(const drogon::HttpResponsePtr&)>&& cb,
      const std::string& language);
};
