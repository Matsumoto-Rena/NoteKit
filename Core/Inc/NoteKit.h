/*
 * NoteKit.h
 *
 *  Created on: Oct 3, 2025
 *      Author: m-ren
 */

#ifndef INC_NOTEKIT_H_
#define INC_NOTEKIT_H_

#include "main.h"
#include "stdbool.h"

#define NUM_BUZZERS 6

typedef enum {
    PITCH_C = 0,  // C
    PITCH_CS, // C#
    PITCH_D,  // D
    PITCH_DS, // D#
    PITCH_E,  // E
    PITCH_F,  // F
    PITCH_FS, // F#
    PITCH_G,  // G
    PITCH_GS, // G#
    PITCH_A,  // A
    PITCH_AS, // A#
    PITCH_B,  // B
    PITCH_COUNT  // 12
} PitchClass;

typedef enum {
	OCTAVE_2 = 0,
	OCTAVE_3,
	OCTAVE_4,
	OCTAVE_5,
	OCTAVE_6,
	OCTAVE_7,
	NUM_OCTAVES,
} OctaveClass;

extern uint16_t BPM;
typedef struct {
    float whole;          // 全音符・全休符の長さ
    float half;           // 2分音符・2分休符の長さ
    float quarter;        // 4分音符・4分休符の長さ
    float eighth;          // 8分音符・8分休符の長さ
    float eighth_triplet;  // 8分音符の三連符
    float sixteenth;      // 16分音符・16分休符の長さ
    float thirty_second;  // 32分音符・32分休符の長さ
} DurationType;
extern DurationType duration_type;

typedef struct {
	uint8_t tim_clock_MHz;
	TIM_HandleTypeDef* tim_handle;
	uint32_t tim_channel;
	uint32_t end_time_ms;
	bool is_playing;
}Buzzer;

typedef struct {
    uint32_t frequency_hz; // 音の周波数 (Hz)。0なら休符。
    uint32_t duration_ms;  // 音の長さ (ミリ秒)。
} ScoreNote;

extern float frequences[PITCH_COUNT][NUM_OCTAVES];

void NoteKit_TickHandler();
void Set_Frequences();
void Set_Tempo(uint8_t bpm);
void Set_Buzzer(uint8_t buzzer_id, uint8_t tim_clock_MHz, TIM_HandleTypeDef* htim, uint32_t tim_channel);
void PlayNote(uint8_t buzzer_id, uint32_t frequency_hz, float duration);

#endif /* INC_NOTEKIT_H_ */
