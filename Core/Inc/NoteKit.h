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

#define MAX_NUM_BUZZERS 10
#define MINIMUM_AUDIBLE_MS 10

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
    PITCH_COUNT,  // 12
	PITCH_REST // 休符
} PitchClass;

typedef enum {
	OCTAVE_1 = 0,
	OCTAVE_2,
	OCTAVE_3,
	OCTAVE_4,
	OCTAVE_5,
	OCTAVE_6,
	OCTAVE_7,
	NUM_OCTAVES,
} OctaveClass;

typedef enum {
    // 基本音符
    DURATION_WHOLE,          // 全音符 (4拍)
    DURATION_HALF,           // 2分音符 (2拍)
    DURATION_QUARTER,        // 4分音符 (1拍)
    DURATION_EIGHTH,         // 8分音符 (0.5拍)
    DURATION_SIXTEENTH,      // 16分音符 (0.25拍)
    DURATION_THIRTY_SECOND,  // 32分音符 (0.125拍)

    // 付点音符 (元の長さの1.5倍)
    DURATION_DOTTED_HALF,    // 付点2分音符 (3拍)
    DURATION_DOTTED_QUARTER, // 付点4分音符 (1.5拍)
    DURATION_DOTTED_EIGHTH,  // 付点8分音符 (0.75拍)

    // 三連符
    DURATION_QUARTER_TRIPLET, // 4分音符の三連符 (約0.67拍)
    DURATION_EIGHTH_TRIPLET,  // 8分音符の三連符 (約0.33拍)

} Duration;

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

// 楽譜の1音を表す構造体
typedef struct {
    PitchClass pitch;
    OctaveClass octave;
    Duration duration_type;
} ScoreNote;

typedef struct {
    TIM_HandleTypeDef* tim_handle;
    uint32_t tim_channel;
    uint32_t tim_clock_MHz;
    volatile bool is_playing;
    volatile uint32_t end_time_ms;
    const ScoreNote* assigned_score; // 担当する楽譜へのポインタ
    int score_length;                // 楽譜の長さ
    int score_index;                 // 楽譜の現在位置 (0, 1, 2...)
    bool part_is_active;             // このパート（チャンネル）が有効か
    bool loop;                       // 楽譜をループ再生するか
}Buzzer;

extern int score_index;
extern float frequences[PITCH_COUNT][NUM_OCTAVES];

extern const ScoreNote part1_melody[];
extern const ScoreNote part2_bass[];
extern const ScoreNote part3_bass[];
extern const ScoreNote part4_bass[];

extern const int part1_len;
extern const int part2_len;
extern const int part3_len;
extern const int part4_len;

void NoteKit_Init();
void Set_Frequences();
void Set_Tempo(uint8_t bpm);
void Set_Buzzer(uint8_t buzzer_id, uint8_t tim_clock_MHz, TIM_HandleTypeDef* htim, uint32_t tim_channel);

void NoteKit_TickHandler();
bool NoteKit_IsPlaying(uint8_t buzzer_id);
void NoteKit_SetPart(uint8_t channel_id, const ScoreNote* score, int length, bool loop);
uint32_t get_duration_ms(Duration type);
void NoteKit_SequencerUpdate(void);
void Sequencer_Update(void);

#endif /* INC_NOTEKIT_H_ */
