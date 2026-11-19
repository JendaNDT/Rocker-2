# Rocker 2

Domácí studio pro kytaru a bicí. Můžeš nahrát bicí, dohrát k nim kytaru a druhý den změnit její zvuk přímo ve skladbě. Aparát, ladička a měření zvukovky jsou k dispozici. Aplikace běží bez internetu a účtu.

Ruční zkoušky podle níže uvedených postupů proběhnou až po vydání hotové aplikace. Během vývoje se každá etapa ověřuje automaticky.

## Stažení v1.0.0 a spuštění

| Počítač | Co stáhnout |
|---|---|
| Windows, Intel nebo AMD | [Instalátor x64](https://github.com/JendaNDT/Rocker-2/releases/download/v1.0.0/Rocker2-Setup-x64.exe) nebo [přenosný ZIP](https://github.com/JendaNDT/Rocker-2/releases/download/v1.0.0/Rocker2-Windows-x64.zip) |
| Windows, procesor ARM (například Snapdragon) | [Instalátor ARM64](https://github.com/JendaNDT/Rocker-2/releases/download/v1.0.0/Rocker2-Setup-ARM64.exe) nebo [přenosný ZIP](https://github.com/JendaNDT/Rocker-2/releases/download/v1.0.0/Rocker2-Windows-ARM64.zip) |
| Mac s čipem Apple | [ZIP s aplikací pro Mac](https://github.com/JendaNDT/Rocker-2/releases/download/v1.0.0/Rocker2-Mac.zip) |

1. Ve Windows najdeš typ procesoru v **Nastavení → Systém → O systému → Typ systému**. Spusť odpovídající instalátor pod svým běžným účtem. Používáš-li ZIP, rozbal jej a otevři **Rocker 2.exe**. U SmartScreenu při spuštění souboru z tohoto vydání otevři **Další informace → Přesto spustit**. Při odinstalování zůstanou tvoje skladby na disku.
2. Na Macu rozbal ZIP, přesuň **Rocker 2.app** do Aplikací a spusť ji. Povol vstup mikrofonu pro nahrávání a měřiče. Pokud macOS zastaví první spuštění, otevři **Nastavení systému → Soukromí a zabezpečení → Přesto otevřít**.
3. V **Zapojení** vyber zvukovku a vyzkoušej tón ve sluchátkách. Na úvod se dostaneš tlačítkem **Úvodní obrazovka**. [Vydání, snímky a kontrolní součty](https://github.com/JendaNDT/Rocker-2/releases/tag/v1.0.0)
4. Vývojová sestavení jsou také na GitHub Actions. Otevři zelený běh **Rocker 2**, dole v **Artifacts** zvol aplikaci nebo snímky pro svůj systém. GitHub při stahování z Actions vyžaduje přihlášení. [Běhy sestavení](https://github.com/JendaNDT/Rocker-2/actions/workflows/ci.yml)

Novější vývojový balík pro Mac z E13 obsahuje i verzi pro Intel. Na stránce sestavení dole vyber balík **Rocker2-Mac**. V nabídce **Poslat mix…** na Macu zvol **M4A (AAC) · 256 kb/s**, pokud chceš menší soubor k poslechu. [Ověřený balík E13](https://github.com/JendaNDT/Rocker-2/pull/16)

## Přenos skladby a kontrola sestavy

1. Ve skladbě otevři **Soubor → Zabalit skladbu** a ulož jeden soubor s příponou **.rocker2**. Obsahuje skladbu, zobrazení, WAV, MIDI a uložené verze. Na druhém počítači zvol **Otevřít balík** nebo přetáhni soubor na úvod. Rocker zkontroluje obsah a vytvoří novou složku skladby.
2. Pokud se jména vstupů liší, u každé stopy vyber zásuvku nové zvukovky. Přiřazení můžeš odložit nebo znovu otevřít přes **Soubor → Přiřadit vstupy**.
3. V **Soubor → Uklidit skladbu** zkontroluj počet nepoužitých souborů. Teprve **Přesunout do koše** pošle soubory do systémového koše. Nahrávky potřebné pro pokusy, díly, Zpět a uložené verze zůstanou.
4. Pod 30 minut volného místa pro právě připravené vstupy se objeví varování. Pod rezervou 500 MB Rocker záznam zastaví a dokončí zápis. Při odpojení zvukovky otevři Zapojení, připoj ji a zkontroluj vstupy.
5. V **Hraj → Diagnostika** najdeš **Kontrolu sestavy**. **Uložit zprávu** vytvoří místní textový soubor pro Jendu. **Složka skladeb** vybere místo pro nové skladby, například místní složku mimo OneDrive. Při různých vstupních a výstupních zařízeních Windows Audio nejprve vyber jednu zvukovku pro oba směry.

Pokud se frekvence přenesené skladby liší od nastavení zvukovky, Rocker ukáže obě hodnoty. Nabízí-li zvukovka požadovanou frekvenci, použij přímo ve skladbě **Přepnout zvukovku na …**. Po přepnutí můžeš znovu změřit zpoždění v Diagnostice.

**Složka skladeb** určuje místo pro nové skladby. Nápady dál najdeš ve stejné knihovně a ze zachyceného riffu můžeš vytvořit skladbu v nově vybrané složce.

V Míchej otevři **Poslat mix… → Stopy zvlášť → Uložit stopy**. V `Exporty` vznikne složka s WAV soubory. V jiném studiu je vlož všechny na začátek projektu. Mají stejnou délku, včetně dozvuku. Bicí ze společné soupravy jsou v jednom stereo souboru. Ztlumené stopy se vynechají. Soubory mají hlasitost a polohu podle jednotlivých stop. Celkovou hlasitost výstupu nastavíš v cílovém studiu.

## Co vyzkoušet po vydání

- **Windows a Mac**. Spusť aplikaci, porovnej úvod se snímkem, v Zapojení vyber ASIO nebo CoreAudio a poslechni tón vlevo, vpravo a v obou sluchátkách. Kytara ve vstupu 1 má rozhýbat pouze jeho měřič. Přebuzení se označí **PŘEBUZENO**.
- **Celá píseň**. Zachyť nápad, vytvoř skladbu, nahraj bicí a kytaru, oprav jeden takt, zopakuj refrén a pošli výsledný mix. Poznamenej si první místo, kde sis nebyl jistý ovládáním. Na další den si ověř návrat ke stejnému taktu a mixu.
- **Přenos**. Zabal skladbu a otevři kopii na druhém počítači. Do původní složky můžeš dál nahrávat.
- **Volitelně**. Zkus notebook bez externí zvukovky. Pokud máš modul MIDI, vyzkoušej naučení padů, stereo odposlech a změnu sady. [Úplný plán ručních zkoušek a měření](docs/03_ZKOUSKY_NA_WINDOWS.md)

## Jak nahrát elektronické bicí přes MIDI

1. Připoj modul USB nebo MIDI a zvukový výstup pro sluchátka. V **Zapojení → Vstupy → Bicí (MIDI)** vyber modul. Výchozí rozložení padů je General MIDI. Když některý pad hraje jiný buben, zvol **Naučit pady** a postupně udeř do ukázaných osmi padů.
2. K záznamu úderů zvol **Údery MIDI** a vytvoř skladbu. Ve stávající skladbě lze přidat stopu přes **Přidat nastavené bicí**. Zapni na ní nahrávání a stiskni R. Sluchátka připoj ke zvukovce.
3. Chceš-li také původní zvuk modulu, nastav v **Zahraj na bicí** stereo dvojici a potom u MIDI zvol **Zvuk modulu i MIDI**. Obě stopy se připraví společně. **Poslech MIDI** a **Poslech modulu** přepínají, kterou slyšíš.
4. Vyber stopu **Bicí (MIDI)**, otevři nastavení stopy a změň **Zvuk sady**. Původní údery zůstanou uložené. Pokusy, díly, střih i export používají vybranou sadu.
5. Pro měření společně nahraj 16 oddělených úderů do virblu. V Diagnostice otevři **Porovnat MIDI a zvuk**. Změřenou korekci Rocker použije pro další záznam se stejným modulem a nastavením zvukovky.

## Jak připravit bicí

1. V **Zapojení → Vstupy** zvol **Zahraj na bicí**. Udeř postupně do všech bubnů tak silně, jak budeš hrát. Pak klikni na **Vyhodnotit údery**. U každého vstupu zkontroluj špičku. Při −6 dBFS nebo výš stáhni gain na zvukovce a měření zopakuj.
2. S jedním mikrofonem začni přibližně 1–1,5 m před soupravou ve výšce hlavy. U stereo modulu zapoj jeho L a R do dvou vstupů a sluchátka do zvukovky. Se dvěma signály můžeš vybrat také dva samostatné mikrofony. U více mikrofonů přiřaď názvy, vstupy a polohy overheadů.
3. Zvol **Použít pro nové skladby**. V rozpracované skladbě přidáš tutéž sestavu přes přidání stopy a **Přidat nastavené bicí**. Zapnutí záznamu na jednom mikrofonu připraví celou sadu. Společný střih zachová vzájemné načasování. V režimu **Míchej** vyber mikrofon ze sady a použij **Bicí celkem**.
4. **F11** otevře Pohled zdálky. Před záznamem můžeš nastavit cílový takt. Odpočet je jantarový, záznam červený a úspěšné uložení zelené. **Esc** a **F11** tě vrátí ke stopám i během hraní. Záznam zastaví **R** nebo **Enter**.
5. Na numerické klávesnici **0** přehrává a zastavuje, **Enter** nahrává a zastavuje, **+** přepíná pokusy ve vybraném úseku a **−** vrací ukazatel o takt. Pro USB spínač zvol v prvním kroku Zapojení **Nožní spínač** a sešlápni jej. Odteď stejným stiskem spustíš a zastavíš záznam.

## Jak zachytit riff

1. V úvodu nebo v knihovně **Nápady** stiskni **R**. První dvě sekundy před stiskem se přidají k nahrávce. Při **R** nebo mezerníku se záznam zastaví a uloží. Ve sluchátkách můžeš slyšet poslední zvuk aparátu. Při náhradním mikrofonu notebooku zůstane odposlech vypnutý.
2. Pokud už jsi riff dohrál, stiskni **U — Ulož, co jsem hrál**. Pojistka uchovává místně poslední dvě minuty vybraného vstupu. Vypneš ji v Zapojení u vstupů nebo přímo v knihovně. Změna zapojení začíná novou historii.
3. U povedeného riffu klikni na **★ Povedené**, připiš poznámku a klikni na **Uložit poznámku**. V knihovně můžeš vyhledat část poznámky a zapnout **Jen ★**. Náhled průběhu, čas a ladění pomohou najít správný záznam. Chceš-li uvolnit místo, zvol **Smazat nápad** a potvrď přesun do systémového koše.
4. U volně hraného riffu zvol **Doťukat tempo**. Jakmile se přehrává, ťukni osmkrát mezerníkem, poprvé na první dobu taktu. Potom zvol **Udělat z toho skladbu**. Ve skladbě bude první stopa Kytara a celý původní riff. Před jeho začátkem může vzniknout krátké místo, aby první doba seděla na začátek taktu.
5. V režimu **Hraj** zvol **Podklad: od začátku** nebo **Podklad: na ukazatel** a klikni na **Přidat podklad**. Můžeš také přetáhnout soubor do okna skladby. Pro WAV, AIFF, FLAC, Ogg Vorbis, Ogg Opus a MP3 proběhne převod automaticky. Soubory M4A a AAC využívají dekodér Windows nebo macOS. Vybraný podklad posuneš šipkou o 1 ms, se Shiftem o 10 ms. Importovaný zvuk zůstane ve složce skladby i po přesunu původního souboru.

## Osm taktů bicích a kytara přes ně

1. Na úvodu zvol **Nová skladba** nebo stiskni **N**. Pojmenuj ji, nastav **112 BPM**, metrum **4/4** a klikni na **Vytvořit skladbu**. Vzniknou stopy Bicí a Kytara. Rytmus můžeš také několikrát vyťukat tlačítkem **Naťukat tempo**.
2. Vyber **Bicí → Nastavit stopu**. Zvol skutečný vstup mikrofonu nebo dva sousední vstupy pro stereo modul. Stiskni **Potvrdit**, potom **Připravit**. Rozsvícené **Připraveno** označuje stopu, do které půjde záznam. Podobně lze připravit více stop současně.
3. Zapni **Klik** a **Odpočet**. **F11** zvětší stav a číslo taktu pro hráče za bicími. Stiskni **R**, po čtyřech dobách nahraj osm taktů a dalším **R** zastav. **F11** tě vrátí na časovou osu. Mezera přehrává a zastavuje.
4. U bicích vypni **Připraveno**, zvol **Na začátek** a vyber **Kytara**. Nastav její vstup, připrav ji a zvol odposlech **Se zvukem**. S drátovými sluchátky ve zvukovce a přímým odposlechem přepnutým na USB/Playback stiskni **R**. Uslyšíš bicí a živý aparát. Do nové stopy se uloží čistá kytara. Při přehrávání použije kytarová stopa vlastní nastavení aparátu.
5. Kliknutím do pravítka umístíš ukazatel. Šipky vlevo/vpravo přejdou na sousední takt, **+** a **−** mění přiblížení. Vyzkoušej **Ztlumit** a **Sólo**. Uložení probíhá automaticky. Po zavření otevři **Pokračovat** a ověř polohu i výběr stopy. Každá skladba má vlastní složku v **Rocker 2/Skladby** v domovské složce počítače.

## Jak poslat mix

1. Otevři skladbu a stiskni **3 Míchej**. Pusť přehrávání mezerníkem. V řádku kytary uprav hlasitost a polohu ve sluchátkách. Dvojklik vrátí jezdec na 0 dB nebo doprostřed. **S** nechá znít vybranou stopu, **M** ji ztlumí. Nahoře uvidíš jména stop se zapnutým sólem.
2. Dole sleduj **Celkem**. U značky **PŘEBUZENÝ MIX** klikni na takt a uber celkovou hlasitost. **PŘEBUZENÝ VSTUP** označuje přebuzení při nahrávání. V tom případě uber vstupní zesílení na zvukovce a nahraj úsek znovu.
3. Stiskni **Ctrl+E**. Vyber celou skladbu nebo takty označené v režimu Stříhej. WAV 24 bit / 48 kHz se hodí pro další práci, WAV 16 bit / 44,1 kHz pro běžné přehrávače. Pro telefon můžeš ve Windows vybrat **MP3 · 320 kb/s**, pokud je systémový kodér dostupný.
4. Klikni na **Vytvořit soubor**. Pokud zůstalo zapnuté sólo, potvrď **Exportovat se sólem**, nebo se vrať a vypni ho. Po dokončení se otevře složka s výsledkem. Tlačítko **Přehrát výsledný soubor** ho otevře v přehrávači. Při překročení 0 dBFS uvidíš takt a nabídku ubrat hlasitost a opakovat export.
5. **Po vydání** porovnej poslech ve studiu s výsledkem. Pro M-03 exportuj WAV se zapnutým klikem, v Diagnostice zvol **Hledat klik v souboru** a vyber WAV při stejném tempu skladby. Pro M-13 otevři MP3 ve Windows i v telefonu a ověř začátek, konec a délku. [Sestavení a snímky E7](https://github.com/JendaNDT/Rocker-2/pull/8)

## Jak zopakovat refrén

1. Otevři skladbu se šestnácti takty a stiskni **2 Stříhej**. Oddal pohled tak, abys viděl alespoň 24 taktů.
2. Klikni na barevnou pásku **Díly** nad hranicí mezi takty 8 a 9 a zvol **Rozdělit díl zde**. Uvidíš Sloku a Refrén. Ve spodní desce můžeš změnit název i barvu pásky.
3. Vyber pásku Refrén a stiskni **D**. Druhý refrén ponese značku **⧉**. Podržením Ctrl při tažení pásky také vložíš spojený výskyt. Tažení za střed bez Ctrl přesune díl v pořadí.
4. Chyť pravý okraj druhého refrénu a táhni ho o dva takty doleva. Uvidíš **6 z 8**, za hranou šrafovaný konec. První refrén má stále osm taktů. Následující díly se posunou o dva takty blíž.
5. Délku vrátíš přes **Ctrl+Z**, tažením okraje zpět nebo pravým tlačítkem na pásku a volbou **Obnovit celou délku**.
6. Na pravítku vyber takt ve spojeném refrénu a stiskni **P**. **Opravit oba** použije nové hraní v obou refrénech. **Jen tento výskyt** vytvoří samostatný refrén. [Snímek pásu Díly](docs/zkousky/E6/dily.png)

## Jak opravit jediný takt

1. V otevřené skladbě vyber kytarovou stopu a její vstup, potom stiskni **2 Stříhej**. Klikni na číslo **6** na pravítku. Ve spodní části můžeš změnit **Předstih** a **Dohru**.
2. Stiskni **P**. Při výchozím nastavení začne skladba hrát od taktu 4. Po zapnutém odpočtu hraj s nahrávkou a oprav chybu v taktu 6. Taktem 7 průchod končí.
3. Rocker přehraje takty 5–7 s opravou. **Tab** přepíná mezi původním hraním a opravou za běhu. Další **P** přidá další pokus. **Enter** ponechá právě poslouchaný pokus.
4. Ke všem pokusům se dostaneš přes štítek nad opraveným taktem. Jeho levý nebo pravý okraj můžeš přetáhnout. Se stisknutým **Alt** se pohybuje volně. **Zahodit pokus** jej přesune do koše. Po zastavení jej vrátí **Vrátit z koše**.
5. Při H-04 po vydání změř, kolik času uběhne od objevení chyby po výběr opravy. Cíl je do jedné minuty. Poslechni si oba okraje opravy i doznívání zkreslené kytary. [Snímek Pokusů](docs/zkousky/E5/pokusy.png)

## Jak vystřihnout a přesunout dva takty

1. Otevři nahranou skladbu, zastav přehrávání a stiskni **2** pro **Stříhej**. Klikni na název stopy, kterou chceš upravit. **Oddálit** nebo **−** zobrazí víc taktů. **Další takty** posune pohled doprava.
2. Klikni na číslo **5** a se stisknutým **Shift** klikni na **6**. Modrý pruh pokryje oba takty. Chyť výběr uvnitř průběhu vybrané stopy a táhni jeho začátek na **9**. Výběr se přichytí na dobu. **Alt** při tažení umožní volnou polohu.
3. Stiskni **D**. Kopie obou taktů se objeví hned za přesunutým úsekem. Přesun, opakování a vložení na obsazené místo nahradí zvuk vybrané stopy v cílovém rozsahu. **Ctrl+Z** vrací jednotlivé kroky, **Ctrl+Y** je provede znovu.
4. Kliknutím do klipu umístíš ukazatel a klávesou **X** klip rozstřihneš. **Ctrl+C** kopíruje výběr, **Ctrl+V** vloží kopii na ukazatel vybrané stopy. **Delete** výběr smaže. Každá úprava ponechá zdrojový WAV na disku.
5. Chyť levý nebo pravý okraj klipu. Nad ukazatelem se otevře čtyřnásobná lupa s průběhem, mřížkou a polohou okraje. Okraj můžeš vytáhnout až na hranici původní nahrávky.
6. V nabídce **Soubor** vyber dřívější datum a čas a stiskni **Obnovit tuto verzi**. Rocker před obnovením uloží i současný stav. Posledních 50 verzí najdeš i po novém spuštění. [Výběr taktů](docs/zkousky/E4/strihej.png) · [Lupa při tažení](docs/zkousky/E4/lupa.png)

**Ruční zkouška E4 po vydání.** Na vlastní nahrávce proveď kroky 1–3, přehraj přechod a dvakrát stiskni Ctrl+Z. Zapiš, zda šel vybraný úsek umístit přesně na začátek taktu 9, jestli se vrátil původní poslech a co při ovládání překáželo.

## Jak změnit zvuk nahrané kytary

1. V otevřené skladbě spusť přehrávání a klikni na štítek **Crunch** pod názvem kytarové stopy. Ve skladbě z E2 tam najdeš **Bez aparátu**. Ve spodní desce vyber **Metal**. Změnu uslyšíš za běhu.
2. Posuň **Zkreslení** nebo přidej **Ozvěnu** a **Dozvuk**. Hvězdička u názvu označuje upravený zvuk. Volba **Bez aparátu** pustí původní nahrávku.
3. Pro další skladby zvol **Uložit jako předvolbu**, napiš nový název a stiskni **Uložit předvolbu**. Samotná skladba si pamatuje i úpravy, které si do knihovny nepřidáš. **Zpět do skladby** zavře spodní desku. [Snímek kytary při přehrávání](docs/zkousky/E3/zvuk-kytary.png)

### Ruční zkoušky E3 po vydání

| Zkouška | Co udělat a zapsat |
|---|---|
| H-03 · zvuk | Nahraj krátký riff s drátovými sluchátky ve zvukovce. Vyzkoušej **Čistý, Crunch, Lead a Metal**. Každý zvuk ohodnoť 1–5 a napiš, co bys upravil. Alespoň jeden má dostat 4 nebo 5. Během přehrávání přepni Crunch → Metal → Crunch, přidej ozvěnu, zavři skladbu a přes **Pokračovat** si ji pusť znovu. |
| M-05 · výkon | Ve Windows nastav ASIO, 48 kHz a buffer 128. V **Diagnostice** nastav **Aparáty** na 8 a spusť **Zkouška výkonu**. Přehraje se deset minut na šestnácti stopách. Zapiš zařízení, procesor, nejvyšší zobrazenou zátěž a počet výpadků. Cíl je pod 50 % a 0 výpadků. Pokud ovladač počítadlo neposkytne, zapiš „ovladač nehlásí“ a poznamenej slyšitelné výpadky. Zopakuj na baterii a na síťovém napájení. Při potížích sniž počet aparátů a zapiš poslední použitelný počet. |

## Ruční ověření E2 po vydání

| Zkouška | Postup a co zapsat |
|---|---|
| M-02 · souhra | Se staženým výstupem a vypnutým DIRECT MONITOR propoj levý linkový výstup se vstupem jako při M-01. V Diagnostice vyber tento vstup a spusť **Zkouška souhry**. Deset minut ponech kabel zapojený. Zapiš oba posuny. Na začátku i konci má být velikost posunu nejvýše 1 ms a jejich rozdíl nejvýše 0,1 ms. Zopakuj po restartu a při jiném bufferu s jeho vlastní kalibrací. Před hraním odpoj kabel. |
| H-02 · dohrání kytary | Osm taktů podle postupu výše, potom delší verze kolem pěti minut. Poslechni spojení kytary s kopákem na začátku i na konci. Zapiš, zda se objevil dvojitý úder. |
| M-04 · obnova po pádu | Založ novou zkušební skladbu a začni nahrávat kytaru. V čase 1:30 ukonči Rocker ve Správci úloh. Po spuštění zvol **Obnovit**. Zapiš délku, polohu a zda klip hraje. Očekáváme aspoň 1:28 zvuku. |
| H-10 · druhý den | Poznamenej si takt, zvolenou stopu, přiblížení, pohled a stav Sólo/Ztlumit. Zavři aplikaci, restartuj počítač a zvol **Pokračovat**. Porovnej polohu, pohled a poslech. |

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

Jendo, nech kamaráda zahrát stejný šestnáctinový riff přes **Crunch** s metronomem. Mezi pokusy změň pořadí dostupných bufferů 64, 128 a 256 tak, aby hráč nevěděl, který poslouchá. Po změně ověř vstup a znovu zapni **Se zvukem**. Ke každému pokusu si před prozrazením nastavení vyžádej známku 1–5. **5** znamená „žádné zpoždění necítím“. Zapiš i praskání nebo dvojitý zvuk a hodnotu z panelu **Co slyším**.

Při zpoždění do 10 ms má být hodnocení alespoň 4. Z vyhovujících výsledků vybereme největší buffer s hodnocením 4 nebo 5. Stačí mi poslat údaje zprávou ve tvaru níže, zapíšu je za tebe.

| Počítač a zvukovka | Ovladač, kHz, buffer | Pět měření M-01 ve vzorcích | Odhad / rozdíl ve vzorcích | Změřeno v ms / s aparátem v ms | Známka H-01 a co bylo slyšet |
|---|---|---|---|---|---|
| | | | | | |

## Kontrolní seznam pro první zapojení

- [ ] Porovnej úvodní obrazovku s 01 / 5.7. Zkus **R**, **N**, **Enter** a **T**. Klávesa T otevře ladičku a dalším stiskem tě vrátí. N založí skladbu, Enter otevře poslední skladbu a R na úvodu ukáže připravované zachycení nápadu.
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
