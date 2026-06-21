#include "STDInc.hpp"

/*
	TODO LIST:
	- Make the refactoring where possible!!!
	- Fix label (char) for command execution
	- Fix the graphcs for DevGUI
	- Add safe area for DevGUI
*/
namespace Components
{
	Game::devguiGlob_t devguiGlob;
	Game::ClGuiGlob clGuiGlob;
	Game::DevGuiInput s_input;

	int32_t s_butMapsKey[11] =
	{
		Game::K_UPARROW,
		Game::K_DOWNARROW,
		Game::K_LEFTARROW,
		Game::K_RIGHTARROW,
		Game::K_ENTER,
		Game::K_ESCAPE,
		Game::K_TAB,
		Game::K_SPACE,
		Game::K_INS,
		Game::K_DEL,
		Game::K_F5
	};

	const char* MYINSTRUCTIONS = "<Y> Edits node,  <LB> Adds new node,  <RB> Removes node,  <START> Saves to disk";

	int Devgui::DevGui_GetScreenWidth()
	{
		return Game::cls->vidConfig.displayWidth;
	}

	int Devgui::DevGui_GetScreenHeight()
	{
		return Game::cls->vidConfig.displayHeight;
	}

	void Devgui::DevGui_RegisterDvars()
	{
		Dvars::devgui_colorBgnd = Dvars::Register::Dvar_RegisterColor("devgui_colorBgnd", 0.0f, 0.4f, 0.0f, 0.75f, Game::dvar_flags::none, "Color background for the devgui");
		Dvars::devgui_colorText = Dvars::Register::Dvar_RegisterColor("devgui_colorText", 1.0f, 1.0f, 1.0f, 1.0f, Game::dvar_flags::none, "Text color for the devgui");
		Dvars::devgui_colorBgndSel = Dvars::Register::Dvar_RegisterColor("devgui_colorBgndSel", 0.0f, 0.7f, 0.0f, 0.75f, Game::dvar_flags::none, "Selection background color for the devgui");
		Dvars::devgui_colorTextSel = Dvars::Register::Dvar_RegisterColor("devgui_colorTextSel", 1.0f, 1.0f, 0.0f, 1.0f, Game::dvar_flags::none, "Selection text color for the devgui");
		Dvars::devgui_colorBgndGray = Dvars::Register::Dvar_RegisterColor("devgui_colorBgndGray", 0.2f, 0.2f, 0.2f, 0.9f, Game::dvar_flags::none, "Grayed out background color for the devgui");
		Dvars::devgui_colorTextGray = Dvars::Register::Dvar_RegisterColor("devgui_colorTextGray", 0.7f, 0.7f, 0.7f, 1.0f, Game::dvar_flags::none, "Greyed out text color for the devgui");
		Dvars::devgui_colorBgndGraySel = Dvars::Register::Dvar_RegisterColor("devgui_colorBgndGraySel", 0.4f, 0.4f, 0.4f, 0.9f, Game::dvar_flags::none, "Greyed out, selected background color for the devgui");
		Dvars::devgui_colorTextGraySel = Dvars::Register::Dvar_RegisterColor("devgui_colorTextGraySel", 1.0f, 1.0f, 0.0f, 1.0f, Game::dvar_flags::none, "Greyed out, selected text color for the devgui");
		Dvars::devgui_colorSliderBgnd = Dvars::Register::Dvar_RegisterColor("devgui_colorSliderBgnd", 1.0f, 1.0f, 1.0f, 0.75f, Game::dvar_flags::none, "Color slider background for the devgui");
		Dvars::devgui_colorSliderKnob = Dvars::Register::Dvar_RegisterColor("devgui_colorSliderKnob", 1.0f, 1.0f, 1.0f, 1.0f, Game::dvar_flags::none, "Knob color for the devgui");
		Dvars::devgui_colorSliderKnobSel = Dvars::Register::Dvar_RegisterColor("devgui_colorSliderKnobSel", 1.0f, 1.0f, 0.0f, 1.0f, Game::dvar_flags::none, "Selected knob color for the devgui");
		Dvars::devgui_bevelShade = Dvars::Register::Dvar_RegisterFloat("devgui_bevelShade", "Bevel shade for the devgui", 0.7f, 0.0f, 1.0f, Game::dvar_flags::none);
		Dvars::devgui_colorGraphKnotNormal = Dvars::Register::Dvar_RegisterColor("devgui_colorGraphKnotNormal", 0.0f, 1.0f, 1.0f, 0.7f, Game::dvar_flags::none, "Devgiu Color graph knot normal color");
		Dvars::devgui_colorGraphKnotSelected = Dvars::Register::Dvar_RegisterColor("devgui_colorGraphKnotSelected", 1.0f, 0.0f, 0.0f, 0.7f, Game::dvar_flags::none, "Devgui color graph knot selected color");
		Dvars::devgui_colorGraphKnotEditing = Dvars::Register::Dvar_RegisterColor("devgui_colorGraphKnotEditing", 1.0f, 0.0f, 1.0f, 1.0f, Game::dvar_flags::none, "Devgui color graph knot editing color");
	}

	Game::devguiGlob_t* Devgui::DevGui_GetMenu(uint16_t handle)
	{
		return (Game::devguiGlob_t*)((char*)&devguiGlob + 40 * handle - 40);
	}

	void Devgui::DevGui_AdvanceChildNum(int32_t numberToAdvance)
	{
		Game::devguiGlob_t* menu;

		for (int32_t numberIter = 0; numberIter != numberToAdvance; ++numberIter)
		{
			menu = DevGui_GetMenu(devguiGlob.selectedMenu);
			if (!menu->menus[0].nextSibling)
				break;
			devguiGlob.selectedMenu = menu->menus[0].nextSibling;
			DevGui_GetMenu(devguiGlob.selectedMenu);
		}
	}

	void Devgui::DevGui_SelectTopLevelChild()
	{
		Game::devguiGlob_t* menu;

		menu = DevGui_GetMenu(devguiGlob.selectedMenu);
		if (!menu->menus[0].parent)
		{
			devguiGlob.selectedMenu = menu->menus[0].child.menu;
			DevGui_AdvanceChildNum(menu->menus[0].childMenuMemory);
		}
	}

	uint16_t Devgui::DevGui_GetMenuParent(uint16_t handle)
	{
		return DevGui_GetMenu(handle)->menus[0].parent;
	}

	int32_t Devgui::DevGui_SubMenuTextWidth()
	{
		return Game::R_TextWidth(" >", 0, Game::sharedUiInfo->assets.consoleFont);
	}

	int32_t Devgui::DevGui_MenuItemWidth(const Game::DevMenuItem* menu)
	{
		if (!menu) return 0;

		int32_t width = Game::R_TextWidth(menu->label, 0, Game::sharedUiInfo->assets.consoleFont) + 8;
		if (menu->childType == 0 && menu->child.menu)
			width += DevGui_SubMenuTextWidth();
		return width;
	}

	int32_t Devgui::DevGui_MaxChildMenuWidth(const Game::DevMenuItem* menu)
	{
		Game::devguiGlob_t* childMenu; // [esp+0h] [ebp-10h]
		int32_t widthCur; // [esp+4h] [ebp-Ch]
		int32_t widthMax; // [esp+8h] [ebp-8h]
		uint16_t childHandle; // [esp+Ch] [ebp-4h]

		widthMax = DevGui_MenuItemWidth(menu);
		for (childHandle = menu->child.menu; childHandle; childHandle = childMenu->menus[0].nextSibling)
		{
			childMenu = DevGui_GetMenu(childHandle);
			widthCur = DevGui_MenuItemWidth(childMenu->menus);
			if (widthMax < widthCur)
				widthMax = widthCur;
		}
		return widthMax;
	}

	void Devgui::DevGui_ChooseOrigin(int32_t* origin)
	{
		uint16_t handle; // [esp+0h] [ebp-18h]
		Game::devguiGlob_t* childMenu; // [esp+4h] [ebp-14h]
		Game::devguiGlob_t* menu; // [esp+8h] [ebp-10h]
		uint16_t activeChild; // [esp+Ch] [ebp-Ch]
		uint16_t childHandle; // [esp+10h] [ebp-8h]
		int32_t w; // [esp+14h] [ebp-4h]

		w = 0;
		activeChild = devguiGlob.selectedMenu;
		for (handle = DevGui_GetMenuParent(devguiGlob.selectedMenu); handle; handle = menu->menus[0].parent)
		{
			menu = DevGui_GetMenu(handle);
			w += DevGui_MaxChildMenuWidth(menu->menus);
			activeChild = handle;
		}
		for (childHandle = devguiGlob.topmostMenu.child.menu;
			childHandle != activeChild;
			childHandle = childMenu->menus[0].nextSibling)
		{
			childMenu = DevGui_GetMenu(childHandle);
			w += DevGui_MenuItemWidth(childMenu->menus);
		}
		*origin = devguiGlob.left;
		origin[1] = devguiGlob.top;
		if (w > devguiGlob.right - devguiGlob.left)
			*origin -= w - (devguiGlob.right - devguiGlob.left);
	}

	int32_t Devgui::DevGui_GetFontHeight()
	{
		return Game::R_TextHeight(Game::sharedUiInfo->assets.consoleFont);
	}

	int32_t Devgui::DevGui_GetFontWidth(const char* text)
	{
		return Game::R_TextWidth(text, 0, Game::sharedUiInfo->assets.consoleFont);
	}

	void Devgui::DevGui_DrawFont(int32_t x, int32_t y, const uint8_t* color, const char* text)
	{
		float unpackedColor[4];

		if (*text)
		{
			Game::Byte4UnpackRgba(color, unpackedColor);
			Game::R_AddCmdDrawTextASM(text, 0x7FFFFFFF, Game::sharedUiInfo->assets.consoleFont, x, static_cast<float>(y + DevGui_GetFontHeight()), 1.0, 1.0, 0.0, unpackedColor, 0);
		}
	}

	void Devgui::DevGui_DrawBox(int32_t x, int32_t y, int32_t w, int32_t h, const uint8_t* color)
	{
		float unpackedColor[4];
		Game::Byte4UnpackRgba(color, unpackedColor);
		Game::R_AddCmdDrawStretchPic(Game::sharedUiInfo->assets.whiteMaterial, x, y, w, h, 0.0, 0.0, 0.0, 0.0, unpackedColor);
	}

	void Devgui::DevGui_DrawQuad(int32_t(*vtxs)[2], float* color)
	{
		float xy[4][2];

		for (int i = 0; i < 4; ++i) 
		{
			xy[i][0] = static_cast<float>(vtxs[i][0]);
			xy[i][1] = static_cast<float>(vtxs[i][1]);
		}

		Game::R_AddCmdDrawQuadPic(xy, color, Game::sharedUiInfo->assets.whiteMaterial);
	}

	void Devgui::DevGui_DrawBevelBox(int32_t x, int32_t y, int32_t w, int32_t h, float shade, const uint8_t* color)
	{
		float unpackedColor[4];
		int32_t vertices[4][2];

		Game::Byte4UnpackRgba(color, unpackedColor);
		DevGui_DrawBox(x, y, w, h, color);
		Game::Vec3Scale(unpackedColor, shade, unpackedColor);

		for (int i = 0; i < 3; ++i) 
		{
			unpackedColor[i] = std::max(0.0f, std::min(unpackedColor[i], 1.0f));
		}

		vertices[0][0] = x;
		vertices[0][1] = y;
		vertices[1][0] = x + 4;
		vertices[1][1] = y + 4;
		vertices[2][0] = x + 4;
		vertices[2][1] = y + h - 4;
		vertices[3][0] = x;
		vertices[3][1] = h + y;
		DevGui_DrawQuad(vertices, unpackedColor);

		Game::Vec3Scale(unpackedColor, shade, unpackedColor);
		for (int i = 0; i < 3; ++i) 
		{
			unpackedColor[i] = std::max(0.0f, std::min(unpackedColor[i], 1.0f));
		}

		vertices[0][0] = x;
		vertices[0][1] = y;
		vertices[1][0] = w + x;
		vertices[1][1] = y;
		vertices[2][0] = x + w - 4;
		vertices[2][1] = y + 4;
		vertices[3][0] = x + 4;
		vertices[3][1] = y + 4;
		DevGui_DrawQuad(vertices, unpackedColor);

		Game::Vec3Scale(unpackedColor, shade, unpackedColor);
		for (int i = 0; i < 3; ++i) 
		{
			unpackedColor[i] = std::max(0.0f, std::min(unpackedColor[i], 1.0f));
		}

		vertices[0][0] = x;
		vertices[0][1] = h + y;
		vertices[1][0] = x + 4;
		vertices[1][1] = y + h - 4;
		vertices[2][0] = x + w - 4;
		vertices[2][1] = vertices[1][1];
		vertices[3][0] = w + x;
		vertices[3][1] = h + y;
		DevGui_DrawQuad(vertices, unpackedColor);

		Game::Vec3Scale(unpackedColor, shade, unpackedColor);
		for (int i = 0; i < 3; ++i) 
		{
			unpackedColor[i] = std::max(0.0f, std::min(unpackedColor[i], 1.0f));
		}

		vertices[0][0] = w + x;
		vertices[0][1] = y;
		vertices[1][0] = w + x;
		vertices[1][1] = h + y;
		vertices[2][0] = x + w - 4;
		vertices[2][1] = y + h - 4;
		vertices[3][0] = vertices[2][0];
		vertices[3][1] = y + 4;
		DevGui_DrawQuad(vertices, unpackedColor);
	}

	void Devgui::DevGui_DrawMenuHorizontally(const Game::DevMenuItem* menu, uint16_t activeChild, int32_t* origin)
	{
		Game::devguiGlob_t* childMenu; // [esp+8h] [ebp-20h]
		uint8_t bgndColor[4]; 
		uint8_t textColor[4];
		int32_t x; // [esp+14h] [ebp-14h]
		int32_t y; // [esp+18h] [ebp-10h]
		int32_t h; // [esp+1Ch] [ebp-Ch]
		uint16_t childHandle; // [esp+20h] [ebp-8h]
		int32_t w; // [esp+24h] [ebp-4h]

		x = *origin;
		y = origin[1];
		h = Game::R_TextHeight(Game::sharedUiInfo->assets.consoleFont) + 8;
		for (childHandle = menu->child.menu; childHandle; childHandle = childMenu->menus[0].nextSibling)
		{
			childMenu = DevGui_GetMenu(childHandle);
			w = DevGui_MenuItemWidth(childMenu->menus);
			if (childHandle == activeChild)
			{
				*origin = x;
				origin[1] = h + y;
				*(DWORD*)bgndColor = Dvars::devgui_colorBgndSel->current.integer;
				*(DWORD*)textColor = Dvars::devgui_colorTextSel->current.integer;
			}
			else
			{
				*(DWORD*)bgndColor = Dvars::devgui_colorBgnd->current.integer;
				*(DWORD*)textColor = Dvars::devgui_colorText->current.integer;
			}
			DevGui_DrawBevelBox(x, y, w, h, Dvars::devgui_bevelShade->current.value, bgndColor);
			DevGui_DrawFont(x + 4, y + 4, textColor, (char*)childMenu);
			x += w;
		}
	}

	bool Devgui::DevGui_EditableMenuItem(const Game::DevMenuItem* menu)
	{
		if (menu->childType == 3)
			return 1;
		if (menu->childType != 1)
			return 0;
		if (*((BYTE*)menu->child.command + 10) == 7)
			return 0;
		return *((BYTE*)menu->child.command + 10) != 6 || *((uint32_t*)menu->child.command + 15);
	}

	bool Devgui::DevGui_MenuItemDisabled(const Game::DevMenuItem* menu)
	{
		return menu->childType == 1 && !DevGui_EditableMenuItem(menu);
	}

	bool Devgui::DevGui_IsButtonDown(Game::DevGuiInputButton button)
	{
		return s_input.buttonDown[button];
	}

	void Devgui::DevGui_DrawMenuVertically(const Game::DevMenuItem* menu, uint16_t activeChild, int32_t* origin)
	{
		Game::devguiGlob_t* childMenu; // [esp+Ch] [ebp-38h]
		Game::devguiGlob_t* childMenua; // [esp+Ch] [ebp-38h]
		int32_t activeChildIndex; // [esp+10h] [ebp-34h]
		uint8_t bgndColor[4]; // [esp+18h] [ebp-2Ch] BYREF
		uint8_t textColor[4]; // [esp+1Ch] [ebp-28h] BYREF
		int32_t subMenuStringPos; // [esp+20h] [ebp-24h]
		int32_t visibleMenuCount; // [esp+24h] [ebp-20h]
		int32_t x; // [esp+28h] [ebp-1Ch]
		int32_t y; // [esp+2Ch] [ebp-18h]
		int32_t h; // [esp+30h] [ebp-14h]
		float shade; // [esp+34h] [ebp-10h]
		int32_t childCount; // [esp+38h] [ebp-Ch]
		uint16_t childHandle; // [esp+3Ch] [ebp-8h]
		int32_t w; // [esp+40h] [ebp-4h]

		x = *origin;
		y = origin[1];
		w = DevGui_MaxChildMenuWidth(menu);
		h = Game::R_TextHeight(Game::sharedUiInfo->assets.consoleFont) + 8;
		subMenuStringPos = x + w - 4 - DevGui_SubMenuTextWidth();
		childCount = 0;
		activeChildIndex = 0;
		for (childHandle = menu->child.menu; childHandle; childHandle = childMenu->menus[0].nextSibling)
		{
			childMenu = DevGui_GetMenu(childHandle);
			if (childHandle == activeChild)
				activeChildIndex = childCount;
			++childCount;
		}
		visibleMenuCount = (devguiGlob.bottom - 2 * h - y) / h;
		if (visibleMenuCount < childCount && activeChildIndex > visibleMenuCount)
		{
			*(DWORD*)bgndColor = Dvars::devgui_colorBgnd->current.integer;
			*(DWORD*)textColor = Dvars::devgui_colorText->current.integer;
			shade = Dvars::devgui_bevelShade->current.value;
			DevGui_DrawBevelBox(x, y, w, h, shade, bgndColor);
			DevGui_DrawFont(x + 4, y + 4, textColor, (char*)"...");
			y += h;
		}
		childCount = 0;
		for (childHandle = menu->child.menu; childHandle; childHandle = childMenua->menus[0].nextSibling)
		{
			childMenua = DevGui_GetMenu(childHandle);
			if (activeChildIndex - visibleMenuCount <= childCount)
			{
				if (childHandle == activeChild)
				{
					*origin = w + x;
					origin[1] = y;
					if (DevGui_MenuItemDisabled(childMenua->menus))
					{
						*(DWORD*)bgndColor = Dvars::devgui_colorBgndGraySel->current.integer;
						*(DWORD*)textColor = Dvars::devgui_colorTextGraySel->current.integer;
					}
					else
					{
						*(DWORD*)bgndColor = Dvars::devgui_colorBgndSel->current.integer;
						*(DWORD*)textColor = Dvars::devgui_colorTextSel->current.integer;
					}
					if (childHandle == devguiGlob.selectedMenu && DevGui_IsButtonDown(Game::INPUT_ACCEPT))
						shade = 2.0 - Dvars::devgui_bevelShade->current.value;
					else
						shade = Dvars::devgui_bevelShade->current.value;
				}
				else
				{
					if (DevGui_MenuItemDisabled(childMenua->menus))
					{
						*(DWORD*)bgndColor = Dvars::devgui_colorBgndGray->current.integer;
						*(DWORD*)textColor = Dvars::devgui_colorTextGray->current.integer;
					}
					else
					{
						*(DWORD*)bgndColor = Dvars::devgui_colorBgnd->current.integer;
						*(DWORD*)textColor = Dvars::devgui_colorText->current.integer;
					}
					shade = Dvars::devgui_bevelShade->current.value;
				}
				DevGui_DrawBevelBox(x, y, w, h, shade, bgndColor);
				DevGui_DrawFont(x + 4, y + 4, textColor, (char*)childMenua);
				if (!childMenua->menus[0].childType && childMenua->menus[0].child.menu)
					DevGui_DrawFont(subMenuStringPos, y + 4, textColor, (char*)" >");
				y += h;
				++childCount;
			}
			else
			{
				++childCount;
			}
		}
	}

	void Devgui::DevGui_DrawMenu(uint16_t menuHandle, uint16_t activeChild, int32_t* origin)
	{
		Game::devguiGlob_t* menu; // [esp+0h] [ebp-4h]

		menu = DevGui_GetMenu(menuHandle);
		if (menu->menus[0].parent)
			DevGui_DrawMenu(menu->menus[0].parent, menuHandle, origin);
		else
			DevGui_DrawMenuHorizontally(&devguiGlob.topmostMenu, menuHandle, origin);
		DevGui_DrawMenuVertically(menu->menus, activeChild, origin);
	}

	bool Devgui::DevGui_IsActive()
	{
		return devguiGlob.isActive;
	}

	int32_t Devgui::DevGui_DvarRowCount(const Game::dvar_s* dvar) 
	{
		if (dvar == nullptr)
			return 0;

		switch (dvar->type) 
		{
			case Game::dvar_type::vec2: return 2;
			case Game::dvar_type::vec3: return 3;
			case Game::dvar_type::vec4:
			case Game::dvar_type::color: return 4;
			default: return 1;
		}
	}

	int32_t Devgui::DevGui_GetSliderPath(uint16_t menuHandle, char* path, int32_t pathLen)
	{
		int32_t SliderPath; // eax
		uint32_t v5; // [esp+0h] [ebp-18h]
		Game::devguiGlob_t* menu; // [esp+10h] [ebp-8h]

		menu = DevGui_GetMenu(menuHandle);
		if (menu->menus[0].parent)
		{
			SliderPath = DevGui_GetSliderPath(menu->menus[0].parent, path, pathLen);
			path[SliderPath] = 47;
			pathLen = SliderPath + 1;
		}
		v5 = strlen((const char*)menu);
		memcpy((uint8_t*)&path[pathLen], (uint8_t*)menu, v5 + 1);
		return v5 + pathLen;
	}

	void Devgui::DevGui_DrawSliderPath(int32_t x, int32_t y)
	{
		char path[132]; // [esp+0h] [ebp-88h] BYREF

		DevGui_GetSliderPath(devguiGlob.selectedMenu, path, 0);
		DevGui_DrawFont(x, y, (const uint8_t*)&Dvars::devgui_colorText->current, path);
	}

	void Devgui::DevGui_DrawSingleSlider(int32_t x, int32_t y, int32_t rowWidth, int32_t rowHeight, float fraction, const uint8_t* knobColor)
	{
		DevGui_DrawBevelBox(
			x,
			y,
			rowWidth,
			rowHeight,
			Dvars::devgui_bevelShade->current.value,
			(const uint8_t*)&Dvars::devgui_colorSliderBgnd->current);
		DevGui_DrawBevelBox((int)(fraction * (float)(rowWidth - 8) + (float)x), y, 8, rowHeight, Dvars::devgui_bevelShade->current.value, knobColor);
	}

	void Devgui::DevGui_DrawDvarValue(int32_t x, int32_t y, const Game::dvar_s* dvar)
	{
		const char* v3; // eax
		const char* v4; // [esp+0h] [ebp-8h]
		const char* text; // [esp+4h] [ebp-4h]

		if ((bool)dvar->type)
		{
			if (dvar->type == Game::dvar_type::enumeration)
			{
				v3 = Dvars::Functions::Dvar_ValueToString(dvar, dvar->latched);
				text = Utils::String::VA("%i: %s", dvar->latched.integer, v3);
			}
			else
			{
				text = Dvars::Functions::Dvar_ValueToString(dvar, dvar->latched);
			}
			DevGui_DrawFont(x, y, (const uint8_t*)&Dvars::devgui_colorText->current, text);
		}
		else
		{
			if (dvar->latched.enabled)
				v4 = "On";
			else
				v4 = "Off";
			DevGui_DrawFont(x, y, (const uint8_t*)&Dvars::devgui_colorText->current, (char*)v4);
		}
	}


	void Devgui::DevGui_DrawSliders(const Game::DevMenuItem* menu)
	{
		//const char* v1; // eax
		//const char* v2; // eax
		float v3; // [esp+14h] [ebp-3Ch]
		float* v4; // [esp+18h] [ebp-38h]
		float* p_current; // [esp+1Ch] [ebp-34h]
		int32_t rowHeight; // [esp+24h] [ebp-2Ch]
		int32_t width; // [esp+2Ch] [ebp-24h]
		int32_t height; // [esp+30h] [ebp-20h]
		float fractiona; // [esp+34h] [ebp-1Ch]
		float fractionb; // [esp+34h] [ebp-1Ch]
		float fraction; // [esp+34h] [ebp-1Ch]
		int32_t rowCount; // [esp+38h] [ebp-18h]
		int32_t row; // [esp+3Ch] [ebp-14h]
		int32_t rowa; // [esp+3Ch] [ebp-14h]
		const Game::dvar_s* dvar; // [esp+40h] [ebp-10h]
		int32_t xa; // [esp+44h] [ebp-Ch]
		int32_t x; // [esp+44h] [ebp-Ch]
		int32_t x_4a; // [esp+48h] [ebp-8h]
		int32_t x_4; // [esp+48h] [ebp-8h]
		int32_t rowWidth; // [esp+4Ch] [ebp-4h]

		dvar = menu->child.dvar;
		rowWidth = devguiGlob.sliderWidth;
		rowHeight = DevGui_GetFontHeight();
		rowCount = DevGui_DvarRowCount(dvar);
		width = rowWidth + 8;
		height = rowHeight * (rowCount + 2) + 2 * rowCount + 14;
		if (dvar->type == Game::dvar_type::color)
			height += rowHeight + 2;
		xa = devguiGlob.left + (devguiGlob.right - devguiGlob.left - width) / 2;
		x_4a = devguiGlob.bottom - height;
		DevGui_DrawBevelBox(
			xa,
			devguiGlob.bottom - height,
			width,
			height,
			Dvars::devgui_bevelShade->current.value,
			(const uint8_t*)&Dvars::devgui_colorBgnd->current);
		x = xa + 4;
		x_4 = x_4a + 6;
		DevGui_DrawSliderPath(x, x_4);
		if (dvar->type == Game::dvar_type::color)
		{
			x_4 += rowHeight + 2;
			DevGui_DrawBox(x, x_4, rowWidth, rowHeight, (const uint8_t*)&dvar->latched);
			for (row = 0; row < rowCount; ++row)
			{
				x_4 += rowHeight + 2;
				if (row == devguiGlob.selRow)
					p_current = Dvars::devgui_colorSliderKnobSel->current.vector;
				else
					p_current = Dvars::devgui_colorSliderKnob->current.vector;
				fractiona = (double)dvar->latched.color[row] * 1.0 / 255.0;
				DevGui_DrawSingleSlider(x, x_4, rowWidth, rowHeight, fractiona, (const uint8_t*)p_current);
			}
		}
		//dvar->type == Game::dvar_type::color
		else if (dvar->type == Game::dvar_type::vec2 || dvar->type == Game::dvar_type::vec3 || dvar->type == Game::dvar_type::vec4)
		{
			for (rowa = 0; rowa < rowCount; ++rowa)
			{
				x_4 += rowHeight + 2;
				if (rowa == devguiGlob.selRow)
					v4 = Dvars::devgui_colorSliderKnobSel->current.vector;
				else
					v4 = Dvars::devgui_colorSliderKnob->current.vector;
				fractionb = (dvar->latched.vector[rowa] - dvar->domain.value.min)
					/ (dvar->domain.value.max - dvar->domain.value.min);
				DevGui_DrawSingleSlider(x, x_4, rowWidth, rowHeight, fractionb, (const uint8_t*)v4);
			}
		}
		else
		{
			x_4 += rowHeight + 2;
			if ((int)dvar->type)
			{
				switch (dvar->type)
				{
				case Game::dvar_type::integer:
					if (dvar->domain.integer.max == dvar->domain.enumeration.stringCount)
						fraction = 0.5;
					else
						fraction = (double)(dvar->latched.integer - dvar->domain.enumeration.stringCount)
						/ (double)(dvar->domain.integer.max - dvar->domain.enumeration.stringCount);
					break;
				case Game::dvar_type::enumeration:
					if (dvar->domain.enumeration.stringCount > 1)
						fraction = (double)dvar->latched.integer / (double)(dvar->domain.enumeration.stringCount - 1);
					else
						fraction = 0.5;
					break;
				case Game::dvar_type::value:
					fraction = (dvar->latched.value - dvar->domain.value.min) / (dvar->domain.value.max - dvar->domain.value.min);
					break;
				default:
					fraction = 0.5;
					break;
				}
			}
			else
			{
				if (dvar->latched.enabled)
					v3 = 1.0;
				else
					v3 = 0.0;
				fraction = v3;
			}
			DevGui_DrawSingleSlider(
				x,
				x_4,
				rowWidth,
				rowHeight,
				fraction,
				(const uint8_t*)&Dvars::devgui_colorSliderKnobSel->current);
		}
		DevGui_DrawDvarValue(x, x_4 + rowHeight + 2, dvar);
	}

	void Devgui::DevGui_DrawBoxCentered(int32_t centerX, int32_t centerY, int32_t w, int32_t h, const uint8_t* color)
	{
		DevGui_DrawBox(centerX - w / 2, centerY - h / 2, w, h, color);
	}

	void Devgui::DevGui_DrawLine(float* start, float* end, int32_t width, const uint8_t* color)
	{
		float h; // [esp+Ch] [ebp-C0h]
		float x; // [esp+2Ch] [ebp-A0h]
		float y; // [esp+30h] [ebp-9Ch]
		float v7; // [esp+38h] [ebp-94h]
		float v8; // [esp+3Ch] [ebp-90h]
		float v9; // [esp+40h] [ebp-8Ch]
		float v10; // [esp+44h] [ebp-88h]
		float v11; // [esp+48h] [ebp-84h]
		float v12; // [esp+4Ch] [ebp-80h]
		float v13; // [esp+50h] [ebp-7Ch]
		float v14; // [esp+54h] [ebp-78h]
		float v15; // [esp+58h] [ebp-74h]
		float v16; // [esp+70h] [ebp-5Ch]
		float v17; // [esp+74h] [ebp-58h]
		float v18; // [esp+78h] [ebp-54h]
		float v19; // [esp+7Ch] [ebp-50h]
		float v20; // [esp+80h] [ebp-4Ch]
		float v21; // [esp+84h] [ebp-48h]
		float v22; // [esp+88h] [ebp-44h]
		float v23; // [esp+8Ch] [ebp-40h]
		float pos; // [esp+90h] [ebp-3Ch]
		float pos_4; // [esp+94h] [ebp-38h]
		float diff[3]; // [esp+98h] [ebp-34h] BYREF
		float tl[2]; // [esp+A4h] [ebp-28h]
		float angle; // [esp+ACh] [ebp-20h]
		float len; // [esp+B0h] [ebp-1Ch]
		float br[2]; // [esp+B4h] [ebp-18h]
		float unpackedColor[4]; // [esp+BCh] [ebp-10h] BYREF

		v22 = *start;
		v23 = *end;
		v15 = v23 - v22;
		if (v15 < 0.0)
			v14 = v23;
		else
			v14 = v22;
		tl[0] = v14;
		v20 = start[1];
		v21 = end[1];
		v13 = v21 - v20;
		if (v13 < 0.0)
			v12 = v21;
		else
			v12 = v20;
		tl[1] = v12;
		v18 = *start;
		v19 = *end;
		v11 = v18 - v19;
		if (v11 < 0.0)
			v10 = v19;
		else
			v10 = v18;
		br[0] = v10;
		v16 = start[1];
		v17 = end[1];
		v9 = v16 - v17;
		if (v9 < 0.0)
			v8 = v17;
		else
			v8 = v16;
		br[1] = v8;
		diff[0] = *end - *start;
		diff[1] = end[1] - start[1];
		diff[2] = 0.0;
		pos = (br[0] - tl[0]) / 2.0 + tl[0];
		pos_4 = (v8 - tl[1]) / 2.0 + tl[1];
		len = Game::Vec2Length(diff);
		Game::Vec3Normalize(diff);
		v7 = acos(diff[0]);
		angle = v7 * 57.2957763671875;
		if (start[1] > (double)end[1])
			angle = -angle;
		while (angle < 0.0)
			angle = angle + 360.0;
		Game::Byte4UnpackRgba(color, unpackedColor);
		h = (float)width;
		y = pos_4 - (double)(width / 2);
		x = pos - len / 2.0;
		//Game::CL_DrawStretchPicPhysicalRotateXY(x, y, len, h, 0.0f, 0.0f, 1.0f, 1.0f, angle, unpackedColor, Game::sharedUiInfo->assets.whiteMaterial);
	}

	void Devgui::DevGui_DrawGraph(const Game::DevMenuItem* menu, int32_t localClientNum)
	{
		float* v3; // ecx
		float* v4; // [esp+24h] [ebp-15Ch]
		float* v5; // [esp+28h] [ebp-158h]
		int32_t rowHeight; // [esp+2Ch] [ebp-154h]
		int32_t graphBottomY; // [esp+30h] [ebp-150h]
		float nextKnot; // [esp+34h] [ebp-14Ch]
		float nextKnot_4; // [esp+38h] [ebp-148h]
		int32_t width; // [esp+3Ch] [ebp-144h]
		Game::DevGraph* graph; // [esp+44h] [ebp-13Ch]
		int32_t knotX; // [esp+48h] [ebp-138h]
		float endKnotPos[2]; // [esp+4Ch] [ebp-134h] BYREF
		int32_t knotY; // [esp+54h] [ebp-12Ch]
		int32_t x; // [esp+58h] [ebp-128h]
		int32_t x_4; // [esp+5Ch] [ebp-124h]
		int32_t rowWidth; // [esp+60h] [ebp-120h]
		float startKnotPos[2]; // [esp+64h] [ebp-11Ch] BYREF
		float knot[2]; // [esp+6Ch] [ebp-114h]
		int32_t knotIndex; // [esp+74h] [ebp-10Ch]
		char text[260]; // [esp+78h] [ebp-108h] BYREF

		graph = menu->child.graph;
		rowWidth = devguiGlob.sliderWidth;
		rowHeight = DevGui_GetFontHeight();
		width = rowWidth + 8;
		x = devguiGlob.left + (devguiGlob.right - devguiGlob.left - (rowWidth + 8)) / 2;
		x_4 = devguiGlob.bottom - (3 * rowHeight + 16);
		DevGui_DrawBevelBox(
			x,
			x_4,
			rowWidth + 8,
			3 * rowHeight + 16,
			Dvars::devgui_bevelShade->current.value,
			(const uint8_t*)&Dvars::devgui_colorBgnd->current);
		graphBottomY = (int)((double)x_4 * 0.949999988079071);
		DevGui_DrawBox(x, graphBottomY, rowWidth + 8, 2, (const uint8_t*)&Dvars::devgui_colorBgndSel->current);
		x += 4;
		x_4 += 6;
		DevGui_DrawSliderPath(x, x_4);
		for (knotIndex = 0; knotIndex < *graph->knotCount; ++knotIndex)
		{
			v5 = graph->knots[knotIndex];
			knot[0] = *v5;
			knot[1] = v5[1];
			knotX = x + (int)((double)width * knot[0]);
			knotY = graphBottomY - (int)((double)(graphBottomY - 48) * knot[1]);
			if (knotIndex == graph->selectedKnot)
			{
				if (devguiGlob.editingKnot)
					DevGui_DrawBoxCentered(knotX, knotY, 19, 19, (const uint8_t*)&Dvars::devgui_colorGraphKnotEditing->current);
				else
					DevGui_DrawBoxCentered(knotX, knotY, 16, 16, (const uint8_t*)&Dvars::devgui_colorGraphKnotSelected->current);
			}
			else
			{
				DevGui_DrawBoxCentered(knotX, knotY, 12, 12, (const uint8_t*)&Dvars::devgui_colorGraphKnotNormal->current);
			}
		}
		for (knotIndex = 0; knotIndex < *graph->knotCount - 1; ++knotIndex)
		{
			v4 = graph->knots[knotIndex];
			knot[0] = *v4;
			knot[1] = v4[1];
			nextKnot = graph->knots[knotIndex + 1][0];
			nextKnot_4 = graph->knots[knotIndex + 1][1];
			startKnotPos[0] = (double)width * knot[0] + (double)x;
			startKnotPos[1] = (double)graphBottomY - (double)(graphBottomY - 48) * knot[1];
			endKnotPos[0] = (double)width * nextKnot + (double)x;
			endKnotPos[1] = (double)graphBottomY - (double)(graphBottomY - 48) * nextKnot_4;
			DevGui_DrawLine(startKnotPos, endKnotPos, 2, (const uint8_t*)&Dvars::devgui_colorGraphKnotNormal->current);
		}
		x_4 += rowHeight + 2;
		v3 = graph->knots[graph->selectedKnot];
		knot[0] = *v3;
		knot[1] = v3[1];
		if (graph->textCallback)
			((void(__cdecl*)(Game::DevGraph*, DWORD, DWORD, char*, int))graph->textCallback)(
				graph,
				knot[0],
				knot[1],
				text,
				256);
		else
			sprintf(text, "X: %.4f, Y: %.4f", knot[0], knot[1]);
		DevGui_DrawFont(x, x_4, (const uint8_t*)&Dvars::devgui_colorText->current, text);
		x_4 += rowHeight + 2;
		DevGui_DrawFont(x, x_4, (const uint8_t*)&Dvars::devgui_colorText->current, (char*)MYINSTRUCTIONS);
		if (graph->eventCallback)
			graph->eventCallback(graph, Game::EVENT_DRAW, localClientNum);
	}

	void Devgui::DevGui_DrawBindNextKey()
	{
		const uint32_t fadeColor = 0xAA000000;
		const char* textHint = "Press key to bind (ESC to cancel)...";

		DevGui_DrawBox(devguiGlob.left, devguiGlob.top, devguiGlob.right - devguiGlob.left, devguiGlob.bottom - devguiGlob.top, reinterpret_cast<const uint8_t*>(&fadeColor));

		int32_t textWidth = DevGui_GetFontWidth(textHint);
		int32_t textHeight = DevGui_GetFontHeight();
		int32_t x = (devguiGlob.right + devguiGlob.left - textWidth) / 2;
		int32_t y = (devguiGlob.bottom + devguiGlob.top - textHeight) / 2;

		const uint32_t textColor = static_cast<uint32_t>(-1);
		DevGui_DrawFont(x, y, reinterpret_cast<const uint8_t*>(&textColor), textHint);
	}

	void Devgui::DevGui_Draw(int32_t localClientNum)
	{
		int32_t origin[2]; // [esp+0h] [ebp-10h] BYREF
		Game::DevMenuItem* menuItem; // [esp+8h] [ebp-8h]
		uint16_t parent; // [esp+Ch] [ebp-4h]

		if (devguiGlob.isActive)
		{
			menuItem = (Game::DevMenuItem*)DevGui_GetMenu(devguiGlob.selectedMenu);
			if (devguiGlob.editingMenuItem)
			{
				if (menuItem->childType == 1)
					DevGui_DrawSliders(menuItem);
				else
					DevGui_DrawGraph(menuItem, localClientNum);
			}
			else
			{
				origin[0] = devguiGlob.left;
				origin[1] = devguiGlob.top;
				parent = DevGui_GetMenuParent(devguiGlob.selectedMenu);
				DevGui_ChooseOrigin(origin);
				DevGui_DrawMenu(parent, devguiGlob.selectedMenu, origin);
			}
			if (devguiGlob.bindNextKey)
				DevGui_DrawBindNextKey();
		}
	}

	void Devgui::DevGui_Reset()
	{
		devguiGlob.isActive = 0;
		devguiGlob.bindNextKey = 0;
		devguiGlob.editingMenuItem = 0;
		devguiGlob.selectedMenu = 0;
	}

	void Devgui::DevGui_Toggle()
	{
		if (devguiGlob.topmostMenu.child.menu)
		{
			if (devguiGlob.selectedMenu)
				goto LABEL_10;
			devguiGlob.selectedMenu = devguiGlob.topmostMenu.child.menu;
			if (devguiGlob.topmostMenu.child.menu)
			{
				DevGui_SelectTopLevelChild();
				if (devguiGlob.selectedMenu)
					LABEL_10:
				devguiGlob.isActive = !devguiGlob.isActive;
			}
		}
	}

	void Devgui::DevGui_InputInit()
	{
		Command::Add("devgui", DevGui_Toggle);
	}

	void Devgui::DevGui_Init()
	{
		uint32_t menuIndex = 0;
		int32_t screen_yPad = 0;
		int32_t screen_xPad = 0;

		DevGui_RegisterDvars();

		for (menuIndex = 0; menuIndex < 599; ++menuIndex)
			*(uint32_t*)devguiGlob.menus[menuIndex].label = (uint32_t)&devguiGlob.menus[menuIndex + 1];
		*(uint32_t*)devguiGlob.menus[menuIndex].label = 0;
		devguiGlob.nextFreeMenu = (Game::DevMenuItem*)&devguiGlob;
		devguiGlob.topmostMenu.childType = 0;
		devguiGlob.topmostMenu.childMenuMemory = 0;
		devguiGlob.topmostMenu.child.menu = 0;
		devguiGlob.topmostMenu.nextSibling = 0;
		devguiGlob.topmostMenu.prevSibling = 0;
		devguiGlob.topmostMenu.parent = 0;
		devguiGlob.isActive = 0;
		devguiGlob.editingMenuItem = 0;
		devguiGlob.selRow = -1;
		devguiGlob.editingKnot = 0;
		devguiGlob.left = screen_xPad;
		devguiGlob.right = DevGui_GetScreenWidth() - screen_xPad;
		devguiGlob.top = screen_yPad;
		devguiGlob.bottom = DevGui_GetScreenHeight() - screen_yPad;
		devguiGlob.sliderWidth = 3 * (devguiGlob.right - devguiGlob.left) / 4;
		DevGui_InputInit();
	}

	void Devgui::R_CreateDevGui()
	{
		Game::Cbuf_InsertText(0, "exec devgui_renderer");
		Game::Cbuf_InsertText(0, "exec devgui_visibility");
	}

	const char* I_stristr(const char* s0, const char* substr)
	{
		int v3; // esi
		int s0Char; // [esp+4h] [ebp-8h]
		int substrChar; // [esp+8h] [ebp-4h]

		for (s0Char = 0; s0[s0Char]; ++s0Char)
		{
			substrChar = -1;
			do
			{
				if (!substr[++substrChar])
					return &s0[s0Char];
				v3 = tolower(s0[substrChar + s0Char]);
			} while (v3 == tolower(substr[substrChar]));
		}

		return 0;
	}

	void Devgui::CG_AddVisionSetMenuItem(Game::XAssetHeader header)
	{
		std::string name = header.rawfile->name;

		if (name.ends_with(".vision") && name.starts_with("vision/"))
		{
			const std::string path = name.substr(0, name.size() - 7);
			const std::string visionName = name.substr(7, path.size() - 7);
			DevGui_AddCommand(Utils::String::VA("Renderer/Vision Sets/%s", visionName.c_str()), Utils::String::VA("VisionSetNaked %s", visionName.c_str()));
		}
	}

	void Devgui::CG_InitVisionSetsMenu()
	{
		FastFiles::EnumAssets(Game::ASSET_TYPE_RAWFILE, [](Game::XAssetHeader header)
		{
			CG_AddVisionSetMenuItem(header);
		}, false);
	}

	void Devgui::CL_CreateDevGui()
	{
		clGuiGlob.inited = 1;
		R_CreateDevGui();
		Game::Cbuf_InsertText(0, "exec devgui_main");
		Game::Cbuf_InsertText(0, "exec iw3sp_mod_devgui_maps_sp"); // just a devmap cmds with stock sp maps
	}

	Game::DevGuiTokenResult Devgui::DevGui_PathToken(const char** pathInOut, char* label, __int16* sortKeyOut)
	{
		const char* path; // [esp+0h] [ebp-10h]
		__int16 sign; // [esp+4h] [ebp-Ch]
		int32_t labelLen; // [esp+8h] [ebp-8h]
		__int16 sortKey; // [esp+Ch] [ebp-4h]

		path = *pathInOut;
		labelLen = 0;
		*sortKeyOut = 0x7FFF;
		while (*path)
		{
			if (*path == 58)
			{
				label[labelLen] = 0;
				++path;
				sortKey = 0;
				sign = 1;
				if (*path == 45)
				{
					sign = -1;
					++path;
				}
				else if (*path == 43)
				{
					++path;
				}
				if (*path < 48 || *path > 57)
					return (Game::DevGuiTokenResult)0;
				do
					sortKey = 10 * sortKey + *path++ - 48;
				while (*path >= 48 && *path <= 57);
				*sortKeyOut = sign * sortKey;
				if (*path && (*path != 47 || path[1] == 47))
					return (Game::DevGuiTokenResult)0;
			}
			else
			{
				if (*path == 47 && *++path != 47)
				{
					label[labelLen] = 0;
					*pathInOut = path;
					return (Game::DevGuiTokenResult)1;
				}
				label[labelLen] = *path++;
				if (++labelLen == 26)
					return (Game::DevGuiTokenResult)0;
			}
		}
		if (!labelLen)
			return (Game::DevGuiTokenResult)0;
		*pathInOut = path;
		label[labelLen] = 0;
		return (Game::DevGuiTokenResult)2;
	}

	bool Devgui::DevGui_IsValidPath(const char* path)
	{
		char label[28]; // [esp+20h] [ebp-2Ch] BYREF
		const char* originalPath; // [esp+40h] [ebp-Ch]
		Game::DevGuiTokenResult tokResult; // [esp+44h] [ebp-8h]
		__int16 sortKey; // [esp+48h] [ebp-4h] BYREF

		if (strlen(path) <= 0x78)
		{
			originalPath = path;
			tokResult = DevGui_PathToken(&path, label, &sortKey);
			if (tokResult == Game::DEVGUI_TOKEN_LAST)
			{
				Game::Com_Printf(11, "Path '%s' must have at least one menu separator ('/' character).\n", originalPath);
				return 0;
			}
			else
			{
				do
				{
					if (tokResult == Game::DEVGUI_TOKEN_ERROR)
					{
						Game::Com_Printf(
							11,
							"path '%s' is invalid.  Format is 'menu name:sortkey/submenu/...', where 'sortkey' is any (possibly signed) integer.\n",
							originalPath);
						return 0;
					}
					tokResult = DevGui_PathToken(&path, label, &sortKey);
				} while (tokResult != Game::DEVGUI_TOKEN_LAST);
				return 1;
			}
		}
		else
		{
			Game::Com_Printf(11, "Path '%s' must be no longer than %i characters (currently %i).\n", path, 120, strlen(path));
			return 0;
		}
	}

	uint16_t Devgui::DevGui_FindMenu(uint16_t parentHandle, const char* label)
	{
		Game::devguiGlob_t* childMenu; // [esp+0h] [ebp-Ch]
		uint16_t childHandle; // [esp+4h] [ebp-8h]
		Game::devguiGlob_t* parentMenu; // [esp+8h] [ebp-4h]

		if (parentHandle)
			parentMenu = DevGui_GetMenu(parentHandle);
		else
			parentMenu = (Game::devguiGlob_t*)&devguiGlob.topmostMenu;
		for (childHandle = parentMenu->menus[0].child.menu; childHandle; childHandle = childMenu->menus[0].nextSibling)
		{
			childMenu = DevGui_GetMenu(childHandle);
			if (!Game::I_stricmp(0x7FFFFFFF, label, (const char*)childMenu))
				return childHandle;
		}
		return 0;
	}

	void Devgui::DevGui_OpenMenu(const char* path)
	{
		uint16_t handle; // [esp+0h] [ebp-30h]
		char label[28]; // [esp+4h] [ebp-2Ch] BYREF
		Game::DevMenuItem* menu; // [esp+24h] [ebp-Ch]
		Game::DevGuiTokenResult tokResult; // [esp+28h] [ebp-8h]
		__int16 sortKey; // [esp+2Ch] [ebp-4h] BYREF

		if (DevGui_IsValidPath(path))
		{
			handle = 0;
			while (1)
			{
				tokResult = DevGui_PathToken(&path, label, &sortKey);
				handle = DevGui_FindMenu(handle, label);
				if (!handle)
					break;
				if (tokResult == Game::DEVGUI_TOKEN_LAST)
				{
					menu = (Game::DevMenuItem*)DevGui_GetMenu(handle);
					if (menu->childType == 2)
					{
						Game::Cbuf_InsertText(0, menu->child.command);
					}
					else
					{
						devguiGlob.isActive = 1;
						devguiGlob.selectedMenu = handle;
						devguiGlob.editingMenuItem = DevGui_EditableMenuItem(menu);
						if (!devguiGlob.editingMenuItem && !menu->childType)
							devguiGlob.selectedMenu = menu->child.menu;
						devguiGlob.selRow = 0;
					}
					return;
				}
			}
		}
	}

	uint16_t Devgui::DevGui_GetMenuHandle(Game::DevMenuItem* menu)
	{
		uint16_t handle; // [esp+0h] [ebp-4h]

		handle = ((char*)menu - (char*)&devguiGlob) / 40 + 1;
		return ((char*)menu - (char*)&devguiGlob) / 40 + 1;
	}

	int32_t Devgui::DevGui_CompareMenus(const Game::DevMenuItem* menu0, const Game::DevMenuItem* menu1)
	{
		if (menu0->sortKey == menu1->sortKey)
			return Game::I_stricmp(0x7FFFFFF, menu0->label, menu1->label);
		else
			return menu0->sortKey - menu1->sortKey;
	}

	uint16_t Devgui::DevGui_CreateMenu(uint16_t parentHandle, const char* label, __int16 sortKey)
	{
		char v4; // [esp+3h] [ebp-25h]
		Game::DevMenuItem* v5; // [esp+8h] [ebp-20h]
		uint16_t handle; // [esp+10h] [ebp-18h]
		uint16_t* prevNext; // [esp+14h] [ebp-14h]
		Game::DevMenuItem* menu; // [esp+18h] [ebp-10h]
		uint16_t prev; // [esp+1Ch] [ebp-Ch]
		Game::devguiGlob_t* nextMenu; // [esp+20h] [ebp-8h]
		Game::devguiGlob_t* parentMenu; // [esp+24h] [ebp-4h]

		menu = devguiGlob.nextFreeMenu;
		if (!devguiGlob.nextFreeMenu)
			Game::Com_Error(1, "Too many devgui entries (more than %i)", 600);
		devguiGlob.nextFreeMenu = *(Game::DevMenuItem**)menu->label;
		handle = DevGui_GetMenuHandle(menu);
		v5 = menu;
		do
		{
			v4 = *label;
			v5->label[0] = *label++;
			v5 = (Game::DevMenuItem*)((char*)v5 + 1);
		} while (v4);
		menu->childType = 0;
		menu->childMenuMemory = 0;
		menu->sortKey = sortKey;
		menu->parent = parentHandle;
		menu->child.menu = 0;
		if (parentHandle)
			parentMenu = DevGui_GetMenu(parentHandle);
		else
			parentMenu = (Game::devguiGlob_t*)&devguiGlob.topmostMenu;
		prev = 0;
		for (prevNext = (uint16_t*)&parentMenu->menus[0].child; *prevNext; prevNext = &nextMenu->menus[0].nextSibling)
		{
			nextMenu = DevGui_GetMenu(*prevNext);
			if (DevGui_CompareMenus(nextMenu->menus, menu) > 0)
				break;
			prev = *prevNext;
		}
		menu->nextSibling = *prevNext;
		menu->prevSibling = prev;
		*prevNext = handle;
		if (menu->nextSibling)
			DevGui_GetMenu(menu->nextSibling)->menus[0].prevSibling = handle;
		return handle;
	}

	uint16_t Devgui::DevGui_RegisterMenu(uint16_t parentHandle, const char* label, __int16 sortKey)
	{
		uint16_t childHandle; // [esp+0h] [ebp-4h]

		childHandle = DevGui_FindMenu(parentHandle, label);
		if (!childHandle)
			return DevGui_CreateMenu(parentHandle, label, sortKey);
		return childHandle;
	}

	uint16_t Devgui::DevGui_ConstructPath_r(uint16_t parent, const char* path)
	{
		char label[28]; // [esp+0h] [ebp-28h] BYREF
		Game::DevGuiTokenResult tokResult; // [esp+20h] [ebp-8h]
		__int16 sortKey; // [esp+24h] [ebp-4h] BYREF

		do
		{
			tokResult = (Game::DevGuiTokenResult)DevGui_PathToken(&path, label, &sortKey);
			parent = DevGui_RegisterMenu(parent, label, sortKey);
		} while (tokResult != Game::DEVGUI_TOKEN_LAST);
		return parent;
	}


	void Devgui::DevGui_AddDvar(const char* path, const Game::dvar_s* dvar)
	{
		uint16_t handle; // [esp+0h] [ebp-8h]
		Game::devguiGlob_t* menu; // [esp+4h] [ebp-4h]

		if (DevGui_IsValidPath(path))
		{
			handle = DevGui_ConstructPath_r(0, path);
			menu = DevGui_GetMenu(handle);
			if (!menu->menus[0].childType && !menu->menus[0].child.menu
				|| menu->menus[0].childType == 1 && menu->menus[0].child.dvar == dvar)
			{
				menu->menus[0].childType = 1;
				menu->menus[0].child.command = (const char*)dvar;
			}
			else
			{
				Game::Com_Printf(
					11,
					"Path '%s' can't be used for dvar '%s' because it is already used for something else.\n",
					path,
					(const char*)dvar);
			}
		}
	}

	void Devgui::DevGui_AddCommand(const char* path, const char* command)
	{
		uint16_t handle; // [esp+0h] [ebp-8h]
		Game::devguiGlob_t* menu; // [esp+4h] [ebp-4h]

		if (DevGui_IsValidPath(path))
		{
			handle = DevGui_ConstructPath_r(0, path);
			menu = DevGui_GetMenu(handle);
			if (!menu->menus[0].childType && !menu->menus[0].child.menu
				|| menu->menus[0].childType == 2 && menu->menus[0].child.command == command)
			{
				menu->menus[0].childType = 2;
				menu->menus[0].child.command = Game::CopyString(command);
			}
			else
			{
				Game::Com_Printf(
					11,
					"Path '%s' can't be used for command '%s' because it is already used for something else.\n",
					path,
					command);
			}
		}
	}

	void Devgui::CL_InitDevGui()
	{
		DevGui_Init();

		Command::Add("devgui_dvar", [](Command::Params* params)
		{
			if (params->size() == 3)
			{
				const char* dvarName = params->get(2);
				Game::dvar_s* dvar = Dvars::Functions::Dvar_FindVar(dvarName);

				if (dvar)
				{
					DevGui_AddDvar(params->get(1), dvar);
				}
				else
				{
					Game::Com_Printf(11, "dvar '%s' doesn't exist\n", dvarName);
				}
			}
			else
			{
				Game::Com_Printf(0, "USAGE: %s \"devgui path\" dvarName\n", params->get(1));
			}
		});

		Command::Add("devgui_cmd", [](Command::Params* params)
		{
			if (params->size() == 3)
			{
				DevGui_AddCommand(params->get(1), params->get(2));
			}
			else
			{
				Game::Com_Printf(0, "USAGE: %s \"devgui path\" \"command text\"\n", params->get(1));
			}
		});

		Command::Add("devgui_open", [](Command::Params* params)
		{
			if (params->size() == 2)
			{
				DevGui_OpenMenu(params->get(1));
			}
			else
			{
				Game::Com_Printf(0, "USAGE: %s \"devgui path\"\n", params->get(1));
			}
		});

		CL_CreateDevGui();
	}

	void Devgui::DevGui_MouseEvent(int32_t dx, int32_t dy)
	{
		s_input.mousePos[0] = static_cast<float>(dx);
		s_input.mousePos[1] = static_cast<float>(dy);
	}

	void Devgui::DevGui_InputUpdateMouse()
	{
		s_input.sliderScrollTime = 100.0f;
		s_input.sliderScrollMaxTimeStep = 0.30000001f;
	}

	void Devgui::DevGui_UpdateScrollInputs(int32_t localClientNum)
	{
		float verticalInput = 0.0f;
		float horizontalInput = 0.0f;
		float upInput = 0.0f;
		float downInput = 0.0f;
		float rightInput = 0.0f;
		float leftInput = 0.0f;
		float lowScale = 1.0f;
		float highScale = 1.0f;

		if (DevGui_IsButtonDown(Game::INPUT_RIGHT))
			rightInput = 1.0f;

		if (DevGui_IsButtonDown(Game::INPUT_LEFT))
			leftInput = 1.0f;

		if (DevGui_IsButtonDown(Game::INPUT_UP))
			upInput = 1.0f;

		if (DevGui_IsButtonDown(Game::INPUT_DOWN))
			downInput = 1.0f;

		if (Game::Key_IsDown(localClientNum, Game::K_MOUSE1))
		{
			horizontalInput = s_input.mousePos[0];
			verticalInput = -s_input.mousePos[1];
		}

		s_input.mousePos[0] = 0.0f;
		s_input.mousePos[1] = 0.0f;

		if ((rightInput != 0.0f || leftInput != 0.0f) && (upInput != 0.0f || downInput != 0.0f))
		{
			rightInput = 0.0f;
			leftInput = 0.0f;
			upInput = 0.0f;
			downInput = 0.0f;
		}

		lowScale = 1.0f;
		highScale = 1.0f;
		s_input.scrollScale = lowScale * highScale;
		s_input.digitalAxis[0] = rightInput - leftInput;
		s_input.digitalAxis[1] = upInput - downInput;
		s_input.analogAxis[0] = horizontalInput;
		s_input.analogAxis[1] = verticalInput;
	}

	void Devgui::DevGui_UpdateScrollStates(float deltaTime, Game::DevGuiInputState* states, float* axis, float* times)
	{
		float v4; // [esp+0h] [ebp-10h]
		Game::DevGuiInputState v5; // [esp+4h] [ebp-Ch]
		int32_t axisIndex; // [esp+Ch] [ebp-4h]

		for (axisIndex = 0; axisIndex < 2; ++axisIndex)
		{
			if (axis[axisIndex] == 0.0)
			{
				states[axisIndex] = Game::SCROLL_NONE;
				times[axisIndex] = 0.0;
			}
			else
			{
				v5 = states[axisIndex];
				if (v5)
				{
					if (v5 == Game::SCROLL_PRESSED)
					{
						states[axisIndex] = Game::SCROLL_STALLED;
						times[axisIndex] = 0.0;
					}
					else if (v5 == Game::SCROLL_STALLED)
					{
						times[axisIndex] = times[axisIndex] + deltaTime;
						if (times[axisIndex] > 0.25)
						{
							states[axisIndex] = Game::SCROLL_HELD;
							times[axisIndex] = 0.0;
						}
					}
					else
					{
						times[axisIndex] = times[axisIndex] + deltaTime;
					}
				}
				else
				{
					v4 = fabs(axis[axisIndex]);
					if (v4 > 0.4000000059604645)
						states[axisIndex] = Game::SCROLL_PRESSED;
				}
			}
		}
	}

	void Devgui::DevGui_UpdateMenuScroll(float deltaTime)
	{
		float v1; // [esp+4h] [ebp-40h]
		float v2; // [esp+8h] [ebp-3Ch]
		float v3; // [esp+Ch] [ebp-38h]
		bool v4; // [esp+10h] [ebp-34h]
		bool v5; // [esp+14h] [ebp-30h]
		float v6; // [esp+18h] [ebp-2Ch]
		float v7; // [esp+1Ch] [ebp-28h]
		float adjustedAnalogAxis[2]; // [esp+30h] [ebp-14h]
		int32_t axisIndex; // [esp+38h] [ebp-Ch]
		bool pressed; // [esp+3Eh] [ebp-6h]
		bool held; // [esp+3Fh] [ebp-5h]
		float axis; // [esp+40h] [ebp-4h]

		v7 = fabs(s_input.analogAxis[0]);
		v6 = fabs(s_input.analogAxis[1]);
		if (v6 >= (double)v7)
		{
			adjustedAnalogAxis[0] = 0.0;
			adjustedAnalogAxis[1] = s_input.analogAxis[1];
		}
		else
		{
			adjustedAnalogAxis[0] = s_input.analogAxis[0];
			adjustedAnalogAxis[1] = 0.0;
		}
		for (axisIndex = 0; axisIndex < 2; ++axisIndex)
		{
			s_input.menuScroll[axisIndex] = 0;
			v5 = s_input.digitalStates[axisIndex] == Game::SCROLL_HELD || s_input.analogStates[axisIndex] == Game::SCROLL_HELD;
			held = v5;
			v4 = s_input.digitalStates[axisIndex] == Game::SCROLL_PRESSED || s_input.analogStates[axisIndex] == Game::SCROLL_PRESSED;
			pressed = v4;
			axis = (s_input.digitalAxis[axisIndex] + adjustedAnalogAxis[axisIndex]) * s_input.scrollScale;
			if (held)
			{
				v3 = fabs(axis);
				for (s_input.menuScrollTime[axisIndex] = deltaTime * v3 + s_input.menuScrollTime[axisIndex];
					s_input.menuScrollTime[axisIndex] > 0.1000000014901161;
					s_input.menuScrollTime[axisIndex] = s_input.menuScrollTime[axisIndex] - 0.1000000014901161)
				{
					if (axis < 0.0)
						v2 = -1.0;
					else
						v2 = 1.0;
					s_input.menuScroll[axisIndex] += (int)v2;
				}
			}
			else if (pressed)
			{
				if (axis < 0.0)
					v1 = -1.0;
				else
					v1 = 1.0;
				s_input.menuScroll[axisIndex] += (int)v1;
				s_input.menuScrollTime[axisIndex] = 0.0;
			}
		}
	}

	bool Devgui::DevGui_InputUpdate(int32_t localClientNum, float deltaTime)
	{
		int32_t butIndex; // [esp+10h] [ebp-8h]

		DevGui_InputUpdateMouse();
		for (butIndex = 0; butIndex < 11; ++butIndex)
		{
			s_input.prevButtonDown[butIndex] = s_input.buttonDown[butIndex];
			s_input.buttonDown[butIndex] = Game::Key_IsDown(localClientNum, s_butMapsKey[butIndex]) != 0;
		}
		DevGui_UpdateScrollInputs(localClientNum);
		DevGui_UpdateScrollStates(deltaTime, s_input.digitalStates, s_input.digitalAxis, s_input.digitalTimes);
		DevGui_UpdateMenuScroll(deltaTime);
		Game::CL_ClearKeys(localClientNum);
		return 1;
	}

	__int16 Devgui::DevGui_GetMenuScroll(Game::DevGuiInputAxis axis)
	{
		return s_input.menuScroll[axis];
	}

	bool Devgui::DevGui_IsButtonReleased(Game::DevGuiInputButton button)
	{
		return !s_input.buttonDown[button] && s_input.prevButtonDown[button];
	}

	void Devgui::DevGui_SelectNextMenuItem()
	{
		Game::devguiGlob_t* menuParent; // [esp+0h] [ebp-8h]
		Game::devguiGlob_t* menu; // [esp+4h] [ebp-4h]

		menu = DevGui_GetMenu(devguiGlob.selectedMenu);
		if (menu->menus[0].parent)
		{
			menuParent = DevGui_GetMenu(menu->menus[0].parent);
			if (menu->menus[0].nextSibling)
				++menuParent->menus[0].childMenuMemory;
			else
				menuParent->menus[0].childMenuMemory = 0;
		}
		if (menu->menus[0].nextSibling)
		{
			devguiGlob.selectedMenu = menu->menus[0].nextSibling;
		}
		else
		{
			while (menu->menus[0].prevSibling)
			{
				devguiGlob.selectedMenu = menu->menus[0].prevSibling;
				menu = DevGui_GetMenu(devguiGlob.selectedMenu);
			}
		}
		DevGui_SelectTopLevelChild();
	}

	void Devgui::DevGui_MoveDown()
	{
		Game::devguiGlob_t* menu; // [esp+0h] [ebp-4h]
		menu = DevGui_GetMenu(devguiGlob.selectedMenu);
		if (!menu->menus[0].parent)
		{
			devguiGlob.selectedMenu = menu->menus[0].child.menu;
			DevGui_GetMenu(devguiGlob.selectedMenu);
		}
		DevGui_SelectNextMenuItem();
	}

	const Game::dvar_s* Devgui::DevGui_SelectedDvar()
	{
		Game::devguiGlob_t* menu; // [esp+0h] [ebp-8h]
		const Game::dvar_s* dvar; // [esp+4h] [ebp-4h]

		menu = DevGui_GetMenu(devguiGlob.selectedMenu);
		dvar = menu->menus[0].child.dvar;
		return dvar;
	}

	int32_t Devgui::DevGui_ScrollDownInternal()
	{
		int32_t result; // eax
		Game::devguiGlob_t* menu; // [esp+0h] [ebp-4h]

		do
		{
			DevGui_MoveDown();
			menu = DevGui_GetMenu(devguiGlob.selectedMenu);
			result = DevGui_EditableMenuItem(menu->menus);
		} while (!result);
		return result;
	}

	int32_t Devgui::DevGui_ScrollDown()
	{
		int32_t result; // eax
		int32_t rowCount; // [esp+0h] [ebp-Ch]
		Game::devguiGlob_t* menu; // [esp+4h] [ebp-8h]
		const Game::dvar_s* dvar; // [esp+8h] [ebp-4h]

		menu = DevGui_GetMenu(devguiGlob.selectedMenu);
		if (menu->menus[0].childType == 3)
			return DevGui_ScrollDownInternal();
		dvar = DevGui_SelectedDvar();
		rowCount = DevGui_DvarRowCount(dvar);
		if (rowCount == 1)
			return DevGui_ScrollDownInternal();
		result = ++devguiGlob.selRow;
		if (devguiGlob.selRow == rowCount)
			devguiGlob.selRow = 0;
		return result;
	}

	void Devgui::DevGui_SelectPrevMenuItem()
	{
		Game::devguiGlob_t* v0; // [esp+0h] [ebp-Ch]
		Game::devguiGlob_t* menu; // [esp+4h] [ebp-8h]

		menu = DevGui_GetMenu(devguiGlob.selectedMenu);
		if (menu->menus[0].parent)
			v0 = DevGui_GetMenu(menu->menus[0].parent);
		else
			v0 = 0;
		if (menu->menus[0].prevSibling)
		{
			if (v0)
				--v0->menus[0].childMenuMemory;
			devguiGlob.selectedMenu = menu->menus[0].prevSibling;
		}
		else
		{
			if (v0)
				v0->menus[0].childMenuMemory = 0;
			while (menu->menus[0].nextSibling)
			{
				if (v0)
					++v0->menus[0].childMenuMemory;
				devguiGlob.selectedMenu = menu->menus[0].nextSibling;
				menu = DevGui_GetMenu(devguiGlob.selectedMenu);
			}
		}
		DevGui_SelectTopLevelChild();
	}

	void Devgui::DevGui_MoveUp()
	{
		Game::devguiGlob_t* menu; // [esp+0h] [ebp-4h]

		menu = DevGui_GetMenu(devguiGlob.selectedMenu);
		if (!menu->menus[0].parent)
		{
			devguiGlob.selectedMenu = menu->menus[0].child.menu;
			DevGui_GetMenu(devguiGlob.selectedMenu);
		}
		DevGui_SelectPrevMenuItem();
	}


	int32_t Devgui::DevGui_ScrollUpInternal()
	{
		int32_t result; // eax
		Game::devguiGlob_t* menu; // [esp+0h] [ebp-4h]

		do
		{
			DevGui_MoveUp();
			menu = DevGui_GetMenu(devguiGlob.selectedMenu);
			result = DevGui_EditableMenuItem(menu->menus);
		} while (!result);
		return result;
	}

	int32_t Devgui::DevGui_ScrollUp()
	{
		int32_t result; // eax
		int32_t rowCount; // [esp+0h] [ebp-Ch]
		Game::devguiGlob_t* menu; // [esp+4h] [ebp-8h]
		const Game::dvar_s* dvar; // [esp+8h] [ebp-4h]

		menu = DevGui_GetMenu(devguiGlob.selectedMenu);
		if (menu->menus[0].childType == 3)
			return DevGui_ScrollUpInternal();
		dvar = DevGui_SelectedDvar();
		result = DevGui_DvarRowCount(dvar);
		rowCount = result;
		if (result == 1)
			return DevGui_ScrollUpInternal();
		if (devguiGlob.selRow)
		{
			--devguiGlob.selRow;
		}
		else
		{
			--result;
			devguiGlob.selRow = rowCount - 1;
		}
		return result;
	}

	int32_t Devgui::DevGui_UpdateSelection()
	{
		int32_t result; // eax
		int32_t scroll; // [esp+0h] [ebp-4h]

		result = DevGui_GetMenuScroll(Game::SCROLL_YAXIS);
		for (scroll = (__int16)result; scroll < 0; ++scroll)
			result = DevGui_ScrollDown();
		while (scroll > 0)
		{
			result = DevGui_ScrollUp();
			--scroll;
		}
		return result;
	}

	void Devgui::DevGui_MoveLeft()
	{
		Game::devguiGlob_t* menu; // [esp+0h] [ebp-4h]

		for (menu = DevGui_GetMenu(devguiGlob.selectedMenu);
			menu->menus[0].parent;
			menu = DevGui_GetMenu(devguiGlob.selectedMenu))
		{
			devguiGlob.selectedMenu = menu->menus[0].parent;
		}
		DevGui_SelectPrevMenuItem();
	}

	void Devgui::DevGui_MoveRight()
	{
		Game::devguiGlob_t* menu; // [esp+0h] [ebp-4h]

		for (menu = DevGui_GetMenu(devguiGlob.selectedMenu);
			menu->menus[0].parent;
			menu = DevGui_GetMenu(devguiGlob.selectedMenu))
		{
			devguiGlob.selectedMenu = menu->menus[0].parent;
		}
		DevGui_SelectNextMenuItem();
	}

	void Devgui::DevGui_MoveSelectionHorizontally()
	{
		int32_t scroll; // [esp+0h] [ebp-4h]

		for (scroll = DevGui_GetMenuScroll(Game::SCROLL_XAXIS); scroll < 0; ++scroll)
			DevGui_MoveLeft();
		while (scroll > 0)
		{
			DevGui_MoveRight();
			--scroll;
		}
	}

	void Devgui::DevGui_MoveSelectionVertically()
	{
		int32_t scroll; // [esp+0h] [ebp-4h]

		for (scroll = DevGui_GetMenuScroll(Game::SCROLL_YAXIS); scroll < 0; ++scroll)
			DevGui_MoveDown();
		while (scroll > 0)
		{
			DevGui_MoveUp();
			--scroll;
		}
	}

	void Devgui::Dvar_SetModified(Game::dvar_s* dvar)
	{
		dvar->modified = true;
	}

	void Dvar_MakeLatchedValueCurrent(Game::dvar_s* dvar)
	{
		Game::Dvar_SetVariant(dvar, dvar->latched, Game::DVAR_SOURCE_INTERNAL);
	}

	void Devgui::DevGui_Accept(int32_t localClientNum)
	{
		Game::devguiGlob_t* menu; // [esp+4h] [ebp-4h]

		menu = DevGui_GetMenu(devguiGlob.selectedMenu);
		switch (menu->menus[0].childType)
		{
		case 0u:
			devguiGlob.selectedMenu = menu->menus[0].child.menu;
			DevGui_AdvanceChildNum(menu->menus[0].childMenuMemory);
			break;
		case 1u:
			if (DevGui_EditableMenuItem(menu->menus))
			{
				if (devguiGlob.editingMenuItem)
				{
					Dvar_MakeLatchedValueCurrent((Game::dvar_s*)menu->menus[0].child.command);
					Dvar_SetModified((Game::dvar_s*)menu->menus[0].child.dvar);
				}
				devguiGlob.editingMenuItem = !devguiGlob.editingMenuItem;
				devguiGlob.selRow = 0;
			}
			break;
		case 2u:
			Game::Cbuf_InsertText(0, (char*)menu->menus[0].child.command);
			break;
		case 3u:
			devguiGlob.editingMenuItem = !devguiGlob.editingMenuItem;
			devguiGlob.selRow = 0;
			if (menu->menus[0].child.command && *((uint32_t*)menu->menus[0].child.command + 4))
				(*((void(__cdecl**)(Game::DevMenuChild, uint32_t, int))menu->menus[0].child.command + 4))(
					menu->menus[0].child,
					0,
					localClientNum);
			break;
		default:
			break;
		}
	}

	void Devgui::DevGui_Reject()
	{
		Game::devguiGlob_t* menu; // [esp+0h] [ebp-4h]

		menu = DevGui_GetMenu(devguiGlob.selectedMenu);
		if (devguiGlob.editingMenuItem)
		{
			//Dvar_ClearLatchedValue((dvar_s*)menu->menus[0].child.command);
			devguiGlob.editingMenuItem = 0;
		}
		else if (menu->menus[0].parent)
		{
			if (DevGui_GetMenu(menu->menus[0].parent)->menus[0].parent)
				devguiGlob.selectedMenu = menu->menus[0].parent;
		}
	}

	bool Devgui::DevGui_IsButtonPressed(Game::DevGuiInputButton button)
	{
		return s_input.buttonDown[button] && !s_input.prevButtonDown[button];
	}

	void Devgui::DevGui_AddGraphKnot(Game::DevGraph* graph, int32_t localClientNum)
	{
		float* v2; // [esp+4h] [ebp-20h]
		float* v3; // [esp+Ch] [ebp-18h]
		float* v4; // [esp+10h] [ebp-14h]
		float averageX; // [esp+14h] [ebp-10h]
		float averageXa; // [esp+14h] [ebp-10h]
		int32_t currentKnotCount; // [esp+18h] [ebp-Ch]
		int32_t knotIndex; // [esp+1Ch] [ebp-8h]
		float averageY; // [esp+20h] [ebp-4h]
		float averageYa; // [esp+20h] [ebp-4h]

		if (*graph->knotCount == graph->knotCountMax)
		{
			Game::Com_Printf(11, "^3Maximum number of knots have reached for this graph\n");
		}
		else
		{
			currentKnotCount = *graph->knotCount;
			if (graph->selectedKnot + 1 >= currentKnotCount)
			{
				averageXa = (graph->knots[graph->selectedKnot][0] + graph->knots[graph->selectedKnot - 1][0]) * 0.5;
				averageYa = (graph->knots[graph->selectedKnot][1] + graph->knots[graph->selectedKnot - 1][1]) * 0.5;
				v2 = graph->knots[currentKnotCount];
				*v2 = *(v2 - 2);
				v2[1] = *(v2 - 1);
				graph->knots[currentKnotCount - 1][0] = averageXa;
				graph->knots[currentKnotCount - 1][1] = averageYa;
				++*graph->knotCount;
			}
			else
			{
				averageX = (graph->knots[graph->selectedKnot][0] + graph->knots[graph->selectedKnot + 1][0]) * 0.5;
				averageY = (graph->knots[graph->selectedKnot][1] + graph->knots[graph->selectedKnot + 1][1]) * 0.5;
				for (knotIndex = *graph->knotCount - 1; knotIndex >= graph->selectedKnot; --knotIndex)
				{
					v3 = graph->knots[knotIndex + 1];
					v4 = graph->knots[knotIndex];
					*v3 = *v4;
					v3[1] = v4[1];
				}
				graph->knots[graph->selectedKnot + 1][0] = averageX;
				graph->knots[++graph->selectedKnot][1] = averageY;
				++*graph->knotCount;
			}
			if (graph->eventCallback)
				graph->eventCallback(graph, Game::EVENT_UPDATE, localClientNum);
		}
	}

	void Devgui::DevGui_RemoveGraphKnot(Game::DevGraph* graph, int32_t localClientNum)
	{
		float* v2; // [esp+0h] [ebp-10h]
		int32_t currentKnotCount; // [esp+8h] [ebp-8h]
		int32_t knotIndex; // [esp+Ch] [ebp-4h]

		currentKnotCount = *graph->knotCount;
		if (graph->selectedKnot && graph->selectedKnot + 1 != currentKnotCount)
		{
			for (knotIndex = graph->selectedKnot; knotIndex < currentKnotCount - 1; ++knotIndex)
			{
				v2 = graph->knots[knotIndex];
				*v2 = v2[2];
				v2[1] = v2[3];
			}
			graph->knots[currentKnotCount - 1][0] = -1.0f;
			graph->knots[currentKnotCount - 1][1] = -1.0f;
			--*graph->knotCount;
			if (graph->eventCallback)
				graph->eventCallback(graph, Game::EVENT_UPDATE, localClientNum);
		}
	}

	double Devgui::DevGui_UpdateFloatScroll( float deltaTime, float value, float min, float max, float step, Game::DevGuiInputAxis axis)
	{
		float analog; // [esp+0h] [ebp-10h]
		float range; // [esp+4h] [ebp-Ch]
		float stepTime; // [esp+8h] [ebp-8h]
		float scroll; // [esp+Ch] [ebp-4h]
		float valuea; // [esp+1Ch] [ebp+Ch]

		range = max - min;
		analog = s_input.analogAxis[axis] * s_input.scrollScale / s_input.sliderScrollTime;
		scroll = analog * range * deltaTime;
		if (s_input.digitalStates[axis] == Game::SCROLL_HELD)
		{
			stepTime = 0.1000000014901161 / s_input.scrollScale;
			if (s_input.sliderScrollMaxTimeStep < (double)stepTime)
				stepTime = s_input.sliderScrollMaxTimeStep;
			for (s_input.digitalSliderTime = s_input.digitalSliderTime + deltaTime;
				stepTime < (double)s_input.digitalSliderTime;
				s_input.digitalSliderTime = s_input.digitalSliderTime - stepTime)
			{
				scroll = s_input.digitalAxis[axis] * step + scroll;
			}
		}
		else if (s_input.digitalStates[axis] == Game::SCROLL_PRESSED)
		{
			scroll = s_input.digitalAxis[axis] * step + scroll;
		}
		valuea = value + scroll;
		if (min >= (double)valuea)
			return min;
		if (max > (double)valuea)
			return valuea;
		return max;
	}

	void Devgui::DevGui_UpdateGraph(int32_t localClientNum, float deltaTime)
	{
		float v2; // [esp+18h] [ebp-A0h]
		float v3; // [esp+1Ch] [ebp-9Ch]
		float v4; // [esp+20h] [ebp-98h]
		float v5; // [esp+28h] [ebp-90h]
		float v6; // [esp+2Ch] [ebp-8Ch]
		float v7; // [esp+30h] [ebp-88h]
		float v8; // [esp+34h] [ebp-84h]
		float v9; // [esp+38h] [ebp-80h]
		float v10; // [esp+3Ch] [ebp-7Ch]
		float v11; // [esp+40h] [ebp-78h]
		bool v12; // [esp+44h] [ebp-74h]
		float v13; // [esp+48h] [ebp-70h]
		float v14; // [esp+4Ch] [ebp-6Ch]
		float v15; // [esp+50h] [ebp-68h]
		float v16; // [esp+54h] [ebp-64h]
		float v17; // [esp+58h] [ebp-60h]
		float v18; // [esp+5Ch] [ebp-5Ch]
		float v19; // [esp+60h] [ebp-58h]
		float v20; // [esp+64h] [ebp-54h]
		float v21; // [esp+68h] [ebp-50h]
		float v22; // [esp+6Ch] [ebp-4Ch]
		float* v23; // [esp+74h] [ebp-44h]
		float* v24; // [esp+78h] [ebp-40h]
		__int16 xAxisDelta; // [esp+7Ch] [ebp-3Ch]
		bool graphUpdated; // [esp+83h] [ebp-35h]
		int32_t currentKnotCount; // [esp+84h] [ebp-34h]
		Game::DevGraph* graph; // [esp+90h] [ebp-28h]
		float deltaX; // [esp+94h] [ebp-24h]
		Game::devguiGlob_t* menu; // [esp+98h] [ebp-20h]
		float updatedY; // [esp+9Ch] [ebp-1Ch]
		float updatedX; // [esp+A8h] [ebp-10h]
		float knot; // [esp+ACh] [ebp-Ch]
		float knot_4; // [esp+B0h] [ebp-8h]
		float deltaY; // [esp+B4h] [ebp-4h]
		float deltaYa; // [esp+B4h] [ebp-4h]

		menu = DevGui_GetMenu(devguiGlob.selectedMenu);
		graph = menu->menus[0].child.graph;
		currentKnotCount = *graph->knotCount;
		if (graph->selectedKnot < 0 || graph->selectedKnot >= currentKnotCount)
			graph->selectedKnot = 0;
		if (DevGui_IsButtonPressed(Game::INPUT_GRAPH_EDIT))
			devguiGlob.editingKnot = !devguiGlob.editingKnot;
		if (DevGui_IsButtonPressed(Game::INPUT_GRAPH_ADD))
		{
			DevGui_AddGraphKnot(graph, localClientNum);
			devguiGlob.editingKnot = 0;
		}
		else if (DevGui_IsButtonPressed(Game::INPUT_GRAPH_REMOVE))
		{
			DevGui_RemoveGraphKnot(graph, localClientNum);
			devguiGlob.editingKnot = 0;
		}
		else if (DevGui_IsButtonPressed(Game::INPUT_GRAPH_SAVE))
		{
			if (graph->eventCallback)
				graph->eventCallback(graph, Game::EVENT_SAVE, localClientNum);
		}
		else if (DevGui_IsButtonPressed(Game::INPUT_ACCEPT))
		{
			if (graph->eventCallback)
				graph->eventCallback(graph, Game::EVENT_ACCEPT, localClientNum);
		}
		else if (DevGui_IsButtonPressed(Game::INPUT_REJECT))
		{
			if (graph->eventCallback)
				graph->eventCallback(graph, Game::EVENT_DEACTIVATE, localClientNum);
			devguiGlob.editingMenuItem = 0;
		}
		else if (devguiGlob.editingKnot)
		{
			v24 = graph->knots[graph->selectedKnot];
			knot = *v24;
			knot_4 = v24[1];
			updatedX = DevGui_UpdateFloatScroll(deltaTime, knot, 0.0f, 1.0f, 0.029999999f, Game::SCROLL_XAXIS);
			updatedY = DevGui_UpdateFloatScroll(deltaTime, knot_4, 0.0f, 1.0f, 0.029999999f, Game::SCROLL_YAXIS);
			if (graph->selectedKnot <= 0 || graph->selectedKnot + 1 >= currentKnotCount)
			{
				if (graph->disableEditingEndPoints)
				{
					graphUpdated = 0;
				}
				else
				{
					deltaYa = updatedY - knot_4;
					v5 = fabs(deltaYa);
					graphUpdated = v5 > 0.0000009999999974752427f;
					graph->knots[graph->selectedKnot][1] = deltaYa * 4.0 + graph->knots[graph->selectedKnot][1];
					v15 = graph->knots[graph->selectedKnot][1];
					v4 = v15 - 1.0;
					if (v4 < 0.0)
						v16 = v15;
					else
						v16 = 1.0;
					v3 = 0.0 - v15;
					if (v3 < 0.0)
						v2 = v16;
					else
						v2 = 0.0;
					graph->knots[graph->selectedKnot][1] = v2;
				}
			}
			else
			{
				deltaX = updatedX - knot;
				deltaY = updatedY - knot_4;
				v14 = fabs(deltaX);
				v12 = 1;
				if (v14 <= 0.0000009999999974752427f)
				{
					v13 = fabs(deltaY);
					if (v13 <= 0.0000009999999974752427f)
						v12 = 0;
				}
				graphUpdated = v12;
				graph->knots[graph->selectedKnot][0] = deltaX * 3.0 + graph->knots[graph->selectedKnot][0];
				graph->knots[graph->selectedKnot][1] = deltaY * 4.0 + graph->knots[graph->selectedKnot][1];
				v23 = graph->knots[graph->selectedKnot - 1];
				v19 = graph->knots[graph->selectedKnot][0];
				v21 = graph->knots[graph->selectedKnot + 1][0] - 0.004999999888241291f;
				v11 = v19 - v21;
				if (v11 < 0.0f)
					v22 = v19;
				else
					v22 = graph->knots[graph->selectedKnot + 1][0] - 0.004999999888241291f;
				v20 = *v23 + 0.004999999888241291f;
				v10 = v20 - v19;
				if (v10 < 0.0f)
					v9 = v22;
				else
					v9 = *v23 + 0.004999999888241291f;
				graph->knots[graph->selectedKnot][0] = v9;
				v17 = graph->knots[graph->selectedKnot][1];
				v8 = v17 - 1.0f;
				if (v8 < 0.0f)
					v18 = v17;
				else
					v18 = 1.0f;
				v7 = 0.0f - v17;
				if (v7 < 0.0f)
					v6 = v18;
				else
					v6 = 0.0f;
				graph->knots[graph->selectedKnot][1] = v6;
			}
			if (graphUpdated && graph->eventCallback)
				graph->eventCallback(graph, Game::EVENT_UPDATE, localClientNum);
		}
		else
		{
			xAxisDelta = DevGui_GetMenuScroll(Game::SCROLL_XAXIS);
			if (xAxisDelta >= 0 || graph->selectedKnot <= 0)
			{
				if (xAxisDelta > 0 && currentKnotCount > graph->selectedKnot + 1)
					++graph->selectedKnot;
			}
			else
			{
				--graph->selectedKnot;
			}
			DevGui_UpdateSelection();
		}
	}

	int32_t Devgui::DevGui_UpdateIntScroll(float deltaTime, int32_t value, int32_t min, int32_t max, Game::DevGuiInputAxis axis)
	{
		float v6; // [esp+0h] [ebp-4Ch]
		float v7; // [esp+4h] [ebp-48h]
		float v8; // [esp+8h] [ebp-44h]
		float v9; // [esp+14h] [ebp-38h]
		int32_t range; // [esp+40h] [ebp-Ch]
		float stepTime; // [esp+44h] [ebp-8h]
		float stepTimea; // [esp+44h] [ebp-8h]
		int32_t scroll; // [esp+48h] [ebp-4h]
		int32_t valuea; // [esp+58h] [ebp+Ch]

		range = max - min;
		if (!range)
			return 0;
		scroll = 0;
		if (s_input.digitalStates[axis] == Game::SCROLL_HELD)
		{
			stepTime = 0.1000000014901161 / s_input.scrollScale;
			if (s_input.sliderScrollMaxTimeStep < (double)stepTime)
				stepTime = s_input.sliderScrollMaxTimeStep;
			for (s_input.digitalSliderTime = s_input.digitalSliderTime + deltaTime;
				stepTime < (double)s_input.digitalSliderTime;
				s_input.digitalSliderTime = s_input.digitalSliderTime - stepTime)
			{
				scroll += (int)(s_input.digitalAxis[axis]);
			}
		}
		else if (s_input.digitalStates[axis] == Game::SCROLL_PRESSED)
		{
			scroll = (int)(s_input.digitalAxis[axis]);
		}
		else
		{
			s_input.digitalSliderTime = 0.0;
		}
		if (s_input.analogAxis[axis] == 0.0)
		{
			s_input.analogSliderTime = 0.0;
		}
		else
		{
			v9 = s_input.scrollScale * s_input.analogAxis[axis];
			v8 = fabs(v9);
			stepTimea = s_input.sliderScrollTime / (double)range / v8;
			if (s_input.sliderScrollMaxTimeStep < (double)stepTimea)
				stepTimea = s_input.sliderScrollMaxTimeStep;
			s_input.analogSliderTime = s_input.analogSliderTime + deltaTime;
			while (stepTimea <= (double)s_input.analogSliderTime)
			{
				if (s_input.analogAxis[axis] < 0.0)
					v7 = -1.0;
				else
					v7 = 1.0;
				scroll += (int)v7;
				s_input.analogSliderTime = s_input.analogSliderTime - stepTimea;
				v6 = fabs(s_input.analogAxis[axis]);
				if (v6 <= 2.0)
				{
					s_input.analogSliderTime = 0.0;
					break;
				}
			}
		}
		valuea = scroll + value;
		if (valuea <= min)
			return min;
		if (valuea < max)
			return valuea;
		return max;
	}

	float Devgui::DevGui_PickFloatScrollStep(float min, float max)
	{
		float v3; // [esp+0h] [ebp-44h]
		float v4; // [esp+4h] [ebp-40h]
		float range; // [esp+38h] [ebp-Ch]
		float step; // [esp+3Ch] [ebp-8h]
		float roundedStep; // [esp+40h] [ebp-4h]

		range = max - min;
		if (max == (float)(int)(max) && min == (float)(int)(min))
		{
			for (step = 1.0f; range > step * 100.0f; step = step + step)
				;
			while (range < step * 100.0f)
				step = step * 0.5f;
		}
		else
		{
			step = range * 0.009999999776482582f;
			roundedStep = (float)(int)(step);
			if (roundedStep != 0.0f)
			{
				v4 = roundedStep - step;
				v3 = fabs(v4);
				if (v3 < 0.1f)
					return (float)(int)(step);
			}
		}
		return step;
	}

	void Dvar_SetBoolFromSource(Game::dvar_s* dvar, bool value, Game::DvarSetSource source)
	{
		const char* v3; // [esp+0h] [ebp-18h]
		Game::DvarValue newValue; // [esp+4h] [ebp-14h]

		if ((bool)dvar->type)
		{
			if (value)
				v3 = "1";
			else
				v3 = "0";
			newValue.integer = (int)v3;
		}
		else
		{
			newValue.enabled = value;
		}
		Game::Dvar_SetVariant(dvar, newValue, source);
	}

	void Dvar_SetFloatFromSource(Game::dvar_s* dvar, float value, Game::DvarSetSource source)
	{
		char string[32]; // [esp+8h] [ebp-34h] BYREF
		Game::DvarValue newValue; // [esp+28h] [ebp-14h]

		if (dvar->type == Game::dvar_type::value)
		{
			newValue.value = value;
		}
		else
		{
			sprintf_s(string, 0x20u, "%g", value);
			newValue.integer = (int)string;
		}
		Game::Dvar_SetVariant(dvar, newValue, source);
	}

	void Dvar_SetVec2FromSource(Game::dvar_s* dvar, float x, float y, Game::DvarSetSource source)
	{
		char string[68]; // [esp+10h] [ebp-58h] BYREF
		Game::DvarValue newValue; // [esp+54h] [ebp-14h]

		if (dvar->type == Game::dvar_type::vec4)
		{
			newValue.value = x;
			newValue.vector[1] = y;
		}
		else
		{
			sprintf_s(string, 0x40u, "%g %g", x, y);
			newValue.integer = (int)string;
		}
		Game::Dvar_SetVariant(dvar, newValue, source);
	}

	void Dvar_SetIntFromSource(Game::dvar_s* dvar, int value, Game::DvarSetSource source)
	{
		char string[32]; // [esp+0h] [ebp-34h] BYREF
		Game::DvarValue newValue; // [esp+20h] [ebp-14h]

		if (dvar->type == Game::dvar_type::integer || dvar->type == Game::dvar_type::enumeration)
		{
			newValue.integer = value;
		}
		else
		{
			sprintf_s(string, 0x20u, "%i", value);
			newValue.integer = (int)string;
		}
		Game::Dvar_SetVariant(dvar, newValue, source);
	}

	void Devgui::Dvar_SetVec3FromSource(Game::dvar_s* dvar, float x, float y, float z, Game::DvarSetSource source)
	{
		char string[100]; // [esp+18h] [ebp-78h] BYREF
		Game::DvarValue newValue; // [esp+7Ch] [ebp-14h]

		if (dvar->type == Game::dvar_type::vec3)
		{
			newValue.value = x;
			newValue.vector[1] = y;
			newValue.vector[2] = z;
		}
		else
		{
			sprintf_s(string, 0x60u, "%g %g %g", x, y, z);
			newValue.integer = (int)string;
		}
		Game::Dvar_SetVariant(dvar, newValue, source);
	}

	void Dvar_SetVec4FromSource(Game::dvar_s* dvar, float x, float y, float z, float w, Game::DvarSetSource source)
	{
		char string[132]; // [esp+20h] [ebp-98h] BYREF
		Game::DvarValue newValue; // [esp+A4h] [ebp-14h]
		if (dvar->type == Game::dvar_type::vec4)
		{
			newValue.value = x;
			newValue.vector[1] = y;
			newValue.vector[2] = z;
			newValue.vector[3] = w;
		}
		else
		{
			sprintf_s(string, 0x80u, "%g %g %g %g", x, y, z, w);
			newValue.integer = (int)string;
		}
		Game::Dvar_SetVariant(dvar, newValue, source);
	}

	void Dvar_SetColorFromSource(Game::dvar_s* dvar, float r, float g, float b, float a, Game::DvarSetSource source)
	{
		char string[132]; // [esp+B0h] [ebp-98h] BYREF
		Game::DvarValue newValue; // [esp+134h] [ebp-14h]

		if (dvar->type == Game::dvar_type::color)
		{
			float clampedR = CLAMP(r, 0.0f, 1.0f);
			newValue.color[0] = (byte)(clampedR * 255.0f);

			float clampedG = CLAMP(g, 0.0f, 1.0f);
			newValue.color[1] = (byte)(clampedG * 255.0f);

			float clampedB = CLAMP(b, 0.0f, 1.0f);
			newValue.color[2] = (byte)(clampedB * 255.0f);

			float clampedA = CLAMP(a, 0.0f, 1.0f);
			newValue.color[3] = (byte)(clampedA * 255.0f);
		}
		else
		{
			sprintf_s(string, 0x80u, "%g %g %g %g", r, g, b, a);
			newValue.integer = (int)string;
		}
		Game::Dvar_SetVariant(dvar, newValue, source);
	}

	void Devgui::DevGui_UpdateDvar(float deltaTime)
	{
		float floatValue; // [esp+38h] [ebp-34h]
		float floatValuea; // [esp+38h] [ebp-34h]
		bool boolValue; // [esp+3Fh] [ebp-2Dh]
		float step; // [esp+40h] [ebp-2Ch]
		float stepa; // [esp+40h] [ebp-2Ch]
		float stepb; // [esp+40h] [ebp-2Ch]
		float stepc; // [esp+40h] [ebp-2Ch]
		float vector[4]; // [esp+44h] [ebp-28h] BYREF
		const Game::dvar_s* dvar; // [esp+54h] [ebp-18h]
		float color[4]; // [esp+58h] [ebp-14h] BYREF
		int32_t intValue; // [esp+68h] [ebp-4h]

		dvar = DevGui_SelectedDvar();
		switch (dvar->type)
		{
		case Game::dvar_type::boolean:
			boolValue = DevGui_UpdateIntScroll(deltaTime, dvar->latched.color[0] != 0, 0, 1, Game::SCROLL_XAXIS) != 0;
			if (boolValue != dvar->latched.enabled)
				Dvar_SetBoolFromSource((Game::dvar_s*)dvar, boolValue, Game::DVAR_SOURCE_DEVGUI);
			break;
		case Game::dvar_type::value:
			step = DevGui_PickFloatScrollStep(dvar->domain.value.min, dvar->domain.value.max);
			floatValue = DevGui_UpdateFloatScroll(
				deltaTime,
				dvar->latched.value,
				dvar->domain.value.min,
				dvar->domain.value.max,
				step,
				Game::SCROLL_XAXIS);
			if (dvar->latched.value != floatValue)
				Dvar_SetFloatFromSource((Game::dvar_s*)dvar, floatValue, Game::DVAR_SOURCE_DEVGUI);
			break;
		case Game::dvar_type::vec2:
			stepa = DevGui_PickFloatScrollStep(dvar->domain.value.min, dvar->domain.value.max);
			vector[0] = dvar->latched.value;
			vector[1] = dvar->latched.vector[1];
			vector[devguiGlob.selRow] = DevGui_UpdateFloatScroll(
				deltaTime,
				vector[devguiGlob.selRow],
				dvar->domain.value.min,
				dvar->domain.value.max,
				stepa,
				Game::SCROLL_XAXIS);
			if (vector[0] != dvar->latched.value || vector[1] != dvar->latched.vector[1])
				Dvar_SetVec2FromSource((Game::dvar_s*)dvar, vector[0], vector[1], Game::DVAR_SOURCE_DEVGUI);
			break;
		case Game::dvar_type::vec3:
			stepb = DevGui_PickFloatScrollStep(dvar->domain.value.min, dvar->domain.value.max);
			vector[0] = dvar->latched.value;
			vector[1] = dvar->latched.vector[1];
			vector[2] = dvar->latched.vector[2];
			vector[devguiGlob.selRow] = DevGui_UpdateFloatScroll(
				deltaTime,
				vector[devguiGlob.selRow],
				dvar->domain.value.min,
				dvar->domain.value.max,
				stepb,
				Game::SCROLL_XAXIS);
			if (vector[0] != dvar->latched.value
				|| vector[1] != dvar->latched.vector[1]
				|| vector[2] != dvar->latched.vector[2])
			{
				Dvar_SetVec3FromSource((Game::dvar_s*)dvar, vector[0], vector[1], vector[2], Game::DVAR_SOURCE_DEVGUI);
			}
			break;
		case Game::dvar_type::vec4:
			stepc = DevGui_PickFloatScrollStep(dvar->domain.value.min, dvar->domain.value.max);
			vector[0] = dvar->latched.value;
			vector[1] = dvar->latched.vector[1];
			vector[2] = dvar->latched.vector[2];
			vector[3] = dvar->latched.vector[3];
			vector[devguiGlob.selRow] = DevGui_UpdateFloatScroll(
				deltaTime,
				vector[devguiGlob.selRow],
				dvar->domain.value.min,
				dvar->domain.value.max,
				stepc,
				Game::SCROLL_XAXIS);
			if (!Game::Vec4Compare(&dvar->latched.value, vector))
				Dvar_SetVec4FromSource((Game::dvar_s*)dvar, vector[0], vector[1], vector[2], vector[3], Game::DVAR_SOURCE_DEVGUI);
			break;
		case Game::dvar_type::integer:
			intValue = DevGui_UpdateIntScroll(
				deltaTime,
				dvar->latched.integer,
				dvar->domain.enumeration.stringCount,
				dvar->domain.integer.max,
				Game::SCROLL_XAXIS);
			if (intValue != dvar->latched.integer)
				Dvar_SetIntFromSource((Game::dvar_s*)dvar, intValue, Game::DVAR_SOURCE_DEVGUI);
			break;
		case Game::dvar_type::enumeration:
			intValue = DevGui_UpdateIntScroll(
				deltaTime,
				dvar->latched.integer,
				0,
				dvar->domain.enumeration.stringCount - 1,
				Game::SCROLL_XAXIS);
			if (intValue != dvar->latched.integer)
				Dvar_SetIntFromSource((Game::dvar_s*)dvar, intValue, Game::DVAR_SOURCE_DEVGUI);
			break;
		case Game::dvar_type::color:
			Game::Byte4UnpackRgba((const uint8_t*)&dvar->latched, color);
			floatValuea = color[devguiGlob.selRow];
			color[devguiGlob.selRow] = DevGui_UpdateFloatScroll(deltaTime, floatValuea, 0.0f, 1.0f, 0.019607844f, Game::SCROLL_XAXIS);
			if (color[devguiGlob.selRow] != floatValuea)
				Dvar_SetColorFromSource((Game::dvar_s*)dvar, color[0], color[1], color[2], color[3], Game::DVAR_SOURCE_DEVGUI);
			break;
		default:
			break;
		}
	}

	void Devgui::DevGui_Update(int32_t localClientNum, float deltaTime)
	{
		Game::devguiGlob_t* selMenuItem; // [esp+4h] [ebp-4h]

		if (devguiGlob.isActive && !devguiGlob.bindNextKey && DevGui_InputUpdate(localClientNum, deltaTime))
		{
			if (DevGui_IsButtonReleased(Game::INPUT_BIND) && devguiGlob.selectedMenu)
			{
				devguiGlob.bindNextKey = 1;
			}
			else
			{
				selMenuItem = DevGui_GetMenu(devguiGlob.selectedMenu);
				if (devguiGlob.editingMenuItem && selMenuItem->menus[0].childType == 3)
				{
					DevGui_UpdateGraph(localClientNum, deltaTime);
				}
				else
				{
					if (devguiGlob.editingMenuItem)
					{
						DevGui_UpdateDvar(deltaTime);
						DevGui_UpdateSelection();
					}
					else
					{
						DevGui_MoveSelectionHorizontally();
						DevGui_MoveSelectionVertically();
					}
					if (DevGui_IsButtonReleased(Game::INPUT_ACCEPT))
						DevGui_Accept(localClientNum);
					if (DevGui_IsButtonReleased(Game::INPUT_REJECT))
						DevGui_Reject();
				}
			}
		}
	}

	void Devgui::GraphFloat_DevGuiCB_Text(const Game::DevGraph* devGuiGraph, float inputX, float inputY, char* text)
	{
		float inputYa; // [esp+24h] [ebp+10h]

		inputYa = inputY * *((float*)devGuiGraph->data + 81);
		sprintf(text, "Fraction: %.3f, Value: %.3f", inputX, inputYa);
	}

	void Devgui::DevGui_AddGraph(const char* path, Game::DevGraph* graph)
	{
		uint16_t handle; // [esp+0h] [ebp-8h]
		Game::devguiGlob_t* menu; // [esp+4h] [ebp-4h]

		if (DevGui_IsValidPath(path))
		{
			handle = DevGui_ConstructPath_r(0, path);
			menu = DevGui_GetMenu(handle);
			if (!menu->menus[0].childType && !menu->menus[0].child.menu
				|| menu->menus[0].childType == 3 && menu->menus[0].child.graph == graph)
			{
				menu->menus[0].childType = 3;
				menu->menus[0].child.command = (const char*)graph;
			}
			else
			{
				Game::Com_Printf(11, "Path '%s' can't be added for this graph because it is already used for something else.\n", path);
			}
		}
	}

	void Devgui::GraphFloat_DevGuiCB_Event(const Game::DevGraph* graph, Game::DevEventType event)
	{
		//TODO: FIX ME!
	}

	void Devgui::GraphFloat_CreateDevGui(Game::GraphFloat* graph, const char* devguiPath)
	{
		// TODO: FIX THIS!
		//graph->devguiGraph.knotCountMax = 32;
		//graph->devguiGraph.knots = graph->knots;
		//graph->devguiGraph.knotCount = &graph->knotCount;
		//graph->devguiGraph.eventCallback = (void(__cdecl*)(const Game::DevGraph*, Game::DevEventType, int))GraphFloat_DevGuiCB_Event;
		//graph->devguiGraph.textCallback = (void(__cdecl*)(const Game::DevGraph*, const float, const float, char*, const int))GraphFloat_DevGuiCB_Text;
		//graph->devguiGraph.data = graph;
		//DevGui_AddGraph(devguiPath, &graph->devguiGraph);
	}

	void Devgui::AimAssist_InitStub()
	{
		char graphName[128];
		int graphIndex;

		Utils::Hook::Call<void()>(0x401BB0)();

		Game::Cbuf_InsertText(0, "exec devgui_aimassist\n");
		for (graphIndex = 0; graphIndex < 4; ++graphIndex)
		{
			sprintf_s(graphName, 0x80u, "Aim Assist/Input Graph:1/Graph %d", graphIndex);
			GraphFloat_CreateDevGui(&Game::aaInputGraph[graphIndex], graphName);
		}

		CG_InitVisionSetsMenu();

		//add model preview here!
		//DevguiModelPreview::CG_ModelPreviewerCreateDevGui();
	}

	bool Devgui::CL_IsConsoleKey(int32_t key)
	{
		// key 184 isn't apart of ASCII
		return (key == '`' || key == '^' || key == 184 || key == '~');
	}

	void Devgui::DevGui_KeyPressed(int32_t key)
	{
		const char* v1; // eax
		uint16_t handle; // [esp+0h] [ebp-8Ch]
		char path[128]; // [esp+4h] [ebp-88h] BYREF
		Game::DevMenuItem* menu; // [esp+88h] [ebp-4h]

		if (devguiGlob.bindNextKey)
		{
			devguiGlob.bindNextKey = 0;
			if (key != 27)
			{
				if (key == 9 || key == 167)
				{
					Game::Com_Printf(11, "Can't rebind 'tab' or 'F1'\n");
				}
				else
				{
					handle = devguiGlob.selectedMenu;
					menu = (Game::DevMenuItem*)DevGui_GetMenu(handle);
					if (menu->parent && (!menu->childType || menu->childType == 1 && !devguiGlob.editingMenuItem))
						handle = menu->parent;
					DevGui_GetSliderPath(handle, path, 0);
					v1 = Utils::String::VA("devgui_open \"%s\"", path);
					Game::Key_SetBinding(0, key, v1);
				}
			}
		}
	}

	void Devgui::DevGui_KeyEvent(const int localClientNum, const int key, const int down, const unsigned time)
	{
		Game::KeyState* keys;

		keys = Game::playerKeys[localClientNum].keys;
		keys[key].down = down;

		if (down)
		{
			if (++keys[key].repeats == 1)
				++Game::playerKeys[localClientNum].anyKeyDown;
		}
		else
		{
			keys[key].repeats = 0;
			if (--Game::playerKeys[localClientNum].anyKeyDown < 0)
				Game::playerKeys[localClientNum].anyKeyDown = 0;
		}

		if (CL_IsConsoleKey(key) || (Game::clientUIActive->keyCatchers & 3) != 0)
		{
			if (DevGui_IsActive())
				DevGui_Toggle();
		}
		else
		{
			const char* kb = keys[key].binding;
			if (kb && !_stricmp(kb, "devgui") && down && keys[key].repeats == 1)
			{
				Game::Cbuf_AddText(localClientNum, keys[key].binding);
				return;
			}
			if (DevGui_IsActive())
			{
				if (down)
				{
					if (keys[key].repeats == 1)
						DevGui_KeyPressed(key);
				}
				return;
			}
		}
	}

	void Devgui::CL_InitUI_stub01()
	{
		Utils::Hook::Call<void()>(0x447F40)(); //CL_InitUI
		CL_InitDevGui();
	}

	void __declspec(naked) Devgui::CL_StartHunkUsersStub()
	{
		const static uint32_t jmp_continue = 0x44554A;
		const static uint32_t jmp_retn_end = 0x445580;
		__asm
		{
			cmp		ds : 85BD70h, 0;
			jz		NOT_LOADED;
			call	CL_InitDevGui;
			jmp		jmp_continue;
		NOT_LOADED:
			jmp		jmp_retn_end;
		}
	}

	Devgui::Devgui()
	{
		Utils::Hook::Nop(0x445541, 7);
		Utils::Hook(0x445541, CL_StartHunkUsersStub, HOOK_JUMP).install()->quick();
		Utils::Hook(0x421192, AimAssist_InitStub, HOOK_CALL).install()->quick();

		Command::Add("VisionSetNaked", [](Command::Params* params)
		{
			if (params->size() < 1)
			{
				Game::Com_Printf(0, "USAGE: visionSetNaked <name> <duration>\n");
				return;
			}

			const char* visionName = params->get(1);
			int duration = 1000;
			if (params->size() == 2)
			{
				duration = atof(params->get(2)) * 1000.0f + 0.5f;
			}
			
			const char* configString = Utils::String::VA("\"%s\" %i", visionName, duration);
			Game::SV_SetConfigstring(1117, configString);
		});

		Scheduler::Loop([]
		{
			DevGui_Draw(0);
			DevGui_Update(0, Game::cls->frametime * 0.001f);
		}, Scheduler::Pipeline::RENDERER);
	}

	Devgui::~Devgui()
	{
	}
}