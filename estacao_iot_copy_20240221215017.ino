#include <ESP8266WiFi.h>        // Biblioteca para conectar o módulo ESP8266 a uma rede Wi-Fi
#include <DHT.h>                // Biblioteca para sensor DHT11
#include <SFE_BMP180.h>         // Biblioteca para sensor BMP180
#include <Wire.h>               // Biblioteca para comunicação I2C
#include <SSD1306Wire.h> // Biblioteca para o display OLED

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C // Endereço I2C do display OLED
//#define BUTTON_PIN D7// Pino onde o botão está conectad

//volatile bool buttonPressed = false;

unsigned long lastSendTime = 0; // Variável para controlar o último momento em que enviou os dados
unsigned long currentTime = 0; // Variável para controlar o tempo atual
unsigned long intervaloEnvio = 900000; // Intervalo de envio em milissegundos (900 segundos ou 15 minutos)

SSD1306Wire display(0x3c, D2, D1); // Inicializa o display OLED

// Configurações da aplicação e de rede
#define STASSID "RENAN 2G"                // Rede Wi-Fi
#define STAPSK "renan438"        // Senha da Rede Wi-Fi
const char* server = "api.thingspeak.com";  // Servidor da aplicação
String apiKey = "TOYKTNUYDK2I0DJR";         // Chave API de gravação - Write API Key
const char* ssid = STASSID;                 // Armazena em ssid o nome da rede Wi-Fi
const char* pass = STAPSK;                  // Armazena em pass a senha da rede Wi-Fi

// Temperatura e umidade
#define DHTPIN D3                           // Pino D3 será responsável pela leitura do DHT11
#define DHTTYPE DHT11                       // Define o DHT11 como o sensor a ser utilizado pela biblioteca <DHT.h>
DHT dht(DHTPIN, DHT11);                      // Inicializando o objeto dht do tipo DHT passando como parâmetro o pino (DHTPIN) e o tipo do sensor (DHTTYPE)
float u = 0.0;                               // Define variável u para armazenar o valor da umidade
float t = 0.0; 
int tempo = 0;                              // Define variável t para armazenar o valor da temperatura
int buttonPin1 = D6;
int buttonPin2 = D7;     
int estadoButton1 = 0;
int estadoButton2 = 0;  

// Qualidade do ar
int sensorAr = A0;                           // Atribui o analógico A0 à variável sensorPin
float m = -0.3376;                           // Parâmetro Slope
float b = 0.7165;                            // Parâmetro Y-Intercept
float R0 = 10.55;                            // Resistência R0 encontrada no código de calibração
float sensor_volt;                           // Define variável sensor_volt para armazenar a tensão do sensor
float Rs;                                    // Define variavél Rs para armazenar a resistência do sensor
float razao;                                 // Define variavel para armazenar o valor da razão entre Rs e R0
float ppm_log;                               // Variável para armazenar o valor de ppm em escala linear
float ppm = 0.0;                             // Variável para armazenar o valor de ppm em escala logarítmica

// Pressão atmosférica
SFE_BMP180 sensorP;                           // Define objeto sensorP na classe SFE_BMP180 da biblioteca
#define ALTITUDE 708                      // Altitude da casa da robótica em metros
char status;                                  // Variável auxiliar para verificação do resultado
double temperatura;                           // Variável para armazenar o valor da temperatura
double pressao_abs;                           // Variável para armazenar o valor da pressão absoluta
double pressao_relativa = 0.0;                // Variável para armazenar a pressão relativa

WiFiClient client;                            // Cria um cliente que pode se conectar a um endereço IP da internet

/*void IRAM_ATTR handleButtonPress() {
    buttonPressed = true;
}*/

// Função para leitura da temperatura e umidade - Sensor DHT11
void sensorDHT() {
    u = dht.readHumidity();                  // Realiza a leitura da umidade
    t = dht.readTemperature();               // Realiza a leitura da temperatura
}

// Função para leitura da qualidade do ar - Sensor MQ-135
void qualidadeAr() {
    sensor_volt = analogRead(sensorAr) * (5.0 / 1023.0);
    Rs = ((5.0 * 10.0) / sensor_volt) - 10.0; // Cálculo para obter a resistência Rs do sensor
    razao = Rs / R0;                         // Calcula a razão entre Rs/R0
    ppm_log = (log10(razao) - b) / m;        // Cálculo para obter o valor de ppm em escala linear de acordo com o valor de razao
    ppm = pow(10, ppm_log);                  // Converte o valor de ppm para escala logarítmica
}

// Função para leitura da pressão absoluta e relativa - Sensor BMP180
void Pressao() {
    status = sensorP.startTemperature();     // Inicializa a leitura da temperatura
    if (status != 0) {                       // Se status for diferente de zero (sem erro de leitura)
        delay(status);                       // Realiza uma pequena pausa para que a leitura seja finalizada
        status = sensorP.getTemperature(temperatura); // Armazena o valor da temperatura na variável temperatura
        if (status != 0) {                   // Se status for diferente de zero (sem erro de leitura)
            // Leitura da Pressão Absoluta
            status = sensorP.startPressure(3); // Inicializa a leitura
            if (status != 0) {               // Se status for diferente de zero (sem erro de leitura)
                delay(status);               // Realiza uma pequena pausa para que a leitura seja finalizada
                status = sensorP.getPressure(pressao_abs, temperatura); // Atribui o valor medido de pressão à variável pressao, em função da variável temperatura
                if (status != 0) {           // Se status for diferente de zero (sem erro de leitura)
                    pressao_relativa = sensorP.sealevel(pressao_abs, ALTITUDE); // Atribui o valor medido de pressão relativa à variável pressao_relativa, em função da ALTITUDE
                }
            }
        }
    }
}

void displayTemperatureAndHumidity() {
    u = dht.readHumidity();
    t = dht.readTemperature();

    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(10, 0, "Temperatura:");
    display.drawString(10, 16, String(t) + " C");
    display.drawString(10, 32, "Umidade:");
    display.drawString(10, 48, String(u) + " %");
    display.display();
    delay(20000);
    display.clear();
    display.display();
}
void enviardados(){
  sensorDHT();     // Chama a função sensorDHT
  qualidadeAr();  // Chama a função qualidadeAr
  Pressao();  // Chama a função Pressao

  if (client.connect(server, 80)) { // Se a conexão com o servidor da aplicação "api.thingspeak.com" foi estabelecida
        // Configuração dos Fields do ThinkSpeak que vão exibir dos dados
        // Monta uma string para passar o valor das variáveis pela URL
        String postStr = apiKey;
        postStr += "&field1=";
        postStr += String(u);
        postStr += "&field2=";
        postStr += String(t);
        postStr += "&field3=";
        postStr += String(pressao_relativa);
        postStr += "&field4=";
        postStr += String(pressao_abs);
        postStr += "&field5=";
        postStr += String(ppm);
        postStr += "\r\n\r\n";

        client.print("POST /update HTTP/1.1\n");
        client.print("Host: api.thingspeak.com\n");
        client.print("Connection: close\n");
        client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
        client.print("Content-Type: application/x-www-form-urlencoded\n");
        client.print("Content-Length: ");
        client.print(postStr.length());
        client.print("\n\n");
        client.print(postStr);

        // Imprime informações no monitor serial
        Serial.print("Temperatura: ");
        Serial.println(t);
        Serial.print("Umidade: ");
        Serial.println(u);
        Serial.print("Pressão absoluta: ");
        Serial.println(pressao_abs, 1);
        Serial.print ("Pressão relativa: ");
        Serial.println(pressao_relativa, 1);
        Serial.print("Qualidade do ar: ");
        Serial.println(ppm);

        Serial.println("Enviando ao ThinkSpeak.....");
      
        Serial.println(" "); // Imprime na serial " "
        Serial.println("Enviado com sucesso !!");
    } 
    else { // Se a conexão com o servidor da aplicação "api.thingspeak.com" não foi estabelecida
        Serial.println("Falha de Conexão");
       
        delay(2000);
        return;
    }
    
    client.stop();// Desconecta do servidor
  }




void setup() {

    Serial.begin(115200);                    // Inicializa a comunicação serial
    dht.begin();                             // Inicializa o sensor DHT11
    sensorP.begin();                         // Inicializa o sensor de pressão atmosférica
    pinMode(buttonPin1 , INPUT);
    pinMode(buttonPin2 , INPUT);
    delay(500); 
    display.init(); // Inicializa o display OLED
    display.flipScreenVertically();
    display.setFont(ArialMT_Plain_16); // Define a fonte do texto no display
    display.setTextAlignment(TEXT_ALIGN_CENTER); // Alinha o texto no centro
    

    Serial.println(" ");
    Serial.print("Conectando à rede WiFi: ");                             // Intervalo de 500

    
    Serial.println(ssid);                    // Imprime na serial o nome da rede Wi-Fi
    WiFi.begin(ssid, pass);                  // Inicializa a conexão Wi-Fi passando como parâmetro o nome da rede e a senha
    Serial.print("Tentando conectar .");
    while (WiFi.status() != WL_CONNECTED) {  // Enquanto o status do Wi-Fi for diferente de WL_CONNECTED
        delay(500);                          // Intervalo de 500
        Serial.print(".");                   // Imprime na serial "."
    }
    Serial.println(" ");
    Serial.println("===== Estação Meteorológica IoT =====");
    enviardados();
  }


void loop() {
  
    currentTime = millis(); // Obtem o tempo atual em milissegundos

    if (currentTime - lastSendTime >= intervaloEnvio) {
    enviardados();
    lastSendTime = currentTime; // Atualiza o último momento em que enviou os dados
    displayTemperatureAndHumidity();
  }
  estadoButton1 = digitalRead(buttonPin1);
  estadoButton2 = digitalRead(buttonPin2);

  if (estadoButton1 == HIGH) {
    enviardados();
    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(10, 0, "ENVIADO");
    display.display();
    delay(10000);
    display.clear();
    display.display();

  }

   if (estadoButton2 == HIGH) {
    displayTemperatureAndHumidity();
  }




}

   


    