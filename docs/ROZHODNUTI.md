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
