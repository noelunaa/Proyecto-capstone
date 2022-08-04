// Proyecto de control de riego de un cultivo en suelo, monitoreando la humedad del suelo y controlando
// la bomba de riego, así como midiendo el flujo de agua para obtener el acumulado y determinar la
// cantidad de agua total utilizada para un cultivo determinado. Se pretende ayudar a eficientar el
// uso del agua, utilizando solo la necesaria para obtener un resultado satisfactorio.
// 3 de Agosto 2022, Universidad Tecnológica del Sur de Sonora.
// Equipo 29 del grupo 5: Gilberto Rengel, Juan Carlos Vazquez y Noé Luna.
// Se utilizan:
//    Tarjeta ESP32-CAM
//    Sensor FC-28 de humedad de suelo
//    Convertidor A/D MCP3008 para el sensor de humedad
//    Micro-bomba de agua sumergible DC3-5V
//    Sensor de flujo de agua YF-S201
//    Transistores 2N2222A y TIP31C para dar potencia a la bomba
//    Fuente de 9Vdc para alimentación de la bomba
//    Fuente de 5Vdc para el resto del circuito en general
// Pines de la tarjeta (GPIO):
//    12, 13, 14, 15 --> Comunicación y manejo del MCP3008 (y sensor de humedad)
//    2  --> Entrada de señal del sensor de flujo de agua
//    16 --> Control de la bomba de agua

// Bibliotecas
#include <WiFi.h>  // Biblioteca para manejar el WiFi del ESP32CAM
#include <DateTime.h>
#include <HTTPClient.h>
#include <Adafruit_MCP3008.h>

Adafruit_MCP3008 mcp = Adafruit_MCP3008(); // Nombre corto: mcp

// Datos de Red
const char* ssid = "Ubee87AA-2.4G";  // Pon aquí el nombre de la red a la que deseas conectarte
const char* password = "3A4D9B87AA";  // Escribe la contraseña de dicha red
const char* serverName = "http://www.riegoapi.somee.com/api/historials";
const char* bomba = "http://riegoapi.somee.com/api/bombas";

// Datos para el ADC:
const unsigned int _MISO = 12;
const unsigned int _MOSI = 13;
const unsigned int _CLK = 14;
const unsigned int _CS0 = 15;
const unsigned int CANAL = 0;

// Led indicador rojo de la ESP32CAM
const unsigned int statusLedPin = 33;

// Factor de conversión del sensor de flujo
const float flowFactor = 7.5;

// Objetos
WiFiClient espClient;   // Este objeto maneja las variables necesarias para una conexion WiFi

// Variables del programa
int humedad ;               // Valor obtenido del sensor FC-28, que será convertido a % de humedad.
int pumpCtrlPin = 4;        // Pin para control de la bomba
int flowPin = 2;            // Pin donde se recibirán pulsos del sensor de flujo.
int pulsos = 0;             // Cuenta pulsos del sensor de flujo.
int valorInicialPin = 0;
int nuevoValor = 0;
String fecha;               // Contendrá fecha y hora actual.
float caudal;               // Caudal, en L/min
float vol_agua = 0;         // Volumen de agua inyectado en cada ocasión por la bomba.
float Agua_acum = 0;        // Volumen acumulado de agua aplicada desde el inicio del sistema hasta el momento.
unsigned long milis_actual; // Milisegundos transcurridos actualmente desde la última ejecución
unsigned long milis_ant;    // Milisegundos transcurridos anteriormente

// Envía los datos obtenidos por el programa a la nube.
void Envia_datos() 
{
  // Si está conectado el WiFi, se hace la conexión con el servidor de datos y se envían los mismos.
  if(WiFi.status()== WL_CONNECTED)
  {
    HTTPClient http;

    // Your Domain name with URL path or IP address with path
    http.begin(espClient, serverName);

    // Specify content-type header
    http.addHeader("Content-Type", "application/json");
      
    // Data to send with HTTP POST
    int httpResponseCode = http.POST("{\"fecha\":\""+ fecha +"\",\"dato\":\""+ humedad +"\"}");
 
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    Serial.println("Datos enviados.");
    Serial.println();
        
    // Free resources
    http.end();
    }
}

// Envía el status actual de la bomba para que se publique en el sitio web
void Actualiza_Bomba (char *valor)
{
  int httpResponseCode;
  
  if(WiFi.status()== WL_CONNECTED)
  {
    HTTPClient http;

    // Your Domain name with URL path or IP address with path
    http.begin(espClient, bomba);

    // Specify content-type header
    http.addHeader("Content-Type", "application/json");
      
    // Data to send with HTTP POST
    if (valor == "true")
    {
      httpResponseCode = http.POST("{\"id\":\"1\",\"status\":\"true\"}");
    }
    else
    {
      httpResponseCode = http.POST("{\"id\":\"1\",\"status\":\"false\"}");
    }
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
         
    // Free resources
    http.end();
    }
}

// Configuración
void setup() {
  // Configura pin de led de status
  pinMode (statusLedPin, OUTPUT);          // Pin de led como salida
  digitalWrite (statusLedPin, HIGH);       // Se inicia con el led apagado
  pinMode (pumpCtrlPin, OUTPUT);           // Pin de coltrol de bomba como salida
  digitalWrite (pumpCtrlPin, LOW);         // Se inicia con la bomba apagada
  pinMode (flowPin, INPUT_PULLUP);         // Pin a sensor de flujo como entrada

  // Inicia comunicación serial
  Serial.begin(115200);
  delay(1000);
  Serial.print("Conectando a: ");
  Serial.println(ssid);
  
  // Iniciar la conexión a WiFi
  WiFi.begin(ssid, password);
  
  // Este bucle espera a que se realice la conexión
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite (statusLedPin, HIGH);   // Led de status apagado
    delay (500);                         // Hay que esperar que se conecte
    digitalWrite (statusLedPin, LOW);    // Led de status encendido
    Serial.print (".");                  // Indicador de progreso
    delay (10);
  }
  // Una vez lograda la conexión, permanecerá encendido el led de status.
  // Una vez conectado, el programa avanza y se informa:
  Serial.println ();
  Serial.println ("WiFi conectado");
  Serial.print ("Dirección IP:   ");
  Serial.println (WiFi.localIP());   
    
  // Estableciendo Zona de Tiempo
  DateTime.setTimeZone("MST7");  // Zona de Tiempo de Hermosillo
  // this method config ntp and wait for time sync
  // default timeout is 10 seconds
  DateTime.begin(/* timeout param */);
  if (!DateTime.isTimeValid()) {
    Serial.println("Failed to get time from server.");
    }
      
  // Iniciando el ADC para obtener el dato del sensor de humedad
  mcp.begin(_CLK, _MOSI, _MISO, _CS0);
  Serial.println ();
  Serial.println ("Leyendo dato del sensor de humedad... ");  // Iniciarán las lecturas
  delay (2000);
} // Fin del void setup

//Cuerpo del programa, bucle principal
void loop() {
  fecha = DateTime.toString();                 // Seguimiento de tiempo
  humedad = mcp.readADC(CANAL);                // Obtiene valor entero enviado por el sensor
  humedad = map(humedad, 1000, 350, 0, 100);   // El valor recibido se pone en el rango 0 a 100
  Serial.print("Fecha y hora: ");
  Serial.print(fecha);
  Serial.print("   ");
  Serial.print("Humedad del suelo: ");
  Serial.print(humedad);                       // Se envía el valor en % al monitor serial
  Serial.println(" %");
  Envia_datos();                               // Envía los datos obtenidos.
  if (humedad < 30)          // Si la humedad es menor a 30%, activa bomba por 5 segundos.
  {                          
    Serial.println("Encendiendo bomba...");
    digitalWrite(pumpCtrlPin, HIGH);
    Actualiza_Bomba("true");
    milis_ant = millis();
    pulsos = 0;
    milis_actual = millis();

    while ((milis_actual - milis_ant) < 5000)
    {
      nuevoValor = digitalRead(flowPin);
      if (nuevoValor != valorInicialPin) 
      { 
        pulsos++;
      }
      
      while (digitalRead(flowPin) == nuevoValor) 
      {
        delay(1);
      }
      milis_actual = millis();
    }
    
    digitalWrite(pumpCtrlPin, LOW);
    Serial.println("Bomba apagada");
    Actualiza_Bomba("false");
    Serial.print("Pulsos: ");
    Serial.println(pulsos);
     
    caudal = pulsos / (5 * flowFactor);
    Serial.print("Caudal: ");
    Serial.print(caudal);
    Serial.println(" L/min");
   
    vol_agua = caudal / 60;                // Litros
    Serial.print("Se inyectaron ");
    Serial.print(vol_agua);
    Serial.println(" L de agua");
    Agua_acum = Agua_acum + vol_agua;         // Obtiene total acumulado de agua utilizada al momento.
    Serial.print("Volumen acumulado: ");
    Serial.print(Agua_acum);
    Serial.println(" Litros");
  }  
  delay(5000);        // Las lecturas serán cada 5 segundos.
} // Fin de void loop
