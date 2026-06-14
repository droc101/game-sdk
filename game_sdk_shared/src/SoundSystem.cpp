//
// Created by droc101 on 6/13/26.
//

#include <cstdint>
#include <game_sdk/SoundSystem.h>
#include <libassets/util/Logger.h>
#include <libassets/asset/SoundAsset.h>

SoundSystem &SoundSystem::Get()
{
    static SoundSystem soundSystemSingleton{};

    return soundSystemSingleton;
}

bool SoundSystem::Init()
{
    if (initialized)
    {
        return true;
    }
    const ma_result res = ma_engine_init(nullptr, &engine);
    if (res != MA_SUCCESS)
    {
        Logger::Error("ma_engine_init() failed: {}", static_cast<int>(res));
        return false;
    }
    initialized = true;
    return true;
}

void SoundSystem::Destroy()
{
    if (!initialized)
    {
        return;
    }
    ma_engine_uninit(&engine);
    initialized = false;
}

void SoundSystem::UnloadSound(Sound &sound)
{
    if (!initialized)
    {
        return;
    }
    if (!sound.loaded)
    {
        return;
    }
    ma_sound_stop(&sound.sound);
    ma_sound_uninit(&sound.sound);
    ma_decoder_uninit(&sound.decoder);
    sound.loaded = false;
}

bool SoundSystem::LoadSound(const SoundAsset &soundAsset, Sound &dest)
{
    if (!initialized)
    {
        return false;
    }
    UnloadSound(dest);
    ma_result result = ma_decoder_init_memory(soundAsset.GetData().data(),
                                              soundAsset.GetDataSize(),
                                              nullptr,
                                              &dest.decoder);
    if (result != MA_SUCCESS)
    {
        return false;
    }
    result = ma_sound_init_from_data_source(&engine, &dest.decoder, MA_SOUND_FLAG_DECODE, nullptr, &dest.sound);
    if (result != MA_SUCCESS)
    {
        return false;
    }
    dest.loaded = true;
    return true;
}

float SoundSystem::GetVolume()
{
    if (!initialized)
    {
        return 0;
    }
    return ma_engine_get_volume(&engine);
}

void SoundSystem::SetVolume(const float volume)
{
    if (!initialized)
    {
        return;
    }
    ma_engine_set_volume(&engine, volume);
}

float SoundSystem::Sound::GetCursor() const
{
    if (!loaded)
    {
        return 0;
    }
    float cursor = 0;
    ma_sound_get_cursor_in_seconds(&sound, &cursor);
    return cursor;
}

float SoundSystem::Sound::GetLength() const
{
    if (!loaded)
    {
        return 0;
    }
    float length = 0;
    ma_sound_get_length_in_seconds(&sound, &length);
    return length;
}

SoundSystem::Sound::Format SoundSystem::Sound::GetFormat()
{
    Format fmt{};
    if (!loaded)
    {
        return fmt;
    }
    ma_decoder_get_data_format(&decoder, &fmt.format, &fmt.channels, &fmt.sampleRate, nullptr, 0);
    return fmt;
}

uint64_t SoundSystem::Sound::GetLengthInFrames() const
{
    if (!loaded)
    {
        return 0;
    }
    ma_uint64 pcmFrames = 0;
    ma_sound_get_length_in_pcm_frames(&sound, &pcmFrames);
    return pcmFrames;
}

bool SoundSystem::Sound::IsLooping() const
{
    if (!loaded)
    {
        return false;
    }
    return ma_sound_is_looping(&sound) == 1;
}

bool SoundSystem::Sound::IsPlaying() const
{
    if (!loaded)
    {
        return false;
    }
    return ma_sound_is_playing(&sound) == 1;
}

void SoundSystem::Sound::Play()
{
    if (!loaded)
    {
        return;
    }
    ma_sound_start(&sound);
}

void SoundSystem::Sound::Stop()
{
    if (!loaded)
    {
        return;
    }
    ma_sound_stop(&sound);
}

void SoundSystem::Sound::Seek(const float seconds)
{
    if (!loaded)
    {
        return;
    }
    ma_sound_seek_to_second(&sound, seconds);
}

void SoundSystem::Sound::SetLooping(const bool looping)
{
    if (!loaded)
    {
        return;
    }
    ma_sound_set_looping(&sound, static_cast<ma_bool32>(looping));
}

bool SoundSystem::Sound::IsLoaded() const
{
    return loaded;
}
