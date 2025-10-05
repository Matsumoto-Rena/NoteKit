/*
 * NoteKit.c
 *
 *  Created on: Oct 3, 2025
 *      Author: m-ren
 */

#include "NoteKit.h"
#include "math.h"

float frequences[PITCH_COUNT][NUM_OCTAVES];
Buzzer buzzer[NUM_BUZZERS];
uint16_t BPM = 120;
DurationType duration_type = {2000, 1000, 500, 250, 125, 62.5};
extern TIM_HandleTypeDef htim6;

/*
 *
 */
//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
//{
//	if(htim == &htim6) {
//		NoteKit_TickHandler();
//	}
//}

/*
 *
 */
void NoteKit_TickHandler(void) {
    uint32_t current_time = HAL_GetTick();

    for (int i = 0; i < NUM_BUZZERS; i++) {
        // 演奏中で、かつ終了時刻を過ぎていたら
        if (buzzer[i].is_playing && ((int32_t)(current_time - buzzer[i].end_time_ms) >= 0)) {
            // 音を止める
            __HAL_TIM_SET_COMPARE(buzzer[i].tim_handle, buzzer[i].tim_channel, 0);
            // 演奏中フラグを倒す
            buzzer[i].is_playing = false;
        }
    }
}

/*
 *  音名とオクターブからMIDIノート番号を計算する関数
 */
int to_midi_index(PitchClass pc, int real_octave) {
    // C-1がMIDIノート番号0になるマッピング
    return (real_octave + 1) * 12 + pc;
}

/*
 *
 */
void Set_Frequences() {
    // 基準音の定義
    const double A4_FREQUENCY = 440.0;

    const int A4_MIDI_NOTE = to_midi_index(PITCH_A, 4); // 69になる

    const double SEMITONE_RATIO = pow(2.0, 1.0 / 12.0);

    for (int i = 0; i < NUM_OCTAVES; i++) {

        // `i` (0,1,2,3,4) を実際のオクターブ番号 (3,4,5,6,7) に変換
        int real_octave = i + 2;

        for (int pitch = 0; pitch < PITCH_COUNT; pitch++) {

            // to_midi_indexには変換後の「実際のオクターブ番号」を渡す
            int current_midi_note = to_midi_index((PitchClass)pitch, real_octave);

            int distance_from_a4 = current_midi_note - A4_MIDI_NOTE;

            double frequency = A4_FREQUENCY * pow(SEMITONE_RATIO, distance_from_a4);

            // 重要：配列のインデックスには、ループ変数 `i` (0..4) を使う
            frequences[pitch][i] = (float)frequency;
        }
    }
}

/*
 *
 */
void Set_Tempo(uint8_t bpm) {
	BPM = bpm;
	float quarter_duration = 60000.0f / BPM;
	duration_type.whole = quarter_duration * 4;
	duration_type.half = quarter_duration * 2;
	duration_type.quarter = quarter_duration;
	duration_type.eighth = quarter_duration / 2;
	duration_type.eighth_triplet = quarter_duration / 3;
	duration_type.sixteenth = quarter_duration / 4;
	duration_type.thirty_second = quarter_duration / 8;
}

/*
 *
 */
void Set_Buzzer(uint8_t buzzer_id, uint8_t tim_clock_MHz, TIM_HandleTypeDef* htim, uint32_t tim_channel) {

	HAL_TIM_PWM_Start(htim, tim_channel);
	buzzer[buzzer_id].tim_clock_MHz = tim_clock_MHz;
	buzzer[buzzer_id].tim_handle = htim;
	buzzer[buzzer_id].tim_channel = tim_channel;
}

/*
 *
 */
void PlayNote(uint8_t buzzer_id, uint32_t frequency_hz, float duration)
{
    // 周波数が0の場合は、無音（デューティー比0）にしてすぐに関数を抜ける
    if (frequency_hz == 0) {
        __HAL_TIM_SET_COMPARE(buzzer[buzzer_id].tim_handle, buzzer[buzzer_id].tim_channel, 0);
        return;
    }

    // タイマのクロック周波数 (Hz) を計算
    uint64_t timer_freq_hz = (uint64_t)buzzer[buzzer_id].tim_clock_MHz * 1000000;

    uint32_t prescaler = (buzzer[buzzer_id].tim_handle)->Init.Prescaler;

    // Counter Period (ARR)の値を計算 (すべて整数演算)
    uint64_t arr_calc = (timer_freq_hz / (prescaler + 1) / frequency_hz) - 1;

    // タイマのARRは16ビット(65535)が上限であることが多いので、チェックする
    if (arr_calc > 65535) {
        arr_calc = 65535; // 上限値に丸める
    }
    uint32_t arr_value = (uint32_t)arr_calc;
    __HAL_TIM_SET_AUTORELOAD(buzzer[buzzer_id].tim_handle, arr_value);

    // Pulse (CCR)の値を計算 (デューティー比50%)
    uint32_t pulse_value = arr_value / 2;
    __HAL_TIM_SET_COMPARE(buzzer[buzzer_id].tim_handle, buzzer[buzzer_id].tim_channel, pulse_value);

    buzzer[buzzer_id].is_playing = true;
    // 「現在の時刻 + 音の長さ」で終了時刻を計算して保存
    buzzer[buzzer_id].end_time_ms = HAL_GetTick() + duration;
}
