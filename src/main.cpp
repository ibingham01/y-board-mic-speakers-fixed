#include "Arduino.h"
#include "coefficients.h"
#include "tone_gen.h"
#include "yboard.h"

std::string SONG_FILENAME = "/sd_card/ode_to_joy.wav";
std::string TONE_FILENAME = "/sd_card/audio_with_tone.wav";
std::string RECORDING_FILENAME = "/sd_card/recording.wav";
int tone_volume = 5;
int wav_volume = 5;

static Pipeline pipeline;
static File sound_file;
static WAVDecoder wav_decoder;
static EncodedAudioStream dec(&wav_decoder);
static FilteredStream<int16_t, float> filter_stream(dec, 1);
static VolumeStream volume;
static StreamCopy copier(pipeline, sound_file);

int16_t tremoloDuration = 200;
float tremoloDepth = 0.5;
const int sample_rate = 44100;

void play_wav_file(std::string filename);

void play_notes(std::string song);
void play_song();
void filter_tone();
void record();

void setup() {
    Serial.begin(115200);
    //AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Info);

    Yboard.setup();
    delay(100); // Display needs time to initialize
    Yboard.display.clearDisplay();
    Yboard.display.setTextColor(1,0);
    Yboard.display.setRotation(90); // Can be 0, 90, 180, or 270
    Yboard.display.setTextWrap(false);
    uint8_t text_size = 1;
    Yboard.display.setTextSize(text_size);
    Yboard.display.setCursor(0, 0);

    tone_gen_setup(tone_volume);
}

void loop() {
    // tone_gen_loop();
    // play_notes(mario);
    play_song();
    // filter_tone();
    // record();
}

// Functions for playing notes and wav files
bool sd_check(std::string filename) {
    if (!SD.exists(filename.c_str())) {
        Yboard.display.clearDisplay();
        Yboard.display.setCursor(0, 0);
        Yboard.display.write("Insert SD and Reset");
        Yboard.display.display();
        for (int i = 0; i < 5; i++) {
            Yboard.set_all_leds_color(255, 0, 0);
            delay(250);
            Yboard.set_all_leds_color(0, 0, 0);
            delay(250);
        }
        Yboard.display.clearDisplay();
        Yboard.display.display();
        return false;
    }
    return true;
}
void play_notes(std::string song) {
    Yboard.display.clearDisplay();
    Yboard.display.setCursor(0, 0);
    Yboard.display.write("Press BTN1 to play");
    Yboard.display.display();
    if (Yboard.get_button(1)) {
        Yboard.set_all_leds_color(0, 255, 0);
        Yboard.display.clearDisplay();
        Yboard.display.setCursor(0, 0);
        Yboard.display.write("Playing notes");
        Yboard.display.display();
        Yboard.play_notes(song);
        Yboard.display.clearDisplay();
        Yboard.set_all_leds_color(0, 0, 0);
    }
}

void play_song() {
    if (Yboard.get_button(1)) {
        if (!sd_check(SONG_FILENAME)) {
            return;
        }
        Yboard.set_sound_file_volume(wav_volume);
        Yboard.set_all_leds_color(0, 255, 0);

        Yboard.display.clearDisplay();
        Yboard.display.setCursor(0, 0);
        Yboard.display.write("Playing Song");
        Yboard.display.display();

        // Set up filter (none)
        volume.setVolume(0.5);
        filter_stream.setFilter(0, nullptr);

        play_wav_file(SONG_FILENAME);

        Yboard.display.clearDisplay();
        Yboard.display.display();    

        Yboard.set_all_leds_color(0, 0, 0);
    }
}

void filter_tone() {
    if (Yboard.get_button(1)) {
        if (!sd_check(TONE_FILENAME)) {
            return;
        }
        Yboard.set_sound_file_volume(wav_volume);
        Yboard.set_all_leds_color(0, 255, 0);

        Yboard.display.clearDisplay();
        Yboard.display.setCursor(0, 0);
        Yboard.display.write("Playing unfiltered file");
        Yboard.display.display();

        // Set up filter (none)
        volume.setVolume(0.5);
        filter_stream.setFilter(0, nullptr);

        play_wav_file(TONE_FILENAME);
        Yboard.display.clearDisplay();
        Yboard.display.display();
        Yboard.set_all_leds_color(0, 0, 0);
    }
    if (Yboard.get_button(2)) {
        if (!sd_check(TONE_FILENAME)) {
            return;
        }
        Yboard.set_sound_file_volume(wav_volume * 2);
        Yboard.set_all_leds_color(0, 0, 255);
        Yboard.display.clearDisplay();
        Yboard.display.setCursor(0, 0);
        Yboard.display.write("Playing filtered file");
        Yboard.display.display();

        // Set up filter (band reject)
        volume.setVolume(1.0);
        filter_stream.setFilter(0, new FIR<float>(coef));

        play_wav_file(TONE_FILENAME);
        Yboard.set_all_leds_color(0, 0, 0);
        Yboard.display.clearDisplay();
        Yboard.display.display();
    }
}

void record() {
    if (Yboard.get_button(1)) {
        bool started_recording = Yboard.start_recording(RECORDING_FILENAME);
        while (Yboard.get_button(1)) {
            if (started_recording) {
                Yboard.set_all_leds_color(255, 0, 0);

                Yboard.display.clearDisplay();
                Yboard.display.setCursor(0, 0);
                Yboard.display.write("Recording...");
                Yboard.display.display();
                delay(100);
            } else {
                Yboard.set_all_leds_color(255, 0, 0);
                delay(100);
                Yboard.set_all_leds_color(0, 0, 0);
                delay(100);
            }
        }

        if (started_recording) {
            delay(200); // Don't stop the recording immediately
            Yboard.stop_recording();
        }

        Yboard.display.clearDisplay();
        Yboard.display.display();
        Yboard.set_all_leds_color(0, 0, 0);
    }

    if (Yboard.get_button(2)) {
        if (!sd_check(RECORDING_FILENAME)) {
            return;
        }
        Yboard.set_sound_file_volume(wav_volume);
        Yboard.set_all_leds_color(0, 255, 0);

        if (Yboard.get_switch(1)) {
            Yboard.display.clearDisplay();
            Yboard.display.setCursor(0, 0);
            Yboard.display.write("Playing low pass filtered recording");
            Yboard.display.display();
            // Setup filter (low pass)
            volume.setVolume(1.0);
            filter_stream.setFilter(0, new FIR<float>(low_pass_coef));
        } else if (Yboard.get_switch(2)) {
            Yboard.display.clearDisplay();
            Yboard.display.setCursor(0, 0);
            Yboard.display.write("Playing band pass filtered recording");
            Yboard.display.display();
            // Setup filter (band pass)
            volume.setVolume(1.0);
            filter_stream.setFilter(0, new FIR<float>(band_pass_coef));
        } else {
            Yboard.display.clearDisplay();
            Yboard.display.setCursor(0, 0);
            Yboard.display.write("Playing unfiltered recording");
            Yboard.display.display();
            // Setup filter (none)
            volume.setVolume(0.5);
            filter_stream.setFilter(0, nullptr);
        }

        play_wav_file(RECORDING_FILENAME);
        Yboard.display.clearDisplay();
        Yboard.display.display();
        Yboard.set_all_leds_color(0, 0, 0);
    }
}

void play_wav_file(std::string filename) {
    sound_file = SD.open(filename.c_str());

    // setup pipeline
    pipeline.add(dec);
    pipeline.add(filter_stream);
    pipeline.add(volume);
    pipeline.setOutput(Yboard.get_speaker_stream());
    // start all components with their default values
    pipeline.begin();

    while (sound_file.available()) {
        copier.copy();
    }

    pipeline.end();
}
