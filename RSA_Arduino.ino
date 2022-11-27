#include <SoftwareSerial.h>
#include <math.h>

#define SOFTRX 10
#define SOFTTX 11

SoftwareSerial SerialSoft(SOFTRX, SOFTTX);

String id, id_other;
bool primo_in_ascolto;

uint64_t e, d, n;
uint64_t e2, n2;
static const int MIN_NUM = 10, MAX_NUM = 200;
static const char DELIMITER = '_';

// function to calculate l.c.m
uint64_t lcm(uint64_t a, uint64_t b) {
  uint64_t m, n;

  m = a;
  n = b;

  while (m != n) {
    if (m < n) {
      m = m + a;
    } else {
      n = n + b;
    }
  }

  return m;
}

uint64_t gcd(uint64_t a, uint64_t b) {
  if (b == 0)
    return a;
  return gcd(b, a % b);
}

bool checkPrime(int n) {
  for (int a = 2; a < n; a++) {
    if (n % a == 0) {
      return false;
    }
  }
  return true;
}

// numbers go from MIN_NUM to MAX_NUM - 1
int generatePrime() {
  while (true) {
    double randNum = random();

    while (randNum > 1) {
      randNum /= 10;
    }
    randNum *= (MAX_NUM - MIN_NUM);
    randNum += MIN_NUM;

    // LUCA POP3, SCARCERATELO
    if (checkPrime(floor(randNum))) {
      return floor(randNum);
    }
  }
}

// a naive method to find modular multiplicative inverse of 'a' under modulo 'prime'
uint64_t modInverse(uint64_t a, uint64_t prime) {
  a = a % prime;
  for (uint64_t x = 1; x < prime; x++)
    if ((a * x) % prime == 1)
      return x;

  return -1;
}

// performs base^exponent % modulus in a way that avoids floating point precision loss
static uint64_t powmod(uint64_t base, uint64_t exponent, uint64_t modulus) {
  uint64_t num = base;

  for (uint64_t i = 1; i < exponent; i++) {
    num = num * base % modulus;
  }

  return num;
}

// Jenkins one at a time hash function
uint32_t hashString(String str, size_t len) {
  char key[str.length()];
  str.toCharArray(key, str.length());

  uint32_t hash, i;
  for (hash = i = 0; i < len; ++i) {
    hash += key[i];
    hash += (hash << 10);
    hash ^= (hash >> 6);
  }

  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);

  return hash;
}

String encryptMessage(String message) {
  String encryptedMessage = "";

  for (char c : message) {
    uint64_t encryptedChar = int(c);
    // encrypt with recipient public key
    encryptedChar = powmod(encryptedChar, e2, n2);

    encryptedMessage += ulltos(encryptedChar) + DELIMITER;
  }

  uint64_t hash = hashString(message, sizeof(message));
  encryptedMessage += ulltos(powmod(hash, d, n));

  return encryptedMessage;
}

String decryptMessage(String message) {
  String decryptedMessage = "";

  // Split the string into substrings
  while (true) {
    int index = message.indexOf(DELIMITER);

    if (index != -1)  // No space found
    {
      String token = message.substring(0, index);
      message = message.substring(index + 1);

      uint64_t decryptedChar = stoull(token);
      // decrypt with recipient private key
      decryptedChar = powmod(decryptedChar, d, n);

      decryptedMessage += char(decryptedChar);
    } else {
      uint64_t sentHash = stoull(message);
      sentHash = powmod(sentHash, e2, n2);

      uint64_t messageHash = hashString(decryptedMessage, sizeof(decryptedMessage));
      break;
    }
  }

  return decryptedMessage;
}

uint64_t to_uint64(double x) {
  uint64_t a;
  memcpy(&a, &x, sizeof(x));
  return a;
}

String ulltos(uint64_t val) {
  String str = "";

  while (val) {
    str = char(48 + val % 10) + str;
    val /= 10;
  }

  return str;
}

uint64_t stoull(String string) {
  uint64_t num = 0;

  for (char c : string) {
    num *= 10;
    num += (c - '0');
  }

  return num;
}

void setup() {
  pinMode(SOFTTX, OUTPUT);
  pinMode(SOFTRX, INPUT);

  Serial.begin(9600);
  SerialSoft.begin(9600);

  randomSeed(analogRead(0));

  // choose two distinct prime numbers
  int p = generatePrime();

  int q;
  do {
    q = generatePrime();
  } while (p == q);

  // compute n = pq
  n = p * q;
  // Compute the Carmichael's totient function of the product as λ(n) = lcm(p − 1, q − 1)
  uint64_t phi = lcm(p - 1, q - 1);

  // public key
  // choose any number 1 < e < phi that is coprime to phi.
  do {
    e = generatePrime();
  } while (!(e < phi && gcd(e, phi) == 1));

  // private key
  // compute d, the modular multiplicative inverse of e mod phi
  d = modInverse(e, phi);

  Serial.println("Questo Arduino è il primo ad essere acceso / configurato? (y/n)");
  while (true) {
    if (Serial.available()) {
      String risposta = Serial.readString();
      risposta = risposta.substring(0, risposta.length() - 1);  // tolgo lo \n di troppo

      if (risposta == "y" || risposta == "Y") {
        primo_in_ascolto = true;
        id = "1";
        break;

      } else if (risposta == "n" || risposta == "N") {
        primo_in_ascolto = false;
        id = "2";
        break;

      } else
        Serial.println("Inserisci una risposta valida! (y/n)");
    }
  }

  id_other = (id == "1") ? "2" : "1";  // nomino l'altro Arduino in base al nome di questo

  Serial.println("\nCRITTOGRAFIA RSA ARDUINO " + String(id));

  if (primo_in_ascolto) {
    Serial.println("Sono in ascolto, in attesa di Arduino " + id_other + "..");
    delay(1000);
    while (true) {
      if (SerialSoft.available() > 0) {
        String str = SerialSoft.readString();

        int pos = str.indexOf('\n');

        String sub1 = str.substring(0, pos);
        // DEBUG SerialSoft.print(sub1 + "' " + String(sub1.length()));

        String sub2 = str.substring(pos + 1, str.length() - 1);
        // DEBUG SerialSoft.print(sub2 + "' " + String(sub2.length()));

        e2 = stoull(sub1);
        n2 = stoull(sub2);

        Serial.println("\nChiave pubblica Arduino " + id_other + ": " + ulltos(e2));
        Serial.println("Chiave N arduino " + id_other + ": " + ulltos(n2) + "\n");

        // DEBUG SerialSoft.println(ulltos(e2) + " - " + ulltos(n2));
        break;
      }
    }

    SerialSoft.println(ulltos(e));
    SerialSoft.println(ulltos(n) + "u$");
    Serial.println("Chiave pubblica Arduino " + id + ": " + ulltos(e));
    Serial.println("Chiave N Arduino " + id + ": " + ulltos(n) + "\n");
    delay(1000);

  } else {
    SerialSoft.print(ulltos(e) + "\n");
    SerialSoft.print(ulltos(n) + "\n");
    Serial.println("\nChiave pubblica Arduino " + id + ": " + ulltos(e));
    Serial.println("Chiave N Arduino " + id + ": " + ulltos(n));

    delay(1000);

    while (true) {
      if (SerialSoft.available() > 0) {
        String str = SerialSoft.readStringUntil('$');
        int pos = str.indexOf('\n');

        String sub1 = str.substring(0, pos - 1);
        // DEBUG SerialSoft.print(sub1 + "' " + String(sub1.length()));

        String sub2 = str.substring(pos + 1, str.length() - 1);
        // DEBUG SerialSoft.print(sub2 + "' " + String(sub2.length()));

        e2 = stoull(sub1);
        n2 = stoull(sub2);

        Serial.println("\nChiave pubblica Arduino " + id_other + ": " + ulltos(e2));
        Serial.println("Chiave N arduino " + id_other + ": " + ulltos(n2) + "\n");

        break;
      }
    }
  }

  String carattereDiTroppo = SerialSoft.readString();
  Serial.println("Pronto per la comunicazione\n");
}

String send_message;

void loop() {
  if (Serial.available()) {
    send_message = Serial.readString();
    SerialSoft.println(encryptMessage(send_message));
    Serial.println("Messaggio criptato inviato: " + encryptMessage(send_message) + "\n");
  }

  if (SerialSoft.available()) {
    String message = SerialSoft.readString();
    message = message.substring(0, message.indexOf('\n') - 1);
    Serial.println("Messaggio decriptato ricevuto: " + decryptMessage(message));
  }
}