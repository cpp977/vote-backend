/**
 *
 *  Users.cc
 *  This file is manually created to represent the authentication user.
 */

#include "Users.h"
#include <drogon/utils/Utilities.h>
#include <string>

using namespace drogon;
using namespace drogon::orm;
using namespace drogon_model::vote;

const std::string Users::Cols::_id = "\"id\"";
const std::string Users::Cols::_username = "\"username\"";
const std::string Users::Cols::_email = "\"email\"";
const std::string Users::Cols::_password_hash = "\"password_hash\"";
const std::string Users::Cols::_created_at = "\"created_at\"";
const std::string Users::Cols::_updated_at = "\"updated_at\"";
const std::string Users::primaryKeyName = "id";
const bool Users::hasPrimaryKey = true;
const std::string Users::tableName = "\"users\"";

const std::vector<typename Users::MetaData> Users::metaData_={
{"id","int64_t","bigint",8,1,1,1},
{"username","std::string","text",0,0,0,1},
{"email","std::string","text",0,0,0,1},
{"password_hash","std::string","text",0,0,0,1},
{"created_at","::trantor::Date","timestamp with time zone",0,0,0,1},
{"updated_at","::trantor::Date","timestamp with time zone",0,0,0,1}
};
const std::string &Users::getColumnName(size_t index) noexcept(false)
{
    assert(index < metaData_.size());
    return metaData_[index].colName_;
}

Users::Users(const Row &r, const ssize_t indexOffset) noexcept
{
    if(indexOffset < 0)
    {
        if(!r["id"].isNull())
        {
            id_=std::make_shared<int64_t>(r["id"].as<int64_t>());
        }
        if(!r["username"].isNull())
        {
            username_=std::make_shared<std::string>(r["username"].as<std::string>());
        }
        if(!r["email"].isNull())
        {
            email_=std::make_shared<std::string>(r["email"].as<std::string>());
        }
        if(!r["password_hash"].isNull())
        {
            passwordHash_=std::make_shared<std::string>(r["password_hash"].as<std::string>());
        }
        if(!r["created_at"].isNull())
        {
            auto timeStr = r["created_at"].as<std::string>();
            struct tm stm;
            memset(&stm,0,sizeof(stm));
            auto p = strptime(timeStr.c_str(),"%Y-%m-%d %H:%M:%S",&stm);
            time_t t = mktime(&stm);
            size_t decimalNum = 0;
            if(p)
            {
                if(*p=='.')
                {
                    std::string decimals(p+1,&timeStr[timeStr.length()]);
                    while(decimals.length()<6)
                    {
                        decimals += "0";
                    }
                    decimalNum = (size_t)atol(decimals.c_str());
                }
                createdAt_=std::make_shared<::trantor::Date>(t*1000000+decimalNum);
            }
        }
        if(!r["updated_at"].isNull())
        {
            auto timeStr = r["updated_at"].as<std::string>();
            struct tm stm;
            memset(&stm,0,sizeof(stm));
            auto p = strptime(timeStr.c_str(),"%Y-%m-%d %H:%M:%S",&stm);
            time_t t = mktime(&stm);
            size_t decimalNum = 0;
            if(p)
            {
                if(*p=='.')
                {
                    std::string decimals(p+1,&timeStr[timeStr.length()]);
                    while(decimals.length()<6)
                    {
                        decimals += "0";
                    }
                    decimalNum = (size_t)atol(decimals.c_str());
                }
                updatedAt_=std::make_shared<::trantor::Date>(t*1000000+decimalNum);
            }
        }
    }
    else
    {
        size_t offset = (size_t)indexOffset;
        if(offset + 6 > r.size())
        {
            LOG_FATAL << "Invalid SQL result for this model";
            return;
        }
        size_t index;
        index = offset + 0;
        if(!r[index].isNull())
        {
            id_=std::make_shared<int64_t>(r[index].as<int64_t>());
        }
        index = offset + 1;
        if(!r[index].isNull())
        {
            username_=std::make_shared<std::string>(r[index].as<std::string>());
        }
        index = offset + 2;
        if(!r[index].isNull())
        {
            email_=std::make_shared<std::string>(r[index].as<std::string>());
        }
        index = offset + 3;
        if(!r[index].isNull())
        {
            passwordHash_=std::make_shared<std::string>(r[index].as<std::string>());
        }
        index = offset + 4;
        if(!r[index].isNull())
        {
            auto timeStr = r[index].as<std::string>();
            struct tm stm;
            memset(&stm,0,sizeof(stm));
            auto p = strptime(timeStr.c_str(),"%Y-%m-%d %H:%M:%S",&stm);
            time_t t = mktime(&stm);
            size_t decimalNum = 0;
            if(p)
            {
                if(*p=='.')
                {
                    std::string decimals(p+1,&timeStr[timeStr.length()]);
                    while(decimals.length()<6)
                    {
                        decimals += "0";
                    }
                    decimalNum = (size_t)atol(decimals.c_str());
                }
                createdAt_=std::make_shared<::trantor::Date>(t*1000000+decimalNum);
            }
        }
        index = offset + 5;
        if(!r[index].isNull())
        {
            auto timeStr = r[index].as<std::string>();
            struct tm stm;
            memset(&stm,0,sizeof(stm));
            auto p = strptime(timeStr.c_str(),"%Y-%m-%d %H:%M:%S",&stm);
            time_t t = mktime(&stm);
            size_t decimalNum = 0;
            if(p)
            {
                if(*p=='.')
                {
                    std::string decimals(p+1,&timeStr[timeStr.length()]);
                    while(decimals.length()<6)
                    {
                        decimals += "0";
                    }
                    decimalNum = (size_t)atol(decimals.c_str());
                }
                updatedAt_=std::make_shared<::trantor::Date>(t*1000000+decimalNum);
            }
        }
    }

}

Users::Users(const Json::Value &pJson) noexcept(false)
{
    if(pJson.isMember("id"))
    {
        dirtyFlag_[0]=true;
        if(!pJson["id"].isNull())
        {
            id_=std::make_shared<int64_t>((int64_t)pJson["id"].asInt64());
        }
    }
    if(pJson.isMember("username"))
    {
        dirtyFlag_[1]=true;
        if(!pJson["username"].isNull())
        {
            username_=std::make_shared<std::string>(pJson["username"].asString());
        }
    }
    if(pJson.isMember("email"))
    {
        dirtyFlag_[2]=true;
        if(!pJson["email"].isNull())
        {
            email_=std::make_shared<std::string>(pJson["email"].asString());
        }
    }
    if(pJson.isMember("password_hash"))
    {
        dirtyFlag_[3]=true;
        if(!pJson["password_hash"].isNull())
        {
            passwordHash_=std::make_shared<std::string>(pJson["password_hash"].asString());
        }
    }
    if(pJson.isMember("created_at"))
    {
        dirtyFlag_[4]=true;
        if(!pJson["created_at"].isNull())
        {
            auto timeStr = pJson["created_at"].asString();
            struct tm stm;
            memset(&stm,0,sizeof(stm));
            auto p = strptime(timeStr.c_str(),"%Y-%m-%d %H:%M:%S",&stm);
            time_t t = mktime(&stm);
            size_t decimalNum = 0;
            if(p)
            {
                if(*p=='.')
                {
                    std::string decimals(p+1,&timeStr[timeStr.length()]);
                    while(decimals.length()<6)
                    {
                        decimals += "0";
                    }
                    decimalNum = (size_t)atol(decimals.c_str());
                }
                createdAt_=std::make_shared<::trantor::Date>(t*1000000+decimalNum);
            }
        }
    }
    if(pJson.isMember("updated_at"))
    {
        dirtyFlag_[5]=true;
        if(!pJson["updated_at"].isNull())
        {
            auto timeStr = pJson["updated_at"].asString();
            struct tm stm;
            memset(&stm,0,sizeof(stm));
            auto p = strptime(timeStr.c_str(),"%Y-%m-%d %H:%M:%S",&stm);
            time_t t = mktime(&stm);
            size_t decimalNum = 0;
            if(p)
            {
                if(*p=='.')
                {
                    std::string decimals(p+1,&timeStr[timeStr.length()]);
                    while(decimals.length()<6)
                    {
                        decimals += "0";
                    }
                    decimalNum = (size_t)atol(decimals.c_str());
                }
                updatedAt_=std::make_shared<::trantor::Date>(t*1000000+decimalNum);
            }
        }
    }
}

void Users::updateByJson(const Json::Value &pJson) noexcept(false)
{
    if(pJson.isMember("id"))
    {
        if(!pJson["id"].isNull())
        {
            id_=std::make_shared<int64_t>((int64_t)pJson["id"].asInt64());
        }
    }
    if(pJson.isMember("username"))
    {
        dirtyFlag_[1] = true;
        if(!pJson["username"].isNull())
        {
            username_=std::make_shared<std::string>(pJson["username"].asString());
        }
    }
    if(pJson.isMember("email"))
    {
        dirtyFlag_[2] = true;
        if(!pJson["email"].isNull())
        {
            email_=std::make_shared<std::string>(pJson["email"].asString());
        }
    }
    if(pJson.isMember("password_hash"))
    {
        dirtyFlag_[3] = true;
        if(!pJson["password_hash"].isNull())
        {
            passwordHash_=std::make_shared<std::string>(pJson["password_hash"].asString());
        }
    }
    if(pJson.isMember("created_at"))
    {
        dirtyFlag_[4] = true;
        if(!pJson["created_at"].isNull())
        {
            auto timeStr = pJson["created_at"].asString();
            struct tm stm;
            memset(&stm,0,sizeof(stm));
            auto p = strptime(timeStr.c_str(),"%Y-%m-%d %H:%M:%S",&stm);
            time_t t = mktime(&stm);
            size_t decimalNum = 0;
            if(p)
            {
                if(*p=='.')
                {
                    std::string decimals(p+1,&timeStr[timeStr.length()]);
                    while(decimals.length()<6)
                    {
                        decimals += "0";
                    }
                    decimalNum = (size_t)atol(decimals.c_str());
                }
                createdAt_=std::make_shared<::trantor::Date>(t*1000000+decimalNum);
            }
        }
    }
    if(pJson.isMember("updated_at"))
    {
        dirtyFlag_[5] = true;
        if(!pJson["updated_at"].isNull())
        {
            auto timeStr = pJson["updated_at"].asString();
            struct tm stm;
            memset(&stm,0,sizeof(stm));
            auto p = strptime(timeStr.c_str(),"%Y-%m-%d %H:%M:%S",&stm);
            time_t t = mktime(&stm);
            size_t decimalNum = 0;
            if(p)
            {
                if(*p=='.')
                {
                    std::string decimals(p+1,&timeStr[timeStr.length()]);
                    while(decimals.length()<6)
                    {
                        decimals += "0";
                    }
                    decimalNum = (size_t)atol(decimals.c_str());
                }
                updatedAt_=std::make_shared<::trantor::Date>(t*1000000+decimalNum);
            }
        }
    }
}

const int64_t &Users::getValueOfId() const noexcept
{
    static const int64_t defaultValue = int64_t();
    if(id_)
        return *id_;
    return defaultValue;
}
const std::shared_ptr<int64_t> &Users::getId() const noexcept
{
    return id_;
}
void Users::setId(const int64_t &pId) noexcept
{
    id_ = std::make_shared<int64_t>(pId);
    dirtyFlag_[0] = true;
}
const typename Users::PrimaryKeyType & Users::getPrimaryKey() const
{
    assert(id_);
    return *id_;
}

const std::string &Users::getValueOfUsername() const noexcept
{
    static const std::string defaultValue = std::string();
    if(username_)
        return *username_;
    return defaultValue;
}
const std::shared_ptr<std::string> &Users::getUsername() const noexcept
{
    return username_;
}
void Users::setUsername(const std::string &pUsername) noexcept
{
    username_ = std::make_shared<std::string>(pUsername);
    dirtyFlag_[1] = true;
}
void Users::setUsername(std::string &&pUsername) noexcept
{
    username_ = std::make_shared<std::string>(std::move(pUsername));
    dirtyFlag_[1] = true;
}

const std::string &Users::getValueOfEmail() const noexcept
{
    static const std::string defaultValue = std::string();
    if(email_)
        return *email_;
    return defaultValue;
}
const std::shared_ptr<std::string> &Users::getEmail() const noexcept
{
    return email_;
}
void Users::setEmail(const std::string &pEmail) noexcept
{
    email_ = std::make_shared<std::string>(pEmail);
    dirtyFlag_[2] = true;
}
void Users::setEmail(std::string &&pEmail) noexcept
{
    email_ = std::make_shared<std::string>(std::move(pEmail));
    dirtyFlag_[2] = true;
}

const std::string &Users::getValueOfPasswordHash() const noexcept
{
    static const std::string defaultValue = std::string();
    if(passwordHash_)
        return *passwordHash_;
    return defaultValue;
}
const std::shared_ptr<std::string> &Users::getPasswordHash() const noexcept
{
    return passwordHash_;
}
void Users::setPasswordHash(const std::string &pPasswordHash) noexcept
{
    passwordHash_ = std::make_shared<std::string>(pPasswordHash);
    dirtyFlag_[3] = true;
}
void Users::setPasswordHash(std::string &&pPasswordHash) noexcept
{
    passwordHash_ = std::make_shared<std::string>(std::move(pPasswordHash));
    dirtyFlag_[3] = true;
}

const ::trantor::Date &Users::getValueOfCreatedAt() const noexcept
{
    static const ::trantor::Date defaultValue = ::trantor::Date();
    if(createdAt_)
        return *createdAt_;
    return defaultValue;
}
const std::shared_ptr<::trantor::Date> &Users::getCreatedAt() const noexcept
{
    return createdAt_;
}
void Users::setCreatedAt(const ::trantor::Date &pCreatedAt) noexcept
{
    createdAt_ = std::make_shared<::trantor::Date>(pCreatedAt);
    dirtyFlag_[4] = true;
}

const ::trantor::Date &Users::getValueOfUpdatedAt() const noexcept
{
    static const ::trantor::Date defaultValue = ::trantor::Date();
    if(updatedAt_)
        return *updatedAt_;
    return defaultValue;
}
const std::shared_ptr<::trantor::Date> &Users::getUpdatedAt() const noexcept
{
    return updatedAt_;
}
void Users::setUpdatedAt(const ::trantor::Date &pUpdatedAt) noexcept
{
    updatedAt_ = std::make_shared<::trantor::Date>(pUpdatedAt);
    dirtyFlag_[5] = true;
}

void Users::updateId(const uint64_t id)
{
}

const std::vector<std::string> &Users::insertColumns() noexcept
{
    static const std::vector<std::string> inCols={
        "username",
        "email",
        "password_hash",
        "created_at",
        "updated_at"
    };
    return inCols;
}

void Users::outputArgs(drogon::orm::internal::SqlBinder &binder) const
{
    if(dirtyFlag_[1])
    {
        if(getUsername())
        {
            binder << getValueOfUsername();
        }
        else
        {
            binder << nullptr;
        }
    }
    if(dirtyFlag_[2])
    {
        if(getEmail())
        {
            binder << getValueOfEmail();
        }
        else
        {
            binder << nullptr;
        }
    }
    if(dirtyFlag_[3])
    {
        if(getPasswordHash())
        {
            binder << getValueOfPasswordHash();
        }
        else
        {
            binder << nullptr;
        }
    }
    if(dirtyFlag_[4])
    {
        if(getCreatedAt())
        {
            binder << getValueOfCreatedAt();
        }
        else
        {
            binder << nullptr;
        }
    }
    if(dirtyFlag_[5])
    {
        if(getUpdatedAt())
        {
            binder << getValueOfUpdatedAt();
        }
        else
        {
            binder << nullptr;
        }
    }
}

const std::vector<std::string> Users::updateColumns() const
{
    std::vector<std::string> ret;
    if(dirtyFlag_[1])
    {
        ret.push_back(getColumnName(1));
    }
    if(dirtyFlag_[2])
    {
        ret.push_back(getColumnName(2));
    }
    if(dirtyFlag_[3])
    {
        ret.push_back(getColumnName(3));
    }
    if(dirtyFlag_[4])
    {
        ret.push_back(getColumnName(4));
    }
    if(dirtyFlag_[5])
    {
        ret.push_back(getColumnName(5));
    }
    return ret;
}

void Users::updateArgs(drogon::orm::internal::SqlBinder &binder) const
{
    if(dirtyFlag_[1])
    {
        if(getUsername())
        {
            binder << getValueOfUsername();
        }
        else
        {
            binder << nullptr;
        }
    }
    if(dirtyFlag_[2])
    {
        if(getEmail())
        {
            binder << getValueOfEmail();
        }
        else
        {
            binder << nullptr;
        }
    }
    if(dirtyFlag_[3])
    {
        if(getPasswordHash())
        {
            binder << getValueOfPasswordHash();
        }
        else
        {
            binder << nullptr;
        }
    }
    if(dirtyFlag_[4])
    {
        if(getCreatedAt())
        {
            binder << getValueOfCreatedAt();
        }
        else
        {
            binder << nullptr;
        }
    }
    if(dirtyFlag_[5])
    {
        if(getUpdatedAt())
        {
            binder << getValueOfUpdatedAt();
        }
        else
        {
            binder << nullptr;
        }
    }
}

Json::Value Users::toJson() const
{
    Json::Value ret;
    if(getId())
    {
        ret["id"]=(Json::Int64)getValueOfId();
    }
    else
    {
        ret["id"]=Json::Value();
    }
    if(getUsername())
    {
        ret["username"]=getValueOfUsername();
    }
    else
    {
        ret["username"]=Json::Value();
    }
    if(getEmail())
    {
        ret["email"]=getValueOfEmail();
    }
    else
    {
        ret["email"]=Json::Value();
    }
    // NOTE: password_hash is intentionally excluded from JSON serialization
    if(getCreatedAt())
    {
        ret["created_at"]=getCreatedAt()->toDbStringLocal();
    }
    else
    {
        ret["created_at"]=Json::Value();
    }
    if(getUpdatedAt())
    {
        ret["updated_at"]=getUpdatedAt()->toDbStringLocal();
    }
    else
    {
        ret["updated_at"]=Json::Value();
    }
    return ret;
}

std::string Users::toString() const
{
    return toJson().toStyledString();
}

bool Users::validateJsonForCreation(const Json::Value &pJson, std::string &err)
{
    if(pJson.isMember("id"))
    {
        if(!validJsonOfField(0, "id", pJson["id"], err, true))
            return false;
    }
    if(pJson.isMember("username"))
    {
        if(!validJsonOfField(1, "username", pJson["username"], err, true))
            return false;
    }
    else
    {
        err="The username column cannot be null";
        return false;
    }
    if(pJson.isMember("email"))
    {
        if(!validJsonOfField(2, "email", pJson["email"], err, true))
            return false;
    }
    else
    {
        err="The email column cannot be null";
        return false;
    }
    if(pJson.isMember("password_hash"))
    {
        if(!validJsonOfField(3, "password_hash", pJson["password_hash"], err, true))
            return false;
    }
    else
    {
        err="The password_hash column cannot be null";
        return false;
    }
    return true;
}
bool Users::validateJsonForUpdate(const Json::Value &pJson, std::string &err)
{
    if(pJson.isMember("id"))
    {
        if(!validJsonOfField(0, "id", pJson["id"], err, false))
            return false;
    }
    else
    {
        err = "The value of primary key must be set in the json object for update";
        return false;
    }
    if(pJson.isMember("username"))
    {
        if(!validJsonOfField(1, "username", pJson["username"], err, false))
            return false;
    }
    if(pJson.isMember("email"))
    {
        if(!validJsonOfField(2, "email", pJson["email"], err, false))
            return false;
    }
    if(pJson.isMember("password_hash"))
    {
        if(!validJsonOfField(3, "password_hash", pJson["password_hash"], err, false))
            return false;
    }
    return true;
}
bool Users::validJsonOfField(size_t index,
                                 const std::string &fieldName,
                                 const Json::Value &pJson,
                                 std::string &err,
                                 bool isForCreation)
{
    switch(index)
    {
        case 0:
            if(pJson.isNull())
            {
                err="The " + fieldName + " column cannot be null";
                return false;
            }
            if(isForCreation)
            {
                err="The automatic primary key cannot be set";
                return false;
            }
            if(!pJson.isInt64())
            {
                err="Type error in the "+fieldName+" field";
                return false;
            }
            break;
        case 1:
            if(pJson.isNull())
            {
                err="The " + fieldName + " column cannot be null";
                return false;
            }
            if(!pJson.isString())
            {
                err="Type error in the "+fieldName+" field";
                return false;
            }
            break;
        case 2:
            if(pJson.isNull())
            {
                err="The " + fieldName + " column cannot be null";
                return false;
            }
            if(!pJson.isString())
            {
                err="Type error in the "+fieldName+" field";
                return false;
            }
            break;
        case 3:
            if(pJson.isNull())
            {
                err="The " + fieldName + " column cannot be null";
                return false;
            }
            if(!pJson.isString())
            {
                err="Type error in the "+fieldName+" field";
                return false;
            }
            break;
        default:
            err="Internal error in the server";
            return false;
    }
    return true;
}
