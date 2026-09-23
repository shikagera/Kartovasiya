// Procedural audio / Процедурный звук — ни одного аудиофайла.
// Эффекты синтезируются в Wave при старте, музыка генерируется в рантайме
// прямо в колбэке аудиопотока (поэтому она не зацикливается).
#pragma once
#include "core/Types.h"

namespace shichoy {
namespace audio {

enum class Sfx : int {
    UiHover = 0,   // наведение на кнопку
    UiClick,       // нажатие
    UiBack,        // отмена / выход
    CardDraw,      // карта выезжает из колоды
    CardSpell,     // сыграна карта-заклинание
    CardEffect,    // сыграна карта-эффект
    Reaction,      // сработала реакция стихий
    Hit,           // попадание по врагу
    Miss,          // промах по уязвимой точке
    PlayerHurt,    // урон по игроку
    Shield,        // щит
    Heal,          // лечение
    SpotBreak,     // разрушена уязвимая точка
    PhaseShift,    // враг сменил фазу
    Victory,
    Defeat,
    TableScrape,   // стол выезжает из-под фрака
    TableThud,     // стол встал на землю
    Gather,        // собран материал
    Encounter,     // враг заметил и настиг
    CraftStart,
    CraftSuccess,
    CraftFail,
    CraftPerfect,
    Step,          // шаг в оверворлде
    COUNT
};

// Режимы генеративной музыки.
enum class Track : int { Off = 0, Menu, Overworld, Combat };

void init();       // поднимает аудиоустройство, синтезирует все эффекты
void shutdown();
bool ready();

void play(Sfx s, float volume = 1.0f, float pitch = 1.0f);
void playElement(ElementId e, float volume = 1.0f);   // звук стихии карты
// Голос приёма противника: тембр приёма плюс его стихия, взятая ниже.
// moveKind — индекс MoveKind (0..7), чтобы не тянуть боевой заголовок в звук.
void playEnemyMove(int moveKind, ElementId flavour, float volume = 1.0f);
void setTrack(Track t);
void update(float dt);                                 // плавные переходы громкости

void setMuted(bool m);
bool muted();
float masterVolume();
void setMasterVolume(float v);

// Отладка: синтезирует всё в .wav рядом и печатает уровни (без аудиоустройства).
void debugDump(const char* outDir);

} // namespace audio
} // namespace shichoy
