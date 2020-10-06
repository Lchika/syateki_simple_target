/*
 * デバッグ用Serial出力
 * 参考:http://monakaice88.hatenablog.com/entry/20141227/1419634072
 */

#ifndef __DEBUG_H__
#define __DEBUG_H__

//  デバッグ出力ON/OFF用マクロ
#define DEBUG

#ifdef DEBUG
    #define BeginDebugPrint()    Serial.begin( 115200 )
    #define DebugPrint( msg )\
        {\
            char __buff__[ 256 ];\
            Serial.print( msg );\
            sprintf( __buff__\
                   , " (Func:%s)"\
                   ,__func__ );\
            Serial.println( __buff__ );\
            Serial.flush();\
        }
#else
    #define BeginDebugPrint()
    #define DebugPrint( ... )
#endif // DEBUG
#endif // __DEBUG_H__
