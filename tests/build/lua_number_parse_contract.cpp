#include <cstdio>
#include <cstdlib>
#include <cstring>
extern "C" {
#include "../../src/Thorn/lua32/include/lua.h"
#include "../../src/Thorn/lua32/src/lobject.h"
void lua_error(char*) { std::abort(); }
}

// Original boundary with its waiting instruction, using the actual C parser.
__declspec(noinline) static int Legacy(char* text,int sign,TObject* object)
{
    const double parsed=luaO_str2d(text);
    if (!(parsed>=0)) return 2;
    ttype(object)=LUA_T_NUMBER;
    nvalue(object)=parsed*sign;
    __asm { fwait }
    return 0;
}
int main()
{
    const char* inputs[]={"0","1.25",".5","1e3","1e-3","1e309","0e309",
        "bad","-1","1x","1e","10 ","16777217","9007199254740993"};
    unsigned short original;
    __asm { fnstcw original }
    const unsigned short precisions[]={0,0x200,0x300};
    unsigned cases=0;
    for (unsigned short pc:precisions) for (unsigned short rc=0;rc<0x1000;rc+=0x400)
        for (const char* input:inputs) for (int sign=-1;sign<=1;++sign) {
        const unsigned short control=static_cast<unsigned short>((original&~0xf00)|pc|rc|0x3f);
        __asm { fldcw control }
        TObject old,now;std::memset(&old,0x5a,sizeof(old));std::memcpy(&now,&old,sizeof(old));
        const int a=Legacy(const_cast<char*>(input),sign,&old);
        const int b=LuaParseSignedNumberObject(const_cast<char*>(input),sign,&now);
        if (a!=b || std::memcmp(&old,&now,sizeof(old))!=0) {
            __asm { fnclex }
            __asm { fldcw original }
            std::fprintf(stderr,"parse mismatch: %s sign=%d pc=%x rc=%x\n",input,sign,pc,rc);
            return 1;
        }
        ++cases;
    }
    __asm { fnclex }
    __asm { fldcw original }
    std::printf("%u exact parser boundary cases\n",cases);
    return 0;
}
