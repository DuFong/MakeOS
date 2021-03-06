#include "Types.h"

// 함수 선언
void kPrintString( int iX, int iY, const char* pcString );
void kDoubleMappingPrintString( int iX, int iY, const char* pcString );

/**
 *  아래 함수는 C 언어 커널의 시작 부분임
 */
void Main( void )
{
    kPrintString( 0, 11, "Switch To IA-32e Mode Success~!!" );
    kPrintString( 0, 12, "IA-32e C Language Kernel Start..............[Pass]" );

    kDoubleMappingPrintString( 0, 13, "This message is printed through the video memory relocated to 0xAB8000" );

    kPrintString(0, 14, "Read from 0x1fe000 [  ]");
    Readable(0x1FE000, 14);
    kPrintString(0, 15, "Write to 0x1fe000 [  ]");
    Writable(0x1FE000, 15);
    kPrintString(0, 16, "Read from 0x1ff000 [  ]");
    Readable(0x1FF000, 16);
    kPrintString(0, 17, "Write to 0x1ff000 [  ]");
 //   Writable(0x1FF000, 17);
}

// 해당 페이지 읽기 확인
void Readable(DWORD* address, int y){
    DWORD test = 0xFF00FF;
    test = *address;
    
    if(test != 0xFF00FF){
        kPrintString(20, y, "OK");
    }
    else {
        kPrintString(0, y, "                                             ");
    }
}

// 해당 페이지에 쓰기 확인
void Writable(DWORD* address, int y){
    DWORD test = 0xFF00FF;
    *address = test;
    
    if(*address == 0xFF00FF){
        kPrintString(19, y, "OK");
    }
    else {
        kPrintString(0, y, "                                             ");
    }
}

/**
 *  문자열을 X, Y 위치에 출력
 */
void kPrintString( int iX, int iY, const char* pcString )
{
    CHARACTER* pstScreen = ( CHARACTER* ) 0xAB8000;
    int i;
    
    pstScreen += ( iY * 80 ) + iX;

    for( i = 0 ; pcString[ i ] != 0 ; i++ )
    {
        pstScreen[ i ].bCharactor = pcString[ i ];
    }
}

// 0xAB8000 비디오 메모리를 통해서 문자열 출력
void kDoubleMappingPrintString( int iX, int iY, const char* pcString )
{
    CHARACTER* pstScreen = ( CHARACTER* ) 0xAB8000;
    int i;
    
    pstScreen += ( iY * 80 ) + iX;

    for( i = 0 ; pcString[ i ] != 0 ; i++ )
    {
        pstScreen[ i ].bCharactor = pcString[ i ];
    }
}
