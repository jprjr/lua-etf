/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2023 John Regan <john@jrjrtech.com> */
/* Full license details available at end of file. */

#define ETF_VERSION_MAJOR 1
#define ETF_VERSION_MINOR 0
#define ETF_VERSION_PATCH 1
#define STR(x) #x
#define XSTR(x) STR(x)
#define ETF_VERSION XSTR(ETF_VERSION_MAJOR) "." XSTR(ETF_VERSION_MINOR) "." XSTR(ETF_VERSION_PATCH)

#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(_MSC_VER)
  #define ETF_PUBLIC __declspec(dllexport)
#else
  #if defined(__GNUC__) && __GNUC__ >= 4
    #define ETF_PUBLIC __attribute__((visibility("default")))
  #else
    #define ETF_PUBLIC
  #endif
#endif

#if defined(__GNUC__) && __GNUC__ >= 4
#define ETF_PRIVATE __attribute__((unused)) static
#else
#define ETF_PRIVATE static
#endif


/* defines for version 131 tag values */

#define ETF_DEFAULT_VERSION 131

#define ETF_NO_COMPRESSION -2

#define _131_NEW_FLOAT_EXT 70
#define _131_BIT_BINARY_EXT 77
#define _131_ETFZLIB 80
#define _131_ATOM_CACHE_REF 82
#define _131_NEW_PID_EXT 88
#define _131_NEW_PORT_EXT 89
#define _131_NEWER_REFERENCE_EXT 90
#define _131_SMALL_INTEGER_EXT 97
#define _131_INTEGER_EXT 98
#define _131_FLOAT_EXT 99
#define _131_ATOM_EXT 100
#define _131_REFERENCE_EXT 101
#define _131_PORT_EXT 102
#define _131_PID_EXT 103
#define _131_SMALL_TUPLE_EXT 104
#define _131_LARGE_TUPLE_EXT 105
#define _131_NIL_EXT 106
#define _131_STRING_EXT 107
#define _131_LIST_EXT 108
#define _131_BINARY_EXT 109
#define _131_SMALL_BIG_EXT 110
#define _131_LARGE_BIG_EXT 111
#define _131_NEW_FUN_EXT 112
#define _131_EXPORT_EXT 113
#define _131_NEW_REFERENCE_EXT 114
#define _131_SMALL_ATOM_EXT 115
#define _131_MAP_EXT 116
#define _131_FUN_EXT 117
#define _131_ATOM_UTF8_EXT 118
#define _131_SMALL_ATOM_UTF8_EXT 119
#define _131_V4_PORT_EXT 120

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>
#include <lauxlib.h>
#include <stddef.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#if !defined(LUA_VERSION_NUM) || LUA_VERSION_NUM < 502
#include <math.h>
#endif

#ifdef __cplusplus
}
#endif

#define BIGINT_API ETF_PRIVATE
#define BIGINT_IMPLEMENTATION

#if defined(__LP64__) || defined(_WIN64) || (defined(__x86_64__) && !defined(__ILP32__)) || defined(_M_X64) || defined(__ia64) || defined(_M_IA64) || defined(__aarch64__) || defined(_M_ARM64) || defined(__powerpc64__)
#define BIGINT_WORD_WIDTH 8
#else
#define BIGINT_WORD_WIDTH 4
#endif

#include "thirdparty/bigint/bigint.h"

#define MINIZ_NO_STDIO
#define MINIZ_NO_TIME
#define MINIZ_NO_ARCHIVE_APIS
#define MINIZ_NO_ARCHIVE_WRITING_APIS
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#define MINIZ_EXPORT ETF_PRIVATE
#include "thirdparty/miniz/miniz.h"

#define ETF_BUFFER_LEN 4096

#if !defined(LUA_VERSION_NUM) || LUA_VERSION_NUM < 501
typedef struct luaL_Reg {
    const char *name;
    lua_CFunction func;
} luaL_Reg;

typedef int64_t lua_Integer;

static void lua_pushinteger(lua_State *L, lua_Integer val) {
    lua_pushnumber(L,(lua_Number)val);
}

static lua_Integer lua_tointeger(lua_State *L, int index) {
    return (lua_Integer)lua_tonumber(L,index);
}

static void lua_getfield(lua_State *L, int index, const char *name) {
    if(index < 0) index--;
    lua_pushstring(L,name);
    lua_gettable(L,index);
}

static void lua_setfield(lua_State *L, int index, const char *name) {
    lua_pushstring(L,name);
    lua_pushvalue(L,-2);
    /* stack is now value, key, value */
    if(index < 0) index -= 2;
    lua_settable(L,index);
    lua_pop(L,1);
}

static const char * lua_tolstring(lua_State *L, int index, size_t *len) {
    if(len != NULL) *len = lua_strlen(L,index);
    return lua_tostring(L,index);
}

static void lua_createtable(lua_State *L, int narr, int nrec) {
    (void)narr;
    (void)nrec;
    lua_newtable(L);
}

#define luaL_addchar luaL_putchar
#define lua_objlen luaL_getn
#endif

#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM < 502
#define lua_rawlen lua_objlen
#define lua_setuservalue lua_setfenv
#define lua_getuservalue lua_getfenv
#endif

#if !defined(LUA_VERSION_NUM) || LUA_VERSION_NUM < 503
static int lua_isinteger(lua_State *L, int index) {
    /* calling lua_tonumber actually converts the
     * value */
    lua_Number n;
    lua_Integer i;
    int type;

    type = lua_type(L,index);
    if(type != LUA_TNUMBER) return 0;

    lua_pushvalue(L,index);
    n = lua_tonumber(L,-1);
    lua_pop(L,1);

    i = (lua_Integer)n;

    /* TODO - this values like 1.0 will appear
     * as an integer */

    return n == (lua_Number) i;
}
#endif

#if !defined(luaL_newlibtable) \
  && (!defined LUA_VERSION_NUM || LUA_VERSION_NUM==501)
static void luaL_setfuncs (lua_State *L, const luaL_Reg *l, int nup) {
    luaL_checkstack(L, nup+1, "too many upvalues");
    for (; l->name != NULL; l++) {  /* fill the table with given functions */
        int i;
        lua_pushlstring(L, l->name,strlen(l->name));
        for (i = 0; i < nup; i++)  /* copy upvalues to the top */
            lua_pushvalue(L, -(nup+1));
        lua_pushcclosure(L, l->func, nup);  /* closure with those upvalues */
        lua_settable(L, -(nup + 3));
    }
    lua_pop(L, nup);  /* remove upvalues */
}

static
void luaL_setmetatable(lua_State *L, const char *str) {
    luaL_checkstack(L, 1, "not enough stack slots");
    luaL_getmetatable(L, str);
    lua_setmetatable(L, -2);
}

static
void *luaL_testudata (lua_State *L, int i, const char *tname) {
    void *p = lua_touserdata(L, i);
    luaL_checkstack(L, 2, "not enough stack slots");
    if (p == NULL || !lua_getmetatable(L, i)) {
        return NULL;
    }
    else {
        int res = 0;
        luaL_getmetatable(L, tname);
        res = lua_rawequal(L, -1, -2);
        lua_pop(L, 2);
        if (!res) {
            p = NULL;
        }
    }
    return p;
}
#endif

static const char * const etf_integer_mt      = "etf.integer";
static const char * const etf_float_mt        = "etf.float";
static const char * const etf_port_mt         = "etf.port";
static const char * const etf_pid_mt          = "etf.pid";
static const char * const etf_export_mt       = "etf.export";
static const char * const etf_new_fun_mt      = "etf.new_fun";
static const char * const etf_fun_mt          = "etf.fun";
static const char * const etf_reference_mt    = "etf.reference";
static const char * const etf_tuple_mt        = "etf.tuple";
static const char * const etf_list_mt         = "etf.list";
static const char * const etf_map_mt          = "etf.map";
static const char * const etf_atom_mt         = "etf.atom";
static const char * const etf_string_mt       = "etf.string";
static const char * const etf_binary_mt       = "etf.binary";

static const char * const etf_131_decoder_mt  = "etf.decoder.131";
static const char * const etf_131_encoder_mt  = "etf.encoder.131";


typedef struct etf_131_decoder_state_s {
    lua_State *L;
    const uint8_t *data;
    bigint *b_min;
    bigint *b_max;
    int64_t i_min;
    int64_t i_max;
    size_t len;
    uint8_t tag;
    uint8_t key; /* set to 1 if we're decoding a map key */
    uint8_t force_bigint; /* set to 1 if we're forcing all ints to bigints */
    uint8_t force_float; /* set to 1 if we're forcing all floats to etf.floats */
    mz_stream strm;
    size_t offset;
    uint8_t z[ETF_BUFFER_LEN];
    int (*read)(struct etf_131_decoder_state_s *, uint8_t *data, size_t len);
} etf_131_decoder_state;

typedef struct etf_131_encoder_state_s {
    lua_State *L;
    int strtable;
    size_t strcount;
    uint8_t key; /* set to 1 if we're encoding a map key */
    mz_stream strm;
    uint8_t z[ETF_BUFFER_LEN];
    int (*write)(struct etf_131_encoder_state_s *, const uint8_t *data, size_t len);
} etf_131_encoder_state;

static int etf_131_decode(etf_131_decoder_state *D);
static int etf_131_encode(etf_131_encoder_state *E);

static inline
uint64_t unpack_uint64le(const uint8_t *b) {
    return (((uint64_t)b[7])<<56) |
           (((uint64_t)b[6])<<48) |
           (((uint64_t)b[5])<<40) |
           (((uint64_t)b[4])<<32) |
           (((uint64_t)b[3])<<24) |
           (((uint64_t)b[2])<<16) |
           (((uint64_t)b[1])<<8 ) |
           (((uint64_t)b[0])    );
}

static inline
int64_t unpack_int64le(const uint8_t *b) {
    return
      (int64_t)(((int64_t)((  int8_t)b[7])) << 56) |
               (((uint64_t)b[6]) << 48) |
               (((uint64_t)b[5]) << 40) |
               (((uint64_t)b[4]) << 32) |
               (((uint64_t)b[3]) << 24) |
               (((uint64_t)b[2]) << 16) |
               (((uint64_t)b[1]) << 8 ) |
               (((uint64_t)b[0])      );
}

static inline
uint32_t unpack_uint32le(const uint8_t *b) {
    return (((uint32_t)b[3])<<24) |
           (((uint32_t)b[2])<<16) |
           (((uint32_t)b[1])<<8 ) |
           (((uint32_t)b[0])    );
}

static inline
int32_t unpack_int32le(const uint8_t *b) {
    int32_t r;
    r  = (int32_t) ((int8_t)b[3])   << 24;
    r |= (int32_t) ((uint32_t)b[2]) << 16;
    r |= (int32_t) ((uint32_t)b[1]) <<  8;
    r |= (int32_t) ((uint32_t)b[0])      ;
    return r;

}

static inline
uint16_t unpack_uint16le(const uint8_t *b) {
    return (((uint16_t)b[1])<<8) |
           (((uint16_t)b[0])   );
}

static inline
int16_t unpack_int16le(const uint8_t *b) {
    return
      (int16_t)(((  int8_t)b[1]) << 8 ) |
               (((uint16_t)b[0])      );
}

static inline
uint64_t unpack_uint64be(const uint8_t *b) {
    return (((uint64_t)b[0])<<56) |
           (((uint64_t)b[1])<<48) |
           (((uint64_t)b[2])<<40) |
           (((uint64_t)b[3])<<32) |
           (((uint64_t)b[4])<<24) |
           (((uint64_t)b[5])<<16) |
           (((uint64_t)b[6])<<8 ) |
           (((uint64_t)b[7])    );
}

static inline
int64_t unpack_int64be(const uint8_t *b) {
    int64_t r;
    r  = ((int64_t)((  int8_t)b[0])) << 56;
    r |= ((uint64_t)b[1]) << 48;
    r |= ((uint64_t)b[2]) << 40;
    r |= ((uint64_t)b[3]) << 32;
    r |= ((uint64_t)b[4]) << 24;
    r |= ((uint64_t)b[5]) << 16;
    r |= ((uint64_t)b[6]) <<  8;
    r |= ((uint64_t)b[7])      ;
    return r;

}

static inline
uint32_t unpack_uint32be(const uint8_t *b) {
    return (((uint32_t)b[0])<<24) |
           (((uint32_t)b[1])<<16) |
           (((uint32_t)b[2])<<8 ) |
           (((uint32_t)b[3])    );
}

static inline
int32_t unpack_int32be(const uint8_t *b) {
    int32_t r;
    r = (int32_t)((int8_t)b[0]) << 24;
    r |= ((uint32_t)b[1]) << 16;
    r |= ((uint32_t)b[2]) <<  8;
    r |= ((uint32_t)b[3])      ;
    return r;
}

static inline
uint16_t unpack_uint16be(const uint8_t *b) {
    return (((uint16_t)b[0])<<8) |
           (((uint16_t)b[1])   );
}

static inline
int16_t unpack_int16be(const uint8_t *b) {
    return
      (int16_t)(((  int8_t)b[0]) << 8 ) |
               (((uint16_t)b[1])      );
}

static inline
uint8_t pack_uint64le(uint8_t *d, uint64_t n) {
    if(d != NULL) {
        d[0] = (uint8_t)(n       );
        d[1] = (uint8_t)(n >> 8  );
        d[2] = (uint8_t)(n >> 16 );
        d[3] = (uint8_t)(n >> 24 );
        d[4] = (uint8_t)(n >> 32 );
        d[5] = (uint8_t)(n >> 40 );
        d[6] = (uint8_t)(n >> 48 );
        d[7] = (uint8_t)(n >> 56 );
    }
    return 8;
}

static inline
uint8_t pack_int64le(uint8_t *d, int64_t n) {
    if(d != NULL) {
        d[0] = (uint8_t)(n       );
        d[1] = (uint8_t)(n >> 8  );
        d[2] = (uint8_t)(n >> 16 );
        d[3] = (uint8_t)(n >> 24 );
        d[4] = (uint8_t)(n >> 32 );
        d[5] = (uint8_t)(n >> 40 );
        d[6] = (uint8_t)(n >> 48 );
        d[7] = (uint8_t)(n >> 56 );
    }
    return 8;
}

static inline
uint8_t pack_uint32le(uint8_t *d, uint32_t n) {
    if(d != NULL) {
        d[0] = (uint8_t)(n       );
        d[1] = (uint8_t)(n >> 8  );
        d[2] = (uint8_t)(n >> 16 );
        d[3] = (uint8_t)(n >> 24 );
    }
    return 4;
}

static inline
uint8_t pack_int32le(uint8_t *d, int32_t n) {
    if(d != NULL) {
        d[0] = (uint8_t)(n       );
        d[1] = (uint8_t)(n >> 8  );
        d[2] = (uint8_t)(n >> 16 );
        d[3] = (uint8_t)(n >> 24 );
    }
    return 4;
}

static inline
uint8_t pack_uint16le(uint8_t *d, uint16_t n) {
    if(d != NULL) {
        d[0] = (uint8_t)(n       );
        d[1] = (uint8_t)(n >> 8  );
    }
    return 2;
}

static inline
uint8_t pack_int16le(uint8_t *d, int16_t n) {
    if(d != NULL) {
        d[0] = (uint8_t)(n       );
        d[1] = (uint8_t)(n >> 8  );
    }
    return 2;
}

static inline
uint8_t pack_uint64be(uint8_t *d, uint64_t n) {
    if(d != NULL) {
        d[0] = (uint8_t)(n >> 56 );
        d[1] = (uint8_t)(n >> 48 );
        d[2] = (uint8_t)(n >> 40 );
        d[3] = (uint8_t)(n >> 32 );
        d[4] = (uint8_t)(n >> 24 );
        d[5] = (uint8_t)(n >> 16 );
        d[6] = (uint8_t)(n >> 8  );
        d[7] = (uint8_t)(n       );
    }
    return 8;
}

static inline
uint8_t pack_int64be(uint8_t *d, int64_t n) {
    if(d != NULL) {
        d[0] = (uint8_t)(n >> 56 );
        d[1] = (uint8_t)(n >> 48 );
        d[2] = (uint8_t)(n >> 40 );
        d[3] = (uint8_t)(n >> 32 );
        d[4] = (uint8_t)(n >> 24 );
        d[5] = (uint8_t)(n >> 16 );
        d[6] = (uint8_t)(n >> 8  );
        d[7] = (uint8_t)(n       );
    }
    return 8;
}

static inline
uint8_t pack_uint32be(uint8_t *d, uint32_t n) {
    if(d != NULL) {
        d[0] = (uint8_t)(n >> 24 );
        d[1] = (uint8_t)(n >> 16 );
        d[2] = (uint8_t)(n >> 8  );
        d[3] = (uint8_t)(n       );
    }
    return 4;
}

static inline
uint8_t pack_int32be(uint8_t *d, int32_t n) {
    if(d != NULL) {
        d[0] = (uint8_t)(n >> 24 );
        d[1] = (uint8_t)(n >> 16 );
        d[2] = (uint8_t)(n >> 8  );
        d[3] = (uint8_t)(n       );
    }
    return 4;
}

static inline
uint8_t pack_uint16be(uint8_t *d, uint16_t n) {
    if(d != NULL) {
        d[0] = (uint8_t)(n >> 8  );
        d[1] = (uint8_t)(n       );
    }
    return 2;
}

static inline
uint8_t pack_int16be(uint8_t *d, int16_t n) {
    if(d != NULL) {
        d[0] = (uint8_t)(n >> 8  );
        d[1] = (uint8_t)(n       );
    }
    return 2;
}

static const uint8_t _131_atom_nil[5] = {
    _131_SMALL_ATOM_UTF8_EXT, 3, 'n', 'i', 'l'
};

static const uint8_t _131_atom_true[6] = {
    _131_SMALL_ATOM_UTF8_EXT, 4, 't', 'r', 'u', 'e'
};

static const uint8_t _131_atom_false[7] = {
    _131_SMALL_ATOM_UTF8_EXT, 5, 'f', 'a', 'l', 's', 'e'
};

static inline bigint *
etf_isbigint(lua_State *L, int idx) {
    return (bigint *)luaL_testudata(L,idx,etf_integer_mt);
}

static int
etf_tobigint(lua_State *L, int idx, bigint* dest) {
    const char *str = NULL;
    size_t len = 0;

    switch(lua_type(L,idx)) {
        case LUA_TNONE: return 0;
        case LUA_TNIL: return 0;
        case LUA_TBOOLEAN: return bigint_from_u8(dest,(uint8_t)lua_toboolean(L,idx));
        case LUA_TNUMBER: return bigint_from_i64(dest, (int64_t)lua_tointeger(L,idx));
        default: break;
    }

    str = lua_tolstring(L,idx,&len);
    if(str == NULL) {
        return 1;
    }
    return bigint_from_string(dest,str,len,0);
}

static bigint*
etf_pushbigint(lua_State *L) {
    bigint *r = NULL;

    r = (bigint *)lua_newuserdata(L,sizeof(bigint));
    if(r == NULL) {
        luaL_error(L,"out of memory");
        return NULL;
    }
    luaL_setmetatable(L,etf_integer_mt);
    bigint_init(r);

    return r;
}

static inline void
etf_pushu64(etf_131_decoder_state *D, uint64_t val) {
    bigint *b = NULL;
    if(val > (uint64_t)D->i_max) {
        b = etf_pushbigint(D->L);
        if(bigint_from_u64(b,val)) luaL_error(D->L,"out of memory");
    } else {
        lua_pushinteger(D->L,val);
    }
}

static inline void
etf_pushu32(etf_131_decoder_state *D, uint32_t val) {
    bigint *b = NULL;
    if(val > D->i_max) {
        b = etf_pushbigint(D->L);
        if(bigint_from_u32(b,val)) luaL_error(D->L,"out of memory");
    } else {
        lua_pushinteger(D->L,val);
    }
}

static inline void
etf_pushi32(etf_131_decoder_state *D, int32_t val) {
    bigint *b = NULL;
    if(val < D->i_min || val > D->i_max) {
        b = etf_pushbigint(D->L);
        if(bigint_from_i32(b,val)) luaL_error(D->L,"out of memory");
    } else {
        lua_pushinteger(D->L,val);
    }
}

static inline uint32_t
etf_tou32(lua_State *L, int index) {
    uint32_t val = 0;
    bigint *b = NULL;

    if( (b = etf_isbigint(L,index)) != NULL) {
        bigint_to_u32(&val,b);
        return val;
    }

    return (uint32_t)lua_tointeger(L,index);
}

static inline uint64_t
etf_tou64(lua_State *L, int index) {
    uint64_t val = 0;
    bigint *b = NULL;

    if( (b = etf_isbigint(L,index)) != NULL) {
        bigint_to_u64(&val,b);
        return val;
    }

    return (uint64_t)lua_tointeger(L,index);
}

static int
etf_float(lua_State *L) {
    int type = lua_type(L,1);

    lua_newtable(L);

    if(type == LUA_TNONE || type == LUA_TNIL) {
        lua_pushnumber(L,0.0);
    } else if(type == LUA_TNUMBER) {
        lua_pushvalue(L,1);
    } else {
        lua_pushvalue(L, lua_upvalueindex(1));
        lua_pushvalue(L,1);
        lua_call(L,1,1);
        if(lua_type(L,-1) != LUA_TNUMBER) return luaL_error(L,"unacceptable value for float");
    }
    lua_setfield(L,-2,"float");
    luaL_setmetatable(L,etf_float_mt);
    return 1;
}

static inline int etf_isfloat(lua_State *L, int idx) {
    int res = 0;

    if(lua_getmetatable(L,idx) == 0) return 0;
    luaL_getmetatable(L,etf_float_mt);
    res = lua_rawequal(L,-1,-2);
    lua_pop(L,2);

    return res;
}

static inline void etf_getfloat(lua_State *L, int idx) {
    if(etf_isfloat(L,idx)) {
        lua_getfield(L,idx,"float");
    } else {
        lua_pushvalue(L,idx);
    }
}

static int
etf_float__tostring(lua_State *L) {
    lua_pushvalue(L,lua_upvalueindex(1));
    lua_getfield(L,1,"float");
    lua_call(L,1,1);
    return 1;
}

static int
etf_float__concat(lua_State *L) {
    lua_pushvalue(L,lua_upvalueindex(1));
    etf_getfloat(L,1);
    lua_call(L,1,1);

    lua_pushvalue(L,lua_upvalueindex(1));
    etf_getfloat(L,2);
    lua_call(L,1,1);

    lua_concat(L,2);
    return 1;
}

static int
etf_float__add(lua_State *L) {
    lua_newtable(L);

    etf_getfloat(L,1);
    etf_getfloat(L,2);

#if !defined(LUA_VERSION_NUM) || LUA_VERSION_NUM < 502
    lua_pushnumber(L, lua_tonumber(L,-2) + lua_tonumber(L,-1));
    lua_insert(L, lua_gettop(L) - 2);
    lua_pop(L,2);
#else
    lua_arith(L, LUA_OPADD);
#endif

    lua_setfield(L,-2,"float");
    luaL_setmetatable(L,etf_float_mt);

    return 1;
}

static int
etf_float__sub(lua_State *L) {
    lua_newtable(L);

    etf_getfloat(L,1);
    etf_getfloat(L,2);

#if !defined(LUA_VERSION_NUM) || LUA_VERSION_NUM < 502
    lua_pushnumber(L, lua_tonumber(L,-2) - lua_tonumber(L,-1));
    lua_insert(L, lua_gettop(L) - 2);
    lua_pop(L,2);
#else
    lua_arith(L, LUA_OPSUB);
#endif

    lua_setfield(L,-2,"float");
    luaL_setmetatable(L,etf_float_mt);

    return 1;
}

static int
etf_float__mul(lua_State *L) {
    lua_newtable(L);

    etf_getfloat(L,1);
    etf_getfloat(L,2);

#if !defined(LUA_VERSION_NUM) || LUA_VERSION_NUM < 502
    lua_pushnumber(L, lua_tonumber(L,-2) * lua_tonumber(L,-1));
    lua_insert(L, lua_gettop(L) - 2);
    lua_pop(L,2);
#else
    lua_arith(L, LUA_OPMUL);
#endif

    lua_setfield(L,-2,"float");
    luaL_setmetatable(L,etf_float_mt);

    return 1;
}

static int
etf_float__div(lua_State *L) {
    lua_newtable(L);

    etf_getfloat(L,1);
    etf_getfloat(L,2);

#if !defined(LUA_VERSION_NUM) || LUA_VERSION_NUM < 502
    lua_pushnumber(L, lua_tonumber(L,-2) / lua_tonumber(L,-1));
    lua_insert(L, lua_gettop(L) - 2);
    lua_pop(L,2);
#else
    lua_arith(L, LUA_OPDIV);
#endif

    lua_setfield(L,-2,"float");
    luaL_setmetatable(L,etf_float_mt);

    return 1;
}

static int
etf_float__mod(lua_State *L) {
    lua_newtable(L);

    etf_getfloat(L,1);
    etf_getfloat(L,2);

#if !defined(LUA_VERSION_NUM) || LUA_VERSION_NUM < 502
    lua_pushnumber(L, fmod(lua_tonumber(L,-2),lua_tonumber(L,-1)));
    lua_insert(L, lua_gettop(L) - 2);
    lua_pop(L,2);
#else
    lua_arith(L, LUA_OPMOD);
#endif

    lua_setfield(L,-2,"float");
    luaL_setmetatable(L,etf_float_mt);

    return 1;
}

static int
etf_float__pow(lua_State *L) {
    lua_newtable(L);

    etf_getfloat(L,1);
    etf_getfloat(L,2);

#if !defined(LUA_VERSION_NUM) || LUA_VERSION_NUM < 502
    lua_pushnumber(L, pow(lua_tonumber(L,-2),lua_tonumber(L,-1)));
    lua_insert(L, lua_gettop(L) - 2);
    lua_pop(L,2);
#else
    lua_arith(L, LUA_OPPOW);
#endif

    lua_setfield(L,-2,"float");
    luaL_setmetatable(L,etf_float_mt);

    return 1;
}

static int
etf_float__unm(lua_State *L) {
    lua_newtable(L);

    etf_getfloat(L,1);

#if !defined(LUA_VERSION_NUM) || LUA_VERSION_NUM < 502
    lua_pushnumber(L, -(lua_tonumber(L,-1)));
    lua_insert(L, lua_gettop(L) - 1);
    lua_pop(L,1);
#else
    lua_arith(L, LUA_OPUNM);
#endif

    lua_setfield(L,-2,"float");
    luaL_setmetatable(L,etf_float_mt);

    return 1;
}

#if LUA_VERSION_NUM >= 503
static int
etf_float__idiv(lua_State *L) {
    lua_newtable(L);

    etf_getfloat(L,1);
    etf_getfloat(L,2);

    lua_arith(L, LUA_OPIDIV);

    lua_setfield(L,-2,"float");
    luaL_setmetatable(L,etf_float_mt);

    return 1;
}

static int
etf_float__bnot(lua_State *L) {
    lua_newtable(L);

    etf_getfloat(L,1);

    lua_arith(L, LUA_OPBNOT);

    lua_setfield(L,-2,"float");
    luaL_setmetatable(L,etf_float_mt);

    return 1;
}

static int
etf_float__band(lua_State *L) {
    lua_newtable(L);

    etf_getfloat(L,1);
    etf_getfloat(L,2);

    lua_arith(L, LUA_OPBAND);

    lua_setfield(L,-2,"float");
    luaL_setmetatable(L,etf_float_mt);

    return 1;
}

static int
etf_float__bor(lua_State *L) {
    lua_newtable(L);

    etf_getfloat(L,1);
    etf_getfloat(L,2);

    lua_arith(L, LUA_OPBOR);

    lua_setfield(L,-2,"float");
    luaL_setmetatable(L,etf_float_mt);

    return 1;
}

static int
etf_float__bxor(lua_State *L) {
    lua_newtable(L);

    etf_getfloat(L,1);
    etf_getfloat(L,2);

    lua_arith(L, LUA_OPBXOR);

    lua_setfield(L,-2,"float");
    luaL_setmetatable(L,etf_float_mt);

    return 1;
}

static int
etf_float__shl(lua_State *L) {
    lua_newtable(L);

    etf_getfloat(L,1);
    etf_getfloat(L,2);

    lua_arith(L, LUA_OPSHL);

    lua_setfield(L,-2,"float");
    luaL_setmetatable(L,etf_float_mt);

    return 1;
}

static int
etf_float__shr(lua_State *L) {
    lua_newtable(L);

    etf_getfloat(L,1);
    etf_getfloat(L,2);

    lua_arith(L, LUA_OPSHR);

    lua_setfield(L,-2,"float");
    luaL_setmetatable(L,etf_float_mt);

    return 1;
}
#endif

static int
etf_float__eq(lua_State *L) {
    lua_newtable(L);

    etf_getfloat(L,1);
    etf_getfloat(L,2);

#if !defined(LUA_VERSION_NUM) || LUA_VERSION_NUM < 502
    lua_pushboolean(L, lua_equal(L, -2, -1));
#else
    lua_pushboolean(L, lua_compare(L, -2, -1, LUA_OPEQ));
#endif

    return 1;
}

static int
etf_float__lt(lua_State *L) {
    lua_newtable(L);

    etf_getfloat(L,1);
    etf_getfloat(L,2);

#if !defined(LUA_VERSION_NUM) || LUA_VERSION_NUM < 502
    lua_pushboolean(L, lua_lessthan(L, -2, -1));
#else
    lua_pushboolean(L, lua_compare(L, -2, -1, LUA_OPLT));
#endif

    return 1;
}

static int
etf_float__le(lua_State *L) {
    lua_newtable(L);

    etf_getfloat(L,1);
    etf_getfloat(L,2);

#if !defined(LUA_VERSION_NUM) || LUA_VERSION_NUM < 502
    lua_pushboolean(L, !lua_lessthan(L, -1, -2));
#else
    lua_pushboolean(L, lua_compare(L, -2, -1, LUA_OPLE));
#endif

    return 1;
}

static int
etf_bigint(lua_State *L) {
    bigint *r = NULL;
    bigint *o = NULL;
    int top = lua_gettop(L);

    r = etf_pushbigint(L);

    if(top > 0) {
        if( (o = etf_isbigint(L,1)) != NULL) {
            if(bigint_copy(r,o)) return luaL_error(L,"out of memory");
        }
        else {
            if(etf_tobigint(L,1,r)) return luaL_error(L,"invalid value");
        }
    }

    return 1;
}

static int
etf_bigint__unm(lua_State *L) {
    bigint *r = NULL;
    bigint *o = NULL;

    o = (bigint *)lua_touserdata(L,1);
    r = etf_pushbigint(L);

    if(bigint_copy(r,o)) return luaL_error(L,"out of memory");
    r->sign = !r->sign;

    return 1;
}

static int
etf_bigint__add(lua_State *L) {
    bigint *a = NULL;
    bigint *b = NULL;
    bigint *r = NULL;
    bigint tmp_a = BIGINT_INIT;
    bigint tmp_b = BIGINT_INIT;

    r = etf_pushbigint(L);

    if( (a = etf_isbigint(L,1)) == NULL) {
        if(etf_tobigint(L,1,&tmp_a) != 0) {
            bigint_free(&tmp_a);
            return luaL_error(L,"error converting value at index %d",1);
        }
        a = &tmp_a;
    }

    if( (b = etf_isbigint(L,2)) == NULL) {
        if(etf_tobigint(L,2,&tmp_b) != 0) {
            bigint_free(&tmp_a);
            bigint_free(&tmp_b);
            return luaL_error(L,"error converting value at index %d",2);
        }
        b = &tmp_b;
    }

    if(bigint_add(r,a,b)) {
        bigint_free(&tmp_a);
        bigint_free(&tmp_b);
        return luaL_error(L,"out of memory");
    }

    bigint_free(&tmp_a);
    bigint_free(&tmp_b);

    return 1;
}

static int
etf_bigint__sub(lua_State *L) {
    bigint *a = NULL;
    bigint *b = NULL;
    bigint *r = NULL;
    bigint tmp_a = BIGINT_INIT;
    bigint tmp_b = BIGINT_INIT;

    r = etf_pushbigint(L);

    if( (a = etf_isbigint(L,1)) == NULL) {
        if(etf_tobigint(L,1,&tmp_a) != 0) {
            bigint_free(&tmp_a);
            return luaL_error(L,"error converting value at index %d",1);
        }
        a = &tmp_a;
    }

    if( (b = etf_isbigint(L,2)) == NULL) {
        if(etf_tobigint(L,2,&tmp_b) != 0) {
            bigint_free(&tmp_a);
            bigint_free(&tmp_b);
            return luaL_error(L,"error converting value at index %d",2);
        }
        b = &tmp_b;
    }

    if(bigint_sub(r,a,b)) {
        bigint_free(&tmp_a);
        bigint_free(&tmp_b);
        return luaL_error(L,"out of memory");
    }

    bigint_free(&tmp_a);
    bigint_free(&tmp_b);

    return 1;
}

static int
etf_bigint__mul(lua_State *L) {
    bigint *a = NULL;
    bigint *b = NULL;
    bigint *r = NULL;
    bigint tmp_a = BIGINT_INIT;
    bigint tmp_b = BIGINT_INIT;

    r = etf_pushbigint(L);

    if( (a = etf_isbigint(L,1)) == NULL) {
        if(etf_tobigint(L,1,&tmp_a) != 0) {
            bigint_free(&tmp_a);
            return luaL_error(L,"error converting value at index %d",1);
        }
        a = &tmp_a;
    }

    if( (b = etf_isbigint(L,2)) == NULL) {
        if(etf_tobigint(L,2,&tmp_b) != 0) {
            bigint_free(&tmp_a);
            bigint_free(&tmp_b);
            return luaL_error(L,"error converting value at index %d",2);
        }
        b = &tmp_b;
    }

    if(bigint_mul(r,a,b)) {
        bigint_free(&tmp_a);
        bigint_free(&tmp_b);
        return luaL_error(L,"out of memory");
    }

    bigint_free(&tmp_a);
    bigint_free(&tmp_b);

    return 1;
}

static int
etf_bigint__div(lua_State *L) {
    bigint *a = NULL;
    bigint *b = NULL;
    bigint *r = NULL;
    bigint tmp_a = BIGINT_INIT;
    bigint tmp_b = BIGINT_INIT;
    bigint tmp_remain = BIGINT_INIT;

    r = etf_pushbigint(L);

    if( (a = etf_isbigint(L,1)) == NULL) {
        if(etf_tobigint(L,1,&tmp_a) != 0) {
            bigint_free(&tmp_a);
            return luaL_error(L,"error converting value at index %d",1);
        }
        a = &tmp_a;
    }

    if( (b = etf_isbigint(L,2)) == NULL) {
        if(etf_tobigint(L,2,&tmp_b) != 0) {
            bigint_free(&tmp_a);
            bigint_free(&tmp_b);
            return luaL_error(L,"error converting value at index %d",2);
        }
        b = &tmp_b;
    }

    if(bigint_div_mod(r,&tmp_remain,a,b)) {
        bigint_free(&tmp_remain);
        bigint_free(&tmp_a);
        bigint_free(&tmp_b);
        return luaL_error(L,"out of memory");
    }

    bigint_free(&tmp_remain);
    bigint_free(&tmp_a);
    bigint_free(&tmp_b);

    return 1;
}

static int
etf_bigint__mod(lua_State *L) {
    bigint *a = NULL;
    bigint *b = NULL;
    bigint *r = NULL;
    bigint tmp_a = BIGINT_INIT;
    bigint tmp_b = BIGINT_INIT;
    bigint tmp_quot = BIGINT_INIT;

    r = etf_pushbigint(L);

    if( (a = etf_isbigint(L,1)) == NULL) {
        if(etf_tobigint(L,1,&tmp_a) != 0) {
            bigint_free(&tmp_a);
            return luaL_error(L,"error converting value at index %d",1);
        }
        a = &tmp_a;
    }

    if( (b = etf_isbigint(L,2)) == NULL) {
        if(etf_tobigint(L,2,&tmp_b) != 0) {
            bigint_free(&tmp_a);
            bigint_free(&tmp_b);
            return luaL_error(L,"error converting value at index %d",2);
        }
        b = &tmp_b;
    }

    if(bigint_div_mod(&tmp_quot,r,a,b)) {
        bigint_free(&tmp_quot);
        bigint_free(&tmp_a);
        bigint_free(&tmp_b);
        return luaL_error(L,"out of memory");
    }

    bigint_free(&tmp_quot);
    bigint_free(&tmp_a);
    bigint_free(&tmp_b);

    return 1;
}

static int
etf_bigint__eq(lua_State *L) {
    bigint *a = NULL;
    bigint *b = NULL;
    bigint tmp_a = BIGINT_INIT;
    bigint tmp_b = BIGINT_INIT;

    if( (a = etf_isbigint(L,1)) == NULL) {
        if(etf_tobigint(L,1,&tmp_a) != 0) {
            bigint_free(&tmp_a);
            return luaL_error(L,"error converting value at index %d",1);
        }
        a = &tmp_a;
    }

    if( (b = etf_isbigint(L,2)) == NULL) {
        if(etf_tobigint(L,2,&tmp_b) != 0) {
            bigint_free(&tmp_a);
            bigint_free(&tmp_b);
            return luaL_error(L,"error converting value at index %d",2);
        }
        b = &tmp_b;
    }

    lua_pushboolean(L,bigint_eq(a,b));
    bigint_free(&tmp_a);
    bigint_free(&tmp_b);

    return 1;
}

static int
etf_bigint__lt(lua_State *L) {
    bigint *a = NULL;
    bigint *b = NULL;
    bigint tmp_a = BIGINT_INIT;
    bigint tmp_b = BIGINT_INIT;

    if( (a = etf_isbigint(L,1)) == NULL) {
        if(etf_tobigint(L,1,&tmp_a) != 0) {
            bigint_free(&tmp_a);
            return luaL_error(L,"error converting value at index %d",1);
        }
        a = &tmp_a;
    }

    if( (b = etf_isbigint(L,2)) == NULL) {
        if(etf_tobigint(L,2,&tmp_b) != 0) {
            bigint_free(&tmp_a);
            bigint_free(&tmp_b);
            return luaL_error(L,"error converting value at index %d",2);
        }
        b = &tmp_b;
    }

    lua_pushboolean(L,bigint_lt(a,b));
    bigint_free(&tmp_a);
    bigint_free(&tmp_b);

    return 1;
}

static int
etf_bigint__le(lua_State *L) {
    bigint *a = NULL;
    bigint *b = NULL;
    bigint tmp_a = BIGINT_INIT;
    bigint tmp_b = BIGINT_INIT;

    if( (a = etf_isbigint(L,1)) == NULL) {
        if(etf_tobigint(L,1,&tmp_a) != 0) {
            bigint_free(&tmp_a);
            return luaL_error(L,"error converting value at index %d",1);
        }
        a = &tmp_a;
    }

    if( (b = etf_isbigint(L,2)) == NULL) {
        if(etf_tobigint(L,2,&tmp_b) != 0) {
            bigint_free(&tmp_a);
            bigint_free(&tmp_b);
            return luaL_error(L,"error converting value at index %d",2);
        }
        b = &tmp_b;
    }

    lua_pushboolean(L,bigint_le(a,b));
    bigint_free(&tmp_a);
    bigint_free(&tmp_b);

    return 1;
}

static int
etf_bigint__shl(lua_State *L) {
    bigint *a = NULL;
    bigint *r = NULL;
    lua_Integer t = 0;

    r = etf_pushbigint(L);

    a = (bigint *)lua_touserdata(L,1);
    t = lua_tointeger(L,2);
    if(t < 0) {
        return luaL_error(L,"invalid shift value %d",t);
    }

    if(bigint_lshift(r,a,(size_t)t)) {
        return luaL_error(L,"out of memory");
    }

    return 1;
}

static int
etf_bigint__shr(lua_State *L) {
    bigint *a = NULL;
    bigint *r = NULL;
    lua_Integer t = 0;

    r = etf_pushbigint(L);

    a = (bigint *)lua_touserdata(L,1);
    t = lua_tointeger(L,2);
    if(t < 0) {
        return luaL_error(L,"invalid shift value %d",t);
    }

    if(bigint_rshift(r,a,(size_t)t)) {
        return luaL_error(L,"out of memory");
    }

    return 1;
}

static int
etf_bigint__tostring(lua_State *L) {
    bigint *a = NULL;
    size_t len = 0;
    char *str = NULL;

    a = (bigint *)lua_touserdata(L,1);
    len = bigint_to_string(NULL, 0, a, 10);
    if(len == 0) return luaL_error(L,"error getting bigint string length");
    str = (char *)lua_newuserdata(L,sizeof(char) * (len + 1));
    if(str == NULL) return luaL_error(L,"out of memory");
    len = bigint_to_string(str, len, a, 10);

    lua_pushlstring(L,str,len);
    return 1;
}

static int
etf_bigint__gc(lua_State *L) {
    bigint *a = (bigint *)lua_touserdata(L,1);
    bigint_free(a);
    return 0;
}

static int
etf_atom(lua_State *L) {
    size_t len;
    int type;
    int idx;

    type = lua_type(L,1);
    if(type == LUA_TSTRING) {
        idx = 1;
    } else {
        lua_pushvalue(L,lua_upvalueindex(1));
        lua_pushvalue(L,1);
        lua_call(L,1,1);
        idx = lua_gettop(L);
    }

    type = lua_type(L,idx);
    if(type != LUA_TSTRING) {
        return luaL_error(L,"missing required string argument");
    }

    len = lua_rawlen(L,idx);
    if(len > UINT16_MAX) {
        return luaL_error(L,"string length is too long");
    }

    lua_newtable(L);
    lua_pushvalue(L,idx);
    lua_setfield(L,-2,"atom");

    if(lua_isboolean(L,2)) {
        lua_pushvalue(L,2);
    } else {
        lua_pushboolean(L,1);
    }
    lua_setfield(L,-2,"utf8");

    luaL_setmetatable(L,etf_atom_mt);
    return 1;
}

static int
etf_atom__eq(lua_State *L) {
    lua_getfield(L,1,"atom");
    lua_getfield(L,2,"atom");
    lua_pushboolean(L,lua_rawequal(L,-1,-2));
    return 1;
}

static int
etf_atom__tostring(lua_State *L) {
    lua_getfield(L,1,"atom");
    return 1;
}

static int
etf_string(lua_State *L) {
    size_t len;
    int idx = 1;

    if(lua_type(L,idx) != LUA_TSTRING) {
        lua_pushvalue(L,lua_upvalueindex(1));
        lua_pushvalue(L,1);
        lua_call(L,1,1);
        idx = lua_gettop(L);
    }

    if(lua_type(L,idx) != LUA_TSTRING) {
        return luaL_error(L,"missing required string argument");
    }

    len = lua_rawlen(L,idx);

    if(len > UINT16_MAX) {
        return luaL_error(L,"string is too long");
    }

    lua_newtable(L);
    lua_pushvalue(L,idx);
    lua_setfield(L,-2,"string");
    luaL_setmetatable(L,etf_string_mt);
    return 1;
}

static int
etf_string__eq(lua_State *L) {
    lua_getfield(L,1,"string");
    lua_getfield(L,2,"string");
    lua_pushboolean(L,lua_rawequal(L,-1,-2));
    return 1;
}

static int
etf_string__tostring(lua_State *L) {
    lua_getfield(L,1,"string");
    return 1;
}

static int
etf_binary(lua_State *L) {
    size_t len;
    int idx = 1;
    if(lua_type(L,idx) != LUA_TSTRING) {
        lua_pushvalue(L,lua_upvalueindex(1));
        lua_pushvalue(L,1);
        lua_call(L,1,1);
        idx = lua_gettop(L);
    }

    if(lua_type(L,idx) != LUA_TSTRING) {
        return luaL_error(L,"missing required string argument");
    }

    len = lua_rawlen(L,idx);

    if(len > UINT32_MAX) {
        return luaL_error(L,"string argument too long");
    }

    lua_newtable(L);
    lua_pushvalue(L,idx);
    lua_setfield(L,-2,"binary");
    luaL_setmetatable(L,etf_binary_mt);
    return 1;
}

static int
etf_binary__eq(lua_State *L) {
    lua_getfield(L,1,"binary");
    lua_getfield(L,2,"binary");
    lua_pushboolean(L,lua_rawequal(L,-1,-2));
    return 1;
}

static int
etf_binary__tostring(lua_State *L) {
    lua_getfield(L,1,"binary");
    return 1;
}

static int etf_decode_string(etf_131_decoder_state *D, size_t len) {
    size_t r = 0;
    size_t m;
    char *buffer;
    luaL_Buffer buf;

    luaL_buffinit(D->L,&buf);

    while(r < len) {
        buffer = luaL_prepbuffer(&buf);
        m = len - r;
        m = m > LUAL_BUFFERSIZE ? LUAL_BUFFERSIZE : m;
        D->read(D,(uint8_t *)buffer,m);
        luaL_addsize(&buf,m);
        r += m;
    }
    luaL_pushresult(&buf);

    return 1;
}

static int etf_131_decoder_process_bigint(etf_131_decoder_state *D, uint32_t bytes, uint8_t sign) {
    int ret;
    const uint8_t *str;
    bigint *r = NULL;
    bigint_word w = 0;
    bigint tmp = BIGINT_INIT;
    tmp.size = 1;
    tmp.words = &w;
    uint32_t b = bytes;
    int64_t i;

    r = etf_pushbigint(D->L);

    if( (ret = etf_decode_string(D,bytes)) != 1) return ret;
    str = (const uint8_t *)lua_tolstring(D->L,-1,NULL);

    while(b--) {
        w = (bigint_word)str[b];
        if(bigint_lshift_overwrite(r,8)) return luaL_error(D->L,"out of memory");
        if(bigint_add_unsigned(r,&tmp)) return luaL_error(D->L,"out of memory");
    }
    lua_pop(D->L,1);

    r->sign = (size_t)sign;

    /* see if this can be represented in a native lua number */
    /* if r >= D->b_min && r <= D->b_max */
    if(!D->force_bigint) {
      if(bigint_ge(r,D->b_min) && bigint_le(r,D->b_max)) {
          if(bigint_to_i64(&i,r) == 0) {
              lua_pop(D->L,1);
              lua_pushinteger(D->L,i);
          }
      }
    }

    return 1;
}

static int etf_131_decoder_process_atom(etf_131_decoder_state *D, size_t len) {
    int r;
    int idx;

    if( (r = etf_decode_string(D,len)) != 1) return r;

    idx = lua_gettop(D->L);

    lua_getuservalue(D->L,1);
    lua_getfield(D->L,-1,"atom_map");
    lua_pushvalue(D->L,-3);
    lua_pushboolean(D->L,D->key);
    lua_call(D->L,2,1);

    lua_insert(D->L,idx);
    lua_settop(D->L,idx);

    return 1;

}

static int etf_131_decoder_read(etf_131_decoder_state *D, uint8_t *data, size_t len) {
    if(len > D->len) return luaL_error(D->L,"attempt to read beyond available data");
    memcpy(data,D->data,len);

    D->data += len;
    D->len -= len;
    return 0;
}

static int etf_131_decoder_readz(etf_131_decoder_state *D, uint8_t *data, size_t len) {
    int ret;
    size_t o;
    size_t m;

    o = 0;

    do {
        if(D->offset == ETF_BUFFER_LEN) {
            if(!D->strm.avail_in) return luaL_error(D->L,"readz: no data left to decompress");
            D->strm.avail_out = ETF_BUFFER_LEN;
            D->strm.next_out = D->z;
            D->offset = 0;
            ret = mz_inflate(&D->strm, MZ_NO_FLUSH);
            if(!(ret == MZ_OK || ret == MZ_STREAM_END)) {
                return luaL_error(D->L,"inflate error: %d", ret);
            }
        }
        m = ETF_BUFFER_LEN - D->strm.avail_out - D->offset;
        if(m) {
            m = m > len ? len : m;
            memcpy(&data[o],&D->z[D->offset],m);
            o += m;
            len -= m;
            D->offset += m;
        }
    } while(len);

    return 0;
}

static int etf_131_encoder_writez(etf_131_encoder_state *E, const uint8_t *data, size_t len) {
    int r;

    E->strm.avail_in = len;
    E->strm.next_in = data;

    do {
        E->strm.avail_out = ETF_BUFFER_LEN;
        E->strm.next_out = E->z;
        r = mz_deflate(&E->strm, MZ_NO_FLUSH);
        if(r != MZ_OK) return luaL_error(E->L,"error deflating data: %d", r);
        lua_pushlstring(E->L,(const char *)E->z,ETF_BUFFER_LEN - E->strm.avail_out);
        lua_rawseti(E->L,E->strtable,++E->strcount);
    } while (E->strm.avail_in);

    return 0;
}

static int etf_131_encoder_write(etf_131_encoder_state *E, const uint8_t *data, size_t len) {
    lua_pushlstring(E->L,(const char *)data, len);
    lua_rawseti(E->L,E->strtable,++E->strcount);
    return 0;
}

static int etf_131_decoder_ETFZLIB(etf_131_decoder_state *D) {
    uint8_t buffer[4];
    unsigned long len;
    int ret;

    D->read(D,buffer,4);
    len = (unsigned long)unpack_uint32be(buffer);

    D->offset = ETF_BUFFER_LEN;

    D->strm.zalloc = NULL;
    D->strm.zfree = NULL;
    D->strm.opaque = NULL;
    if( (ret = mz_inflateInit(&D->strm) != 0)) {
        return luaL_error(D->L,"error with inflateInit: %d",ret);
    }

    D->read = etf_131_decoder_readz;
    D->strm.next_in = D->data;
    D->strm.avail_in = D->len;

    ret = etf_131_decode(D);

    mz_inflateEnd(&D->strm);

    if(D->strm.total_in != D->len) {
        return luaL_error(D->L,"error, zlib-compressed data didn't consume all bytes");
    }
    if(D->strm.total_out != len) {
        return luaL_error(D->L,"error, zlib-compressed data didn't produce enough bytes");
    }

    D->data += D->strm.total_in;
    D->len -= D->strm.total_in;

    return ret;
}

static int etf_131_decoder_ATOM_CACHE_REF(etf_131_decoder_state *D) {
    uint8_t atom_idx;

    D->read(D,&atom_idx,1);

    lua_pushnil(D->L);
    return 1;
}

static int etf_131_decoder_NEW_FLOAT_EXT(etf_131_decoder_state *D) {
    uint8_t buffer[8];
    union {
        double val;
        uint64_t tmp;
    } u1;

    D->read(D,buffer,8);

    u1.tmp = unpack_uint64be(buffer);

    if(D->force_float) {
        lua_newtable(D->L);
        lua_pushnumber(D->L,(lua_Number)u1.val);
        lua_setfield(D->L,-2,"float");
        luaL_setmetatable(D->L,etf_float_mt);
    } else {
        lua_pushnumber(D->L,u1.val);
    }
    return 1;
}

static int etf_131_decoder_BIT_BINARY_EXT(etf_131_decoder_state *D) {
    uint8_t buffer[5];
    uint32_t len = 0;
    uint32_t i   = 0;
    uint8_t bits = 0;
    luaL_Buffer b;

    D->read(D,buffer,5);

    len  = unpack_uint32be(&buffer[0]);
    bits = buffer[4];

    luaL_buffinit(D->L,&b);
    while(i++ < len) {
        D->read(D,buffer,1);
        if(i == len) buffer[0] >>= (8-bits);
        luaL_addchar(&b,(char)buffer[0]);
    }
    luaL_pushresult(&b);

    return 1;
}

static int etf_131_decoder_PID_EXT(etf_131_decoder_state *D) {
    int r;
    uint32_t id, serial;
    uint8_t creation;
    uint8_t buffer[9];

    lua_newtable(D->L);

    if( (r = etf_131_decode(D)) != 1) return r;
    lua_setfield(D->L,-2,"node");

    D->read(D,buffer,9);

    id       = unpack_uint32be(&buffer[0]);
    serial   = unpack_uint32be(&buffer[4]);
    creation = buffer[8];

    etf_pushu32(D,id);
    lua_setfield(D->L,-2,"id");

    etf_pushu32(D,serial);
    lua_setfield(D->L,-2,"serial");

    lua_pushinteger(D->L,creation);
    lua_setfield(D->L,-2,"creation");

    luaL_setmetatable(D->L,etf_pid_mt);

    return 1;
}

static int etf_131_decoder_NEW_PID_EXT(etf_131_decoder_state *D) {
    int r;
    uint8_t buffer[12];
    uint32_t id, serial, creation;

    lua_newtable(D->L);

    if( (r = etf_131_decode(D)) != 1) return r;
    lua_setfield(D->L,-2,"node");

    D->read(D,buffer,12);

    id       = unpack_uint32be(&buffer[0]);
    serial   = unpack_uint32be(&buffer[4]);
    creation = unpack_uint32be(&buffer[8]);

    etf_pushu32(D,id);
    lua_setfield(D->L,-2,"id");

    etf_pushu32(D,serial);
    lua_setfield(D->L,-2,"serial");

    etf_pushu32(D,creation);
    lua_setfield(D->L,-2,"creation");

    luaL_setmetatable(D->L,etf_pid_mt);

    return 1;
}

static int etf_131_decoder_PORT_EXT(etf_131_decoder_state *D) {
    int r;
    uint32_t id;
    uint8_t buffer[5];
    lua_newtable(D->L);

    if( (r = etf_131_decode(D)) != 1) return r;
    lua_setfield(D->L,-2,"node");

    D->read(D,buffer,5);

    id = unpack_uint32be(&buffer[0]);

    etf_pushu32(D,id);
    lua_setfield(D->L,-2,"id");

    lua_pushinteger(D->L,buffer[4]);
    lua_setfield(D->L,-2,"creation");

    luaL_setmetatable(D->L,etf_port_mt);

    return 1;
}

static int etf_131_decoder_NEW_PORT_EXT(etf_131_decoder_state *D) {
    int r;
    uint8_t buffer[8];
    uint32_t id, creation;
    lua_newtable(D->L);

    if( (r = etf_131_decode(D)) != 1) return r;
    lua_setfield(D->L,-2,"node");

    D->read(D,buffer,8);

    id = unpack_uint32be(&buffer[0]);
    creation = unpack_uint32be(&buffer[4]);

    etf_pushu32(D,id);
    lua_setfield(D->L,-2,"id");

    etf_pushu32(D,creation);
    lua_setfield(D->L,-2,"creation");

    luaL_setmetatable(D->L,etf_port_mt);

    return 1;
}

static int etf_131_decoder_V4_PORT_EXT(etf_131_decoder_state *D) {
    int r;
    uint8_t buffer[12];
    uint64_t id;
    uint32_t creation;
    lua_newtable(D->L);

    if( (r = etf_131_decode(D)) != 1) return r;
    lua_setfield(D->L,-2,"node");

    D->read(D,buffer,12);

    id = unpack_uint64be(&buffer[0]);
    creation = unpack_uint32be(&buffer[8]);

    etf_pushu64(D,id);
    lua_setfield(D->L,-2,"id");

    etf_pushu64(D,creation);
    lua_setfield(D->L,-2,"creation");

    luaL_setmetatable(D->L,etf_port_mt);

    return 1;
}

static int etf_131_decoder_REFERENCE_EXT(etf_131_decoder_state *D) {
    int r;
    uint8_t buffer[5];
    uint32_t id;
    uint8_t creation;

    lua_newtable(D->L);

    if( (r = etf_131_decode(D)) != 1) return r;
    lua_setfield(D->L,-2,"node");

    D->read(D,buffer,5);

    id = unpack_uint32be(&buffer[0]);
    creation = buffer[4];

    lua_createtable(D->L, 1, 0);
    etf_pushu32(D,id);
    lua_rawseti(D->L,-2,1);

    lua_setfield(D->L,-2,"id");

    lua_pushinteger(D->L,creation);
    lua_setfield(D->L,-2,"creation");

    luaL_setmetatable(D->L,etf_reference_mt);

    return 1;
}

static int etf_131_decoder_NEW_REFERENCE_EXT(etf_131_decoder_state *D) {
    int r;
    uint8_t buffer[12];
    uint16_t i;
    uint16_t n;
    uint32_t id;
    uint8_t creation;

    lua_newtable(D->L);

    D->read(D,buffer,2);
    n = unpack_uint16be(&buffer[0]);

    if( (r = etf_131_decode(D)) != 1) return r;
    lua_setfield(D->L,-2,"node");

    D->read(D,buffer,1);
    creation = buffer[0];

    lua_createtable(D->L, n, 0);
    for(i=1;i<=n;i++) {
        D->read(D,buffer,4);
        id = unpack_uint32be(&buffer[0]);

        etf_pushu32(D,id);
        lua_rawseti(D->L,-2,i);
    }
    lua_setfield(D->L,-2,"id");

    lua_pushinteger(D->L,creation);
    lua_setfield(D->L,-2,"creation");

    luaL_setmetatable(D->L,etf_reference_mt);

    return 1;
}

static int etf_131_decoder_NEWER_REFERENCE_EXT(etf_131_decoder_state *D) {
    int r;
    uint8_t buffer[12];
    uint16_t i;
    uint16_t n;
    uint32_t id;
    uint32_t creation;

    lua_newtable(D->L);

    D->read(D,buffer,2);
    n = unpack_uint16be(&buffer[0]);

    if( (r = etf_131_decode(D)) != 1) return r;
    lua_setfield(D->L,-2,"node");

    D->read(D,buffer,4);
    creation = unpack_uint32be(&buffer[0]);

    lua_createtable(D->L, n, 0);
    for(i=1;i<=n;i++) {
        D->read(D,buffer,4);
        id = unpack_uint32be(&buffer[0]);

        etf_pushu32(D,id);
        lua_rawseti(D->L,-2,i);
    }
    lua_setfield(D->L,-2,"id");

    etf_pushu32(D,creation);
    lua_setfield(D->L,-2,"creation");

    luaL_setmetatable(D->L,etf_reference_mt);

    return 1;
}

static int etf_131_decoder_EXPORT_EXT(etf_131_decoder_state *D) {
    int r;

    lua_newtable(D->L);

    if( (r = etf_131_decode(D)) != 1) return r;
    lua_setfield(D->L,-2,"module");

    if( (r = etf_131_decode(D)) != 1) return r;
    lua_setfield(D->L,-2,"function");

    if( (r = etf_131_decode(D)) != 1) return r;
    lua_setfield(D->L,-2,"arity");

    luaL_setmetatable(D->L,etf_export_mt);

    return 1;
}

static int etf_131_decoder_NEW_FUN_EXT(etf_131_decoder_state *D) {
    int r;
    uint32_t val;
    uint32_t i;
    uint8_t buffer[29];

    lua_newtable(D->L);

    D->read(D,&buffer[0],29);
    val = unpack_uint32be(&buffer[0]);
    etf_pushu32(D,val);
    lua_setfield(D->L,-2,"size");

    lua_pushinteger(D->L,buffer[4]);
    lua_setfield(D->L,-2,"arity");

    lua_pushlstring(D->L,(const char *)&buffer[5],16);
    lua_setfield(D->L,-2,"uniq");

    val = unpack_uint32be(&buffer[21]);
    etf_pushu32(D,val);
    lua_setfield(D->L,-2,"index");

    val = unpack_uint32be(&buffer[25]);
    etf_pushu32(D,val);
    lua_setfield(D->L,-2,"numfree");

    if( (r = etf_131_decode(D)) != 1) return r;
    lua_setfield(D->L,-2,"module");

    if( (r = etf_131_decode(D)) != 1) return r;
    lua_setfield(D->L,-2,"oldindex");

    if( (r = etf_131_decode(D)) != 1) return r;
    lua_setfield(D->L,-2,"olduniq");

    if( (r = etf_131_decode(D)) != 1) return r;
    lua_setfield(D->L,-2,"pid");

    lua_createtable(D->L,val,0);
    for(i=1;i<=val;i++) {
        if( (r = etf_131_decode(D)) != 1) return r;
        lua_rawseti(D->L,-2,i);
    }
    lua_setfield(D->L,-2,"free_vars");

    luaL_setmetatable(D->L,etf_new_fun_mt);

    return 1;
}

static int etf_131_decoder_FUN_EXT(etf_131_decoder_state *D) {
    int r;
    uint32_t val;
    uint32_t i;
    uint8_t buffer[4];

    lua_newtable(D->L);

    D->read(D,&buffer[0],4);
    val = unpack_uint32be(&buffer[0]);
    etf_pushu32(D,val);
    lua_setfield(D->L,-2,"numfree");

    if( (r = etf_131_decode(D)) != 1) return r;
    lua_setfield(D->L,-2,"pid");

    if( (r = etf_131_decode(D)) != 1) return r;
    lua_setfield(D->L,-2,"module");

    if( (r = etf_131_decode(D)) != 1) return r;
    lua_setfield(D->L,-2,"index");

    if( (r = etf_131_decode(D)) != 1) return r;
    lua_setfield(D->L,-2,"uniq");

    lua_createtable(D->L,val,0);
    for(i=1;i<=val;i++) {
        if( (r = etf_131_decode(D)) != 1) return r;
        lua_rawseti(D->L,-2,i);
    }
    lua_setfield(D->L,-2,"free_vars");

    luaL_setmetatable(D->L,etf_fun_mt);

    return 1;
}

static int etf_131_decoder_SMALL_INTEGER_EXT(etf_131_decoder_state *D) {
    uint8_t v = 0;
    bigint *r = NULL;

    D->read(D,&v,1);

    if(D->force_bigint) {
        r = etf_pushbigint(D->L);
        if(bigint_from_u8(r,v)) return luaL_error(D->L,"out of memory");
    } else {
        lua_pushinteger(D->L,v);
    }
    return 1;
}

static int etf_131_decoder_FLOAT_EXT(etf_131_decoder_state *D) {
    double f;
    char tmp[32];

    D->read(D,(uint8_t *)tmp,31);
    tmp[31] = '\0';

    sscanf(tmp,"%lf",&f);

    if(D->force_float) {
        lua_newtable(D->L);
        lua_pushnumber(D->L,(lua_Number)f);
        lua_setfield(D->L,-2,"float");
        luaL_setmetatable(D->L,etf_float_mt);
    } else {
        lua_pushnumber(D->L,(lua_Number)f);
    }
    return 1;
}

static int etf_131_decoder_INTEGER_EXT(etf_131_decoder_state *D) {
    int32_t v = 0;
    bigint *r = NULL;
    uint8_t buffer[4];

    D->read(D,buffer,4);
    v = unpack_int32be(&buffer[0]);

    if(D->force_bigint) {
        r = etf_pushbigint(D->L);
        if(bigint_from_i32(r,v)) return luaL_error(D->L,"out of memory");
    } else {
        etf_pushi32(D,v);
    }
    return 1;
}

static int etf_131_decoder_tuple(etf_131_decoder_state *D, uint32_t items) {
    int r = 0;
    uint32_t i = 0;
    lua_createtable(D->L,items,0);

    while(i++<items) {
        D->key = 0;
        r = etf_131_decode(D);
        if(r != 1) return r;
        lua_rawseti(D->L,-2,i);
    }
    luaL_setmetatable(D->L, etf_tuple_mt);

    return 1;
}

static int etf_131_decoder_SMALL_TUPLE_EXT(etf_131_decoder_state *D) {
    uint8_t items = 0;

    D->read(D,&items,1);
    return etf_131_decoder_tuple(D, (uint32_t) items);
}

static int etf_131_decoder_LARGE_TUPLE_EXT(etf_131_decoder_state *D) {
    uint8_t buffer[4];
    uint32_t items = 0;

    D->read(D,buffer,4);

    items = unpack_uint32be(&buffer[0]);

    return etf_131_decoder_tuple(D, items);
}

static int etf_131_decoder_NIL_EXT(etf_131_decoder_state *D) {
    lua_newtable(D->L);
    return 1;
}

static int etf_131_decoder_ATOM_EXT(etf_131_decoder_state *D) {
    uint8_t tmp[2];
    uint16_t len;

    D->read(D,tmp,2);

    len = unpack_uint16be(&tmp[0]);

    return etf_131_decoder_process_atom(D,(size_t)len);
}

static int etf_131_decoder_STRING_EXT(etf_131_decoder_state *D) {
    uint8_t tmp[2];
    uint16_t len;

    D->read(D,tmp,2);
    len = unpack_uint16be(&tmp[0]);

    return etf_decode_string(D, (size_t) len);

}

static int etf_131_decoder_LIST_EXT(etf_131_decoder_state *D) {
    uint8_t tmp[4];
    uint32_t items = 0;
    uint32_t i = 0;
    int r = 0;

    D->read(D,tmp,4);

    items = unpack_uint32be(&tmp[0]);

    lua_createtable(D->L,items,0);

    while(i++<items) {
        D->key = 0;
        r = etf_131_decode(D);
        if(r != 1) return r;
        lua_rawseti(D->L,-2,i);
    }

    /* a proper LIST_EXT is supposed to end with a NIL_EXT,
     * but an improper list may not */
    D->read(D,tmp,1);
    if(tmp[0] != 106) {
        return luaL_error(D->L,"LIST_EXT: list does not end with NIL_EXT marker");
    }

    luaL_setmetatable(D->L, etf_list_mt);
    return 1;
}

static int etf_131_decoder_SMALL_BIG_EXT(etf_131_decoder_state *D) {
    uint8_t tmp[2];

    D->read(D,tmp,2);

    return etf_131_decoder_process_bigint(D, (uint32_t)tmp[0], tmp[1]);
}

static int etf_131_decoder_LARGE_BIG_EXT(etf_131_decoder_state *D) {
    uint32_t n;
    uint8_t tmp[5];

    D->read(D,tmp,5);

    n = unpack_uint32be(&tmp[0]);

    return etf_131_decoder_process_bigint(D, n, tmp[4]);
}


static int etf_131_decoder_BINARY_EXT(etf_131_decoder_state *D) {
    uint32_t len;
    uint8_t tmp[4];

    D->read(D,tmp,4);
    len = unpack_uint32be(&tmp[0]);

    return etf_decode_string(D, (size_t) len);
}

static int etf_131_decoder_SMALL_ATOM_EXT(etf_131_decoder_state *D) {
    uint8_t len;

    D->read(D,&len,1);

    return etf_131_decoder_process_atom(D,(size_t)len);
}

static int etf_131_decoder_MAP_EXT(etf_131_decoder_state *D) {
    uint32_t arity;
    uint32_t i = 0;
    int r = 0;
    uint8_t tmp[4];

    D->read(D,tmp,4);

    arity = unpack_uint32be(&tmp[0]);

    lua_createtable(D->L,0,arity);

    while(i++<arity) {
        D->key = 1;
        r = etf_131_decode(D);
        if(r != 1) return r;
        D->key = 0;
        r = etf_131_decode(D);
        if(r != 1) return r;
        lua_settable(D->L,-3);
    }

    luaL_setmetatable(D->L, etf_map_mt);
    return 1;
}

static int etf_131_decoder_ATOM_UTF8_EXT(etf_131_decoder_state *D) {
    uint16_t len;
    uint8_t tmp[2];

    D->read(D,tmp,2);

    len = unpack_uint16be(&tmp[0]);

    return etf_131_decoder_process_atom(D,len);
}

static int etf_131_decoder_SMALL_ATOM_UTF8_EXT(etf_131_decoder_state *D) {
    uint8_t len;

    D->read(D,&len,1);

    return etf_131_decoder_process_atom(D,len);
}

static int etf_131_encoder_NEW_FLOAT_EXT(etf_131_encoder_state *E, lua_Number f) {
    uint8_t buf[9];
    union {
        double val;
        uint64_t tmp;
    } u1;

    u1.val = (double)f;

    buf[0] = _131_NEW_FLOAT_EXT;
    pack_uint64be(&buf[1],u1.tmp);

    E->write(E,buf,9);
    return 0;
}

static int etf_131_encoder_SMALL_INTEGER_EXT(etf_131_encoder_state *E, uint8_t n) {
    uint8_t buf[2];
    buf[0] = _131_SMALL_INTEGER_EXT;
    buf[1] = n;

    E->write(E,buf,2);
    return 0;
}

static int etf_131_encoder_INTEGER_EXT(etf_131_encoder_state *E, int32_t n) {
    uint8_t buf[5];
    buf[0] = _131_INTEGER_EXT;
    pack_int32be(&buf[1],n);

    E->write(E,buf,5);
    return 0;
}

static int etf_131_encoder_BIG_EXT(etf_131_encoder_state *E, const bigint *n) {
    uint8_t buffer[8];
    uint8_t i = 0;
    size_t len = 0;
    bigint_word w;
    size_t bitlength = bigint_bitlength(n);
    size_t r = bitlength % 8;
    if(r) {
        bitlength += 8 - r;
    }
    len = bitlength / 8;

    if(len < 256) {
        buffer[0] = _131_SMALL_BIG_EXT;
        buffer[1] = (uint8_t)len;
        i = 2;
    } else {
        buffer[0] = _131_LARGE_BIG_EXT;
        pack_uint32be(&buffer[1],len);
        i = 5;
    }
    buffer[i++] = (uint8_t)n->sign;

    E->write(E,buffer,i);

    for(i=0;i<n->size;i++) {
        w = n->words[i];
        len = 0;
        switch(bitlength) {
            default:
#if BIGINT_WORD_WIDTH == 8
            /* fall-through */
            case 64: {
                buffer[len++] = w & 0xFF;
                bitlength -= 8;
                w >>= 8;
            }
            /* fall-through */
            case 56: {
                buffer[len++] = w & 0xFF;
                bitlength -= 8;
                w >>= 8;
            }
            /* fall-through */
            case 48: {
                buffer[len++] = w & 0xFF;
                bitlength -= 8;
                w >>= 8;
            }
            /* fall-through */
            case 40: {
                buffer[len++] = w & 0xFF;
                bitlength -= 8;
                w >>= 8;
            }
#endif
#if BIGINT_WORD_WIDTH >= 4
            /* fall-through */
            case 32: {
                buffer[len++] = w & 0xFF;
                bitlength -= 8;
                w >>= 8;
            }
            /* fall-through */
            case 24: {
                buffer[len++] = w & 0xFF;
                bitlength -= 8;
                w >>= 8;
            }
#endif
#if BIGINT_WORD_WIDTH >= 2
            /* fall-through */
            case 16: {
                buffer[len++] = w & 0xFF;
                bitlength -= 8;
                w >>= 8;
            }
#endif
#if BIGINT_WORD_WIDTH >= 1
            /* fall-through */
            case 8: {
                buffer[len++] = w & 0xFF;
                bitlength -= 8;
                w >>= 8;
            }
#endif
        }
        E->write(E,buffer,len);
    }
    return 0;
}

static int etf_131_encoder_STRING_EXT(etf_131_encoder_state *E) {
    uint8_t header[3];
    const uint8_t *data = NULL;
    size_t len = 0;

    data = (const uint8_t *)lua_tolstring(E->L,-1,&len);

    if(len > UINT16_MAX) {
        return luaL_error(E->L,"string too long for STRING_EXT");
    }
    header[0] = _131_STRING_EXT;
    pack_uint16be(&header[1],(uint16_t)len);
    E->write(E,header,3);
    E->write(E,data,len);

    return 0;
}

static int etf_131_encoder_BINARY_EXT(etf_131_encoder_state *E) {
    uint8_t header[5];
    const uint8_t *data = NULL;
    size_t len = 0;

    data = (const uint8_t *)lua_tolstring(E->L,-1,&len);

    if(len > UINT32_MAX) {
        return luaL_error(E->L,"string too long for BINARY_EXT");
    }
    header[0] = _131_BINARY_EXT;
    pack_uint32be(&header[1],(uint32_t)len);
    E->write(E,header,5);
    E->write(E,data,len);

    return 0;
}

static int etf_131_encoder_TUPLE_EXT(etf_131_encoder_state *E, int total) {
    int i = 0;
    int r = 0;
    uint8_t header[5];

    if(total < 255) {
        header[0] = _131_SMALL_TUPLE_EXT;
        header[1] = (uint8_t) total;
        E->write(E,header,2);
    } else {
        header[0] = _131_LARGE_TUPLE_EXT;
        pack_uint32be(&header[1],(uint32_t)total);
        E->write(E,header,5);
    }

    for(i=1;i<=total;i++) {
        lua_rawgeti(E->L,-1,i);
        E->key = 0;
        r = etf_131_encode(E);
        lua_pop(E->L,1);
        if(r) return r;
    }

    return 0;
}

static int etf_131_encoder_NIL_EXT(etf_131_encoder_state *E) {
    uint8_t tag = _131_NIL_EXT;

    E->write(E,&tag,1);
    return 0;
}

static int etf_131_encoder_LIST_EXT(etf_131_encoder_state *E, int total) {
    int i = 0;
    int r = 0;
    uint8_t header[5];

    if(total == 0) return etf_131_encoder_NIL_EXT(E);

    header[0] = _131_LIST_EXT;
    pack_uint32be(&header[1],total);

    E->write(E,header,5);

    for(i=1;i<=total;i++) {
        lua_rawgeti(E->L,-1,i);
        E->key = 0;
        r = etf_131_encode(E);
        lua_pop(E->L,1);
        if(r) return r;
    }

    return etf_131_encoder_NIL_EXT(E);
}

static int etf_131_encoder_MAP_EXT(etf_131_encoder_state *E, int total) {
    int r;
    uint8_t header[5];

    header[0] = _131_MAP_EXT;
    pack_uint32be(&header[1],total);

    E->write(E,header,5);

    lua_pushnil(E->L);
    while(lua_next(E->L,-2)) {
        lua_pushvalue(E->L,-2);
        E->key = 1;
        r = etf_131_encode(E);
        if(r) return r;
        lua_pop(E->L,1);

        E->key = 0;
        r = etf_131_encode(E);
        if(r) return r;
        lua_pop(E->L,1);
    }

    return 0;
}

static int etf_131_encoder_SMALL_ATOM_UTF8_EXT(etf_131_encoder_state *E) {
    uint8_t header[2];
    const uint8_t *data = NULL;
    size_t len = 0;

    data = (const uint8_t *)lua_tolstring(E->L,-1,&len);

    if(len > UINT8_MAX) return luaL_error(E->L,"atom length > max");

    header[0] = _131_SMALL_ATOM_UTF8_EXT;
    header[1] = (uint8_t)len;
    E->write(E,header,2);
    E->write(E,data,len);

    return 0;
}

static int etf_131_encoder_ATOM_UTF8_EXT(etf_131_encoder_state *E) {
    uint8_t header[3];
    const uint8_t *data = NULL;
    size_t len = 0;

    data = (const uint8_t *)lua_tolstring(E->L,-1,&len);

    if(len > UINT16_MAX) return luaL_error(E->L,"atom length > max");

    header[0] = _131_ATOM_UTF8_EXT;
    pack_uint16be(&header[1],(uint16_t)len);
    E->write(E,header,3);
    E->write(E,data,len);

    return 0;
}


static int etf_131_encoder_TNIL(etf_131_encoder_state *E) {
    E->write(E,_131_atom_nil,5);
    return 0;
}

static int etf_131_encoder_TBOOLEAN(etf_131_encoder_state *E) {
    int b = lua_toboolean(E->L,-1);

    b ? E->write(E,_131_atom_true,6) : E->write(E,_131_atom_false,7);
    return 0;
}

static int etf_131_encoder_TNUMBER(etf_131_encoder_state *E) {
    int ret;
    lua_Number f;
    lua_Integer n;
    bigint t = BIGINT_INIT;

    if(!lua_isinteger(E->L,-1)) {
        f = lua_tonumber(E->L,-1);
        return etf_131_encoder_NEW_FLOAT_EXT(E,f);
    }

    n = lua_tointeger(E->L,-1);

    if(n >= 0 && n <= UINT8_MAX) {
        return etf_131_encoder_SMALL_INTEGER_EXT(E,(uint8_t)n);
    } else if(n >= INT32_MIN && n <= INT32_MAX) {
        return etf_131_encoder_INTEGER_EXT(E,(int32_t)n);
    }

    if(bigint_from_i64(&t,(int64_t)n)) {
        bigint_free(&t);
        return luaL_error(E->L, "out of memory");
    }

    t.sign = n < 0;
    ret = etf_131_encoder_BIG_EXT(E,&t);
    bigint_free(&t);
    return ret;
}

static int etf_131_encoder_TSTRING(etf_131_encoder_state *E) {
    return etf_131_encoder_BINARY_EXT(E);
}

static int etf_131_encoder_TUSERDATA(etf_131_encoder_state *E) {
    int idx;
    int (*encode)(etf_131_encoder_state *) = NULL;

    idx = lua_gettop(E->L);
    lua_getuservalue(E->L,1);
    lua_getfield(E->L,-1,"mt_map");
    lua_getmetatable(E->L,-3);
    lua_gettable(E->L,-2);

    if(lua_type(E->L,-1) != LUA_TLIGHTUSERDATA) {
        return luaL_error(E->L, "Userdata value without corresponding encode function");
    }

    encode = (int (*)(etf_131_encoder_state *)) lua_touserdata(E->L,-1);
    lua_settop(E->L,idx);
    return encode(E);
}


/* returns 1 iff all keys are integers > 0 and non-sparse 0 otherwise */
static int is_array(lua_State *L, int *total) {
    int i = 0;
    int j = 0;

    lua_pushnil(L);
    while(lua_next(L,-2)) {
        i++;
        /* calling lua_tointeger can technically mess with the value so we duplicate it */
        lua_pushvalue(L,-2);

        if(lua_type(L,-1) == LUA_TNUMBER &&
           lua_isinteger(L,-1) &&
           lua_tointeger(L,-1) == j + 1) {
            j++;
        }
        lua_pop(L,2);
    }
    *total = i;

    return i == j;
}

static int etf_131_encoder_ATOM_EXT(etf_131_encoder_state *E) {
    uint8_t header[3];
    const uint8_t *data = NULL;
    size_t len = 0;

    data = (const uint8_t *)lua_tolstring(E->L,-1,&len);

    if(len > UINT16_MAX) return luaL_error(E->L,"atom length > max");

    header[0] = _131_ATOM_EXT;
    pack_uint16be(&header[1],(uint16_t)len);
    E->write(E,header,3);
    E->write(E,data,len);

    return 0;
}

static int etf_131_encoder_atom_mt(etf_131_encoder_state *E) {
    int r;
    size_t len = 0;
    uint8_t utf8 = 0;

    lua_getfield(E->L,-1,"utf8");
    utf8 = lua_toboolean(E->L,-1);

    lua_getfield(E->L,-2,"atom");
    len = lua_rawlen(E->L,-1);

    if(len > 255) {
        r = etf_131_encoder_ATOM_UTF8_EXT(E);
    } else if(utf8) {
        r = etf_131_encoder_SMALL_ATOM_UTF8_EXT(E);
    } else {
        r = etf_131_encoder_ATOM_EXT(E);
    }
    if(r != 0) return r;

    lua_pop(E->L,2);
    return 0;
}

static int etf_131_encoder_export_mt(etf_131_encoder_state *E) {
    int r;
    int idx;
    int type;
    size_t len;
    uint8_t header[1];

    idx = lua_gettop(E->L);

    header[0] = _131_EXPORT_EXT;
    E->write(E,header,1);

    lua_getfield(E->L,idx,"module");
    type = lua_type(E->L,-1);
    if(type == LUA_TSTRING) {
        len = lua_rawlen(E->L,-1);
        if(len > 255) {
            r = etf_131_encoder_ATOM_UTF8_EXT(E);
        } else {
            r = etf_131_encoder_SMALL_ATOM_UTF8_EXT(E);
        }
    } else { /* should be an atom */
        r = etf_131_encode(E);
    }
    if( r != 0 ) return r;

    lua_getfield(E->L,idx,"function");
    type = lua_type(E->L,-1);
    if(type == LUA_TSTRING) {
        len = lua_rawlen(E->L,-1);
        if(len > 255) {
            r = etf_131_encoder_ATOM_UTF8_EXT(E);
        } else {
            r = etf_131_encoder_SMALL_ATOM_UTF8_EXT(E);
        }
    } else { /* should be an atom */
        r = etf_131_encode(E);
    }
    if( r != 0) return r;

    lua_getfield(E->L,idx,"arity");
    if( (r = etf_131_encode(E)) != 0) return r;

    lua_pop(E->L,3);
    return 0;
}

static int etf_131_encoder_pid_mt(etf_131_encoder_state *E) {
    int r;
    uint8_t header[12];
    int type;
    size_t len = 0;
    uint32_t id, serial, creation;

    header[0] = _131_NEW_PID_EXT;
    E->write(E,header,1);

    lua_getfield(E->L,-1,"node");
    type = lua_type(E->L,-1);
    if(type == LUA_TSTRING) {
        len = lua_rawlen(E->L,-1);
        if(len > 255) {
            r = etf_131_encoder_ATOM_UTF8_EXT(E);
        } else {
            r = etf_131_encoder_SMALL_ATOM_UTF8_EXT(E);
        }
    } else {
        r = etf_131_encode(E);
    }
    if(r != 0) return r;

    lua_getfield(E->L,-2,"id");
    id = etf_tou32(E->L,-1);
    pack_uint32be(&header[0],id);

    lua_getfield(E->L,-3,"serial");
    serial = etf_tou32(E->L,-1);
    pack_uint32be(&header[4],serial);

    lua_getfield(E->L,-4,"creation");
    creation = etf_tou32(E->L,-1);
    pack_uint32be(&header[8],creation);

    E->write(E,header,12);

    lua_pop(E->L,4);

    return 0;
}

static int etf_131_encoder_reference_mt(etf_131_encoder_state *E) {
    int r;
    int type;
    uint8_t header[6];
    uint32_t val;
    size_t i = 0;
    size_t id_len = 0;
    size_t len = 0;
    header[0] = _131_NEWER_REFERENCE_EXT;

    lua_getfield(E->L,-1,"id");
    id_len = lua_rawlen(E->L,-1);
    if(id_len > UINT16_MAX) {
        return luaL_error(E->L,"id field too long");
    }
    pack_uint16be(&header[1],(uint16_t)id_len);
    E->write(E,header,3);

    lua_getfield(E->L,-2,"node");
    type = lua_type(E->L,-1);
    if(type == LUA_TSTRING) {
        len = lua_rawlen(E->L,-1);
        if(len > 255) {
            r = etf_131_encoder_ATOM_UTF8_EXT(E);
        } else {
            r = etf_131_encoder_SMALL_ATOM_UTF8_EXT(E);
        }
    } else {
        r = etf_131_encode(E);
    }
    if(r != 0) return r;
    lua_pop(E->L,1);

    lua_getfield(E->L,-2,"creation");
    val = (uint32_t)etf_tou32(E->L,-1);
    pack_uint32be(&header[0],val);
    E->write(E,header,4);
    lua_pop(E->L,1);

    for(i=1;i<=id_len;i++) {
        lua_rawgeti(E->L,-1,i);
        val = (uint32_t)etf_tou32(E->L,-1);
        pack_uint32be(&header[0],val);
        E->write(E,header,4);
        lua_pop(E->L,1);
    }
    lua_pop(E->L,1);
    return 0;
}

/* mis-nomer - it uses either NEW_PORT_EXT or V4_PORT_EXT */
static int etf_131_encoder_port_mt(etf_131_encoder_state *E) {
    int r;
    uint8_t header[1];
    uint8_t enc[12];
    size_t i = 0;
    int type;
    size_t len = 0;
    uint64_t id = 0;
    uint32_t creation = 0;

    lua_getfield(E->L,-1,"id");
    id = etf_tou64(E->L,-1);
    lua_pop(E->L,1);

    if(id > UINT32_MAX) {
        header[0] = _131_V4_PORT_EXT;
        pack_uint64be(&enc[0],id);
        i = 8;
    } else {
        header[0] = _131_NEW_PORT_EXT;
        pack_uint32be(&enc[0],(uint32_t)id);
        i = 4;
    }

    E->write(E,header,1);

    lua_getfield(E->L,-1,"node");
    type = lua_type(E->L,-1);
    if(type == LUA_TSTRING) {
        len = lua_rawlen(E->L,-1);
        if(len > 255) {
            r = etf_131_encoder_ATOM_UTF8_EXT(E);
        } else {
            r = etf_131_encoder_SMALL_ATOM_UTF8_EXT(E);
        }
    } else {
        r = etf_131_encode(E);
    }
    if(r != 0) return r;
    lua_pop(E->L,1);

    lua_getfield(E->L,-1,"creation");
    creation = etf_tou32(E->L,-1);
    pack_uint32be(&enc[i],creation);
    i += 4;
    lua_pop(E->L,1);

    E->write(E,&enc[0],i);

    return 0;
}


static int etf_131_encoder_float_mt(etf_131_encoder_state *E) {
    lua_Number f;

    lua_getfield(E->L,-1,"float");
    f = lua_tonumber(E->L,-1);
    lua_pop(E->L,1);

    return etf_131_encoder_NEW_FLOAT_EXT(E,f);
}

static int etf_131_encoder_list_mt(etf_131_encoder_state *E) {
    return etf_131_encoder_LIST_EXT(E, lua_rawlen(E->L,-1));
}

static int etf_131_encoder_map_mt(etf_131_encoder_state *E) {
    int total;
    is_array(E->L, &total);
    return etf_131_encoder_MAP_EXT(E, total);
}

static int etf_131_encoder_tuple_mt(etf_131_encoder_state *E) {
    return etf_131_encoder_TUPLE_EXT(E, lua_rawlen(E->L,-1));
}

static int etf_131_encoder_string_mt(etf_131_encoder_state *E) {
    int r;

    lua_getfield(E->L,-1,"string");
    r = etf_131_encoder_STRING_EXT(E);
    lua_pop(E->L,1);
    return r;

}

static int etf_131_encoder_binary_mt(etf_131_encoder_state *E) {
    int r;

    lua_getfield(E->L,-1,"binary");
    r = etf_131_encoder_BINARY_EXT(E);
    lua_pop(E->L,1);
    return r;
}

static int etf_131_encoder_integer_mt(etf_131_encoder_state *E) {
    uint8_t tmp8;
    int32_t tmp32;
    bigint *t = NULL;

    t = (bigint *)lua_touserdata(E->L,-1);

    if(!t->sign && bigint_to_u8(&tmp8,t) == 0) {
        return etf_131_encoder_SMALL_INTEGER_EXT(E,tmp8);
    }
    if(bigint_to_i32(&tmp32,t) == 0) {
        return etf_131_encoder_INTEGER_EXT(E,tmp32);
    }
    return etf_131_encoder_BIG_EXT(E,t);
}

static int etf_131_encoder_fun_mt(etf_131_encoder_state *E) {
    int r;
    size_t len;
    uint8_t header[5];
    uint32_t numfree;
    uint32_t i;
    int idx = lua_gettop(E->L);

    header[0] = _131_FUN_EXT;

    lua_getfield(E->L,idx,"numfree");
    numfree = etf_tou32(E->L,-1);
    pack_uint32be(&header[1], numfree);

    E->write(E,header,5);

    lua_getfield(E->L,idx,"pid");
    if( (r = etf_131_encoder_pid_mt(E)) != 0) return r;

    lua_getfield(E->L,idx,"module");
    if(lua_type(E->L,-1) == LUA_TSTRING) {
        len = lua_rawlen(E->L,-1);
        if(len > 255) {
            if( (r = etf_131_encoder_ATOM_UTF8_EXT(E)) != 0) return r;
        } else {
            if( (r = etf_131_encoder_SMALL_ATOM_UTF8_EXT(E)) != 0) return r;
        }
    } else {
        if( (r = etf_131_encoder_atom_mt(E)) != 0) return r;
    }

    lua_getfield(E->L,idx,"index");
    if( (r = etf_131_encode(E)) != 0) return r;

    lua_getfield(E->L,idx,"uniq");
    if( (r = etf_131_encode(E)) != 0) return r;

    lua_getfield(E->L,idx,"free_vars");
    for(i=1;i<=numfree;i++) {
        lua_rawgeti(E->L,-1,i);
        if( (r = etf_131_encode(E)) != 0) return r;
        lua_pop(E->L,1);
    }

    lua_settop(E->L,idx);
    return r;
}

static int etf_131_encoder_new_fun_mt(etf_131_encoder_state *E) {
    int r;
    size_t len;
    const char *str;
    uint8_t header[1 + 4 + 1 + 16 + 4 + 4];
    uint32_t size;
    uint32_t numfree;
    uint32_t i;
    int idx = lua_gettop(E->L);

    header[0] = _131_NEW_FUN_EXT;

    lua_getfield(E->L,idx,"size");
    size = etf_tou32(E->L,-1);
    pack_uint32be(&header[0 + 1], size);

    lua_getfield(E->L,idx,"arity");
    header[0 + 1 + 4] = lua_tointeger(E->L,-1);

    lua_getfield(E->L,idx,"uniq");
    str = lua_tolstring(E->L,-1, &len);
    for(i=0;i<16;i++) {
        if(i > len) {
            header[i + 0 + 1 + 4 + 1] = '\0';
        } else {
            header[i + 0 + 1 + 4 + 1] = (uint8_t)str[i];
        }
    }

    lua_getfield(E->L,idx,"index");
    size = etf_tou32(E->L,-1);
    pack_uint32be(&header[0 + 1 + 4 + 1 + 16], size);

    lua_getfield(E->L,idx,"numfree");
    numfree = etf_tou32(E->L,-1);
    pack_uint32be(&header[0 + 1 + 4 + 1 + 16 + 4], numfree);

    E->write(E,header,1 + 4 + 1 + 16 + 4 + 4);

    lua_getfield(E->L,idx,"module");
    if(lua_type(E->L,-1) == LUA_TSTRING) {
        len = lua_rawlen(E->L,-1);
        if(len > 255) {
            if( (r = etf_131_encoder_ATOM_UTF8_EXT(E)) != 0) return r;
        } else {
            if( (r = etf_131_encoder_SMALL_ATOM_UTF8_EXT(E)) != 0) return r;
        }
    } else {
        if( (r = etf_131_encoder_atom_mt(E)) != 0) return r;
    }

    lua_getfield(E->L,idx,"oldindex");
    if( (r = etf_131_encode(E)) != 0) return r;

    lua_getfield(E->L,idx,"olduniq");
    if( (r = etf_131_encode(E)) != 0) return r;

    lua_getfield(E->L,idx,"pid");
    if( (r = etf_131_encoder_pid_mt(E)) != 0) return r;


    lua_getfield(E->L,idx,"free_vars");
    for(i=1;i<=numfree;i++) {
        lua_rawgeti(E->L,-1,i);
        if( (r = etf_131_encode(E)) != 0) return r;
        lua_pop(E->L,1);
    }

    lua_settop(E->L,idx);
    return r;
}

/* assumptions - value on top has a metatable */
static inline int etf_testmeta(lua_State *L, const char *tname) {
    int res = 0;

    lua_getmetatable(L,-1);
    luaL_getmetatable(L,tname);
    res = lua_rawequal(L,-1,-2);
    lua_pop(L,2);

    return res;
}


static int etf_131_encoder_TTABLE(etf_131_encoder_state *E) {
    int (*encode)(etf_131_encoder_state *) = NULL;
    int total;
    int array = 0;

    if(lua_getmetatable(E->L,-1) != 0) {
        /* stack is: original value, metatable */
        lua_getuservalue(E->L,1);
        /* stack is: original value, metatable, uservalue */
        lua_getfield(E->L,-1,"mt_map");
        /* stack is: original value, metatable, uservalue, mt_map */
        lua_replace(E->L,-2);
        /* stack is: original value, metatable, mt_map */
        lua_insert(E->L,lua_gettop(E->L) - 1);
        /* stack is: original value, mt_map, metatable */
        lua_gettable(E->L,-2);
        /* stack is: original value, mt_map, lightuserdata */
        if(lua_type(E->L,-1) == LUA_TLIGHTUSERDATA) {
            encode = lua_touserdata(E->L,-1);
        }
        lua_pop(E->L,2);
        if(encode != NULL) return encode(E);
    }

    /* no explicit type set, use the following logic:
     * is_array returns true if (and only if) all keys are
     * sequential integers starting at 1.
     * So, { 'a', 'b', 'c' }        -- is an array
     *     { [1] = 'a', [3] = 'c' } -- is not an array
     */

    array = is_array(E->L,&total);
    if(total == 0) {
        return etf_131_encoder_NIL_EXT(E);
    }

    if(array) {
        return etf_131_encoder_LIST_EXT(E, total);
    }

    return etf_131_encoder_MAP_EXT(E, total);
}

#define ETFZLIB _131_ETFZLIB
#define NEW_FLOAT_EXT _131_NEW_FLOAT_EXT
#define BIT_BINARY_EXT _131_BIT_BINARY_EXT
#define NEW_PID_EXT _131_NEW_PID_EXT
#define NEW_PORT_EXT _131_NEW_PORT_EXT
#define SMALL_INTEGER_EXT _131_SMALL_INTEGER_EXT
#define INTEGER_EXT _131_INTEGER_EXT
#define FLOAT_EXT _131_FLOAT_EXT
#define ATOM_EXT _131_ATOM_EXT
#define PORT_EXT _131_PORT_EXT
#define PID_EXT _131_PID_EXT
#define SMALL_TUPLE_EXT _131_SMALL_TUPLE_EXT
#define LARGE_TUPLE_EXT _131_LARGE_TUPLE_EXT
#define NIL_EXT _131_NIL_EXT
#define STRING_EXT _131_STRING_EXT
#define LIST_EXT _131_LIST_EXT
#define BINARY_EXT _131_BINARY_EXT
#define SMALL_BIG_EXT _131_SMALL_BIG_EXT
#define LARGE_BIG_EXT _131_LARGE_BIG_EXT
#define SMALL_ATOM_EXT _131_SMALL_ATOM_EXT
#define MAP_EXT _131_MAP_EXT
#define ATOM_UTF8_EXT _131_ATOM_UTF8_EXT
#define SMALL_ATOM_UTF8_EXT _131_SMALL_ATOM_UTF8_EXT
#define V4_PORT_EXT _131_V4_PORT_EXT
#define REFERENCE_EXT _131_REFERENCE_EXT
#define NEW_REFERENCE_EXT _131_NEW_REFERENCE_EXT
#define NEWER_REFERENCE_EXT _131_NEWER_REFERENCE_EXT
#define EXPORT_EXT _131_EXPORT_EXT
#define NEW_FUN_EXT _131_NEW_FUN_EXT
#define FUN_EXT _131_FUN_EXT
#define ATOM_CACHE_REF _131_ATOM_CACHE_REF

static int
etf_131_decode(etf_131_decoder_state *D) {
    D->read(D,&D->tag,1);

    switch(D->tag) {
        case ETFZLIB: return etf_131_decoder_ETFZLIB(D);
        case NEW_FLOAT_EXT: return etf_131_decoder_NEW_FLOAT_EXT(D);
        case BIT_BINARY_EXT: return etf_131_decoder_BIT_BINARY_EXT(D);
        case SMALL_INTEGER_EXT: return etf_131_decoder_SMALL_INTEGER_EXT(D);
        case NEW_PORT_EXT: return etf_131_decoder_NEW_PORT_EXT(D);
        case NEW_PID_EXT: return etf_131_decoder_NEW_PID_EXT(D);
        case INTEGER_EXT: return etf_131_decoder_INTEGER_EXT(D);
        case FLOAT_EXT: return etf_131_decoder_FLOAT_EXT(D);
        case ATOM_EXT: return etf_131_decoder_ATOM_EXT(D);
        case PORT_EXT: return etf_131_decoder_PORT_EXT(D);
        case PID_EXT: return etf_131_decoder_PID_EXT(D);
        case SMALL_TUPLE_EXT: return etf_131_decoder_SMALL_TUPLE_EXT(D);
        case LARGE_TUPLE_EXT: return etf_131_decoder_LARGE_TUPLE_EXT(D);
        case NIL_EXT: return etf_131_decoder_NIL_EXT(D);
        case STRING_EXT: return etf_131_decoder_STRING_EXT(D);
        case LIST_EXT: return etf_131_decoder_LIST_EXT(D);
        case BINARY_EXT: return etf_131_decoder_BINARY_EXT(D);
        case SMALL_BIG_EXT: return etf_131_decoder_SMALL_BIG_EXT(D);
        case LARGE_BIG_EXT: return etf_131_decoder_LARGE_BIG_EXT(D);
        case SMALL_ATOM_EXT: return etf_131_decoder_SMALL_ATOM_EXT(D);
        case MAP_EXT: return etf_131_decoder_MAP_EXT(D);
        case ATOM_UTF8_EXT: return etf_131_decoder_ATOM_UTF8_EXT(D);
        case SMALL_ATOM_UTF8_EXT: return etf_131_decoder_SMALL_ATOM_UTF8_EXT(D);
        case V4_PORT_EXT: return etf_131_decoder_V4_PORT_EXT(D);
        case REFERENCE_EXT: return etf_131_decoder_REFERENCE_EXT(D);
        case NEW_REFERENCE_EXT: return etf_131_decoder_NEW_REFERENCE_EXT(D);
        case NEWER_REFERENCE_EXT: return etf_131_decoder_NEWER_REFERENCE_EXT(D);
        case EXPORT_EXT: return etf_131_decoder_EXPORT_EXT(D);
        case NEW_FUN_EXT: return etf_131_decoder_NEW_FUN_EXT(D);
        case FUN_EXT: return etf_131_decoder_FUN_EXT(D);
        case ATOM_CACHE_REF: return etf_131_decoder_ATOM_CACHE_REF(D);
        default: break;
    }

    return luaL_error(D->L, "unimplemented ETF tag: %d", D->tag);
}

#undef ETFZLIB
#undef NEW_FLOAT_EXT
#undef BIT_BINARY_EXT
#undef NEW_PID_EXT
#undef NEW_PORT_EXT
#undef SMALL_INTEGER_EXT
#undef INTEGER_EXT
#undef FLOAT_EXT
#undef ATOM_EXT
#undef PORT_EXT
#undef PID_EXT
#undef SMALL_TUPLE_EXT
#undef LARGE_TUPLE_EXT
#undef NIL_EXT
#undef STRING_EXT
#undef LIST_EXT
#undef BINARY_EXT
#undef SMALL_BIG_EXT
#undef LARGE_BIG_EXT
#undef SMALL_ATOM_EXT
#undef MAP_EXT
#undef ATOM_UTF8_EXT
#undef SMALL_ATOM_UTF8_EXT
#undef V4_PORT_EXT
#undef REFERENCE_EXT
#undef NEW_REFERENCE_EXT
#undef NEWER_REFERENCE_EXT
#undef EXPORT_EXT
#undef NEW_FUN_EXT
#undef FUN_EXT
#undef ATOM_CACHE_REF

static int
etf_131_encode(etf_131_encoder_state *E) {
    int type;
    int idx;

    idx = lua_gettop(E->L);
    lua_getuservalue(E->L,1);
    lua_getfield(E->L,-1,"value_map");
    lua_pushvalue(E->L,-3);
    lua_pushboolean(E->L,E->key);
    lua_call(E->L,2,1);

    lua_insert(E->L,idx);
    lua_settop(E->L,idx);

    type = lua_type(E->L,-1);

    switch(type) {
        case LUA_TNUMBER: {
            return etf_131_encoder_TNUMBER(E);
        }
        case LUA_TSTRING: {
            return etf_131_encoder_TSTRING(E);
        }
        case LUA_TTABLE: {
            return etf_131_encoder_TTABLE(E);
        }
        case LUA_TUSERDATA: {
            return etf_131_encoder_TUSERDATA(E);
        }
        case LUA_TBOOLEAN: {
            return etf_131_encoder_TBOOLEAN(E);
        }
        case LUA_TNIL: {
            return etf_131_encoder_TNIL(E);
        }
        case LUA_TLIGHTUSERDATA: /* fall-through */
        case LUA_TFUNCTION: /* fall-through */
        case LUA_TTHREAD: /* fall-through */
        default: break;
    }

    return luaL_error(E->L, "unimplemented lua type: %s",lua_typename(E->L,type));
}

static int
etf_131_decoder_decode(lua_State *L) {
    int ret;
    etf_131_decoder_state *D = NULL;
    const uint8_t *data = NULL;
    size_t len = 0;
    uint8_t buffer = 0;

    D = luaL_checkudata(L,1,etf_131_decoder_mt);
    data = (const uint8_t *)luaL_checklstring(L,2,&len);

    D->L = L;
    D->data = data;
    D->len = len;
    D->read = etf_131_decoder_read;
    D->key = 0;

    D->read(D,&buffer,1);
    if(buffer != 131) {
        return luaL_error(L,"invalid ETF version %d", buffer);
    }

    ret = etf_131_decode(D);

    if(ret == 1 && D->len != 0) {
        return luaL_error(L,"decoder did not consume all bytes, %d remaining",D->len);
    }

    return ret;
}

static int
etf_131_encoder_encode(lua_State *L) {
    int r;
    int compressLevel = 0;
    uint8_t header[6];
    size_t headerlen = 1;
    size_t i;
    etf_131_encoder_state *E = NULL;
    luaL_Buffer buffer;

    E = luaL_checkudata(L,1,etf_131_encoder_mt);
    if(lua_isnone(L,2)) {
        return luaL_error(L,"need value to encode");
    }
    header[0] = 131;

    E->L = L;
    E->key = 0;

    lua_getuservalue(L,1);
    lua_getfield(L,-1,"compress");
    compressLevel = (int)lua_tonumber(L,-1);
    lua_pop(L,1);

    if(compressLevel < -1 || compressLevel > 9) {
        compressLevel = ETF_NO_COMPRESSION;
    }

    if(compressLevel == ETF_NO_COMPRESSION) {
      E->write = etf_131_encoder_write;
    } else {
      E->strm.zalloc = NULL;
      E->strm.zfree = NULL;
      E->strm.opaque = NULL;
      if( (r = mz_deflateInit(&E->strm,compressLevel)) != 0) {
          return luaL_error(L,"error with deflateInit: %d",r);
      }
      E->write = etf_131_encoder_writez;
    }

    lua_newtable(L);
    E->strtable = lua_gettop(L);
    E->strcount = 0; /* we'll replace the header string at the end */
    lua_pushliteral(L,"");
    lua_rawseti(L,E->strtable,++E->strcount);

    lua_pushvalue(L,2);

    if( (r = etf_131_encode(E)) != 0)  return r;

    if(compressLevel != ETF_NO_COMPRESSION) {
        header[1] = _131_ETFZLIB;
        pack_uint32be(&header[2], (uint32_t)E->strm.total_in);

        headerlen = 6;
        do {
            E->strm.avail_out = ETF_BUFFER_LEN;
            E->strm.next_out = E->z;
            r = mz_deflate(&E->strm, MZ_FINISH);
            if(!(r == MZ_OK || r == MZ_STREAM_END)) {
                return luaL_error(L,"error flushing compressed stream");
            }
            lua_pushlstring(E->L,(const char *)E->z,ETF_BUFFER_LEN - E->strm.avail_out);
            lua_rawseti(L,E->strtable,++E->strcount);
        } while (r != MZ_STREAM_END);
        mz_deflateEnd(&E->strm);
    }

    lua_pushlstring(L,(const char *)header,headerlen);
    lua_rawseti(L,E->strtable,1);

    luaL_buffinit(L,&buffer);
    for(i=1;i<=E->strcount;i++) {
        lua_rawgeti(L,E->strtable,i);
        luaL_addvalue(&buffer);
    }
    luaL_pushresult(&buffer);

    return 1;
}

static int
etf_port(lua_State *L) {
    size_t len = 0;
    int type;
    int idx;

    lua_newtable(L);
    idx = lua_gettop(L);

    type = lua_type(L,1);
    if(type != LUA_TTABLE) {
        return luaL_error(L,"table value required");
    }

    lua_getfield(L,1,"node");
    type = lua_type(L,-1);
    if(type == LUA_TTABLE && etf_testmeta(L, etf_atom_mt)) {
        len = 0;
    } else if(type == LUA_TSTRING) {
        len = lua_rawlen(L,-1);
    } else {
        lua_pushvalue(L,lua_upvalueindex(1));
        lua_pushvalue(L,-2);
        lua_call(L,1,1);
        if(lua_type(L,-1) != LUA_TSTRING) return luaL_error(L,"field 'module' is unacceptable type");
        len = lua_rawlen(L,-1);
    }
    if(len > UINT16_MAX) {
        return luaL_error(L,"field 'node' is too long");
    }
    lua_setfield(L,idx,"node");

    lua_getfield(L,1,"creation");
    if(lua_isuserdata(L,-1)) {
        if(luaL_testudata(L,-1,etf_integer_mt) == NULL) {
            return luaL_error(L,"field 'creation' is not acceptable type");
        }
    } else if(!lua_isinteger(L,-1)) {
        return luaL_error(L,"field 'creation' is missing or non-integer");
    }
    lua_setfield(L,idx,"creation");

    lua_getfield(L,1,"id");
    if(lua_isuserdata(L,-1)) {
        if(luaL_testudata(L,-1,etf_integer_mt) == NULL) {
            return luaL_error(L,"field 'id' is not acceptable type");
        }
    } else if(!lua_isinteger(L,-1)) {
        return luaL_error(L,"field 'id' is missing or non-integer");
    }
    lua_setfield(L,idx,"id");

    lua_settop(L,idx);
    luaL_setmetatable(L, etf_port_mt);
    return 1;
}

static int
etf_export(lua_State *L) {
    int type;
    size_t len;
    lua_Integer n;
    bigint *b;
    uint8_t t;
    int idx;

    lua_newtable(L);
    idx = lua_gettop(L);

    type = lua_type(L,1);
    if(type != LUA_TTABLE) {
        return luaL_error(L,"table value required");
    }

    lua_getfield(L,1,"module");
    type = lua_type(L,-1);
    if(type == LUA_TTABLE && etf_testmeta(L, etf_atom_mt)) {
        len = 0;
    } else if(type == LUA_TSTRING) {
        len = lua_rawlen(L,-1);
    } else {
        lua_pushvalue(L,lua_upvalueindex(1));
        lua_pushvalue(L,-2);
        lua_call(L,1,1);
        if(lua_type(L,-1) != LUA_TSTRING) return luaL_error(L,"field 'module' is unacceptable type");
        len = lua_rawlen(L,-1);
    }
    if(len > UINT16_MAX) {
        return luaL_error(L,"field 'module' is too long");
    }
    lua_setfield(L,idx,"module");

    lua_getfield(L,1,"function");
    type = lua_type(L,-1);
    if(type == LUA_TTABLE && etf_testmeta(L, etf_atom_mt)) {
        len = 0;
    } else if(type == LUA_TSTRING) {
        len = lua_rawlen(L,-1);
    } else {
        lua_pushvalue(L,lua_upvalueindex(1));
        lua_pushvalue(L,-2);
        lua_call(L,1,1);
        if(lua_type(L,-1) != LUA_TSTRING) return luaL_error(L,"field 'function' is unacceptable type");
        len = lua_rawlen(L,-1);
    }
    if(len > UINT16_MAX) {
        return luaL_error(L,"field 'function' is too long");
    }
    lua_setfield(L,idx,"function");

    lua_getfield(L,1,"arity");
    if(lua_isuserdata(L,-1)) {
        if( (b = luaL_testudata(L,-1,etf_integer_mt)) == NULL) {
            return luaL_error(L,"field 'arity' is not acceptable type");
        }
        if(b->sign || bigint_to_u8(&t,b)) {
            return luaL_error(L,"field 'arity' is out of range");
        }
        n = t;
    } else if(lua_isinteger(L,-1)) {
        n = lua_tointeger(L,-1);
        if(n < 0 || n > 255) return luaL_error(L,"field 'arity' is not an acceptable integer");
    } else {
        return luaL_error(L,"field 'arity' is missing or non-integer");
    }
    lua_pushinteger(L,n);
    lua_setfield(L,idx,"arity");

    lua_settop(L,idx);
    luaL_setmetatable(L, etf_export_mt);
    return 1;
}


static int
etf_pid(lua_State *L) {
    int type;
    size_t len;
    int idx;

    type = lua_type(L,1);
    if(type != LUA_TTABLE) {
        return luaL_error(L,"table value required");
    }

    lua_newtable(L);
    idx = lua_gettop(L);

    lua_getfield(L,1,"node");
    type = lua_type(L,-1);
    if(type == LUA_TNIL) {
        return luaL_error(L,"field 'module' is missing");
    }
    else if(type == LUA_TTABLE && etf_testmeta(L, etf_atom_mt)) {
        len = 0;
    } else if(type == LUA_TSTRING) {
        len = lua_rawlen(L,-1);
    } else {
        lua_pushvalue(L,lua_upvalueindex(1));
        lua_pushvalue(L,-2);
        lua_call(L,1,1);
        if(lua_type(L,-1) != LUA_TSTRING) return luaL_error(L,"field 'module' is unacceptable type");
        len = lua_rawlen(L,-1);
    }
    if(len > UINT16_MAX) {
        return luaL_error(L,"field 'node' is too long");
    }
    lua_setfield(L,idx,"node");

    lua_getfield(L,1,"creation");
    if(lua_isuserdata(L,-1)) {
        if(luaL_testudata(L,-1,etf_integer_mt) == NULL) {
            return luaL_error(L,"field 'creation' is not acceptable type");
        }
    } else if(!lua_isinteger(L,-1)) {
        return luaL_error(L,"field 'creation' is missing or non-integer");
    }
    lua_setfield(L,idx,"creation");

    lua_getfield(L,1,"id");
    if(lua_isuserdata(L,-1)) {
        if(luaL_testudata(L,-1,etf_integer_mt) == NULL) {
            return luaL_error(L,"field 'id' is not acceptable type");
        }
    } else if(!lua_isinteger(L,-1)) {
        return luaL_error(L,"field 'id' is missing or non-integer");
    }
    lua_setfield(L,idx,"id");

    lua_getfield(L,1,"serial");
    if(lua_isuserdata(L,-1)) {
        if(luaL_testudata(L,-1,etf_integer_mt) == NULL) {
            return luaL_error(L,"field 'serial' is not acceptable type");
        }
    } else if(!lua_isinteger(L,-1)) {
        return luaL_error(L,"field 'serial' is missing or non-integer");
    }
    lua_setfield(L,idx,"serial");

    lua_settop(L,idx);
    luaL_setmetatable(L, etf_pid_mt);
    return 1;
}

static int
etf_reference(lua_State *L) {
    int type;
    size_t len;
    int idx;

    type = lua_type(L, 1);

    lua_newtable(L);
    idx = lua_gettop(L);

    if(type != LUA_TTABLE) {
        return luaL_error(L,"table required");
    }

    lua_getfield(L,1,"node");
    type = lua_type(L,-1);
    if(type == LUA_TTABLE && etf_testmeta(L, etf_atom_mt)) {
        len = 0;
    } else if(type == LUA_TSTRING) {
        len = lua_rawlen(L,-1);
    } else {
        lua_pushvalue(L,lua_upvalueindex(1));
        lua_pushvalue(L,-2);
        lua_call(L,1,1);
        if(lua_type(L,-1) != LUA_TSTRING) return luaL_error(L,"field 'module' is unacceptable type");
        len = lua_rawlen(L,-1);
    }
    if(len > UINT16_MAX) {
        return luaL_error(L,"field 'node' is too long");
    }
    lua_setfield(L,idx,"node");

    lua_getfield(L,1,"creation");
    if(lua_isuserdata(L,-1)) {
        if(luaL_testudata(L,-1,etf_integer_mt) == NULL) {
            return luaL_error(L,"field 'creation' is not acceptable type");
        }
    } else if(!lua_isinteger(L,-1)) {
        return luaL_error(L,"field 'creation' is missing or non-integer");
    }
    lua_setfield(L,idx,"creation");

    lua_getfield(L,1,"id");
    if(!lua_istable(L,-1)) {
        return luaL_error(L,"field 'id' is missing or non-table");
    }
    len = lua_rawlen(L,-1);
    if(len == 0) {
        return luaL_error(L,"field 'id' has 0 values");
    }
    if(len > UINT16_MAX) {
        return luaL_error(L,"field 'id' has too many values");
    }
    lua_setfield(L,idx,"id");

    lua_settop(L,idx);
    luaL_setmetatable(L, etf_reference_mt);
    return 1;
}

static int
etf_tuple(lua_State *L) {
    int type;

    type = lua_type(L,1);
    if(type == LUA_TTABLE) {
        lua_pushvalue(L,1);
    }
    else {
        lua_newtable(L);
    }

    luaL_setmetatable(L, etf_tuple_mt);
    return 1;
}

static int
etf_list(lua_State *L) {
    int type;

    type = lua_type(L,1);
    if(type == LUA_TTABLE) {
        lua_pushvalue(L,1);
    }
    else {
        lua_newtable(L);
    }

    luaL_setmetatable(L, etf_list_mt);
    return 1;
}

static int
etf_map(lua_State *L) {
    int type;

    type = lua_type(L,1);
    if(type == LUA_TTABLE) {
        lua_pushvalue(L,1);
    }
    else {
        lua_newtable(L);
    }

    luaL_setmetatable(L, etf_map_mt);
    return 1;
}

static int
etf_131_atom_map_default(lua_State *L) {
    /* default atom mapping:
     * if key - return string
     * else
     *   if 'true' return boolean true
     *   if 'false' return boolean false
     *   if 'nil' return lightuserdata(NULL)
     *   else return as string
     */
    int b = lua_toboolean(L,2);

    if(b) {
        lua_pushvalue(L,1);
        return 1;
    }

    lua_pushvalue(L,1);
    lua_gettable(L,lua_upvalueindex(1));
    if(lua_isnil(L,-1)) lua_pushvalue(L,1);
    return 1;
}


static int
etf_131_atom_map(lua_State *L) {
    lua_pushvalue(L,1); /* push our string */
    lua_gettable(L,lua_upvalueindex(1));
    if(lua_isnil(L,-1)) lua_pushvalue(L,1);
    return 1;
}

static int
etf_131_decoder_new(lua_State *L) {
    int64_t *t = NULL;
    int type;

    etf_131_decoder_state *D = (etf_131_decoder_state *)lua_newuserdata(L,sizeof(etf_131_decoder_state));

    if(D == NULL) {
        return luaL_error(L,"out of memory");
    }
    luaL_setmetatable(L,etf_131_decoder_mt);

    D->force_bigint = 0;
    D->force_float = 0;

    lua_newtable(L);

    lua_pushvalue(L,lua_upvalueindex(5));
    lua_pushcclosure(L, etf_131_atom_map_default, 1);
    lua_setfield(L, -2, "atom_map");

    if(lua_istable(L,1)) {
        lua_getfield(L,1,"use_integer");
        type = lua_type(L,-1);
        if(type == LUA_TBOOLEAN) {
            D->force_bigint = lua_toboolean(L,-1);
        } else if(type != LUA_TNIL) {
            return luaL_error(L,"unsupported value for use_integer");
        }
        lua_pop(L,1);

        lua_getfield(L,1,"use_float");
        type = lua_type(L,-1);
        if(type == LUA_TBOOLEAN) {
            D->force_float = lua_toboolean(L,-1);
        } else if(type != LUA_TNIL) {
            return luaL_error(L,"unsupported value for use_float");
        }
        lua_pop(L,1);

        lua_getfield(L,1,"atom_map");
        type = lua_type(L,-1);

        if(type == LUA_TTABLE) {
            lua_pushcclosure(L, etf_131_atom_map,1);
            lua_setfield(L,-2,"atom_map");
        } else if(type == LUA_TFUNCTION) {
            lua_setfield(L,-2,"atom_map");
        } else if(type != LUA_TNIL) {
            return luaL_error(L,"unsupported value for atom_map");
        } else {
            lua_pop(L,1);
        }
    }
    lua_setuservalue(L,-2);

    D->b_min = lua_touserdata(L,lua_upvalueindex(1));
    D->b_max = lua_touserdata(L,lua_upvalueindex(2));
    t = lua_touserdata(L,lua_upvalueindex(3));
    D->i_min = *t;
    t = lua_touserdata(L,lua_upvalueindex(4));
    D->i_max = *t;

    return 1;
}

static int
etf_decoder_new(lua_State *L) {
    uint8_t version = ETF_DEFAULT_VERSION;

    if(lua_istable(L,1)) {
        lua_getfield(L,1,"version");
        if(lua_isnumber(L,-1)) {
            version = lua_tonumber(L,-1);
        }
        lua_pop(L,1);
    }

    lua_rawgeti(L,lua_upvalueindex(1),version);
    if(lua_isnil(L,-1)) {
        return luaL_error(L,"unsupported Erlang Term Format version %d",version);
    }
    lua_pushvalue(L,1);
    lua_call(L,1,1);
    return 1;
}

static int
etf_decode(lua_State *L) {
    /* convenience method that's basically:
     * function etf.decode(data,opts)
     *   return etf.decoder(opts):decode(data)
     */
    int args = 0;

    if(!lua_isstring(L,1)) return luaL_error(L,"missing data");

    lua_pushvalue(L, lua_upvalueindex(1));
    if(lua_istable(L,2)) {
        lua_pushvalue(L,2);
        args++;
    }
    lua_call(L,args,1);

    lua_getfield(L,-1,"decode");
    lua_pushvalue(L,-2);
    lua_pushvalue(L,1);
    lua_call(L,2,1);

    return 1;
}

static int
etf_131_table_value_map(lua_State *L) {
    lua_pushvalue(L,1);
    lua_gettable(L,lua_upvalueindex(1));
    if(lua_isnil(L,-1)) lua_pushvalue(L,1);
    return 1;
}

static int
etf_131_default_value_map(lua_State *L) {
    uint8_t is_key = 0;
    /* default value mapping:
     * if we're a key (ie, doing a MAP_EXT):
     *   return the value as a string
     * if boolean:
     *   return 'true' or 'false' atoms
     * else
     *   return as-is
     */

    is_key = lua_toboolean(L,2);
    if(is_key) {
        lua_pushvalue(L,lua_upvalueindex(2));
        lua_pushvalue(L,1);
        lua_call(L,1,1);
        return 1;
    }
    lua_pushvalue(L,1);
    lua_gettable(L,lua_upvalueindex(1));
    if(lua_isnil(L,-1)) lua_pushvalue(L,1);
    return 1;
}

static int
etf_131_encoder_new(lua_State *L) {
    lua_Number c;
    etf_131_encoder_state *E = (etf_131_encoder_state *)lua_newuserdata(L,sizeof(etf_131_encoder_state));

    if(E == NULL) {
        return luaL_error(L,"out of memory");
    }
    luaL_setmetatable(L,etf_131_encoder_mt);

    lua_newtable(L);
    lua_pushinteger(L, ETF_NO_COMPRESSION);
    lua_setfield(L, -2, "compress");

    lua_pushvalue(L,lua_upvalueindex(3));
    lua_setfield(L,-2,"mt_map");

    lua_pushvalue(L,lua_upvalueindex(1));
    lua_pushvalue(L,lua_upvalueindex(2));
    lua_pushcclosure(L, etf_131_default_value_map,2);
    lua_setfield(L,-2,"value_map");

    if(lua_istable(L,1)) {
        lua_getfield(L,1,"compress");
        if(lua_isnil(L,-1)) {
            lua_pop(L,1);
        } else if(lua_isnumber(L,-1)) {
            c = lua_tointeger(L,-1);
            if(c < -1 || c > 9) return luaL_error(L,"invalid compression level");
            lua_setfield(L,-2,"compress");
        } else if(lua_isboolean(L,-1)) {
            if(lua_toboolean(L,-1)) {
                lua_pushinteger(L,-1);
                lua_setfield(L,-3,"compress");
            }
            lua_pop(L,1);
        } else {
            return luaL_error(L,"invalid compression value");
        }

        lua_getfield(L,1,"value_map");
        if(lua_istable(L,-1)) {
            lua_pushcclosure(L, etf_131_table_value_map, 1);
            lua_setfield(L, -2, "value_map");
        } else if(lua_isfunction(L,-1)) {
            lua_setfield(L, -2, "value_map");
        } else {
            lua_pop(L,1);
        }
    }

    lua_setuservalue(L,-2);

    return 1;
}

static int
etf_encoder_new(lua_State *L) {
    uint8_t version = ETF_DEFAULT_VERSION;

    if(lua_istable(L,1)) {
        lua_getfield(L,1,"version");
        if(lua_isnumber(L,-1)) {
            version = lua_tonumber(L,-1);
        }
        lua_pop(L,1);
    }

    lua_rawgeti(L,lua_upvalueindex(1),version);
    if(lua_isnil(L,-1)) {
        return luaL_error(L,"unsupported Erlang Term Format version %d",version);
    }
    lua_pushvalue(L,1);
    lua_call(L,1,1);
    return 1;
}

static int
etf_encode(lua_State *L) {
    /* convenience method that's basically:
     * function etf.encode(data,opts)
     *   return etf.encoder(opts):encode(value)
     */
    int args = 0;
    if(lua_isnone(L,1)) return luaL_error(L,"missing value");

    lua_pushvalue(L, lua_upvalueindex(1));
    if(lua_istable(L,2)) {
        lua_pushvalue(L,2);
        args++;
    }
    lua_call(L,args,1);

    lua_getfield(L,-1,"encode");
    lua_pushvalue(L,-2);
    lua_pushvalue(L,1);
    lua_call(L,2,1);

    return 1;
}

static const struct luaL_Reg etf_bigint_metamethods[] = {
    { "__add",      etf_bigint__add      },
    { "__sub",      etf_bigint__sub      },
    { "__mul",      etf_bigint__mul      },
    { "__div",      etf_bigint__div      },
    { "__idiv",     etf_bigint__div      },
    { "__mod",      etf_bigint__mod      },
    { "__unm",      etf_bigint__unm      },
    /*
    { "__pow",      etf_bigint__pow      },
    { "__band",     etf_bigint__band     },
    { "__bor",      etf_bigint__bor      },
    { "__bxor",     etf_bigint__bxor     },
    { "__bnot",     etf_bigint__bnot     },
    */
    { "__shl",      etf_bigint__shl      },
    { "__shr",      etf_bigint__shr      },
    { "__eq",       etf_bigint__eq       },
    { "__lt",       etf_bigint__lt       },
    { "__le",       etf_bigint__le       },
    { "__tostring", etf_bigint__tostring },
    { "__gc",       etf_bigint__gc       },
    { NULL,         NULL                 },
};

static const struct luaL_Reg etf_float_metamethods[] = {
    { "__add",      etf_float__add      },
    { "__sub",      etf_float__sub      },
    { "__mul",      etf_float__mul      },
    { "__div",      etf_float__div      },
    { "__mod",      etf_float__mod      },
    { "__pow",      etf_float__pow      },
    { "__unm",      etf_float__unm      },
#if LUA_VERSION_NUM >= 503
    { "__idiv",     etf_float__idiv     },
    { "__bnot",     etf_float__bnot     },
    { "__band",     etf_float__band     },
    { "__bor",      etf_float__bor      },
    { "__bxor",     etf_float__bxor     },
    { "__shl",      etf_float__shl      },
    { "__shr",      etf_float__shr      },
#endif
    { "__eq",       etf_float__eq       },
    { "__lt",       etf_float__lt       },
    { "__le",       etf_float__le       },
    { NULL,         NULL                },
};

static const struct luaL_Reg etf_atom_metamethods[] = {
    { "__tostring", etf_atom__tostring },
    { "__eq",       etf_atom__eq       },
    { NULL,         NULL                 },
};

static const struct luaL_Reg etf_string_metamethods[] = {
    { "__tostring", etf_string__tostring },
    { "__eq",       etf_string__eq       },
    { NULL,         NULL                 },
};

static const struct luaL_Reg etf_binary_metamethods[] = {
    { "__tostring", etf_binary__tostring },
    { "__eq",       etf_binary__eq       },
    { NULL,         NULL                 },
};

static const struct luaL_Reg etf_131_decoder_methods[] = {
    { "decode", etf_131_decoder_decode },
    { NULL, NULL },
};

static const struct luaL_Reg etf_131_encoder_methods[] = {
    { "encode", etf_131_encoder_encode },
    { NULL, NULL },
};

#define TOTAL_DECODERS 1
#define TOTAL_ENCODERS 1

typedef struct etf_decoder_rec {
    uint8_t version;
    lua_CFunction func;
} etf_decoder_rec;

static const etf_decoder_rec etf_decoders[TOTAL_DECODERS+1] = {
    { 131, etf_131_decoder_new },
    { 0, NULL },
};

typedef struct etf_encoder_mt_rec {
    const char* name;
    int (*func)(void *encoder_state);
} etf_encoder_mt_rec;

typedef struct etf_encoder_rec {
    uint8_t version;
    lua_CFunction func;
    const etf_encoder_mt_rec *metas;
} etf_encoder_rec;

static const etf_encoder_mt_rec etf_131_encoder_metas[] = {
    { etf_integer_mt,   (int (*)(void *))etf_131_encoder_integer_mt },
    { etf_float_mt,     (int (*)(void *))etf_131_encoder_float_mt },
    { etf_port_mt,      (int (*)(void *))etf_131_encoder_port_mt },
    { etf_pid_mt,       (int (*)(void *))etf_131_encoder_pid_mt },
    { etf_export_mt,    (int (*)(void *))etf_131_encoder_export_mt },
    { etf_new_fun_mt,   (int (*)(void *))etf_131_encoder_new_fun_mt },
    { etf_fun_mt,       (int (*)(void *))etf_131_encoder_fun_mt },
    { etf_reference_mt, (int (*)(void *))etf_131_encoder_reference_mt },
    { etf_tuple_mt,     (int (*)(void *))etf_131_encoder_tuple_mt },
    { etf_list_mt,      (int (*)(void *))etf_131_encoder_list_mt },
    { etf_map_mt,       (int (*)(void *))etf_131_encoder_map_mt },
    { etf_atom_mt,      (int (*)(void *))etf_131_encoder_atom_mt },
    { etf_string_mt,    (int (*)(void *))etf_131_encoder_string_mt },
    { etf_binary_mt,    (int (*)(void *))etf_131_encoder_binary_mt },
    { NULL, NULL },
};

static const etf_encoder_rec etf_encoders[TOTAL_ENCODERS+1] = {
    { 131, etf_131_encoder_new, etf_131_encoder_metas },
    { 0, NULL, NULL },
};

static const struct luaL_Reg etf_functions[] = {
    { "integer" , etf_bigint },
    { "list", etf_list },
    { "map", etf_map },
    { "tuple", etf_tuple },
    { NULL, NULL },
};

#ifdef __cplusplus
extern "C" {
#endif

int luaopen_etf(lua_State *L) {
    int i = 0;
    int eq = 0;
    bigint *b_min = NULL;
    bigint *b_max = NULL;
    int64_t *i_min = NULL;
    int64_t *i_max = NULL;

    const etf_decoder_rec *decoders = etf_decoders;
    const etf_encoder_rec *encoders = etf_encoders;
    const etf_encoder_mt_rec *meta = NULL;
    lua_newtable(L);

    if(luaL_newmetatable(L,etf_integer_mt)) {
        luaL_setfuncs(L,etf_bigint_metamethods,0);
        /* on newer lua versions, luaL_newmetatable automatically sets __name,
         * we just override it so it's the same across all versions */
        lua_pushstring(L,etf_integer_mt);
        lua_setfield(L,-2,"__name");
    }
    lua_setfield(L,-2,"integer_mt");

    if(luaL_newmetatable(L,etf_float_mt)) {
        luaL_setfuncs(L,etf_float_metamethods,0);
        lua_getglobal(L,"tostring");
        lua_pushcclosure(L, etf_float__tostring,1);
        lua_setfield(L,-2,"__tostring");
        lua_getglobal(L,"tostring");
        lua_pushcclosure(L, etf_float__concat,1);
        lua_setfield(L,-2,"__concat");
        lua_pushstring(L,etf_float_mt);
        lua_setfield(L,-2,"__name");
    }
    lua_setfield(L,-2,"float_mt");

    if(luaL_newmetatable(L,etf_131_decoder_mt)) {
        lua_newtable(L);
        luaL_setfuncs(L,etf_131_decoder_methods,0);
        lua_setfield(L,-2,"__index");
        lua_pushstring(L,etf_131_decoder_mt);
        lua_setfield(L,-2,"__name");
    }
    lua_setfield(L,-2,"decoder_131_mt");

    if(luaL_newmetatable(L,etf_131_encoder_mt)) {
        lua_newtable(L);
        luaL_setfuncs(L,etf_131_encoder_methods,0);
        lua_setfield(L,-2,"__index");
        lua_pushstring(L,etf_131_encoder_mt);
        lua_setfield(L,-2,"__name");
    }
    lua_setfield(L,-2,"encoder_131_mt");

    if(luaL_newmetatable(L,etf_atom_mt)) {
        luaL_setfuncs(L,etf_atom_metamethods,0);
        lua_pushstring(L,etf_atom_mt);
        lua_setfield(L,-2,"__name");
    }
    lua_setfield(L,-2,"atom_mt");

    if(luaL_newmetatable(L,etf_string_mt)) {
        luaL_setfuncs(L,etf_string_metamethods,0);
        lua_pushstring(L,etf_string_mt);
        lua_setfield(L,-2,"__name");
    }
    lua_setfield(L,-2,"string_mt");

    if(luaL_newmetatable(L,etf_binary_mt)) {
        luaL_setfuncs(L,etf_binary_metamethods,0);
        lua_pushstring(L,etf_binary_mt);
        lua_setfield(L,-2,"__name");
    }
    lua_setfield(L,-2,"binary_mt");

    if(luaL_newmetatable(L,etf_port_mt)) {
        lua_pushstring(L,etf_port_mt);
        lua_setfield(L,-2,"__name");
    }
    lua_setfield(L,-2,"port_mt");

    if(luaL_newmetatable(L,etf_pid_mt)) {
        lua_pushstring(L,etf_pid_mt);
        lua_setfield(L,-2,"__name");
    }
    lua_setfield(L,-2,"pid_mt");

    if(luaL_newmetatable(L,etf_export_mt)) {
        lua_pushstring(L,etf_export_mt);
        lua_setfield(L,-2,"__name");
    }
    lua_setfield(L,-2,"export_mt");

    if(luaL_newmetatable(L,etf_reference_mt)) {
        lua_pushstring(L,etf_reference_mt);
        lua_setfield(L,-2,"__name");
    }
    lua_setfield(L,-2,"reference_mt");

    if(luaL_newmetatable(L,etf_tuple_mt)) {
        lua_pushstring(L,etf_tuple_mt);
        lua_setfield(L,-2,"__name");
    }
    lua_setfield(L,-2,"tuple_mt");

    if(luaL_newmetatable(L,etf_list_mt)) {
        lua_pushstring(L,etf_list_mt);
        lua_setfield(L,-2,"__name");
    }
    lua_setfield(L,-2,"list_mt");

    if(luaL_newmetatable(L,etf_map_mt)) {
        lua_pushstring(L,etf_map_mt);
        lua_setfield(L,-2,"__name");
    }
    lua_setfield(L,-2,"map_mt");

    if(luaL_newmetatable(L,etf_new_fun_mt)) {
        lua_pushstring(L,etf_new_fun_mt);
        lua_setfield(L,-2,"__name");
    }
    lua_setfield(L,-2,"new_fun_mt");

    if(luaL_newmetatable(L,etf_fun_mt)) {
        lua_pushstring(L,etf_fun_mt);
        lua_setfield(L,-2,"__name");
    }
    lua_setfield(L,-2,"fun_mt");

    lua_pushinteger(L,ETF_VERSION_MAJOR);
    lua_setfield(L,-2,"_VERSION_MAJOR");
    lua_pushinteger(L,ETF_VERSION_MINOR);
    lua_setfield(L,-2,"_VERSION_MINOR");
    lua_pushinteger(L,ETF_VERSION_PATCH);
    lua_setfield(L,-2,"_VERSION_PATCH");
    lua_pushliteral(L,ETF_VERSION);
    lua_setfield(L,-2,"_VERSION");

    lua_getglobal(L,"tonumber");
    lua_pushcclosure(L, etf_float, 1);
    lua_setfield(L,-2,"float");

    lua_getglobal(L,"tostring");

    lua_pushvalue(L,-1);
    lua_pushcclosure(L,etf_string,1);
    lua_setfield(L,-3,"string");

    lua_pushvalue(L,-1);
    lua_pushcclosure(L,etf_binary,1);
    lua_setfield(L,-3,"binary");

    lua_pushvalue(L,-1);
    lua_pushcclosure(L,etf_atom,1);
    lua_setfield(L,-3,"atom");

    lua_pushvalue(L,-1);
    lua_pushcclosure(L,etf_export,1);
    lua_setfield(L,-3,"export");

    lua_pushvalue(L,-1);
    lua_pushcclosure(L,etf_port,1);
    lua_setfield(L,-3,"port");

    lua_pushvalue(L,-1);
    lua_pushcclosure(L,etf_pid,1);
    lua_setfield(L,-3,"pid");

    lua_pushvalue(L,-1);
    lua_pushcclosure(L,etf_reference,1);
    lua_setfield(L,-3,"reference");

    lua_pop(L,1);

    lua_pushinteger(L,sizeof(lua_Number));
    lua_setfield(L,-2,"numsize");

    luaL_setfuncs(L,etf_functions,0);
    lua_getfield(L,-1,"atom");
    lua_pushliteral(L,"nil");
    lua_call(L,1,1);
    lua_setfield(L,-2,"null");

    /* find our min/max for integers */
    b_min = etf_pushbigint(L);
    lua_setfield(L,-2,"mininteger");

    b_max = etf_pushbigint(L);
    lua_setfield(L,-2,"maxinteger");

    lua_getfield(L,-1,"mininteger");
    lua_getfield(L,-2,"maxinteger");

    i_min = (int64_t *)lua_newuserdata(L,sizeof(int64_t));
    if(i_min == NULL) {
        return luaL_error(L,"out of memory");
    }
    *i_min = 0;

    i_max = (int64_t *)lua_newuserdata(L,sizeof(int64_t));
    if(i_max == NULL) {
        return luaL_error(L,"out of memory");
    }
    *i_max = 0;

#if defined(LUA_MAXINTEGER)
    if(bigint_from_i64(b_min,(int64_t)LUA_MININTEGER)) return luaL_error(L,"out of memory");
    if(bigint_from_i64(b_max,(int64_t)LUA_MAXINTEGER)) return luaL_error(L,"out of memory");
    *i_min = LUA_MININTEGER;
    *i_max = LUA_MAXINTEGER;
    (void)eq;
#else
    /* test for using 32-bit floats as integers */
    lua_pushinteger(L,16777216);
    lua_pushinteger(L,16777217);
    eq = lua_rawequal(L,-2,-1);
    lua_pop(L,2);

    if(eq) {
        if(bigint_from_i64(b_min,(int64_t)-16777216)) return luaL_error(L,"out of memory");
        if(bigint_from_i64(b_max,(int64_t) 16777216)) return luaL_error(L,"out of memory");
        *i_min = -16777216LL;
        *i_max =  16777216LL;
    } else {
        /* assume we're using double */
        if(bigint_from_i64(b_min,(int64_t)-9007199254740992LL)) return luaL_error(L,"out of memory");
        if(bigint_from_i64(b_max,(int64_t) 9007199254740992LL)) return luaL_error(L,"out of memory");
        *i_min = -9007199254740992LL;
        *i_max =  9007199254740992LL;
    }
#endif

    /* create our default atom -> value mapping */
    lua_newtable(L);
    lua_getfield(L,-6,"null");
    lua_setfield(L,-2,"nil");
    lua_pushboolean(L,1);
    lua_setfield(L,-2,"true");
    lua_pushboolean(L,0);
    lua_setfield(L,-2,"false");

    /* stack is now:
     * -5 bigint
     * -4 bigint
     * -3 userdata
     * -2 userdata
     * -1 default atom mapping */

    lua_newtable(L);
    while(decoders->func != NULL) {
        for(i=0;i<5;i++) {
            lua_pushvalue(L,-(5 + 1));
        }
        lua_pushcclosure(L, decoders->func, 5);
        lua_rawseti(L,-2,decoders->version);
        decoders++;
    }
    lua_insert(L,lua_gettop(L)-5);
    lua_pop(L,5);

    /* top of the stack is a table mapping values to decoders */
    lua_pushcclosure(L, etf_decoder_new, 1);
    lua_setfield(L,-2,"decoder");

    /* convenience "decode" function */
    lua_getfield(L,-1,"decoder");
    lua_pushcclosure(L,etf_decode,1);
    lua_setfield(L,-2,"decode");

    /* create our value -> atom mapping for booleans */
    lua_newtable(L);
    lua_pushboolean(L,1);
    lua_newtable(L);
    lua_pushstring(L,"true");
    lua_setfield(L,-2,"atom");
    lua_pushboolean(L,1);
    lua_setfield(L,-2,"utf8");
    luaL_setmetatable(L,etf_atom_mt);
    lua_settable(L,-3);

    lua_pushboolean(L,0);
    lua_newtable(L);
    lua_pushstring(L,"false");
    lua_setfield(L,-2,"atom");
    lua_pushboolean(L,1);
    lua_setfield(L,-2,"utf8");
    luaL_setmetatable(L,etf_atom_mt);
    lua_settable(L,-3);

    lua_getglobal(L,"tostring");

    /* stack is now:
     * -2 default value map
     * -1 tostring
     */
    lua_newtable(L);
    while(encoders->func != NULL) {
        for(i=0;i<2;i++) {
            lua_pushvalue(L,-(1 + 2));
        }
        lua_newtable(L);
        meta = encoders->metas;
        while(meta->name != NULL) {
            luaL_getmetatable(L, meta->name);
            lua_pushlightuserdata(L,meta->func);
            lua_settable(L,-3);
            meta++;
        }
        lua_pushcclosure(L, encoders->func, 3);
        lua_rawseti(L,-2, encoders->version);
        encoders++;
    }
    lua_insert(L,lua_gettop(L)-2);
    lua_pop(L,2);

    lua_pushcclosure(L, etf_encoder_new, 1);
    lua_setfield(L,-2,"encoder");

    lua_getfield(L,-1,"encoder");
    lua_pushcclosure(L, etf_encode, 1);
    lua_setfield(L,-2,"encode");

    return 1;
}

#ifdef __cplusplus
}
#endif

#include "thirdparty/miniz/miniz.c"

/*

The MIT License (MIT)

Copyright (c) 2023 John Regan <john@jrjrtech.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/
