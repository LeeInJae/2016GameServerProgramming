#pragma once
#include <unordered_map>
#include <deque>
#include <string>

namespace NCommon
{
	enum class ERROR_CODE : short;
}

using ERROR_CODE = NCommon::ERROR_CODE;

namespace NLogicLib
{
	class User;

	class UserManager
	{
	public:
		UserManager();
		virtual ~UserManager();

		void Init(const int MaxUserCount);

		ERROR_CODE AddUser(const int SessionIndex, const char* pszID);
		ERROR_CODE RemoveUser(const int SessionIndex);

		std::tuple<ERROR_CODE, User*> GetUser(const int SessionIndex);

	private:
		User* AllocUserObjPoolIndex();
		void ReleaseUserObjPoolIndex(const int Index);

		User* FindUser(const int SessionIndex);
		User* FindUser(const char* pszID);

	private:
		std::vector<User> m_UserObjPool;
		std::deque<int> m_UserObjPoolIndex;

		std::unordered_map<int, User*> m_UserSessionDic;
		std::unordered_map<const char*, User*> m_UserIDDic; //Char*는 키로 사용 못함
	};
}