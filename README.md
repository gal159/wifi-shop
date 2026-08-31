# Kremų Namai – offline parduotuvė ant ESP32

Svetainė hostinama tiesiai iš ESP32 (4 MB flash) per WiFi prieigos tašką, be interneto.

## Struktūra

| Aplankas | Paskirtis |
|---|---|
| `data/` | Svetainė LittleFS failų sistemai (~18 KB iš 2,87 MB) |
| `esp32/kremu_namai.ino` | Arduino sketch'as (AP + captive portal + web serveris + užsakymų API) |
| `esp32/partitions.csv` | 4 MB flash išskaidymas |

## Flash išskaidymas (4 MB)

| Sritis | Dydis |
|---|---|
| Sistema (bootloader, part. lentelė, NVS, PHY) | ~124 KB |
| Programa (factory app) | 1 MB |
| LittleFS (svetainė + užsakymai) | ~2,87 MB |

## Įkėlimas (Arduino IDE)

1. Sukurkite sketch'o aplanką `kremu_namai/`, į jį įdėkite `kremu_namai.ino` ir `partitions.csv`.
2. Šalia `.ino` sukurkite `data/` aplanką ir nukopijuokite šio projekto `data/` turinį.
3. Plokštė: **ESP32 Dev Module**, Flash Size: **4MB**, Partition Scheme: **Custom** (naudos `partitions.csv`).
4. Įkelkite sketch'ą, tada svetainę per **ESP32 LittleFS Data Upload** įrankį
   (arba `arduino-cli` / PlatformIO `uploadfs`).

## Naudojimas

- Prisijunkite prie WiFi tinklo **KremuNamai** (atviras; slaptažodį galima nustatyti `AP_PASS`).
- Svetainė atsidaro automatiškai (captive portal) arba adresu **http://192.168.4.1**.
- Užsakymai saugomi LittleFS faile `/orders.jsonl` ir spausdinami į Serial.

## Telegram žinutės apie užsakymus

Pateikus užsakymą ESP32 išsiunčia savininkui Telegram žinutę su kremų pavadinimais,
kiekiais, suma ir pirkėjo kontaktais. Tam ESP32 veikia **AP+STA** režimu: klientams
dalina „KremuNamai" tinklą, o pats jungiasi prie namų/parduotuvės WiFi (internetui).

Konfigūracija `kremu_namai.ino` viršuje:

1. `STA_SSID` / `STA_PASS` – WiFi tinklas su internetu.
2. Telegram'e susikurkite botą per **@BotFather** (`/newbot`) – gausite `TG_TOKEN`.
3. Parašykite botui bet kokią žinutę, tada atsidarykite
   `https://api.telegram.org/bot<TOKEN>/getUpdates` ir nusikopijuokite
   `"chat":{"id":...}` reikšmę į `TG_CHATID`.

Jei interneto nėra (arba laukai tušti) – užsakymai vis tiek saugomi LittleFS
(`/orders.jsonl`), tik žinutė neišsiunčiama.

## Administravimas

- Užsakymų peržiūra: `http://192.168.4.1/api/orders?key=admin123`
- Užsakymų išvalymas: `http://192.168.4.1/api/orders/clear?key=admin123`
- Raktą keiskite `ADMIN_KEY` konstantoje.

## Produktų keitimas

Produktai hardcoded faile `data/app.js` – masyvas `P` (pavadinimas `n`, kategorija `c`,
kaina `p`, spalva `col`, aprašymas `d`, sudėtis `i`, `feat:1` – rodyti pradžioje).
Pakeitus – iš naujo įkelti LittleFS.

## Jūsų nuotraukos

Nuotraukas dėkite į `data/img/` (žr. `data/img/README.txt`):

| Failas | Kur rodoma |
|---|---|
| `p1.jpg` … `p10.jpg` | Produktų nuotraukos (pagal produkto ID) |
| `hero.jpg` | Pradžios puslapio didžioji nuotrauka |
| `about.jpg` | „Apie mus" puslapio nuotrauka |

Jei nuotraukos nėra – automatiškai rodoma graži SVG iliustracija, tad svetainė
veikia ir be jų. Rekomendacija: JPG ~800×600, iki 150 KB (visos kartu turi tilpti
į LittleFS ~2,8 MB). Įkėlus – iš naujo įkelti LittleFS.
