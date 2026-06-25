/**
 *
 *  Users.h
 *  This file is manually created to represent the authentication user.
 */

#pragma once
#include <drogon/orm/Result.h>
#include <drogon/orm/Row.h>
#include <drogon/orm/Field.h>
#include <drogon/orm/SqlBinder.h>
#include <drogon/orm/Mapper.h>
#include <drogon/orm/BaseBuilder.h>
#ifdef __cpp_impl_coroutine
#include <drogon/orm/CoroMapper.h>
#endif
#include <trantor/utils/Date.h>
#include <trantor/utils/Logger.h>
#include <json/json.h>
#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <tuple>
#include <stdint.h>
#include <iostream>

namespace drogon
{
namespace orm
{
class DbClient;
using DbClientPtr = std::shared_ptr<DbClient>;
}
}
namespace drogon_model
{
namespace vote
{

class Users
{
  public:
    struct Cols
    {
        static const std::string _id;
        static const std::string _username;
        static const std::string _email;
        static const std::string _password_hash;
        static const std::string _birth_year;
        static const std::string _gender;
        static const std::string _nationality;
        static const std::string _created_at;
        static const std::string _updated_at;
    };

    static const int primaryKeyNumber;
    static const std::string tableName;
    static const bool hasPrimaryKey;
    static const std::string primaryKeyName;
    using PrimaryKeyType = int64_t;
    const PrimaryKeyType &getPrimaryKey() const;

    explicit Users(const drogon::orm::Row &r, const ssize_t indexOffset = 0) noexcept;
    explicit Users(const Json::Value &pJson) noexcept(false);
    Users() = default;

    void updateByJson(const Json::Value &pJson) noexcept(false);
    static bool validateJsonForCreation(const Json::Value &pJson, std::string &err);
    static bool validateJsonForUpdate(const Json::Value &pJson, std::string &err);
    static bool validJsonOfField(size_t index,
                          const std::string &fieldName,
                          const Json::Value &pJson,
                          std::string &err,
                          bool isForCreation);

    /**  For column id  */
    const int64_t &getValueOfId() const noexcept;
    const std::shared_ptr<int64_t> &getId() const noexcept;
    void setId(const int64_t &pId) noexcept;

    /**  For column username  */
    const std::string &getValueOfUsername() const noexcept;
    const std::shared_ptr<std::string> &getUsername() const noexcept;
    void setUsername(const std::string &pUsername) noexcept;
    void setUsername(std::string &&pUsername) noexcept;

    /**  For column email  */
    const std::string &getValueOfEmail() const noexcept;
    const std::shared_ptr<std::string> &getEmail() const noexcept;
    void setEmail(const std::string &pEmail) noexcept;
    void setEmail(std::string &&pEmail) noexcept;

    /**  For column password_hash  */
    const std::string &getValueOfPasswordHash() const noexcept;
    const std::shared_ptr<std::string> &getPasswordHash() const noexcept;
    void setPasswordHash(const std::string &pPasswordHash) noexcept;
    void setPasswordHash(std::string &&pPasswordHash) noexcept;

    /**  For column birth_year  */
    const int &getValueOfBirthYear() const noexcept;
    const std::shared_ptr<int> &getBirthYear() const noexcept;
    void setBirthYear(const int &pBirthYear) noexcept;

    /**  For column gender  */
    const std::string &getValueOfGender() const noexcept;
    const std::shared_ptr<std::string> &getGender() const noexcept;
    void setGender(const std::string &pGender) noexcept;
    void setGender(std::string &&pGender) noexcept;

    /**  For column nationality  */
    const std::string &getValueOfNationality() const noexcept;
    const std::shared_ptr<std::string> &getNationality() const noexcept;
    void setNationality(const std::string &pNationality) noexcept;
    void setNationality(std::string &&pNationality) noexcept;

    /**  For column created_at  */
    const ::trantor::Date &getValueOfCreatedAt() const noexcept;
    const std::shared_ptr<::trantor::Date> &getCreatedAt() const noexcept;
    void setCreatedAt(const ::trantor::Date &pCreatedAt) noexcept;

    /**  For column updated_at  */
    const ::trantor::Date &getValueOfUpdatedAt() const noexcept;
    const std::shared_ptr<::trantor::Date> &getUpdatedAt() const noexcept;
    void setUpdatedAt(const ::trantor::Date &pUpdatedAt) noexcept;

    static size_t getColumnNumber() noexcept {  return 9;  }
    static const std::string &getColumnName(size_t index) noexcept(false);

    Json::Value toJson() const;
    std::string toString() const;

  private:
    friend drogon::orm::Mapper<Users>;
    friend drogon::orm::BaseBuilder<Users, true, true>;
    friend drogon::orm::BaseBuilder<Users, true, false>;
    friend drogon::orm::BaseBuilder<Users, false, true>;
    friend drogon::orm::BaseBuilder<Users, false, false>;
#ifdef __cpp_impl_coroutine
    friend drogon::orm::CoroMapper<Users>;
#endif
    static const std::vector<std::string> &insertColumns() noexcept;
    void outputArgs(drogon::orm::internal::SqlBinder &binder) const;
    const std::vector<std::string> updateColumns() const;
    void updateArgs(drogon::orm::internal::SqlBinder &binder) const;
    void updateId(const uint64_t id);
    std::shared_ptr<int64_t> id_;
    std::shared_ptr<std::string> username_;
    std::shared_ptr<std::string> email_;
    std::shared_ptr<std::string> passwordHash_;
    std::shared_ptr<int> birthYear_;
    std::shared_ptr<std::string> gender_;
    std::shared_ptr<std::string> nationality_;
    std::shared_ptr<::trantor::Date> createdAt_;
    std::shared_ptr<::trantor::Date> updatedAt_;
    struct MetaData
    {
        const std::string colName_;
        const std::string colType_;
        const std::string colDatabaseType_;
        const ssize_t colLength_;
        const bool isAutoVal_;
        const bool isPrimaryKey_;
        const bool notNull_;
    };
    static const std::vector<MetaData> metaData_;
    bool dirtyFlag_[9]={ false };
  public:
    static const std::string &sqlForFindingByPrimaryKey()
    {
        static const std::string sql="select * from " + tableName + " where id = $1";
        return sql;
    }

    static const std::string &sqlForDeletingByPrimaryKey()
    {
        static const std::string sql="delete from " + tableName + " where id = $1";
        return sql;
    }
    std::string sqlForInserting(bool &needSelection) const
    {
        std::string sql="insert into " + tableName + " (";
        size_t parametersCount = 0;
        needSelection = false;
        if(dirtyFlag_[0])
        {
            sql += "id,";
            ++parametersCount;
        }
        if(dirtyFlag_[1])
        {
            sql += "username,";
            ++parametersCount;
        }
        if(dirtyFlag_[2])
        {
            sql += "email,";
            ++parametersCount;
        }
        if(dirtyFlag_[3])
        {
            sql += "password_hash,";
            ++parametersCount;
        }
        if(dirtyFlag_[4])
        {
            sql += "birth_year,";
            ++parametersCount;
        }
        if(dirtyFlag_[5])
        {
            sql += "gender,";
            ++parametersCount;
        }
        if(dirtyFlag_[6])
        {
            sql += "nationality,";
            ++parametersCount;
        }
        sql += "created_at,";
        ++parametersCount;
        if(!dirtyFlag_[7])
        {
            needSelection=true;
        }
        sql += "updated_at,";
        ++parametersCount;
        if(!dirtyFlag_[8])
        {
            needSelection=true;
        }
        needSelection=true;
        if(parametersCount > 0)
        {
            sql[sql.length()-1]=')';
            sql += " values (";
        }
        else
            sql += ") values (";

        int placeholder=1;
        char placeholderStr[64];
        size_t n=0;
        if(dirtyFlag_[0])
        {
            n = snprintf(placeholderStr,sizeof(placeholderStr),"$%d,",placeholder++);
            sql.append(placeholderStr, n);
        }
        if(dirtyFlag_[1])
        {
            n = snprintf(placeholderStr,sizeof(placeholderStr),"$%d,",placeholder++);
            sql.append(placeholderStr, n);
        }
        if(dirtyFlag_[2])
        {
            n = snprintf(placeholderStr,sizeof(placeholderStr),"$%d,",placeholder++);
            sql.append(placeholderStr, n);
        }
        if(dirtyFlag_[3])
        {
            n = snprintf(placeholderStr,sizeof(placeholderStr),"$%d,",placeholder++);
            sql.append(placeholderStr, n);
        }
        if(dirtyFlag_[4])
        {
            n = snprintf(placeholderStr,sizeof(placeholderStr),"$%d,",placeholder++);
            sql.append(placeholderStr, n);
        }
        if(dirtyFlag_[5])
        {
            n = snprintf(placeholderStr,sizeof(placeholderStr),"$%d,",placeholder++);
            sql.append(placeholderStr, n);
        }
        if(dirtyFlag_[6])
        {
            n = snprintf(placeholderStr,sizeof(placeholderStr),"$%d,",placeholder++);
            sql.append(placeholderStr, n);
        }
        if(dirtyFlag_[7])
        {
            n = snprintf(placeholderStr,sizeof(placeholderStr),"$%d,",placeholder++);
            sql.append(placeholderStr, n);
        }
        else
        {
            sql +="default,";
        }
        if(dirtyFlag_[8])
        {
            n = snprintf(placeholderStr,sizeof(placeholderStr),"$%d,",placeholder++);
            sql.append(placeholderStr, n);
        }
        else
        {
            sql +="default,";
        }
        if(parametersCount > 0)
        {
            sql.resize(sql.length() - 1);
        }
        if(needSelection)
        {
            sql.append(") returning *");
        }
        else
        {
            sql.append(1, ')');
        }
        LOG_TRACE << sql;
        return sql;
    }
};
} // namespace vote
} // namespace drogon_model
