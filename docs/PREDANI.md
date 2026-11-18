# Předání E1 · kytara do sluchátek

## 1. Hotovo

Vyzkoušej **Hraj → Přímo do zvukovky → Se zvukem**, potom **Zvuk kytary → Crunch**. **T** otevře ladičku s devíti laděními a vlastními strunami. **Diagnostika → Změřit zpoždění** uloží úspěšnou kalibraci a při návratu ke stejnému zařízení, frekvenci a bufferu ji znovu zobrazí. Měřiče ukazují vstup i poslechový výstup. [Návod pro Jendu](../README.md)

Místně prošlo 12 automatických zkoušek. Z GitHubu vyber zelený běh pro etapa-1, otevři Artifacts a vyber Rocker2 pro svůj počítač nebo Snimky s 18 obrázky. [Běhy s aplikacemi a snímky pro E1](https://github.com/JendaNDT/Rocker-2/actions/workflows/ci.yml?query=branch%3Aetapa-1) · [Hraní](zkousky/E1/hrani.png) · [Ladička](zkousky/E1/ladicka.png)

## 2. Zbývá

K přijetí E1 chybí kabelové měření M-01 a hraní H-01 na kamarádově sestavě. Tovární volba 128 vzorků slouží pro první spuštění u zařízení, které ji podporuje. Konečný výchozí buffer určí hodnocení hráče. Také první poslech E0 zůstává otevřený. [Stav zkoušek](STAV.md)

## 3. Ruční zkoušky

| Zkouška | Výsledek |
|---|---|
| E0 · ASIO, CoreAudio, tón, měřiče a odpojení | čeká na Jendu · čeká na ruční zkoušku |
| E1 · Crunch, Drop D / B, zvuk brány a přímý odposlech | čeká na Jendu · čeká na ruční zkoušku |
| M-01 · pět opakování pro každý dostupný režim | čeká na Jendu · čeká na ruční zkoušku |
| H-01 · slepé hodnocení riffu při různých bufferech | čeká na Jendu a hráče · čeká na ruční zkoušku |
| Kontrola čitelnosti a rozmístění na Jendově obrazovce | čeká na Jendu |

Postup, tabulka pro čísla a význam známek jsou přímo v návodu. Výsledky může Jenda poslat zprávou. [M-01 a H-01](../README.md)

## 4. Rozhodnutí

Impulsní odezva reproboxu pochází z vlastního výpočtu. Pro H-01 použij externí metronom. E1 navazuje na otevřený návrh E0. Pokud i přes ASIO hraní ruší, zapiš konkrétní měření a s Jendou vyber přímý odposlech zvukovky s aparátem při přehrávání. [Rozhodnutí E1](ROZHODNUTI.md)

## 5. Prompt pro etapu E2

Jsi hlavní vývojář Rockeru 2, českého domácího studia pro Windows. Jenda a jeho kamarád hrají, zkoušejí a schvalují. Ty píšeš kód, testy a dokumentaci. Začni plánem do deseti bodů.

### Podklady

Přečti celé dokumenty 01–05, STAV.md, ROZHODNUTI.md a PREDANI.md. E2 je vymezená dokumentem 04. Zkontroluj výsledky E0 a E1 a doplň všechna měření od Jendy. Nezaměňuj čísla simulace za měření zvukovky.

### Závazné podmínky

**Vzhled a rozložení od nuly, bez inspirace původním Rockerem, podle 01 / kap. 5.** Původní Rocker neměň ani neprohlížej jeho vzhled, navigaci či styly. C++20, JUCE 9.0.5, vlastní zvukové jádro, Catch2, české texty na jednom místě, anglický kód a komentáře. JUCE komponenty s vlastním kreslením a společnými tokeny, AGPLv3 a licence vložených písem. Provoz offline, bez účtu a telemetrie. Zachovej ASIO, režimy Windows Audio i CoreAudio. V callbacku žádné alokace, zámky, soubory nebo logování. Výběr zdrojů podle skutečného vybavení. Zapiš odchylky a alternativy do ROZHODNUTI.md a řekni je Jendovi.

### Úkol

1. Skladba podle 02 / kap. 5 s jedním dílem. Přidat, pojmenovat a nastavit stopy, druh a vstup, připravit k nahrávání, Ztlumit a Sólo.
2. Tempo, metrum, vyťukání tempa, odpočet a klik jen do sluchátek. Přehrávání a nahrávání při poslechu stop s kompenzací zpoždění. Kytara se ukládá čistě i při odposlechu přes Crunch.
3. Časová osa, pravítko taktů, průběhy a ukazatel. Stavový pás, červený rám při nahrávání a Pohled zdálky s taktem a odpočtem.
4. Atomické automatické uložení, zobrazeni.json, Pokračovat na stejném místě. Deník záchrany, obnova po pádu a zákaz spánku při nahrávání.
5. Diagnostika → Zkouška souhry pro M-02. Vnitřní offline render pro A-06.

### Ověření

Zachovej E0/E1, A-17, snímky a sestavení Windows x64, Windows ARM64 i macOS. Proveď A-03, A-04, A-06 až A-09, A-14 a A-15. Připrav Jendovi M-02 s kabelem, H-02 s kytarou přes bicí, M-04 po ukončení úlohy a H-10 po restartu. Předveď osm taktů bicích při 112 BPM, přehrání s kytarou, znovuotevření a záchranu nahrávky. Při přetrvávajícím selhání M-02 nebo A-07 po opravě zastav, zdokumentuj výsledek a s Jendou rozhodni o přechodu na Tracktion Engine. M-01, H-01, M-02 a H-02 musejí být provedené před pokračováním za E3.

### Předání

Pracuj ve větvi etapa-2 po malých commitech. Otevři český pull request s odkazem na ověřený běh, aplikace a snímky. Aktualizuj STAV.md a ROZHODNUTI.md. PREDANI.md má pořadí Hotovo, Zbývá, Ruční zkoušky, Rozhodnutí, Prompt pro E3 podle 04. Neověřené hraní označ „čeká na ruční zkoušku“. Závěr pro Jendu česky na 5–10 řádků.
