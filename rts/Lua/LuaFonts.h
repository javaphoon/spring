#ifndef LUA_FONTS_H
#define LUA_FONTS_H
// LuaFonts.h: interface for the LuaFonts class.
//
//////////////////////////////////////////////////////////////////////

#include "Rendering/GL/myGL.h"
#include "Rendering/glFont.h"

struct lua_State;


class LuaFonts {
	public:
		LuaFonts();
		~LuaFonts();

		const void* GetMetatable() const { return metatable; }

	public:
		static bool PushEntries(lua_State* L);

	private:
		const void* metatable;

	private: // helpers
		static bool CreateMetatable(lua_State* L);

	private: // metatable methods
		static int meta_gc(lua_State* L);
		static int meta_index(lua_State* L);

	private: // call-outs
		static int LoadFont(lua_State* L);
		static int DeleteFont(lua_State* L);

	private: // userdata call-outs
		static int Print(lua_State* L);

		static int Begin(lua_State* L);
		static int End(lua_State* L);

		static int WrapText(lua_State* L);

		static int GetTextWidth(lua_State* L);
		static int GetTextHeight(lua_State* L);

		static int SetTextColor(lua_State* L);
		static int SetOutlineColor(lua_State* L);
		static int SetAutoOutlineColor(lua_State* L);

		static int BindTexture(lua_State* L);
};


#endif /* LUA_FONTS_H */
