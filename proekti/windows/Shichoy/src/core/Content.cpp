#include "core/Content.h"
#include <unordered_map>

namespace shichoy {

Lang g_lang = Lang::Ru;
Rng  g_rng(0xC0FFEEull);

// ── Statuses ──────────────────────────────────────────────────────────────
static const StatusDef kStatuses[] = {
    { {"—","—"},                    {"",""},                                                    GRAY,            false },
    { {"Ожог","Burn"},              {"Урон в конце хода","Damage at end of turn"},              hex(0xE85D04),   false },
    { {"Кровотечение","Bleed"},     {"Урон, растёт от ударов","Damage, grows with hits"},       hex(0x9B2226),   false },
    { {"Отравление","Poison"},      {"Урон и коррозия брони","Damage and armor corrosion"},     hex(0x52B788),   false },
    { {"Заморозка","Freeze"},       {"Пропуск действия","Skips its action"},                    hex(0x48CAE4),   false },
    { {"Оглушение","Stun"},         {"Пропуск действия","Skips its action"},                    hex(0xFFD60A),   false },
    { {"Разряд","Shock"},           {"Следующий урон усилен","Next damage amplified"},          hex(0x9D4EDD),   false },
    { {"Коррозия","Corrode"},       {"Снижает защиту","Lowers defense"},                        hex(0xADB5BD),   false },
    { {"Уязвимость","Vulnerable"},  {"Входящий урон +50%","Incoming damage +50%"},              hex(0xF72585),   false },
    { {"Ослабление","Weaken"},      {"Урон врага снижен","Enemy damage lowered"},               hex(0x6C757D),   false },
    { {"Замедление","Slow"},        {"Пропуск шага паттерна","Skips a pattern step"},           hex(0x6B705C),   false },
    { {"Регенерация","Regen"},      {"Лечение в конце хода","Heals at end of turn"},            hex(0x95D5B2),   true  },
    { {"Щит","Shield"},             {"Поглощает урон","Absorbs damage"},                        hex(0x90E0EF),   true  },
    { {"Сосредоточение","Focus"},   {"Точность и крит","Accuracy and crit"},                    hex(0xFFF3B0),   true  },
};
const StatusDef& statusDef(StatusId s) {
    int i = static_cast<int>(s);
    return kStatuses[(i < 0 || i >= static_cast<int>(StatusId::COUNT)) ? 0 : i];
}

// ── Elements ──────────────────────────────────────────────────────────────
static std::vector<ElementDef> makeElements() {
    using E = ElementId;
    using T = ElementTier;
    return {
        { E::None, T::Basic, {"—","—"}, {"",""}, GRAY, GRAY, E::None, "?", StatusId::None },
        { E::Fire,      T::Basic,    {"Огонь","Fire"},        {"Жар и разрушение. Поджог.","Raw heat. Burns over time."},                hex(0xE85D04), hex(0xFFBA08), E::Water,     "F", StatusId::Burn },
        { E::Water,     T::Basic,    {"Вода","Water"},        {"Поток, давление, промозглость.","Flow, pressure, cold damp."},           hex(0x0077B6), hex(0x90E0EF), E::Fire,      "W", StatusId::Slow },
        { E::Earth,     T::Basic,    {"Земля","Earth"},       {"Камень и сокрушительный вес.","Stone and crushing weight."},             hex(0x6B705C), hex(0xA5A58D), E::Air,       "E", StatusId::Weaken },
        { E::Air,       T::Basic,    {"Воздух","Air"},        {"Скорость и точные удары.","Speed and precise strikes."},                 hex(0x9CC7D6), hex(0xCAF0F8), E::Earth,     "A", StatusId::Vulnerable },
        { E::Lightning, T::Basic,    {"Молния","Lightning"},  {"Цепные разряды.","Chain arcs and burst."},                               hex(0x9D4EDD), hex(0xE0AAFF), E::Earth,     "Z", StatusId::Shock },
        { E::Ice,       T::Basic,    {"Лёд","Ice"},           {"Заморозка и хрупкость.","Freeze and brittleness."},                      hex(0x48CAE4), hex(0xCAF0F8), E::Fire,      "I", StatusId::Freeze },
        { E::Nature,    T::Basic,    {"Природа","Nature"},    {"Шипы и разрастание.","Thorns and overgrowth."},                          hex(0x2D6A4F), hex(0x95D5B2), E::Shadow,    "N", StatusId::Bleed },
        { E::Shadow,    T::Basic,    {"Тень","Shadow"},       {"Страх и вытягивание сил.","Fear and draining."},                         hex(0x5A189A), hex(0x240046), E::Light,     "S", StatusId::Weaken },
        { E::Metal,     T::Extended, {"Металл","Metal"},      {"Клинки и пробитие.","Blades and pierce."},                               hex(0xADB5BD), hex(0xDEE2E6), E::Lightning, "M", StatusId::Bleed },
        { E::Light,     T::Extended, {"Свет","Light"},        {"Раскрытие и очищение.","Reveal and cleanse."},                           hex(0xFFD60A), hex(0xFFF3B0), E::Shadow,    "L", StatusId::Vulnerable },
        { E::Poison,    T::Extended, {"Яд","Poison"},         {"Кислота и коррозия.","Acid and corrosion."},                             hex(0x52B788), hex(0xB7E4C7), E::Nature,    "P", StatusId::Poisoned },
        { E::Aether,    T::Extended, {"Эфир","Aether"},       {"Разломы пустоты.","Void rifts."},                                        hex(0x7209B7), hex(0xF72585), E::Arcane,    "X", StatusId::Corrode },
        { E::Blood,     T::Extended, {"Кровь","Blood"},       {"Обмен жизнью.","Life traded for power."},                                hex(0x9B2226), hex(0xAE2012), E::Light,     "B", StatusId::Bleed },
        { E::Storm,     T::Extended, {"Буря","Storm"},        {"Ветер и молния слиты воедино.","Wind and lightning fused."},             hex(0x4CC9F0), hex(0x4361EE), E::Earth,     "T", StatusId::Shock },
        { E::Crystal,   T::Extended, {"Кристалл","Crystal"},  {"Преломление и усиление.","Refraction and amplification."},               hex(0xF72585), hex(0xB5179E), E::Metal,     "C", StatusId::Vulnerable },
        { E::Arcane,    T::Extended, {"Аркана","Arcane"},     {"Нестабильная чистая магия.","Unstable pure magic."},                     hex(0xB5179E), hex(0xF72585), E::Aether,    "R", StatusId::Corrode },
    };
}
const std::vector<ElementDef>& allElements() { static std::vector<ElementDef> v = makeElements(); return v; }
const ElementDef& elem(ElementId id) {
    const auto& v = allElements();
    int i = eidx(id);
    return v[(i < 0 || i >= int(v.size())) ? 0 : i];
}
ElementId randomElement(Rng& r, int depth) {
    float extChance = clampf(0.10f + depth * 0.035f, 0.10f, 0.55f);
    if (r.chance(extChance)) return ElementId(r.i(9, 16));
    return ElementId(r.i(1, 8));
}

// ── Reactions ─────────────────────────────────────────────────────────────
static uint32_t pairKey(ElementId a, ElementId b) {
    int x = eidx(a), y = eidx(b);
    if (x > y) { int t = x; x = y; y = t; }
    return uint32_t(x) * 32u + uint32_t(y);
}

struct RSeed {
    ElementId a, b; const char* ru; const char* en; const char* dru; const char* den;
    ReactionKind kind; int bonus; float mult; float risk;
    StatusId st; int stacks; int turns; uint32_t tint;
};

static const RSeed kReactionSeeds[] = {
    { ElementId::Fire, ElementId::Water,   "Пар","Steam",                 "Обжигающая завеса","Scalding veil",            ReactionKind::Dot,     10, 1.35f, 0.f, StatusId::Burn,      3, 2, 0x8ECAE6 },
    { ElementId::Fire, ElementId::Earth,   "Горящий булыжник","Burning Boulder","Раскалённый удар","Molten impact",       ReactionKind::Damage,  14, 1.45f, 0.f, StatusId::Burn,      2, 2, 0xDC2F02 },
    { ElementId::Water, ElementId::Earth,  "Грязь","Mud",                 "Вязнет и давит","Bogged and crushed",          ReactionKind::Control,  8, 1.25f, 0.f, StatusId::Slow,      1, 2, 0x6B705C },
    { ElementId::Ice, ElementId::Water,    "Обморожение","Frostbite",     "Глубокий холод","Deep chill",                  ReactionKind::Control, 11, 1.35f, 0.f, StatusId::Freeze,    1, 1, 0x48CAE4 },
    { ElementId::Air, ElementId::Lightning,"Цепная молния","Chain Lightning","Разряд прыгает","Arc jumps",                ReactionKind::Damage,  12, 1.40f, 0.f, StatusId::Shock,     2, 2, 0x9D4EDD },
    { ElementId::Lightning, ElementId::Water,"Громовой хлопок","Thunderclap","Удар через влагу","Shock through moisture", ReactionKind::Damage,  13, 1.45f, 0.f, StatusId::Shock,     3, 2, 0x7209B7 },
    { ElementId::Lightning, ElementId::Fire,"Плазма","Plasma",            "Сверхгорячая дуга","Superheated arc",          ReactionKind::Damage,  16, 1.50f, 0.f, StatusId::Burn,      2, 2, 0xF72585 },
    { ElementId::Fire, ElementId::Ice,     "Термошок","Thermal Shock",    "Треск и раскол","Crack and shatter",           ReactionKind::Debuff,  13, 1.40f, 0.f, StatusId::Vulnerable,1, 2, 0xFF6B35 },
    { ElementId::Air, ElementId::Earth,    "Песчаная буря","Sandstorm",   "Ослепление и срез","Blind and chip",           ReactionKind::Debuff,   9, 1.30f, 0.f, StatusId::Weaken,    2, 2, 0xD4A373 },
    { ElementId::Fire, ElementId::Nature,  "Палы","Wildfire",             "Пламя расходится","Spreading flames",          ReactionKind::Dot,     12, 1.40f, 0.f, StatusId::Burn,      4, 3, 0xE85D04 },
    { ElementId::Shadow, ElementId::Light, "Затмение","Eclipse",          "Сумеречный разрез","Twilight cut",             ReactionKind::Damage,  18, 1.60f, 0.f, StatusId::Vulnerable,1, 2, 0x3C096C },
    { ElementId::Ice, ElementId::Air,      "Метель","Blizzard",           "Ледяной ветер","Freezing winds",               ReactionKind::Control, 11, 1.35f, 0.f, StatusId::Slow,      1, 2, 0x90E0EF },
    { ElementId::Earth, ElementId::Lightning,"Землетрясение","Quake",     "Сотрясение камня","Stone convulses",           ReactionKind::Damage,  12, 1.40f, 0.f, StatusId::Stun,      1, 1, 0x6C584C },
    { ElementId::Nature, ElementId::Water, "Трясина","Mire",              "Корни и сырость","Roots and damp",             ReactionKind::Control,  9, 1.30f, 0.f, StatusId::Slow,      1, 2, 0x2D6A4F },
    { ElementId::Fire, ElementId::Shadow,  "Тёмный огонь","Dark Fire",    "Пламя тьмы","Shadow-fed flame",                ReactionKind::Dot,     15, 1.50f, 0.f, StatusId::Burn,      5, 3, 0x5A189A },
    { ElementId::Lightning, ElementId::Shadow,"Тёмная молния","Dark Lightning","Разряд пустоты","Void-charged bolt",      ReactionKind::Damage,  15, 1.50f, 0.f, StatusId::Shock,     3, 2, 0x3C096C },
    { ElementId::Water, ElementId::Shadow, "Чёрный прилив","Black Tide",  "Тёмная давящая вода","Dark crushing water",    ReactionKind::Drain,   12, 1.40f, 0.f, StatusId::Weaken,    2, 2, 0x240046 },
    { ElementId::Lightning, ElementId::Storm,"Зов бури","Storm Call",     "Усиленная буря","Amplified tempest",           ReactionKind::Damage,  18, 1.55f, 0.f, StatusId::Shock,     4, 3, 0x4CC9F0 },
    { ElementId::Ice, ElementId::Earth,    "Ледяной осколок","Ice Shard", "Копья изо льда и камня","Frozen rock spears",  ReactionKind::Damage,  11, 1.35f, 0.f, StatusId::Bleed,     2, 2, 0xCAF0F8 },
    { ElementId::Poison, ElementId::Air,   "Ядовитое облако","Poison Cloud","Токсичная дымка","Toxic haze",               ReactionKind::Dot,     11, 1.35f, 0.f, StatusId::Poisoned,  3, 3, 0x52B788 },
    { ElementId::Fire, ElementId::Poison,  "Ядовитое пламя","Toxic Blaze","Ожог и токсин","Burn and toxin",               ReactionKind::Dot,     16, 1.50f, 0.f, StatusId::Poisoned,  4, 3, 0x40916C },
    { ElementId::Metal, ElementId::Lightning,"Металлическая искра","Metal Spark","Проводящий пробой","Conductive pierce", ReactionKind::Damage,  16, 1.50f, 0.f, StatusId::Corrode,   2, 2, 0xADB5BD },
    { ElementId::Crystal, ElementId::Lightning,"Кристальная дуга","Crystal Arc","Молния сквозь призму","Prism lightning", ReactionKind::Damage,  18, 1.55f, 0.f, StatusId::Vulnerable,2, 2, 0xB5179E },
    { ElementId::Blood, ElementId::Storm,  "Кровавая буря","Blood Storm", "Буря с вампиризмом","Lifesteal tempest",       ReactionKind::Drain,   18, 1.55f, 0.f, StatusId::Bleed,     3, 3, 0x9B2226 },
    { ElementId::Aether, ElementId::Shadow,"Разлом пустоты","Void Rift",  "Огромный урон, может ударить в ответ","Huge damage, may backlash", ReactionKind::Risky, 24, 1.75f, 0.28f, StatusId::Corrode, 3, 2, 0x3A0CA3 },
    { ElementId::Blood, ElementId::Arcane, "Душевный пакт","Soul Pact",   "Платишь кровью за силу","Pay blood for power", ReactionKind::Risky,   26, 1.80f, 0.32f, StatusId::Vulnerable,2, 2, 0xAE2012 },
    { ElementId::Ice, ElementId::Lightning,"Заряженный иней","Charged Frost","Ледяные иглы с током","Static ice needles", ReactionKind::Damage,  13, 1.40f, 0.f, StatusId::Shock,     2, 2, 0x48CAE4 },
    { ElementId::Light, ElementId::Nature, "Цветение","Bloom",            "Свет питает рост","Light feeds growth",        ReactionKind::Drain,   10, 1.30f, 0.f, StatusId::Regen,     3, 3, 0x95D5B2 },
    { ElementId::Light, ElementId::Water,  "Призма","Prism",              "Преломлённый залп","Refracted volley",         ReactionKind::Damage,  12, 1.40f, 0.f, StatusId::Vulnerable,1, 2, 0xFFF3B0 },
    { ElementId::Metal, ElementId::Fire,   "Расплавленный клинок","Molten Blade","Металл течёт по ране","Metal runs into the wound", ReactionKind::Dot, 15, 1.50f, 0.f, StatusId::Bleed, 4, 3, 0xE85D04 },
    { ElementId::Metal, ElementId::Earth,  "Оковы","Ironbound",           "Скован железом","Bound in iron",               ReactionKind::Control, 10, 1.30f, 0.f, StatusId::Slow,      2, 2, 0x6C757D },
    { ElementId::Crystal, ElementId::Light,"Преломление","Refraction",    "Свет режет гранями","Light cuts through facets",ReactionKind::Damage,  20, 1.60f, 0.f, StatusId::Vulnerable,2, 3, 0xF72585 },
    { ElementId::Arcane, ElementId::Aether,"Расплетение","Unmaking",      "Реальность рвётся","Reality tears",            ReactionKind::Risky,   28, 1.90f, 0.30f, StatusId::Corrode,  4, 3, 0x7209B7 },
    { ElementId::Blood, ElementId::Nature, "Кровавый цвет","Sanguine Bloom","Корни пьют кровь","Roots drink blood",       ReactionKind::Drain,   14, 1.45f, 0.f, StatusId::Bleed,     3, 3, 0x9B2226 },
    { ElementId::Poison, ElementId::Water, "Зараза","Contagion",          "Расходится по жилам","Spreads through veins",  ReactionKind::Dot,     12, 1.40f, 0.f, StatusId::Poisoned,  4, 4, 0x52B788 },
    { ElementId::Poison, ElementId::Nature,"Гниль","Blight",              "Плоть подводит","Flesh fails",                 ReactionKind::Debuff,  11, 1.35f, 0.f, StatusId::Weaken,    3, 3, 0x40916C },
    { ElementId::Storm, ElementId::Water,  "Ливень","Deluge",             "Стена воды","Wall of water",                   ReactionKind::Damage,  14, 1.45f, 0.f, StatusId::Slow,      1, 2, 0x4361EE },
    { ElementId::Storm, ElementId::Air,    "Циклон","Cyclone",            "Срывает с ног","Sweeps it off its feet",       ReactionKind::Control, 13, 1.45f, 0.f, StatusId::Stun,      1, 1, 0x4CC9F0 },
    { ElementId::Shadow, ElementId::Nature,"Тлен","Rot",                  "Тьма разъедает живое","Dark eats the living",  ReactionKind::Dot,     13, 1.40f, 0.f, StatusId::Poisoned,  3, 3, 0x240046 },
    { ElementId::Blood, ElementId::Shadow, "Обескровливание","Exsanguinate","Кровь уходит во тьму","Blood drawn into dark",ReactionKind::Drain,  16, 1.50f, 0.f, StatusId::Bleed,     4, 3, 0x6A040F },
    { ElementId::Crystal, ElementId::Earth,"Жеода","Geode",               "Камень прорастает гранями","Stone grows facets",ReactionKind::Damage,  12, 1.40f, 0.f, StatusId::Corrode,   2, 2, 0xB5179E },
    { ElementId::Aether, ElementId::Light, "Откровение","Revelation",     "Видно всё, что скрыто","All that hides is shown",ReactionKind::Debuff, 17, 1.55f, 0.f, StatusId::Vulnerable,3, 3, 0xF72585 },
    { ElementId::Arcane, ElementId::Fire,  "Адское пламя","Hellfire",     "Магия горит сама","Magic burns itself",        ReactionKind::Risky,   22, 1.70f, 0.22f, StatusId::Burn,     6, 3, 0xE85D04 },
    { ElementId::Metal, ElementId::Shadow, "Чёрное железо","Black Iron",  "Холодный тихий разрез","A cold, quiet cut",     ReactionKind::Damage,  15, 1.50f, 0.f, StatusId::Bleed,     3, 2, 0x343A40 },
    { ElementId::Ice, ElementId::Shadow,   "Чёрный иней","Black Frost",   "Холод без света","Cold without light",         ReactionKind::Control, 14, 1.45f, 0.f, StatusId::Freeze,    1, 2, 0x3C096C },
    { ElementId::Air, ElementId::Fire,     "Огненный вихрь","Firestorm",  "Раздутое пламя","Fanned flames",               ReactionKind::Dot,     13, 1.40f, 0.f, StatusId::Burn,      4, 2, 0xFFBA08 },
    { ElementId::Air, ElementId::Water,    "Шквал","Squall",              "Сбивающий шквал","Knocking squall",            ReactionKind::Control,  9, 1.30f, 0.f, StatusId::Weaken,    2, 2, 0x90E0EF },
    { ElementId::Earth, ElementId::Nature, "Прорастание","Overgrowth",    "Корни ломают камень","Roots split stone",      ReactionKind::Dot,     10, 1.35f, 0.f, StatusId::Bleed,     3, 3, 0x52B788 },
    { ElementId::Light, ElementId::Fire,   "Испепеление","Immolation",    "Свет добела","Light burned white",             ReactionKind::Damage,  17, 1.55f, 0.f, StatusId::Burn,      3, 2, 0xFFD60A },
    { ElementId::Metal, ElementId::Poison, "Ржавчина","Rust",             "Металл разъеден","Metal eaten away",           ReactionKind::Debuff,  12, 1.40f, 0.f, StatusId::Corrode,   3, 3, 0x8D6A4F },
};

static const std::unordered_map<uint32_t, const RSeed*>& reactionMap() {
    static std::unordered_map<uint32_t, const RSeed*> m = [] {
        std::unordered_map<uint32_t, const RSeed*> t;
        for (const auto& s : kReactionSeeds) t.emplace(pairKey(s.a, s.b), &s);
        return t;
    }();
    return m;
}

ReactionDef reactionFor(ElementId a, ElementId b) {
    ReactionDef d;
    if (a == ElementId::None || b == ElementId::None || a == b) return d;
    d.a = a; d.b = b; d.valid = true;

    const auto& m = reactionMap();
    auto it = m.find(pairKey(a, b));
    if (it != m.end()) {
        const RSeed* s = it->second;
        d.name = LS(s->ru, s->en);
        d.desc = LS(s->dru, s->den);
        d.kind = s->kind; d.bonus = s->bonus; d.mult = s->mult; d.risk = s->risk;
        d.applies = s->st; d.statusStacks = s->stacks; d.statusTurns = s->turns;
        d.tint = hex(s->tint);
        return d;
    }

    // Fallback / Запасной вариант: «Разлад» — слабее табличных реакций,
    // но каждая из 120 пар стихий всё равно что-то делает.
    const ElementDef& ea = elem(a);
    const ElementDef& eb = elem(b);
    bool opposed = (ea.opposed == b) || (eb.opposed == a);
    int  ext = (ea.tier == ElementTier::Extended ? 1 : 0) + (eb.tier == ElementTier::Extended ? 1 : 0);

    d.name = LS(std::string(opposed ? "Разлад: " : "Смесь: ") + ea.name.ru + " + " + eb.name.ru,
                std::string(opposed ? "Discord: " : "Blend: ") + ea.name.en + " + " + eb.name.en);
    d.desc = LS("Незнакомое сочетание — эффект нестабилен.",
                "An unfamiliar pairing — the effect is unstable.");
    d.kind    = opposed ? ReactionKind::Damage : ReactionKind::Debuff;
    d.bonus   = 6 + ext * 3 + (opposed ? 4 : 0);
    d.mult    = 1.15f + ext * 0.05f + (opposed ? 0.08f : 0.0f);
    d.risk    = 0.0f;
    d.applies = eb.mark;
    d.statusStacks = 1 + ext;
    d.statusTurns  = 2;
    d.tint    = lerpC(ea.primary, eb.primary, 0.5f);
    return d;
}

// ── Materials ─────────────────────────────────────────────────────────────
static const MaterialDef kMaterials[MATERIAL_COUNT] = {
    { {"Пепел","Ash"},            hex(0x7A6A62) },
    { {"Кость","Bone"},           hex(0xD8CBB3) },
    { {"Ихор","Ichor"},           hex(0x52B788) },
    { {"Серебро","Silver"},       hex(0xC7CBD1) },
    { {"Осколок","Shard"},        hex(0x9D4EDD) },
    { {"Пергамент","Vellum"},     hex(0xE0C89A) },
    { {"Печать","Sigil"},         hex(0xFFD60A) },
    { {"Сердцевина","Heartwood"}, hex(0x8A5A3B) },
};
const MaterialDef& material(int id) { return kMaterials[clampi(id, 0, MATERIAL_COUNT - 1)]; }

// ── Name pools ────────────────────────────────────────────────────────────
static std::vector<LS> mk(std::initializer_list<std::pair<const char*, const char*>> l) {
    std::vector<LS> v; v.reserve(l.size());
    for (auto& p : l) v.emplace_back(p.first, p.second);
    return v;
}

const std::vector<LS>& spellNouns() {
    static auto v = mk({
        {"Удар","Strike"}, {"Залп","Volley"}, {"Клеймо","Brand"}, {"Приговор","Verdict"},
        {"Литания","Litany"}, {"Разрез","Incision"}, {"Реквием","Requiem"}, {"Оковы","Fetters"},
        {"Печать","Seal"}, {"Взыскание","Levy"}, {"Панихида","Dirge"},
        {"Аркан","Arcanum"}, {"Пассаж","Passage"}, {"Дознание","Inquest"}, {"Долг","Debt"},
    });
    return v;
}
const std::vector<LS>& spellPrefixes() {
    static auto v = mk({
        {"Ветхий","Withered"}, {"Полуночный","Midnight"}, {"Господский","Manorial"},
        {"Погребальный","Funerary"}, {"Гербовый","Heraldic"}, {"Ртутный","Mercurial"},
        {"Восковой","Waxen"}, {"Траурный","Mourning"}, {"Позолоченный","Gilded"},
        {"Заёмный","Borrowed"}, {"Тихий","Silent"}, {"Отравленный","Tainted"},
        {"Витражный","Stained-Glass"}, {"Обугленный","Charred"},
    });
    return v;
}
const std::vector<LS>& effectNouns() {
    static auto v = mk({
        {"Этикет","Etiquette"}, {"Реверанс","Curtsy"}, {"Вексель","Promissory Note"},
        {"Завещание","Testament"}, {"Приглашение","Invitation"}, {"Родословная","Bloodline"},
        {"Титул","Title"}, {"Устав","Charter"}, {"Дуэль","Duel"}, {"Салонный трюк","Parlour Trick"},
        {"Молчание","Silence"}, {"Учтивость","Courtesy"},
    });
    return v;
}
const std::vector<LS>& enemyPrefixes() {
    static auto v = mk({
        {"Погребальный","Funerary"}, {"Заброшенный","Forsaken"}, {"Ртутный","Quicksilver"},
        {"Плакальный","Wailing"}, {"Витражный","Stained-Glass"}, {"Гербовый","Armorial"},
        {"Восковой","Waxen"}, {"Ветхий","Mouldering"}, {"Полуночный","Midnight"},
        {"Костяной","Ossuary"}, {"Кладбищенский","Graveyard"}, {"Салонный","Drawing-Room"},
    });
    return v;
}
const std::vector<LS>& enemyNouns() {
    static auto v = mk({
        {"Жнец","Reaper"}, {"Распорядитель","Steward"}, {"Плакальщик","Mourner"},
        {"Канделябр","Candelabrum"}, {"Гувернёр","Tutor"}, {"Душеприказчик","Executor"},
        {"Кредитор","Creditor"}, {"Портрет","Portrait"}, {"Органист","Organist"},
        {"Виночерпий","Cupbearer"}, {"Егерь","Huntsman"}, {"Нотариус","Notary"},
        {"Хормейстер","Choirmaster"}, {"Гость","Guest"},
    });
    return v;
}
const std::vector<LS>& affixNames() {
    static auto v = mk({
        {"жадный","greedy"}, {"хрупкий","brittle"}, {"двуликий","two-faced"},
        {"настойчивый","insistent"}, {"скорый","hasty"}, {"тяжёлый","heavy"},
        {"кровный","blooded"}, {"ветхий","worn"},
    });
    return v;
}

} // namespace shichoy
