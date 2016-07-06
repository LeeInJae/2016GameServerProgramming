#include <algorithm>
#include "../../Common/ErrorCode.h"
#include "User.h"
#include "UserManager.h"

namespace NLogicLib
{
	UserManager::UserManager()
	{
	}

	UserManager::~UserManager()
	{
	}

	void UserManager::Init(const int MaxUserCount)
	{
		for (int i = 0; i < MaxUserCount; ++i)
		{
User user;
user.Init((short)i);

m_UserObjPool.push_back(user);
m_UserObjPoolIndex.push_back(i);
		}
	}

	User* UserManager::AllocUserObjPoolIndex()
	{
		if (m_UserObjPoolIndex.empty())
		{
			return nullptr;
		}

		int Index = m_UserObjPoolIndex.front();
		m_UserObjPoolIndex.pop_front();

		return &m_UserObjPool[Index];
	}

	void UserManager::ReleaseUserObjPoolIndex(const int Index)
	{
		m_UserObjPoolIndex.push_back(Index);
		m_UserObjPool[Index].Clear();
	}

	ERROR_CODE UserManager::AddUser(const int SessionIndex, const char* pszID)
	{
		if (FindUser(pszID) != nullptr)
		{
			return ERROR_CODE::USER_MGR_ID_DUPLICATION;
		}

		auto pUser = AllocUserObjPoolIndex();
		if (pUser == nullptr)
		{
			return ERROR_CODE::USER_MGR_MAX_USER_COUNT;
		}

		pUser->Set(SessionIndex, pszID);

		m_UserSessionDic.insert({ SessionIndex, pUser });
		m_UserIDDic.insert({ pszID, pUser });

		return ERROR_CODE::NONE;
	}

	ERROR_CODE UserManager::RemoveUser(const int SessionIndex)
	{
		auto pUser = FindUser(SessionIndex);

		if (pUser == nullptr)
		{
			return ERROR_CODE::USER_MGR_REMOVE_INVALID_SESSION;
		}

		auto Index = pUser->GetIndex();
		auto pszID = pUser->GetID();

		m_UserSessionDic.erase(SessionIndex);
		m_UserIDDic.erase(pszID.c_str());
		ReleaseUserObjPoolIndex(Index);

		return ERROR_CODE::NONE;
	}

	std::tuple<ERROR_CODE, User*> UserManager::GetUser(const int SessionIndex)
	{
		auto pUser = FindUser(SessionIndex);

		if (pUser == nullptr)
		{
			return{ std::tuple<ERROR_CODE, User*>(ERROR_CODE::USER_MGR_INVALID_SESSION_INDEX, nullptr) };
		}

		if (pUser->IsConfirm() == false)
		{
			return{ std::tuple<ERROR_CODE, User*>(ERROR_CODE::USER_MGR_NOT_CONFIRM_USER, nullptr) };
		}

		return{ std::tuple<ERROR_CODE, User*>(ERROR_CODE::NONE, pUser) };
	}

	User* UserManager::FindUser(const int SessionIndex)
	{
		auto findIter = m_UserSessionDic.find(SessionIndex);

		if (findIter == m_UserSessionDic.end())
		{
			return nullptr;
		}
		return (User*)findIter->second;
	}

	User* UserManager::FindUser(const char* pszID)
	{
		auto findIter = m_UserIDDic.find(pszID);

		if (findIter == m_UserIDDic.end())
		{
			return nullptr;
		}

		return (User*)findIter->second;
	}
}