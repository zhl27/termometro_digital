//
// Created by lucaz on 20/11/2026.
//

#ifndef MUESTRAS_TERMISTOR_FUNCIONAPROXIMACION_H
#define MUESTRAS_TERMISTOR_FUNCIONAPROXIMACION_H

#include <WString.h>

constexpr double euler = 2.71828;

typedef double (*FuncionAproximacion)(double); // toma tension en V y retorna Temperatura en Kelvin
static double TemperaturaPorAproximacionCuadratica(double Voltios);
static double TemperaturaPorAproximacionExponencial(double Voltios);
static double TemperaturaPorAproximacionLineal(double Voltios);
static double TemperaturaPorAproximacionLogaritmica(double Voltios);


enum funcion_aproximacion_enum {
    QUAD = 0,
    EXP,
    LIN,
    LOG
};
extern funcion_aproximacion_enum funcion_aproximacion_actual; // declaramos y asignamos valor default.
extern FuncionAproximacion calcularTemperaturaPor[];
extern String funcion_aproximacion_string[];






#endif //MUESTRAS_TERMISTOR_FUNCIONAPROXIMACION_H
