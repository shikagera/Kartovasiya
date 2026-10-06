# Генератор таблицы реликвий: enum и данные из одного источника, чтобы
# порядок гарантированно совпадал. Поля задаются присваиваниями, а не
# назначенными инициализаторами: смешивать их с позиционными нельзя.
import io, sys

R = []
def r(e, rn, en, rd, ed, col, unlock=-1, special="None", **f):
    R.append((e, rn, en, rd, ed, col, unlock, special, f))

GOLD="0xF0DC8C"; BLUE="0x90E0EF"; AMBER="0xFFD60A"; FIRE="0xE85D04"
PURP="0x9D4EDD"; BONE="0xC9B78E"; BLOOD="0x9B2226"; GREEN="0x52B788"
STEEL="0x8D99AE"; ROSE="0xE56B6F"; TEAL="0x4EA8DE"; LIME="0x95D5B2"

# ── Исходные одиннадцать: имена, смысл и идентификаторы сохранены ─────────
r("ExtraSlot","Лишний палец","The Extra Finger",
  "+1 слот хода. Никто не считает, сколько их должно быть.",
  "+1 card slot each turn. Nobody counts how many there should be.", GOLD, slots=1)
r("DeepDraw","Бездонный карман","Bottomless Pocket",
  "+2 карты к добору каждый ход.","+2 cards drawn every turn.", BLUE, draw=2)
r("FirstFree","Право первой руки","Right of First Hand",
  "Первая карта за ход не тратит слот.","The first card each turn costs no slot.",
  AMBER, special="FirstFree")
r("LingeringMark","Несмываемое клеймо","Indelible Brand",
  "Метка стихии на враге больше не спадает со временем.",
  "The elemental mark no longer fades with time.", FIRE, special="LingeringMark")
r("ReactionEcho","Эхо в пустой зале","Echo in an Empty Hall",
  "Каждая реакция срабатывает дважды, по 60% урона.",
  "Every reaction fires twice, at 60% damage each.", PURP, special="ReactionEcho")
r("SpotHunter","Глаз анатома","The Anatomist's Eye",
  "По уязвимым точкам не бывает промаха.","Strikes at weak spots never miss.",
  BONE, special="SpotHunter")
r("BloodPact","Кровный уговор","The Blood Pact",
  "+30% урона. Каждая карта стоит 2 HP.","+30% damage. Every card costs 2 HP.",
  BLOOD, special="BloodPact")
r("Hoarder","Скупая ладонь","The Miser's Palm",
  "Одна карта из руки остаётся в колоде после боя.",
  "One card from your hand stays in the deck after the fight.", GOLD, special="Hoarder")
r("Undying","Восковой двойник","The Wax Double",
  "Раз за бой вы переживаете смертельный удар с 1 HP.",
  "Once per fight you survive a lethal blow at 1 HP.", BONE, special="Undying")
r("Gravedigger","Лопата могильщика","The Gravedigger's Spade",
  "Разрушенная точка даёт вдвое больше материалов.",
  "A destroyed weak spot yields twice the materials.", STEEL, special="Gravedigger")
r("Alembic","Перегонный куб","The Alembic",
  "Слияние сохраняет цепочку: составная карта уходит в колоду.",
  "Merging keeps the chain: the compound card goes to your deck.", TEAL, special="Alembic")

# ── Рука: слоты и добор ───────────────────────────────────────────────────
r("SecondFinger","Второй лишний палец","The Second Extra Finger",
  "+1 слот хода. Рука привыкает ко всему.","+1 card slot each turn. The hand adapts.", GOLD, 2, slots=1)
r("WideCuff","Широкий обшлаг","The Wide Cuff",
  "+1 карта к добору.","+1 card drawn each turn.", BLUE, draw=1)
r("LinedCoat","Подбитый фрак","The Lined Tailcoat",
  "+3 карты к добору, входящий урон выше на 10%.",
  "+3 cards drawn; you take 10% more damage.", BLUE, 1, draw=3, takenMul=1.10)
r("ThinDeck","Тонкая колода","The Thin Deck",
  "+2 слота хода, добор меньше на 2 карты.",
  "+2 card slots; you draw 2 cards fewer.", GOLD, 3, slots=2, draw=-2)
r("GlovedHand","Лайковая перчатка","The Kid Glove",
  "+1 слот хода и +1 карта к добору. Цена — 10% урона.",
  "+1 slot and +1 card drawn. The price is 10% of your damage.", GOLD, 2, slots=1, draw=1, dmgMul=0.90)
r("FreeMerge","Щипцы ювелира","The Jeweller's Pliers",
  "Первое слияние за ход не тратит слот.",
  "The first merge each turn costs no slot.", TEAL, 1, special="FreeMerge")
r("SilkLining","Шёлковая подкладка","The Silk Lining",
  "+2 карты к добору.","+2 cards drawn each turn.", BLUE, 2, draw=2)
r("CardCase","Визитница","The Card Case",
  "+1 слот хода, лечение слабее на четверть.",
  "+1 card slot; healing is a quarter weaker.", GOLD, 1, slots=1, healMul=0.75)
r("LooseThread","Распущенная нить","The Loose Thread",
  "+4 карты к добору, максимум здоровья меньше на 12.",
  "+4 cards drawn; 12 less maximum health.", BLUE, 3, draw=4, bonusHp=-12)
r("ThirdFinger","Третий лишний палец","The Third Extra Finger",
  "+1 слот хода. Перчатку давно шьют на заказ.",
  "+1 card slot. The glove is bespoke by now.", GOLD, 5, slots=1)

# ── Стихии ────────────────────────────────────────────────────────────────
r("EmberRing","Перстень углей","The Ember Ring",
  "Карты-стихии бьют на 20% сильнее.","Element cards hit 20% harder.", FIRE, elemMul=1.20)
r("SaltCellar","Солонка","The Salt Cellar",
  "Карты-стихии бьют на 35% сильнее, лечение слабее на треть.",
  "Element cards hit 35% harder; healing is a third weaker.", FIRE, 2, elemMul=1.35, healMul=0.66)
r("StormGlass","Штормовое стекло","The Storm Glass",
  "Метка держится на два хода дольше.","Marks last two turns longer.", TEAL, markTurns=2)
r("ColdHearth","Остывший очаг","The Cold Hearth",
  "Карты-стихии бьют на 15% сильнее, метка держится на ход дольше.",
  "Element cards hit 15% harder and marks last a turn longer.", FIRE, 1, elemMul=1.15, markTurns=1)
r("DoubleMark","Двуглавая печать","The Two-Headed Seal",
  "Первая стихия за бой ставит сразу две метки.",
  "The first element each fight sets two marks at once.", PURP, 4, special="DoubleMark")
r("PrismShard","Осколок призмы","The Prism Shard",
  "Карты-стихии бьют на 25% сильнее.","Element cards hit 25% harder.", TEAL, 2, elemMul=1.25)
r("WaxTablet","Восковая табличка","The Wax Tablet",
  "Метка держится на три хода дольше, ваш урон слабее на 10%.",
  "Marks last three turns longer; your damage is 10% weaker.", BONE, 3, markTurns=3, dmgMul=0.90)
r("BurntSpoon","Обожжённая ложка","The Burnt Spoon",
  "Карты-стихии бьют на 40% сильнее, заклинания — на 20% слабее.",
  "Element cards hit 40% harder; spells 20% weaker.", FIRE, 4, elemMul=1.40, spellMul=0.80)
r("TideMarker","Отметка прилива","The Tide Marker",
  "Карты-стихии бьют на 18% сильнее, щит в начале боя 8.",
  "Element cards hit 18% harder; 8 shield each fight.", TEAL, 1, elemMul=1.18, startShield=8)

# ── Реакции ───────────────────────────────────────────────────────────────
r("CatalystVial","Склянка катализатора","The Catalyst Vial",
  "Реакции бьют на 25% сильнее.","Reactions hit 25% harder.", PURP, reactionMul=1.25)
r("TwinFlask","Двойная колба","The Twin Flask",
  "Реакции бьют на 45% сильнее, обычный урон слабее на 10%.",
  "Reactions hit 45% harder; ordinary damage 10% weaker.", PURP, 2, reactionMul=1.45, dmgMul=0.90)
r("ReagentBelt","Пояс реагентов","The Reagent Belt",
  "Реакции бьют на 20% сильнее, метка держится на ход дольше.",
  "Reactions hit 20% harder; marks last a turn longer.", PURP, 1, reactionMul=1.20, markTurns=1)
r("AlchemistMask","Маска алхимика","The Alchemist's Mask",
  "Реакции бьют на 30% сильнее.","Reactions hit 30% harder.", GREEN, 3, reactionMul=1.30)
r("MortarPestle","Ступка и пестик","Mortar and Pestle",
  "Реакции бьют на 35% сильнее, добор меньше на карту.",
  "Reactions hit 35% harder; one card fewer drawn.", GREEN, 2, reactionMul=1.35, draw=-1)
r("QuenchBasin","Закалочный чан","The Quenching Basin",
  "Реакции бьют на 22% сильнее, входящий урон меньше на 10%.",
  "Reactions hit 22% harder; you take 10% less damage.", TEAL, 3, reactionMul=1.22, takenMul=0.90)
r("RetortGlass","Реторта","The Retort",
  "Реакции бьют вдвое сильнее, но здоровья меньше на 20.",
  "Reactions hit twice as hard; 20 less health.", PURP, 6, reactionMul=2.00, bonusHp=-20)

# ── Слияние: главный крюк игры ────────────────────────────────────────────
r("WeldingSeal","Сварная печать","The Welded Seal",
  "Слитые карты бьют на 25% сильнее.","Merged cards hit 25% harder.", GOLD, mergeMul=1.25)
r("GreatCrucible","Большой тигель","The Great Crucible",
  "Слитые карты бьют на 50% сильнее, обычные — на 15% слабее.",
  "Merged cards hit 50% harder; unmerged ones 15% weaker.", FIRE, 2, mergeMul=1.50, dmgMul=0.85)
r("ChainTongs","Цепные клещи","The Chain Tongs",
  "Слитые карты бьют на 30% сильнее, первое слияние за ход бесплатно.",
  "Merged cards hit 30% harder; the first merge each turn is free.", TEAL, 3,
  mergeMul=1.30, special="FreeMerge")
r("UltimaKey","Ключ от Ultima","The Ultima Key",
  "Слитые карты бьют на 70% сильнее. Входящий урон выше на четверть.",
  "Merged cards hit 70% harder. You take a quarter more damage.", AMBER, 5,
  mergeMul=1.70, takenMul=1.25)
r("GoldLeaf","Сусальное золото","The Gold Leaf",
  "Слитые карты бьют на 20% сильнее.","Merged cards hit 20% harder.", GOLD, 1, mergeMul=1.20)
r("SolderIron","Паяльник","The Soldering Iron",
  "Слитые карты бьют на 35% сильнее.","Merged cards hit 35% harder.", STEEL, 3, mergeMul=1.35)
r("FurnaceHeart","Сердце печи","The Furnace Heart",
  "Слитые карты бьют вдвое сильнее, лечение не работает.",
  "Merged cards hit twice as hard; healing does nothing.", FIRE, 7, mergeMul=2.00, healMul=0.0)
r("AmberBead","Янтарная бусина","The Amber Bead",
  "Слитые карты бьют на 22% сильнее, щит в начале боя 10.",
  "Merged cards hit 22% harder; 10 shield each fight.", AMBER, 2, mergeMul=1.22, startShield=10)
r("TwinCrucible","Парный тигель","The Paired Crucible",
  "Слитые карты бьют на 40% сильнее, реакции — на 15% слабее.",
  "Merged cards hit 40% harder; reactions 15% weaker.", GOLD, 4, mergeMul=1.40, reactionMul=0.85)

# ── Защита ────────────────────────────────────────────────────────────────
r("IronCorset","Железный корсет","The Iron Corset",
  "Входящий урон меньше на 15%.","You take 15% less damage.", STEEL, takenMul=0.85)
r("LeadLining","Свинцовая подкладка","The Lead Lining",
  "Входящий урон меньше на 25%, ваш урон — на 10%.",
  "You take 25% less damage; you deal 10% less.", STEEL, 1, takenMul=0.75, dmgMul=0.90)
r("StartShield","Нагрудник наготове","The Ready Breastplate",
  "Бой начинается с щитом 12.","Each fight begins with 12 shield.", TEAL, startShield=12)
r("ThornBrooch","Терновая брошь","The Thorn Brooch",
  "Атакующий получает 4 урона в ответ.","Attackers take 4 damage in return.", BLOOD, thorns=4)
r("BramblePin","Шипастая булавка","The Bramble Pin",
  "Атакующий получает 9 урона в ответ.","Attackers take 9 damage in return.", BLOOD, 3, thorns=9)
r("HeavyWatch","Тяжёлый хронометр","The Heavy Chronometer",
  "+20 к максимуму здоровья.","+20 maximum health.", BONE, bonusHp=20)
r("SignetOfYears","Перстень лет","The Signet of Years",
  "+35 к максимуму здоровья, добор меньше на карту.",
  "+35 maximum health; one card fewer drawn.", BONE, 2, bonusHp=35, draw=-1)
r("ColdOpen","Выстуженная зала","The Chilled Hall",
  "Враг пропускает свой первый ход.","The enemy skips its first turn.", BLUE, 4, special="ColdOpen")
r("StoneCollar","Каменный воротник","The Stone Collar",
  "Входящий урон меньше на 20%, добор меньше на карту.",
  "You take 20% less damage; one card fewer drawn.", STEEL, 2, takenMul=0.80, draw=-1)
r("MourningVeil","Траурная вуаль","The Mourning Veil",
  "Щит в начале боя 20, ваш урон слабее на 15%.",
  "20 shield each fight; your damage is 15% weaker.", BONE, 3, startShield=20, dmgMul=0.85)
r("IronThorns","Железные шипы","The Iron Thorns",
  "Атакующий получает 15 урона в ответ, здоровья меньше на 15.",
  "Attackers take 15 damage in return; 15 less health.", BLOOD, 5, thorns=15, bonusHp=-15)
r("DeadBolt","Мёртвый засов","The Dead Bolt",
  "Входящий урон меньше на треть, слотов хода на один меньше.",
  "You take a third less damage; one fewer card slot.", STEEL, 6, takenMul=0.66, slots=-1)
r("PaleLocket","Бледный медальон","The Pale Locket",
  "+30 к максимуму здоровья.","+30 maximum health.", BONE, 4, bonusHp=30)

# ── Лечение ───────────────────────────────────────────────────────────────
r("WarmDecoction","Горячий отвар","The Warm Decoction",
  "Всё лечение сильнее на 40%.","All healing is 40% stronger.", LIME, healMul=1.40)
r("SlowMend","Медленное срастание","The Slow Knitting",
  "В конце своего хода вы лечитесь на 3.","You heal 3 at the end of your turn.", LIME, regen=3)
r("DeepMend","Глубокое срастание","The Deep Knitting",
  "В конце своего хода вы лечитесь на 7, урон слабее на 10%.",
  "You heal 7 at end of turn; your damage is 10% weaker.", LIME, 2, regen=7, dmgMul=0.90)
r("LeechSeal","Пиявочная печать","The Leech Seal",
  "Лечение сильнее вдвое, максимум здоровья меньше на 10.",
  "Healing is doubled; maximum health is 10 lower.", GREEN, 3, healMul=2.00, bonusHp=-10)
r("GreenPoultice","Зелёная припарка","The Green Poultice",
  "В конце своего хода вы лечитесь на 5.","You heal 5 at the end of your turn.", LIME, 2, regen=5)
r("SurgeonKit","Набор хирурга","The Surgeon's Kit",
  "Лечение сильнее на 60%, щит в начале боя 8.",
  "Healing is 60% stronger; 8 shield each fight.", LIME, 4, healMul=1.60, startShield=8)

# ── Трофеи ────────────────────────────────────────────────────────────────
r("DeepPockets","Глубокие карманы","Deep Pockets",
  "С каждого боя падает больше материалов.","Every fight yields more materials.", GOLD, rewardRolls=2)
r("CollectorEye","Глаз собирателя","The Collector's Eye",
  "Заметно больше материалов, но здоровья меньше на 10.",
  "Far more materials, but 10 less health.", GOLD, 2, rewardRolls=4, bonusHp=-10)
r("LedgerClasp","Застёжка гроссбуха","The Ledger Clasp",
  "Больше материалов, лечение слабее на четверть.",
  "More materials; healing a quarter weaker.", GOLD, 1, rewardRolls=3, healMul=0.75)
r("MagpieNest","Сорочье гнездо","The Magpie's Nest",
  "Очень много материалов, но входящий урон выше на 20%.",
  "A great deal more materials; you take 20% more damage.", GOLD, 5, rewardRolls=6, takenMul=1.20)

# ── Чистая агрессия ───────────────────────────────────────────────────────
r("DuelistCane","Трость дуэлянта","The Duellist's Cane",
  "Весь урон выше на 15%.","All damage is 15% higher.", ROSE, dmgMul=1.15)
r("BlackPowder","Чёрный порох","The Black Powder",
  "Весь урон выше на 30%, входящий — тоже на 30%.",
  "All damage is 30% higher; so is damage taken.", FIRE, 1, dmgMul=1.30, takenMul=1.30)
r("LastStand","Последний рубеж","The Last Stand",
  "Ниже трети здоровья ваш урон удваивается.",
  "Below a third of your health your damage doubles.", BLOOD, 3, special="LastStand")
r("SpotSplit","Раскол","The Cleaving",
  "Урон по уязвимой точке задевает и корпус.",
  "Damage to a weak spot also bites the body.", STEEL, 2, special="SpotSplit")
r("GrimoireStub","Обрывок гримуара","The Grimoire Stub",
  "Карты-заклинания бьют на 30% сильнее.","Spell cards hit 30% harder.", PURP, spellMul=1.30)
r("InkedFingers","Пальцы в чернилах","Ink-Stained Fingers",
  "Карты-заклинания бьют на 50% сильнее, стихии — на 10% слабее.",
  "Spell cards hit 50% harder; elements 10% weaker.", PURP, 2, spellMul=1.50, elemMul=0.90)
r("DuelPistol","Дуэльный пистолет","The Duelling Pistol",
  "Весь урон выше на 22%.","All damage is 22% higher.", ROSE, 3, dmgMul=1.22)
r("RazorTie","Бритвенный шейный платок","The Razor Cravat",
  "Весь урон выше на 40%, максимум здоровья меньше на 20.",
  "All damage is 40% higher; 20 less maximum health.", BLOOD, 5, dmgMul=1.40, bonusHp=-20)
r("SilverNib","Серебряное перо","The Silver Nib",
  "Заклинания бьют на 35% сильнее, щит в начале боя 6.",
  "Spells hit 35% harder; 6 shield each fight.", PURP, 2, spellMul=1.35, startShield=6)
r("WidowKnife","Вдовий нож","The Widow's Knife",
  "Весь урон выше на 18%, лечение слабее вдвое.",
  "All damage is 18% higher; healing is halved.", BLOOD, 1, dmgMul=1.18, healMul=0.50)
r("BrokenMirror","Разбитое зеркало","The Broken Mirror",
  "Весь урон выше на 25%, входящий — на 15%.",
  "All damage is 25% higher; damage taken 15% higher.", PURP, 2, dmgMul=1.25, takenMul=1.15)
r("HangedSeal","Печать повешенного","The Hanged Man's Seal",
  "Весь урон выше на 55%, слотов хода на один меньше.",
  "All damage is 55% higher; one fewer card slot.", BLOOD, 7, dmgMul=1.55, slots=-1)

# ── Смешанные, поздние ────────────────────────────────────────────────────
r("NotaryStamp","Нотариальный штамп","The Notary's Stamp",
  "+1 слот хода, +1 карта к добору, здоровья меньше на 15.",
  "+1 slot, +1 card drawn; 15 less health.", GOLD, 4, slots=1, draw=1, bonusHp=-15)
r("GildedCage","Золочёная клетка","The Gilded Cage",
  "Щит в начале боя 25, сбежать нельзя — но это решает ступень.",
  "25 shield each fight; the cage is gilded all the same.", AMBER, 4, startShield=25)
r("SootGlove","Закопчённая перчатка","The Sooted Glove",
  "Стихии и слияния бьют на 20% сильнее.",
  "Elements and merges hit 20% harder.", FIRE, 3, elemMul=1.20, mergeMul=1.20)
r("OldDebt","Старый долг","The Old Debt",
  "+2 слота хода, входящий урон выше на 35%.",
  "+2 card slots; you take 35% more damage.", BLOOD, 6, slots=2, takenMul=1.35)
r("CandleStub","Огарок","The Candle Stub",
  "В конце хода лечение на 4, метка держится на ход дольше.",
  "Heal 4 at end of turn; marks last a turn longer.", LIME, 1, regen=4, markTurns=1)
r("GlassEye","Стеклянный глаз","The Glass Eye",
  "По точкам не бывает промаха, урон по ним задевает корпус.",
  "Weak-spot strikes never miss and bite the body too.", BONE, 6, special="SpotSplit")
r("DeathMask","Посмертная маска","The Death Mask",
  "Ниже трети здоровья урон удваивается, щит в начале боя 15.",
  "Below a third health your damage doubles; 15 shield each fight.", BONE, 8,
  special="LastStand", startShield=15)
r("TarotBack","Рубашка колоды","The Card Back",
  "+2 карты к добору и +12 к здоровью.",
  "+2 cards drawn and +12 health.", BLUE, 3, draw=2, bonusHp=12)
r("SilentBell","Беззвучный колокол","The Silent Bell",
  "Враг пропускает первый ход, ваш урон слабее на 10%.",
  "The enemy skips its first turn; your damage is 10% weaker.", BLUE, 7,
  special="ColdOpen", dmgMul=0.90)
r("CrackedLens","Треснувшая линза","The Cracked Lens",
  "Реакции и заклинания бьют на 20% сильнее.",
  "Reactions and spells hit 20% harder.", TEAL, 2, reactionMul=1.20, spellMul=1.20)
r("BoneDice","Костяные кости","The Bone Dice",
  "Весь урон выше на 20%, материалов больше, здоровья меньше на 10.",
  "All damage 20% higher, more materials, 10 less health.", BONE, 4,
  dmgMul=1.20, rewardRolls=2, bonusHp=-10)
r("VelvetRope","Бархатный шнур","The Velvet Rope",
  "Входящий урон меньше на 12%, лечение сильнее на 25%.",
  "You take 12% less damage; healing is 25% stronger.", ROSE, 1, takenMul=0.88, healMul=1.25)
r("LastWill","Завещание","The Last Will",
  "Раз за бой переживаете смертельный удар, затем урон удваивается.",
  "Survive one lethal blow per fight; your damage doubles below a third.", BONE, 9,
  special="Undying", dmgMul=1.10)
r("EtherLamp","Эфирная лампа","The Aether Lamp",
  "Метка держится на два хода дольше, реакции на 25% сильнее.",
  "Marks last two turns longer; reactions 25% stronger.", PURP, 5,
  markTurns=2, reactionMul=1.25)
r("SealedLetter","Запечатанное письмо","The Sealed Letter",
  "Первая карта за ход бесплатна, добор меньше на карту.",
  "The first card each turn is free; one card fewer drawn.", AMBER, 3,
  special="FirstFree", draw=-1)
r("HeirloomRing","Фамильный перстень","The Heirloom Ring",
  "+25 к здоровью, лечение сильнее на треть.",
  "+25 health; healing a third stronger.", BONE, 2, bonusHp=25, healMul=1.33)
r("ScorchedMap","Обугленная карта","The Scorched Map",
  "Стихии бьют на 30% сильнее, входящий урон выше на 15%.",
  "Elements hit 30% harder; you take 15% more damage.", FIRE, 3, elemMul=1.30, takenMul=1.15)
r("NightLedger","Ночной гроссбух","The Night Ledger",
  "Слияния на 25% сильнее, материалов больше.",
  "Merges 25% stronger; more materials.", GOLD, 4, mergeMul=1.25, rewardRolls=2)
r("CrowFeather","Воронье перо","The Crow's Feather",
  "+1 карта к добору, атакующий получает 5 урона в ответ.",
  "+1 card drawn; attackers take 5 damage in return.", BLOOD, 2, draw=1, thorns=5)
r("SaltRing","Соляной круг","The Circle of Salt",
  "Входящий урон меньше на 18%, метка держится дольше на ход.",
  "You take 18% less damage; marks last a turn longer.", TEAL, 3, takenMul=0.82, markTurns=1)
r("PorcelainHand","Фарфоровая ладонь","The Porcelain Hand",
  "+1 слот хода, входящий урон выше на 15%.",
  "+1 card slot; you take 15% more damage.", BONE, 1, slots=1, takenMul=1.15)
r("ObsidianStud","Обсидиановая запонка","The Obsidian Stud",
  "Слияния на 30% сильнее, лечение слабее на треть.",
  "Merges 30% stronger; healing a third weaker.", PURP, 3, mergeMul=1.30, healMul=0.66)

ORDER = ["dmgMul","elemMul","spellMul","reactionMul","mergeMul","takenMul","healMul",
         "startShield","bonusHp","slots","draw","regen","markTurns","rewardRolls","thorns"]

def esc(s): return s.replace('"','\\"')

seen = set()
for (e, *_rest) in R:
    if e in seen: raise SystemExit("дубликат идентификатора: " + e)
    seen.add(e)

hdr = ("// Сгенерировано genrelics.py — правьте генератор, а не этот блок.\n"
       "enum class RelicId : int {\n    None = 0,\n"
       + "".join("    %s,\n" % e for (e,*_ ) in R)
       + "    COUNT\n};\n")

rows = []
for (e, rn, en, rd, ed, col, unlock, special, f) in R:
    sets = []
    for k in ORDER:
        if k in f:
            v = f[k]
            sets.append("r.%s = %s;" % (k, ("%.2ff" % v) if isinstance(v, float) else str(v)))
    body = ""
    if sets:
        line = "      " + " ".join(sets) + "\n"
        if len(line) > 96:          # длинные строки разбиваем
            line = "".join("      %s\n" % x for x in sets)
        body = line
    rows.append(
        '    { RelicDef& r = add(RelicId::%s,\n'
        '        LS("%s", "%s"),\n'
        '        LS("%s",\n'
        '           "%s"),\n'
        '        hex(%s), %d, RelicSpecial::%s);\n%s      (void)r; }\n'
        % (e, esc(rn), esc(en), esc(rd), esc(ed), col, unlock, special, body))

print("RELICS:", len(R))
io.open(sys.argv[1], "w", encoding="utf-8").write(hdr)
io.open(sys.argv[2], "w", encoding="utf-8").write("".join(rows))
