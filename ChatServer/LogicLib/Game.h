#pragma once

#include <chrono>

namespace NLogicLib
{
	enum class GameState
	{
		NONE,
		STARTING,//���ӽ���
		ING, //������
		END, //���ӳ�
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

		//TODO : Game ������ ��û�� ������� List�� ������ �־����

		__int64 m_SelectTime;
		int m_GameSelect1; //����(0), ����(1), ��(2)
		int m_GameSelect2; //����(0), ����(1), ��(2)

	};
}