#pragma once

/*
** libsocket
** A LuaSocket C++ Implementation
**
** DISCLAIMER:
** This code is based on LuaSocket and (LÖVE's own modifications).
** It is somewhat an altered source variant of the original code,
** but as it is based on LuaSocket, please aknowledge the work done
** by the original authors of both LuaSocket and LöVE.
**
*/

#include "common/runtime.h"

#include "socket/objects/http/wrap_http.h"
#include "socket/objects/socket/wrap_socket.h"
#include "socket/objects/udp/wrap_udp.h"
#include "socket/objects/tcp/wrap_tcp.h"

#define SO_MAX_BUFSIZE 0x100000

namespace LuaSocket
{
    int Open(lua_State * L);

    int OpenSocket(lua_State * L);

    int OpenCore(lua_State * L);

    void NewClass(lua_State * L, const char * classname, ...);

    int ToString(lua_State * L);

    int Sleep(lua_State * L);

    int OpenHTTP(lua_State * L);
}