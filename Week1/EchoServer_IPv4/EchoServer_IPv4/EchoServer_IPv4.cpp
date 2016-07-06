#pragma comment( lib, "ws2_32")
#include <winsock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <stdlib.h>

#define SERVERPORT	30000
#define BUFSIZE		512		

void err_quit( char* msg )
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM ,
		NULL , WSAGetLastError( ) ,
		MAKELANGID( LANG_NEUTRAL , SUBLANG_DEFAULT ) ,
		( LPTSTR )&lpMsgBuf , 0 , NULL );

	MessageBox( NULL , ( LPCTSTR )lpMsgBuf , msg , MB_ICONERROR );
	LocalFree( lpMsgBuf );
	exit( 1 );
}

void err_display( char* msg )
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM ,
		NULL , WSAGetLastError( ) ,
		MAKELANGID( LANG_NEUTRAL , SUBLANG_DEFAULT ) ,
		( LPTSTR )&lpMsgBuf , 0 , NULL );
	printf( "[%s] %s" , msg , ( char * )lpMsgBuf );
	LocalFree( lpMsgBuf );
}


int main( void )
{
	int retval;

	WSADATA wsa;
	if(WSAStartup( MAKEWORD( 2 , 2 ) , &wsa ) != 0)
	{
		return 1;
	}

	SOCKET listen_socket = socket( AF_INET , SOCK_STREAM , IPPROTO_TCP );
	if(listen_socket == INVALID_SOCKET)
	{
		err_quit( "socket()" );
	}

	SOCKADDR_IN serveraddr;
	ZeroMemory( &serveraddr , sizeof( serveraddr ) );
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl( INADDR_ANY );
	serveraddr.sin_port = htons( SERVERPORT );
	retval = bind( listen_socket , ( SOCKADDR* )&serveraddr , sizeof( serveraddr ) );
	if(retval == SOCKET_ERROR)
		err_quit( "bind()" );

	retval = listen( listen_socket , SOMAXCONN );
	if(retval == SOCKET_ERROR)
		err_quit( "listen()" );

	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	char buf[ BUFSIZE + 1 ];

	while(1)
	{
		addrlen = sizeof( clientaddr );
		client_sock = accept( listen_socket , ( SOCKADDR* )&clientaddr , &addrlen );
		if(client_sock == INVALID_SOCKET)
		{
			err_display( "accept()" );
			break;
		}

		char clientIP[ 32 ] = { 0, };
		inet_ntop( AF_INET , &( clientaddr.sin_addr ) , clientIP , 32 - 1 );
		printf( "\n[TCP 서버] 클라이언트 접속 : IP 주소 = %s, 포트번호 = %d\n" , clientIP , ntohs( clientaddr.sin_port ) );

		while(1)
		{
			retval = recv( client_sock , buf , BUFSIZE , 0 );
			if(retval == SOCKET_ERROR)
			{
				err_display( "recv()" );
				break;
			}
			else if(retval == 0)
				break;

			buf[ retval ] = '\0';
			printf( "[TCP/%s:%d] %s\n" , clientIP , ntohs( clientaddr.sin_port ) , buf );

			retval = send( client_sock , buf , retval , 0 );
			if(retval == SOCKET_ERROR)
			{
				err_display( "send()" );
				break;
			}
		}

		closesocket( client_sock );
		printf( "[TCP 서버] 클라이언트 종료: IP 주소 = %s, 포트 번호 = %d\n" , clientIP , ntohs( clientaddr.sin_port ) );
	}

	closesocket( listen_socket );
	WSACleanup();

	getchar( );
	return 0;
}