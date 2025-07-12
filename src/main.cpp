// Pomodoro Touch Timer con descansos y ciclo completo

#include <TFT_eSPI.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "kafkar_logo.h"

TFT_eSPI tft = TFT_eSPI();

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define FONT_SIZE 2

// Pines para XPT2046
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

const unsigned long DURACION_POMODORO = 25 * 1000;
const unsigned long DURACION_BREAK_CORTO = 10 * 1000;
const unsigned long DURACION_BREAK_LARGO = 15 * 1000;

enum PantallaEstado { PANTALLA_INICIO, PANTALLA_POMODORO };
enum EstadoTemporizador { DETENIDO, CORRIENDO, PAUSADO };
enum TipoSesion { SESION_POMODORO, SESION_BREAK_CORTO, SESION_BREAK_LARGO };

PantallaEstado pantallaActual = PANTALLA_INICIO;
EstadoTemporizador estadoTemporizador = DETENIDO;
TipoSesion tipoSesion = SESION_POMODORO;

int pomodorosCompletados = 0;
unsigned long startTime = 0;
unsigned long pausaTiempo = 0;
bool mostrarModal = false;

void mostrarCiclos(int completados) {
  int total = 4;
  int radio = 12;
  int espaciado = 30;
  int totalAncho = (total - 1) * espaciado;
  int xStart = (SCREEN_WIDTH - totalAncho) / 2;
  int y = 80;

  for (int i = 0; i < total; i++) {
    int x = xStart + i * espaciado;
    if (i < completados)
      tft.fillCircle(x, y, radio, TFT_RED);
    else
      tft.drawCircle(x, y, radio, TFT_DARKGREY);
  }
}

void mostrarTemporizador(unsigned long elapsed) {
  unsigned long duracion = tipoSesion == SESION_POMODORO ? DURACION_POMODORO : (tipoSesion == SESION_BREAK_CORTO ? DURACION_BREAK_CORTO : DURACION_BREAK_LARGO);
  unsigned long restante = (duracion > elapsed) ? duracion - elapsed : 0;

  int totalSegundos = restante / 1000;
  int minutos = totalSegundos / 60;
  int segundos = totalSegundos % 60;

  tft.setTextColor(TFT_BLACK, pastelYellow);
  tft.fillRect(60, 120, 120, 40, pastelYellow);
  tft.setTextSize(4);

  char buffer[6];
  sprintf(buffer, "%02d:%02d", minutos, segundos);
  tft.drawCentreString(buffer, SCREEN_WIDTH / 2, 120, 4);
}

void dibujarBoton(int x, int y, int w, int h, uint16_t color, const char* texto, uint16_t colorBorde) {
  tft.fillRoundRect(x, y, w, h, 10, color);
  tft.drawRoundRect(x, y, w, h, 10, colorBorde);
  tft.setTextColor(TFT_BLACK, color);
  tft.setTextSize(2);
  int textX = x + w/2 - (strlen(texto) * 6);
  int textY = y + h/2 - 8;
  tft.setCursor(textX, textY);
  tft.print(texto);
}

void mostrarPantallaPomodoro() {
  uint16_t fondo = tipoSesion == SESION_POMODORO ? pastelPink : (tipoSesion == SESION_BREAK_CORTO ? pastelGreen : pastelBlue);
  tft.fillScreen(fondo);
  tft.setTextColor(TFT_BLACK, fondo);
  tft.setTextSize(2);

  const char* titulo = tipoSesion == SESION_POMODORO ? "Pomodoro en curso" : (tipoSesion == SESION_BREAK_CORTO ? "Descanso corto" : "Descanso largo");
  tft.drawCentreString(titulo, SCREEN_WIDTH / 2, 20, FONT_SIZE);

  mostrarCiclos(pomodorosCompletados);
  mostrarTemporizador(0);

  dibujarBoton(5, 250, 70, 40, pastelPink, "Inicio", TFT_WHITE);
  dibujarBoton(87, 250, 70, 40, pastelGreen, "Pausa", TFT_WHITE);
  dibujarBoton(166, 250, 70, 40, pastelBlue, "Reset", TFT_BLACK);
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

void mostrarModalCicloCompleto() {
  tft.fillRoundRect(30, 90, 180, 140, 10, pastelYellow);
  tft.setTextColor(TFT_BLACK, pastelYellow);
  tft.setTextSize(2);
  tft.drawCentreString("Ciclo completo!", SCREEN_WIDTH/2, 110, FONT_SIZE);
  tft.drawCentreString("\xBFNuevo ciclo?", SCREEN_WIDTH/2, 140, FONT_SIZE);
  dibujarBoton(70, 180, 100, 30, pastelGreen, "Si", pastelGreen);
  mostrarModal = true;
}

int detectarBoton(int x, int y) {
  if (mostrarModal) {
    if (x >= 70 && x <= 170 && y >= 180 && y <= 210) return 3; // Boton del modal
    return -1;
  }

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
  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    TS_Point p = touchscreen.getPoint();
    int x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    int y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);

    if (pantallaActual == PANTALLA_INICIO) {
      pantallaActual = PANTALLA_POMODORO;
      mostrarPantallaPomodoro();
    }
    else if (pantallaActual == PANTALLA_POMODORO) {
      int boton = detectarBoton(x, y);

      if (boton == 0 && (estadoTemporizador == DETENIDO || estadoTemporizador == PAUSADO)) {
        if (estadoTemporizador == PAUSADO)
          startTime = millis() - pausaTiempo;
        else
          startTime = millis();
        estadoTemporizador = CORRIENDO;
      } else if (boton == 1 && estadoTemporizador == CORRIENDO) {
        pausaTiempo = millis() - startTime;
        estadoTemporizador = PAUSADO;
      } else if (boton == 2) {
        estadoTemporizador = DETENIDO;
        pausaTiempo = 0;
        mostrarTemporizador(0);
      } else if (boton == 3) { // Boton "Si" del modal
        pomodorosCompletados = 0;
        tipoSesion = SESION_POMODORO;
        mostrarModal = false;
        mostrarPantallaPomodoro();
      }
    }
  }

  if (pantallaActual == PANTALLA_POMODORO && estadoTemporizador == CORRIENDO) {
    unsigned long elapsed = millis() - startTime;
    mostrarTemporizador(elapsed);

    unsigned long duracion = tipoSesion == SESION_POMODORO ? DURACION_POMODORO : (tipoSesion == SESION_BREAK_CORTO ? DURACION_BREAK_CORTO : DURACION_BREAK_LARGO);

    if (elapsed >= duracion) {
      estadoTemporizador = DETENIDO;

      if (tipoSesion == SESION_POMODORO) {
        pomodorosCompletados++;
        tipoSesion = (pomodorosCompletados % 4 == 0) ? SESION_BREAK_LARGO : SESION_BREAK_CORTO;
      } else {
        if (pomodorosCompletados >= 4) {
          mostrarModalCicloCompleto();
          return;
        }
        tipoSesion = SESION_POMODORO;
      }

      mostrarPantallaPomodoro();
    }
  }
}
