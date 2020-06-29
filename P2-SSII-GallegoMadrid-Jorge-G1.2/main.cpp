#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>
#include <string>
#include <map>
#include <regex>
#include <set>
#include <list>
#include <climits>

using namespace std;

struct literal{
    string atributo;
    string comparador;
    string valor;
};
typedef struct literal literal;

struct regla{
    int numero;
    list<literal> antec;
    literal consec;

    bool operator<(const regla&r) const{
        return numero < r.numero;
    }
};

typedef struct regla regla;

// Array para guardar las reglas
regla * reglas;
// Mapa para guardar los tipos de los atributos, 0 si es nom, 1 si es nu
map<string, int> tipoAtributo;
// Mapa para guardar los atributos
map<string, string> atributos;
// Mapa para guardar los hechos
map<string, string> hechos;
// Mapa para guardar los hechos
map<string, string> hechosFinales;
// Array para llevar la cuenta del uso de las reglas
bool * usoReglas;
// Mapa para guardar las reglas que se usan para cada atributo
map<string, int> backtrace;
// Conjunto para guardar las reglas de la rama solución del grafo
set<regla> ramaSolucion;
// Lista para guardar las reglas que se usan
list<regla> reglasUsadas;


int numAtributos;
int * prioridades;
int numReglas;
string objetivo;
string dominio;

literal parseLiteral(string s){
    literal l;
    unsigned int pos = s.find(" ");
    l.atributo = s.substr(0, pos);
    string otra = s.substr(pos+1, s.length());
    pos = otra.find(" ");
    l.comparador = otra.substr(0, pos);
    l.valor = otra.substr(pos+1, otra.length());
    return l;
}

void parseAntecedente(string antec, int numRegla){
    string r = antec;
    literal l;
    unsigned int init = 0;
    unsigned int pos = r.find(" y ");
    // Si solo hay un literal
    if (pos == -1){
        l = parseLiteral(r.substr(init, r.length()));
        reglas[numRegla].antec.push_back(l);
    }
        // Si hay mas de un literal
    else{
        bool fin = false;
        do{
            l = parseLiteral(r.substr(init, pos));
            reglas[numRegla].antec.push_back(l);
            r = r.substr(pos + 3, r.length());
            pos = r.find(" y ");
            if(pos == -1){
                l = parseLiteral(r);
                reglas[numRegla].antec.push_back(l);
                fin = true;
            }

        }while (!fin);
    }
}

// Función para leer la base de conocimientos
void leeBaseConocimientos(char * fichero){
    ifstream file(fichero);
    string linea;
    // No tratamos la palabra reservada
    getline(file, linea);
    dominio = linea;
    // Leemos el numero de reglas
    getline(file, linea);
    numReglas = atoi(linea.c_str());
    // Leemos y almacenamos las reglas
    reglas = new regla[numReglas];
    smatch match;
    regex re("Si (.*?) Entonces ");
    for(int i = 0; i < numReglas; ++i) {
        getline(file, linea);
        while(regex_search(linea, match, re)){
            string ant =  match[0].str();
            ant = ant.substr(3, ant.size()-13);
            string con = match.suffix().str();
            reglas[i].numero = i;
            parseAntecedente(ant, i);
            reglas[i].consec = parseLiteral(con);
            linea = match.suffix().str();
        }
    }
}

// Función para leer la configuración
void leeConfiguracion(char * fichero){
    ifstream file(fichero);
    string linea;
    while(getline(file, linea)){
        istringstream ss(linea);
        // Si es la parte de atributos
        if (strcmp("ATRIBUTOS", linea.c_str()) == 0){
            getline(file, linea);
            istringstream ss(linea);
            ss >> numAtributos;
            for (int i = 0; i < numAtributos; ++i){
                string 	nombre, tipo, valores;
                getline(file, linea);
                istringstream ss(linea);
                ss >> nombre >> tipo >> valores;
                // 0 si es nom, 1 si es nu
                if (strcmp("NU", tipo.c_str()) == 0) {
                    tipoAtributo[nombre] = 1;
                    atributos[nombre] = "";
                }
                else if (strcmp("Nom", tipo.c_str()) == 0){
                    tipoAtributo[nombre] = 0;
                    atributos[nombre] = valores;
                }
            }
        }
            // Parte objetivo
        else if(strcmp("OBJETIVO", linea.c_str()) == 0){
            getline(file, linea);
            istringstream ss(linea);
            ss >> objetivo;
        }
            // Parte prioridades-reglas
        else if(strcmp("PRIORIDADES-REGLAS", linea.c_str()) == 0){
            getline(file, linea);;
            numReglas = atoi(linea.c_str());
            prioridades = new int[numReglas];
            for(int i = 0; i < numReglas; i++){
                getline(file, linea);
                prioridades[i] = atoi(linea.c_str());
            }
        }
    }
}

// Función para leer la base de hechos
void leeBaseHechos(char * fichero){
    ifstream file(fichero);
    string linea;
    // Leemos el numero de "hechos"
    int numHechos;
    getline(file, linea);
    numHechos = atoi(linea.c_str());
    for (int i = 0; i < numHechos; ++i) {
        getline(file, linea);
        istringstream ss(linea);
        string nombre, igual, valor;
        ss >> nombre >> igual >> valor;
        hechos[nombre] = valor;
    }

}

// Funciones para el algoritmo
bool noContenida(string objetivo, map<string, string> bh){
    return bh.count(objetivo) == 0;
}

//
bool compruebaLiteral(string atrBh, string valorBh, string comp, string valor){
    if (tipoAtributo[atrBh] == 1){
        if (comp == ">")
            return atoi(valorBh.c_str()) > atoi(valor.c_str());
        else if (comp == "<")
            return atoi(valorBh.c_str()) < atoi(valor.c_str());
        else if (comp == ">=")
            return atoi(valorBh.c_str()) >= atoi(valor.c_str());
        else if (comp == "<=")
            return atoi(valorBh.c_str()) <= atoi(valor.c_str());
        else if (comp == "=")
            return atoi(valorBh.c_str()) == atoi(valor.c_str());
    }
    else
        return valorBh == valor;

}

set<regla> equiparar(map<string, string> bh){
    set<regla> resultado;
    // Para cada regla comprobamos si podemos usarla
    for (int i = 0; i < numReglas; ++i) {
        bool noSirve = false;
        if(!usoReglas[i]){
            list<literal>::const_iterator it;
            // Si podemos usarla hay que ver si tenemos el antecedente
            for (it = reglas[i].antec.begin(); it != reglas[i].antec.end(); ++it){
                // Si no encontramos un literal del antecedente en la bh no nos sirve la regla
                if(bh.count(it->atributo) == 0){
                    noSirve = true;
                    break;
                }
                if(!compruebaLiteral(it->atributo, bh[it->atributo], it->comparador, it->valor)){
                    noSirve = true;
                    break;
                }
            }
            if(!noSirve){
                resultado.insert(reglas[i]);
            }
        }
    }
    return resultado;
}

regla resolver(set<regla> conjuntoConflicto){
    regla resultado;
    set<regla>::iterator it;
    int max = INT_MIN;
    int r;
    for(it = conjuntoConflicto.begin(); it !=conjuntoConflicto.end(); ++it){
        if (prioridades[it->numero] > max){
            max = prioridades[it->numero];
            r = it->numero;
        }
    }
    resultado = reglas[r];
    return resultado;
}

map<string, string> aplicar(regla regla, map<string, string> bh){
    map<string, string> resultado = map<string, string> (bh);
    resultado[regla.consec.atributo] = regla.consec.valor;
    usoReglas[regla.numero] = true;
    return resultado;
}

/*
 * FUNCION MotorEncaminamientoHaciaDelante
 *      BH = hechosIniciales
 *      ConjuntoConflicto = ExtraeCualquierRegla(BC)
 *      MIENTRAS NoContenida(Meta, BH) Y NoVacio(ConjuntoConflicto) HACER
 *          ConjuntoConflicto = Equiparar(antecedente(BC), BH)
 *          SI NoVacio(ConjuntoConflicto) ENTONCES
 *              R = Resolver(ConjuntoConflicto)
 *              NuevosHechos = Aplicar(R, BH)
 *              Actualizar(BH, NuevosHechos)
 *          FIN
 *      FIN
 *      SI Contenida(Meta, BH) ENTONCES devolver "exito"
 * FIN
 */

bool algoritmo(){
    regla reg;
    map<string, string> bh = map<string, string>(hechos);
    set<regla> conjuntoConflicto;
    do{
        conjuntoConflicto = equiparar(bh);
        if(!conjuntoConflicto.empty()){
            reg = resolver(conjuntoConflicto);
            bh = aplicar(reg, bh);
            usoReglas[reg.numero] = true;
            backtrace[reg.consec.atributo] = reg.numero;
            reglasUsadas.push_back(reg);
        }
    }while(noContenida(objetivo, bh) && !conjuntoConflicto.empty());
    bool encontrado = !noContenida(objetivo, bh);
    hechosFinales = bh;
    return encontrado;
}

void buscaReglasObjetivo(string atributo) {
    if (hechos.count(atributo) == 0){
        int r = backtrace[atributo];
        ramaSolucion.insert(reglas[r]);
        list<literal>::iterator it;
        for(it = reglas[r].antec.begin(); it != reglas[r].antec.end(); ++it){
            buscaReglasObjetivo(it->atributo);
        }
    }
}

string reglaToString(regla regla){
    string resultado = "";
    stringstream ss;
    ss << regla.numero+1;
    resultado += "R" + ss.str() + ": Si ";
    list<literal>::iterator it;
    for (it = regla.antec.begin(); it != regla.antec.end(); ++it)
        resultado += it->atributo + " " + it->comparador + " " + it->valor + " y ";
    resultado = resultado.substr(0, resultado.length()-2);
    resultado += "Entonces " + regla.consec.atributo + " " + regla.consec.comparador + " " + regla.consec.valor;
    return resultado;
}

// Main
int main(int argc, char **argv){

    if(argc != 4){
        cout << "Numero de parametros incorrecto" << endl;
        return -1;
    }

    // Variables para los ficheros
    char * bc = argv[1];
    char * conf = argv[2];
    char * bh = argv[3];


    cout << "Leyendo los ficheros..." << endl;
    // Lee las reglas y las almacena en la estructura reglas, formada por struct regla
    leeBaseConocimientos(bc);
    // Lee la configuracion. Guarda los atributos en el mapa atributos y sus tipos en el mapa tipoAtributo.
    // Guarda el objetivo y las prioridades de las reglas en el array prioridades
    leeConfiguracion(conf);
    // Lee la base de hechos y guarda los elementos en el mapa hechos.
    leeBaseHechos(bh);
    // Inicializamos todas las reglas a no usadas.
    usoReglas = new bool[numReglas];
    for (int i = 0; i < numReglas; ++i) {
        usoReglas[i] = false;
    }

    // Fichero de salida
    ofstream outF;
    string fichSol = "solucion_" + string(bh);
    outF.open(fichSol, ios::out | ios::trunc);

    cout << "Dominio: " << dominio << endl;
    outF << "Buscando una solución en el dominio " << dominio << endl;
    outF << "El objetivo es: " << objetivo << endl;
    outF << "La base de hechos inicial (" << bh << ") es: " << endl;
    map<string, string>::iterator iter;
    for (iter = hechos.begin(); iter !=hechos.end(); ++iter){
        outF << "   " << iter->first << " = " << iter->second << endl;
    }
    outF << "-----------------------------------------------" << endl;
    cout << "Aplicando el algoritmo y buscando una solucion... " << endl;
    if (algoritmo()){
        cout << "Solucion encontrada, detalles en: " << fichSol << endl;
        outF << endl << "Solución encontrada: " << objetivo << " = " << hechosFinales[objetivo] << endl << endl;
        buscaReglasObjetivo(objetivo);
        if (ramaSolucion.size()){
            outF << "Reglas usadas para llegar a la solucion: " << endl;
            list<regla>::iterator it;
            for(it = reglasUsadas.begin(); it != reglasUsadas.end(); ++it){
                if(ramaSolucion.count(*it))
                    outF << "   " << reglaToString(*it) << endl;
            }
        }
    }
    else {
        cout << "Solucion no encontrada, detalles en: " << fichSol << endl;
        outF << "Solución no encontrada" << endl;
    }

    outF.close();
    return 0;
}