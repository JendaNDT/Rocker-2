# Rocker 2 — zkoušky na Windows

**Dokument 03 z 5.** Krátké praktické zkoušky s očekávaným výsledkem. Zkoušky se dělí na tři skupiny:

| Skupina | Kdo a kde | Jak poznám výsledek |
|---|---|---|
| **A – automatické** | Spouští je CI (GitHub Actions, Windows i macOS) bez zvukové karty, nebo implementující model lokálně. | Test projde nebo neprojde. Implementující model je píše spolu s kódem. |
| **M – měření na počítači** | Windows notebook se zvukovkou, často se **smyčkovým kabelem** (výstup → vstup). Spouští Jenda nebo kamarád podle návodu. Většinu měření dělá Rocker 2 sám v **Nápověda → Diagnostika → Zkoušky**. | Číslo, které Rocker 2 vypíše, porovnané s očekáváním. |
| **H – skutečné hraní** | Kamarád s kytarou a bicími, Jenda zapisuje. | Kombinace pozorování (sedí / nesedí) a hodnocení 1–5. |

Odkazy „01 / 4.3“ znamenají dokument 01, kapitolu 4.3. Etapa (E0–E11) odkazuje na dokument 04.

## Pomůcky

- **Smyčkový kabel:** kabel jack 6,3 mm – jack 6,3 mm, případně s redukcí z 3,5 mm. Vede z výstupu zvukovky (LINE OUT nebo výstup na sluchátka) do vstupu 1. **Před zapojením stáhněte výstup na minimum**, pak ho opatrně zvyšujte, dokud měřič vstupu neukazuje asi −12 dBFS.
- Drátová sluchátka, kytara, bicí, kamarádův notebook a zvukovka.
- Pro zkoušku M-10 USB flash disk s asi 1 GB volného místa.

## Záznam výsledku

Každou zkoušku M a H zapište do souboru `docs/zkousky/RRRR-MM-DD.md` v repozitáři (nebo ji pošlete implementujícímu modelu do chatu) v tomto tvaru:

```
Zkouška: M-02            Datum: 2026-12-03       Verze Rockeru 2: 0.3.0 (E2)
Notebook: …              Windows: 11 24H2        Zvukovka: …
Ovladač: ASIO, 48 kHz, buffer 128
Výsledek: posun na začátku +0,2 ms, na konci +0,2 ms → PROŠLO
Poznámka: …
```

---

## A. Automatické zkoušky

| ID | Co ověřuje | Postup (stručně) | Očekávaný výsledek | Etapa |
|---|---|---|---|---|
| **A-01** | Úpravy a Zpět | 1 000 náhodných úprav, pak vše Zpět, pak vše Znovu. | Po Zpět je skladba shodná s výchozí, po Znovu s konečnou (porovnání JSON). | E4 |
| **A-02** | Zkrácení druhého výskytu refrénu | Skladba Sloka 8 + Refrén 8 + Refrén 8 (spojený), zkrácení třetího výskytu na 6 taktů. | Druhý výskyt (první refrén) má dál 8 taktů. Vše za třetím výskytem se posune o přesně 2 takty ve vzorcích. Obnova délky nebo Zpět vrátí shodný stav. Zvukové soubory se nezměnily (kontrolní součet). | E6 |
| **A-03** | Uložení a načtení skladby | Uložit, načíst a porovnat. Zahrnuje diakritiku, starší `schemaVersion` (migrace) a poškozený JSON. | Shoda po načtení. Starší verze se převede. Poškozený soubor nespadne a nabídne poslední verzi z `Verze/`. | E2 |
| **A-04** | Mřížka taktů bez ujíždění | Falešné zařízení, 112 BPM, 10 min. Jádro vloží impuls na začátek každého taktu, zachytí výstup a porovná s výpočtem. | Každý impuls na očekávaném vzorku, odchylka **0 vzorků** i v posledním taktu. | E2 |
| **A-05** | Export = poslech | Stejná skladba (2 kytary s aparátem, bicí, dozvuk) zpracovaná cestou reálného času s blokem 64, 128, 512 a 1024 a offline exportem. Export se opakuje dvakrát. | Rozdíl libovolných dvou výstupů ≤ −120 dBFS. Dva exporty jsou **bitově shodné**. | E3, E7 |
| **A-06** | Klik se nepřimíchá do výsledku | Export se zapnutým a vypnutým klikem a odpočtem. | Oba soubory **bitově shodné**. | E2, E7 |
| **A-07** | Kompenzace zpoždění (simulace) | Falešné zařízení se zpožděním 211 vzorků na vstupu a 173 na výstupu a se smyčkou výstup → vstup. Nahrávání přes stopu s impulsy 10 min. | Nahrané impulsy leží na impulsech přehrávané stopy **±1 vzorek**, na začátku i na konci. | E2 |
| **A-08** | Záchrana po pádu | Podproces nahrává falešný vstup (rostoucí čísla vzorků), po 20 s ho test násilně ukončí (`TerminateProcess`). Pak se spustí obnova. | Obnoveno ≥ 19 s, soubor je čitelný a vzorky souvislé. Klip leží na původní poloze. Deník po potvrzení zmizí. | E2 |
| **A-09** | Atomické ukládání | 1 000× ukončení procesu v náhodné chvíli během ukládání. | `skladba.json` je vždy čitelný, buď starý, nebo nový, nikdy rozepsaný. | E2 |
| **A-10** | Diakritika a dlouhé cesty | Skladba „Noční riff – ř ů ě“ ve složce uživatele s diakritikou. Uložit, načíst, zabalit, rozbalit. | Vše funguje, názvy se zachovají. | E11 |
| **A-11** | Zabalit a rozbalit skladbu | Zabalit, rozbalit jinam, porovnat. | Shodný obsah (kontrolní součty), skladba se otevře a hraje. | E11 |
| **A-12** | Export MP3 | Na Windows: export MP3 320 kb/s přes Media Foundation a zpětné dekódování. | Soubor se dekóduje a délka sedí ±1 rámec. Pokud obraz CI kodér nemá (Windows Server může vyžadovat doplněk Media Foundation), test se **přeskočí s hlášením** a ověří se v M-13. | E7 |
| **A-13** | Dochází místo (simulace) | Funkce „volné místo“ podstrčená testem klesá během nahrávání. | Při hodnotě pro 30 minut nahrávání přijde varování. Pod 500 MB nastane řízené zastavení. Všechny vzorky do zastavení jsou v souboru. | E11 |
| **A-14** | Zaseknutý disk | Simulované zablokování zápisu na 5 s a na 15 s. | Při 5 s se nic neztratí. Při 15 s vznikne událost „výpadek zápisu“ se značkou na časové ose. Nikdy se nic neztratí potichu. | E2 |
| **A-15** | Pravidla zvukového vlákna | Ladicí sestavení zachytává alokace a zámky ve zpětném volání. 10 min přehrávání a nahrávání. Na macOS ThreadSanitizer. | 0 alokací a 0 zámků ve zvukovém vlákně, žádné souběhové chyby. | E2 a dál |
| **A-16** | Ladička | Syntetické tóny E2…E4, B1 (Drop B), tón s harmonickými a se šumem. | Odchylka ±1 cent u čistého tónu, ±3 centy se šumem. Žádná chyba o oktávu u B1. | E1 |
| **A-17** | Vzhled se vejde | Vykreslení hlavních obrazovek při 1280×720 a 1920×1080 se zvětšením 150 %. | Žádný text nepřetéká z prvku (automatická kontrola rozměrů). Obrázky PNG jsou artefakty CI pro Jendovu kontrolu. | každá |
| **A-18** | Pojistka | Falešný vstup 3 min, pak *Ulož, co jsem hrál*. | Uloženo přesně posledních 2:00, souvisle. | E8 |
| **A-19** | Import | WAV 44,1 kHz, FLAC, OGG, Opus a MP3. | Převod na 48 kHz, délka zachovaná ±1 vzorek. Výška tónu testovacího sinu je beze změny. | E8 |
| **A-20** | Aparát je opakovatelný | Stejný vstup dvakrát a přepnutí Crunch → Metal → Crunch. | Výstupy bitově shodné. | E1, E3 |
| **A-21** | Plynulé prolnutí pokusů | Oprava taktu uprostřed sinusovky a měření skoku signálu na hranách. | Žádný skok nad práh slyšitelného cvaknutí (detektor nespojitosti). | E5 |
| **A-22** | Načasování MIDI úderů | Falešný vstup MIDI posílá údery s časovými značkami přesně na doby, falešné zařízení má výstupní latenci 173 vzorků. | Po kompenzaci leží nahrané údery na dobách **±1 vzorek**, i po 10 minutách. Po násilném ukončení během nahrávání se obnoví všechny údery kromě nejvýš poslední sekundy. | E10 |
| **A-23** | MIDI bicí v exportu | Stejné údery dvakrát exportovat a mezi tím vyměnit sadu a vrátit ji zpět. | Exporty se stejnou sadou jsou **bitově shodné**. | E10 |
| **A-24** | Sestavy, které nemáme | Simulovaná zařízení: 1, 2, 8 a 18 vstupů, bez ASIO, frekvence 44,1 a 96 kHz, žádná zvukovka (jen mikrofon notebooku), jen MIDI modul, zvuk i MIDI zároveň, odpojení uprostřed Zapojení. | Zapojení vždy doběhne. Nabídne jen smysluplné sestavy a u nemožných poradí. Nahrávání a přehrávání funguje u všech. Nic nespadne. | E0 (Zapojení), E9, E10, E11 |

---

## M. Měření na počítači se zvukovkou

| ID | Co ověřuje | Postup | Očekávaný výsledek | Etapa |
|---|---|---|---|---|
| **M-01** | **Zpoždění ve sluchátkách** (celá cesta tam a zpět) | Smyčkový kabel z výstupu L do vstupu 1. **Diagnostika → Změřit zpoždění** pro: ASIO s bufferem 64, 128 a 256; WASAPI exkluzivní; WASAPI sdílený nízkolatenční; WASAPI sdílený. Každé měření 5×. | Rocker 2 vypíše *změřeno*, *údaj ovladače* a *rozdíl*. Měření se opakuje ±1 vzorek. Rozdíl se uloží jako kalibrace. Orientačně ASIO 128 ≤ 12 ms (předpoklad, upřesní H-01). | E1 |
| **M-02** | **Nová stopa sedí k bicím i na konci dlouhé skladby** | Smyčkový kabel. **Diagnostika → Zkouška souhry**: Rocker 2 vytvoří zkušební skladbu (10 min, impuls na každou dobu), přehraje ji a přes kabel nahraje na novou stopu. Pak porovná polohy impulsů. Opakuje se po restartu programu a po změně bufferu. | Posun na začátku i na konci **≤ 1 ms**, rozdíl mezi začátkem a koncem **≤ 0,1 ms** (bez rozjíždění). Totéž po restartu a pro každý buffer. | E2 |
| **M-03** | Klik jen ve sluchátkách | Přehrávání se zapnutým klikem, export, export se pak naimportuje. **Diagnostika → Hledat klik v souboru**. | Klik se v exportu nenajde. | E7 |
| **M-04** | Záchrana na skutečném zařízení | Nahrávat kytaru 2 min. V 1:30 ukončit Rocker 2 přes Správce úloh (*Ukončit úlohu*). Spustit znovu. | Nabídka obnovení ≥ 1:28. Po vložení leží nahrávka na správném místě a hraje. | E2 |
| **M-05** | Výkon notebooku | Zkušební skladba: 16 stop, 8 s aparátem, 10 min přehrávání při ASIO 128. Jednou na síťovém napájení, jednou na baterii. | Výpadky zvuku: **0**. Zátěž zvukového vlákna v Diagnostice pod 50 %. Jinak se zapíše, kolik stop s aparátem notebook zvládne. | E3, E7 |
| **M-06** | Disk a antivirus | Nahrávat všechny dostupné vstupy 30 min se zapnutým Defenderem. Pak zkusit složku Skladby uvnitř OneDrive. | Žádný výpadek zápisu. U cesty v OneDrive se ukáže doporučení jiné složky. | E2, E11 |
| **M-07** | Výpadek proudu (volitelné, jednou) | Notebook na baterii, nahrávat 2 min, v 1:30 vynutit vypnutí podržením vypínače 10 s. *Zkouška nese malé riziko pro systém, proto jen jednou a s uloženou prací v jiných programech.* | Po startu nabídka obnovení ≥ 1:27 (ztráta nejvýš asi 3 s). | E11 |
| **M-08** | Odpojení zvukovky | Během nahrávání vytáhnout USB kabel zvukovky, po 10 s ho vrátit. | Nahrávání se zastaví s hláškou a nahrané je uložené. Po připojení se zařízení samo obnoví. | E11 |
| **M-09** | Notebook neusne | Ve Windows nastavit uspání po 1 minutě, nahrávat 3 minuty. | Během nahrávání neusne. Po zastavení se uspí podle nastavení. | E11 |
| **M-10** | Plný disk (skutečně) | Složka Skladby na USB disku s asi 1 GB volného místa. Nahrávat, dokud nepřijdou varování. | Varování při 30 minutách zbývajícího času, řízené zastavení nad 500 MB volného místa. Soubor je čitelný. | E11 |
| **M-11** | Varování u nevhodného zapojení | (a) Zvolit Bluetooth sluchátka. (b) Ve WASAPI zvolit vstup z mikrofonu notebooku a výstup do jiného zařízení. | (a) Varování o zpoždění Bluetooth. (b) Nahrávání přes stopy je zablokované s vysvětlením. | E11 |
| **M-12** | Instalace | Čistý uživatelský účet Windows: instalátor → SmartScreen (*Další informace → Přesto spustit*) → první spuštění → odinstalace. | Instalace bez dalších požadavků (žádný VC++ redistributable). Po odinstalaci zůstanou skladby na disku. | E11 |
| **M-14** | MIDI proti zvuku u elektronických bicích (jen pokud modul posílá obojí) | Nahrát zároveň zvuk modulu a údery MIDI, 16 úderů do virblu. **Diagnostika → Porovnat MIDI a zvuk**. | Rocker 2 vypíše průměrný posun MIDI proti zvuku a uloží ho jako `korekceMIDI`. Po uložení je posun do ±1 ms. | E10 |
| **M-15** | Kontrola sestavy u kamaráda | Na kamarádově počítači s jeho vybavením: Zapojení a pak **Diagnostika → Kontrola sestavy**. | Srozumitelný přehled: ovladač, vstupy, zpoždění, MIDI, procesor a co funguje či nefunguje. Uloží se jako soubor pro Jendu. | E11 (první verze už v E1) |
| **M-13** | MP3 na kamarádově Windows | Export MP3, přehrát ve Windows a v telefonu. | Hraje, délka sedí. Pokud kodér chybí, Rocker 2 to vysvětlí a nabídne WAV. | E7 |

**Jak měřit, když není smyčkový kabel:** Rocker 2 použije údaj ovladače a v panelu Co slyším ukáže „zpoždění odhadnuté“. Náhradní kontrola je **Úder do kliku**: kamarád 16× udeří paličkou o ráfek přesně s klikem a Rocker 2 ukáže průměrnou odchylku úderů od mřížky. Je to jen orientační, protože obsahuje i lidské načasování. Kabel je spolehlivější a stojí pár korun.

---

## H. Skutečné hraní s nástroji

| ID | Co ověřuje | Postup | Očekávaný výsledek | Etapa |
|---|---|---|---|---|
| **H-01** | **Zpoždění při hraní přes aparát neruší** | Kamarád hraje rychlý riff v šestnáctinách s klikem přes předvolbu Crunch. Jenda bez jeho vědomí přepíná buffer (256 / 128 / 64, případně WASAPI). Kamarád hodnotí 1–5 („5 = necítím zpoždění“). | Při zpoždění ≤ 10 ms hodnocení ≥ 4. Výchozí buffer se nastaví na největší, který dostal 4 a víc. Výsledek zapsat i s naměřeným ms z M-01. | E1 |
| **H-02** | **Kytara přes bicí** (01 / 2.2, kroky 1–2) | 8 taktů bicích s odpočtem a pak kytara při jejich poslechu. Poté delší verze asi 5 minut. | Při poslechu není slyšet „flam“ (dvojitý úder) mezi kytarou a kopákem ani na konci. Při přiblížení leží nástupy na stejné čáře mřížky. | E2 |
| **H-03** | Zvuk aparátu a změna po nahrání | Kamarád zkusí 4 předvolby se svou kytarou a hodnotí je. Pak na nahrané stopě přepne předvolbu. | Nejméně jedna předvolba je „použitelná pro nahrávání“ (hodnocení ≥ 4). Změna předvolby na nahrávce funguje bez přehrání znovu. Požadované úpravy zvuku zapsat. | E3 |
| **H-04** | Oprava jednoho taktu | Úmyslná chyba v taktu 6, oprava klávesou **P**, porovnání pokusů klávesou **Tab** a ponechání klávesou **Enter**. | Hotovo do 1 minuty od zjištění chyby. Přechody bez cvaknutí a bez useknutého dozvuku zkreslení. | E5 |
| **H-05** | Pohled zdálky | Z místa za bicími (změřit vzdálenost): poznat stav nahrávání, číslo odpočtu a číslo taktu. Zopakovat ve dne. | Stav nahrávání pozná do 1 s. Odpočet a číslo taktu přečte bez naklánění. | E9 |
| **H-06** | Ovládání s nástrojem v ruce | 10× spustit a zastavit nahrávání klávesou R, nožním spínačem a numerickou klávesnicí paličkou. | Žádná nechtěná akce, žádný ztracený pokus. | E9 |
| **H-07** | Bicí podle sestavy | (a) 1 mikrofon: nejsilnější úder; (b) elektronické bicí stereo; (c) více mikrofonů: úder do každého bubnu, střih skupiny. | (a) Bez přebuzení vstupu, nebo jasná rada ubrat gain. (b) Stereo L/P správně. (c) Rozsvítí se správné stopy a střih je na stejném místě ve všech stopách. | E9 |
| **H-08** | Klik se nedostane do mikrofonu | Bubeník v uzavřených sluchátkách s obvyklou hlasitostí kliku. Nahrávka bicích se pak pustí sólo v tichém místě. | Klik v nahrávce není slyšet. Pokud je, zapsat hlasitost kliku a typ sluchátek a doplnit radu do Zapojení. | E9 |
| **H-09** | **Celá písnička** (akceptační zkouška v1) | Kamarád sám, bez Jendovy pomoci: nápad → skladba → bicí → kytara → oprava → díly → mix → MP3 odeslané Jendovi. | Hotovo bez ztracené nahrávky a bez dotazu „jak se to dělá“. Zapsat čas a každé zaváhání. | E11 |
| **H-10** | Pokračování zítra | Zavřít Rocker 2, restartovat počítač a druhý den otevřít. | Skladba je na stejném taktu, v režimu, se stejným zvukem kytary a mixem. | E2 (základ), E11 |
| **H-11** | Pojistka | Zahrát riff bez nahrávání a pak stisknout **U**. | Riff je v Nápadech celý. | E8 |
| **H-12** | Najít povedený nápad | Po týdnu najít ★ nápad. | Do 10 s. | E8 |
| **H-13** | MIDI bicí (jen pokud je kamarád má) | Naučit pady, nahrát 8 taktů, po nahrání vyměnit sadu, opravit takt klávesou P. | Každý pad hraje správný buben. Údery sedí na klik. Výměna sady funguje bez nového nahrávání. | E10 |

---

## Pořadí zkoušek podle rizika

Nejdřív se ověřují věci, které by mohly změnit volbu řešení:

1. **E1:** M-01 a H-01. Je zpoždění přes aparát snesitelné?
2. **E2:** M-02 a H-02. Sedí kytara k bicím i na konci? A-08 a M-04: přežije nahrávka pád? H-10 (základ): otevře se skladba přesně tam, kde skončila?
3. **E3:** H-03. Je zvuk aparátu použitelný?

Pokud M-01 nebo H-01 selže i s ASIO, Rocker 2 nabídne cestu *přímý odposlech zvukovky + aparát jen při přehrávání* (02 / kap. 15). Pokud selže M-02, je potřeba opravit kompenzaci, než se pokračuje dál.
