## Changelog gpiodWrapV1
### Autor: Kay Donau

Alle signifikanten Änderungen am gpiodWrapV1 werden hier dokumentiert.


## [0.1.2] 2026-11-08

> **Hotfix & Stabilitäts-Patch für Thread-Synchronisation und Lebenszyklus**

Heutige Schwerpunkte: Konstruktor, Destruktor, Signal-Handling und der Watchdog-Thread.

### Fixed
- **Watchdog-Thread (Programmende-Blockade gelöst):** Ein Logikfehler in der `while`-Schleife des Watchdogs wurde behoben. Der Thread war nicht korrekt mit seinem `running`-Flag verknüpft, was dazu führte, dass sich regulär auslaufende Programme am Ende von `main()` im Hintergrund aufgehängt haben. 
- **Race Condition beim Beenden behoben:** Hauptthread (Destruktor) und Hintergrundthread (Watchdog) wurden sauber synchronisiert. Es ist nun systemweit ausgeschlossen, dass beide Instanzen gleichzeitig oder nacheinander `closeChip()` ausführen. Das doppelte Ausführen der Aufräum-Logik ist damit vollständig behoben.

### Optimized
- **Konstruktor-Struktur:** Der Watchdog-Thread ist bei deaktiviertem Signal-Handling nun vollständig inaktiv. Für zukünftige Versionen ist vorbereitet, dass der Watchdog weitere Kontrollaufgaben übernimmt und dann standardmäßig aktiv sein wird.
- **Destruktor & Signal-Lebenszyklus:** 
  - *Szenario A (Reguläres Ende):* Läuft ein Programm bis zum Ende durch, deaktiviert der Destruktor den Watchdog kontrolliert und führt `closeChip()` aus (`Safe-by-Default`). 
  - *Szenario B (Abbruch via Strg+C):* Wird das Programm hart unterbrochen, fängt der Watchdog das Signal ab, versetzt alle Leitungen augenblicklich in den sicheren Zustand (`Safe-by-Default`) und gibt das Signal via POSIX-`raise()` (^C) sauber an das Betriebssystem zurück.
  - *Wichtiger Hinweis:* Ohne Signal-Handler können die GPIO-Lines bei einem harten Abbruch nicht in den sicheren Input-Zustand zurückgesetzt werden.

---
## [0.1.1] 2026-11-07

> **Hotfix & Stabilitäts-Patch für die Flankenerkennung und API-Sicherheit**

### Fixed
- **`getPinChange()`:** Initialisierungs-Bug behoben: Beim Starten der Anwendung wird nicht mehr fälschlicherweise ein initialer Pegelwechsel signalisiert, wenn der Pin stabil auf seinem Standard-Level liegt. Die Funktion synchronisiert sich nun beim allerersten Aufruf unsichtbar mit dem aktuellen Hardware-Zustand des Pins. Erst bei einer tatsächlichen Pegeländerung nach dem Start wird ein Event gefeuert.
- **Systemweite API-Absicherung:** Die validRequest()-Guard-Clause wurde nun konsequent in allen I/O-Pfaden (setPin, getPin, getPinChange, configurePin) aktiv geschaltet. Ungültige Pin-Offsets werden sofort am Funktionseingang abgefangen, wodurch Speicherzugriffsfehler (Segfaults) und undefiniertes Verhalten systemweit ausgeschlossen sind.
- **CMakeLists:** Strukturierte generierung der Binärdateien im Ausgabeordner.

### Optimized
- **Kompaktes Zustands-Tracking (`LineChange`):** 
  Einführung einer Struktur zur Verfolgung von Pegeländerungen mittels C++ Bitfields. Der aktuelle Zustand (`current`) und das Initialisierungs-Flag (`received`) belegen zusammen **nur 2 Bits** im Speicher. 
  Das gesamte `line_change`-Array für alle verfügbaren GPIO-Lines belegt dadurch nur minimale Bytes im RAM, was maximale CPU-Cache-Effizienz garantiert und jeglichen Heap-Overhead ausschließt.

---
## [0.1.0] 2026-11-06

Initiales Release (v0.1.0) der libgpiod V1-Erweiterung.
Maßgeschneidert auf maximale Performance und Stabilität für ältere Linux-Systeme 
und Single-Core-Hardware wie den Raspberry Pi 1 (ARMv6) unter Bookworm (libgpiod v1.6.3).

### Features und Optimierungen
- **Feste Datenstrukturen statt Maps:** Jede GPIO-Line hat ihren eigenen, festen Speicherbereich (`static_line_data`). Das Layout wurde so optimiert, dass die Settings ideal in den CPU-Cache geladen werden können.
- **ARMv6 Lock-Free Signal-Handling:** Die Signal-Flags verwenden `std::atomic<uint32_t>`, um auf dem alten Pi 1 (ARMv6) garantiert über `LDREX`/`STREX` echte Lock-Free-Operationen zu nutzen, anstatt teure Mutex-Sperren zu erzwingen.
- **Kompaktes PinMode-Design:** Vereint logische Richtung und physikalische Hardware-Widerstände (Pull-Up/Pull-Down) in einer einzigen, selbsterklärenden Enum-Kombination (z. B. `PULLUP`, `OPEN_DRAIN`).
- **Natives Kernel-Debouncing:** Die Entprellung nutzt die native `gpiod_line_set_config()`-Funktion des Kernels in Mikrosekunden-Präzision (µs).
- **Zentraler Build-Ausgabeordner:** Das CMake-System wurde so optimiert, dass alle kompilierten Binärdateien der Beispiele automatisch in einem zentralen `/bin`-Ordner landen.

### Added (API v0.1.0)
- `configurePin()`: Richtet einen GPIO-Pin (Offset) mit kombiniertem `PinMode` und optionaler Entprellzeit (Debounce in ms) ein.
- `setPin()`: Setzt den logischen Pegel eines Ausgangs-Pins (`HIGH`/`LOW`) hocheffizient direkt auf dem V1 `gpiod_line`-Handle.
- `getPin()`: Liest den aktuellen Hardware-Zustand eines Pins aus.
- `getPinChange()`: Zustandsüberwachung, die nur dann anschlägt und einen Wert zurückgibt (`std::optional`), wenn ein tatsächlicher Pegelwechsel stattgefunden hat.
- `findChip()`: Automatischer Suchlauf im `/dev`-Verzeichnis nach bekannten Pi-GPIO-Chips (`pinctrl-rp1`, `rp1-gpio`, `pcm2712` etc.) anhand ihrer Driver-Label.
- `closeChip()`: Schließt den Chip sicher, beendet den Watchdog und versetzt alle aktiven Pins über `safeStateLine()` automatisch in einen hochohmigen, sicheren `INPUT`-Zustand zurück.

### Signal Handling
- Integrierter Watchdog-Thread und Signal-Handler (`SIGINT`, `SIGTERM`) zur automatischen Ressourcenfreigabe bei Programmabbruch. Kann über das CMake-Flag `GPIODWRAPV1_DISABLE_SIGNALS` vor dem Kompilieren komplett deaktiviert werden.
