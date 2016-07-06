#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

//#define  SERVERIP  "64.137.228.223"
//const int  SERVERPORT = 19999;

#define		SERVERIP  "127.0.0.1"
const int	SERVERPORT = 30000;

const int  BUFFSIZE = 512;

void err_quit( char *msg )
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

// 소켓 함수 오류 출력
void err_display( char *msg )
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

int recvn( SOCKET s , char* buf , int len , int flags )
{
	int received;
	char* ptr = buf;
	int left = len;

	while(left > 0)
	{
		received = recv( s , ptr , left , flags );
		if(received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if(received == 0)
			break;

		left -= received;
		ptr += received;
	}

	return ( len - left );
}

int main( void )
{
	int retval;

	WSADATA wsa;
	if(WSAStartup( MAKEWORD( 2 , 2 ) , &wsa ) != 0)
		return 1;

	SOCKET sock = socket( AF_INET , SOCK_STREAM , 0 );
	if(sock == INVALID_SOCKET)
		err_quit( "socket()" );

	SOCKADDR_IN serveraddr;
	ZeroMemory( &serveraddr , sizeof( serveraddr ) );
	serveraddr.sin_family = AF_INET;

	auto ret = inet_pton( AF_INET , SERVERIP , ( void * )&serveraddr.sin_addr.s_addr );
	serveraddr.sin_port = htons( SERVERPORT );

	retval = connect( sock , ( SOCKADDR* )&serveraddr , sizeof( serveraddr ) );
	if(retval == SOCKET_ERROR)
	{
		err_quit( "connect()" );
		return 2;
	}

	char buf[ BUFFSIZE + 1 ];
	int len;

	while(1)
	{
		printf( "\n[보낼 데이터] " );
		if(fgets( buf , BUFSIZ + 1 , stdin ) == NULL)
			break;

		len = strlen( buf );
		if(buf[ len - 1 ] == '\n')
			buf[ len - 1 ] = '\0';
		if(strlen( buf ) == 0)
			break;

		retval = send( sock , buf , strlen( buf ) , 0 );
		if(retval == SOCKET_ERROR)
		{
			err_display( "send()" );
			break;
		}
		printf( "[TCP 클라이언트 %d 바이트를 보냈습니다.\n" , retval );

		retval = recvn( sock , buf , retval , 0 );
		if(retval == SOCKET_ERROR)
		{
			err_display( "recv()" );
			break;
		}
		else if(retval == 0)
			break;

		buf[ retval ] = '\0';
		printf( "[TCP 클라이언트] %d바이트를 받았습니다.\n" , retval );
		printf( "[받은 데이터] %s\n" , buf );
	}
	closesocket( sock );
	WSACleanup( );
	getchar( );
	return 0;
}