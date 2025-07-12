// Pomodoro Touch Timer con descansos y ciclo completo

#include <TFT_eSPI.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "kafkar_logo.h"

TFT_eSPI tft = TFT_eSPI();

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define FONT_SIZE 2

// Pines XPT2046
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

// Colores pastel
uint16_t pastelPink = tft.color565(255, 182, 193);
uint16_t pastelGreen = tft.color565(152, 251, 152);
uint16_t pastelBlue = tft.color565(173, 216, 230);
uint16_t pastelYellow = tft.color565(255, 255, 204);

// Duraciones (test)
const unsigned long DURACION_POMODORO = 25 * 1000;
const unsigned long DURACION_DESCANSO_CORTO = 5 * 1000;
const unsigned long DURACION_DESCANSO_LARGO = 15 * 1000;

// Estados
enum PantallaEstado { PANTALLA_INICIO, PANTALLA_POMODORO, PANTALLA_DESCANSO };
enum EstadoTemporizador { DETENIDO, CORRIENDO, PAUSADO };

PantallaEstado pantallaActual = PANTALLA_INICIO;
EstadoTemporizador estadoTemporizador = DETENIDO;
bool enDescansoLargo = false;

// Variables globales
int pomodorosCompletados = 0;
unsigned long startTime = 0;
unsigned long pausaTiempo = 0;

void mostrarCiclos(int completados) {
  int total = 4;
  int radio = 8;
  int espaciado = 25;
  int totalAncho = (total - 1) * espaciado;
  int xStart = (SCREEN_WIDTH - totalAncho) / 2;
  int y = 60;

  for (int i = 0; i < total; i++) {
    int x = xStart + i * espaciado;
    if (i < completados)
      tft.fillCircle(x, y, radio, TFT_RED);
    else
      tft.drawCircle(x, y, radio, TFT_DARKGREY);
  }
}

void mostrarTemporizador(unsigned long restante) {
  int totalSegundos = restante / 1000;
  int minutos = totalSegundos / 60;
  int segundos = totalSegundos % 60;

  char buffer[6];
  sprintf(buffer, "%02d:%02d", minutos, segundos);
  tft.setTextColor(TFT_BLACK, pastelYellow);
  tft.fillRect(70, 120, 100, 40, pastelYellow);
  tft.setTextSize(4);
  tft.setCursor(70, 120);
  tft.print(buffer);
}

void dibujarBoton(int x, int y, int w, int h, uint16_t color, const char* texto) {
  tft.fillRoundRect(x, y, w, h, 10, color);
  tft.setTextColor(TFT_BLACK, color);
  tft.setTextSize(2);
  int textX = x + w/2 - (strlen(texto) * 6);
  int textY = y + h/2 - 8;
  tft.setCursor(textX, textY);
  tft.print(texto);
}

void mostrarPantallaPomodoro() {
  tft.fillScreen(pastelYellow);
  tft.setTextColor(TFT_BLACK, pastelYellow);
  tft.setTextSize(2);
  tft.setCursor(20, 20);
  tft.println("Pomodoro en curso");
  mostrarCiclos(pomodorosCompletados);
  mostrarTemporizador(0);
  dibujarBoton(10, 250, 70, 40, pastelPink, "Inicio");
  dibujarBoton(90, 250, 70, 40, pastelGreen, "Pausa");
  dibujarBoton(170, 250, 70, 40, pastelBlue, "Reset");
}

void mostrarPantallaDescanso(bool largo) {
  tft.fillScreen(pastelGreen);
  tft.setTextColor(TFT_BLACK, pastelGreen);
  tft.setTextSize(2);
  tft.setCursor(30, 20);
  tft.println(largo ? "Descanso largo" : "Descanso corto");
  mostrarTemporizador(0);
}

void mostrarPantallaInicio() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawCentreString("Hola perrito!", SCREEN_WIDTH / 2, 30, FONT_SIZE);
  tft.drawCentreString("Toca para comenzar", SCREEN_WIDTH / 2, 280, FONT_SIZE);
  int x = (tft.width() - 116) / 2;
  int y = (tft.height() - 110) / 2;
  tft.drawBitmap(x, y, bitmap_kafkar_logo, 116, 100, TFT_WHITE);
}

int detectarBoton(int x, int y) {
  if (y >= 250 && y <= 290) {
    if (x >= 10 && x <= 80) return 0;
    if (x >= 90 && x <= 160) return 1;
    if (x >= 170 && x <= 240) return 2;
  }
  return -1;
}

void setup() {
  Serial.begin(115200);
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(2);
  tft.init();
  tft.setRotation(2);
  mostrarPantallaInicio();
}

void loop() {
  if (pantallaActual == PANTALLA_INICIO) {
    if (touchscreen.tirqTouched() && touchscreen.touched()) {
      TS_Point p = touchscreen.getPoint();
      int x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
      int y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
      pantallaActual = PANTALLA_POMODORO;
      mostrarPantallaPomodoro();
    }
  }

  else if (pantallaActual == PANTALLA_POMODORO || pantallaActual == PANTALLA_DESCANSO) {
    unsigned long duracion = (pantallaActual == PANTALLA_POMODORO) ? DURACION_POMODORO : (enDescansoLargo ? DURACION_DESCANSO_LARGO : DURACION_DESCANSO_CORTO);

    if (touchscreen.tirqTouched() && touchscreen.touched()) {
      TS_Point p = touchscreen.getPoint();
      int x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
      int y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
      int boton = detectarBoton(x, y);

      if (boton == 0) {
        if (estadoTemporizador == DETENIDO || estadoTemporizador == PAUSADO) {
          startTime = (estadoTemporizador == PAUSADO) ? millis() - pausaTiempo : millis();
          estadoTemporizador = CORRIENDO;
        }
      } else if (boton == 1 && estadoTemporizador == CORRIENDO) {
        pausaTiempo = millis() - startTime;
        estadoTemporizador = PAUSADO;
      } else if (boton == 2) {
        estadoTemporizador = DETENIDO;
        pausaTiempo = 0;
        mostrarTemporizador(0);
      }
    }

    if (estadoTemporizador == CORRIENDO) {
      unsigned long elapsed = millis() - startTime;
      unsigned long restante = (elapsed < duracion) ? duracion - elapsed : 0;
      mostrarTemporizador(restante);

      if (elapsed >= duracion) {
        estadoTemporizador = DETENIDO;
        pausaTiempo = 0;
        if (pantallaActual == PANTALLA_POMODORO) {
          pomodorosCompletados++;
          if (pomodorosCompletados >= 4) {
            enDescansoLargo = true;
            mostrarPantallaDescanso(true);
            pantallaActual = PANTALLA_DESCANSO;
          } else {
            enDescansoLargo = false;
            mostrarPantallaDescanso(false);
            pantallaActual = PANTALLA_DESCANSO;
          }
        } else if (pantallaActual == PANTALLA_DESCANSO) {
          if (pomodorosCompletados >= 4) {
            pomodorosCompletados = 0;
            tft.fillScreen(pastelBlue);
            tft.setTextColor(TFT_BLACK, pastelBlue);
            tft.setTextSize(2);
            tft.drawCentreString("Ciclo completo!", SCREEN_WIDTH/2, 120, FONT_SIZE);
            tft.drawCentreString("Toca para otro", SCREEN_WIDTH/2, 160, FONT_SIZE);
            pantallaActual = PANTALLA_INICIO;
          } else {
            pantallaActual = PANTALLA_POMODORO;
            mostrarPantallaPomodoro();
          }
        }
      }
    }
  }
}
