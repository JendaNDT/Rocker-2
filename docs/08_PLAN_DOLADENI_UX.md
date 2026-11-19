# Rocker 2 · čtyři vylepšení při nahrávání

Plán navazuje na [redesign 1b](07_PLAN_REDESIGNU_1B.md) ve verzi `375ed0f` z [PR č. 22](https://github.com/JendaNDT/Rocker-2/pull/22). Popisuje práci ve skladbě. Změny a výsledky zkoušek všech čtyř kroků jsou v [PR č. 23](https://github.com/JendaNDT/Rocker-2/pull/23).

## 1. Po zastavení si poslechnout záběr nebo nahrát další

Po stisku Stop zůstane skladba otevřená na stejné stopě. Jakmile jsou zvuk i změna skladby bezpečně zapsané, objeví se například **Kytara · pokus 3 · uloženo**. Pod zprávou budou dvě akce dostupné také z klávesnice.

- **Poslechnout pokus** přehraje záběr jedním stiskem, od jeho skutečného začátku do konce, společně s doprovodem. Respektuje mix skladby. U nahrávání dokola přehraje poslední úplný průchod. Odpočet před nahráváním se do poslechu nezapočítá.
- **Nahrát další** začne ze stejného hudebního místa se stejnými stopami a nastaveným odpočtem. Při opravě zachová vybraný úsek. U volného hraní může být druhý záběr jinak dlouhý. Předchozí pokus půjde znovu vybrat ve Stříhej a při přehrání zazní jen zvolená varianta. Běžné tlačítko Nahrát dál slouží také k přihrávání dalších stop.
- Při společném záznamu více vstupů patří všechny nahrané stopy k jednomu pokusu. Při zastavení v odpočtu nevznikne prázdný pokus. Při plném disku, výpadku nebo chybě zápisu má přednost skutečný výsledek a případná možnost obnovy.

**Vazba na kód.** V `src/ui/ProjectPage.cpp` už Stop dokončuje zápis a volá `recoverTake`. Z výsledku zápisu v `src/engine/Timeline.cpp` získat totožnost stop, pokusu a skutečné hranice záběru. Pro opakování využít skupiny pokusů ze `src/song/Song.h` a výběr pokusu v `src/ui/EditPage.cpp`. Samostatný součet uložených záběrů nestačí k výběru správné nahrávky. Po změně skladby nebo vrácení záběru přes Zpět znovu ověřit, že cíl tlačítek pořád existuje.

**Důkaz.** Do dvou pokusů nahrát různé známé signály. Ověřit první a poslední přehrávaný vzorek, správný doprovod a přepnutí na původní pokus i po znovuotevření skladby. Zopakovat s více stopami, smyčkou a rozpracovaným posledním průchodem. Při vynucené chybě uložení nesmí vzniknout potvrzení úspěchu.

## 2. Přepnout Hraj → Stříhej → Míchej na stejném místě

Když kytarista při poslechu v 17. taktu otevře Stříhej, uvidí 17. takt a tutéž stopu. V Míchej bude vybraná odpovídající stopa mixu. Přepnutí režimu zachová přehrávání, označený úsek i přiblížení. Pokud jinak široký panel zakryje kurzor, osa se posune jen o tolik, aby byl znovu vidět.

**Vazba na kód.** Využít společný `ViewState` a skutečnou přehrávanou polohu z `Studio::playPosition()`. Projít přechody `editMode` a `mixMode` i návrat boční lištou. Za přehrávání se uložený kurzor může lišit od taktu, který muzikant právě slyší. Při obnovení pohledu ověřit vybranou stopu podle její identity, zejména po změně pořadí stop.

**Důkaz.** Projít všechny dvojice režimů v obou směrech myší i zkratkami 1–3, v klidu a během přehrávání. Ověřit stejnou stopu, výběr taktů, přiblížení, viditelnost přehrávaného místa a souvislý zvuk. Zopakovat s úzkým oknem, se sbaleným panelem mixu a při 150% zvětšení.

## 3. Vyřešit dvojí odposlech přímo u vstupu

U volby odposlechu otevře **Slyšíš kytaru dvakrát?** krátkou nápovědu. Mohla by znít takhle. „Zvukovka ti může posílat kytaru přímo do sluchátek. Když ji slyšíš i přes Rocker, vypni odposlech v aplikaci.“ Tlačítko **Vypnout odposlech v aplikaci** ztiší živou kytaru z Rockeru. Doprovod dál hraje, vstupní měřič reaguje a záznam pokračuje. Na stejném místě půjde původní odposlech zase zapnout.

**Vazba na kód.** Navázat na volbu v `src/ui/Workspace.cpp` a na `Studio::configureSongMonitor()`. Zpřístupnit ovládání i během záznamu. Prověřit zvlášť běžný živý vstup a odposlech při opravě úseku, který má v `src/engine/Timeline.cpp` vlastní cestu. Změnu předat běžícímu zvuku bezpečně bez restartu zařízení. Rocker nemůže spolehlivě zjistit polohu ovladače Direct Monitor na zvukovce.

**Důkaz.** Do vstupu a doprovodu poslat dva rozlišitelné signály. Po vypnutí musí z výstupu zmizet jen živý signál. Porovnat uloženou stopu, nepřerušený doprovod a stav měřiče. Zopakovat pro čistý odposlech, aparát i opravu úseku. Při již vypnutém odposlechu nápověda odkáže na nastavení zvukovky.

## 4. Ukázat přímo u stopy, že její vstup zmizel

U připojené kytary v tichu prostě klesne měřič. Při ztrátě přiřazeného vstupu se na řádku stopy objeví například **Vstup 1 odpojen** a **Vybrat vstup**. Hudebník zůstane ve své skladbě. Při výměně zařízení si sám zvolí, kam stopu znovu zapojit. Mikrofon notebooku je výslovná možnost. Pokud k odpojení dojde během nahrávání, zobrazí se také výsledek záchrany rozehraného záběru z kroku 1.

**Vazba na kód.** `Studio::refresh` dnes po upozornění na změnu zařízení znovu hledá zvukovky a při otevřené skladbě přechází do Zapojení. Porovnat skutečnou dostupnost původního zařízení a kanálů pomocí `Connection` v `src/engine/Devices.h`. Stejné jméno „Vstup 1“ na jiné zvukovce nesmí samo obnovit přiřazení. Kde ovladač nedovolí spolehlivě rozpoznat původní zařízení, nabídnout výběr. Při chybějící polovině stereo páru označit konkrétní kanál. Výběr náhrady zachová přiřazení ostatních stop.

**Důkaz.** Pomocí `tests/FakeDevice.h` nasimulovat ticho, ztrátu jednoho kanálu, odpojení celé zvukovky během záznamu, návrat stejné zvukovky a připojení jiné se stejně pojmenovanými vstupy. Ověřit nabídku dostupných vstupů, zachování zaznamenané části podle výsledku zápisu a nové nahrávání až po platném přiřazení. Zachovat náhradní režim při prvním spuštění a stávající zkoušky průvodce Zapojení.

## Ověření a předání

1. Každý krok zahájit reprodukcí popsané situace na ověřeném redesignu. Chování, které už funguje, zachytit testem. Pro chybějící chování nejdřív doložit selhání, potom výsledek po opravě.
2. Rozšířit zkoušky skutečných tlačítek v `tests/StudioTests.cpp` a příslušné zkoušky zvuku a ukládání. České texty patří do `Strings.h`, rozměry a barvy do `Tokens.h`. Zvukové vlákno musí zůstat bez zámků, alokací a zápisu souborů. Při případném rozšíření datového formátu ověřit i otevření dosavadní skladby.
3. V českém PR doložit celé CI pro Windows x64, Windows ARM64 a Mac. Přiložit balíky a snímky stavu po záběru, přechodu na 17. taktu, nápovědy k odposlechu a odpojeného vstupu. Ověřit rozložení v 1280 × 720 i 1920 × 1080 při 150 % a ovládání klávesnicí.
4. **Po vydání** Jenda zahraje stejný riff dvakrát jako dva pokusy, poslechne si první, při poslechu přepne režim, porovná přímý odposlech s Rockerem a odpojí zvukovku během krátkého záběru. Poslech na skutečné sestavě a hraní celé písně čekají na tuto společnou zkoušku.
