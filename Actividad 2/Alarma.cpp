#include <iostream>
#include "Alarma.h"

Alarma::Alarma(const std::string& nombre) : nombre(nombre), activa(false) {}

void Alarma::disparar() {
    activa = true;
}

void Alarma::detener() {
    activa = false;
}

void Alarma::mostrarEstado() {
    std::cout << "Alarma [" << nombre << "] está "
              << (activa ? "ACTIVADA" : "DESACTIVADA") << std::endl;
}
