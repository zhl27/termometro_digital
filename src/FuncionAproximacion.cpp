//
// Created by lucaz on 20/11/2026.
//

#include "FuncionAproximacion.h"

#include <math.h>

funcion_aproximacion_enum funcion_aproximacion_actual = LOG; // declaramos y asignamos valor default.
String funcion_aproximacion_string[] = {
    "Cuadratica",
    "Exponencial",
    "Lineal",
    "Logaritmica"
};
FuncionAproximacion calcularTemperaturaPor[] = {
    TemperaturaPorAproximacionCuadratica,
    TemperaturaPorAproximacionExponencial,
    TemperaturaPorAproximacionLineal,
    TemperaturaPorAproximacionLogaritmica
};

static double TemperaturaPorAproximacionCuadratica(const double Voltios) {
    const double Temp_Kelvin = 16.27 * (Voltios*Voltios) - 88.406 * Voltios + 393.564;
    return Temp_Kelvin;
}
static double TemperaturaPorAproximacionExponencial(const double Voltios) {
    const double Temp_Kelvin = 377.28 * pow(euler,-0.157*Voltios);
    return Temp_Kelvin;
}
static double TemperaturaPorAproximacionLineal(const double Voltios) {
    const double Temp_Kelvin = -48.764 * Voltios + 371.25;
    return Temp_Kelvin;
}
static double TemperaturaPorAproximacionLogaritmica(const double Voltios) {
    const double Temp_Kelvin = -39.2272 * std::log(Voltios) + 317.6936;
    return Temp_Kelvin;
}