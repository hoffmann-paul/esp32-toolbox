#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>                  
#include <TimeLib.h>      
#include <string.h>

LiquidCrystal_I2C lcd(0x26, 16, 2);

char symbols[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '#', '+', '*', '<', '>', ',', ';', '.', ':', '-', '_', '!', '$', '%', '&', '/', '(', ')', '=', '?', ' '};

bool Clock = false;
bool wifiView = false;

char *menuStrings[] = {"Clock", "Dino Game", "Scan for Wifis", "Connect WLAN", "Configure WLAN"}; // Wenn reinfolge geändert menu Index in loop() änder!!!!!!!
int menuIndex = 0;

char ssid[33] = ""; //Standard WLAN
char password[64] = ""; //Standard Passwort
WiFiUDP ntpUDP;

char Time[ ] = "TIME:00:00:00";
char Date[ ] = "DATE:00/00/2000";
byte last_second, second_, minute_, hour_, day_, month_;
int year_;
const char* TZ_INFO = "CET-1CEST,M3.5.0,M10.5.0/3";

int dino_score = 0;

bool dino_allow_jump = false;

bool get_button() {
  return digitalRead(12);
}

byte dino_l[8] = {
  B00000111,
  B00000101,
  B00000111,
  B00010110,
  B00011111,
  B00011110,
  B00001110,
  B00000100
};

// dino right leg char
byte dino_r[8] = {
  B00000111,
  B00000101,
  B00000111,
  B00010110,
  B00011111,
  B00011110,
  B00001110,
  B00000010
};

// small cactus
byte cactus_small[8] = {
  0b00100,
  0b00101,
  0b10101,
  0b10101,
  0b10111,
  0b11100,
  0b00100,
  0b00000
};

// big cactus
byte cactus_big[8] = {
  B00000000,
  B00000100,
  B00000101,
  B00010101,
  B00010110,
  B00001100,
  B00000100,
  B00000100
};

  // game world
  char world[] = {
    32, 32, 32, 32, 32, 32, 32, 83, 99, 111, 114, 101, 58, 32, 32, 32,
    32, 0, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
  };

// infinite run routine
uint8_t scroll_world() {
  //Delay to control game speed
  delay(100);

  // create next random object on the map (small/big cactus or empty square)
  char random_object = random(2, 35);  // '35' is a probability of encountering a cactus

  // place cactus on map
  if (random_object < 4) world[31] = random_object;

  // place empty square on map
  else world[31] = 32;

  // loop over second row of game world map
  for (int i = 16; i < 32; i++) {
    // scroll only if encounter a cactus
    if (world[i] == 2 or world[i] == 3) {
      // previous cell (cactus or empty)
      char prev = (i < 31) ? world[i + 1] : 32;

      // dino and cactus collision detection (game over)
      if (world[i - 1] < 2) return 1;

      // scroll world
      world[i - 1] = world[i];
      world[i] = prev;
    }
  }

  // erase top right cell on map (otherwise it gets a cactus as a side effect)
  world[15] = 32;

  // erase cactus behind dino after jump
  if (world[16] < 2) world[16] = 32;

  // no collisions, hence keep running
  return 0;
}

void setup() {   
  pinMode(13, INPUT_PULLUP); // Menu
  pinMode(12, INPUT_PULLUP); // Ok
  pinMode(14, INPUT_PULLUP); // Rechts
  pinMode(27, INPUT_PULLUP); // Links
  pinMode(26, INPUT_PULLUP); // Hoch
  pinMode(25, INPUT_PULLUP); // Runter

  Wire.begin(21, 22);

  lcd.createChar(0, dino_l);
  lcd.createChar(1, dino_r);
  lcd.createChar(2, cactus_small);
  lcd.createChar(3, cactus_big);

  Serial.begin(115200); 
	lcd.init();                      
  lcd.backlight();
  lcd.setCursor(0, 0); 
   
  menu();
}

void initClock() {
  Clock = true;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(Time);
  lcd.setCursor(0, 1);
  lcd.print(Date);

  configTzTime(TZ_INFO, "pool.ntp.org", "time.nist.gov");
}

void menu() {
  Clock = false;
  lcd.clear();
  lcd.setCursor(0, 0);
  if(menuIndex >= sizeof(menuStrings) / sizeof(menuStrings[0])) {
    menuIndex = 0;
  }
  lcd.print(menuStrings[menuIndex]);
  lcd.setCursor(0, 1);
  for (int i = 0; i < 17; i++) {
    lcd.print("-");
  }
  lcd.setCursor(menuIndex, 1);
  lcd.print("#");
}

void wifiScan() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Setup Wifi...");
  WiFi.mode(WIFI_STA);
  lcd.setCursor(0, 0);
  lcd.print("Setup Done     ");
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("Scanning...");
  int n = WiFi.scanNetworks();
  int i = 0;
  if (n == 0) {
    lcd.setCursor(0, 0);
    lcd.print("0 networks     ");
  } else {
    lcd.setCursor(0, 0);
    lcd.print(String(n) + " networks     ");
    wifiView = true;
    while(wifiView) {
      while(digitalRead(14) == 0) {
        if(i >= n) {
          wifiView = false;
          menu();
          return;
        }
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Wifi " + String(i + 1) + ": " + String(WiFi.RSSI(i)));
        lcd.setCursor(0, 1);
        lcd.print(String(WiFi.SSID(i)));
        i++;
        delay(250);
      }
      while(digitalRead(27) == 0) {
        if(i > n) {
          wifiView = false;
          menu();
          return;
        }
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Wifi " + String(i + 1) + ": " + String(WiFi.RSSI(i)));
        lcd.setCursor(0, 1);
        lcd.print(String(WiFi.SSID(i)));
        i = i - 1;
        delay(250);
      }
    }
  }
}

void dinoGame() {
  lcd.setCursor(0, 1);
  lcd.write(byte(0));
  lcd.write(byte(3));
  lcd.print("Press Start!");
  lcd.write(byte(3));
  lcd.write(byte(0));
  while (get_button() == HIGH)
    ;

  // game loop
  while(digitalRead(13) != 0) {
    // dissallow dino hanging on the upper row all of the time
    dino_allow_jump ^= 3;

    // on button press
    if (get_button() == 0 && dino_allow_jump == true) {
      // update dino position on LCD
      lcd.setCursor(1, 1);
      lcd.write(byte(32));
      lcd.setCursor(1, 0);
      lcd.write(byte(0));

      // update dino position on world map
      world[1] = byte(0);
      world[17] = byte(32);

      // scroll map while dino is jumping
      for (int i = 0; i < 4; i++) update_world();

      // update dino position on world map
      world[1] = byte(32);
      world[17] = byte(0);

      // update dino position on LCD
      lcd.setCursor(1, 0);
      lcd.write(byte(32));
      lcd.setCursor(1, 1);
      lcd.write(byte(0));
    }

    // otherwise just scroll the world and update display
    update_world();
  }
  menu();
  return;
}

void update_world() {
  // scroll map, detect collisions
  int game_over = scroll_world();

  // winning condition
  if (dino_score == 999) {
    lcd.setCursor(0, 0);
    lcd.print("    YOU WIN!    ");
    lcd.setCursor(0, 1);
    lcd.write(byte(0));
    lcd.write(byte(32));
    lcd.write(byte(2));
    lcd.write(byte(2));
    lcd.write(byte(2));
    lcd.write(byte(3));
    lcd.write(byte(3));
    lcd.write(byte(3));
    lcd.write(byte(3));
    lcd.write(byte(3));
    lcd.write(byte(3));
    lcd.write(byte(2));
    lcd.write(byte(2));
    lcd.write(byte(2));
    lcd.write(byte(32));
    lcd.write(byte(1));
    while (1)
      ;
  }

  // hit a cactus
  if (game_over) {
    lcd.setCursor(0, 1);
    lcd.write(byte(0));
    lcd.write(byte(3));
    lcd.print(" GAME OVER! ");
    lcd.write(byte(3));
    lcd.write(byte(0));
    dino_score = 0;
    delay(1500);
    lcd.setCursor(0, 1);
    lcd.write(byte(0));
    lcd.write(byte(3));
    lcd.print("Press Start!");
    lcd.write(byte(3));
    lcd.write(byte(0));
    while (get_button() == HIGH)
      ;
  }

  // increase the score
  dino_score++;

  // update score
  lcd.setCursor(13, 0);
  lcd.print("   ");
  lcd.setCursor(13, 0);
  lcd.print(dino_score);

  // set cursor to top left corner of LCD
  lcd.setCursor(0, 0);

  // loop over game world map array
  for (int i = 0; i < 32; i++) {
    // mimic dino stepping left and right legs
    if (world[i] < 2) world[i] ^= 1;

    // update cursor for rendering lower row of th LCD
    if (i == 16) lcd.setCursor(0, 1);

    // do not overwrite score to avoid flickering
    if (i < 13 || i > 15)
      lcd.write(byte(world[i]));
  }
}

void connectWLAN() {
  WiFi.disconnect(true);
  delay(500);
  WiFi.begin(ssid, password);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(ssid);
  lcd.setCursor(0, 1);
  lcd.print("Connecting");
  float timeConnecting = 0.0;
  while ( WiFi.status() != WL_CONNECTED ) {
    lcd.setCursor(10, 1);
    lcd.print(".");
    if(timeConnecting <= 9) {
      lcd.setCursor(15, 1);
    } else {
      lcd.setCursor(14, 1);
    } 
    timeConnecting = timeConnecting + 0.5;
    lcd.print(String(timeConnecting));
    delay(500);

    lcd.setCursor(11, 1);
    lcd.print(".");
    if(timeConnecting <= 9) {
      lcd.setCursor(15, 1);
    } else {
      lcd.setCursor(14, 1);
    } 
    timeConnecting = timeConnecting + 0.5;
    lcd.print(String(timeConnecting));
    delay(500);

    lcd.setCursor(12, 1);
    lcd.print(".");
    if(timeConnecting <= 9) {
      lcd.setCursor(15, 1);
    } else {
      lcd.setCursor(14, 1);
    } 
    timeConnecting = timeConnecting + 0.5;
    lcd.print(String(timeConnecting));
    delay(500);

    lcd.setCursor(10, 1);
    lcd.print("   ");
    if(timeConnecting <= 9) {
      lcd.setCursor(15, 1);
    } else {
      lcd.setCursor(14, 1);
    } 
    timeConnecting = timeConnecting + 0.5;
    lcd.print(String(timeConnecting));
    delay(500);
    while(digitalRead(13) == 0) {
      menuIndex++;
      menu();
      return;
      delay(250);
    }
  }
}

void configureWLAN() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SSID:");
  ssid[0] = '\0';

  int typeIndex = 0;
  int symbolIndex = 0;
  int anzahlSymbole = sizeof(symbols) / sizeof(symbols[0]); 
  delay(300);

  while (digitalRead(12) != 0) {
    int len = strlen(ssid);

    lcd.setCursor(typeIndex, 1);
    lcd.print(symbols[symbolIndex]); 
    
    lcd.cursor();
    lcd.blink();

    if (digitalRead(25) == 0) {
      symbolIndex++;
      if (symbolIndex >= anzahlSymbole) symbolIndex = 0;
      lcd.setCursor(typeIndex, 1);
      lcd.print(symbols[symbolIndex]);
      delay(250); 
    }

    if (digitalRead(26) == 0) {
      symbolIndex--;
      if (symbolIndex < 0) symbolIndex = anzahlSymbole - 1; 
      
      lcd.setCursor(typeIndex, 1);
      lcd.print(symbols[symbolIndex]);
      delay(250);
    }

    if (digitalRead(14) == 0) {
      typeIndex++;
      if (typeIndex > 15) typeIndex = 15;
      if (len < 32) {
        ssid[len] = symbols[symbolIndex];
        ssid[len + 1] = '\0';
      }
      symbolIndex = 0; 
      delay(250);
    }

    delay(50);
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Password:");
  password[0] = '\0';

  typeIndex = 0;
  symbolIndex = 0;
  delay(300);

  while (digitalRead(12) != 0) {
    int len = strlen(password);

    lcd.setCursor(typeIndex, 1);
    lcd.print(symbols[symbolIndex]);

    if (digitalRead(25) == 0) {
      symbolIndex++;
      if (symbolIndex >= anzahlSymbole) symbolIndex = 0;
      lcd.setCursor(typeIndex, 1);
      lcd.print(symbols[symbolIndex]);
      delay(250); 
    }

    if (digitalRead(26) == 0) {
      symbolIndex--;
      if (symbolIndex < 0) symbolIndex = anzahlSymbole - 1; 
      
      lcd.setCursor(typeIndex, 1);
      lcd.print(symbols[symbolIndex]);
      delay(250);
    }

    if (digitalRead(14) == 0) {
      typeIndex++;
      if (typeIndex > 15) typeIndex = 15;
      if (len < 32) {
        password[len] = symbols[symbolIndex];
        password[len + 1] = '\0';
      }
      symbolIndex = 0; 
      delay(250);
    }

    delay(50);
  }
  lcd.noBlink();
  lcd.noCursor();
}

void loop() {
  while(digitalRead(13) == 0) {
    menuIndex++;
    menu();
    delay(250);
  }
  while(digitalRead(12) == 0) {
    if(menuIndex == 0) {
      initClock();
    } else {
      if(menuIndex == 1) {
        dinoGame();
      } else {
        if(menuIndex == 2) {
          wifiScan();
        } else {
          if(menuIndex == 3){
            connectWLAN();
          } else {
            if(menuIndex == 4) {
              configureWLAN();
            }
          }
        }
      }
    }
    delay(250);
  }
  if(Clock) {
    time_t now = time(nullptr);

    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    second_ = timeinfo.tm_sec;

    if (last_second != second_) {
      minute_ = timeinfo.tm_min;
      hour_   = timeinfo.tm_hour;
      day_    = timeinfo.tm_mday;
      month_  = timeinfo.tm_mon + 1;
      year_   = timeinfo.tm_year + 1900;

      Time[12] = second_ % 10 + 48;

      Time[11] = second_ / 10 + 48;

      Time[9]  = minute_ % 10 + 48;

      Time[8]  = minute_ / 10 + 48;

      Time[6]  = hour_   % 10 + 48;

      Time[5]  = hour_   / 10 + 48;

  

      Date[5]  = day_   / 10 + 48;

      Date[6]  = day_   % 10 + 48;

      Date[8]  = month_  / 10 + 48;

      Date[9]  = month_  % 10 + 48;

      Date[13] = (year_   / 10) % 10 + 48;

      Date[14] = year_   % 10 % 10 + 48;
      lcd.setCursor(0, 0);

      lcd.print(Time);

      lcd.setCursor(0, 1);

      lcd.print(Date);

      last_second = second_;
    }
    delay(200);
  }
}
