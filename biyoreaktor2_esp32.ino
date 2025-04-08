#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

const char* ssid = "FiberHGW_TPE1C2_2.4GHz";
const char* password = "EvLAjdX7";

WebServer server(80);

struct SensorData {
  int value;
  String timestamp;
  String name;
};

SensorData sensorData[7] = {
  {0, "", "Sıcaklık"},
  {0, "", "Nem"},
  {0, "", "Su Seviyesi"},
  {0, "", "Işık Seviyesi"},
  {0, "", "Su Sıcaklığı"},
  {0, "", "Metan Gazı"},
  {0, "", "Alkol"}
};

// Fonksiyon: Geçerli tarihi ve saati al
String getTimeStamp() {
  time_t now = time(nullptr);
  now += 3 * 3600; // UTC+3 için 3 saat ekle
  struct tm* timeinfo = localtime(&now);
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
  return String(buffer);
}

void setup() {
  Serial.begin(9600); // Seri monitör için baud hızı
  Serial2.begin(115200, SERIAL_8N1, 16, 17); // UART2 kullanarak Arduino ile iletişim için baud hızı (RX GPIO 16, TX GPIO 17)

  // WiFi'ye bağlan
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  // WiFi bağlantısı başarılı
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP()); // IP adresini Seri Monitör'de göster

  // Zaman sunucusuna bağlan
  configTime(3 * 3600, 0, "pool.ntp.org"); // UTC+3 için zaman dilimi ayarı
  while (!time(nullptr)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nTime synchronized");

  // Web sunucusu yapılandırması
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();
  
  // Arduino'dan gelen seri verileri oku
  if (Serial2.available()) {
    String input = Serial2.readStringUntil('\n'); // Yeni satıra kadar oku
    int sensorValues[7];
    int index = 0;
    char* token = strtok((char*)input.c_str(), ",");
    while (token != nullptr && index < 7) {
      sensorValues[index] = atoi(token);
      sensorData[index].value = sensorValues[index];
      sensorData[index].timestamp = getTimeStamp();
      token = strtok(nullptr, ",");
      index++;
    }
  }

  // Seri Monitör'de sensör verilerini göster
  for (int i = 0; i < 7; i++) {
    Serial.print(sensorData[i].name);
    Serial.print(": ");
    Serial.print(sensorData[i].value);
    Serial.print(" at ");
    Serial.println(sensorData[i].timestamp);
  }
  delay(1000);
}

void handleRoot() {
  String html = "<html><head><meta charset='UTF-8'><title>Sensor Data</title>";
  html += "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>";
  html += "<script src='https://cdn.jsdelivr.net/npm/moment@2.29.1/moment.min.js'></script>";
  html += "<script src='https://cdn.jsdelivr.net/npm/chartjs-adapter-moment@1.0.0/dist/chartjs-adapter-moment.min.js'></script>";
  html += "<style>body { font-family: Arial, sans-serif; } .chart-container { display: flex; flex-wrap: wrap; } .chart { flex: 1 1 45%; padding: 10px; }</style>";
  html += "<script>";
  html += "async function fetchData() { const response = await fetch('/data'); const data = await response.json(); return data; }";
  html += "async function updateChart(chart, sensorIndex) { const data = await fetchData(); chart.data.labels.push(moment(data[sensorIndex].timestamp)); chart.data.datasets[0].data.push(data[sensorIndex].value); chart.update(); }";
  html += "window.onload = function() {";
  const char* colors[] = {"'rgba(75, 192, 192, 1)'", "'rgba(255, 99, 132, 1)'", "'rgba(54, 162, 235, 1)'", "'rgba(255, 206, 86, 1)'", "'rgba(75, 192, 192, 1)'", "'rgba(153, 102, 255, 1)'", "'rgba(255, 159, 64, 1)'"}; 
  for (int i = 0; i < 7; i++) {
    html += "const ctx" + String(i) + " = document.getElementById('myChart" + String(i) + "').getContext('2d');";
    html += "const chart" + String(i) + " = new Chart(ctx" + String(i) + ", { type: 'line', data: { labels: [], datasets: [{ label: '" + sensorData[i].name + " Değerleri', data: [], borderColor: " + colors[i] + ", borderWidth: 1 }] }, options: { scales: { x: { type: 'time', time: { unit: 'second' }, title: { display: true, text: 'Zaman' } }, y: { beginAtZero: true, title: { display: true, text: 'Değer' } } } } });";
    html += "setInterval(() => updateChart(chart" + String(i) + ", " + String(i) + "), 1000);";
  }
  html += "};";
  html += "</script></head><body>";
  html += "<h1>Sensör Değerleri</h1>";
  html += "<div class='chart-container'>";
  for (int i = 0; i < 7; i++) {
    html += "<div class='chart'><h2>" + sensorData[i].name + "</h2>";
    html += "<canvas id='myChart" + String(i) + "' width='400' height='200'></canvas></div>";
  }
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

void handleData() {
  DynamicJsonDocument doc(2048); // Increased size for 7 sensors
  for (int i = 0; i < 7; i++) {
    doc[i]["value"] = sensorData[i].value;
    doc[i]["timestamp"] = sensorData[i].timestamp;
    doc[i]["name"] = sensorData[i].name;
  }
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}
