#include "Game.h"

namespace NLogicLib
{
	Game::Game( )
	{

	}
	Game::~Game( )
	{

	}

	void Game::Clear( )
	{

	}

	//제한시간내에 가위바위보를 선태했는지 안했는지
	void Game::CheckSelectTime( )
	{
		auto CurTime = std::chrono::system_clock::now( );
		auto CurSecTime = std::chrono::system_clock::to_time_t( curTime );
		
		auto diff = CurSecTime - m_SelectTime;
		if(diff >= 60)
		{
			//시간을 over했니? 제한시간내에 서버에서 기다리는 요청이 안왔니?라는 걸 체크하는것
			return true;
		}

		return false;
	}
}