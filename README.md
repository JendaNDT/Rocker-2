# Rocker 2

Domácí studio pro kytaru a bicí. V etapě E1 můžeš hrát do sluchátek přes kytarový aparát, naladit struny a změřit zpoždění svojí zvukovky. Aplikace běží bez internetu a účtu. Nahrávání bicích a kytary přijde v E2.

## Stažení a spuštění

1. Na GitHubu otevři poslední zelený běh **Rocker 2** pro větev **etapa-1**. Dole najdeš **Artifacts**. Pro stažení z GitHub Actions se přihlas ke GitHubu. [Běhy pro E1](https://github.com/JendaNDT/Rocker-2/actions/workflows/ci.yml?query=branch%3Aetapa-1)
2. Ve Windows s procesorem Intel nebo AMD stáhni **Rocker2-Windows-x64**. Pro Snapdragon nebo jiný procesor Windows na ARM vyber **Rocker2-Windows-ARM64**. Typ najdeš v **Nastavení → Systém → O systému → Typ systému**.
3. Rozbal stažený ZIP. Pokud je uvnitř další ZIP, rozbal i ten. Spusť **Rocker 2.exe**. Když se u tohoto sestavení objeví SmartScreen, klikni na **Další informace → Přesto spustit**.
4. Na Macu s čipem Apple stáhni **Rocker2-Mac**, rozbal balík a přesuň **Rocker 2.app** do Aplikací. Spusť ji a povol vstup z mikrofonu, aby fungovaly měřiče. Pokud macOS zastaví první otevření stažené aplikace, po pokusu o spuštění otevři **Nastavení systému → Soukromí a zabezpečení → Přesto otevřít** a potvrď otevření.
5. Při prvním startu se otevře **Zapojení**. Tlačítko **Úvodní obrazovka** tě vezme na úvod podle náčrtu 01 / 5.7. Odtud se do průvodce kdykoli vrátíš přes **Zapojení a zvukovka**.

## První riff a Drop D

1. Zapoj kytaru do vstupu **INST / Hi-Z** a drátová sluchátka do téže zvukovky. V průvodci dokonči zkoušku sluchátek a podle brnknutí pojmenuj vstup. Na zvukovce přepni přímý odposlech na **USB / Playback**, aby ses neslyšel dvakrát.
2. Na úvodu stiskni **Hraj**. Vyber **Přímo do zvukovky**, vstup kytary a **Odposlech → Se zvukem**. Vpravo uvidíš, co jde do sluchátek a jaké má cesta zpoždění. Bez kalibrace se zobrazuje odhad ovladače, pokud ho zvukovka poskytne.
3. Ve **Zvuk kytary** vyber **Crunch** a zahraj. Stiskni **Změř ticho** a dvě sekundy nehraj. Vyzkoušej zkreslení, basy, středy, výšky, jas a hlasitost. V Hraj můžeš přepnout na **Čistě** nebo **Vypnuto**.
4. Pokud snímáš kombo mikrofonem nebo linkou, vyber odpovídající zdroj. Při zapojení čisté kytary i komba přiřaď dva různé vstupy a zvol, který chceš poslouchat. U komba můžeš ponechat odposlech vypnutý a poslouchat ho z místnosti.
5. Stiskni **T**, vyber **Drop D** a brnkni na nejhlubší strunu. Cílem je **D2** uprostřed stupnice. Po ustálení zazní krátké potvrzení. Podržením **Referenční tón** si cílovou strunu poslechneš. Zkus také **Drop B** nebo ulož vlastní ladění zadáním tónů od nejhlubší struny. Po návratu z ladičky zapni odposlech pro další hraní.

## M-01 · změř zpoždění kabelem

1. V **Zapojení** vyber zvukovku, pro první měření ASIO při 48 kHz a dostupný buffer. Stáhni hlasitost linkového výstupu na minimum, vypni **DIRECT MONITOR** a 48V napájení na měřeném vstupu. Odpoj kytaru. Propoj **levý linkový výstup L → linkový vstup 1**. Použij linkový výstup zvukovky. Reproduktorový výstup kytarového zesilovače sem nepatří.
2. V **Diagnostika** vyber vstup kabelu. Stiskni **Změřit zpoždění**. Pokud se signál nevrátí, zkontroluj vstup, pomalu přidej úroveň výstupu a měření opakuj. Během propojení nech přímý odposlech vypnutý.
3. Každé nastavení změř **pětkrát**. Zapiš pět hodnot ve vzorcích, odhad ovladače, rozdíl a změřené ms. Cílem je opakovatelnost ±1 vzorek. Zopakuj pro dostupné **ASIO 64, 128 a 256** a pro režimy **Windows Audio exkluzivní**, **s nízkou latencí** a **sdílený**. Na Macu použij CoreAudio. Chybějící režim stačí označit jako nedostupný.
4. Stáhni výstup, **odpoj smyčkový kabel**, vrať kytaru a sluchátka. Stejné zařízení, frekvence a buffer mají v Diagnostice ukázat poslední úspěšné měření i po novém spuštění.

## H-01 · vyber buffer podle hraní

Jendo, nech kamaráda zahrát stejný šestnáctinový riff přes **Crunch** s externím metronomem. Mezi pokusy změň pořadí dostupných bufferů 64, 128 a 256 tak, aby hráč nevěděl, který poslouchá. Po změně ověř vstup a znovu zapni **Se zvukem**. Ke každému pokusu si před prozrazením nastavení vyžádej známku 1–5. **5** znamená „žádné zpoždění necítím“. Zapiš i praskání nebo dvojitý zvuk a hodnotu z panelu **Co slyším**.

Při zpoždění do 10 ms má být hodnocení alespoň 4. Z vyhovujících výsledků vybereme největší buffer s hodnocením 4 nebo 5. Stačí mi poslat údaje zprávou ve tvaru níže, zapíšu je za tebe.

| Počítač a zvukovka | Ovladač, kHz, buffer | Pět měření M-01 ve vzorcích | Odhad / rozdíl ve vzorcích | Změřeno v ms / s aparátem v ms | Známka H-01 a co bylo slyšet |
|---|---|---|---|---|---|
| | | | | | |

## Kontrolní seznam pro první zapojení

- [ ] Porovnej úvodní obrazovku s 01 / 5.7. Zkus **R**, **N**, **Enter** a **T**. Klávesa T otevře ladičku a dalším stiskem tě vrátí. R, N a Enter zatím ukážou etapu připravované funkce.
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
