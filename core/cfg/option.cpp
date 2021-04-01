/*
	Copyright 2021 flyinghead

	This file is part of Flycast.

    Flycast is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Flycast is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Flycast.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "option.h"

namespace config {

// Dynarec

Option<bool> DynarecEnabled("Dynarec.Enabled", true);
Option<bool> DynarecIdleSkip("Dynarec.idleskip", true);
Option<bool> DynarecSafeMode("Dynarec.safe-mode");
Option<bool> DisableVmem32("Dynarec.DisableVmem32");

// General

Option<int> Cable("Dreamcast.Cable", 3);			// TV Composite
Option<int> Region("Dreamcast.Region", 1);			// USA
Option<int> Broadcast("Dreamcast.Broadcast", 0);	// NTSC
Option<int> Language("Dreamcast.Language", 1);		// English
Option<bool> FullMMU("Dreamcast.FullMMU");
Option<bool> ForceWindowsCE("Dreamcast.ForceWindowsCE");
Option<bool> AutoSavestate("Dreamcast.AutoSavestate");

// Sound

Option<bool> DSPEnabled("aica.DSPEnabled", false);
Option<bool> DisableSound("aica.NoSound");
#if HOST_CPU == CPU_ARM
Option<int> AudioBufferSize("aica.BufferSize", 5644);	// 128 ms
#else
Option<int> AudioBufferSize("aica.BufferSize", 2822);	// 64 ms
#endif
Option<bool> AutoLatency("aica.AutoLatency",
#ifdef __ANDROID__
		true
#else
		false
#endif
		);

OptionString AudioBackend("backend", "auto", "audio");

// Rendering

RendererOption RendererType;
Option<bool> UseMipmaps("rend.UseMipmaps", true);
Option<bool> Widescreen("rend.WideScreen");
Option<bool> ShowFPS("rend.ShowFPS");
Option<bool> RenderToTextureBuffer("rend.RenderToTextureBuffer");
Option<int> RenderToTextureUpscale("rend.RenderToTextureUpscale", 1);
Option<bool> TranslucentPolygonDepthMask("rend.TranslucentPolygonDepthMask");
Option<bool> ModifierVolumes("rend.ModifierVolumes", true);
Option<int> TextureUpscale("rend.TextureUpscale", 1);
Option<int> MaxFilteredTextureSize("rend.MaxFilteredTextureSize", 256);
Option<float> ExtraDepthScale("rend.ExtraDepthScale", 1.f);
Option<bool> CustomTextures("rend.CustomTextures");
Option<bool> DumpTextures("rend.DumpTextures");
Option<int> ScreenStretching("rend.ScreenStretching", 100);
Option<bool> Fog("rend.Fog", true);
Option<bool> FloatVMUs("rend.FloatVMUs");
Option<bool> Rotate90("rend.Rotate90");
Option<bool> PerStripSorting("rend.PerStripSorting");
Option<bool> DelayFrameSwapping("rend.DelayFrameSwapping");
Option<bool> WidescreenGameHacks("rend.WidescreenGameHacks");
std::array<Option<int>, 4> CrosshairColor {
	Option<int>("rend.CrossHairColor1"),
	Option<int>("rend.CrossHairColor2"),
	Option<int>("rend.CrossHairColor3"),
	Option<int>("rend.CrossHairColor4"),
};
Option<int> SkipFrame("ta.skip");
Option<int> MaxThreads("pvr.MaxThreads", 3);
Option<int> AutoSkipFrame("pvr.AutoSkipFrame", 0);
Option<int> RenderResolution("rend.Resolution", 480);

// Misc

Option<bool> SerialConsole("Debug.SerialConsoleEnabled");
Option<bool> SerialPTY("Debug.SerialPTY");
Option<bool> UseReios("UseReios");

Option<bool> OpenGlChecks("OpenGlChecks", false, "validate");

Option<std::vector<std::string>, false> ContentPath("Dreamcast.ContentPath");
Option<bool, false> HideLegacyNaomiRoms("Dreamcast.HideLegacyNaomiRoms", true);

// Network

Option<bool> NetworkEnable("Enable", false, "network");
Option<bool> ActAsServer("ActAsServer", false, "network");
OptionString DNS("DNS", "46.101.91.123", "network");
OptionString NetworkServer("server", "", "network");
Option<bool> EmulateBBA("EmulateBBA", false, "network");

// Dojo

Option<bool> DojoEnable("Enable", false, "dojo");
Option<bool> DojoActAsServer("ActAsServer", true, "dojo");
Option<bool> RecordMatches("RecordMatches", false, "dojo");
Option<bool> Spectating("Spectating", false, "dojo");
Option<bool> Transmitting("Transmitting", false, "dojo");
Option<bool> Receiving("Receiving", false, "dojo");
OptionString DojoServerIP("ServerIP", "127.0.0.1", "dojo");
OptionString DojoServerPort("ServerPort", "6000", "dojo");
Option<int> Delay("Delay", 1, "dojo");
Option<int> Debug("Debug", 8, "dojo");
OptionString ReplayFilename("ReplayFilename", "", "dojo");
Option<int> PacketsPerFrame("PacketsPerFrame", 2, "dojo");
Option<bool> EnableBackfill("EnableBackfill", true, "dojo");
Option<int> NumBackFrames("NumBackFrames", 3, "dojo");
Option<bool> EnableLobby("EnableLobby", false, "dojo");
OptionString PlayerName("PlayerName", "Player", "dojo");
OptionString OpponentName("OpponentName", "Opponent", "dojo");
Option<bool> TestGame("TestGame", false, "dojo");
OptionString SpectatorIP("SpectatorIP", "127.0.0.1", "dojo");
OptionString SpectatorPort("SpectatorPort", "7000", "dojo");
OptionString LobbyMulticastAddress("LobbyMulticastAddress", "26.255.255.255", "dojo");
OptionString LobbyMulticastPort("LobbyMulticastPort", "52001", "dojo");
Option<bool> EnableMatchCode("EnableMatchCode", false, "dojo");
OptionString MatchmakingServerAddress("MatchmakingServerAddress", "match.dojo.ooo", "dojo");
OptionString MatchmakingServerPort("MatchmakingServerPort", "52001", "dojo");
OptionString MatchCode("MatchCode", "", "dojo");
OptionString GameName("GameName", "", "dojo");
Option<bool> EnableMemRestore("EnableMemRestore", true, "dojo");
OptionString DojoProtoCall("DojoProtoCall", "", "dojo");
Option<bool> EnablePlayerNameOverlay("EnablePlayerNameOverlay", true, "dojo");
Option<bool> IgnoreNetSave("IgnoreNetSave", false, "dojo");
Option<bool> NetCustomVmu("NetCustomVmu", false, "dojo");
Option<bool> ShowPlaybackControls("ShowPlaybackControls", true, "dojo");
Option<int> RxFrameBuffer("RxFrameBuffer", 1800, "dojo");

#ifdef SUPPORT_DISPMANX
Option<bool> DispmanxMaintainAspect("maintain_aspect", true, "dispmanx");
#endif

#ifdef USE_OMX
Option<int> OmxAudioLatency("audio_latency", 100, "omx");
Option<bool> OmxAudioHdmi("audio_hdmi", true, "omx");
#endif

// Maple

Option<int> MouseSensitivity("MouseSensitivity", 100, "input");
Option<int> VirtualGamepadVibration("VirtualGamepadVibration", 20, "input");

std::array<Option<MapleDeviceType>, 4> MapleMainDevices {
	Option<MapleDeviceType>("device1", MDT_SegaController, "input"),
	Option<MapleDeviceType>("device2", MDT_None, "input"),
	Option<MapleDeviceType>("device3", MDT_None, "input"),
	Option<MapleDeviceType>("device4", MDT_None, "input"),
};
std::array<std::array<Option<MapleDeviceType>, 2>, 4> MapleExpansionDevices {
	Option<MapleDeviceType>("device1.1", MDT_SegaVMU, "input"),
	Option<MapleDeviceType>("device1.2", MDT_SegaVMU, "input"),

	Option<MapleDeviceType>("device2.1", MDT_None, "input"),
	Option<MapleDeviceType>("device2.2", MDT_None, "input"),

	Option<MapleDeviceType>("device3.1", MDT_None, "input"),
	Option<MapleDeviceType>("device3.2", MDT_None, "input"),

	Option<MapleDeviceType>("device4.1", MDT_None, "input"),
	Option<MapleDeviceType>("device4.2", MDT_None, "input"),
};

} // namespace config
