#include "Types.h"

// �Լ� ����
void kPrintString( int iX, int iY, const char* pcString );
void kDoubleMappingPrintString( int iX, int iY, const char* pcString );

/**
 *  �Ʒ� �Լ��� C ��� Ŀ���� ���� �κ���
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
 *  ���ڿ��� X, Y ��ġ�� ���
 */
void kPrintString( int iX, int iY, const char* pcString )
{
    CHARACTER* pstScreen = ( CHARACTER* ) 0xAB8000;
    int i;
    
    // X, Y ��ǥ�� �̿��ؼ� ���ڿ��� ����� ��巹���� ���
    pstScreen += ( iY * 80 ) + iX;

    // NULL�� ���� ������ ���ڿ� ���
    for( i = 0 ; pcString[ i ] != 0 ; i++ )
    {
        pstScreen[ i ].bCharactor = pcString[ i ];
    }
}

// 0xAB8000 ���� �޸𸮸� ���ؼ� ���ڿ� ���
void kDoubleMappingPrintString( int iX, int iY, const char* pcString )
{
    CHARACTER* pstScreen = ( CHARACTER* ) 0xAB8000;
    int i;
    
    // X, Y ��ǥ�� �̿��ؼ� ���ڿ��� ����� ��巹���� ���
    pstScreen += ( iY * 80 ) + iX;

    // NULL�� ���� ������ ���ڿ� ���
    for( i = 0 ; pcString[ i ] != 0 ; i++ )
    {
        pstScreen[ i ].bCharactor = pcString[ i ];
    }
}
