#pragma once
#include <string>

class Alarma {
private:
    std::string nombre;
    bool activa;

public:
    Alarma(const std::string& nombre);
    void disparar();
    void detener();
    void mostrarEstado();
};
