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
| Zaseknutý disk | Fronta má deset sekund na stopu. Při zaplnění Rocker ukončí průchod, dokončí zapsatelné vzorky a označí místo výpadku na časové ose. | Pokračování se zaznamenanou mezerou by vyžadovalo odlišit přerušené úseky jednoho pokusu. |
| Čtení nahrávek | Soubory WAV se mapují do paměti. Pracovní vlákno načítá stránky před ukazatelem. Při otevření se vypočítá obálka průběhu po 256 vzorcích. | Samostatná mezipaměť náhledů na disku může zkrátit otevírání dlouhých projektů. |
| Zkouška souhry | Diagnostika použije dočasnou skladbu, skutečný výstup a stejnou záznamovou cestu jako nahrávání. Po změření posunu na začátku a konci dočasné soubory uklidí. | Měření v otevřené skladbě by vyžadovalo přidat a následně odstranit zkušební stopy. |

M-02, H-02, M-04 i H-10 proběhnou na vydané aplikaci podle Jendova rozhodnutí.
