/*
	Copyright 2019 flyinghead

	This file is part of reicast.

    reicast is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    reicast is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with reicast.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "gamepad_device.h"
#include "cfg/cfg.h"
#include "oslib/oslib.h"
#include "rend/gui.h"
#include "emulator.h"
#include "hw/maple/maple_devs.h"
#include "stdclass.h"

#include <algorithm>
#include <climits>
#include <fstream>

#include "dojo/DojoSession.hpp"

#define MAPLE_PORT_CFG_PREFIX "maple_"

// Gamepads
u32 kcode[4] = { ~0u, ~0u, ~0u, ~0u };
s8 joyx[4];
s8 joyy[4];
s8 joyrx[4];
s8 joyry[4];
u8 rt[4];
u8 lt[4];

std::vector<std::shared_ptr<GamepadDevice>> GamepadDevice::_gamepads;
std::mutex GamepadDevice::_gamepads_mutex;
bool loading_state;

#ifdef TEST_AUTOMATION
#include "hw/sh4/sh4_sched.h"
static FILE *record_input;
#endif

bool GamepadDevice::gamepad_btn_input(u32 code, bool pressed)
{
	if (_input_detected != nullptr && _detecting_button
			&& os_GetSeconds() >= _detection_start_time && pressed)
	{
		_input_detected(code);
		_input_detected = nullptr;
		return true;
	}
	if (!input_mapper || _maple_port < 0 || _maple_port > (int)ARRAY_SIZE(kcode))
		return false;

	auto handle_key = [&](u32 port, DreamcastKey key)
	{
		if (key == EMU_BTN_NONE)
			return false;

		if (key <= DC_BTN_RELOAD)
		{
			if (pressed)
			{
				kcode[port] &= ~key;
				// Avoid two opposite dpad keys being pressed simultaneously
				switch (key)
				{
				case DC_DPAD_UP:
					kcode[port] |= DC_DPAD_DOWN;
					break;
				case DC_DPAD_DOWN:
					kcode[port] |= DC_DPAD_UP;
					break;
				case DC_DPAD_LEFT:
					kcode[port] |= DC_DPAD_RIGHT;
					break;
				case DC_DPAD_RIGHT:
					kcode[port] |= DC_DPAD_LEFT;
					break;
				case DC_DPAD2_UP:
					kcode[port] |= DC_DPAD2_DOWN;
					break;
				case DC_DPAD2_DOWN:
					kcode[port] |= DC_DPAD2_UP;
					break;
				case DC_DPAD2_LEFT:
					kcode[port] |= DC_DPAD2_RIGHT;
					break;
				case DC_DPAD2_RIGHT:
					kcode[port] |= DC_DPAD2_LEFT;
					break;
				default:
					break;
				}
			}
			else
				kcode[port] |= key;
#ifdef TEST_AUTOMATION
			if (record_input != NULL)
				fprintf(record_input, "%ld button %x %04x\n", sh4_sched_now64(), port, kcode[port]);
#endif
		}
		else
		{
			switch (key)
			{
			case EMU_BTN_ESCAPE:
				if (pressed)
					dc_exit();
				break;
			case EMU_BTN_MENU:
				if (pressed)
					gui_open_settings();
				break;
			case EMU_BTN_FFORWARD:
				if (pressed)
					settings.input.fastForwardMode = !settings.input.fastForwardMode;
				break;
			case EMU_BTN_JUMP_STATE:
				if (pressed && !loading_state)
				{
					loading_state = true;
					bool dojo_invoke = config::DojoEnable.get();
					invoke_jump_state(dojo_invoke);
					loading_state = false;
				}
				break;
			case EMU_BTN_QUICK_SAVE:
				if (pressed && (!config::DojoEnable || config::Training))
				{
					dc_savestate();
				}
				break;
			case EMU_BTN_RECORD:
				if (pressed && config::Training)
				{
					dojo.ToggleRecording(0);
				}
				break;
			case EMU_BTN_PLAY:
				if (pressed && config::Training)
				{
					dojo.TogglePlayback(0);
				}
				break;
			case EMU_BTN_RECORD_1:
				if (pressed && config::Training)
				{
					dojo.ToggleRecording(1);
				}
				break;
			case EMU_BTN_PLAY_1:
				if (pressed && config::Training)
				{
					dojo.TogglePlayback(1);
				}
				break;
			case EMU_BTN_RECORD_2:
				if (pressed && config::Training)
				{
					dojo.ToggleRecording(2);
				}
				break;
			case EMU_BTN_PLAY_2:
				if (pressed && config::Training)
				{
					dojo.TogglePlayback(2);
				}
				break;
			case EMU_BTN_SWITCH_PLAYER:
				if (pressed && config::Training)
				{
					dojo.SwitchPlayer();
				}
				break;
			case EMU_BTN_TRIGGER_LEFT:
				lt[port] = pressed ? 255 : 0;
				break;
			case EMU_BTN_TRIGGER_RIGHT:
				rt[port] = pressed ? 255 : 0;
				break;
			case EMU_BTN_ANA_UP:
				joyy[port] = pressed ? -128 : 0;
				break;
			case EMU_BTN_ANA_DOWN:
				joyy[port] = pressed ? 127 : 0;
				break;
			case EMU_BTN_ANA_LEFT:
				joyx[port] = pressed ? -128 : 0;
				break;
			case EMU_BTN_ANA_RIGHT:
				joyx[port] = pressed ? 127 : 0;
				break;
			case DC_CMB_X_Y_A_B:
				if (pressed)
					kcode[port] &= ~(DC_BTN_X | DC_BTN_Y | DC_BTN_A | DC_BTN_B);
				else
					kcode[port] |= (DC_BTN_X | DC_BTN_Y | DC_BTN_A | DC_BTN_B);
				break;
			case DC_CMB_X_Y_LT:
				if (pressed)
					kcode[port] &= ~(DC_BTN_X | DC_BTN_Y);
				else
					kcode[port] |= (DC_BTN_X | DC_BTN_Y);
				lt[port] = pressed ? 255 : 0;
				break;
			case DC_CMB_A_B_RT:
				if (pressed)
					kcode[port] &= ~(DC_BTN_A | DC_BTN_B);
				else
					kcode[port] |= (DC_BTN_A | DC_BTN_B);
				rt[port] = pressed ? 255 : 0;
				break;
			case DC_CMB_X_A:
				if (pressed)
					kcode[port] &= ~(DC_BTN_X | DC_BTN_A);
				else
					kcode[port] |= (DC_BTN_X | DC_BTN_A);
				break;
			case DC_CMB_Y_B:
				if (pressed)
					kcode[port] &= ~(DC_BTN_Y | DC_BTN_B);
				else
					kcode[port] |= (DC_BTN_Y | DC_BTN_B);
				break;
			case DC_CMB_LT_RT:
				lt[port] = pressed ? 255 : 0;
				rt[port] = pressed ? 255 : 0;
				break;
			case NAOMI_CMB_1_2_3:
				if (pressed)
					kcode[port] &= ~(DC_BTN_A | DC_BTN_B | DC_BTN_X);
				else
					kcode[port] |= (DC_BTN_A | DC_BTN_B | DC_BTN_X);
				break;
			case NAOMI_CMB_4_5:
				if (pressed)
					kcode[port] &= ~(DC_BTN_Y | DC_DPAD2_UP);
				else
					kcode[port] |= (DC_BTN_Y | DC_DPAD2_UP);
				break;
			case NAOMI_CMB_4_5_6:
				if (pressed)
					kcode[port] &= ~(DC_BTN_Y | DC_DPAD2_UP | DC_DPAD2_DOWN);
				else
					kcode[port] |= (DC_BTN_Y | DC_DPAD2_UP | DC_DPAD2_DOWN);
				break;
			case NAOMI_CMB_1_4:
				if (pressed)
					kcode[port] &= ~(DC_BTN_A | DC_BTN_Y);
				else
					kcode[port] |= (DC_BTN_A | DC_BTN_Y);
				break;
			case NAOMI_CMB_2_5:
				if (pressed)
					kcode[port] &= ~(DC_BTN_B | DC_DPAD2_UP);
				else
					kcode[port] |= (DC_BTN_B | DC_DPAD2_UP);
				break;
			case NAOMI_CMB_3_4:
				if (pressed)
					kcode[port] &= ~(DC_BTN_X | DC_BTN_Y);
				else
					kcode[port] |= (DC_BTN_X | DC_BTN_Y);
				break;

			case NAOMI_CMB_3_6:
				if (pressed)
					kcode[port] &= ~(DC_BTN_X | DC_DPAD2_DOWN);
				else
					kcode[port] |= (DC_BTN_X | DC_DPAD2_DOWN);
				break;
			case NAOMI_CMB_1_2:
				if (pressed)
					kcode[port] &= ~(DC_BTN_A | DC_BTN_B);
				else
					kcode[port] |= (DC_BTN_A | DC_BTN_B);
				break;
			case NAOMI_CMB_2_3:
				if (pressed)
					kcode[port] &= ~(DC_BTN_B | DC_BTN_X);
				else
					kcode[port] |= (DC_BTN_B | DC_BTN_X);
				break;
			default:
				return false;
			}
		}

		DEBUG_LOG(INPUT, "%d: BUTTON %s %x -> %d. kcode=%x", port, pressed ? "down" : "up", code, key, kcode[port]);

		return true;
	};

	bool rc = false;
	if (_maple_port == 4)
	{
		for (int port = 0; port < 4; port++)
		{
			DreamcastKey key = input_mapper->get_button_id(port, code);
			rc = handle_key(port, key) || rc;
		}
	}
	else
	{
		DreamcastKey key = input_mapper->get_button_id(0, code);
		rc = handle_key(_maple_port, key);
	}

	return rc;
}

bool GamepadDevice::gamepad_axis_input(u32 code, int value)
{
	s32 v;
	if (input_mapper->get_axis_inverted(0, code))
		v = (get_axis_min_value(code) + get_axis_range(code) - value) * 255 / get_axis_range(code) - 128;
	else
		v = (value - get_axis_min_value(code)) * 255 / get_axis_range(code) - 128; //-128 ... +127 range
	if (_input_detected != NULL && !_detecting_button
			&& os_GetSeconds() >= _detection_start_time && (v >= 64 || v <= -64))
	{
		_input_detected(code);
		_input_detected = NULL;
		return true;
	}
	if (!input_mapper || _maple_port < 0 || _maple_port > 4)
		return false;

	auto handle_axis = [&](u32 port, DreamcastKey key)
	{
		// Combinations
		if (key == EMU_AXIS_CMB_X_Y_A_B)
		{
			kcode[port] |= DC_BTN_X | DC_BTN_Y | DC_BTN_A | DC_BTN_B |
				(DC_BTN_X | DC_BTN_Y | DC_BTN_A | DC_BTN_B << 1);
			if (v <= -64)
			{
				kcode[port] |= ~((DC_BTN_X | DC_BTN_Y | DC_BTN_A | DC_BTN_B) << 1);
			}
			else if (v >= 64)
			{
				kcode[port] &= ~(DC_BTN_X | DC_BTN_Y | DC_BTN_A | DC_BTN_B);
			}
			return true;
		}

		if (key == EMU_AXIS_CMB_X_Y_LT)
		{
			kcode[port] |= DC_BTN_X | DC_BTN_Y | (DC_BTN_X | DC_BTN_Y << 1);
			if (v <= -64)
			{
				kcode[port] |= ~((DC_BTN_X | DC_BTN_Y) << 1);
				lt[port] = 0;
			}
			else if (v >= 64)
			{
				kcode[port] &= ~(DC_BTN_X | DC_BTN_Y);
				lt[port] = 255;
			}
			return true;
		}

		if (key == EMU_AXIS_CMB_A_B_RT)
		{
			kcode[port] |= DC_BTN_A | DC_BTN_B | (DC_BTN_A | DC_BTN_B << 1);
			if (v <= -64)
			{
				kcode[port] |= ~((DC_BTN_A | DC_BTN_B) << 1);
				rt[port] = 0;
			}
			else if (v >= 64)
			{
				kcode[port] &= ~(DC_BTN_A | DC_BTN_B);
				rt[port] = 255;
			}
			return true;
		}

		if (key == EMU_AXIS_CMB_X_A)
		{
			kcode[port] |= DC_BTN_X | DC_BTN_A | (DC_BTN_X | DC_BTN_A << 1);
			if (v <= -64)
			{
				kcode[port] |= ~((DC_BTN_X | DC_BTN_A) << 1);
			}
			else if (v >= 64)
			{
				kcode[port] &= ~(DC_BTN_X | DC_BTN_A);
			}
			return true;
		}

		if (key == EMU_AXIS_CMB_Y_B)
		{
			kcode[port] |= DC_BTN_Y | DC_BTN_B | (DC_BTN_Y | DC_BTN_B << 1);
			if (v <= -64)
			{
				kcode[port] |= ~((DC_BTN_Y | DC_BTN_B) << 1);
			}
			else if (v >= 64)
			{
				kcode[port] &= ~(DC_BTN_Y | DC_BTN_B);
			}
			return true;
		}

		if (key == EMU_AXIS_CMB_LT_RT)
		{
			if (v <= -64)
			{
				lt[port] = 0;
				rt[port] = 0;
			}
			else if (v >= 64)
			{
				lt[port] = 255;
				rt[port] = 255;
			}
			return true;
		}

		if (key == EMU_AXIS_CMB_1_2_3)
		{
			kcode[port] |= DC_BTN_A | DC_BTN_B | DC_BTN_X | (DC_BTN_A | DC_BTN_B | DC_BTN_X << 1);
			if (v <= -64)
			{
				kcode[port] |= ~((DC_BTN_A | DC_BTN_B | DC_BTN_X) << 1);
			}
			else if (v >= 64)
			{
				kcode[port] &= ~(DC_BTN_A | DC_BTN_B | DC_BTN_X);
			}
			return true;
		}

		if (key == EMU_AXIS_CMB_4_5)
		{
			kcode[port] |= DC_BTN_Y | DC_DPAD2_UP | (DC_BTN_Y | DC_DPAD2_UP << 1);
			if (v <= -64)
			{
				kcode[port] |= ~((DC_BTN_Y | DC_DPAD2_UP ) << 1);
			}
			else if (v >= 64)
			{
				kcode[port] &= ~(DC_BTN_Y | DC_DPAD2_UP);
			}
			return true;
		}

		if (key == EMU_AXIS_CMB_4_5_6)
		{
			kcode[port] |= DC_BTN_Y | DC_DPAD2_UP | DC_DPAD2_DOWN | (DC_BTN_Y | DC_DPAD2_UP | DC_DPAD2_DOWN << 1);
			if (v <= -64)
			{
				kcode[port] |= ~((DC_BTN_Y | DC_DPAD2_UP | DC_DPAD2_DOWN) << 1);
			}
			else if (v >= 64)
			{
				kcode[port] &= ~(DC_BTN_Y | DC_DPAD2_UP | DC_DPAD2_DOWN);
			}
			return true;
		}

		if (key == EMU_AXIS_CMB_1_4)
		{
			kcode[port] |= DC_BTN_A | DC_BTN_Y | (DC_BTN_A | DC_BTN_Y << 1);
			if (v <= -64)
			{
				kcode[port] |= ~((DC_BTN_A | DC_BTN_Y) << 1);
			}
			else if (v >= 64)
			{
				kcode[port] &= ~(DC_BTN_A | DC_BTN_Y);
			}
			return true;
		}

		if (key == EMU_AXIS_CMB_2_5)
		{
			kcode[port] |= DC_BTN_B | DC_DPAD2_UP | (DC_BTN_B | DC_DPAD2_UP << 1);
			if (v <= -64)
			{
				kcode[port] |= ~((DC_BTN_B | DC_DPAD2_UP) << 1);
			}
			else if (v >= 64)
			{
				kcode[port] &= ~(DC_BTN_B | DC_DPAD2_UP);
			}
			return true;
		}

		if (key == EMU_AXIS_CMB_3_4)
		{
			kcode[port] |= DC_BTN_X | DC_BTN_Y | (DC_BTN_X | DC_BTN_Y << 1);
			if (v <= -64)
			{
				kcode[port] |= ~((DC_BTN_X | DC_BTN_Y) << 1);
			}
			else if (v >= 64)
			{
				kcode[port] &= ~(DC_BTN_X | DC_BTN_Y);
			}
			return true;
		}



		if (key == EMU_AXIS_CMB_3_6)
		{
			kcode[port] |= DC_BTN_X | DC_DPAD2_DOWN | (DC_BTN_X | DC_DPAD2_DOWN << 1);
			if (v <= -64)
			{
				kcode[port] |= ~((DC_BTN_X | DC_DPAD2_DOWN) << 1);
			}
			else if (v >= 64)
			{
				kcode[port] &= ~(DC_BTN_X | DC_DPAD2_DOWN);
			}
			return true;
		}

		if (key == EMU_AXIS_CMB_1_2)
		{
			kcode[port] |= DC_BTN_A | DC_BTN_B | (DC_BTN_A | DC_BTN_B << 1);
			if (v <= -64)
			{
				kcode[port] |= ~((DC_BTN_A | DC_BTN_B) << 1);
			}
			else if (v >= 64)
			{
				kcode[port] &= ~(DC_BTN_A | DC_BTN_B);
			}
			return true;
		}

		if (key == EMU_AXIS_CMB_2_3)
		{
			kcode[port] |= DC_BTN_B | DC_BTN_X | (DC_BTN_B | DC_BTN_X << 1);
			if (v <= -64)
			{
				kcode[port] |= ~((DC_BTN_B | DC_BTN_X) << 1);
			}
			else if (v >= 64)
			{
				kcode[port] &= ~(DC_BTN_B | DC_BTN_X);
			}
			return true;
		}

		if (key == EMU_AXIS_BTN_JUMP_STATE)
		{
			if (v >= 64)
			{
				if (config::Training ||
					config::DojoEnable && config::EnableSessionQuickLoad ||
					!config::DojoEnable)
				{
					if (!loading_state)
					{
						loading_state = true;
						bool dojo_invoke = config::DojoEnable.get();
						invoke_jump_state(dojo_invoke);
						loading_state = false;
					}
				}
			}
			return true;
		}

		if (key == EMU_AXIS_BTN_QUICK_SAVE)
		{
			if (v >= 64)
			{
				if (!config::DojoEnable || config::Training)
				{
					dc_savestate();
				}
			}
			return true;
		}

		if (key == EMU_AXIS_BTN_PLAY)
		{
			if (v >= 64)
			{
				if (config::Training)
				{
					dojo.TogglePlayback(0);
				}
			}
			return true;
		}

		if (key == EMU_AXIS_BTN_RECORD)
		{
			if (v >= 64)
			{
				if (config::Training)
				{
					dojo.ToggleRecording(0);
				}
			}
			return true;
		}

		if (key == EMU_AXIS_BTN_PLAY_1)
		{
			if (v >= 64)
			{
				if (config::Training)
				{
					dojo.TogglePlayback(1);
				}
			}
			return true;
		}

		if (key == EMU_AXIS_BTN_RECORD_1)
		{
			if (v >= 64)
			{
				if (config::Training)
				{
					dojo.ToggleRecording(1);
				}
			}
			return true;
		}

		if (key == EMU_AXIS_BTN_PLAY_2)
		{
			if (v >= 64)
			{
				if (config::Training)
				{
					dojo.TogglePlayback(2);
				}
			}
			return true;
		}

		if (key == EMU_AXIS_BTN_RECORD_2)
		{
			if (v >= 64)
			{
				if (config::Training)
				{
					dojo.ToggleRecording(2);
				}
			}
			return true;
		}

		if (key == EMU_AXIS_BTN_SWITCH_PLAYER)
		{
			if (v >= 64)
			{
				if (config::Training)
				{
					dojo.SwitchPlayer();
				}
			}
			return true;
		}

		if ((int)key < 0x10000)
		{
			kcode[port] |= key | (key << 1);
			if (v <= -64)
				kcode[port] &= ~key;
			else if (v >= 64)
				kcode[port] &= ~(key << 1);

			// printf("Mapped to %d %d %d\n",mo,kcode[port]&mo,kcode[port]&(mo*2));
		}
		else if (((int)key >> 16) == 1)	// Triggers
		{
			//printf("T-AXIS %d Mapped to %d -> %d\n",key, value, v + 128);

			if (key == DC_AXIS_LT)
				lt[port] = (u8)(v + 128);
			else if (key == DC_AXIS_RT)
				rt[port] = (u8)(v + 128);
			else
				return false;
		}
		else if (((int)key >> 16) == 2) // Analog axes
		{
			//printf("AXIS %d Mapped to %d -> %d\n", key, value, v);
			s8 *this_axis;
			s8 *other_axis;
			switch (key)
			{
			case DC_AXIS_X:
				this_axis = &joyx[port];
				other_axis = &joyy[port];
				break;

			case DC_AXIS_Y:
				this_axis = &joyy[port];
				other_axis = &joyx[port];
				break;

			case DC_AXIS_X2:
				this_axis = &joyrx[port];
				other_axis = &joyry[port];
				break;

			case DC_AXIS_Y2:
				this_axis = &joyry[port];
				other_axis = &joyrx[port];
				break;

			default:
				return false;
			}
			// Radial dead zone
			// FIXME compute both axes at the same time
			if ((float)(v * v + *other_axis * *other_axis) < input_mapper->dead_zone * input_mapper->dead_zone * 128.f * 128.f)
			{
				*this_axis = 0;
				*other_axis = 0;
			}
			else
				*this_axis = (s8)v;
		}
		else if (((int)key >> 16) == 4) // Map triggers to digital buttons
		{
			if (v <= -64)
				kcode[port] |=  (key & ~0x40000); // button released
			else if (v >= 64)
				kcode[port] &= ~(key & ~0x40000); // button pressed
		}
		else
			return false;

		return true;
	};

	bool rc = false;
	if (_maple_port == 4)
	{
		for (u32 port = 0; port < 4; port++)
		{
			DreamcastKey key = input_mapper->get_axis_id(port, code);
			rc = handle_axis(port, key) || rc;
		}
	}
	else
	{
		DreamcastKey key = input_mapper->get_axis_id(0, code);
		rc = handle_axis(_maple_port, key);
	}

	return rc;
}

int GamepadDevice::get_axis_min_value(u32 axis) {
	auto it = axis_min_values.find(axis);
	if (it == axis_min_values.end()) {
		load_axis_min_max(axis);
		it = axis_min_values.find(axis);
		if (it == axis_min_values.end())
			return INT_MIN;
	}
	return it->second;
}

unsigned int GamepadDevice::get_axis_range(u32 axis) {
	auto it = axis_ranges.find(axis);
	if (it == axis_ranges.end()) {
		load_axis_min_max(axis);
		it = axis_ranges.find(axis);
		if (it == axis_ranges.end())
			return UINT_MAX;
	}
	return it->second;
}

std::string GamepadDevice::make_mapping_filename(bool instance)
{
	std::string mapping_file = api_name() + "_" + name();
	if (instance)
		mapping_file += "-" + _unique_id;
	std::replace(mapping_file.begin(), mapping_file.end(), '/', '-');
	std::replace(mapping_file.begin(), mapping_file.end(), '\\', '-');
	std::replace(mapping_file.begin(), mapping_file.end(), ':', '-');
	std::replace(mapping_file.begin(), mapping_file.end(), '?', '-');
	std::replace(mapping_file.begin(), mapping_file.end(), '*', '-');
	std::replace(mapping_file.begin(), mapping_file.end(), '|', '-');
	std::replace(mapping_file.begin(), mapping_file.end(), '"', '-');
	std::replace(mapping_file.begin(), mapping_file.end(), '<', '-');
	std::replace(mapping_file.begin(), mapping_file.end(), '>', '-');
	mapping_file += ".cfg";

	return mapping_file;
}

void GamepadDevice::verify_or_create_system_mappings()
{
	std::string dc_name = make_mapping_filename(false);
	std::string arcade_name = make_mapping_filename(false, 2);

	std::string dc_path = get_readonly_config_path(std::string("mappings/") + dc_name);
	std::string arcade_path = get_readonly_config_path(std::string("mappings/") + arcade_name);

	if (!file_exists(arcade_path))
	{
		save_mapping(2);
		input_mapper->ClearMappings();
	}
	if (!file_exists(dc_path))
	{
		save_mapping(0);
		input_mapper->ClearMappings();
	}

	find_mapping(DC_PLATFORM_DREAMCAST);
}

void GamepadDevice::load_system_mappings(int system)
{
	for (int i = 0; i < GetGamepadCount(); i++)
	{
		std::shared_ptr<GamepadDevice> gamepad = GetGamepad(i);
		gamepad->find_mapping(system);
	}
}

std::string GamepadDevice::make_mapping_filename(bool instance, int system)
{
	std::string mapping_file = api_name() + "_" + name();
	if (instance)
		mapping_file += "-" + _unique_id;
	if (system != 0)
		mapping_file += "_arcade";
	std::replace(mapping_file.begin(), mapping_file.end(), '/', '-');
	std::replace(mapping_file.begin(), mapping_file.end(), '\\', '-');
	std::replace(mapping_file.begin(), mapping_file.end(), ':', '-');
	std::replace(mapping_file.begin(), mapping_file.end(), '?', '-');
	std::replace(mapping_file.begin(), mapping_file.end(), '*', '-');
	std::replace(mapping_file.begin(), mapping_file.end(), '|', '-');
	std::replace(mapping_file.begin(), mapping_file.end(), '"', '-');
	std::replace(mapping_file.begin(), mapping_file.end(), '<', '-');
	std::replace(mapping_file.begin(), mapping_file.end(), '>', '-');
	mapping_file += ".cfg";

	return mapping_file;
}

bool GamepadDevice::find_mapping(int system)
{
	if (!_remappable)
		return true;
	std::string mapping_file;
	mapping_file = make_mapping_filename(false, system);

	// if legacy dc mapping name is used, rename to default
	std::string arcade_name = make_mapping_filename(false, 2);
	size_t postfix_loc = arcade_name.find("_arcade");
	std::string old_dc_name = arcade_name.substr(0, postfix_loc) + "_dc.cfg";

	std::string old_dc_path = get_readonly_config_path(std::string("mappings/") + old_dc_name);
	std::string dc_path = get_readonly_config_path(std::string("mappings/") + make_mapping_filename(false));

	if (file_exists(old_dc_path))
	{
		if (file_exists(dc_path))
			remove(dc_path.data());

		rename(old_dc_path.data(), dc_path.data());
	}

	std::shared_ptr<InputMapping> mapper = InputMapping::LoadMapping(mapping_file.c_str());

	if (!mapper)
		return false;
	input_mapper = mapper;
	return true;
}

bool GamepadDevice::find_mapping(const char *custom_mapping /* = nullptr */)
{
	if (!_remappable)
		return true;
	std::string mapping_file;
	if (custom_mapping != nullptr)
		mapping_file = custom_mapping;
	else
		mapping_file = make_mapping_filename(true);

	input_mapper = InputMapping::LoadMapping(mapping_file.c_str());
	if (!input_mapper && custom_mapping == nullptr)
	{
		mapping_file = make_mapping_filename(false);
		input_mapper = InputMapping::LoadMapping(mapping_file.c_str());
	}
	return !!input_mapper;
}

int GamepadDevice::GetGamepadCount()
{
	_gamepads_mutex.lock();
	int count = _gamepads.size();
	_gamepads_mutex.unlock();
	return count;
}

std::shared_ptr<GamepadDevice> GamepadDevice::GetGamepad(int index)
{
	_gamepads_mutex.lock();
	std::shared_ptr<GamepadDevice> dev;
	if (index >= 0 && index < (int)_gamepads.size())
		dev = _gamepads[index];
	else
		dev = NULL;
	_gamepads_mutex.unlock();
	return dev;
}

void GamepadDevice::save_mapping()
{
	if (!input_mapper)
		return;
	std::string filename = make_mapping_filename();
	InputMapping::SaveMapping(filename.c_str(), input_mapper);
}

void GamepadDevice::save_mapping(int system)
{
	if (!input_mapper)
		return;
	std::string filename = make_mapping_filename(false, system);
	input_mapper->set_dirty();
	InputMapping::SaveMapping(filename.c_str(), input_mapper);
}

void UpdateVibration(u32 port, float power, float inclination, u32 duration_ms)
{
	int i = GamepadDevice::GetGamepadCount() - 1;
	for ( ; i >= 0; i--)
	{
		std::shared_ptr<GamepadDevice> gamepad = GamepadDevice::GetGamepad(i);
		if (gamepad != NULL && gamepad->maple_port() == (int)port && gamepad->is_rumble_enabled())
			gamepad->rumble(power, inclination, duration_ms);
	}
}

void GamepadDevice::detect_btn_input(input_detected_cb button_pressed)
{
	_input_detected = button_pressed;
	_detecting_button = true;
	_detection_start_time = os_GetSeconds() + 0.2;
}

void GamepadDevice::detect_axis_input(input_detected_cb axis_moved)
{
	_input_detected = axis_moved;
	_detecting_button = false;
	_detection_start_time = os_GetSeconds() + 0.2;
}

#ifdef TEST_AUTOMATION
static FILE *get_record_input(bool write)
{
	if (write && !cfgLoadBool("record", "record_input", false))
		return NULL;
	if (!write && !cfgLoadBool("record", "replay_input", false))
		return NULL;
	std::string game_dir = settings.imgread.ImagePath;
	size_t slash = game_dir.find_last_of("/");
	size_t dot = game_dir.find_last_of(".");
	std::string input_file = "scripts/" + game_dir.substr(slash + 1, dot - slash) + "input";
	return nowide::fopen(input_file.c_str(), write ? "w" : "r");
}
#endif

void GamepadDevice::Register(const std::shared_ptr<GamepadDevice>& gamepad)
{
	int maple_port = cfgLoadInt("input",
			MAPLE_PORT_CFG_PREFIX + gamepad->unique_id(), 12345);
	if (maple_port != 12345)
		gamepad->set_maple_port(maple_port);
#ifdef TEST_AUTOMATION
	if (record_input == NULL)
	{
		record_input = get_record_input(true);
		if (record_input != NULL)
			setbuf(record_input, NULL);
	}
#endif
	_gamepads_mutex.lock();
	_gamepads.push_back(gamepad);
	_gamepads_mutex.unlock();
}

void GamepadDevice::Unregister(const std::shared_ptr<GamepadDevice>& gamepad)
{
	gamepad->save_mapping();
	_gamepads_mutex.lock();
	for (auto it = _gamepads.begin(); it != _gamepads.end(); it++)
		if (*it == gamepad) {
			_gamepads.erase(it);
			break;
		}
	_gamepads_mutex.unlock();
}

void GamepadDevice::SaveMaplePorts()
{
	for (int i = 0; i < GamepadDevice::GetGamepadCount(); i++)
	{
		std::shared_ptr<GamepadDevice> gamepad = GamepadDevice::GetGamepad(i);
		if (gamepad != NULL && !gamepad->unique_id().empty())
			cfgSaveInt("input", MAPLE_PORT_CFG_PREFIX + gamepad->unique_id(), gamepad->maple_port());
	}
}

void Mouse::setAbsPos(int x, int y, int width, int height) {
	SetMousePosition(x, y, width, height, maple_port());
}

void Mouse::setRelPos(int deltax, int deltay) {
	SetRelativeMousePosition(deltax, deltay, maple_port());
}

void Mouse::setWheel(int delta) {
	if (maple_port() >= 0 && maple_port() < ARRAY_SIZE(mo_wheel_delta))
		mo_wheel_delta[maple_port()] += delta;
}

void Mouse::setButton(Button button, bool pressed)
{
	if (maple_port() >= 0 && maple_port() < ARRAY_SIZE(mo_buttons))
	{
		if (pressed)
			mo_buttons[maple_port()] &= ~(1 << (int)button);
		else
			mo_buttons[maple_port()] |= 1 << (int)button;
	}
	if (gui_is_open() && !is_detecting_input())
		// Don't register mouse clicks as gamepad presses when gui is open
		// This makes the gamepad presses to be handled first and the mouse position to be ignored
		return;
	gamepad_btn_input(button, pressed);
}


void SystemMouse::setAbsPos(int x, int y, int width, int height) {
	gui_set_mouse_position(x, y);
	Mouse::setAbsPos(x, y, width, height);
}

void SystemMouse::setButton(Button button, bool pressed) {
	int uiBtn = (int)button - 1;
	if (uiBtn < 2)
		uiBtn ^= 1;
	gui_set_mouse_button(uiBtn, pressed);
	Mouse::setButton(button, pressed);
}

void SystemMouse::setWheel(int delta) {
	gui_set_mouse_wheel(delta * 35);
	Mouse::setWheel(delta);
}

#ifdef TEST_AUTOMATION
#include "cfg/option.h"
static bool replay_inited;
FILE *replay_file;
u64 next_event;
u32 next_port;
u32 next_kcode;
bool do_screenshot;

void replay_input()
{
	if (!replay_inited)
	{
		replay_file = get_record_input(false);
		replay_inited = true;
	}
	u64 now = sh4_sched_now64();
	if (config::UseReios)
	{
		// Account for the swirl time
		if (config::Broadcast == 0)
			now = std::max((int64_t)now - 2152626532L, 0L);
		else
			now = std::max((int64_t)now - 2191059108L, 0L);
	}
	if (replay_file == NULL)
	{
		if (next_event > 0 && now - next_event > SH4_MAIN_CLOCK * 5)
			die("Automation time-out after 5 s\n");
		return;
	}
	while (next_event <= now)
	{
		if (next_event > 0)
			kcode[next_port] = next_kcode;

		char action[32];
		if (fscanf(replay_file, "%ld %s %x %x\n", &next_event, action, &next_port, &next_kcode) != 4)
		{
			fclose(replay_file);
			replay_file = NULL;
			NOTICE_LOG(INPUT, "Input replay terminated");
			do_screenshot = true;
			break;
		}
	}
}
#endif
