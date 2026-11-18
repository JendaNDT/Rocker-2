# Rocker 2

Domácí studio pro kytaru a bicí. V etapě E0 si můžeš vyzkoušet zvukovku a zjistit, který vstup patří ke kterému nástroji. Aplikace běží bez internetu a účtu. Nahrávání přijde v další části vývoje.

## Stažení a spuštění

1. Na GitHubu otevři poslední zelený běh **Rocker 2** pro větev **etapa-0**. Dole najdeš **Artifacts**. Pro stažení z GitHub Actions se přihlas ke GitHubu. [Běhy pro E0](https://github.com/JendaNDT/Rocker-2/actions/workflows/ci.yml?query=branch%3Aetapa-0)
2. Ve Windows s procesorem Intel nebo AMD stáhni **Rocker2-Windows-x64**. Pro Snapdragon nebo jiný procesor Windows na ARM vyber **Rocker2-Windows-ARM64**. Typ najdeš v **Nastavení → Systém → O systému → Typ systému**.
3. Rozbal stažený ZIP. Pokud je uvnitř další ZIP, rozbal i ten. Spusť **Rocker 2.exe**. Když se u tohoto sestavení objeví SmartScreen, klikni na **Další informace → Přesto spustit**.
4. Na Macu s čipem Apple stáhni **Rocker2-Mac**, rozbal balík a přesuň **Rocker 2.app** do Aplikací. Spusť ji a povol vstup z mikrofonu, aby fungovaly měřiče. Pokud macOS zastaví první otevření stažené aplikace, po pokusu o spuštění otevři **Nastavení systému → Soukromí a zabezpečení → Přesto otevřít** a potvrď otevření.
5. Při prvním startu se otevře **Zapojení**. Tlačítko **Úvodní obrazovka** tě vezme na úvod podle náčrtu 01 / 5.7. Odtud se do průvodce kdykoli vrátíš přes **Zapojení a zvukovka**.

## Co vyzkoušet se sluchátky a kytarou

- [ ] Porovnej úvodní obrazovku s 01 / 5.7. Zkus **R**, **N**, **Enter** a **T**. Každá volba vypíše, ve které etapě přibude.
- [ ] Ve Windows připoj zvukovku a v Zapojení vyber **ASIO** a ovladač výrobce. Bez ASIO vyber dostupný režim Windows Audio. Na Macu vyber **CoreAudio** a vstup i výstup své zvukovky.
- [ ] Zvol **48 kHz**, pokud je v nabídce. Jiné podporované frekvence můžeš vyzkoušet také. Vyber buffer a pokračuj k poslechu.
- [ ] Připoj drátová sluchátka ke zvukovce, začni s nízkou hlasitostí a stiskni **Pustit zkušební tón**. Se stereo výstupem zní první sekunda vlevo, druhá vpravo a třetí v obou. Vyber odpověď, která odpovídá tomu, co slyšíš.
- [ ] V kroku **Vstupy** připoj kytaru do vstupu 1 (INST nebo Hi-Z). Brnkni. Pokud do ostatních vstupů nejde signál, má reagovat jen první měřič. Stiskni **Najít vstup kytary** a znovu brnkni. Pokud se ozve jediný dosud nepojmenovaný vstup, dostane jméno **Kytara**. Jméno můžeš také napsat přímo do řádku. Máš-li bicí, pojmenuj i jejich vstup. U větší zvukovky můžeš seznam posouvat.
- [ ] Na zvukovce krátce přidej GAIN a silně brnkni. Při přebuzení zůstane u příslušného vstupu **PŘEBUZENO**. Stáhni GAIN a klikni na značku. Otevři aplikaci znovu a ověř, že si pamatuje nastavení a jména.
- [ ] Na Macu zopakuj se svojí zvukovkou stejný poslech, brnknutí a zkoušku přebuzení.
- [ ] Volitelně odpoj externí zvukovku a vyber vestavěný mikrofon. U méně obvyklého názvu použij **Používám mikrofon notebooku**. Zobrazí se omezení pro hraní přes aparát. Bez jediného zařízení můžeš průvodce projít tlačítkem **Přeskočit**.
- [ ] Z téhož běhu stáhni **Snimky-Windows-x64**, **Snimky-Windows-ARM64** nebo **Snimky-Mac**. Porovnej úvod a tři kroky Zapojení s 01 / 5.7 a 4.1. Pošli mi, co nesedí, s názvem zvukovky, systémem a zvoleným ovladačem. Stačí napsat zprávu.

[Stav zkoušek](docs/STAV.md) · [Předání pro další etapu](docs/PREDANI.md) · [Rozhodnutí](docs/ROZHODNUTI.md)

## Licence a sestavení pro vývojáře

C++20, CMake 3.25 nebo novější, JUCE 9.0.5 a Catch2 v3. Při vývoji se závislosti stáhnou při konfiguraci. Sestavení a spuštění zkoušek přesně popisuje [workflow](.github/workflows/ci.yml). Licence projektu je [AGPLv3](LICENSE). Písma pocházejí z vydání [Inter 4.1](https://github.com/rsms/inter/releases/tag/v4.1) a [JetBrains Mono 2.304](https://github.com/JetBrains/JetBrainsMono/releases/tag/v2.304). Obě licence OFL jsou ve [složce písem](assets/fonts).
