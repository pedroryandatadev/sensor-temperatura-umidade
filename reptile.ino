#include <SPI.h>
#include <Ethernet.h>
#include "LiquidCrystal_I2C.h"
#include "Wire.h"
#include "DHT.h"

// Define o endereço IP, gateway e máscara de sub-rede que o Arduino vai usar
byte ip[] = {192, 168, 1, 180};
byte gateway[] = {192, 168, 1, 1};
byte subnet[] = {255, 255, 255, 0};
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // Substitua isso com o endereço MAC do seu dispositivo Ethernet
EthernetServer server(80);

// Definições para o sensor DHT
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Definições para o LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Definições para o buzzer e LDR
#define BUZZER_PIN 8
#define LDR_PIN A0

float temp_limite_curto = 20.0;
float temp_limite_longo = 25.0;
float last_temperature = 0.0;
int lightThreshold = 1000;

// Variáveis para armazenar os valores lidos do sensor
float humidity = 0.0;
float temperature = 0.0;
int lightIntensity = 0;

void setup() {
  // Inicializa a porta serial para debug
  Serial.begin(9600);

  // Inicializa o Ethernet Shield com parâmetros específicos
  Ethernet.begin(mac, ip, gateway, subnet);

  // Inicializa o servidor
  server.begin();

  // Inicializa o LCD
  lcd.init();
  lcd.backlight();

  // Inicializa o sensor DHT
  dht.begin();

  // Configura os pinos do buzzer e LDR
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LDR_PIN, INPUT);
}

void loop() {
  // Verifica se há clientes conectados
  EthernetClient client = server.available();
  if (client) {
    // Manipula o cliente
    handleClient(client);
  }

  // Aguarda 2 segundos entre as leituras
  delay(2000);

  // Lê a umidade e temperatura do sensor DHT
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  lightIntensity = analogRead(LDR_PIN);

  // Limpa o display LCD
  lcd.clear();

  // Exibe a temperatura e umidade no LCD
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.print("Um: ");
  lcd.print(humidity);
  lcd.print("%");

  // Verifica se a temperatura mudou desde a última medição
  if (temperature != last_temperature) {
    last_temperature = temperature;

    // Verifica os limites de temperatura e emite bips
    if (temperature >= temp_limite_longo) {
      emitirBipLongo();
    } else if (temperature > temp_limite_curto) {
      emitirBipCurto();
    } else {
      noTone(BUZZER_PIN);
    }
  }

  // Verifica a luminosidade e imprime uma mensagem no LCD
  lcd.setCursor(11, 1);
  if (lightIntensity < lightThreshold) {
    lcd.print("ON");
  } else {
    lcd.print("OFF");
    emitirBipLuzDesligada();
  }
}

void handleClient(EthernetClient client) {
  // Aguarda até que o cliente esteja conectado
  while (!client.available()) {
    delay(1);
  }

  // Responde apenas após o cliente enviar uma solicitação
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();

  // Verifica se a solicitação GET contém um parâmetro "text"
  int textIndex = request.indexOf("GET /?text=");
  if (textIndex != -1) {
    int startIndex = textIndex + 11; // posição do início do texto
    int endIndex = request.indexOf(" ", startIndex); // posição do final do texto
    String lcdText = request.substring(startIndex, endIndex);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(lcdText);
  }

  // Atualiza os valores dos sensores
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  lightIntensity = analogRead(LDR_PIN);

 // Constrói a página HTML
  String htmlResponse = "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='3'>";
  htmlResponse += "<title>Arduino Web Server</title>";
  htmlResponse += "<style>body { font-family: Arial, sans-serif; text-align: center; }";
  htmlResponse += "h1 { color: red; } p { font-size: 1.2em; }";
  htmlResponse += "form { margin-top: 20px; }</style></head><body>";
  htmlResponse += "<h1>Info do ambiente:</h1>";
  htmlResponse += "<p>Temperatura: ";
  htmlResponse += temperature;
  htmlResponse += "°C</p>";
  htmlResponse += "<p>Umidade: ";
  htmlResponse += humidity;
  htmlResponse += "%</p>";
  htmlResponse += "<p>Luz: ";
  if (lightIntensity < lightThreshold) {
    htmlResponse += "ON";
  } else {
    htmlResponse += "OFF";
  }
  htmlResponse += "</p>";
  
  htmlResponse += "<form method='get'>";
  htmlResponse += "<input type='text' name='text' placeholder='Texto para o LCD'>";
  htmlResponse += "<button type='submit'>Enviar</button>";
  htmlResponse += "</form>";
  htmlResponse += "</body></html>";

  // Envia a página HTML para o cliente
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println();
  client.println(htmlResponse);

  // Fecha a conexão
  delay(1);
  client.stop();
}

// Função para emitir um bip curto
void emitirBipCurto() {
  tone(BUZZER_PIN, 1000);
  delay(100);
  noTone(BUZZER_PIN);
}

// Função para emitir um bip longo
void emitirBipLongo() {
  tone(BUZZER_PIN, 1000);
  delay(1000);
  noTone(BUZZER_PIN);
}

// Função para emitir um bip quando a luz está desligada
void emitirBipLuzDesligada() {
  tone(BUZZER_PIN, 2000);
  delay(500);
  noTone(BUZZER_PIN);
}

