#include "STDInc.hpp"

namespace Dvars
{
	Game::dvar_s* ui_language = nullptr;
	Game::dvar_s* language_first_setting = nullptr;
	Game::dvar_s* r_noborder = nullptr;
	Game::dvar_s* r_videomode = nullptr;
	Game::dvar_s* ui_nativeCursor = nullptr;
	Game::dvar_s* safeArea_horizontal = nullptr;
	Game::dvar_s* safeArea_vertical = nullptr;

	// d3d9
	Game::dvar_s* r_d3d9ex = nullptr;
	Game::dvar_s* r_increaseRenderBuffers = nullptr;

	// Field of view dvars
	Game::dvar_s* cg_fovScale = nullptr;
	Game::dvar_s* cg_fovMin = nullptr;
	Game::dvar_s* cg_fov = nullptr;
	Game::dvar_s* cg_drawViewpos = nullptr;

	Game::dvar_s* ui_debugMode = nullptr;
	Game::dvar_s* console_menu_style = nullptr;

	// Player settings dvars
	Game::dvar_s* p_allowFire = nullptr;
	Game::dvar_s* player_sprintUnlimited = nullptr;
	Game::dvar_s* paintball_mode = nullptr;

	Game::dvar_s* external_console = nullptr;
	Game::dvar_s* allowCompactFolderForMod = nullptr;

	Game::dvar_s* M_RawInput = nullptr;

	// Gamepad
	Game::dvar_s* gpad_enabled = nullptr;
	Game::dvar_s* gpad_debug = nullptr;
	Game::dvar_s* gpad_present = nullptr;
	Game::dvar_s* gpad_in_use = nullptr;
	Game::dvar_s* gpad_style = nullptr;
	Game::dvar_s* gpad_sticksConfig = nullptr;
	Game::dvar_s* gpad_buttonConfig = nullptr;
	Game::dvar_s* gpad_menu_scroll_delay_first = nullptr;
	Game::dvar_s* gpad_menu_scroll_delay_rest = nullptr;
	Game::dvar_s* gpad_stick_pressed_hysteresis = nullptr;
	Game::dvar_s* gpad_stick_pressed = nullptr;
	Game::dvar_s* gpad_stick_deadzone_min = nullptr;
	Game::dvar_s* gpad_stick_deadzone_max = nullptr;
	Game::dvar_s* gpad_button_deadzone = nullptr;
	Game::dvar_s* gpad_button_lstick_deflect_max = nullptr;
	Game::dvar_s* gpad_button_rstick_deflect_max = nullptr;
	Game::dvar_s* gpad_use_hold_time = nullptr;
	Game::dvar_s* gpad_lockon_enabled = nullptr;
	Game::dvar_s* gpad_slowdown_enabled = nullptr;
	Game::dvar_s* gpad_autoaim_enabled = nullptr;
	Game::dvar_s* gpad_aimassist = nullptr;
	Game::dvar_s* input_viewSensitivity = nullptr;
	Game::dvar_s* input_invertPitch = nullptr;
	Game::dvar_s* aim_aimAssistRangeScale = nullptr;
	Game::dvar_s* aim_autoAimRangeScale = nullptr;
	Game::dvar_s* aim_lockon_pitch_strength = nullptr;

	//devgui
	Game::dvar_s* devgui_colorBgnd = nullptr;
	Game::dvar_s* devgui_colorText = nullptr;
	Game::dvar_s* devgui_colorBgndSel = nullptr;
	Game::dvar_s* devgui_colorTextSel = nullptr;
	Game::dvar_s* devgui_colorBgndGray = nullptr;
	Game::dvar_s* devgui_colorTextGray = nullptr;
	Game::dvar_s* devgui_colorBgndGraySel = nullptr;
	Game::dvar_s* devgui_colorTextGraySel = nullptr;
	Game::dvar_s* devgui_colorSliderBgnd = nullptr;
	Game::dvar_s* devgui_colorSliderKnob = nullptr;
	Game::dvar_s* devgui_colorSliderKnobSel = nullptr;
	Game::dvar_s* devgui_bevelShade = nullptr;
	Game::dvar_s* devgui_colorGraphKnotNormal = nullptr;
	Game::dvar_s* devgui_colorGraphKnotSelected = nullptr;
	Game::dvar_s* devgui_colorGraphKnotEditing = nullptr;

	namespace Functions
	{
		Dvar_FindVar_t Dvar_FindVar = (Dvar_FindVar_t)0x587B00;

		void Dvar_SetStringByName(const char* dvarName, const char* value)
		{
			const static uint32_t Dvar_SetStringByName_func = 0x589720;
			__asm
			{
				mov		eax, dvarName;
				push	value;
				call	Dvar_SetStringByName_func;
				add     esp, 4;
			}
		}

		void Dvar_SetIntByName(const char* dvarName, int value)
		{
			const static uint32_t Dvar_SetIntByName_func = 0x589630;
			__asm
			{
				mov		eax, dvarName;
				push	value;
				call	Dvar_SetIntByName_func;
				add     esp, 4;
			}
		}

		void Dvar_Reset(Game::DvarSetSource setSource, Game::dvar_s* dvar)
			{
			Game::Dvar_SetVariant(dvar, dvar->reset, setSource);
		}

		void SetRaw(const char* dvarName, bool value)
		{
			if (const auto& dvar = Functions::Dvar_FindVar(dvarName); dvar)
			{
				dvar->current.enabled = value;
				dvar->latched.enabled = value;
			}
		}

		void SetRaw(Game::dvar_s* dvar, bool value)
		{
			dvar->current.enabled = value;
			dvar->latched.enabled = value;
		}

		void SetRaw(const char* dvarName, const char* value)
		{
			if (const auto& dvar = Functions::Dvar_FindVar(dvarName); dvar)
			{
				dvar->current.string = value;
				dvar->latched.string = value;
			}
		}

		void SetRaw(Game::dvar_s* dvar, const char* value)
		{
			dvar->current.string = value;
			dvar->latched.string = value;
		}

		const char* Dvar_ValueToString(const Game::dvar_s* dvar, Game::DvarValue value)
		{
			const char* text = "";
			const char* v4;

			switch (dvar->type)
			{
			case Game::dvar_type::boolean:
				if (value.enabled)
					v4 = "1";
				else
					v4 = "0";
				text = v4;
				break;
			case Game::dvar_type::value:
				text = Utils::String::VA("%g", value.value);
				break;
			case Game::dvar_type::vec2:
				text = Utils::String::VA("%g %g", value.value, value.vector[1]);
				break;
			case Game::dvar_type::vec3:
				text = Utils::String::VA("%g %g %g", value.value, value.vector[1], value.vector[2]);
				break;
			case Game::dvar_type::vec4:
				text = Utils::String::VA("%g %g %g %g", value.value, value.vector[1], value.vector[2], value.vector[3]);
				break;
			case Game::dvar_type::integer:
				text = Utils::String::VA("%i", value.integer);
				break;
			case Game::dvar_type::enumeration:
				if ((value.integer < 0 || value.integer >= dvar->domain.enumeration.stringCount) && value.integer)
					if (dvar->domain.enumeration.stringCount)
						text = *(const char**)(dvar->domain.integer.max + 4 * value.integer);
					else
						text = "";
				break;
			case Game::dvar_type::string:
				text = Utils::String::VA("%s", value.string);
				break;
			case Game::dvar_type::color:
				text = Utils::String::VA(
					"%g %g %g %g",
					(double)value.color[0] * 0.003921568859368563,
					(double)value.color[1] * 0.003921568859368563,
					(double)value.color[2] * 0.003921568859368563,
					(double)value.color[3] * 0.003921568859368563);
				break;
			default:
				text = "";
				break;
			}
			return text;
		}
	}

	namespace Register
	{
		Game::dvar_s* Dvar_RegisterColor/*eax*/(const char* dvarName/*edi*/, float r, float g, float b, float a, int flags, const char* description)
		{
			static Game::dvar_s* result;
			const static uint32_t Dvar_RegisterColor_func = 0x588D40;
			__asm
			{
				push	description;
				push	flags;

				sub     esp, 10h;

				fld		a;
				fstp	[esp + 0Ch];

				fld		b;
				fstp	[esp + 8];

				fld		g;
				fstp	[esp + 4];

				fld		r;
				fstp	[esp];

				mov		edi, dvarName;
				call	Dvar_RegisterColor_func;
				add     esp, 18h;
				mov		result, eax;
			}
			return result;
		}
	}

	namespace Override
	{
		void DvarBoolOverride(const char* dvarName, const bool value, Game::dvar_flags flags)
		{
			if (const auto& dvar = Functions::Dvar_FindVar(dvarName); dvar)
			{
				dvar->current.enabled = value;
				dvar->latched.enabled = value;
				dvar->flags = flags;
				dvar->modified = false;
			}
		}

		void DvarBoolOverride(const char* dvarName, Game::dvar_flags flags)
		{
			if (const auto& dvar = Functions::Dvar_FindVar(dvarName); dvar)
			{
				dvar->flags = flags;
				dvar->modified = false;
			}
		}

		void DvarIntOverride(const char* dvarName, const int value, Game::dvar_flags flags)
		{
			if (const auto& dvar = Functions::Dvar_FindVar(dvarName); dvar)
			{
				dvar->current.integer = value;
				dvar->latched.integer = value;
				dvar->flags = flags;
				dvar->modified = false;
			}
		}

		void DvarVec4Override(const char* dvarName, const float* dvar_value)
		{
			if (const auto& dvar = Functions::Dvar_FindVar(dvarName); dvar)
			{
				for (auto i = 0; i < 4; i++)
				{
					dvar->current.vector[i] = dvar_value[i];
					dvar->latched.vector[i] = dvar_value[i];
					dvar->modified = false;
				}
			}
		}

		void DvarVec4OverrideDefaultValue(const char* dvarName, const float* dvar_value)
		{
			if (const auto& dvar = Functions::Dvar_FindVar(dvarName); dvar)
			{
				for (auto i = 0; i < 4; i++)
				{
					dvar->reset.vector[i] = dvar_value[i];
					dvar->modified = false;
				}
			}
		}

		void DvarFloatOverride(const char* dvarName, const float value, Game::dvar_flags flags)
		{
			if (const auto& dvar = Functions::Dvar_FindVar(dvarName); dvar)
			{
				dvar->current.value = value;
				dvar->latched.value = value;
				dvar->flags = flags;
				dvar->modified = false;
			}
		}

		void DvarFloatOverride(const char* dvarName, const float value, Game::dvar_flags flags, float mins, float maxs)
		{
			if (const auto& dvar = Functions::Dvar_FindVar(dvarName); dvar)
			{
				dvar->current.value = value;
				dvar->latched.value = value;
				dvar->domain.value.min = mins;
				dvar->domain.value.max = maxs;
				dvar->flags = flags;
				dvar->modified = false;
			}
		}
	}
}