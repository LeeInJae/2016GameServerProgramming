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

	//���ѽð����� ������������ �����ߴ��� ���ߴ���
	void Game::CheckSelectTime( )
	{
		auto CurTime = std::chrono::system_clock::now( );
		auto CurSecTime = std::chrono::system_clock::to_time_t( curTime );
		
		auto diff = CurSecTime - m_SelectTime;
		if(diff >= 60)
		{
			//�ð��� over�ߴ�? ���ѽð����� �������� ��ٸ��� ��û�� �ȿԴ�?��� �� üũ�ϴ°�
			return true;
		}

		return false;
	}
}