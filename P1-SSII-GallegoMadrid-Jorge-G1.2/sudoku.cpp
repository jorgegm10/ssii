#include <ga/GASimpleGA.h> // Algoritmo Genetico simple
#include <ga/GA1DArrayGenome.h> //genoma-> array de enteros (dim. 1) alelos
#include <iostream>
#include <fstream>
#include <string.h>

#define NUM_GEN 12000

using namespace std;

struct plantilla{
	int tam; // tamaÃ±o del sudoku
    int *fijo; // valores fijos iniciales    
};

// Lectura del sudoku inicial en S
void leerSudoku(struct plantilla *S,char *nombreF){   
	ifstream f(nombreF);
	f >> S->tam;
	S->fijo = new int[S->tam*S->tam];
	for(int i = 0; i < S->tam * S->tam; i++)
   		f >> S->fijo[i];
   	f.close();
}

// Inicia el genoma sin repetidos en filas y respetando fijos
void InicioSudoku(GAGenome& g){    
	//************************************************   
	//************************************************
	// Hacer el casting correspondiente para obtener genome
	
	GA1DArrayAlleleGenome<int> & genome = (GA1DArrayAlleleGenome<int> &)g;

	//************************************************
	//************************************************
	// Obtenemos la estructura auxiliar 
	struct plantilla * plantilla1;
	plantilla1 = (struct plantilla *) genome.userData();
	int aux[plantilla1->tam];

	// Por cada fila
	for(int f=0;f<plantilla1->tam;f++){
		// se inicializa a 0 la estructura auxiliar
		for(int j=0;j<plantilla1->tam;j++) 
			aux[j]=0;
		// se inicializa la fila de 1 a tam sin repetidos
		for(int j=1;j<=plantilla1->tam;j++){
			int v=GARandomInt(0,plantilla1->tam-1);
			while (aux[v]!=0) v=(v+1)%plantilla1->tam;
			aux[v]=j;
		}
		// se colocan los fijos en su lugar
		int i=0;
		while(i<plantilla1->tam){
			while((plantilla1->fijo[(f*plantilla1->tam)+i]==0) && (i<plantilla1->tam)) 
				i++;
			// se encuentra un fijo en la plantilla
			if (i<plantilla1->tam){
				// se busca el fijo en aux
				bool encontrado=false;
				for(int j=0;(j<plantilla1->tam) && (!encontrado);j++) 
					if (aux[j]==plantilla1->fijo[(f*plantilla1->tam)+i]) {
						encontrado=true;
						aux[j]=aux[i];
					}
				// se pone el fijo en su sitio
				aux[i]=plantilla1->fijo[(f*plantilla1->tam)+i];
			}
			i++;
		}
		// se copia la fila en el genoma
		for(int c=0;c<plantilla1->tam;c++)
			genome.gene((f*plantilla1->tam)+c,aux[c]);   
	}
}      

// Cruce por un punto que coincide con final de una fila
int CruceSudoku(const GAGenome& p1,const GAGenome & p2,GAGenome* c1,GAGenome* c2){  
     //************************************************
     //************************************************
     // Hacer el casting correspondiente para obtener m y p
     // galibdoc pag 92
     // Madre
     GA1DArrayAlleleGenome<int> &m = (GA1DArrayAlleleGenome<int> &) p1;
     // Padre
     GA1DArrayAlleleGenome<int> &p = (GA1DArrayAlleleGenome<int> &) p2;     
     //************************************************
     //************************************************    
    struct plantilla * plantilla1 = (struct plantilla *) m.userData();
    int n=0;
        
    // buscamos un punto de cruce por filas    
    int punto1=GARandomInt(0,m.length()); 
    while ((punto1%plantilla1->tam)!=0) punto1 = (punto1 + 1)%plantilla1->tam;
    int punto2=m.length()-punto1;
    
    // el hijo 1 hereda la primera parte del padre 1 y la segunda parte del
    // padre 2.     
    if (c1){
     	      //************************************************
              //************************************************
              // Hacer el casting correspondiente para obtener h1
              // galibdoc pag 92
              // Hijo1
              GA1DArrayGenome<int> &h1 = (GA1DArrayGenome<int> &)*c1;
              //************************************************
              //************************************************
              
             h1.copy(m,0,0,punto1);
             h1.copy(p,punto1,punto1,punto2);
             n++;
    }    
    // el hijo 2 hereda la primera parte del padre 2 y la segunda parte del
    // padre 1.     
    if (c2){
              //************************************************
              //************************************************
              // Hacer el casting correspondiente para obtener h2
			  // galibdoc pag 92
			  // Hijo2
              GA1DArrayGenome<int> &h2 = (GA1DArrayGenome<int> &)*c2;
              //************************************************
              //************************************************              
             h2.copy(p,0,0,punto1);
             h2.copy(m,punto1,punto1,punto2);
             n++;
    }   
    return n;    
}

// Comprueba si una columna del sudoku tiene valores repetidos
// check contiene la cuenta de valores repetidos
bool checkColumna(int col[], int * check, int tam){
     bool repe=false;     
     for(int i=0;i<tam;i++) check[i]=0;    
     for(int i=0;i<tam;i++) 
             check[col[i]-1]++;
     for(int i=0;i<tam;i++) if (check[i]>1) repe=true;     
     return repe;       
}

// Si un gen debe mutar elegimos si lo hace en fila o columna. En fila es intercambio
// en columna es mas heurístico: cambio un valor repetido por uno que no exista en la columna 
int MutacionSudoku(GAGenome& g,float pmut){
    
     //************************************************
     //************************************************
     // Hacer el casting correspondiente para obtener genome
     // galibdoc 91
     GA1DArrayAlleleGenome<int> &genome = (GA1DArrayAlleleGenome<int> &) g;
     //************************************************
     //************************************************
    
    // Obtenemos la estructura auxiliar 
    struct plantilla * plantilla1;
    plantilla1 = (struct plantilla *) genome.userData();
    int nmut=0;
    int aux,aux1;
    int fil;
    bool fila;
    
    int caux[plantilla1->tam];
    int *checkC=new int[plantilla1->tam];
    
    if (pmut<=0.0) return 0;
    for(int f=0; f<plantilla1->tam; f++)
       for(int c=0; c<plantilla1->tam; c++)
          if (plantilla1->fijo[(f*plantilla1->tam)+c]==0){ // si no es fijo
           if (GAFlipCoin(pmut) ){ // si toca mutar
                if (GAFlipCoin(0.5)) fila = true; // cambiamos fila
                else fila = false; // cambiamos columna                   
                if (!fila){ // mutamos columna y arreglamos fila                
                      for(int j=0;j<plantilla1->tam;j++) caux[j]=genome.gene((j*plantilla1->tam)+c); // obtenemos la columna
                      if (checkColumna(caux,checkC,plantilla1->tam)){ // si hay repetidos en la columna
                         int v1 = GARandomInt(0,plantilla1->tam-1); // v1 es valor repetido
                         while (checkC[v1]<=1) v1=(v1+1)%plantilla1->tam;  
                         v1++;
                         int v2 = GARandomInt(0,plantilla1->tam-1); // v2 es un valor que no existe
                         while (checkC[v2]!=0) v2=(v2+1)%plantilla1->tam; 
                         v2++;
                         //buscamos donde está v1 y se cambia por v2                         
                         bool encontrado = false;
                         for(int j=0;j<plantilla1->tam && !encontrado;j++) 
                                 if ((plantilla1->fijo[j*(plantilla1->tam)+c]==0)&&(genome.gene(j*(plantilla1->tam)+c)==v1)){
                                    encontrado = true;
                                    genome.gene((j*plantilla1->tam)+c,v2);
                                    fil = j;  // v1 esta en fil                                                      
                                 }                                                     
                         //arreglamos la fila fil donde está v1: buscamos v2 y ponemos v1                         
                         int col=(c+1)%plantilla1->tam;
                         while(genome.gene((fil*plantilla1->tam)+col)!=v2) col=(col+1)%plantilla1->tam;
                         if (plantilla1->fijo[(fil*plantilla1->tam)+col]==0) { // si v2 no es fijo en fil se lleva a cabo la mutación 
                                nmut++; 
                                genome.gene((fil*plantilla1->tam)+col,v1);                         
                         }
                         else { // si es fijo se deshace la mutacion
                              genome.gene((fil*plantilla1->tam)+c,v1);
            		     }                          
                      } // fin de si hay repetidos               
                }
                else{ // mutamos fila: cambiamos f,c por f,v1
                   int v1 = (c + 1) %plantilla1->tam;
                   while ((plantilla1->fijo[(f*plantilla1->tam)+v1]!=0)) v1=(v1+1)%plantilla1->tam; //busco un no fijo
                   aux = genome.gene((f*plantilla1->tam)+c);
                   genome.gene((f*plantilla1->tam)+c,genome.gene((f*plantilla1->tam)+v1));
                   genome.gene((f*plantilla1->tam)+v1,aux);
                   nmut++;
                }               
           } // si toca mutar
          } // si no fijo    
    return nmut;   
}

// Funcion auxiliar para calcular las repeticiones
// dentro de un array. Modificada a partir de checkColumna
float cuentaRepeticiones(int col[], int * check, int tam){
     int repe = 0;     
     for(int i=0;i<tam;i++)
        check[i]=0;    
     for(int i=0;i<tam;i++) 
        check[col[i]-1]++;
     for(int i=0;i<tam;i++)
        if (check[i]>1)
            repe += check[i]-1;     
     return repe;       
}



// Funcion objetivo
// Numero de repeticiones que se encuentran
float Objective(GAGenome& g) {
    GA1DArrayAlleleGenome<int> & genome = (GA1DArrayAlleleGenome<int> &)g;
    
    struct plantilla * plant;
    plant = (struct plantilla *) genome.userData();
    float repeticiones = 0;

    // Comprobamos repeticiones en filas
    int aux[plant->tam];
    int * check = new int[plant->tam];
    // Para cada fila
    for (int i = 0; i < plant->tam*plant->tam; i+=plant->tam){
        // Relleno aux con sus valores
        for (int j = 0; j < plant->tam; j++){
            aux[j] = genome.gene(j+i);
        }
        // Se la paso a cuentaRepeticiones
        repeticiones += cuentaRepeticiones(aux, check, plant->tam);
    }

    // Comprobamos repeticiones en columnas
    // Para cada columna
    for (int i = 0; i < plant->tam; i++){
        // Relleno aux con sus valores
        for (int j = i; j < plant->tam*plant->tam; j+=plant->tam){
            aux[j/plant->tam] = genome.gene(j);
        }
        // Se lo paso a cuentaRepeticiones
        repeticiones += cuentaRepeticiones(aux, check, plant->tam);
    }

    // Comprobamos repeticiones en cuadriculas
    int ind = 0;
    // Para cada esquina superior izquierda de una cuadricula
    for (int i = 0; i < plant->tam*plant->tam; i+=3){
        // Lo recorro por filas
        for (int j = i; j < (i+plant->tam*3); j+=plant->tam){
            // Me situo en la columna
             for(int k = j; k < j+3; k++){
                aux[ind] = genome.gene(k);
                ind++;
             }
        }
        // Si llego a la cuadricula mas a la derecha
        // salto a la siguiente de la izquierda más abajo
        if(i%plant->tam == plant->tam-3){
            i+=plant->tam*2;
        }
        // Se la paso a cuentaRepeticiones
        repeticiones += cuentaRepeticiones(aux, check, plant->tam);
        ind = 0;              
    }
    return repeticiones;
}

// Funcion de terminacion
GABoolean Termina(GAGeneticAlgorithm & ga){
    // Para si encuentra el 0 (sudoku solucion) o si
    // llegamos al limite de generaciones (12000)
    if ((ga.statistics().minEver()==0) || (ga.statistics().generation()==ga.nGenerations()))
        return gaTrue;
    else 
        return gaFalse;
}

// Funcion main
int main(int argc, char **argv){
	// Parametros:
    //    1          2            3            4            5
    // Fichero - Tam. pobl. - Op. selec - Prob. cruce - Prob. mut 
    // Declaramos variables para los parametros del GA y las inicializamos
    char * fic = argv[1];
    int tampobl = atoi(argv[2]);
    string opselec = argv[3];
    float pcruce = atof(argv[4]);
    float pmut = atof(argv[5]);
    
    // Control de errores en los parametros del programa
    if(tampobl != 100 && tampobl !=150){
        cerr << "El tamaño de la poblacion no es correcto" << endl;
        exit(-1);
    }
    else if(pcruce < 0 || pcruce > 1){
         cerr << "Probabilidad de cruce no esta entre 0 y 1" << endl;
         exit(-1);
    }
    else if(pmut < 0 || pmut > 1){
         cerr << "Probabilidad de mutacion no esta entre 0 y 1" << endl;
         exit(-1);
    }
    
    cout << "Solucion del sudoku " << fic << " con los siguientes parametros:" << endl;
    cout << "   -Poblacion: " << tampobl << endl;
    cout << "   -Operador de seleccion: " << opselec << endl;
    cout << "   -Probabilidad de cruce: " << pcruce << endl;
    cout << "   -Probabilidad de mutacion: " << pmut << endl << endl;
    
    cout << "Resolviendo el sudoku..." << endl;
    
    // Leemos el sudoku        
    struct plantilla * plant = new plantilla;
	leerSudoku(plant, fic);
      
    // Conjunto enumerado de alelos --> valores posibles de cada gen del genoma
    GAAlleleSet<int> alelos;
    for(int i=1;i<10;i++)alelos.add(i);
    // Creamos el GA. Primero creamos el genoma y creamos una poblacion de genomas
    GA1DArrayAlleleGenome<int> genome(plant->tam*plant->tam,alelos,Objective,plant);
    // Funcion para inicializar el sudoku
    genome.initializer(InicioSudoku);
    // Tipo de cruce
    genome.crossover(CruceSudoku);
    // Tipo de mutacion
    genome.mutator(MutacionSudoku);
    // Defino el tipo algoritmo, genome como tipo de indiviudo, algoritmo de tipo SimpleGA
    GASimpleGA ga(genome);
    // Minimiza el algortimo
    ga.minimaxi(-1);
    // Tamaño de poblacion
    ga.populationSize(tampobl);
    // Num de gen
    ga.nGenerations(NUM_GEN);
    // Prob de mut
    ga.pMutation(pmut);
    // Prob de cruce
    ga.pCrossover(pcruce);
    
    // Elegimos el selector en funcion del parametro
    if (opselec=="ruleta"){                         
       GARouletteWheelSelector selector;
       ga.selector(selector);
    }
    else if (opselec=="torneo"){
       GATournamentSelector selector;
       ga.selector(selector);
    }  
    
    // Se invoca terminator, pasandole como parametro la funcion
    ga.terminator(Termina);
        
    // Evolucionar el problema hasta que encuentra cond de parada
    // La semilla aleatoria se pone como argumento de evolve, ga.evolve(1)
    // Para que el AG tenga un comportamiento estable con respecto al generador
    // de numeros aleatorios, invocaremos en el codigo el metodo evolve(1)
    ga.evolve(1);
    
    // Imprimimos el sudoku solucion y su valor fitness
    cout << "Sudoku solucion: " << endl;
    GA1DArrayAlleleGenome<int> & genomeSol = (GA1DArrayAlleleGenome<int> &)ga.statistics().bestIndividual();
    cout << "    ";
    for(int i = 0; i < genomeSol.length(); i++){
        cout << genomeSol.gene(i) << " ";
        if(i%9==8){
            cout << endl;
            cout << "    ";
        }
    }
    cout << endl << "Valor de fitness: " << ga.statistics().minEver() << endl << endl;

    system("pause");
    return 0;
}
