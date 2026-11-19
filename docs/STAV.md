# Stav Rockeru 2

## E17 · hlasitější poslech v telefonu

Volitelný export cílí na −14 LUFS a chrání skutečné špičky pod −1 dBTP. A-32 ověřuje pět případů EBU a výsledné WAV při 44,1 a 48 kHz. Míchej ukazuje naměřenou hodnotu posledního exportu. Poslech čeká po E18. [Zkoušky a sestavení](https://github.com/JendaNDT/Rocker-2/pull/20)

## E16 · vodicí rytmus

V Hraj lze přepnout mezi Klikem, Rytmem a tichem. A-31 porovnává každý zvukový vzorek vodítka po deseti minutách a ověřuje shodu vývozu s vodítkem a bez něj. Poslech a nahrávání na skutečné sestavě čekají po E18. [Zkoušky a sestavení](https://github.com/JendaNDT/Rocker-2/pull/19)

## E15 · zvuk zpěvu a baskytary

Stopa Jiná nabízí předvolby Bez úprav, Zpěv a Baskytara. Parametry se ukládají se skladbou a lze je měnit během přehrávání. A-30 porovnává zvuk při čtyřech velikostech bloku, znovuotevření a nedotčený WAV. Poslech a zkoušky celé skladby přijdou po E18. [Zkoušky a sestavení](https://github.com/JendaNDT/Rocker-2/pull/18)

## E14 · export jednotlivých stop

**Poslat mix… → Stopy zvlášť** uloží číslované stereo WAV ve frekvenci skladby do jedné složky. A-29 sečte samostatné soubory a vzorek po vzorku je porovná s mixem s celkovou hlasitostí 0 dB. Bicí ze soupravy tvoří jeden soubor. Hraní a poslech čekají po E18. [Zkoušky, sestavení a snímky](https://github.com/JendaNDT/Rocker-2/pull/17)

## E13 · koš nápadů, M4A a Mac s Intelem

Nápad lze po potvrzení přesunout do systémového koše. Na Windows a Macu lze přidat M4A nebo AAC jako podklad. Na Macu lze poslat mix ve formátu M4A. Balík pro Mac obsahuje spustitelné části pro Apple Silicon i Intel. Vedle tlačítka Přidat podklad je volba jeho umístění. [Zkoušky a balíky E13](https://github.com/JendaNDT/Rocker-2/pull/16)

Skutečný Mac s Intelem, poslech exportu a ostatní hraní čekají na ruční zkoušku po E18.

## E12 · spolehlivé nápady, historie a frekvence

Knihovna Nápady používá stejnou složku při ukládání i přehrávání náhledu, ať nové skladby ukládáš kamkoli. Zesílení stopy v Míchej se zobrazuje v dB. Hlášky rozlišují nastavení, skladbu, nahrávku, nápad, podklad a úklid. Historie ukládá jen skutečné změny. Při odlišné frekvenci zvukovky uvidíš obě hodnoty a můžeš přepnout na frekvenci skladby, pokud ji zařízení nabízí. [Výsledky a snímky](https://github.com/JendaNDT/Rocker-2/pull/15)

Hraní a měření na skutečné sestavě čekají na ruční zkoušku po E18. [Rozhodnutí](ROZHODNUTI.md#e12--složka-nápadů-historie-a-připojení)

## E11 · přenos, odolnost a vydání v1

Balík skladby používá ZIP64 a kontrolní součty SHA-256 jednotlivých souborů. Na novém počítači se rozbalí do samostatné složky. Při otevření lze přemapovat vstupy. Volné místo kontroluje zapisovač, během nahrávání se dá odděleně ověřit varování i zastavení. Na Windows se k ověření instalátoru přidává skutečná instalace, spuštění okna a odinstalování pod účtem běžce. [Výsledky, balíky a snímky E11](https://github.com/JendaNDT/Rocker-2/pull/12)

A-10 a A-11 porovnávají obsah zabalené a přenesené skladby v dlouhé cestě s diakritikou. A-13 čte každý uložený vzorek po simulovaném zaplnění disku. A-24 odpojuje běžící zvukovku a přehrává scénář na druhém počítači s odlišnými názvy vstupů. Výsledky skutečného poslechu, měření a ručních zkoušek se zapíší po vydání. [Plán zkoušek](03_ZKOUSKY_NA_WINDOWS.md)

Sloučená E11 prošla automatickými zkouškami. [Výsledky pro Windows a Mac](https://github.com/JendaNDT/Rocker-2/actions/runs/44461855081) · [Vydání v1 a ověření stažených souborů](https://github.com/JendaNDT/Rocker-2/actions/runs/44463135386)

## E10 · elektronické bicí přes MIDI

Stopa MIDI ukládá jednotlivé údery a jejich sílu do standardního souboru. Před přehráním nebo exportem zazní podle vybrané sady. Při společném záznamu vznikne navíc samostatná stereo stopa se zvukem modulu. [Jak zapojit modul](../README.md#jak-nahrát-elektronické-bicí-přes-midi)

A-22 měří odchylku jednotlivých úderů po dobu deseti minut a obnoví přerušený MIDI záznam po ukončení procesu. A-23 porovnává výsledné WAV před výměnou sady a po návratu. Část A-24 prochází celé zapojení a nahrávání, jak se samotným modulem bez zvukových vstupů, tak se stereo signálem. H-13 a fyzické M-14 čekají na ruční zkoušku po vydání. [Výsledky a snímky](https://github.com/JendaNDT/Rocker-2/pull/11)

## E9 · bicí od jednoho mikrofonu po celou sadu

Průvodce pracuje se vstupy, na kterých během zkoušky zazněl úder. Akustická sada má společné nahrávání, pokusy, střih a hlasitost. Stereo modul má samostatnou levou a pravou stranu. F11 přepne Pohled zdálky a USB spínači lze přiřadit záznam. [Postup pro muzikanta](../README.md#jak-připravit-bicí)

A-24 zkouší sestavy s 1, 2, 8 a 18 vstupy, včetně tichých vstupů, odpojení, ovládání spínačem a návratu z Pohledu zdálky během nahrávání. H-05 až H-08 čekají na ruční zkoušku po vydání v1. [Sestavení, snímky a výsledky](https://github.com/JendaNDT/Rocker-2/pull/10)

## E8 · nápady, Pojistka a podklady

Z úvodu se riff nahrává klávesou R, historie vstupu se uloží klávesou U. Knihovna zachová hvězdy, poznámky, ladění a tempo. Podklady se převádějí na frekvenci skladby při importu. [Postup pro Jendu](../README.md#jak-zachytit-riff)

- **A-18** posílá tři minuty číslovaných vzorků a porovnává každý vzorek posledních dvou minut. Dále vyzkouší dvousekundový předstih, konec záznamu, vypnutí a opětovné zapnutí Pojistky.
- **A-19** kontroluje délku i výšku tónu převedeného podkladu ve WAV, AIFF, FLAC, Ogg Vorbis, Ogg Opus a MP3. V každém stereo kanálu měří jinou frekvenci.
- **A-17** obsluhuje nahrávání, hvězdu, poznámku, osm dob, převod na skladbu a přetažení souboru. Kontroluje i posuny o 1 a 10 ms. [Nahrávání](zkousky/E8/nahravani.png) · [Knihovna](zkousky/E8/napady.png)
- H-11 a H-12 na skutečném počítači **čekají na ruční zkoušku po vydání**.

## E7 · mix a export

Režim **3 Míchej** má v řádku každé stopy hlasitost, polohu, sólo, ztlumení a měřič. Celkem ukazuje špičku a její takt. Záznam uchovává značku přebuzeného vstupu. Přebuzený mix má vlastní značku a při exportu lze celkovou hlasitost opravit. [Sestavení, snímky a zkoušky na třech systémech](https://github.com/JendaNDT/Rocker-2/pull/8)

- **A-05 a A-06** porovnávají výsledný 24bitový WAV s přehráváním ve čtyřech velikostech bloku a hlídají, že klik ani odpočet nevstoupí do souboru. Další zkoušky kontrolují stereo polohu, přebuzení před kvantováním, délku výběru a opakovatelnost 16bitového ditheru.
- **A-12** ve Windows zjišťuje dostupnost stereo kodéru pro 320 kb/s. Pokud ho systém nabízí, zapíše skutečný MP3, znovu jej dekóduje a porovná délku. Výsledek je v kroku **MP3 encoder result (A-12)**.
- **A-17** zkouší řádky mixu, výběr rozsahu, upozornění na sólo a soubor vytvořený skutečným tlačítkem. Diagnostika má zkoušku s čistým souborem i se skutečným signálem metronomu. [Mix](zkousky/E7/mix.png) · [Poslat mix](zkousky/E7/export.png)
- Poslech, M-03 a M-13 **čekají na ruční zkoušku po vydání**. [Postup pro hráče](../README.md#jak-poslat-mix)

## E6 · sloky, refrény a pořadí

Pás Díly ukazuje jednotlivé výskyty skladby. Umožňuje je přejmenovat, obarvit, rozdělit, zopakovat, přesunout, zkrátit a osamostatnit. Při opravě taktu ve spojeném výskytu hráč vybírá rozsah změny. [Výsledky CI a sestavení](https://github.com/JendaNDT/Rocker-2/pull/7)

- **A-02** měří polohu následujícího dílu ve vzorcích, ověřuje shodu původního WAV, průběh obou hran ořezu, návrat délky, Zpět a opětovné načtení. Samostatná zkouška upraví viditelnou část zkráceného refrénu a zkontroluje zvuk po roztažení.
- **Zkouška ovládání** připraví Sloku a dva spojené refrény, zkrátí druhý refrén z osmi taktů na šest a porovná stav po Zpět, tažení okraje i příkazu Obnovit celou délku. Zkouška používá jeho tlačítko ve spodní desce. Nahraje opravu pro oba refrény a další jen pro druhý. [Díly](zkousky/E6/dily.png) · [Výběr rozsahu opravy](zkousky/E6/oprava.png)
- Hraní, poslech návaznosti a posouzení čitelnosti hráčem **čeká na ruční zkoušku po vydání**. [Postup](../README.md#jak-zopakovat-refrén)

## E5 · oprava jednoho taktu

Klávesa P pořídí celý čistý průchod s předstihem a dohrou. Ve výběru zní aktivní pokus. Při přepnutí klávesou Tab dál běží čas i aparát stopy. Enter uloží výběr. [PR, sestavení a snímky](https://github.com/JendaNDT/Rocker-2/pull/6)

- **A-21** srovnává každý vzorek opraveného úseku se spočteným desetimiliskundovým prolnutím dvou sinusovek s opačnou fází. Dále zkouší změnu volby během přehrávání.
- **Ovládání a uložení** ověřuje dva nové pokusy v taktu 6, návrat k původnímu, kompenzaci 373 vzorků, přesnou délku celého WAV, zahození, obnovení, Zpět a znovuotevření skladby. [Pokusy](zkousky/E5/pokusy.png)
- **H-04 čeká na ruční zkoušku po vydání.** [Postup pro hráče](../README.md#jak-opravit-jediný-takt)

## E4 · střih, vracení úprav a verze

V režimu Stříhej je rozsah vidět přes všechny stopy. Střih, přesun, opakování, vložení, zkrácení a smazání mění odkazy vybrané stopy na zvukové soubory. Zpět a Znovu vracejí snímky skladby. Nabídka Soubor umožňuje vybrat uloženou verzi podle data a času. [Návrh E4 s výsledky a sestaveními](https://github.com/JendaNDT/Rocker-2/pull/5)

- **A-01** provede 1 000 náhodných změn. Po každé načte nový stav, pak vrátí všechny kroky a provede je znovu. Porovná celé JSON skladby. Samostatně se zkouší zrušení větve Znovu po nové úpravě, mezery ve výběru, přesné polohy zdrojových vzorků a meze okrajů.
- **A-17** ovládá skutečné okno testovací skladby. Vybere takty 5–6, přesune je na 9, zopakuje výběr, vrátí jej, posune okraj s Alt a obnoví starší verzi. Ověřuje polohu vzorků a bajtovou shodu zdrojového WAV. [Výběr](zkousky/E4/strihej.png) · [Lupa](zkousky/E4/lupa.png)
- Přesný střih a poslech na Jendově počítači **čeká na ruční zkoušku po vydání**. [Postup](../README.md)

## E3 · změna zvuku po nahrání

Kytarová stopa zpracuje při přehrávání kopii nastavení uloženou ve skladbě. Ve spodní desce lze přepnout aparát, změnit parametry a uložit vlastní předvolbu. Zachování zdrojového WAV, poloha vůči bicím a stejné nastavení po obnovení skladby mají automatické zkoušky. [Snímek](zkousky/E3/zvuk-kytary.png) · [Sestavení a výsledky pro jednotlivé systémy](https://github.com/JendaNDT/Rocker-2/pull/4)

| Zkouška | Co se ověřuje |
|---|---|
| A-05 | Dvě stereo kytary, bicí a doznívající efekty. Reálná cesta přes zvukové jádro s bloky 64, 128, 512 a 1024 se porovnává s WAV v obou kanálech. Tolerance vůči WAV je 0,0000005 plné úrovně, takže rozdíl mezi libovolnými dvěma výsledky může být nejvýše −120 dBFS. Dva soubory z renderu musí být bitově shodné. |
| A-20 | Změna při nepřerušeném přehrávání a návrat na Crunch. Znovuotevření změněného zvuku, vlastní předvolba odstraněná z knihovny, čistý zdroj nahrávky a posun o latenci aparátu. |
| A-17 a provoz jádra | Panel při přehrávání ve dvou velikostech. Na Macu zahrnuje kontrola zvukového vlákna také přehrávání s aktivními aparáty. |
| Příprava M-05 | Automat ověřuje délku, počet stop, osm aparátů a zvuk do posledního bloku. Skutečný běh má v Diagnostice vlastní spuštění a výsledek. |

Na vývojovém Linuxu s procesorem AMD EPYC 9V74 zpracoval Release deset minut této skladby (48 kHz, blok 128, 16 stop, osm aparátů) za 39,75 s, průměrně 6,62 % dostupného času. Šlo o výpočet bez zvukového zařízení. Výpadky ovladače ani provoz na baterii toto měření nepokrývá.

**H-03 i M-05 čekají na ruční zkoušku po vydání.** [Postup pro hráče a měření výkonu](../README.md)

## E2 · nahrávání a návrat ke skladbě

Nová skladba, odpočet a nahrávání jsou dostupné z úvodu. Kontrola v rozhraní prošla cestou od osmi taktů bicích přes samostatnou čistou kytaru až po zavření a opětovné otevření na uloženém vzorku. [Postup pro Jendu](../README.md) · [Výsledky pro jednotlivé systémy](https://github.com/JendaNDT/Rocker-2/pull/3/checks)

| Automatická zkouška | Co se porovnává |
|---|---|
| A-03 | Diakritika, stopy, klipy, převod staršího schématu a uložení pohledu. Při poškozeném popisu se vyhledá čitelná verze. |
| A-04 | Deset minut kliku při 112 BPM. Každý nástup odpovídá přesnému vzorku odvozenému od pořadí doby. |
| A-06 | Dva vyrenderované WAV při různém nastavení kliku a odpočtu jsou bitově shodné. |
| A-07 | Simulovaný vstup má zpoždění 211 vzorků a výstup 173. Návraty impulsů po deseti minutách sedí na původních pozicích. |
| A-08 | Po 20 sekundách je násilně ukončen proces zapisující rostoucí vzorky. Jiný proces ověřuje alespoň 19 sekund souvislého WAV, původní polohu a dokončení obnovy. |
| A-09 | Proces je 1 000krát ukončen při střídavém zápisu dvou stavů. Po každém ukončení musí být skladba kompletní. |
| A-14 | Pětisekundové pozastavení disku zachová všechny vzorky. Při patnáctisekundovém pozastavení je po vyčerpání fronty uložen čas výpadku. |
| A-15 | Na Macu prochází stejná desetiminutová zkouška pod RealtimeSanitizerem a ThreadSanitizerem, včetně ladicího sestavení. Výsledek je v krocích kontroly na GitHubu. |
| A-17 | Snímky založení skladby, dvou nahraných stop a Pohledu zdálky při nahrávání včetně velikosti 1920 × 1080 při 150 %. |

Místně prošly automatické kontroly nahrávání, obnovy a ukládání. **M-02, H-02, M-04 a H-10 čekají na ruční zkoušku po vydání.** Naměřené zpoždění konkrétní zvukovky ani výsledek hraní od Jendy zatím nemáme.

## E1 · kytara do sluchátek

Na Windows x64, Windows ARM64 i macOS prošlo sestavení a všech 12 automatických zkoušek. Každý počítač vytvořil balík s aplikací a 18 snímků. [Ověřený běh](https://github.com/JendaNDT/Rocker-2/actions/runs/44403854014)

| Automatická zkouška | Výsledek |
|---|---|
| A-16 | Při 44,1 / 48 / 96 kHz všechny půltóny od B1 po E4, rozladění +7,5 centu. Čistý tón se vešel do ±1 centu, tón s přidanými harmonickými a šumem do ±3 centů. |
| A-20 | Na stejném vstupu se Crunch po přepnutí na Metal i po dalších předvolbách vrátil k bitově shodnému výstupu při různých velikostech bloků. |
| Směrování | Čistý poslech čte pouze vybraný vstup, oba kanály sluchátek mají stejný signál. Referenční a potvrzovací tón se nepřimíchaly do vstupu. Změř ticho nastavil práh po dvou sekundách. |
| Kabel v simulaci | Pět pokusů na každé ze tří frekvencí našlo známých 373 vzorků i s obrácenou polaritou a útlumem. Bez návratu signálu měření selhalo. Samostatná zkouška přes obrazovku Diagnostika uložila 411 vzorků a korekci +38. Neúspěšný další pokus výsledek zachoval. |
| Ladění a nastavení | Devět továrních ladění, Drop D / B, neplatné názvy tónů, dva různé vstupy a vlastní ladění s českým názvem po novém otevření. |
| A-17 a snímky | Devět obrazovek ve dvou velikostech, včetně ladičky s hrajícím D2. Simulovaná zvukovka dodala signál pro měřiče. |

Při spuštění skutečné aplikace na místním Linuxu bez zvukovky se otevřelo okno 1280 × 720. Klávesa T zobrazila ladičku s radou, kde vybrat vstup. Další stisk vrátil průvodce Zapojení.

**M-01 a H-01 čekají na ruční zkoušku po vydání hotové aplikace.** Čísla ze simulace výše slouží k ověření výpočtu. Jendova zvukovka, sluchátka, procesor, buffer, pět měření a hodnocení riffu zatím čekají na první zápis. Pro hraní ještě nebyl schválen výchozí buffer. [Postup a tabulka pro měření](../README.md) · [Náhled hraní](zkousky/E1/hrani.png) · [Náhled ladičky](zkousky/E1/ladicka.png)

## E0 — první zvuk

Na Windows x64, Windows ARM64 a macOS prošlo sestavení i všech šest automatických zkoušek. GitHub uložil tři balíky s aplikací a tři sady po osmi snímcích. [Ověřený běh CI](https://github.com/JendaNDT/Rocker-2/actions/runs/44374946240)

| Místní zkouška | Výsledek |
|---|---|
| Měřiče | Prošlo. Špička záporného vzorku, ticho, hranice −0,1 dBFS, podržení 1,5 s a ruční zrušení přebuzení zvlášť pro každý vstup. |
| Tón | Prošlo. Každý vzorek na čtyřech výstupech porovnán se sinem 440 Hz, −18 dBFS. Délky, stereo i závěrečné ticho souhlasí při 44,1 / 48 / 96 kHz. |
| Nastavení | Prošlo. Uložení a načtení jmen s diakritikou, zařízení, frekvence a bufferu. Špatný typ, neznámé pole a poškozený JSON aplikaci neshodily. |
| A-17 | Prošlo. Rozměry textu na úvodu, ve třech krocích, u odpovědí na tón i u budoucích funkcí. Vzniklo osm PNG. Fonty dodaly vlastní kresbu ř, ů, ě, ť. |
| A-24 pro Zapojení | Prošlo pro 1, 2, 8 a 18 vstupů při třech frekvencích. Sestavy bez ASIO, mikrofon notebooku, odpojení během průvodce a opětovné připojení. |
| Pojmenování brnknutím | Prošlo. Krátký impuls do druhého vstupu následovaný tichem mezi obnovami obrazovky označil pouze druhý vstup. |
| Spuštění bez zařízení | Na místním Linuxu se otevřelo okno 1280 × 720. Vyzkoušeny tři kroky, návrat na úvod a klávesa T. |

Výsledky cílových počítačů jsou uvedené jednotlivě u návrhu změn. [Výsledky CI](https://github.com/JendaNDT/Rocker-2/pull/1/checks) · [Běhy s balíky a snímky](https://github.com/JendaNDT/Rocker-2/actions/workflows/ci.yml?query=branch%3Aetapa-0)

## Ruční zkoušky E0 po vydání hotové aplikace

- Windows a fyzická zvukovka s ovladačem ASIO nebo Windows Audio. Poslech tří částí tónu, kytara ve vstupu 1, přebuzení, změna bufferu a odpojení USB.
- Mac, CoreAudio a Jendova zvukovka, stejný poslech a práce s gainem.
- Volitelně mikrofon notebooku a varování u jeho skutečného názvu.
- Jendovo porovnání úvodu a Zapojení s 01 / 5.7 a 4.1.

Jenda ověří tón, měřiče i zpoždění na vydané aplikaci. [Kontrolní seznam](../README.md)
