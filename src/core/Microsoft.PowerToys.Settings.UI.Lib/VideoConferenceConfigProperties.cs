﻿// Copyright (c) Microsoft Corporation
// The Microsoft Corporation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System.Text.Json;
using System.Text.Json.Serialization;

namespace Microsoft.PowerToys.Settings.UI.Lib
{
    public class VideoConferenceConfigProperties
    {
        public VideoConferenceConfigProperties()
        {
            this.MuteCameraAndMicrophoneHotkey = new KeyboardKeysProperty(
                new HotkeySettings()
                {
                    Win = true,
                    Ctrl = false,
                    Alt = false,
                    Shift = false,
                    Key = "N",
                    Code = 78,
                });

            this.MuteMicrophoneHotkey = new KeyboardKeysProperty(
                new HotkeySettings()
                {
                    Win = true,
                    Ctrl = false,
                    Alt = false,
                    Shift = true,
                    Key = "A",
                    Code = 65,
                });

            this.MuteCameraHotkey = new KeyboardKeysProperty(
            new HotkeySettings()
            {
                Win = true,
                Ctrl = false,
                Alt = false,
                Shift = true,
                Key = "O",
                Code = 79,
            });

            Theme = new StringProperty("light");

            this.HideOverlayWhenUnmuted = new BoolProperty(true);
        }

        [JsonPropertyName("mute_camera_and_microphone_hotkey")]
        public KeyboardKeysProperty MuteCameraAndMicrophoneHotkey { get; set; }

        [JsonPropertyName("mute_microphone_hotkey")]
        public KeyboardKeysProperty MuteMicrophoneHotkey { get; set; }

        [JsonPropertyName("mute_camera_hotkey")]
        public KeyboardKeysProperty MuteCameraHotkey { get; set; }

        [JsonPropertyName("selected_camera")]
        public StringProperty SelectedCamera { get; set; } = string.Empty;

        [JsonPropertyName("overlay_position")]
        public StringProperty OverlayPosition { get; set; } = "Top right corner";

        [JsonPropertyName("overlay_monitor")]
        public StringProperty OverlayMonitor { get; set; } = "Main monitor";

        [JsonPropertyName("camera_overlay_image_path")]
        public StringProperty CameraOverlayImagePath { get; set; } = string.Empty;

        [JsonPropertyName("theme")]
        public StringProperty Theme { get; set; }

        [JsonPropertyName("hide_overlay_when_unmuted")]
        public BoolProperty HideOverlayWhenUnmuted { get; set; }

        // converts the current to a json string.
        public string ToJsonString()
        {
            return JsonSerializer.Serialize(this);
        }
    }
}
