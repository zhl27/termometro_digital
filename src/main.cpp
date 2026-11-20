#include <Arduino.h>
#include "BleJsonFacade.h"
#include "FuncionAproximacion.h"
#include "MeanFilterLib.h"

static BleJsonFacade bleJson;
static MeanFilter<uint32_t> meanFilter(10000);

// // Declaro variables globales y las instancio con valor default.
// static FuncionAproximacion funcionAproximacionActual = TemperaturaPorAproximacionExponencial;
// static String funcion_aproximacion_actual_string = "Exponencial";

constexpr int PIN_ADC = 33; // ESP32 ADC pin (GPIO 34)
constexpr float R1 = 10000.0; // Resistencia fija en ohmios (ej. 10k)
constexpr float Vin_mV = 3300.0; // Medir con tester el pin 3V3 real de tu placa para mayor precisión


static int leerADC(const int pin) {
    const int adcValue = analogRead(pin);
    // Prevent division by zero or out-of-range ADC glitches
    if (adcValue <= 0 || adcValue >= 4095) {
        Serial.println("ADC reading out of bounds!");
        delay(1000);
        return 0;
    }
    return adcValue;
}

static uint32_t leerPinConFiltro(const int pin, const uint32_t nroMuestras = 100) {
    // uint32_t sum=0;
    // for(int i=0; i<nroMuestras; i++) {
    //     // sum += leerADC(pin);
    //     sum += analogReadMilliVolts(pin);
    // }
    // return sum/nroMuestras; // CUIDADO, PERDIDA DE DECIMALES POR ARITMETICA INTEGER.
    // Añadir un nuevo valor al filtro y devolver el valor filtrado
    return meanFilter.AddValue(analogReadMilliVolts(pin));
}
//
// typedef int (*FuncionFiltro)(int, int);
// static uint32_t leerConFiltro(const int pin, const uint32_t nroMuestras, FuncionFiltro filtro) {
//     //
// }
//
// static uint32_t filtroEMA() {
//     //filtrado = α * nuevaMuestra + (1 - α) * filtradoAnterior;
// }


void setup() {
    Serial.begin(115200);
    analogSetPinAttenuation(PIN_ADC, ADC_11db);
    //
    // 1. Start BLE with one line
    bleJson.begin("ESP32_Termometro_Digital");

    // 2. Optional: Handle commands sent from Bluefruit Connect's UART input bar
    bleJson.onReceive([](const String& cmd) {
        Serial.print("iPhone command: ");
        Serial.println(cmd);

        if (cmd == "PING") {
            bleJson.sendValue("status", "PONG");
        }
        else if (cmd == "APROX") {
            bleJson.sendValue("funcionAproximacion", funcion_aproximacion_string[funcion_aproximacion_actual]);
        }
        else if (cmd == "EXP") {
            bleJson.sendValue("msg", "Cambiando a Exponencial");
            funcion_aproximacion_actual = EXP;

        }
        else if (cmd == "QUAD") {
            bleJson.sendValue("msg", "Cambiando a Cuadratica");
            funcion_aproximacion_actual = QUAD;
        }
        else if (cmd == "LIN") {
            bleJson.sendValue("msg", "Cambiando a Lineal");
            funcion_aproximacion_actual = LIN;
        }
    });
}



void loop() {

    const auto Vout_mV = static_cast<float>(leerPinConFiltro(PIN_ADC));

    const uint32_t muestreo_timestamp = millis();
    Serial.println(Vout_mV);

    const auto Vout_V = Vout_mV/1000;
    // const double Temp_Kelvin = 16.27 * (Vout_V*Vout_V) - 88.406 * Vout_V + 393.564;
    // const double Temp_Kelvin = 377.28 * pow(euler,-0.157*Vout_V);
    const double Temp_Kelvin = calcularTemperaturaPor[funcion_aproximacion_actual](Vout_V);

    const double Temp_Celcius = Temp_Kelvin - 273.15;


    // Keeps advertising active if the iPhone disconnects
    bleJson.update();

    static unsigned long lastSend = 0;
    if (bleJson.isConnected() && millis() - lastSend >= 2000) {
    // if (bleJson.isConnected()) {
        lastSend = millis();

        // // Example A: Send a single key-value JSON object -> {"uptime_s":14}\n
        // bleJson.sendValue("uptime_s", static_cast<int>(millis() / 1000));

        // Example B: Bundle multiple fields into one JSON packet
        // Output in Bluefruit UART: {"temp":24.65,"humidity":58.2,"alert":false}\n
        bleJson.beginObject();
        bleJson.addField("muestreo_timestamp", muestreo_timestamp, 0);
        bleJson.addField("mV", Vout_mV, 0);
        bleJson.addField("Temp Kelvin", Temp_Kelvin, 5);
        bleJson.addField("Temp Celcius", Temp_Celcius, 5);
        bleJson.sendObject();
    }
}