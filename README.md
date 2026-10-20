# Shapetrator

Effekt plugin (VST3) FL Studiohoz: egy Infiltrator-szerű, lépésszekvenciás hangformáló, ShaperBox-szerű rajzolható görbékkel, de egyszerű felülettel. Arra a mixer sávra hat, amire betöltöd.

## Mit tud?

Négy sáv, mindegyikhez 16 rajzolható lépés. A mintázat a lejátszó tempójához szinkronban fut.

| Sáv | Mit csinál a lépés magassága |
|---|---|
| **Volume** | magasabb oszlop = hangosabb (alacsony = elnémul, gate / pumpálás) |
| **Filter** | az oszlop a vágási frekvenciát állítja (alul mély, felül magas) |
| **Pan** | középvonal = közép, fel = jobb, le = bal |
| **Crush** | magasabb oszlop = erősebb bit- és mintavétel-csökkentés |

Vezérlők:

- **Depth** (minden sávnál): mennyire érvényesüljön a sáv. 0% = a sáv ki van kapcsolva.
- **Rate**: lépés hossza (1/2 ... 1/32, triolák is).
- **Steps**: hány lépésből álljon a minta (2-16).
- **Smooth**: 0% = éles lépcső, 100% = lágy átmenet a lépések között.
- **Reso / Filter**: szűrő rezonancia és típus (Lowpass, Highpass, Bandpass).
- **Mix**: dry/wet.
- **RANDOM**: véletlen preset (új mintázatok, mélységek, ráta, simítás, szűrőtípus). A Mix-et nem változtatja. Nyomkodd, amíg tetszik valami.

Rajzolás: kattints és húzd az egeret az oszlopokon. Dupla kattintás egy oszlopon visszaállítja az alapértéket.

## Telepítés FL Studióba

1. Másold a `Shapetrator.vst3` mappát ide: `C:\Program Files\Common Files\VST3`
2. FL Studio: **Options > Manage plugins > Find installed plugins** (vagy a Plugin Database-ben **Refresh**).
3. Mixer sávon egy insert slotban válaszd ki: **Shapetrator**. (Effektként tölthető be, nem generátorként.)

## Fordítás

### A) GitHub Actions (nem kell semmit telepítened)

1. Csinálj egy GitHub repót, és töltsd fel ennek a mappának a tartalmát.
2. Az **Actions** fülön lefut a "Build VST3" (kb. 10-15 perc, a JUCE-t is letölti).
3. A lefutott futás alján, az **Artifacts** résznél töltsd le a `Shapetrator-VST3-windows-latest` csomagot.

### B) Helyben Windowson

Kell hozzá: Visual Studio 2022 (C++ asztali fejlesztés) és CMake.

```
cmake -B build
cmake --build build --config Release
```

Az eredmény: `build\Shapetrator_artefacts\Release\VST3\Shapetrator.vst3`

Ha azt szeretnéd, hogy a build után magától bemásolja a rendszer VST3 mappájába, adj hozzá `-DSHAPETRATOR_COPY_AFTER_BUILD=ON` kapcsolót (rendszergazdai jog kell hozzá).

## Megjegyzés

A JUCE keretrendszer AGPLv3 / kereskedelmi licenc alatt áll. Saját használatra ez nem jelent gondot, ha viszont terjeszted a plugint, ennek megfelelően kell eljárnod.
