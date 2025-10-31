/*
 * NoteKit.c
 *
 *  Created on: Oct 3, 2025
 *      Author: m-ren
 */

#include "NoteKit.h"
#include "math.h"

float frequences[PITCH_COUNT][NUM_OCTAVES];
Buzzer buzzer[MAX_NUM_BUZZERS];
uint8_t num_buzzers;
uint16_t current_bpm = 120;
DurationType duration_type = {2000, 1000, 500, 250, 125, 62.5};
int score_index = 0;


/**
 * @brief  NoteKit シーケンサーのメイン更新関数（ティックハンドラ）。
 * @brief  再生中の全てのチャンネルを監視し、指定された演奏時間が終了した音を自動的に停止させます。
 * @note   この関数は、`SysTick_Handler` などのタイマ割り込みハンドラから、
 * @note   定期的に（通常は1msごと）呼び出される必要があります。
 * @param  None
 * @retval None
 */
void NoteKit_TickHandler(void) {
    uint32_t current_time = HAL_GetTick();

    for (int i = 0; i < num_buzzers; i++) {
        // 演奏中で、かつ終了時刻を過ぎていたら
        if (buzzer[i].is_playing && ((int32_t)(current_time - buzzer[i].end_time_ms) >= 0)) {
            // 音を止める
            __HAL_TIM_SET_COMPARE(buzzer[i].tim_handle, buzzer[i].tim_channel, 0);
            // 演奏中フラグを倒す
            buzzer[i].is_playing = false;
        }
    }
}

/**
 * @brief  指定したサウンドチャンネルが現在、音を再生中（自動停止待ち）かを確認します。
 * @note   主にシーケンサーが、次の音符へ進むタイミングを判断するために使用します。
 * @param  buzzer_id: 確認したいチャンネルのID (0 ~ NUM_BUZZERS-1)
 * @retval bool: true の場合、チャンネルは再生中で、自動停止を待っている状態です。
 * @retval bool: false の場合、チャンネルはアイドル状態（停止している）です。
 */
bool NoteKit_IsPlaying(uint8_t buzzer_id)
{
    if (buzzer_id >= num_buzzers) return false;
    return buzzer[buzzer_id].is_playing;
}

/**
 * @brief  PitchClass とオクターブ番号から、標準MIDIノート番号を計算します。
 * @note   C-1 を MIDIノート番号 0 とするマッピングを使用します。
 * @param  pc: PitchClass の enum 値 (例: PITCH_C, PITCH_CS)
 * @param  real_octave: 実際のオクターブ番号 (例: C4 を計算したい場合は 4)
 * @retval int: 計算されたMIDIノート番号 (例: C4 = 60, A4 = 69)
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

        // `i` (0,1,2,3,4...) を実際のオクターブ番号 (1,2,3,4,5...) に変換
        int real_octave = i + 1;

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
	current_bpm = bpm;
}

void NoteKit_Init() {
	Set_Frequences();
}

/*
 *
 */
void Set_Buzzer(uint8_t buzzer_id, uint8_t tim_clock_MHz, TIM_HandleTypeDef* htim, uint32_t tim_channel) {

	HAL_TIM_PWM_Start(htim, tim_channel);
	buzzer[buzzer_id].tim_clock_MHz = tim_clock_MHz;
	buzzer[buzzer_id].tim_handle = htim;
	buzzer[buzzer_id].tim_channel = tim_channel;
	num_buzzers++;

}

/**
 * @brief 指定したチャンネルで音を鳴らします（ノンブロッキング）。
 * @param channel_id   チャンネル番号 (0 ~ NUM_BUZZERS-1)
 * @param frequency_hz 周波数 (Hz)。0を指定すると音を停止します。
 * @param duration_ms  鳴らす時間 (ミリ秒)。0を指定すると手動で停止するまで鳴らし続けます。
 */
void NoteKit_NoteOn(uint8_t channel_id, uint32_t frequency_hz, uint32_t duration_ms)
{
    // 無効なチャンネルIDなら何もしない
    if (channel_id >= num_buzzers) {
        return;
    }

    if (frequency_hz == 0) {
        // CCRを0にしてPWM出力を無音化
        __HAL_TIM_SET_COMPARE(buzzer[channel_id].tim_handle, buzzer[channel_id].tim_channel, 0);
        // is_playing は true にして、休符の長さだけ待つようにする
        buzzer[channel_id].is_playing = true;
        buzzer[channel_id].end_time_ms = HAL_GetTick() + duration_ms;
        return; // 音を鳴らす処理は行わない
    }

    // --- タイマのレジスタ値を計算 ---

    // 1. タイマのクロック周波数 (Hz) を64ビットで計算
    uint64_t timer_freq_hz = (uint64_t)buzzer[channel_id].tim_clock_MHz * 1000000;

    // 2. 現在のプリスケーラ値を取得
    uint32_t prescaler = (buzzer[channel_id].tim_handle)->Init.Prescaler;

    // 3. ARR（周期）の値を64ビットで計算
    // (timer_freq_hz / (prescaler + 1)) が、カウンタが1秒間に数える数
    uint64_t arr_calc = (timer_freq_hz / (prescaler + 1) / frequency_hz) - 1;

    // 4. 計算結果がタイマの上限（多くは16ビット=65535）を超えないようにクリップ
    if (arr_calc > 65535) {
        arr_calc = 65535;
    }
    uint32_t arr_value = (uint32_t)arr_calc;

    // 5. CCR（デューティ比50%）の値を計算
    uint32_t pulse_value = arr_value / 2;

    // --- ハードウェアと状態を更新 ---

    // 6. 計算した値をタイマのレジスタに設定
    __HAL_TIM_SET_AUTORELOAD(buzzer[channel_id].tim_handle, arr_value);
    __HAL_TIM_SET_COMPARE(buzzer[channel_id].tim_handle, buzzer[channel_id].tim_channel, pulse_value);

    HAL_TIM_GenerateEvent(buzzer[channel_id].tim_handle, TIM_EVENTSOURCE_UPDATE);

//    // もし計算された音の長さが20ms未満だったら...
//    if (duration_ms > 0 && duration_ms < 20) {
//        // ...テストとして、強制的に20msに書き換える
//    	duration_ms = 20;
//    }

    // 7. 演奏状態を設定
    if (duration_ms > 0) {
        // is_playingフラグを立てる（TickHandlerが監視を開始する）
        buzzer[channel_id].is_playing = true;
        // 「現在の時刻 + 音の長さ」で終了時刻を計算して保存
        buzzer[channel_id].end_time_ms = HAL_GetTick() + duration_ms;
    } else {
        // duration_msが0の場合は無限再生
        // is_playingはfalseのままなので、TickHandlerは自動で止めない
        buzzer[channel_id].is_playing = false;
    }
}

/*
 *
 */
void Sequencer_Update(void) {
    // 全てのチャンネル（ブザー）をチェック
    for (int i = 0; i < num_buzzers; i++) {
        // このチャンネルが有効で、かつ演奏を終えている場合
        if (buzzer[i].part_is_active && !buzzer[i].is_playing) {

            // このチャンネルの楽譜の、次の音へ進む
            if (buzzer[i].score_index < buzzer[i].score_length) {
                // これから鳴らす音の情報を、このチャンネル専用の楽譜から取得
                const ScoreNote* next_note = &buzzer[i].assigned_score[buzzer[i].score_index];

                uint32_t frequency_to_play = 0; // デフォルトは0Hz (休符)

                // もし、楽譜の音名が休符でなければ、周波数を計算する
                if (next_note->pitch != PITCH_REST) {
                    frequency_to_play = (uint32_t)frequences[next_note->octave][next_note->pitch];
                }

                // 長さを計算する
                uint32_t duration_ms = get_duration_ms(next_note->duration_type);

                // PlayNoteには、休符なら0、そうでなければ計算した周波数が渡される
                NoteKit_NoteOn(i, frequency_to_play, duration_ms);

                // このチャンネルの再生位置を一つ進める
                buzzer[i].score_index++;
            }
            else if (buzzer[i].loop) {
                // ループ再生なら、再生位置を先頭に戻す
                buzzer[i].score_index = 0;
            }
            else {
                // ループしないなら、このパートを非アクティブにする
                buzzer[i].part_is_active = false;
            }
        }
    }
}

/**
 * @brief 指定したチャンネルに演奏するパート（楽譜）を設定します。
 */
void NoteKit_SetPart(uint8_t channel_id, const ScoreNote* score, int length, bool loop) {
    if (channel_id >= num_buzzers) return;

    buzzer[channel_id].assigned_score = score;
    buzzer[channel_id].score_length = length;
    buzzer[channel_id].score_index = 0; // 再生位置を先頭にリセット
    buzzer[channel_id].part_is_active = true;
    buzzer[channel_id].loop = loop;
}

/**
 * @brief 長さの種類(Duration enum)から、現在のBPMに基づいた具体的な時間(ms)を計算して返す
 * @param type 長さの種類 (例: DURATION_QUARTER)
 * @return 計算された音の長さ (ミリ秒)
 */
uint32_t get_duration_ms(Duration type) {
    // 基準となる4分音符の長さを計算 (浮動小数点数で精度を確保)
    float quarter_note_ms = 60000.0f / current_bpm;
    float duration_ms = 0.0f;

    switch (type) {
        // 基本音符
        case DURATION_WHOLE:         duration_ms = quarter_note_ms * 4.0f; break;
        case DURATION_HALF:          duration_ms = quarter_note_ms * 2.0f; break;
        case DURATION_QUARTER:       duration_ms = quarter_note_ms;        break;
        case DURATION_EIGHTH:        duration_ms = quarter_note_ms / 2.0f; break;
        case DURATION_SIXTEENTH:     duration_ms = quarter_note_ms / 4.0f; break;
        case DURATION_THIRTY_SECOND: duration_ms = quarter_note_ms / 8.0f; break;

        // 付点音符 (元の長さの1.5倍)
        case DURATION_DOTTED_HALF:    duration_ms = (quarter_note_ms * 2.0f) * 1.5f; break;
        case DURATION_DOTTED_QUARTER: duration_ms = quarter_note_ms * 1.5f;          break;
        case DURATION_DOTTED_EIGHTH:  duration_ms = (quarter_note_ms / 2.0f) * 1.5f; break;

        // 三連符
        case DURATION_QUARTER_TRIPLET: duration_ms = (quarter_note_ms * 2.0f) / 3.0f; break;
        case DURATION_EIGHTH_TRIPLET:  duration_ms = quarter_note_ms / 3.0f;          break;

        default: duration_ms = 0.0f; break; // 未知の型の場合は0を返す
    }

    uint32_t rounded_duration_ms = (uint32_t)duration_ms;

	if (rounded_duration_ms == 0 && duration_ms > 0) {
		rounded_duration_ms = MINIMUM_AUDIBLE_MS;
	}

	return rounded_duration_ms;
}

/**
 * @brief 音楽シーケンサーを1ステップ進めます。mainのwhile(1)から呼び出します。
 */
void NoteKit_SequencerUpdate(void) {
    // 全てのチャンネル（ブザー）をチェック
    for (int i = 0; i < num_buzzers; i++) {
        // このチャンネルが有効で、かつ演奏を終えている（次の音を鳴らす準備ができている）場合
        if (buzzer[i].part_is_active && !buzzer[i].is_playing) {

            // このチャンネルの楽譜の、次の音へ進む
            if (buzzer[i].score_index < buzzer[i].score_length) {
                // これから鳴らす音の情報を、このチャンネル専用の楽譜から取得
                const ScoreNote* next_note = &buzzer[i].assigned_score[buzzer[i].score_index];

                // pitchとoctaveから周波数(Hz)を周波数テーブルで引く
                uint32_t frequency_hz = (uint32_t)frequences[next_note->pitch][next_note->octave];

				//　duration_typeから長さ(ms)を計算する関数を呼び出す
				uint32_t duration_ms = get_duration_ms(next_note->duration_type);

				// 計算・変換した値を NoteKit_NoteOn に渡す
				NoteKit_NoteOn(i, frequency_hz, duration_ms);

                // このチャンネルの再生位置を一つ進める
                buzzer[i].score_index++;
            }
            else if (buzzer[i].loop) {
                // ループ再生なら、再生位置を先頭に戻す
                buzzer[i].score_index = 0;
            }
            else {
                // ループしないなら、このパートを非アクティブにする
                buzzer[i].part_is_active = false;
            }
        }
    }
}
