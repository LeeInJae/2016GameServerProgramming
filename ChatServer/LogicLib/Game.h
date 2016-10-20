#pragma once

#include <chrono>

namespace NLogicLib
{
	enum class GameState
	{
		NONE,
		STARTING,//게임시작
		ING, //게임중
		END, //게임끝
	};

	class Game
	{
	public:
		Game( );
		virtual ~Game( );

		void Clear( );

		GameState GetState( ) { return m_State; }

		void SetState( const GameState state ) { m_State = state; }

		void CheckSelectTime( );
	private:
		GameState m_State = GameState::NONE;

		//TODO : Game 시작을 요청한 사람들의 List를 가지고 있어야함

		__int64 m_SelectTime;
		int m_GameSelect1; //가위(0), 바위(1), 보(2)
		int m_GameSelect2; //가위(0), 바위(1), 보(2)

	};
}