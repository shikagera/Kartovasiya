// Enemy roster: mythology, spirits, races, constructs, beasts, humans, bosses
// Каталог врагов по категориям + swarm-флаг
// Chaos Spellweaver (Картовасия)

using System;
using System.Collections.Generic;
using UnityEngine;
using Kartovasiya.Core;

namespace Kartovasiya.Combat
{
    public enum EnemyCategory
    {
        Myth = 0,
        Spirit = 1,
        Fantasy = 2,
        Construct = 3,
        Beast = 4,
        Human = 5,
        Exotic = 6,
        NamedBoss = 7
    }

    /// <summary>Art key for procedural portrait.</summary>
    public enum EnemyVisual
    {
        DragonFire, DragonIce, DragonShadow, DragonAncient,
        Griffin, Hydra, Cerberus, Minotaur, Harpy, Siren,
        Vampire, Werewolf, Ghoul, Ghost, Banshee, Poltergeist,
        ElementalFire, ElementalWater, ElementalEarth, ElementalAir,
        Fairy, Troll, Goblin, Ogre, Kraken, Leviathan,
        AncestorSpirit, DemonLesser, DemonLord, FallenAngel, NatureSpirit,
        Gnoll, Orc, Kobold, DarkElf, Lich, Skeleton, Zombie, SkeletonKnight,
        GolemStone, GolemIron, Chimera, Manticore, Basilisk, Beholder,
        Automaton, RuinGuardian, CrystalConstruct, SteamGolem, WarMachine,
        GiantWolf, GiantBear, GiantSpider, HellHound, BloodRaven, MutantBeast,
        DarkMage, Necromancer, Cultist, ArtifactHunter, FallenKnight, MadAlchemist,
        Dragonkin, Lamia, Gargoyle, Phoenix, Titan, ExileGod, VoidEntity,
        TimeParasite, MirrorTwin, Bandit,
        // Named bosses
        LordFlame, QueenIce, ShadowWeaver, ChaosRift,
        FallenAristocrat, AncientRuinGuard, DeadKing
    }

    [Serializable]
    public sealed class EnemyDef
    {
        public string Id;
        public string NameEn;
        public string NameRu;
        public EnemyCategory Category;
        public EnemyVisual Visual;
        public bool IsNamedBoss;
        /// <summary>Swarm encounter: several weaker units.</summary>
        public bool IsSwarm;
        public int SwarmMin = 2;
        public int SwarmMax = 4;
        /// <summary>Per-unit HP as fraction of normal solo HP (swarm only).</summary>
        public float SwarmHpFactor = 0.38f;
        /// <summary>Per-unit ATK as fraction of normal solo ATK.</summary>
        public float SwarmAtkFactor = 0.42f;
        public float HpMult = 1f;
        public float AtkMult = 1f;
        public ElementId Affinity = ElementId.None;
    }

    /// <summary>Static enemy pool for procedural waves.</summary>
    public static class EnemyCatalog
    {
        private static List<EnemyDef> _all;
        private static List<EnemyDef> _normal;
        private static List<EnemyDef> _bosses;

        public static IReadOnlyList<EnemyDef> All
        {
            get
            {
                Ensure();
                return _all;
            }
        }

        public static EnemyDef PickNormal(System.Random rng, int wave)
        {
            Ensure();
            // Slight weight toward higher tiers later
            int idx = rng.Next(0, _normal.Count);
            if (wave > 8 && rng.NextDouble() < 0.25)
                idx = rng.Next(_normal.Count / 2, _normal.Count);
            return _normal[idx];
        }

        public static EnemyDef PickBoss(System.Random rng, int wave)
        {
            Ensure();
            // Named bosses rotate; late game can reuse
            int idx = (wave / Mathf.Max(1, BalanceConfig.BossEveryN) + rng.Next(0, 3)) % _bosses.Count;
            if (idx < 0) idx = 0;
            return _bosses[idx];
        }

        public static EnemyDef Find(string id)
        {
            Ensure();
            for (int i = 0; i < _all.Count; i++)
                if (_all[i].Id == id)
                    return _all[i];
            return _normal[0];
        }

        private static void Ensure()
        {
            if (_all != null) return;
            _all = Build();
            _normal = new List<EnemyDef>();
            _bosses = new List<EnemyDef>();
            for (int i = 0; i < _all.Count; i++)
            {
                if (_all[i].IsNamedBoss) _bosses.Add(_all[i]);
                else _normal.Add(_all[i]);
            }
        }

        private static EnemyDef E(
            string id, string en, string ru, EnemyCategory cat, EnemyVisual vis,
            bool swarm = false, int sMin = 2, int sMax = 4,
            float hp = 1f, float atk = 1f, ElementId aff = ElementId.None,
            bool namedBoss = false)
        {
            return new EnemyDef
            {
                Id = id,
                NameEn = en,
                NameRu = ru,
                Category = cat,
                Visual = vis,
                IsSwarm = swarm,
                SwarmMin = sMin,
                SwarmMax = sMax,
                SwarmHpFactor = swarm ? 0.36f : 1f,
                SwarmAtkFactor = swarm ? 0.40f : 1f,
                HpMult = hp,
                AtkMult = atk,
                Affinity = aff,
                IsNamedBoss = namedBoss
            };
        }

        private static List<EnemyDef> Build()
        {
            var list = new List<EnemyDef>(80);

            // ── Myth ────────────────────────────────────────────────
            list.Add(E("dragon_fire", "Fire Dragon", "Огненный дракон", EnemyCategory.Myth, EnemyVisual.DragonFire, hp: 1.35f, atk: 1.25f, aff: ElementId.Fire));
            list.Add(E("dragon_ice", "Ice Dragon", "Ледяной дракон", EnemyCategory.Myth, EnemyVisual.DragonIce, hp: 1.35f, atk: 1.2f, aff: ElementId.Ice));
            list.Add(E("dragon_shadow", "Shadow Dragon", "Теневой дракон", EnemyCategory.Myth, EnemyVisual.DragonShadow, hp: 1.4f, atk: 1.25f, aff: ElementId.Shadow));
            list.Add(E("dragon_ancient", "Ancient Dragon", "Древний дракон", EnemyCategory.Myth, EnemyVisual.DragonAncient, hp: 1.55f, atk: 1.3f, aff: ElementId.Arcane));
            list.Add(E("griffin", "Griffin", "Грифон", EnemyCategory.Myth, EnemyVisual.Griffin, hp: 1.1f, atk: 1.15f, aff: ElementId.Air));
            list.Add(E("hydra", "Hydra", "Гидра", EnemyCategory.Myth, EnemyVisual.Hydra, hp: 1.4f, atk: 1.1f, aff: ElementId.Poison));
            list.Add(E("cerberus", "Cerberus", "Цербер", EnemyCategory.Myth, EnemyVisual.Cerberus, hp: 1.25f, atk: 1.2f, aff: ElementId.Fire));
            list.Add(E("minotaur", "Minotaur", "Минотавр", EnemyCategory.Myth, EnemyVisual.Minotaur, hp: 1.2f, atk: 1.2f, aff: ElementId.Earth));
            list.Add(E("harpies", "Harpies", "Гарпии", EnemyCategory.Myth, EnemyVisual.Harpy, swarm: true, sMin: 2, sMax: 4, aff: ElementId.Air));
            list.Add(E("sirens", "Sirens", "Сирены", EnemyCategory.Myth, EnemyVisual.Siren, swarm: true, sMin: 2, sMax: 3, aff: ElementId.Water));
            list.Add(E("vampire", "Vampire", "Вампир", EnemyCategory.Myth, EnemyVisual.Vampire, hp: 1.15f, atk: 1.15f, aff: ElementId.Blood));
            list.Add(E("werewolf", "Werewolf", "Вервольф", EnemyCategory.Myth, EnemyVisual.Werewolf, hp: 1.15f, atk: 1.2f, aff: ElementId.Nature));
            list.Add(E("ghouls", "Ghouls", "Гули", EnemyCategory.Myth, EnemyVisual.Ghoul, swarm: true, sMin: 2, sMax: 4, aff: ElementId.Poison));
            list.Add(E("ghosts", "Ghosts", "Призраки", EnemyCategory.Myth, EnemyVisual.Ghost, swarm: true, sMin: 2, sMax: 3, aff: ElementId.Aether));
            list.Add(E("poltergeists", "Poltergeists", "Полтергейсты", EnemyCategory.Spirit, EnemyVisual.Poltergeist, swarm: true, sMin: 2, sMax: 4, aff: ElementId.Aether));
            list.Add(E("banshee", "Banshee", "Банши", EnemyCategory.Myth, EnemyVisual.Banshee, hp: 1.05f, atk: 1.25f, aff: ElementId.Shadow));
            list.Add(E("elem_fire", "Fire Elemental", "Элементаль огня", EnemyCategory.Myth, EnemyVisual.ElementalFire, aff: ElementId.Fire));
            list.Add(E("elem_water", "Water Elemental", "Элементаль воды", EnemyCategory.Myth, EnemyVisual.ElementalWater, aff: ElementId.Water));
            list.Add(E("elem_earth", "Earth Elemental", "Элементаль земли", EnemyCategory.Myth, EnemyVisual.ElementalEarth, aff: ElementId.Earth));
            list.Add(E("elem_air", "Air Elemental", "Элементаль воздуха", EnemyCategory.Myth, EnemyVisual.ElementalAir, aff: ElementId.Air));
            list.Add(E("fairies", "Sidhe Fairies", "Феи / Сидхе", EnemyCategory.Myth, EnemyVisual.Fairy, swarm: true, sMin: 3, sMax: 5, aff: ElementId.Nature));
            list.Add(E("troll", "Troll", "Тролль", EnemyCategory.Myth, EnemyVisual.Troll, hp: 1.3f, atk: 1.05f, aff: ElementId.Earth));
            list.Add(E("goblins", "Goblins", "Гоблины", EnemyCategory.Fantasy, EnemyVisual.Goblin, swarm: true, sMin: 3, sMax: 5, aff: ElementId.Nature));
            list.Add(E("ogre", "Ogre", "Огр", EnemyCategory.Myth, EnemyVisual.Ogre, hp: 1.35f, atk: 1.15f, aff: ElementId.Earth));
            list.Add(E("kraken", "Kraken", "Кракен", EnemyCategory.Myth, EnemyVisual.Kraken, hp: 1.5f, atk: 1.2f, aff: ElementId.Water));
            list.Add(E("leviathan", "Leviathan", "Левиафан", EnemyCategory.Myth, EnemyVisual.Leviathan, hp: 1.55f, atk: 1.25f, aff: ElementId.Water));

            // ── Spirits ─────────────────────────────────────────────
            list.Add(E("ancestor", "Ancestor Spirit", "Дух предка", EnemyCategory.Spirit, EnemyVisual.AncestorSpirit, aff: ElementId.Light));
            list.Add(E("shadow_wraiths", "Shadow Wraiths", "Теневые призраки", EnemyCategory.Spirit, EnemyVisual.Ghost, swarm: true, sMin: 2, sMax: 4, aff: ElementId.Shadow));
            list.Add(E("demon_lesser", "Lesser Demons", "Низшие демоны", EnemyCategory.Spirit, EnemyVisual.DemonLesser, swarm: true, sMin: 2, sMax: 3, aff: ElementId.Fire));
            list.Add(E("demon_lord", "Demon Prince", "Князь демонов", EnemyCategory.Spirit, EnemyVisual.DemonLord, hp: 1.45f, atk: 1.3f, aff: ElementId.Fire));
            list.Add(E("fallen_angel", "Fallen Angel", "Падший ангел", EnemyCategory.Spirit, EnemyVisual.FallenAngel, hp: 1.3f, atk: 1.25f, aff: ElementId.Light));
            list.Add(E("nature_spirits", "Nature Spirits", "Духи природы", EnemyCategory.Spirit, EnemyVisual.NatureSpirit, swarm: true, sMin: 2, sMax: 4, aff: ElementId.Nature));
            list.Add(E("revenants", "Revenants", "Ревенанты", EnemyCategory.Spirit, EnemyVisual.Zombie, swarm: true, sMin: 2, sMax: 3, aff: ElementId.Blood));

            // ── Fantasy races ────────────────────────────────────────
            list.Add(E("gnolls", "Gnolls", "Гноллы", EnemyCategory.Fantasy, EnemyVisual.Gnoll, swarm: true, sMin: 2, sMax: 4));
            list.Add(E("orcs", "Orcs", "Орки", EnemyCategory.Fantasy, EnemyVisual.Orc, swarm: true, sMin: 2, sMax: 4));
            list.Add(E("kobolds", "Kobolds", "Кобольды", EnemyCategory.Fantasy, EnemyVisual.Kobold, swarm: true, sMin: 3, sMax: 5));
            list.Add(E("drow", "Dark Elf", "Тёмный эльф", EnemyCategory.Fantasy, EnemyVisual.DarkElf, hp: 1.05f, atk: 1.2f, aff: ElementId.Shadow));
            list.Add(E("lich", "Lich", "Лич", EnemyCategory.Fantasy, EnemyVisual.Lich, hp: 1.35f, atk: 1.25f, aff: ElementId.Arcane));
            list.Add(E("skeletons", "Skeletons", "Скелеты", EnemyCategory.Fantasy, EnemyVisual.Skeleton, swarm: true, sMin: 3, sMax: 5));
            list.Add(E("zombies", "Zombies", "Зомби", EnemyCategory.Fantasy, EnemyVisual.Zombie, swarm: true, sMin: 2, sMax: 4));
            list.Add(E("skel_knights", "Skeleton Knights", "Скелет-рыцари", EnemyCategory.Fantasy, EnemyVisual.SkeletonKnight, swarm: true, sMin: 2, sMax: 3, hp: 1.05f));
            list.Add(E("golem_stone", "Stone Golem", "Каменный голем", EnemyCategory.Construct, EnemyVisual.GolemStone, hp: 1.4f, atk: 1.0f, aff: ElementId.Earth));
            list.Add(E("golem_iron", "Iron Golem", "Железный голем", EnemyCategory.Construct, EnemyVisual.GolemIron, hp: 1.45f, atk: 1.1f, aff: ElementId.Metal));
            list.Add(E("chimera", "Chimera", "Химера", EnemyCategory.Fantasy, EnemyVisual.Chimera, hp: 1.3f, atk: 1.25f));
            list.Add(E("manticore", "Manticore", "Мантикора", EnemyCategory.Fantasy, EnemyVisual.Manticore, hp: 1.25f, atk: 1.2f, aff: ElementId.Poison));
            list.Add(E("basilisk", "Basilisk", "Василиск", EnemyCategory.Fantasy, EnemyVisual.Basilisk, hp: 1.2f, atk: 1.15f, aff: ElementId.Poison));
            list.Add(E("beholder", "Beholder", "Бехолдер", EnemyCategory.Fantasy, EnemyVisual.Beholder, hp: 1.25f, atk: 1.3f, aff: ElementId.Arcane));

            // ── Constructs ───────────────────────────────────────────
            list.Add(E("automatons", "Automatons", "Автоматоны", EnemyCategory.Construct, EnemyVisual.Automaton, swarm: true, sMin: 2, sMax: 4, aff: ElementId.Metal));
            list.Add(E("ruin_guard", "Ruin Guardian", "Страж руин", EnemyCategory.Construct, EnemyVisual.RuinGuardian, hp: 1.5f, atk: 1.15f, aff: ElementId.Earth));
            list.Add(E("crystal_constructs", "Crystal Constructs", "Кристаллические конструкты", EnemyCategory.Construct, EnemyVisual.CrystalConstruct, swarm: true, sMin: 2, sMax: 3, aff: ElementId.Crystal));
            list.Add(E("steam_golem", "Steam Golem", "Паровой голем", EnemyCategory.Construct, EnemyVisual.SteamGolem, hp: 1.35f, atk: 1.1f, aff: ElementId.Fire));
            list.Add(E("war_machine", "Ancient War Machine", "Древняя боевая машина", EnemyCategory.Construct, EnemyVisual.WarMachine, hp: 1.5f, atk: 1.2f, aff: ElementId.Metal));

            // ── Beasts ───────────────────────────────────────────────
            list.Add(E("giant_wolves", "Giant Wolves", "Гигантские волки", EnemyCategory.Beast, EnemyVisual.GiantWolf, swarm: true, sMin: 2, sMax: 4, aff: ElementId.Nature));
            list.Add(E("giant_bear", "Giant Bear", "Гигантский медведь", EnemyCategory.Beast, EnemyVisual.GiantBear, hp: 1.3f, atk: 1.15f, aff: ElementId.Nature));
            list.Add(E("giant_spiders", "Giant Spiders", "Гигантские пауки", EnemyCategory.Beast, EnemyVisual.GiantSpider, swarm: true, sMin: 2, sMax: 4, aff: ElementId.Poison));
            list.Add(E("hell_hounds", "Hell Hounds", "Адские гончие", EnemyCategory.Beast, EnemyVisual.HellHound, swarm: true, sMin: 2, sMax: 4, aff: ElementId.Fire));
            list.Add(E("blood_ravens", "Blood Ravens", "Кровавые вороны", EnemyCategory.Beast, EnemyVisual.BloodRaven, swarm: true, sMin: 3, sMax: 5, aff: ElementId.Blood));
            list.Add(E("mutant_beasts", "Mutant Beasts", "Мутанты", EnemyCategory.Beast, EnemyVisual.MutantBeast, swarm: true, sMin: 2, sMax: 3));

            // ── Humans / bandits ──────────────────────────────────────
            list.Add(E("dark_mage", "Dark Mage", "Тёмный маг", EnemyCategory.Human, EnemyVisual.DarkMage, hp: 0.95f, atk: 1.25f, aff: ElementId.Shadow));
            list.Add(E("necromancer", "Necromancer", "Некромант", EnemyCategory.Human, EnemyVisual.Necromancer, hp: 1.0f, atk: 1.2f, aff: ElementId.Arcane));
            list.Add(E("cultists", "Cultist Raiders", "Разбойники-культисты", EnemyCategory.Human, EnemyVisual.Cultist, swarm: true, sMin: 3, sMax: 5));
            list.Add(E("bandits", "Bandits", "Разбойники", EnemyCategory.Human, EnemyVisual.Bandit, swarm: true, sMin: 3, sMax: 5));
            list.Add(E("artifact_hunters", "Artifact Hunters", "Охотники за артефактами", EnemyCategory.Human, EnemyVisual.ArtifactHunter, swarm: true, sMin: 2, sMax: 3));
            list.Add(E("fallen_knight", "Fallen Knight", "Падший рыцарь", EnemyCategory.Human, EnemyVisual.FallenKnight, hp: 1.2f, atk: 1.2f));
            list.Add(E("mad_alchemist", "Mad Alchemist", "Алхимик-безумец", EnemyCategory.Human, EnemyVisual.MadAlchemist, hp: 0.95f, atk: 1.15f, aff: ElementId.Poison));

            // ── Exotic ───────────────────────────────────────────────
            list.Add(E("dragonkin", "Dragonkin", "Дракониды", EnemyCategory.Exotic, EnemyVisual.Dragonkin, swarm: true, sMin: 2, sMax: 3, aff: ElementId.Fire));
            list.Add(E("lamia", "Lamia", "Ламия", EnemyCategory.Exotic, EnemyVisual.Lamia, hp: 1.15f, atk: 1.15f, aff: ElementId.Shadow));
            list.Add(E("gargoyles", "Gargoyles", "Горгульи", EnemyCategory.Exotic, EnemyVisual.Gargoyle, swarm: true, sMin: 2, sMax: 3, aff: ElementId.Earth));
            list.Add(E("phoenix", "Hostile Phoenix", "Враждебный феникс", EnemyCategory.Exotic, EnemyVisual.Phoenix, hp: 1.2f, atk: 1.25f, aff: ElementId.Fire));
            list.Add(E("titan", "Ancient Titan", "Древний титан", EnemyCategory.Exotic, EnemyVisual.Titan, hp: 1.6f, atk: 1.2f, aff: ElementId.Earth));
            list.Add(E("exile_god", "Exile Godling", "Бог-изгой", EnemyCategory.Exotic, EnemyVisual.ExileGod, hp: 1.45f, atk: 1.3f, aff: ElementId.Light));
            list.Add(E("void_entity", "Void Entity", "Пустотная сущность", EnemyCategory.Exotic, EnemyVisual.VoidEntity, hp: 1.3f, atk: 1.25f, aff: ElementId.Aether));
            list.Add(E("time_parasites", "Time Parasites", "Временные паразиты", EnemyCategory.Exotic, EnemyVisual.TimeParasite, swarm: true, sMin: 3, sMax: 5, aff: ElementId.Arcane));
            list.Add(E("mirror_twins", "Mirror Twins", "Зеркальные двойники", EnemyCategory.Exotic, EnemyVisual.MirrorTwin, swarm: true, sMin: 2, sMax: 2, aff: ElementId.Light));

            // ── Named bosses ─────────────────────────────────────────
            list.Add(E("boss_flame", "Lord of Flame", "Лорд Пламени", EnemyCategory.NamedBoss, EnemyVisual.LordFlame, namedBoss: true, hp: 1.0f, atk: 1.0f, aff: ElementId.Fire));
            list.Add(E("boss_ice", "Queen of Ice", "Королева Льда", EnemyCategory.NamedBoss, EnemyVisual.QueenIce, namedBoss: true, aff: ElementId.Ice));
            list.Add(E("boss_shadow", "Shadow Weaver", "Ткач Теней", EnemyCategory.NamedBoss, EnemyVisual.ShadowWeaver, namedBoss: true, aff: ElementId.Shadow));
            list.Add(E("boss_chaos", "Chaos Rift", "Разлом Хаоса", EnemyCategory.NamedBoss, EnemyVisual.ChaosRift, namedBoss: true, aff: ElementId.Aether));
            list.Add(E("boss_aristocrat", "Fallen Aristocrat", "Падший Аристократ", EnemyCategory.NamedBoss, EnemyVisual.FallenAristocrat, namedBoss: true, aff: ElementId.Shadow));
            list.Add(E("boss_ruin", "Ancient Ruin Guard", "Древний Страж Руин", EnemyCategory.NamedBoss, EnemyVisual.AncientRuinGuard, namedBoss: true, aff: ElementId.Earth));
            list.Add(E("boss_deadking", "King of the Dead", "Король Мёртвых", EnemyCategory.NamedBoss, EnemyVisual.DeadKing, namedBoss: true, aff: ElementId.Blood));

            return list;
        }
    }
}
