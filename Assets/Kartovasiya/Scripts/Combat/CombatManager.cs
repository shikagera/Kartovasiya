// Combat loop: turns, card limit, auto-drop, cast, enemy AI
// Боевой цикл: ходы, лимит карт, авто-дроп, каст, враг
// Chaos Spellweaver (Картовасия)

using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Kartovasiya.Cards;
using Kartovasiya.Combos;
using Kartovasiya.Core;
using Kartovasiya.Crafting;
using Kartovasiya.Generation;
using Kartovasiya.Hand;
using Kartovasiya.Save;

namespace Kartovasiya.Combat
{
    /// <summary>
    /// Owns the offline arcade combat loop (vertical StS-like).
    /// Управляет оффлайн-аркадным боем. No mana — only cards-per-turn.
    /// Player may Exit anytime (TurnPhase.Exited).
    /// </summary>
    public class CombatManager : MonoBehaviour
    {
        public static CombatManager Instance { get; private set; }

        [Header("Refs / Ссылки")]
        [SerializeField] private HandManager _hand;
        [SerializeField] private ComboDatabaseSO _comboDatabase;

        [Header("Player / Игрок")]
        [SerializeField] private int _playerMaxHp = GameConstants.PlayerStartHp;
        [SerializeField] private int _baseCardsPerTurn = GameConstants.BaseCardsPerTurn;

        [Header("Drop / Авто-дроп")]
        [SerializeField] private int _openingHandSize = GameConstants.DefaultHandSize;
        [SerializeField] private int _cardsDroppedPerTurn = GameConstants.CardsDroppedPerTurn;

        [Header("Timing / Тайминги")]
        [SerializeField] private float _enemyActionDelay = 0.55f;
        [SerializeField] private float _betweenEnemyDelay = 0.8f;

        // ── Runtime state ─────────────────────────────────────────────

        public TurnPhase Phase { get; private set; } = TurnPhase.None;
        public int TurnNumber { get; private set; }
        public int EnemyIndex { get; private set; }
        public int PlayerHp { get; private set; }
        public int PlayerMaxHp => _playerMaxHp;
        public int CardsRemaining { get; private set; }
        public int CardsPerTurnLimit { get; private set; }
        public CombatEnemy Enemy { get; private set; } = new CombatEnemy();
        public int Seed => _seed;
        public bool IsCombatActive =>
            Phase != TurnPhase.None &&
            Phase != TurnPhase.Victory &&
            Phase != TurnPhase.Defeat &&
            Phase != TurnPhase.Exited;

        public bool CanPlayerAct => Phase == TurnPhase.PlayerAction && CardsRemaining > 0 && !_busy;

        /// <summary>True when a run can be written to disk. / Можно ли сейвить run.</summary>
        public bool CanSaveRun =>
            PlayerHp > 0 &&
            TurnNumber > 0 &&
            Phase != TurnPhase.Defeat &&
            Phase != TurnPhase.None;

        private ProceduralCardGenerator _generator;
        private int _seed;
        private bool _busy;
        private bool _autoSaveEnabled = true;

        // Cached combo power from last fusion on a card (instance id → bonus)
        private readonly Dictionary<string, (int bonus, float mult)> _comboPower =
            new Dictionary<string, (int, float)>();

        // Buffs locked in by playing Effect cards on the player (persist for the run)
        private int _committedCardsPerTurnBonus;
        private float _committedCritBonus;
        private int _committedFloodBonus;

        public event Action<TurnPhase> PhaseChanged;
        public event Action StateChanged;
        public event Action<string> LogMessage;
        public event Action<int> PlayerHpChanged;
        public event Action CombatEnded; // victory wave or defeat — UI hooks
        public event Action RunExited;
        public event Action BossDefeated;
        public event Action GlobalEventOffered;
        public event Action<int, int> EnemyLootDropped; // essence, shard

        private MaterialInventory _materials;

        public MaterialInventory Materials => _materials;

        private void Awake()
        {
            if (Instance != null && Instance != this)
            {
                Destroy(gameObject);
                return;
            }

            Instance = this;
            if (_hand == null)
                _hand = HandManager.Instance;
        }

        private void OnDestroy()
        {
            if (Instance == this)
                Instance = null;
            UnhookHand();
        }

        public void Configure(HandManager hand, ComboDatabaseSO combos, int? seed = null)
        {
            UnhookHand();
            _hand = hand;
            _comboDatabase = combos;
            _seed = seed ?? Environment.TickCount;
            _generator = new ProceduralCardGenerator(_seed, runDepth: 1);
            HookHand();
        }

        public void BindMaterials(MaterialInventory materials)
        {
            _materials = materials;
        }

        public void HealPlayer(int amount)
        {
            PlayerHp = Mathf.Min(PlayerMaxHp, PlayerHp + Mathf.Max(0, amount));
            PlayerHpChanged?.Invoke(PlayerHp);
            StateChanged?.Invoke();
        }

        public void DamagePlayerPublic(int amount) => DamagePlayer(amount);

        private void HookHand()
        {
            if (_hand == null) return;
            _hand.FusionSucceeded += OnFusionSucceeded;
        }

        private void UnhookHand()
        {
            if (_hand == null) return;
            _hand.FusionSucceeded -= OnFusionSucceeded;
        }

        // ── Public API ────────────────────────────────────────────────

        /// <summary>Start infinite arcade combat. / Старт бесконечной аркады.</summary>
        public void StartRun()
        {
            StopAllCoroutines();
            _busy = false;

            if (_hand == null)
                _hand = HandManager.Instance;
            if (_seed == 0)
                _seed = Environment.TickCount;
            _generator = new ProceduralCardGenerator(_seed, runDepth: 1);

            if (_comboDatabase != null && _hand != null)
                _hand.SetComboDatabase(_comboDatabase);

            HookHand();
            _comboPower.Clear();
            ClearCommittedBuffs();

            // New run replaces previous save after first autosave
            PlayerHp = _playerMaxHp;
            PlayerHpChanged?.Invoke(PlayerHp);
            TurnNumber = 0;
            EnemyIndex = 0;

            SpawnNextEnemy();
            _hand?.Clear();

            // Opening hand
            var opening = _generator.GenerateOpeningHand(_openingHandSize);
            for (int i = 0; i < opening.Count; i++)
                _hand?.AddCard(opening[i]);

            Emit(_useRu
                ? "Run начат. Тяни элементы на заклинания, заклинания — на врага."
                : "Run started. Fuse elements, drag spells onto the enemy.");

            BeginTurn();
            TryAutoSave();
            StateChanged?.Invoke();
        }

        private bool _useRu = true;

        public void SetLanguage(bool russian) => _useRu = russian;
        public void SetAutoSave(bool enabled) => _autoSaveEnabled = enabled;

        /// <summary>Exit arcade anytime — saves progress first. / Выход с сохранением.</summary>
        public void ExitRun()
        {
            StopAllCoroutines();
            _busy = false;

            // Save while still in a valid mid-run phase
            if (PlayerHp > 0 && TurnNumber > 0 && Phase != TurnPhase.Defeat)
            {
                // Force PlayerAction-ish snapshot for clean Continue
                if (Phase != TurnPhase.PlayerAction && Phase != TurnPhase.Exited)
                    SetPhase(TurnPhase.PlayerAction);
                SaveRun();
            }

            SetPhase(TurnPhase.Exited);
            Emit(_useRu
                ? "Выход. Прогресс run сохранён — можно продолжить позже."
                : "Exited. Run progress saved — you can continue later.");
            RunExited?.Invoke();
            StateChanged?.Invoke();
        }

        // ── Save / Load ───────────────────────────────────────────────

        public RunSaveData CaptureSaveData()
        {
            var data = new RunSaveData
            {
                seed = _seed,
                turnNumber = TurnNumber,
                enemyIndex = EnemyIndex,
                playerHp = PlayerHp,
                playerMaxHp = _playerMaxHp,
                cardsRemaining = CardsRemaining,
                cardsPerTurnLimit = CardsPerTurnLimit,
                // Always resume as player action for stable Continue
                phase = (int)(Phase == TurnPhase.Defeat || Phase == TurnPhase.Exited
                    ? Phase
                    : TurnPhase.PlayerAction),
                enemy = new EnemySaveData
                {
                    nameEn = Enemy?.NameEn,
                    nameRu = Enemy?.NameRu,
                    maxHp = Enemy?.MaxHp ?? 0,
                    currentHp = Enemy?.CurrentHp ?? 0,
                    attackDamage = Enemy?.AttackDamage ?? 0,
                    level = Enemy?.Level ?? 1,
                    isBoss = Enemy != null && Enemy.IsBoss,
                    archetypeId = Enemy?.ArchetypeId ?? "",
                    visual = (int)(Enemy?.Visual ?? EnemyVisual.Goblin),
                    isSwarm = Enemy != null && Enemy.IsSwarm,
                    unitCount = Enemy?.UnitCount ?? 1,
                    unitsAlive = Enemy?.UnitsAlive ?? 1,
                    hpPerUnit = Enemy?.HpPerUnit ?? 0,
                    attackPerUnit = Enemy?.AttackPerUnit ?? 0,
                    affinity = (int)(Enemy?.Affinity ?? ElementId.None)
                },
                matEssence = _materials?.Essence ?? 0,
                matShard = _materials?.Shard ?? 0,
                matChaosDust = _materials?.ChaosDust ?? 0,
                matBossCore = _materials?.BossCore ?? 0,
                committedCardsPerTurnBonus = _committedCardsPerTurnBonus,
                committedCritBonus = _committedCritBonus,
                committedFloodBonus = _committedFloodBonus
            };

            if (_hand != null && _hand.Cards.Count > 0)
            {
                data.hand = new CardSaveData[_hand.Cards.Count];
                for (int i = 0; i < _hand.Cards.Count; i++)
                    data.hand[i] = CardSaveData.FromInstance(_hand.Cards[i]);
            }
            else
            {
                data.hand = Array.Empty<CardSaveData>();
            }

            if (_comboPower.Count > 0)
            {
                data.comboPowers = new ComboPowerSave[_comboPower.Count];
                int i = 0;
                foreach (var kv in _comboPower)
                {
                    data.comboPowers[i++] = new ComboPowerSave
                    {
                        instanceId = kv.Key,
                        bonus = kv.Value.bonus,
                        mult = kv.Value.mult
                    };
                }
            }
            else
            {
                data.comboPowers = Array.Empty<ComboPowerSave>();
            }

            return data;
        }

        public bool SaveRun()
        {
            if (PlayerHp <= 0)
                return false;
            if (TurnNumber <= 0 && (_hand == null || _hand.Count == 0))
                return false;

            var data = CaptureSaveData();
            bool ok = RunSaveService.Save(data);
            if (ok)
                Emit(_useRu ? "💾 Run сохранён." : "💾 Run saved.");
            return ok;
        }

        public void TryAutoSave()
        {
            if (!_autoSaveEnabled)
                return;
            if (PlayerHp <= 0 || Phase == TurnPhase.Defeat || Phase == TurnPhase.None)
                return;
            if (TurnNumber <= 0 && (_hand == null || _hand.Count == 0))
                return;

            // Silent autosave (log only failures)
            var data = CaptureSaveData();
            if (!RunSaveService.Save(data))
                Emit(_useRu ? "Не удалось автосохранить." : "Autosave failed.");
        }

        /// <summary>Load save from disk into this combat. / Загрузить run с диска.</summary>
        public bool TryContinueRun()
        {
            if (!RunSaveService.TryLoad(out var data))
            {
                Emit(_useRu ? "Сохранение не найдено." : "No save found.");
                return false;
            }

            return ApplySaveData(data);
        }

        public bool ApplySaveData(RunSaveData data)
        {
            if (data == null)
                return false;

            StopAllCoroutines();
            _busy = false;

            if (_hand == null)
                _hand = HandManager.Instance;

            _seed = data.seed != 0 ? data.seed : Environment.TickCount;
            TurnNumber = Mathf.Max(0, data.turnNumber);
            EnemyIndex = Mathf.Max(0, data.enemyIndex);
            _playerMaxHp = data.playerMaxHp > 0 ? data.playerMaxHp : _playerMaxHp;
            PlayerHp = Mathf.Clamp(data.playerHp, 1, _playerMaxHp);
            PlayerHpChanged?.Invoke(PlayerHp);

            CardsRemaining = Mathf.Max(0, data.cardsRemaining);
            CardsPerTurnLimit = data.cardsPerTurnLimit > 0
                ? data.cardsPerTurnLimit
                : _baseCardsPerTurn;
            _committedCardsPerTurnBonus = Mathf.Max(0, data.committedCardsPerTurnBonus);
            _committedCritBonus = Mathf.Max(0f, data.committedCritBonus);
            _committedFloodBonus = Mathf.Max(0, data.committedFloodBonus);

            _generator = new ProceduralCardGenerator(_seed + TurnNumber * 9176, runDepth: EnemyIndex + 1);

            if (_comboDatabase != null && _hand != null)
                _hand.SetComboDatabase(_comboDatabase);
            HookHand();

            // Enemy
            Enemy.HpChanged -= OnEnemyHpChanged;
            if (data.enemy != null && data.enemy.maxHp > 0)
            {
                Enemy.Reset(
                    data.enemy.maxHp,
                    data.enemy.attackDamage,
                    data.enemy.level > 0 ? data.enemy.level : EnemyIndex + 1,
                    string.IsNullOrEmpty(data.enemy.nameEn) ? "Foe" : data.enemy.nameEn,
                    string.IsNullOrEmpty(data.enemy.nameRu) ? "Враг" : data.enemy.nameRu,
                    data.enemy.isBoss);
                Enemy.ArchetypeId = data.enemy.archetypeId ?? "";
                Enemy.Visual = (EnemyVisual)data.enemy.visual;
                Enemy.IsSwarm = data.enemy.isSwarm;
                Enemy.UnitCount = Mathf.Max(1, data.enemy.unitCount);
                Enemy.UnitsAlive = Mathf.Max(0, data.enemy.unitsAlive);
                Enemy.HpPerUnit = data.enemy.hpPerUnit;
                Enemy.AttackPerUnit = data.enemy.attackPerUnit > 0
                    ? data.enemy.attackPerUnit
                    : data.enemy.attackDamage;
                Enemy.Affinity = (ElementId)data.enemy.affinity;
                int targetHp = Mathf.Clamp(data.enemy.currentHp, 0, data.enemy.maxHp);
                if (targetHp <= 0) targetHp = 1;
                Enemy.SetHp(targetHp);
            }
            else
            {
                SpawnNextEnemy();
            }

            Enemy.HpChanged += OnEnemyHpChanged;

            // Materials (v2)
            if (_materials != null)
            {
                _materials.Essence = Mathf.Max(0, data.matEssence);
                _materials.Shard = Mathf.Max(0, data.matShard);
                _materials.ChaosDust = Mathf.Max(0, data.matChaosDust);
                _materials.BossCore = Mathf.Max(0, data.matBossCore);
                _materials.NotifyChanged();
            }

            // Combo cache
            _comboPower.Clear();
            if (data.comboPowers != null)
            {
                for (int i = 0; i < data.comboPowers.Length; i++)
                {
                    var cp = data.comboPowers[i];
                    if (cp == null || string.IsNullOrEmpty(cp.instanceId))
                        continue;
                    _comboPower[cp.instanceId] = (cp.bonus, cp.mult <= 0f ? 1f : cp.mult);
                }
            }

            // Hand
            _hand?.Clear();
            if (data.hand != null)
            {
                for (int i = 0; i < data.hand.Length; i++)
                {
                    if (data.hand[i] == null) continue;
                    var card = data.hand[i].ToInstance();
                    _hand?.AddCard(card);
                }
            }

            // If empty hand somehow — give a few cards
            if (_hand != null && _hand.Count == 0)
            {
                var emergency = _generator.GenerateOpeningHand(Mathf.Min(4, _openingHandSize));
                for (int i = 0; i < emergency.Count; i++)
                    _hand.AddCard(emergency[i]);
            }

            if (CardsRemaining <= 0)
                CardsRemaining = Mathf.Max(1, CardsPerTurnLimit);

            SetPhase(TurnPhase.PlayerAction);
            Emit(_useRu
                ? $"Продолжение run: ход {TurnNumber}, волна {EnemyIndex + 1}, HP {PlayerHp}."
                : $"Continued run: turn {TurnNumber}, wave {EnemyIndex + 1}, HP {PlayerHp}.");
            StateChanged?.Invoke();
            return true;
        }

        private void OnApplicationPause(bool pauseStatus)
        {
            if (pauseStatus)
                TryAutoSave();
        }

        private void OnApplicationQuit()
        {
            TryAutoSave();
        }

        public void EndPlayerTurn()
        {
            if (Phase != TurnPhase.PlayerAction || _busy)
                return;
            StartCoroutine(EndTurnRoutine());
        }

        /// <summary>
        /// Spend card-play budget (fusion or cast). / Списать лимит карт.
        /// </summary>
        public bool TrySpendPlays(int cost)
        {
            if (Phase != TurnPhase.PlayerAction)
                return false;
            cost = Mathf.Max(1, cost);
            if (CardsRemaining < cost)
            {
                Emit(_useRu ? "Лимит карт на ход исчерпан." : "Card limit reached this turn.");
                return false;
            }

            CardsRemaining -= cost;
            StateChanged?.Invoke();
            return true;
        }

        public int PeekFinalSpellPower(CardInstance spell)
        {
            if (spell == null) return 0;
            if (_comboPower.TryGetValue(spell.InstanceId, out var c))
                return spell.GetFinalPower(c.bonus, c.mult);
            return spell.GetFinalPower();
        }

        /// <summary>
        /// Activate Effect / buff card on the player. Locks bonuses for the run and frees the hand slot.
        /// Активировать бафф на себе: бонус на весь run, карта уходит из руки.
        /// </summary>
        public bool TryPlayEffect(CardView view)
        {
            if (view?.Data == null || _busy)
                return false;
            if (Phase != TurnPhase.PlayerAction)
                return false;
            if (view.Data.CardType != CardType.Effect)
            {
                Emit(_useRu ? "Сюда кладут только баффы (✦)." : "Only buff cards (✦) go here.");
                return false;
            }

            if (view.Data.HasFlag(CardFlags.Exhausted))
                return false;

            var data = view.Data;
            Vector3 from = view.transform.position;
            Vector3 to = GetPlayerCastOrigin();

            // Detach first; animate fly into aristocrat, then apply
            if (_hand != null)
                _hand.DetachCard(data, out _);
            var canvas = view.GetComponentInParent<Canvas>();
            if (canvas != null)
                view.transform.SetParent(canvas.transform, true);

            _busy = true;
            Kartovasiya.UI.PlayerCharacterView.Instance?.PlayCastGesture(0.45f);

            void ApplyBuff()
            {
                int plays = data.GetTotalCardsPerTurnBonus();
                float crit = data.GetTotalCritChanceBonus();
                int flood = data.GetTotalCardFloodBonus();

                _committedCardsPerTurnBonus += plays;
                _committedCritBonus += crit;
                _committedFloodBonus += flood;

                if (plays > 0)
                {
                    CardsPerTurnLimit = Mathf.Clamp(CardsPerTurnLimit + plays, 1, GameConstants.MaxCardsPerTurn);
                    CardsRemaining += plays;
                }

                string name = data.GetDisplayName(_useRu);
                string detail = BuildBuffDetail(plays, crit, flood, _useRu);
                Emit(_useRu
                    ? $"✦ Бафф «{name}» на Аристократа! {detail}"
                    : $"✦ Buff '{name}' on the Aristocrat! {detail}");

                if (view != null)
                    Destroy(view.gameObject);

                Kartovasiya.UI.CombatVfxService.Instance?.PlaySelfBuff(to, Kartovasiya.UI.TarotPalette.GoldBright);
                Kartovasiya.UI.ProceduralSfx.Instance?.PlayCombo();

                if (Phase == TurnPhase.PlayerAction)
                    _busy = false;
                TryAutoSave();
                StateChanged?.Invoke();
            }

            // Card flies down into player, then golden aura
            if (view.Animator != null)
            {
                view.Animator.PlayCastFly(to, 0.32f, ApplyBuff);
            }
            else
            {
                ApplyBuff();
            }

            // Soft projectile trail self-buff
            Kartovasiya.UI.CombatVfxService.Instance?.PlaySpellProjectile(
                from, to, ElementId.Light, Kartovasiya.UI.TarotPalette.Gold, null, 0.28f);

            return true;
        }

        /// <summary>UI helper: short message from drag feedback. / Подсказка из drag.</summary>
        public void EmitPlayerHint(string ru, string en) => Emit(_useRu ? ru : en);

        /// <summary>
        /// Summary of active buffs (hand passives + committed). For HUD.
        /// </summary>
        public string GetBuffSummary(bool ru)
        {
            int plays = _committedCardsPerTurnBonus;
            float crit = _committedCritBonus;
            int flood = _committedFloodBonus;

            if (_hand != null)
            {
                for (int i = 0; i < _hand.Cards.Count; i++)
                {
                    var c = _hand.Cards[i];
                    if (c == null) continue;
                    // Hand passives only for cards still in hand (committed already counted above)
                    if (c.CardType == CardType.Effect || c.CardsPerTurnBonus > 0 || c.CritChanceBonus > 0f || c.CardFloodBonus > 0)
                    {
                        plays += c.GetTotalCardsPerTurnBonus();
                        crit += c.GetTotalCritChanceBonus();
                        flood += c.GetTotalCardFloodBonus();
                    }
                }
            }

            if (plays <= 0 && crit <= 0.001f && flood <= 0)
                return string.Empty;

            var parts = new System.Collections.Generic.List<string>(3);
            if (plays > 0)
                parts.Add(ru ? $"+{plays} карт" : $"+{plays} plays");
            if (crit > 0.001f)
                parts.Add(ru ? $"крит {Mathf.RoundToInt(crit * 100)}%" : $"crit {Mathf.RoundToInt(crit * 100)}%");
            if (flood > 0)
                parts.Add(ru ? $"+{flood} дроп" : $"+{flood} flood");

            return (ru ? "Бафы: " : "Buffs: ") + string.Join(" · ", parts);
        }

        private static string BuildBuffDetail(int plays, float crit, int flood, bool ru)
        {
            var parts = new System.Collections.Generic.List<string>(3);
            if (plays > 0)
                parts.Add(ru ? $"+{plays} карт/ход" : $"+{plays} plays/turn");
            if (crit > 0.001f)
                parts.Add(ru ? $"крит +{Mathf.RoundToInt(crit * 100)}%" : $"crit +{Mathf.RoundToInt(crit * 100)}%");
            if (flood > 0)
                parts.Add(ru ? $"+{flood} дроп" : $"+{flood} flood");
            return parts.Count > 0 ? string.Join(", ", parts) : (ru ? "активен" : "active");
        }

        private void ClearCommittedBuffs()
        {
            _committedCardsPerTurnBonus = 0;
            _committedCritBonus = 0f;
            _committedFloodBonus = 0;
        }

        /// <summary>
        /// Cast spell card view (fly animation) onto enemy.
        /// Каст view-заклинания с анимацией полёта.
        /// </summary>
        public bool TryCastSpell(CardView view)
        {
            if (view?.Data == null || !CanPlayerAct || _busy)
                return false;
            if (view.Data.CardType != CardType.Spell || Enemy == null || Enemy.IsDead)
                return false;

            int cost = Mathf.Max(1, view.Data.CostCards);
            if (CardsRemaining < cost)
            {
                Emit(_useRu ? "Лимит карт на ход исчерпан." : "Card limit reached this turn.");
                view.Animator?.PlayReject();
                Kartovasiya.UI.ProceduralSfx.Instance?.PlayDropFail();
                return false;
            }

            var data = view.Data;

            // Detach from hand immediately so layout updates
            if (_hand != null)
                _hand.DetachCard(data, out _);

            // Parent to canvas root for free flight
            var canvas = view.GetComponentInParent<Canvas>();
            if (canvas != null)
                view.transform.SetParent(canvas.transform, true);

            _busy = true;
            Vector3 enemyPos = EnemyDropZone.Instance != null
                ? EnemyDropZone.Instance.transform.position
                : view.transform.position + Vector3.up * 400f;

            // Aristocrat cast gesture while spell flies
            Kartovasiya.UI.PlayerCharacterView.Instance?.PlayCastGesture(0.55f);

            // Short card toss toward enemy, then elemental projectile from player → boss
            if (view.Animator != null)
            {
                view.Animator.PlayCastFly(enemyPos, 0.22f, () =>
                {
                    if (view != null)
                        Destroy(view.gameObject);
                    LaunchSpellProjectile(data, enemyPos);
                });
            }
            else
            {
                if (view != null)
                    Destroy(view.gameObject);
                LaunchSpellProjectile(data, enemyPos);
            }

            return true;
        }

        /// <summary>
        /// After card leaves hand: fire elemental ball/bolt from player to enemy, then resolve damage.
        /// </summary>
        private void LaunchSpellProjectile(CardInstance data, Vector3 enemyPos)
        {
            Vector3 from = GetPlayerCastOrigin();
            ElementId element = ResolveCastElement(data);
            Color tint = string.IsNullOrEmpty(data.ActiveComboId)
                ? Kartovasiya.UI.TarotPalette.ForElement(element)
                : Color.Lerp(Kartovasiya.UI.TarotPalette.ForElement(element), data.ActiveComboTint, 0.55f);

            var vfx = Kartovasiya.UI.CombatVfxService.Instance;
            if (vfx == null)
            {
                ResolveSpellDamage(data, alreadyDetached: true);
                if (Phase == TurnPhase.PlayerAction)
                    _busy = false;
                return;
            }

            vfx.PlaySpellProjectile(from, enemyPos, element, tint, () =>
            {
                ResolveSpellDamage(data, alreadyDetached: true);
                // Keep busy if victory/defeat coroutine owns the flow
                if (Phase == TurnPhase.PlayerAction)
                    _busy = false;
            });
        }

        private static Vector3 GetPlayerCastOrigin()
        {
            // Prefer aristocrat hand / cast origin
            if (Kartovasiya.UI.PlayerCharacterView.Instance != null)
                return Kartovasiya.UI.PlayerCharacterView.Instance.CastOrigin;

            if (PlayerDropZone.Instance != null)
                return PlayerDropZone.Instance.transform.position;

            if (EnemyDropZone.Instance != null)
            {
                var enemy = EnemyDropZone.Instance.transform.position;
                return new Vector3(enemy.x, enemy.y - 450f, enemy.z);
            }

            return new Vector3(Screen.width * 0.5f, Screen.height * 0.2f, 0f);
        }

        /// <summary>Pick dominant element for projectile look (combo / infused / affinity).</summary>
        private static ElementId ResolveCastElement(CardInstance spell)
        {
            if (spell == null)
                return ElementId.Arcane;

            // Last infused element often defines the mix flavor
            if (spell.InfusedElements != null && spell.InfusedElements.Count > 0)
            {
                for (int i = spell.InfusedElements.Count - 1; i >= 0; i--)
                {
                    if (ElementUtil.IsValid(spell.InfusedElements[i]))
                        return spell.InfusedElements[i];
                }
            }

            if (ElementUtil.IsValid(spell.PrimaryAffinity))
                return spell.PrimaryAffinity;

            return ElementId.Arcane;
        }

        /// <summary>
        /// Cast by data only (no fly). Removes from hand if still present.
        /// Каст по данным без полёта.
        /// </summary>
        public bool TryCastSpell(CardInstance spell)
        {
            if (!CanPlayerAct || _busy || spell == null)
                return false;
            if (spell.CardType != CardType.Spell)
            {
                Emit(_useRu ? "На врага можно кидать только заклинания." : "Only spells can be cast on enemy.");
                return false;
            }

            if (Enemy == null || Enemy.IsDead)
                return false;

            return ResolveSpellDamage(spell, alreadyDetached: false);
        }

        private bool ResolveSpellDamage(CardInstance spell, bool alreadyDetached)
        {
            if (spell == null || Enemy == null || Enemy.IsDead)
                return false;

            // If called after fly, phase may still be PlayerAction; allow spend even if _busy
            int cost = Mathf.Max(1, spell.CostCards);
            if (CardsRemaining < cost)
                return false;

            CardsRemaining -= cost;

            int power = PeekFinalSpellPower(spell);

            float crit = GetHandCritChance();
            bool isCrit = UnityEngine.Random.value < crit;
            if (isCrit)
                power = Mathf.RoundToInt(power * BalanceConfig.CritMultiplier);

            if (spell.HasFlag(CardFlags.Corrupted) && UnityEngine.Random.value < BalanceConfig.CorruptionBacklashChance)
            {
                int back = Mathf.Max(1, power / 4);
                DamagePlayer(back);
                Emit(_useRu
                    ? $"Порча! Отдача {back} урона тебе."
                    : $"Corruption backlash {back} to you.");
            }

            int dealt = Enemy.TakeDamage(power);
            string pack = Enemy.IsSwarm
                ? (_useRu ? $" · осталось {Enemy.UnitsAlive}/{Enemy.UnitCount}" : $" · left {Enemy.UnitsAlive}/{Enemy.UnitCount}")
                : "";
            if (Enemy.LastUnitsKilled > 0 && Enemy.IsSwarm)
            {
                Emit(_useRu
                    ? $"Сражён{(Enemy.LastUnitsKilled > 1 ? "о" : "")} {Enemy.LastUnitsKilled} из стаи!"
                    : $"{Enemy.LastUnitsKilled} from the pack down!");
            }

            Emit(_useRu
                ? $"{spell.GetDisplayName(true)} → {dealt} урона{(isCrit ? " КРИТ!" : "")}. {Enemy.GetName(true)} {Enemy.CurrentHp}/{Enemy.MaxHp}{pack}"
                : $"{spell.GetDisplayName(false)} → {dealt} dmg{(isCrit ? " CRIT!" : "")}. {Enemy.GetName(false)} {Enemy.CurrentHp}/{Enemy.MaxHp}{pack}");

            Vector3 impactPos = EnemyDropZone.Instance != null
                ? EnemyDropZone.Instance.transform.position
                : Vector3.zero;
            Color tint = string.IsNullOrEmpty(spell.ActiveComboId)
                ? Kartovasiya.UI.TarotPalette.ForElement(spell.PrimaryAffinity)
                : spell.ActiveComboTint;
            // Hit burst + damage numbers (projectile already flew in)
            Kartovasiya.UI.CombatVfxService.Instance?.PlayCastImpact(impactPos, tint, dealt);
            if (isCrit)
            {
                Kartovasiya.UI.CombatVfxService.Instance?.FloatText(
                    impactPos + Vector3.up * 40f, "CRIT!", Kartovasiya.UI.TarotPalette.GoldBright, 30);
            }

            _comboPower.Remove(spell.InstanceId);

            if (!alreadyDetached)
                _hand?.RemoveCard(spell, destroyView: true);

            StateChanged?.Invoke();

            TryAutoSave();

            if (Enemy.IsDead)
            {
                StartCoroutine(OnEnemyDefeatedRoutine());
                return true;
            }

            if (CardsRemaining <= 0 && Phase == TurnPhase.PlayerAction && !_busy)
                EndPlayerTurn();
            else if (CardsRemaining <= 0 && Phase == TurnPhase.PlayerAction)
                StartCoroutine(EndTurnWhenFree());

            return true;
        }

        private IEnumerator EndTurnWhenFree()
        {
            while (_busy)
                yield return null;
            if (Phase == TurnPhase.PlayerAction && CardsRemaining <= 0)
                EndPlayerTurn();
        }

        // ── Turn flow ─────────────────────────────────────────────────

        private void BeginTurn()
        {
            if (Phase == TurnPhase.Exited || Phase == TurnPhase.Defeat)
                return;

            TurnNumber++;
            _generator = new ProceduralCardGenerator(_seed + TurnNumber * 9176, runDepth: EnemyIndex + 1);

            SetPhase(TurnPhase.StartOfTurn);

            // Global random event every N turns
            if (TurnNumber > 1 && TurnNumber % GameConstants.GlobalEventIntervalTurns == 0)
                FireGlobalEvent();

            SetPhase(TurnPhase.DropCards);
            AutoDropCards();

            RecalculateCardsPerTurn();
            CardsRemaining = CardsPerTurnLimit;

            SetPhase(TurnPhase.PlayerAction);
            Emit(_useRu
                ? $"Ход {TurnNumber}. Карт за ход: {CardsRemaining}. Враг: {Enemy.GetName(true)} ({Enemy.CurrentHp}/{Enemy.MaxHp})"
                : $"Turn {TurnNumber}. Plays: {CardsRemaining}. Enemy: {Enemy.GetName(false)} ({Enemy.CurrentHp}/{Enemy.MaxHp})");

            string buffs = GetBuffSummary(_useRu);
            if (!string.IsNullOrEmpty(buffs))
                Emit(buffs);

            TryAutoSave();
            StateChanged?.Invoke();
        }

        private void AutoDropCards()
        {
            if (_hand == null || _generator == null)
                return;

            int flood = GetHandCardFlood();
            int toDrop = _cardsDroppedPerTurn + flood;

            // Also top-up toward opening size if hand emptied
            int room = _hand.MaxHandSize - _hand.Count;
            toDrop = Mathf.Min(toDrop, room);

            // First turn already has opening hand — still drop a little chaos
            if (TurnNumber == 1)
                toDrop = Mathf.Min(toDrop, Mathf.Max(0, room));

            int added = 0;
            for (int i = 0; i < toDrop; i++)
            {
                if (!_hand.CanAddCard())
                    break;
                _hand.AddCard(_generator.GenerateDrop());
                added++;
            }

            if (added > 0)
                Emit(_useRu ? $"+{added} карт в руку (авто-дроп)" : $"+{added} cards (auto-drop)");
        }

        private IEnumerator EndTurnRoutine()
        {
            _busy = true;
            SetPhase(TurnPhase.Resolve);
            yield return null;

            SetPhase(TurnPhase.EnemyAction);
            StateChanged?.Invoke();

            if (Enemy != null && !Enemy.IsDead && Phase != TurnPhase.Exited)
            {
                yield return new WaitForSeconds(_enemyActionDelay);
                int dmg = Enemy.GetAttackDamage();
                DamagePlayer(dmg);
                Emit(_useRu
                    ? $"{Enemy.GetName(true)} бьёт на {dmg}. HP: {PlayerHp}/{PlayerMaxHp}"
                    : $"{Enemy.GetName(false)} hits for {dmg}. HP: {PlayerHp}/{PlayerMaxHp}");
            }

            if (PlayerHp <= 0)
            {
                SetPhase(TurnPhase.Defeat);
                RunSaveService.Delete();
                Emit(_useRu ? "Поражение. Сейв сброшен." : "Defeat. Save cleared.");
                CombatEnded?.Invoke();
                _busy = false;
                StateChanged?.Invoke();
                yield break;
            }

            SetPhase(TurnPhase.EndOfTurn);
            yield return null;

            _busy = false;
            BeginTurn();
        }

        private IEnumerator OnEnemyDefeatedRoutine()
        {
            _busy = true;
            bool wasBoss = Enemy != null && Enemy.IsBoss;
            SetPhase(TurnPhase.Victory);

            // Loot (balance table)
            int essence = BalanceConfig.EssenceOnKill(EnemyIndex, wasBoss);
            int shard = BalanceConfig.ShardOnKill(wasBoss);
            EnemyLootDropped?.Invoke(essence, shard);

            Emit(_useRu
                ? $"{(wasBoss ? "Босс" : "Враг")} повержен! Волна {EnemyIndex + 1}."
                : $"{(wasBoss ? "Boss" : "Enemy")} down! Wave {EnemyIndex + 1}.");
            StateChanged?.Invoke();

            if (wasBoss)
                BossDefeated?.Invoke();

            yield return new WaitForSeconds(_betweenEnemyDelay);

            if (Phase == TurnPhase.Exited)
            {
                _busy = false;
                yield break;
            }

            EnemyIndex++;
            SpawnNextEnemy();
            _busy = false;

            // Reward heal (boss heals more)
            int heal = wasBoss ? BalanceConfig.HealOnBossKill : BalanceConfig.HealOnNormalKill;
            PlayerHp = Mathf.Min(PlayerMaxHp, PlayerHp + heal);
            PlayerHpChanged?.Invoke(PlayerHp);

            BeginTurn();
        }

        private void SpawnNextEnemy()
        {
            int level = EnemyIndex + 1;
            bool bossWave = level % BalanceConfig.BossEveryN == 0;
            int soloHp = Mathf.RoundToInt(BalanceConfig.EnemyHpBase + level * BalanceConfig.EnemyHpPerLevel);
            int soloAtk = Mathf.RoundToInt(BalanceConfig.EnemyAtkBase + level * BalanceConfig.EnemyAtkPerLevel);
            if (bossWave)
            {
                soloHp = Mathf.RoundToInt(soloHp * BalanceConfig.BossHpMult) + BalanceConfig.BossHpFlat;
                soloAtk = Mathf.RoundToInt(soloAtk * BalanceConfig.BossAtkMult) + BalanceConfig.BossAtkFlat;
            }

            var rng = new System.Random(_seed + EnemyIndex * 7919 + 13);
            EnemyDef def = bossWave
                ? EnemyCatalog.PickBoss(rng, level)
                : EnemyCatalog.PickNormal(rng, level);

            int units = 1;
            if (def != null && def.IsSwarm)
                units = rng.Next(def.SwarmMin, def.SwarmMax + 1);

            Enemy.HpChanged -= OnEnemyHpChanged;
            Enemy.ResetFromDef(def, level, soloHp, soloAtk, units, bossWave);
            // Level tag in name for non-named
            if (def != null && !def.IsNamedBoss)
            {
                Enemy.NameEn = $"{def.NameEn}  Lv{level}";
                Enemy.NameRu = $"{def.NameRu}  Ур.{level}";
            }
            else if (def != null && def.IsNamedBoss)
            {
                Enemy.NameEn = $"{def.NameEn}";
                Enemy.NameRu = $"{def.NameRu}";
            }

            Enemy.HpChanged += OnEnemyHpChanged;

            string kind = Enemy.IsBoss
                ? (_useRu ? "БОСС" : "BOSS")
                : Enemy.IsSwarm
                    ? (_useRu ? "СТАЯ" : "PACK")
                    : (_useRu ? "враг" : "enemy");

            Emit(_useRu
                ? $"Новый {kind}: {Enemy.GetName(true)} ({Enemy.MaxHp} HP, удар {Enemy.GetAttackDamage()})"
                : $"New {kind}: {Enemy.GetName(false)} ({Enemy.MaxHp} HP, atk {Enemy.GetAttackDamage()})");
            StateChanged?.Invoke();
        }

        private void OnEnemyHpChanged(int current, int max) => StateChanged?.Invoke();

        private void FireGlobalEvent()
        {
            // UI-driven event (mini-game). Fallback if no listeners: mild bonus.
            if (GlobalEventOffered != null)
            {
                Emit(_useRu ? "⚡ Глобальное событие!" : "⚡ Global event!");
                GlobalEventOffered.Invoke();
                return;
            }

            _baseCardsPerTurnBonusTemp = 1;
            Emit(_useRu ? "⚡ Глобальное событие: +1 карта на этот ход" : "⚡ Global: +1 play this turn");
        }

        private int _baseCardsPerTurnBonusTemp;

        private void RecalculateCardsPerTurn()
        {
            int limit = _baseCardsPerTurn + _baseCardsPerTurnBonusTemp + _committedCardsPerTurnBonus;
            _baseCardsPerTurnBonusTemp = 0;

            if (_hand != null)
            {
                for (int i = 0; i < _hand.Cards.Count; i++)
                {
                    var c = _hand.Cards[i];
                    if (c == null) continue;
                    // Effect cards grant passive while still in hand
                    if (c.CardType == CardType.Effect || c.CardsPerTurnBonus > 0)
                        limit += c.GetTotalCardsPerTurnBonus();
                }
            }

            CardsPerTurnLimit = Mathf.Clamp(limit, 1, GameConstants.MaxCardsPerTurn);
        }

        private float GetHandCritChance()
        {
            float sum = _committedCritBonus;
            if (_hand == null) return sum;
            for (int i = 0; i < _hand.Cards.Count; i++)
            {
                var c = _hand.Cards[i];
                if (c != null)
                    sum += c.GetTotalCritChanceBonus();
            }

            return sum;
        }

        private int GetHandCardFlood()
        {
            int sum = _committedFloodBonus;
            if (_hand == null) return sum;
            for (int i = 0; i < _hand.Cards.Count; i++)
            {
                var c = _hand.Cards[i];
                if (c != null)
                    sum += c.GetTotalCardFloodBonus();
            }

            return sum;
        }

        private void DamagePlayer(int amount)
        {
            PlayerHp = Mathf.Max(0, PlayerHp - amount);
            PlayerHpChanged?.Invoke(PlayerHp);
            StateChanged?.Invoke();
        }

        private void OnFusionSucceeded(CardInstance source, CardInstance target, ComboResolveResult result)
        {
            // Spend after successful fusion
            if (Phase == TurnPhase.PlayerAction && CardsRemaining > 0)
                CardsRemaining--;

            if (target != null && target.CardType == CardType.Element)
            {
                // Element + Element hybrid
                Emit(_useRu
                    ? $"Смесь стихий → «{target.GetDisplayName(true)}»! Перетащи на заклинание."
                    : $"Element mix → '{target.GetDisplayName(false)}'! Drag onto a spell.");
            }
            else if (result.Success && target != null && target.CardType == CardType.Spell)
            {
                int bonus = result.AppliedPowerBonus;
                float mult = result.AppliedPowerMultiplier;
                if (_comboPower.TryGetValue(target.InstanceId, out var prev))
                {
                    bonus = Mathf.Max(prev.bonus, bonus) + Mathf.Max(0, bonus / 4);
                    mult = Mathf.Max(prev.mult, mult);
                }

                _comboPower[target.InstanceId] = (bonus, mult);

                if (result.Recipe != null)
                {
                    Emit(_useRu
                        ? $"Комбо «{result.Recipe.GetResultName(true)}»! +{bonus} силы ×{mult:0.00} → ≈ {PeekFinalSpellPower(target)}"
                        : $"Combo '{result.Recipe.GetResultName(false)}'! +{bonus} power ×{mult:0.00} → ≈ {PeekFinalSpellPower(target)}");
                }
                else
                {
                    Emit(_useRu
                        ? $"Стихии влиты! +{bonus} силы ×{mult:0.00} → ≈ {PeekFinalSpellPower(target)}"
                        : $"Elements infused! +{bonus} power ×{mult:0.00} → ≈ {PeekFinalSpellPower(target)}");
                }
            }
            else if (target != null && target.CardType == CardType.Spell)
            {
                Emit(_useRu
                    ? $"Стихия влита ({target.InfusedElements.Count}/{target.ElementSockets})"
                    : $"Element infused ({target.InfusedElements.Count}/{target.ElementSockets})");
            }

            TryAutoSave();
            StateChanged?.Invoke();

            if (Phase == TurnPhase.PlayerAction && CardsRemaining <= 0)
                EndPlayerTurn();
        }

        private void SetPhase(TurnPhase phase)
        {
            Phase = phase;
            PhaseChanged?.Invoke(phase);
        }

        private void Emit(string msg)
        {
            Debug.Log("[Combat] " + msg);
            LogMessage?.Invoke(msg);
        }
    }
}
