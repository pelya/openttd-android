/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file libtimidity.cpp Playing music via the timidity library. */

#include "../stdafx.h"
#include "../openttd.h"
#include "../sound_type.h"
#include "../debug.h"
#include "../core/math_func.hpp"
#include "libtimidity.h"
#include "midifile.hpp"
#include "../mixer.h"
#include <timidity.h>
#include <mutex>

#include "../safeguards.h"

/** The state of playing. */
enum MidiState {
	MIDI_STOPPED = 0,
	MIDI_PLAYING = 1,
};

static struct {
	MidIStream *stream;
	MidSongOptions options;
	MidSong *song;

	MidiState status;
	uint32 song_length;
	uint32 song_position;
	std::mutex synth_mutex;        ///< Guard mutex for synth access
} _midi; ///< Metadata about the midi we're playing.

static void MidiMixMusic(int16 *stream, size_t len)
{
	std::unique_lock<std::mutex> lock{ _midi.synth_mutex, std::try_to_lock };

	if (_midi.status == MIDI_PLAYING) {
		mid_song_read_wave(_midi.song, (sint8 *)stream, len*2);
	}
}

/** Factory for the libtimidity driver. */
static FMusicDriver_LibTimidity iFMusicDriver_LibTimidity;

enum { TIMIDITY_MAX_VOLUME = 50 };
const char *MusicDriver_LibTimidity::Start(const StringList &param)
{
	std::lock_guard<std::mutex> lock{ _midi.synth_mutex };

	_midi.status = MIDI_STOPPED;
	_midi.song = NULL;
	volume = TIMIDITY_MAX_VOLUME; // Avoid clipping
	uint32 samplerate = MxSetMusicSource(MidiMixMusic);

	if (mid_init(NULL) < 0) {
		/* If init fails, it can be because no configuration was found.
		 *  If it was not forced via param, try to load it without a
		 *  configuration. Who knows that works. */
		if (mid_init_no_config() < 0) {
			MxSetMusicSource(nullptr);
			return "error initializing timidity";
		}
	}
	DEBUG(driver, 1, "successfully initialised timidity");

	_midi.options.rate = samplerate;
	_midi.options.format = MID_AUDIO_S16LSB;
	_midi.options.channels = 2;
	_midi.options.buffer_size = _midi.options.rate;

	return NULL;
}

void MusicDriver_LibTimidity::Stop()
{
	MxSetMusicSource(nullptr);

	std::lock_guard<std::mutex> lock{ _midi.synth_mutex };

	if (_midi.status == MIDI_PLAYING) this->StopSong();
	mid_exit();
}

void MusicDriver_LibTimidity::PlaySong(const MusicSongInfo &song)
{
	std::string filename = MidiFile::GetSMFFile(song);

	this->StopSong();
	if (filename.empty()) return;

	std::lock_guard<std::mutex> lock{ _midi.synth_mutex };

	_midi.stream = mid_istream_open_file(filename.c_str());
	if (_midi.stream == NULL) {
		DEBUG(driver, 0, "Could not open music file");
		return;
	}

	_midi.song = mid_song_load(_midi.stream, &_midi.options);
	mid_istream_close(_midi.stream);
	_midi.song_length = mid_song_get_total_time(_midi.song);

	if (_midi.song == NULL) {
		DEBUG(driver, 1, "Invalid MIDI file");
		return;
	}

	mid_song_set_volume(_midi.song, volume);
	mid_song_start(_midi.song);
	_midi.status = MIDI_PLAYING;
}

void MusicDriver_LibTimidity::StopSong()
{
	std::lock_guard<std::mutex> lock{ _midi.synth_mutex };

	_midi.status = MIDI_STOPPED;
	/* mid_song_free cannot handle NULL! */
	if (_midi.song != NULL) mid_song_free(_midi.song);
	_midi.song = NULL;
}

bool MusicDriver_LibTimidity::IsSongPlaying()
{
	std::lock_guard<std::mutex> lock{ _midi.synth_mutex };

	if (_midi.status == MIDI_PLAYING) {
		_midi.song_position = mid_song_get_time(_midi.song);
		if (_midi.song_position >= _midi.song_length) {
			_midi.status = MIDI_STOPPED;
			_midi.song_position = 0;
		}
	}

	return (_midi.status == MIDI_PLAYING);
}

void MusicDriver_LibTimidity::SetVolume(byte vol)
{
	std::lock_guard<std::mutex> lock{ _midi.synth_mutex };

	volume = vol * TIMIDITY_MAX_VOLUME / 127; // I'm not sure about that value
	if (_midi.song != NULL) mid_song_set_volume(_midi.song, vol);
}
