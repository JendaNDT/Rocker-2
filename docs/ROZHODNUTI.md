# Rozhodnutí v E0

| Otázka | Rozhodnutí a důvod | Alternativa |
|---|---|---|
| Repozitář | Na účtu JendaNDT už existoval prázdný veřejný Rocker-2. Použili jsme jej a ve větvi main vytvořili prázdný výchozí commit pro pull request. | Zakládat druhý repozitář s názvem rocker2. |
| Chybějící dokument 05 | Jenda přiložil čtyři dokumenty. Dokument 05 v docs je označený přepis dnešního zadání. | Po dodání původního 05 oba texty porovnat. |
| Windows ARM64 | Vedle windows-2025 pro x64 běží nativní windows-11-arm. Umožňuje sestavení i spuštění testů ARM64. Mac používá macos-latest s Apple Silicon. | Podle 02 / 13.2 lze ARM64 přeložit na x64 a testy následně přenést na ARM. |
| ZIP a instalátor | V E0 předáváme přenosný ZIP podle zadání. Etapa E11 obsahuje instalátor a jeho zkoušku. | Zařadit instalátor z 02 / 13.2 už do E0. |
| První otevření | Poprvé se otevře Zapojení podle 01 / 5.8. Úvod je hned dostupný a při dalších spuštěních se otevře přímo. Příkladové skladby z náčrtu zastupují prázdné řádky. | Vložit vzorové nahrávky, které by v E0 nešly přehrát. |
| Zvětšení | Podle 01 / 5.1 je 1920 × 1080 při 150 % plocha 1280 × 720 logických bodů. Test ukládá oba snímky, při 100 % i 150 %. Kontroluje také širší rozložení úvodu. | Zvětšit okno široké 1920 logických bodů na 2880 obrazových bodů. |
| Mono a náhradní režim | U dvou výstupů hraje tón L / P / oba. U jediného výstupu hraje tři sekundy v dostupném kanálu a průvodce to vysvětlí. Známý vestavěný mikrofon rozpoznává podle názvu, nejasný název dovolí označit ručně. | Přeskočit poslech u mono zařízení nebo se na vybavení ptát při každém otevření. |
| Brnknutí a více nástrojů | Po stisku Najít vstup kytary sledujeme aktivitu nad −36 dBFS. Při souběhu nabídneme ruční pojmenování. Rozpoznání celé soupravy patří do E9 a volba zdroje kytary do E1 podle 04. | Při více aktivních vstupech hádat podle nejhlasitějšího. |

Přednost má fungující uložené zapojení. Pro první otevření se preferuje 48 kHz a 128 vzorků, pokud je ovladač nabízí. Výchozí buffer pro hraní přes aparát určí H-01.

## E1 · hraní do sluchátek

| Otázka | Volba a důvod | Alternativa |
|---|---|---|
| E0 stále čeká na přijetí | Na Jendův pokyn začíná E1 z posledního ověřeného sestavení E0. Návrh E1 směřuje do větve etapa-0 a obsahuje pouze přírůstek. Ruční poslech E0 je otevřený. | Po sloučení E0 přesměrovat návrh E1 do main. |
| Původ zvuku reproboxu | Rocker při otevření zvukovky vypočte původní krátkou impulsní odezvu pro její frekvenci. Pásmo kytarového reproduktoru, zdvih středů a útlum vysokých středů jsou popsané přímo v Amp.cpp. Koeficienty vznikají ze vzorce, celý tento kód a odezva patří pod AGPLv3 projektu. | Po poslechovém srovnání přidat měřenou odezvu s doloženou licencí. |
| Výpočet výšky tónu | Použili jsme vlastní implementaci YIN s upřesněním délky periody. Detekce probíhá mimo zvukové vlákno. Kritéria přesnosti a odolnosti jsou podle A-16 včetně Drop B. | Dokument 01 zmiňuje algoritmus McLeod. Můžeme jej porovnat při potížích s konkrétní kytarou. |
| První poslech a kalibrace | Zapojení začne s vypnutým odposlechem. Přechod na ladičku nebo měření jej vypne. Kabelový test hledá návrat do půl sekundy a odmítne slabou shodu. Uchovává poslední úspěch pro daný ovladač, vstupní a výstupní zařízení, frekvenci a buffer. | Zapnout odposlech automaticky až po výslovném potvrzení vhodného zapojení. |
| Klik při H-01 | Hráč v E1 použije externí metronom. Klik studia přibude s transportem v E2 podle dokumentu 04. Výchozí buffer se rozhodne podle H-01. | H-01 zopakovat také s klikem studia v E2. |
| Zpoždění a zdroje | Údaj s aparátem zahrnuje přibližné zpoždění digitálních filtrů. Kabel změří celou cestu zvukovkou bez aparátu. Bluetooth se rozpoznává ze známých označení výstupu, skryté bezdrátové propojení zvukovka aplikaci nemusí sdělit. | U nejasného výstupu ověřit kabelové propojení se zvukovkou. |

[Zdroj impulsní odezvy](../src/dsp/Amp.cpp) · [Licence projektu](../LICENSE)

<a id="termin-rucnich-zkousek"></a>

## Termín ručních zkoušek

Jenda rozhodl, že veškeré ruční testování proběhne až po vydání hotové aplikace. Týká se poslechu, měření na skutečné zvukovce, hraní i kontroly vzhledu. Původní plán rozděloval tyto zkoušky mezi etapy a vyžadoval čtyři výsledky před pokračováním za E3. Během vývoje budou postupy připravené a každá neprovedená zkouška ponese stav „čeká na ruční zkoušku po vydání“. Automatické testy a sestavení běží v každé etapě. Po vydání zapíšeme skutečně naměřené hodnoty a zkušenosti hráče.

## E2 · první nahraná skladba

| Otázka | Volba a důvod | Alternativa |
|---|---|---|
| Délka a tempo | První díl má osm taktů. Delší záznam jej prodlouží na celý takt. Změnu tempa či metra po nahrávání potvrzuje hráč na obrazovce s upozorněním. | Rozdělení mezi více dílů patří do etapy s aranžováním. |
| Uložení a obnova | Každý připravený vstup nebo stereo pár má soubor float WAV. Společný deník vzniká před spuštěním záznamu. Po úspěšném uložení klipů se odstraní. Záchrana využívá stejné vložení klipů při běžném zastavení i po pádu. | Samostatný import zbylých WAV by potřeboval ruční umístění na časovou osu. |
| Přerušení ukládání ve Windows | Zkouška A-09 zachytila chybějící soubor skladby po ukončení procesu při nahrazování přes JUCE. Nový obsah zapisujeme přes JUCE TemporaryFile ve stejné složce, zápis dokončíme a ve Windows přejmenujeme jedním voláním MoveFileExW. Stejný postup využívají nastavení, pohled a deník. | Původní postup na Windows v této zkoušce selhal. |
| Zaseknutý disk | Fronta má deset sekund na stopu. Při zaplnění Rocker ukončí průchod, dokončí zapsatelné vzorky a označí místo výpadku na časové ose. | Pokračování se zaznamenanou mezerou by vyžadovalo odlišit přerušené úseky jednoho pokusu. |
| Čtení nahrávek | Soubory WAV se mapují do paměti. Pracovní vlákno načítá stránky před ukazatelem. Při otevření se vypočítá obálka průběhu po 256 vzorcích. | Samostatná mezipaměť náhledů na disku může zkrátit otevírání dlouhých projektů. |
| Zkouška souhry | Diagnostika použije dočasnou skladbu, skutečný výstup a stejnou záznamovou cestu jako nahrávání. Po změření posunu na začátku a konci dočasné soubory uklidí. | Měření v otevřené skladbě by vyžadovalo přidat a následně odstranit zkušební stopy. |

M-02, H-02, M-04 i H-10 proběhnou na vydané aplikaci podle Jendova rozhodnutí.

## E3 · zvuk kytary ve skladbě

| Otázka | Volba a důvod | Alternativa |
|---|---|---|
| Starší skladby | Kytary z E2 bez uloženého aparátu se otevřou s původním čistým zvukem. Štítek Bez aparátu umožní kdykoli vybrat předvolbu. Nová přímá kytara začíná s Crunch. | Zapnout aparát při převodu starší skladby by změnilo první poslech. |
| Co se ukládá | Stopa má vlastní název zvuku, všechny hodnoty, příznak úpravy a vypnutí aparátu. Vlastní předvolby počítače jsou v nastaveni.json. Pro jiné nastavení stejné předvolby lze zvolit nový název. | Odkaz na předvolbu v knihovně by přestal fungovat po jejím odstranění. |
| Ozvěna a dozvuk | Ozvěna má opakování po 300 ms. Čtyři pevné odrazy dozvuku začínají po přibližně 30–44 ms. Hráč mění podíl každého efektu. Při přehrávání a renderu mají kytarové klipy na konci skladby dvě sekundy na doznění. | Další ovládání délky ozvěny nebo typu prostoru by rozšířilo panel nad požadavky E3. |
| Zkouška výkonu | Diagnostika vytvoří šestnáct dočasných desetisekundových WAV, po jednom na stopu, a jejich opakováním přehraje deset minut. Sleduje nejvyšší vyhlazenou zátěž zvukového vlákna a přírůstek výpadků hlášených ovladačem. Při nedostupném počítadle ukáže „ovladač nehlásí“. | Šestnáct desetiminutových souborů by zatížilo také disk a potřebovalo při 48 kHz přibližně 1,8 GB. |
| Větev E3 | E3 vychází z ověřeného konce E2. Při zahájení byl návrh E2 ještě otevřený. Návrh E3 proto míří do větve etapa-2, aby ukazoval pouze změny zvuku. | Po sloučení E2 lze návrh E3 přesměrovat do hlavní větve. |

## E4 · střih a verze

| Otázka | Rozhodnutí a důvod | Alternativa |
|---|---|---|
| Obsazený cíl | Přesun, vložení i opakování v cílovém rozsahu nahradí materiál vybrané stopy. Části před a za cílem zůstanou na místě, stejně jako ostatní stopy. Uvnitř výběru se zachovají mezery. Předchozí skladbu vrátí Zpět. | Odsunout zbytek stopy by posunulo její souhru s ostatními nástroji. |
| Dvě historie | Zpět a Znovu drží během práce se skladbou její snímky v paměti. Režim, přiblížení, ukazatel a výběr se ukládají zvlášť. Obnovení datované verze kontroluje čitelnost popisu i nahrávek a má vlastní krok Zpět. | Procházet ručně starší soubory ve složce Verze. |
| Samostatné pokračování | Jenda požádal o dokončení zbývajících etap v1 bez potvrzování každé z nich. Práce navazuje na automaticky ověřené větve a předání. Termín ručního hraní a měření zůstává po vydání. | Přerušovat práci mezi etapami a čekat na jednotlivé pokyny. |

## E5 · záznam pokusů a koš

Každý průchod má vlastní čistý WAV. Koš je příznak u pokusu ve skladbě. Přesunutí samotného WAV do jiné složky by zneplatnilo cesty ve starších uložených verzích. Příkaz **Vrátit z koše** zpřístupní zahozený průchod. O trvalém odstranění souborů bude v E11 rozhodovat **Uklidit skladbu**.

Ovladač může dodat poslední vzorky vstupu se zpožděním. Zapisovač proto doběhne ještě o nastavenou kompenzaci. Po posunu má průchod přesnou délku od předstihu do konce dohry. Při přerušení před koncem opravovaného úseku se čitelná část uloží mezi zahozené pokusy v místě skutečného záznamu. Přerušený záznam tak automaticky nepřekryje původní hraní.

Prolnutí mezi původním zahráním a opravou trvá 10 ms na každém okraji. U úseku kratšího než 20 ms se oba okraje vejdou do zvolené délky. Přepnutí během poslechu také používá krátké prolnutí. Stále běží stejný aparát dané stopy.

## E6 · díl a jeho výskyty

Zvuky a pokusy jsou uložené jednou u dílu. Každý výskyt si pamatuje svůj začátek v dílu a počet hraných taktů. Osamostatnění zkopíruje popis dílu a odkazy na WAV. Při rozdělení uvnitř spojeného výskytu vzniknou dva díly pro tento výskyt, ostatní výskyty si ponechají společný zdroj.

Konec zkráceného výskytu se ztišuje posledních 5 ms. První vzorek po hraně začíná na nule a zvuk následujícího dílu nabíhá 5 ms. Na hraně se vyčistí dozvuk aparátu, poloha dalšího taktu zůstane přesná. Prolnutí pokusů uvnitř dílu má nadále 10 ms.

Při tempu s necelou délkou doby ve vzorcích se poloha na době převádí přes číslo doby a zaokrouhlí se až v cílovém výskytu. Prosté sčítání dvou zaokrouhlených vzdáleností by mohlo posunout opravu o jeden vzorek.

## E7 · hlasitost a soubor pro posluchače

| Otázka | Volba a důvod | Alternativa |
|---|---|---|
| Hlasitost starší nahrávky | Starší mono stopa s nahrávkou dostane při prvním otevření +3,01 dB. Vyrovná tím útlum nového panorámování uprostřed a zachová hlasitost v obou sluchátkách. Nová stopa začíná na 0 dB. | Otevřít starší mono nahrávky o 3 dB tišeji. |
| Výběr a dozvuk | Celá skladba dostane dvě sekundy na dozvuk. Výběr končí přesně na zvolené hranici. Před jeho začátkem se zpracuje předchozí zvuk, aby na začátku výběru byla slyšet stejná ozvěna jako při přehrávání celé skladby. | Exportovat výběr s nově spuštěným aparátem a přidat mu samostatný dozvuk. |
| Opakovaný export | Každý export má vlastní soubor ve složce Exporty u skladby. U přebuzeného mixu nabízí Rocker snížení o přesah špičky plus 1 dB rezervy a výpočet nového souboru. | Zapisovat přes předchozí export. |
| MP3 | Ve Windows použijeme systémový stereo kodér pro 320 kb/s při 44,1 kHz. Pokud systém takový kodér nenabídne, hráč vidí důvod u šedé volby MP3. | Přibalit další kodér a jeho licenci. Dokument 02 požaduje Media Foundation. |

Kodér MP3 ve Windows ARM64 přidal v první zkoušce k dvousekundovému výběru 1 776 dekódovaných vzorků navíc. Rocker do souboru přidává hlavičku Info s počtem zvukových rámců a s délkou rozběhu a výplně. A-12 kontroluje soubor systémovým dekodérem a nezávislým dekodérem minimp3 (CC0) s podporou přesné délky. Kontroluje také zvuk na obou krajích výběru a datový tok zvukových rámců. Přehrávače, které tuto informaci ignorují, mohou přehrát i krátkou výplň kodéru.

## E8 · nápad a jeho tempo

| Otázka | Volba a důvod | Alternativa |
|---|---|---|
| Zdroj Pojistky | Pojistka a rychlý záznam používají zvolený vstup kytary. Pro dva či více vstupů je v knihovně jejich výběr. Dvě minuty a pěti­sekundová rezerva zůstanou pod limitem 256 MB i při 384 kHz. Vypnutí uvolní dlouhou paměť a zachová pouze paměť pro dvousekundový předstih. | U všech vstupů současně by délku při vícekanálové sestavě omezila dostupná paměť. Více stop patří do skladby. |
| První doba volně hraného riffu | Osm ťuknutí určí tempo. Nový klip se umístí tak, aby první ťuknutí připadlo na začátek taktu. Uloží se i hra před první dobou. | Oříznout začátek riffu. |
| Formáty podkladu | Převod frekvence obstarává libsamplerate 0.2.2 v režimu SINC_BEST_QUALITY. Zdroj se kopíruje do nového 32bitového WAV. Licence BSD-2-Clause cestuje se sestavením. M4A/AAC využívá Windows Media Foundation. | Při každém přehrávání znovu dekódovat původní soubor. |

## E9 · dvě aktivní zásuvky a klávesy u soupravy

Dva vstupy mohou znamenat jediný stereo modul i dva akustické mikrofony. Průvodce nabídne obě možnosti a dovolí zaměnit pořadí vstupů. Sestavu ukládá podle zařízení. Pro čistý stereo odposlech vede každou stranu modulu do odpovídajícího sluchátka.

JUCE předává oba Enter jako stejnou klávesu. Ve skladbě tedy oba ovládají záznam. Při porovnávání pokusů má Enter původní význam a v textovém poli slouží k potvrzení. Spínač, který se v systému hlásí jako klávesnice, se učí jediným stiskem. MIDI spínač patří do rozšíření po v1.

## E10 · původní sady a přehrávání úderů

Architektura navrhuje juce::Synthesiser a současně vylučuje zámky ve zvukovém vlákně. Přehrávač proto připraví vlastní jednoúderové vzorky předem a na výstupu používá pevný počet 32 hlasů. Dvě původní sady vznikají syntézou. Na výsledné zvuky se vztahuje přiložená CC0. [Původ zvuků](../assets/drums/README.md)

Zdroj ve skladbě je standardní MIDI s časovými značkami. Pro střih, pokusy, přehrání a export se mimo zvukové vlákno připraví dočasný WAV pro danou sadu. Uzavření skladby dočasné soubory uvolní. Při změně sady zůstávají původní údery a jejich síla uložené. Při souběžném záznamu je zprvu slyšet původní modul, aby oba zvuky nehrály přes sebe.

## E11 · balíky, koš a vydání

Běžná skladba může překročit 4 GB. Pro balíky používáme miniz 3.0.2 s podporou ZIP64 a postupným zápisem a čtením souborů. Každý soubor je v balíku uvedený také s kontrolním součtem SHA-256. Poškozený přenos se nedostane mezi uložené skladby. Přidáváme i uložené verze, aby se dalo pokračovat s historií. [Licence miniz](https://github.com/richgel999/miniz/blob/3.0.2/LICENSE)

Úklid zachovává audio využité kterýmkoli dostupným pokusem, skrytým dílem, stavem Zpět nebo uloženou verzí. Vlastní přesun do systémového koše vyžaduje výslovné potvrzení v aplikaci. Při souběžné obnově se soubory nenabízejí k úklidu.

V režimech Windows Audio porovnáváme identitu zařízení vstupu a výstupu. Když ji Windows neurčí a názvy se liší, nahrávání přes stopy vyžaduje volbu společné zvukovky nebo vědomé povolení dané kombinace. Rozhraní ukáže důvod.

Instalátory jsou určené pro běžný účet, každá architektura se instaluje, spouští a odinstaluje v CI. Vydání se sestavuje z hlavní větve. Balíky se stáhnou zpět a ověří před zveřejněním. Ruční ověření na reálném vybavení se podle Jendova pokynu přesouvá za vydání v1.

## E12 · složka nápadů, historie a připojení

Jenda určil pořadí E12 až E18 a přesunul hraní celé písničky i další ruční zkoušky na konec po E18. V průběhu rozhodují o dokončení jednotlivých etap automatické zkoušky.

Nápady pokračují v dosavadní složce `Rocker 2/Napady` vedle výchozí složky skladeb. Nahrávání i náhled berou cestu ze stejné knihovny. Přesouvat nápady při každé změně cíle pro skladby by vyžadovalo migraci a zvýšilo riziko ztráty přístupu k uloženým riffům.

U historie porovnáváme načtený obsah nejnovější verze. Skutečné změny dostávají vzestupné názvy, i když vzniknou během jedné milisekundy. Poškozený poslední soubor vyvolá nový zápis. Čisté porovnání textu by mohlo považovat odlišně formátovaný JSON za novou verzi.

Zpráva o očekávané obnově nahrávky se ukáže, jen pokud je k dispozici deník. Při částečném úklidu systém uchová již přesunuté soubory v koši.

Při přepnutí frekvence z nové skladby zrušíme kalibraci pro nově aktivní kombinaci a uchováme ostatní měření. Pokud zvukovka frekvenci skladby nenabízí, Rocker zobrazí požadovanou hodnotu. Alternativou je převod celé skladby včetně historie a MIDI. Takový převod vyžaduje vlastní etapu.

## E13 · koš, M4A a balík pro Mac

Mazání nápadu potvrzuje přesun jeho WAV do systémového koše. Po přesunu katalog vynechá položku i v případě, že se index později nepodaří zapsat. Přímé mazání souboru by znemožnilo jeho vrácení z koše.

M4A pro Mac vzniká systémovým kodérem AAC 256 kb/s. V tomto kroku systémový AAC export ve Windows ponecháváme mezi rozšířeními. Windows nabízí MP3, pokud je kodér k dispozici. Import má syntetický vzorek CC0, který vznikl místně ve FFmpeg ze dvou sinusů 440 a 660 Hz. Délka importu má kvůli rozdílnému vyhodnocení začátku a konce dekodéry toleranci 50 ms. Vlastní export ověřuje přesnou délku na Macu.

Balík pro Mac má arm64 a x86_64. Sanitizéry a běžné testy běží v sadě pro Apple Silicon. Stroj s Intelem čeká na ruční zkoušku po E18.

## E14 · samostatné stopy

Pro každou stopu, případně jednu společnou sadu bicích, se použije stejný výpočet jako pro celý mix. Výsledkem je stereo WAV v plovoucí čárce a při frekvenci skladby. Vyhneme se přitom zaokrouhlení při převodu na celá čísla. Předání používá společný začátek a délku s dozvukem. Odlišná délka podle posledního klipu stopy by ztížila přenos.

Složka stop se zveřejní jediným přejmenováním po úspěšném zápisu všech WAV. Pokus s již existujícím cílem skončí bez přepisování. Tím zůstávají použitelné i dříve vyexportované stopy.

## E15 · zpěv a baskytara

Stopa používá jednoduchý třípásmový ekvalizér, kompresor a krátké odrazy. Procesor pracuje průběžně od prvního vzorku. Suchá cesta nemá přidané pevné zpoždění, takže kompenzace je nula. Předvolby se přenášejí do zvukového vlákna jako jedno atomické číslo. Paměť odrazů se připravuje před přehráváním. Náročnější konvoluce by zvýšila zpoždění a spotřebu procesoru.

Skladba ukládá volitelné pole `zvuk`. Starší skladby se otevřou s předvolbou Bez úprav. Efekty se přidávají až při přehrávání a exportu, takže je lze kdykoli změnit i u dříve nahraného WAV.

## E16 · vodicí rytmus ve sluchátkách

Vodítko střídá Klik, Rytmus a Bez kliku pod jedním tlačítkem. Delší nabídka by zabrala místo ovládání vstupů v minimální velikosti okna. Volbu ukládá stav zobrazení skladby. Odpočet si ponechává známý klik.

Ve 4/4 hraje kopák na první a třetí dobu, virbl na druhou a čtvrtou a hi-hat v osminách. V ostatních metrech zvýrazňuje rytmus první a poslední dobu. Vzorky jsou připravené před spuštěním přehrávání a rytmus jde jen do výstupu pro hráče.

## E17 · hlasitost a skutečná špička

Měření integrované hlasitosti a skutečných špiček používá libebur128 1.2.6 pod licencí MIT. Windows sestavení přikládá také licenci BSD pro jeho implementaci fronty. ARM64 používá přenosnou větev filtru. Překladač pro tuto knihovnu zapíná režim C, který podporuje její direktivu #warning. Pro cílové frekvence vývozu používá čtyřnásobný převzorek. Zkouška A-32 samostatně generuje pět signálů z tabulky 1 EBU Tech 3341. Tím lze měření spouštět v CI bez přerozdělování referenčních nahrávek EBU. [EBU Tech 3341](https://tech.ebu.ch/docs/tech/tech3341.pdf) · [Licence libebur128](https://github.com/jiixyj/libebur128/blob/v1.2.6/COPYING)

Volitelný vývoz pro telefon má střídmý omezovač s předstihem. Měření hotového souboru zahrnuje vliv převodu frekvence a kodeku. Když se u velmi krátkého nebo dynamického materiálu liší bezpečná dosažitelná hlasitost, aplikace ukáže skutečně naměřenou hodnotu. Pouhé zesílení by mohlo překročit mez špičky.

## E18 · nahrávání dokola

Zařízení zapisuje spojitě. Hudební ukazatel se vrací na začátek výběru, zatímco délka záznamu běží dál. Tím lze po zastavení nebo po pádu vyznačit jednotlivé průchody v jednom souboru na vstup. Po stisku Zastavit záznam doběhne o kompenzaci zvukovky a odposlech odehraje nastavenou dohru. Úplné průchody přebírají prolínání z opravy taktu. Rozpracovaný poslední průchod je v koši pokusů.

Tlačítko v režimu Stříhej zatím stačí. Pro budoucí zkratku připadá v úvahu Ctrl+Shift+R. Zkratku použijeme po zkoušce hraní a ověření, že nenaruší běžné nahrávání klávesou R.
