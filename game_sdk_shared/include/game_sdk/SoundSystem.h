//
// Created by droc101 on 6/13/26.
//

#pragma once

#include <cstdint>
#include <libassets/asset/SoundAsset.h>
#include <miniaudio.h>

class SoundSystem
{
    public:
        class Sound
        {
                friend SoundSystem;

            public:
                struct Format
                {
                        ma_format format = ma_format_unknown;
                        uint32_t channels = 0;
                        uint32_t sampleRate = 0;
                };

                [[nodiscard]] bool IsLoaded() const;

                void Seek(float seconds);
                [[nodiscard]] float GetCursor() const;

                [[nodiscard]] bool IsPlaying() const;
                void Play();
                void Stop();

                [[nodiscard]] bool IsLooping() const;
                void SetLooping(bool looping);

                [[nodiscard]] Format GetFormat();
                [[nodiscard]] uint64_t GetLengthInFrames() const;
                [[nodiscard]] float GetLength() const;

            protected:
                bool loaded = false;
                ma_decoder decoder{};
                ma_sound sound{};
        };

        static SoundSystem &Get();

        bool Init();

        void Destroy();

        bool LoadSound(const SoundAsset &soundAsset, Sound &dest);

        void UnloadSound(Sound &sound);

        void SetVolume(float volume);

        float GetVolume();

    private:
        bool initialized = false;
        ma_engine engine{};
};
