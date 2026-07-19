// Full playable prototype entry: menu, combat, craft, pause, events
// Полный entry прототипа: меню, бой, крафт, пауза, события
// Chaos Spellweaver (Картовасия)

using System.IO;
using System.Text;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;
using Kartovasiya.Cards;
using Kartovasiya.Combat;
using Kartovasiya.Combos;
using Kartovasiya.Core;
using Kartovasiya.Crafting;
using Kartovasiya.Hand;
using Kartovasiya.Save;
using Kartovasiya.UI;

namespace Kartovasiya.Game
{
    /// <summary>
    /// Drop on empty scene → complete offline prototype loop.
    /// Повесь на пустой объект сцены — полный оффлайн-прототип.
    /// </summary>
    public class GameApp : MonoBehaviour
    {
        public static GameApp Instance { get; private set; }

        [SerializeField] private bool _buildOnAwake = true;
        [SerializeField] private bool _russian = true;
        [SerializeField] private int _seed;

        private Canvas _canvas;
        private CombatManager _combat;
        private HandManager _hand;
        private MaterialInventory _materials = new MaterialInventory();
        private ComboDatabaseSO _combos;

        private GameObject _mainMenu;
        private GameObject _pauseMenu;
        private GameObject _settingsMenu;
        private GameObject _hudRoot;
        private RectTransform _handZone;
        private GameObject _eventModal;
        private Text _eventBody;
        private Text _hudText;
        private Text _logText;
        private Text _enemyTitle;
        private Text _enemyHp;
        private Image _enemyPortrait;
        private Text _playerHp;
        private Text _matHud;
        private Text _musicVolLabel;
        private Text _sfxVolLabel;
        private readonly StringBuilder _log = new StringBuilder(512);
        private const int MaxLogLines = 7;

        private bool _inRun;
        private bool _paused;
        private bool _pendingEventMiniGame;
        private System.Action _eventAfterMini;

        private void Awake()
        {
            Instance = this;
            if (_buildOnAwake)
                Build();
        }

        private void OnDestroy()
        {
            if (Instance == this)
                Instance = null;
            Time.timeScale = 1f;
        }

        private void Update()
        {
            // Escape / Android back ≈ pause
            if (_inRun && Input.GetKeyDown(KeyCode.Escape))
                TogglePause();
        }

        [ContextMenu("Build Game App")]
        public void Build()
        {
            EnsureEventSystem();
            EnsureOrientation();

            _canvas = CreateCanvas();
            BuildBackground(_canvas.transform);

            var sfx = new GameObject("Sfx");
            sfx.transform.SetParent(transform, false);
            sfx.AddComponent<ProceduralSfx>();

            var music = new GameObject("MenuMusic");
            music.transform.SetParent(transform, false);
            music.AddComponent<ProceduralMenuMusic>();

            var runMusic = new GameObject("RunMusic");
            runMusic.transform.SetParent(transform, false);
            runMusic.AddComponent<ProceduralRunMusic>();

            // Apply saved volumes after audio components exist
            GameAudioSettings.Load();

            var vfx = new GameObject("Vfx");
            vfx.transform.SetParent(transform, false);
            vfx.AddComponent<CombatVfxService>().Configure(_canvas);

            var mini = new GameObject("MiniGame");
            mini.transform.SetParent(transform, false);
            var timing = mini.AddComponent<TimingMiniGame>();
            timing.Build(_canvas.transform);

            var craftGo = new GameObject("CraftPanel");
            craftGo.transform.SetParent(transform, false);
            craftGo.AddComponent<CraftPanel>().Build(_canvas.transform);

            var tutGo = new GameObject("Tutorial");
            tutGo.transform.SetParent(transform, false);
            tutGo.AddComponent<TutorialOverlay>().Build(_canvas.transform);

            BuildCombatUi(_canvas.transform);
            BuildMainMenu(_canvas.transform);
            BuildSettingsMenu(_canvas.transform);
            BuildPauseMenu(_canvas.transform);
            BuildEventModal(_canvas.transform);

            _combos = ScriptableObject.CreateInstance<ComboDatabaseSO>();
            _combos.LoadPrototypeDefaults();

            // Hand + combat under app
            var handGo = new GameObject("HandManager", typeof(RectTransform));
            handGo.transform.SetParent(_handZone != null ? _handZone : _canvas.transform, false);
            if (_handZone != null)
                Stretch(handGo.GetComponent<RectTransform>());
            _hand = handGo.AddComponent<HandManager>();
            _hand.SetCanvas(_canvas);
            if (_handZone != null)
                _hand.SetHandContainer(_handZone);
            _hand.SetComboDatabase(_combos);

            var combatGo = new GameObject("CombatManager");
            combatGo.transform.SetParent(transform, false);
            _combat = combatGo.AddComponent<CombatManager>();
            _combat.SetLanguage(_russian);
            _combat.Configure(_hand, _combos, _seed != 0 ? _seed : (int?)null);
            _combat.BindMaterials(_materials);

            _combat.StateChanged += RefreshHud;
            _combat.LogMessage += OnLog;
            _combat.PlayerHpChanged += _ => RefreshHud();
            _combat.PhaseChanged += OnPhase;
            _combat.RunExited += OnRunExited;
            _combat.CombatEnded += OnCombatEnded;
            _combat.BossDefeated += OnBossDefeated;
            _combat.GlobalEventOffered += OnGlobalEvent;
            _combat.EnemyLootDropped += OnLoot;
            _materials.Changed += RefreshHud;

            MobilePerformance.Apply();
            ShowMainMenu();
        }

        // ── Menus ─────────────────────────────────────────────────────

        private void ShowMainMenu()
        {
            _inRun = false;
            _paused = false;
            Time.timeScale = 1f;
            SetCombatUiVisible(false);
            _pauseMenu.SetActive(false);
            if (_settingsMenu != null)
                _settingsMenu.SetActive(false);
            _eventModal.SetActive(false);
            CraftPanel.Instance?.Close();
            _mainMenu.SetActive(true);
            _mainMenu.transform.SetAsLastSibling();
            ProceduralRunMusic.Instance?.StopRun();
            ProceduralMenuMusic.Instance?.PlayMenu();

            // Continue button state
            var cont = _mainMenu.transform.Find("Panel/BtnContinue");
            if (cont != null)
                cont.gameObject.SetActive(RunSaveService.HasSave());
        }

        private void OpenSettings()
        {
            if (_settingsMenu == null) return;
            RefreshSettingsLabels();
            _settingsMenu.SetActive(true);
            _settingsMenu.transform.SetAsLastSibling();
        }

        private void CloseSettings()
        {
            if (_settingsMenu != null)
                _settingsMenu.SetActive(false);
        }

        private void AdjustMusic(float delta)
        {
            GameAudioSettings.SetMusicVolume(GameAudioSettings.MusicVolume + delta);
            RefreshSettingsLabels();
            ProceduralSfx.Instance?.PlayUi();
        }

        private void AdjustSfx(float delta)
        {
            GameAudioSettings.SetSfxVolume(GameAudioSettings.SfxVolume + delta);
            RefreshSettingsLabels();
            ProceduralSfx.Instance?.PlayUi();
        }

        private void RefreshSettingsLabels()
        {
            if (_musicVolLabel != null)
            {
                int pct = Mathf.RoundToInt(GameAudioSettings.MusicVolume * 100f);
                _musicVolLabel.text = _russian ? $"Музыка: {pct}%" : $"Music: {pct}%";
            }

            if (_sfxVolLabel != null)
            {
                int pct = Mathf.RoundToInt(GameAudioSettings.SfxVolume * 100f);
                _sfxVolLabel.text = _russian ? $"Звуки: {pct}%" : $"SFX: {pct}%";
            }
        }

        private void StartNewRun()
        {
            RunSaveService.Delete();
            _materials.Clear();
            _materials.Add(MaterialType.Essence, 3);
            _materials.Add(MaterialType.Shard, 1);
            _mainMenu.SetActive(false);
            ProceduralMenuMusic.Instance?.StopMenu();

            // Tutorial first (once), then combat
            TutorialOverlay.Instance?.TryShow(_russian, BeginCombatAfterTutorial);
        }

        private void BeginCombatAfterTutorial()
        {
            SetCombatUiVisible(true);
            _inRun = true;
            _combat.StartRun();
            ProceduralRunMusic.Instance?.PlayRun();
            RefreshHud();
            OnLog(_russian
                ? "Новый run. Огонь+Вода на печати = Пар → кинь заклинание на врага. «Кузня» / Esc."
                : "New run. Fire+Water on sigil = Steam → cast on enemy. Forge / Esc.");
            CombatVfxService.Instance?.PlayScreenFlash(TarotPalette.VoidPurple, 0.3f, 0.35f);
        }

        private void ContinueRun()
        {
            _mainMenu.SetActive(false);
            ProceduralMenuMusic.Instance?.StopMenu();
            SetCombatUiVisible(true);
            _inRun = true;
            if (!_combat.TryContinueRun())
            {
                StartNewRun();
                return;
            }

            ProceduralRunMusic.Instance?.PlayRun();
            // Materials restored inside combat Apply if bound
            RefreshHud();
            OnLog(_russian ? "Run продолжен." : "Run continued.");
        }

        private void ExitToMenu()
        {
            if (_inRun && _combat != null && _combat.PlayerHp > 0)
                _combat.ExitRun();
            ShowMainMenu();
        }

        private void TogglePause()
        {
            if (!_inRun) return;
            if (CraftPanel.Instance != null && CraftPanel.Instance.IsOpen) return;
            if (_eventModal.activeSelf) return;

            _paused = !_paused;
            Time.timeScale = _paused ? 0f : 1f;
            _pauseMenu.SetActive(_paused);
            if (_paused)
                _pauseMenu.transform.SetAsLastSibling();
        }

        private void OnRunExited()
        {
            ShowMainMenu();
        }

        private void OnCombatEnded()
        {
            // Defeat: wipe already done in CombatManager
            OnLog(_russian ? "Поражение. Возврат в меню…" : "Defeat. Returning to menu…");
            CombatVfxService.Instance?.PlayScreenFlash(TarotPalette.BloodRed, 0.4f, 0.45f);
            CombatVfxService.Instance?.PlayScreenShake(0.35f, 18f);
            StartCoroutine(ReturnToMenuAfterDefeat());
        }

        private System.Collections.IEnumerator ReturnToMenuAfterDefeat()
        {
            yield return new WaitForSecondsRealtime(1.6f);
            _inRun = false;
            Time.timeScale = 1f;
            ShowMainMenu();
            OnLog(_russian
                ? "Попробуй новый run. Комбо и кузня сильнее с материалами."
                : "Try a new run. Combos + forge get stronger with mats.");
        }

        private void OnPhase(TurnPhase p)
        {
            RefreshHud();
        }

        private void OnLoot(int essence, int shard)
        {
            _materials.Add(MaterialType.Essence, essence);
            if (shard > 0)
                _materials.Add(MaterialType.Shard, shard);
            OnLog(_russian
                ? $"+материалы: эссенция {essence}" + (shard > 0 ? $", осколок {shard}" : "")
                : $"+mats: essence {essence}" + (shard > 0 ? $", shard {shard}" : ""));
        }

        private void OnBossDefeated()
        {
            _materials.Add(MaterialType.BossCore, 1);
            _materials.Add(MaterialType.ChaosDust, 2);
            _materials.Add(MaterialType.Essence, 4);
            OnLog(_russian ? "Босс повержен! +Ядро и пыль. Бонус-мини-игра…" : "Boss down! +Core. Bonus mini-game…");

            TimingMiniGame.Instance?.Play(MiniGameContext.BossBonus, _russian, skill =>
            {
                if (skill >= 0.5f)
                {
                    _materials.Add(MaterialType.BossCore, 1);
                    _materials.Add(MaterialType.Shard, 2);
                    OnLog(_russian ? "Бонус босса: доп. ядро и осколки!" : "Boss bonus: extra core & shards!");
                    CombatVfxService.Instance?.PlayComboBurst(Vector3.zero, TarotPalette.Risk, "BOSS!", false);
                }
                else
                {
                    _materials.Add(MaterialType.Essence, 2);
                    OnLog(_russian ? "Бонус слабый: +эссенция." : "Weak bonus: +essence.");
                }

                _combat?.TryAutoSave();
                // Offer craft after boss
                OpenCraft();
            }, greenHalf: 0.11f);
        }

        private void OnGlobalEvent()
        {
            if (!_inRun) return;
            ShowEventModal();
        }

        private void ShowEventModal()
        {
            _eventModal.SetActive(true);
            _eventModal.transform.SetAsLastSibling();
            int roll = Random.Range(0, 3);
            if (_russian)
            {
                _eventBody.text = roll switch
                {
                    0 => "Судьба шепчет…\nПройди мини-игру за хаотическую пыль\nили получи удар (−3 HP).",
                    1 => "Разлом реальности!\nМини-игра → лечение\nили потеря 1 карты-силы.",
                    _ => "Блуждающая кузница.\nМини-игра → шанс бесплатного крафта\nили −2 эссенции."
                };
            }
            else
            {
                _eventBody.text = "A global event! Play the mini-game for a reward, or take a penalty.";
            }

            _eventAfterMini = () =>
            {
                // resolved in button handlers
            };

            // Store roll on modal
            _eventModal.GetComponent<EventRollHolder>().Roll = roll;
        }

        private void ResolveEventAccept()
        {
            int roll = _eventModal.GetComponent<EventRollHolder>().Roll;
            _eventModal.SetActive(false);
            TimingMiniGame.Instance?.Play(MiniGameContext.GlobalEvent, _russian, skill =>
            {
                if (skill >= 0.45f)
                {
                    switch (roll)
                    {
                        case 0:
                            _materials.Add(MaterialType.ChaosDust, 2 + (skill > 0.8f ? 1 : 0));
                            OnLog(_russian ? "Событие: +Пыль хаоса!" : "Event: +Chaos Dust!");
                            break;
                        case 1:
                            _combat.HealPlayer(5 + (int)(skill * 4));
                            OnLog(_russian ? "Событие: исцеление!" : "Event: healed!");
                            break;
                        default:
                            _materials.Add(MaterialType.Essence, 2);
                            OnLog(_russian ? "Событие: эссенция. Открываем кузню…" : "Event: essence. Opening forge…");
                            OpenCraft();
                            break;
                    }

                    CombatVfxService.Instance?.PlayScreenFlash(TarotPalette.Gold, 0.2f, 0.25f);
                }
                else
                {
                    switch (roll)
                    {
                        case 0:
                            _combat.DamagePlayerPublic(BalanceConfig.EventFailPenaltyHp);
                            OnLog(_russian
                                ? $"Событие провалено: −{BalanceConfig.EventFailPenaltyHp} HP."
                                : $"Event failed: −{BalanceConfig.EventFailPenaltyHp} HP.");
                            break;
                        case 1:
                            _combat.DamagePlayerPublic(BalanceConfig.EventSkipPenaltyHp);
                            OnLog(_russian
                                ? $"Событие провалено: −{BalanceConfig.EventSkipPenaltyHp} HP."
                                : $"Event failed: −{BalanceConfig.EventSkipPenaltyHp} HP.");
                            break;
                        default:
                            _materials.Add(MaterialType.Essence, -2);
                            OnLog(_russian ? "Событие: −2 эссенции." : "Event: −2 essence.");
                            break;
                    }
                }

                _combat.TryAutoSave();
                RefreshHud();
            }, greenHalf: 0.12f);
        }

        private void ResolveEventSkip()
        {
            _eventModal.SetActive(false);
            _combat.DamagePlayerPublic(BalanceConfig.EventSkipPenaltyHp);
            OnLog(_russian
                ? $"Событие проигнорировано (−{BalanceConfig.EventSkipPenaltyHp} HP)."
                : $"Event skipped (−{BalanceConfig.EventSkipPenaltyHp} HP).");
            _combat.TryAutoSave();
        }

        private void OpenCraft()
        {
            if (!_inRun || _combat == null) return;
            if (_combat.Phase != TurnPhase.PlayerAction)
            {
                OnLog(_russian ? "Кузня только в твой ход." : "Forge only on your turn.");
                return;
            }

            CraftPanel.Instance?.Open(_materials, _hand, _russian, OnLog, () =>
            {
                _combat.TryAutoSave();
                RefreshHud();
                // Refresh card views
                if (_hand != null)
                {
                    for (int i = 0; i < _hand.Views.Count; i++)
                        _hand.Views[i]?.RefreshVisuals();
                }
            });
        }

        // ── HUD ───────────────────────────────────────────────────────

        private void RefreshHud()
        {
            if (_combat == null) return;

            if (_enemyTitle != null)
                _enemyTitle.text = _combat.Enemy?.GetName(_russian) ?? "—";

            if (_enemyHp != null && _combat.Enemy != null)
            {
                var e = _combat.Enemy;
                string tag = e.IsBoss
                    ? (_russian ? " [БОСС]" : " [BOSS]")
                    : e.IsSwarm
                        ? (_russian ? " [СТАЯ]" : " [PACK]")
                        : "";
                _enemyHp.text = e.IsSwarm
                    ? $"HP {e.CurrentHp}/{e.MaxHp}  ATK {e.GetAttackDamage()}  ({e.UnitsAlive}/{e.UnitCount}){tag}"
                    : $"HP {e.CurrentHp}/{e.MaxHp}  ATK {e.GetAttackDamage()}{tag}";
            }

            if (_enemyPortrait != null && _combat.Enemy != null)
            {
                _enemyPortrait.sprite = ProceduralEnemyArt.Get(_combat.Enemy.Visual, _combat.Enemy.Affinity);
                _enemyPortrait.color = Color.white;
                _enemyPortrait.preserveAspect = true;
            }

            if (_playerHp != null)
                _playerHp.text = _russian
                    ? $"Ты: {_combat.PlayerHp}/{_combat.PlayerMaxHp}"
                    : $"You: {_combat.PlayerHp}/{_combat.PlayerMaxHp}";

            if (_hudText != null)
            {
                string buffs = _combat.GetBuffSummary(_russian);
                string core = _russian
                    ? $"Ход {_combat.TurnNumber} | {_combat.CardsRemaining}/{_combat.CardsPerTurnLimit} карт | Волна {_combat.EnemyIndex + 1}"
                    : $"Turn {_combat.TurnNumber} | {_combat.CardsRemaining}/{_combat.CardsPerTurnLimit} plays | Wave {_combat.EnemyIndex + 1}";
                _hudText.text = string.IsNullOrEmpty(buffs) ? core : $"{core}\n{buffs}";
            }

            if (_matHud != null)
                _matHud.text = _materials.Summary(_russian);
        }

        private void OnLog(string msg)
        {
            _log.Insert(0, msg + "\n");
            int lines = 0;
            for (int i = 0; i < _log.Length; i++)
            {
                if (_log[i] != '\n') continue;
                lines++;
                if (lines <= MaxLogLines) continue;
                _log.Length = i;
                break;
            }

            if (_logText != null)
                _logText.text = _log.ToString();
            Debug.Log("[Game] " + msg);
        }

        private void SetCombatUiVisible(bool on)
        {
            if (_hudRoot != null)
                _hudRoot.SetActive(on);
        }

        // ── UI construction ───────────────────────────────────────────

        private void BuildMainMenu(Transform canvas)
        {
            // Full-screen menu: neon backdrop + KTV logo + panel
            _mainMenu = new GameObject("MainMenu", typeof(RectTransform));
            _mainMenu.transform.SetParent(canvas, false);
            Stretch(_mainMenu.GetComponent<RectTransform>());

            var backdrop = new GameObject("MenuBackdrop", typeof(RectTransform), typeof(Image));
            backdrop.transform.SetParent(_mainMenu.transform, false);
            Stretch(backdrop.GetComponent<RectTransform>());
            var bgImg = backdrop.GetComponent<Image>();
            bgImg.sprite = ProceduralBrandArt.MenuBackdrop(270, 480);
            bgImg.color = Color.white;
            bgImg.raycastTarget = true;

            // Light veil so panel text stays readable over the art
            var veil = new GameObject("Veil", typeof(RectTransform), typeof(Image));
            veil.transform.SetParent(_mainMenu.transform, false);
            Stretch(veil.GetComponent<RectTransform>());
            var veilImg = veil.GetComponent<Image>();
            veilImg.color = new Color(0.04f, 0.02f, 0.08f, 0.38f);
            veilImg.raycastTarget = false;

            // Three bright KTV cards logo
            var logo = new GameObject("Logo", typeof(RectTransform), typeof(Image));
            logo.transform.SetParent(_mainMenu.transform, false);
            var logoRt = logo.GetComponent<RectTransform>();
            logoRt.anchorMin = new Vector2(0.18f, 0.72f);
            logoRt.anchorMax = new Vector2(0.82f, 0.96f);
            logoRt.offsetMin = Vector2.zero;
            logoRt.offsetMax = Vector2.zero;
            var logoImg = logo.GetComponent<Image>();
            logoImg.sprite = ProceduralBrandArt.KtvLogo(384);
            logoImg.preserveAspect = true;
            logoImg.raycastTarget = false;

            var (panel, title, body) = ModalPanelFactory.CreateCardPanel(
                _mainMenu.transform, "Panel",
                new Vector2(0.1f, 0.08f), new Vector2(0.9f, 0.70f));
            title.text = _russian ? "КАРТОВАСИЯ" : "CHAOS SPELLWEAVER";
            // Short tagline — buttons take most of the panel
            body.text = _russian
                ? "Оффлайн roguelike · стихии · комбо · кузня"
                : "Offline roguelike · elements · combos · forge";
            UiFactory.Anchor(body.rectTransform, new Vector2(0.08f, 0.68f), new Vector2(0.92f, 0.78f));

            // Order top→bottom: Continue → New Run → Tutorial → Settings → Exit
            const float h = 0.10f;
            const float gap = 0.012f;
            float y = 0.56f; // top of first button

            void Place(string name, string label, Color col, System.Action action)
            {
                float y0 = y - h;
                ModalPanelFactory.CreateButton(panel.transform, name, label,
                    new Vector2(0.12f, y0), new Vector2(0.88f, y),
                    col, () => action());
                y = y0 - gap;
            }

            Place("BtnContinue",
                _russian ? "Продолжить" : "Continue",
                new Color(0.15f, 0.4f, 0.3f), ContinueRun);

            Place("BtnNew",
                _russian ? "Новый run" : "New Run",
                new Color(0.35f, 0.15f, 0.45f), StartNewRun);

            Place("BtnHelp",
                _russian ? "Туториал" : "Tutorial",
                new Color(0.25f, 0.3f, 0.45f), () =>
                {
                    TutorialOverlay.ResetTutorial();
                    TutorialOverlay.Instance?.ForceShow(_russian);
                });

            Place("BtnSettings",
                _russian ? "Настройки" : "Settings",
                new Color(0.28f, 0.28f, 0.38f), OpenSettings);

            Place("BtnQuit",
                _russian ? "Выход" : "Quit",
                new Color(0.35f, 0.15f, 0.15f), QuitApp);

            _mainMenu.SetActive(true);
        }

        private void BuildSettingsMenu(Transform canvas)
        {
            _settingsMenu = ModalPanelFactory.CreateDimRoot(canvas, "SettingsMenu", 0.78f);
            var (panel, title, body) = ModalPanelFactory.CreateCardPanel(
                _settingsMenu.transform, "S",
                new Vector2(0.1f, 0.22f), new Vector2(0.9f, 0.78f));
            title.text = _russian ? "НАСТРОЙКИ" : "SETTINGS";
            body.text = _russian
                ? "Громкость. Сохраняется автоматически."
                : "Volume. Saved automatically.";
            UiFactory.Anchor(body.rectTransform, new Vector2(0.08f, 0.72f), new Vector2(0.92f, 0.80f));

            // Music row
            _musicVolLabel = UiFactory.Text(panel.transform, "MusicLbl", "Музыка: 70%", 20,
                TextAnchor.MiddleCenter, TarotPalette.Parchment, FontStyle.Bold);
            UiFactory.Anchor(_musicVolLabel.rectTransform, new Vector2(0.1f, 0.52f), new Vector2(0.9f, 0.62f));

            ModalPanelFactory.CreateButton(panel.transform, "MusMinus", "−",
                new Vector2(0.12f, 0.40f), new Vector2(0.32f, 0.50f),
                new Color(0.3f, 0.2f, 0.35f), () => AdjustMusic(-0.1f));
            ModalPanelFactory.CreateButton(panel.transform, "MusPlus", "+",
                new Vector2(0.68f, 0.40f), new Vector2(0.88f, 0.50f),
                new Color(0.25f, 0.35f, 0.3f), () => AdjustMusic(+0.1f));
            ModalPanelFactory.CreateButton(panel.transform, "MusMute",
                _russian ? "Выкл" : "Mute",
                new Vector2(0.36f, 0.40f), new Vector2(0.64f, 0.50f),
                new Color(0.25f, 0.25f, 0.3f), () =>
                {
                    GameAudioSettings.SetMusicVolume(GameAudioSettings.MusicVolume > 0.05f ? 0f : 0.7f);
                    RefreshSettingsLabels();
                    ProceduralSfx.Instance?.PlayUi();
                });

            // SFX row
            _sfxVolLabel = UiFactory.Text(panel.transform, "SfxLbl", "Звуки: 80%", 20,
                TextAnchor.MiddleCenter, TarotPalette.Parchment, FontStyle.Bold);
            UiFactory.Anchor(_sfxVolLabel.rectTransform, new Vector2(0.1f, 0.28f), new Vector2(0.9f, 0.38f));

            ModalPanelFactory.CreateButton(panel.transform, "SfxMinus", "−",
                new Vector2(0.12f, 0.16f), new Vector2(0.32f, 0.26f),
                new Color(0.3f, 0.2f, 0.35f), () => AdjustSfx(-0.1f));
            ModalPanelFactory.CreateButton(panel.transform, "SfxPlus", "+",
                new Vector2(0.68f, 0.16f), new Vector2(0.88f, 0.26f),
                new Color(0.25f, 0.35f, 0.3f), () => AdjustSfx(+0.1f));
            ModalPanelFactory.CreateButton(panel.transform, "SfxMute",
                _russian ? "Выкл" : "Mute",
                new Vector2(0.36f, 0.16f), new Vector2(0.64f, 0.26f),
                new Color(0.25f, 0.25f, 0.3f), () =>
                {
                    GameAudioSettings.SetSfxVolume(GameAudioSettings.SfxVolume > 0.05f ? 0f : 0.8f);
                    RefreshSettingsLabels();
                    ProceduralSfx.Instance?.PlayUi();
                });

            ModalPanelFactory.CreateButton(panel.transform, "Back",
                _russian ? "Назад" : "Back",
                new Vector2(0.15f, 0.04f), new Vector2(0.85f, 0.13f),
                new Color(0.2f, 0.35f, 0.4f), CloseSettings);

            _settingsMenu.SetActive(false);
            RefreshSettingsLabels();
        }

        private static void QuitApp()
        {
#if UNITY_EDITOR
            UnityEditor.EditorApplication.isPlaying = false;
#else
            Application.Quit();
#endif
        }

        private void BuildPauseMenu(Transform canvas)
        {
            _pauseMenu = ModalPanelFactory.CreateDimRoot(canvas, "PauseMenu", 0.7f);
            var (panel, title, body) = ModalPanelFactory.CreateCardPanel(
                _pauseMenu.transform, "P",
                new Vector2(0.15f, 0.3f), new Vector2(0.85f, 0.7f));
            title.text = _russian ? "ПАУЗА" : "PAUSE";
            body.text = _russian ? "Run сохранён при выходе в меню." : "Run saves when exiting to menu.";

            ModalPanelFactory.CreateButton(panel.transform, "Resume",
                _russian ? "Продолжить" : "Resume",
                new Vector2(0.15f, 0.08f), new Vector2(0.85f, 0.2f),
                new Color(0.2f, 0.4f, 0.3f), TogglePause);

            ModalPanelFactory.CreateButton(panel.transform, "Save",
                _russian ? "Сохранить" : "Save",
                new Vector2(0.15f, 0.22f), new Vector2(0.85f, 0.34f),
                new Color(0.2f, 0.35f, 0.45f), () =>
                {
                    _combat?.SaveRun();
                    OnLog(_russian ? "Сохранено." : "Saved.");
                });

            ModalPanelFactory.CreateButton(panel.transform, "Menu",
                _russian ? "В меню" : "Main menu",
                new Vector2(0.15f, 0.36f), new Vector2(0.85f, 0.48f),
                new Color(0.4f, 0.2f, 0.2f), () =>
                {
                    _paused = false;
                    Time.timeScale = 1f;
                    _pauseMenu.SetActive(false);
                    ExitToMenu();
                });
        }

        private void BuildEventModal(Transform canvas)
        {
            _eventModal = ModalPanelFactory.CreateDimRoot(canvas, "EventModal", 0.75f);
            _eventModal.AddComponent<EventRollHolder>();
            var (panel, title, body) = ModalPanelFactory.CreateCardPanel(
                _eventModal.transform, "E",
                new Vector2(0.1f, 0.28f), new Vector2(0.9f, 0.72f));
            title.text = _russian ? "ГЛОБАЛЬНОЕ СОБЫТИЕ" : "GLOBAL EVENT";
            _eventBody = body;

            ModalPanelFactory.CreateButton(panel.transform, "Accept",
                _russian ? "Мини-игра" : "Mini-game",
                new Vector2(0.08f, 0.08f), new Vector2(0.48f, 0.2f),
                new Color(0.25f, 0.35f, 0.5f), ResolveEventAccept);

            ModalPanelFactory.CreateButton(panel.transform, "Skip",
                _russian ? "Пропустить" : "Skip",
                new Vector2(0.52f, 0.08f), new Vector2(0.92f, 0.2f),
                new Color(0.4f, 0.2f, 0.25f), ResolveEventSkip);
        }

        private void BuildCombatUi(Transform canvas)
        {
            _hudRoot = new GameObject("CombatHud", typeof(RectTransform));
            _hudRoot.transform.SetParent(canvas, false);
            Stretch(_hudRoot.GetComponent<RectTransform>());

            // ── Atmospheric battle backdrop (magic circle / ruins) ──
            var battleBg = new GameObject("BattleBg", typeof(RectTransform), typeof(Image));
            battleBg.transform.SetParent(_hudRoot.transform, false);
            Stretch(battleBg.GetComponent<RectTransform>());
            var bbg = battleBg.GetComponent<Image>();
            bbg.sprite = ProceduralBattleBg.Get();
            bbg.color = Color.white;
            bbg.raycastTarget = false;

            // ══════════════════════════════════════════════════════════
            // LAYOUT (portrait, bottom → top) — dark Tarot battler
            // Hand 0–0.18 | Player aristocrat 0.14–0.48 (center, large)
            // Side buttons / HP | Mid strip | Enemy 0.54–0.98 (large)
            // ══════════════════════════════════════════════════════════

            // ── ENEMY (top, large) ────────────────────────────────────
            var enemy = new GameObject("EnemyZone", typeof(RectTransform), typeof(Image), typeof(EnemyDropZone));
            enemy.transform.SetParent(_hudRoot.transform, false);
            var er = enemy.GetComponent<RectTransform>();
            er.anchorMin = new Vector2(0.04f, 0.54f);
            er.anchorMax = new Vector2(0.96f, 0.985f);
            er.offsetMin = Vector2.zero;
            er.offsetMax = Vector2.zero;
            var eimg = enemy.GetComponent<Image>();
            eimg.sprite = ProceduralSpriteFactory.RoundedRect(160, 120, 16,
                new Color(0.10f, 0.04f, 0.12f, 0.88f), TarotPalette.Gold, 5);
            eimg.color = Color.white;
            eimg.raycastTarget = true;

            _enemyPortrait = UiFactory.Image(enemy.transform, "Portrait",
                ProceduralSpriteFactory.RoundedRect(64, 64, 8, TarotPalette.Night, TarotPalette.Gold, 2),
                Color.white, false);
            UiFactory.Anchor(_enemyPortrait.rectTransform, new Vector2(0.12f, 0.22f), new Vector2(0.88f, 0.90f));
            _enemyPortrait.preserveAspect = true;
            _enemyPortrait.raycastTarget = false;

            _enemyTitle = UiFactory.Text(enemy.transform, "T", "Enemy", 26, TextAnchor.UpperCenter, TarotPalette.Parchment, FontStyle.Bold);
            UiFactory.Anchor(_enemyTitle.rectTransform, new Vector2(0.04f, 0.88f), new Vector2(0.96f, 0.99f));
            _enemyTitle.resizeTextForBestFit = true;
            _enemyTitle.resizeTextMinSize = 14;
            _enemyTitle.resizeTextMaxSize = 26;

            _enemyHp = UiFactory.Text(enemy.transform, "H", "HP", 18, TextAnchor.MiddleCenter, TarotPalette.Risk);
            UiFactory.Anchor(_enemyHp.rectTransform, new Vector2(0.06f, 0.10f), new Vector2(0.94f, 0.24f));
            _enemyHp.resizeTextForBestFit = true;
            _enemyHp.resizeTextMinSize = 12;
            _enemyHp.resizeTextMaxSize = 18;

            var castHint = UiFactory.Text(enemy.transform, "Hint",
                _russian ? "⬆ заклинание на врага" : "⬆ spell on enemy", 13, TextAnchor.LowerCenter, TarotPalette.Gold);
            UiFactory.Anchor(castHint.rectTransform, new Vector2(0.05f, 0.01f), new Vector2(0.95f, 0.10f));

            // ── MID HUD strip ─────────────────────────────────────────
            var strip = new GameObject("Strip", typeof(RectTransform), typeof(Image));
            strip.transform.SetParent(_hudRoot.transform, false);
            var sr = strip.GetComponent<RectTransform>();
            sr.anchorMin = new Vector2(0.05f, 0.485f);
            sr.anchorMax = new Vector2(0.95f, 0.535f);
            sr.offsetMin = Vector2.zero;
            sr.offsetMax = Vector2.zero;
            strip.GetComponent<Image>().sprite = ProceduralSpriteFactory.RoundedRect(128, 32, 8,
                new Color(0.06f, 0.04f, 0.1f, 0.92f), TarotPalette.Gold, 2);
            strip.GetComponent<Image>().color = Color.white;
            strip.GetComponent<Image>().raycastTarget = false;
            _hudText = UiFactory.Text(strip.transform, "Hud", "", 14, TextAnchor.MiddleCenter, TarotPalette.Parchment);
            Stretch(_hudText.rectTransform);
            _hudText.horizontalOverflow = HorizontalWrapMode.Wrap;
            _hudText.verticalOverflow = VerticalWrapMode.Truncate;
            _hudText.resizeTextForBestFit = true;
            _hudText.resizeTextMinSize = 10;
            _hudText.resizeTextMaxSize = 14;

            // Player HP / mats — just under strip
            _playerHp = UiFactory.Text(_hudRoot.transform, "PHP", "", 15, TextAnchor.MiddleLeft, new Color(0.5f, 0.9f, 0.6f));
            UiFactory.Anchor(_playerHp.rectTransform, new Vector2(0.04f, 0.445f), new Vector2(0.48f, 0.482f));

            _matHud = UiFactory.Text(_hudRoot.transform, "Mats", "", 13, TextAnchor.MiddleRight, TarotPalette.Gold);
            UiFactory.Anchor(_matHud.rectTransform, new Vector2(0.48f, 0.445f), new Vector2(0.96f, 0.482f));

            _logText = UiFactory.Text(_hudRoot.transform, "Log", "", 12, TextAnchor.UpperLeft, new Color(0.7f, 0.75f, 0.85f));
            UiFactory.Anchor(_logText.rectTransform, new Vector2(0.04f, 0.40f), new Vector2(0.96f, 0.442f));

            // Side action buttons (don't cover center aristocrat)
            ModalPanelFactory.CreateButton(_hudRoot.transform, "BtnEnd",
                _russian ? "Ход →" : "End",
                new Vector2(0.78f, 0.40f), new Vector2(0.98f, 0.44f),
                new Color(0.25f, 0.35f, 0.55f), () => _combat?.EndPlayerTurn());

            ModalPanelFactory.CreateButton(_hudRoot.transform, "BtnCraft",
                _russian ? "Кузня" : "Forge",
                new Vector2(0.78f, 0.355f), new Vector2(0.98f, 0.395f),
                new Color(0.4f, 0.25f, 0.45f), OpenCraft);

            ModalPanelFactory.CreateButton(_hudRoot.transform, "BtnPause",
                _russian ? "Ⅱ" : "II",
                new Vector2(0.02f, 0.40f), new Vector2(0.20f, 0.44f),
                new Color(0.3f, 0.3f, 0.35f), TogglePause);

            ModalPanelFactory.CreateButton(_hudRoot.transform, "BtnExit",
                _russian ? "Меню" : "Menu",
                new Vector2(0.02f, 0.355f), new Vector2(0.20f, 0.395f),
                new Color(0.45f, 0.2f, 0.2f), ExitToMenu);

            // ── PLAYER ARISTOCRAT (bottom center, large) ──────────────
            // Drawn before hand so cards sit in front
            var playerAvatar = PlayerCharacterView.Build(_hudRoot.transform);
            playerAvatar.SetSprites(
                LoadCharacterSprite("Player_Idle"),
                LoadCharacterSprite("Player_Cast"));
            var nameT = playerAvatar.transform.Find("Name");
            if (nameT != null)
            {
                var nt = nameT.GetComponent<Text>();
                if (nt != null)
                    nt.text = _russian ? "АРИСТОКРАТ" : "ARISTOCRAT";
            }

            // Buff drop zone = body of the aristocrat (self-target)
            var buffZone = new GameObject("BuffZone", typeof(RectTransform), typeof(Image), typeof(PlayerDropZone));
            buffZone.transform.SetParent(_hudRoot.transform, false);
            var bz = buffZone.GetComponent<RectTransform>();
            bz.anchorMin = new Vector2(0.22f, 0.16f);
            bz.anchorMax = new Vector2(0.78f, 0.46f);
            bz.offsetMin = Vector2.zero;
            bz.offsetMax = Vector2.zero;
            var bImg = buffZone.GetComponent<Image>();
            bImg.color = new Color(0.1f, 0.35f, 0.18f, 0.12f); // subtle — character visible under
            bImg.raycastTarget = true;
            var buffHint = UiFactory.Text(buffZone.transform, "BuffHint",
                _russian ? "✦ бафф → на себя" : "✦ buff → self",
                13, TextAnchor.UpperCenter, new Color(0.85f, 0.75f, 0.4f, 0.85f));
            UiFactory.Anchor(buffHint.rectTransform, new Vector2(0.1f, 0.88f), new Vector2(0.9f, 1f));
            buffHint.raycastTarget = false;

            // ── HAND (very bottom, over character feet) ───────────────
            var hand = new GameObject("HandZone", typeof(RectTransform), typeof(Image));
            hand.transform.SetParent(_hudRoot.transform, false);
            _handZone = hand.GetComponent<RectTransform>();
            _handZone.anchorMin = new Vector2(0f, 0f);
            _handZone.anchorMax = new Vector2(1f, 0.20f);
            _handZone.offsetMin = Vector2.zero;
            _handZone.offsetMax = Vector2.zero;
            hand.GetComponent<Image>().color = new Color(0.04f, 0.03f, 0.08f, 0.35f);
            hand.GetComponent<Image>().raycastTarget = false;

            _hudRoot.SetActive(false);
        }

        private static Sprite LoadCharacterSprite(string fileNameNoExt)
        {
            // Resources/Characters first (build-friendly)
            var fromRes = Resources.Load<Sprite>("Characters/" + fileNameNoExt);
            if (fromRes != null)
                return fromRes;

            var texRes = Resources.Load<Texture2D>("Characters/" + fileNameNoExt);
            if (texRes != null)
            {
                return Sprite.Create(texRes, new Rect(0, 0, texRes.width, texRes.height),
                    new Vector2(0.5f, 0f), 100f);
            }

            // Editor / dev: load PNG from Art folder on disk
            string path = Path.Combine(Application.dataPath, "Kartovasiya/Art/Characters", fileNameNoExt + ".png");
            if (!File.Exists(path))
                return null;

            try
            {
                byte[] bytes = File.ReadAllBytes(path);
                var tex = new Texture2D(2, 2, TextureFormat.RGBA32, false);
                if (!tex.LoadImage(bytes))
                    return null;
                tex.filterMode = FilterMode.Bilinear;
                tex.wrapMode = TextureWrapMode.Clamp;
                return Sprite.Create(tex, new Rect(0, 0, tex.width, tex.height),
                    new Vector2(0.5f, 0f), 100f);
            }
            catch
            {
                return null;
            }
        }

        private static void BuildBackground(Transform canvas)
        {
            var go = new GameObject("BG", typeof(RectTransform), typeof(Image));
            go.transform.SetParent(canvas, false);
            go.transform.SetAsFirstSibling();
            Stretch(go.GetComponent<RectTransform>());
            var img = go.GetComponent<Image>();
            // Deep night base; combat layer adds magic circle on top
            img.sprite = ProceduralSpriteFactory.Vignette(64, 64, new Color(0.08f, 0.04f, 0.12f), TarotPalette.Night);
            img.raycastTarget = false;
        }

        private static Canvas CreateCanvas()
        {
            var go = new GameObject("GameCanvas", typeof(Canvas), typeof(CanvasScaler), typeof(GraphicRaycaster));
            var c = go.GetComponent<Canvas>();
            c.renderMode = RenderMode.ScreenSpaceOverlay;
            c.sortingOrder = 20;
            var s = go.GetComponent<CanvasScaler>();
            s.uiScaleMode = CanvasScaler.ScaleMode.ScaleWithScreenSize;
            s.referenceResolution = new Vector2(1080, 1920);
            s.matchWidthOrHeight = 0.55f;
            return c;
        }

        private static void EnsureEventSystem()
        {
            if (FindAnyObjectByType<EventSystem>() != null) return;
            var es = new GameObject("EventSystem");
            es.AddComponent<EventSystem>();
#if ENABLE_INPUT_SYSTEM
            es.AddComponent<UnityEngine.InputSystem.UI.InputSystemUIInputModule>();
#else
            es.AddComponent<StandaloneInputModule>();
#endif
        }

        private static void EnsureOrientation()
        {
            if (FindAnyObjectByType<OrientationService>() != null) return;
            new GameObject("Orientation").AddComponent<OrientationService>();
        }

        private static void Stretch(RectTransform r)
        {
            r.anchorMin = Vector2.zero;
            r.anchorMax = Vector2.one;
            r.offsetMin = Vector2.zero;
            r.offsetMax = Vector2.zero;
        }

        private class EventRollHolder : MonoBehaviour
        {
            public int Roll;
        }
    }
}
