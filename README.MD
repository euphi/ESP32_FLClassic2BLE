# ESP32_FLClassic2BLE

*"Übersetzt" Bluetooth Classic eines Forumslader V5 in Bluetooth LE.*


## Warum dieses Projekt?

Der großartige Forumslader (www.forumslader.de) übermittelt per Bluetooth die über den Nabendynamo und einen Luftdrucksensor gewonnene Daten an eine App.
Darauf aufbauend habe ich mir einen Computertacho und Gehäuse für mein Trekkingrad gebaut (https://github.com/euphi/ESP32_BTTacho), welches die Forumslader-Daten (und die eines Pulsmessers) anzeigt und eine einigermaßen regenfeste USB-Buchse bereit stellt. Zur Anzeige wird dabei ein 128x64 monochromes OLED-Display und ein 8fach-LED-Streifen eingesetzt.
Auf der Suche nach einem besseren Display, idealerweise auch mit Touchbedienung, bin ich auf die fertigen T-RGB Module von LilyGO gestoßen (https://www.lilygo.cc/products/t-rgb). Diese für den Leistungsumfang günstigen Modulen beinhalten fast alles, was ich mir wünschte:

* einen leistungsfähigen Microcontroller mit Bluetooth und WIFI (ESP32-S3)
* ein gutes Display (480px rund)
* Touchbedienung
* SD-Kartenleser
* Batterieanschluss (und damit RTC (Uhr))
* I2C-Stecker für weitere Sensoren

(Eine erste Software dazu liegt unter https://github.com/euphi/T-RGB-Experiments)

Leider gibt es ein kleines Problem: die ESP32-S3 unterstützen anders als die ersten ESP32 kein Bluetooth Classic mehr, sondern nur noch Bluetooth LE. Damit ist also kein Verbindungsaufbau zu älteren Forumsladern mehr möglich.

*Als Lösung nutze ich zusätzlich einen Standard ESP32, der sich per Bluetooth Classic zum Forumslader V5 verbindet und die empfangenen Daten wieder über Bluetooth LE verteilt.*


## Was macht das Programm?

* Initialisiert Bluetooth LE und "advertised" den BLE Dienst
* schaltet Wifi aus, um keinen unnötigen Strom zu verbrauchen
* Sucht regelmäßig nach dem Forumslader Classic und verbindet sich (neu) _(noch ungetestet)_
* Schickt empfangene Daten direkt wieder über Bluetooth LE als Notification heraus
* Wenn kein BLE-Client mehr verbunden ist, wird der Dienst wieder "advertised"

## Hardware

* möglichst sparsames ESP32-Modul (bei mir kommt zur Zeit ein "ESP32 D1 Mini" zum Einsatz, der sich über 5V versorgen lässt. Geht aber bestimmt noch etwas sparsamer)
* _sonst nichts!_

## Installation

* _Zur Zeit ist die Forumslader Bluetooth-Adresse noch hard-kodiert im Code. Diese an die Adresse des eigenen Forumslader anpassen (Zeile 9)!_
* Baut mit [Platformio](https://platformio.org/) und lässt sich damit installieren:
* `platformio run -t upload`
* Keine weitere Konfiguration nötig

## TODO - (Unterstützung gesucht!)

Prio 1:

- [ ] Robustheittests
- [ ] Ausprobieren, ob größere MTU möglich (alle Daten eines Strings auf einmal übertragen)
- [ ] Automatisches Suche nach Forumslader-Adresse

Prio 2:

- [ ] Parsen eines Teil der Forumslader-Daten, um diese als Standarddienste über Bluetooth LE zu verteilen
- [ ] Veröffentlichen der Daten auf BLE wie von einem Forumslader V6

