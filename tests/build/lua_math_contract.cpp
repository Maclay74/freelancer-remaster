// Exercise the actual C math-library registration and callback functions without
// the interpreter, renderer or proprietary game data.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
extern "C" {
#include "../../src/Thorn/lua32/include/lua.h"
#include "../../src/Thorn/lua32/include/lauxlib.h"
#include "../../src/Thorn/lua32/include/lualib.h"
}

static luaL_reg* library;
static int libraryCount, checks[4], checkCount, pushes;
static double arguments[2], result;
extern "C" {
void luaL_openlib(luaL_reg* functions, int count) { library = functions; libraryCount = count; }
double luaL_check_number(int index) { checks[checkCount++] = index; return arguments[index-1]; }
double luaL_opt_number(int, double fallback) { return fallback; }
void luaL_argerror(int, char*) { std::abort(); }
void lua_pushnumber(double value) { result = value; ++pushes; }
void lua_pushcclosure(lua_CFunction, int) {}
lua_Object lua_lua2C(int) { return LUA_NOOBJECT; }
lua_Object lua_pop() { return LUA_NOOBJECT; }
int lua_tag(lua_Object) { return 1; }
lua_Object lua_settagmethod(int, char*) { return LUA_NOOBJECT; }
void lua_setglobal(char*) {}
}

int main()
{
    lua_mathlibopen();
    struct Case { const char* name; double x, y, expected; int arguments; };
    const Case cases[] = {{"sin",90,0,1,1}, {"cos",180,0,-1,1}, {"tan",45,0,1,1},
        {"atan",1,0,45,1}, {"atan2",1,0,90,2}, {"atan2",0,-1,180,2},
        {"sqrt",4,0,2,1}, {"log",1,0,0,1}, {"log10",100,0,2,1}, {"exp",0,0,1,1}};
    for (const auto& sample : cases) {
        arguments[0] = sample.x; arguments[1] = sample.y;
        checkCount = pushes = 0;
        bool found = false;
        for (int index = 0; index < libraryCount; ++index)
            if (std::strcmp(library[index].name,sample.name) == 0) {
                library[index].func(); found = true; break;
            }
        if (!found || pushes != 1 || checkCount != sample.arguments || checks[0] != 1 ||
            (sample.arguments == 2 && checks[1] != 2) ||
            !(std::fabs(result-sample.expected) <= 1e-12)) {
            std::fprintf(stderr,"Lua math callback failed: %s => %.17g\n",sample.name,result);
            return 1;
        }
    }
    return 0;
}
