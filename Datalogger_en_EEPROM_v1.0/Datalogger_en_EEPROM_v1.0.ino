
/*
  #       _\|/_   A ver..., ¿que tenemos por aqui?
  #       (O-O)        
  # ---oOO-(_)-OOo---------------------------------
   
   
  ##########################################################
  # ****************************************************** #
  # *            DOMOTICA PARA AFICIONADOS               * #
  # *          DataLogger en EEPROM del 328p             * #
  # *          Autor:  Eulogio López Cayuela             * #
  # *                                                    * #
  # *       Versión v1.0      Fecha: 25/08/2019          * #
  # ****************************************************** #
  ##########################################################
*/

#define __VERSION__ "Datalogger EEPROM v1.0 nano BLE"

/*
  
 ===== NOTAS DE LA VERSION =====
 
  0.- Datalogger simple para cohete de agua.
      Se usa en algunas partes del codigo el led onboard (PIN13) para indicaciones
      pero en este montaje estamos usando un nano BLE que incorpora comunicaciones BT
      pero que no dispone de dicho led como en los NANOs o UNOs tradicionales.
      No afecta para nada al normal funcionamiento
  
  1.- Grabacion (en la EEPROM del 328p) de datos de temperatura y altura (no altitud), 
      tomando cota cero el punto de lanzamiento.
      Para ahorrar memoria de grabacion, pasamos los datos a enteros tras multiplicarlos por x100
      asi disponemos de dos decimales de resolucion y de este modo solo necesitamos 2 byte por variable y muestra,
      disponiendo en nuestro caso de 255 muestras, ya que reservamos las 4 primeras posiciones para otros usos.
      El tiempo disponible de grabacion vendra determinado por la frecuencia de toma de dichas muestras 
      que estara determinado mediante (INTERVALO_MUESTRAS), valor en milisegundos.

  2.- La grabacion de datos no se inicia hasta un cierto valor de altura (UMBRAL_LANZAMIENTO), valor en metros 
      para evitar quedarnos sin memoria, dadas las limitaciones que tenemos (maximo 1 kb)

  3.- Disponemos de un servo que activara la apertura del compartimento del paracaidas 
      cuando detecte que el cohete ha entrado en descenso mediante (UMBRAL_DESCENSO), valor en metros.
      En este momento en las posiciones 2 y 3 de la eeprom se guardara la altura maxima a efectos de respaldo.

  4.- Para que se active el protocolo de grabacion y se desencadenen el resto de acciones, 
      ha de darse la orden explicita mediante el puerto Serie (en este caso por BT).
      Se sispone tambien de varios comandos para pruebas que se pueden eliminar 
      si se desea minimizar el uso de ROM y RAM, aunque el programa es lo suficientemente ligero
      para no causar ningun tipo de inestabilidad en ese aspecto.
      
  5.- Los comando disponibles son los siguientes (indistintamente mayusculas o minusculas):
      - (P) Test del servo, giros en uno u otro sentido. Util para comprobar 
            el correcto funcionamiento del mecanismo de apertura del paracaidas.
      - (E) Habilita el uso de eeprom para guardar datos (¡por defecto se encuentra desactivada!).
            Hay que activarla de forma explicita mediante este comando.
      - (D) Deshabilita el uso de eeprom. (¡Ojo, Por defecto se encuentra desactivada!)
      - (L) Lista el contenido de la eeprom. Utilidad para sacar los datos a puerto Serie.      
      - (M) Activa/desactiva el monitor de cota cero, que mostrara por serie la cota actual
            de manera que podemos evaluar las variaciones que se producen estando en reposo 
            y asi ajustar mejor UMBRAL_LANZAMIENTO para evitar inicios de grabacion indeseados.
      -(0-9) Modifica el UMBRAL_LANZAMIENTO por esa cantidad de metros. Cero metros util para pruebas.
            Recomendables valores de almenos 2 o 3 metros, debido a la inestabilidad del sensor.
      - (A) finaliza la entrada de comandos e inicia el calibrado a cota CERO y la espera hasta superar 
            el UMBRAL_LANZAMIENTO para iniciar la grabacion de datos.
            
      Como nota indicar que no deberiamos dejar pasar mas de unos minutos antes del lanzamiento
      ya que la altura se calcula en relacion a la presion atmosferica y esta varia de forma continua 
      debido a cambios en el clima o simplemente por efecto de las 'mareas barometricas'.
      Dichos efectos los despreciamos durante la toma de muestras ya que seran periodos de pocos segundos.
      Un largo periodo entre la opcion (A) y el lanzamiento podria afectar a la cota Cero de referencia.
      y por tanto producirse errores en las mediciones o peor aun, iniciarse la grabacion 
      si UMBRAL_LANZAMIENTO es demasiado pequeño.

  6.- Una vez terminado el proceso de grabacion (eeprom completa), se desactiva el acceso a la misma para evitar 
      sobreescrituras y perdida de datos. Y se comienza a emitir por el puerto serie (en este caso BT)
      la altura maxima alcanzada durante el trayecto.

  7.- Si tras una grabacion de datos de interes se reinica el sistema, la eeprom queda a salvo 
      pues se necesita la intervencion expresa del usuario para que se active de nuevo la grabacion.
      Esto permite reiniciar el sistema conectado a un Pc despues de usarlo y listar por puerto serie
      los datos de la sesion anterior.
      No se necesita borrar la eeprom tras su uso, esta se sobreescribe completamente en la siguiente sesion.
      Ademas no es aconsejable borrarla ya que la memoria eeprom de los 328p tiene un numero limitado 
      (aunque elevado, aprox 100.000) de ciclos de escritura.
      
            

    Este Sketch usa 13632 bytes, el (44%) del espacio de ROM
    Las variables Globales usan 533 bytes, el (26%)  del espacio de RAM
 
      
  CONEXIONES:

 =======================
  ARDUINO     BMP280
 =======================

   A4   --->   SDA  
   A5   --->   SCL
   GND  --->   GND
   3.3v --->   Vcc

   
 =======================
  ARDUINO     Servo 9g
 =======================
   D10  --->   PWM  
   GND  --->   GND
   5v   --->   Vcc
   
*/ 



/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//        IMPORTACION DE LIBRERIAS 
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

#include <Wire.h>                 // Utilidades para comunicaciones I2C
#include <EEPROM.h>               // Funciones para lectura/escritura en eeprom
#include <Adafruit_BMP280.h>      // Biblioteca de funciones para el barometro
#include <Servo.h>                // Utilizamos la biblioteca estandar para servos


/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//        SECCION DE DECLARACION DE CONSTANTES  Y  VARIABLES GLOBALES
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

//------------------------------------------------------
//algunas definiciones personales para mi comodidad al escribir codigo
//------------------------------------------------------
#define AND    &&
#define OR     ||
#define NOT     !
#define ANDbit  &
#define ORbit   |
#define XORbit  ^
#define NOTbit  ~
//Versiones actuales de Arduino ya contemplan 'and', 'or'...  pero arrastro la costumbre de cuando no era asi

      
/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//        SECCION DE DECLARACION DE CONSTANTES  Y  VARIABLES GLOBALES
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

//------------------------------------------------------
//Otras definiciones para pines y variables
//------------------------------------------------------
#define PIN_LED_OnBoard         13    // Led on Board
#define PIN_SERVO_PARACAIDAS    10    // Servo para apertura del paracaidas
#define ALTITUD_BASE           640    // Altitud de uleila del campo (sin uso)
#define INTERVALO_MUESTRAS      30    // tiempo en milisegundos entre cada muestra
                                      // Recordad que disponemos de memoria para 255 muestras,
                                      // dependiendo del intervalo, tendremos mayor o menor tiempo de grabacion 

int UMBRAL_LANZAMIENTO = 2;           // Valor en metros que se ha de elevar para empezar la toma de muestras
float UMBRAL_DESCENSO = 2;            // Valor en metros que ha de descender desde la altura_max para 
                                      // que se active la apertura del paracaidas
                                      
/* creacion de un nuevo tipo de datos para contener las muestras */
struct struct_type_datos {
                          int16_t temperatura;   
                          int16_t altura;     
                         };


float altura_actual = 0;                  // Altura actual del cohete
float altura_max = 0;                     // Registro de altura maxima alcanzada. Dato de maximo interes, 
                                          // que tambien es usado para el despliegue del paracaidas
                                       
unsigned long momento_inicio = 0;         // Control de tiempos para la frecuencia de muestreo

boolean FLAG_paracaidas_auto = true;      // Para iniciar el despliegue del paracaidas en funcion de la altura
boolean FLAG_uso_eeprom = false;          // Preserva la eeprom de usos innecesarios
int puntero_eeprom = 4;                   // Reservamos las cuatro primeras posiciones:   0,1,2 y 3

float presion_lanzamiento;                // Usada como referencia para el calculos de la cota Cero



//------------------------------------------------------
// Creamos las instancia de los objetos:
//------------------------------------------------------

/* creaccion del objeto barometro/altimetro */
Adafruit_BMP280 bmp; // sensor barometrico conectado por I2C

/* creaccion del objeto servo */
Servo servo_paracaidas; 




//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm 
//***************************************************************************************************
//         FUNCION DE CONFIGURACION
//***************************************************************************************************
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm 

void setup() 
{
  Serial.begin(9600); 
  Serial.println(F(__VERSION__));   //  MostarVersion por puerto serie 

  pinMode(PIN_LED_OnBoard, OUTPUT);
  digitalWrite(PIN_LED_OnBoard, LOW);   

  pinMode(PIN_SERVO_PARACAIDAS, OUTPUT);
  servo_paracaidas.attach(PIN_SERVO_PARACAIDAS);
  servo_paracaidas.write(0);
        
  if (!bmp.begin(0x76)) {
    Serial.println(F("Error de Altimetro"));
    while (true);
  }
   
  /* esperando el pistoletazo de salida que hara que cuando empiece a elevarse, empieze a grabar */
  Serial.println(F("Esperando orden:"));
  int comando = 0;
  boolean FLAG_posicion_servo_open = false; //para alternar la posicion del servo con cada pulsacion de 'p'
  boolean FLAG_monitor_cota_cero = false;   //para activar/desactivar el monitor de cota cero con 'z'
  while(comando >= 0){
    comando = comandosPuertoSerie();
    if (comando > 9){
      UMBRAL_LANZAMIENTO = comando/10;
      if(UMBRAL_LANZAMIENTO>10){
        UMBRAL_LANZAMIENTO = 0;  //util para pruebas
      }
      Serial.print(F("UMBRAL_LANZAMIENTO = "));Serial.println(UMBRAL_LANZAMIENTO);
    }    
    if (comando == 1){
      FLAG_uso_eeprom = true;
      Serial.println(F("EEPROM Activada"));
    }
    if (comando == 2){
      FLAG_uso_eeprom = false;
      Serial.println(F("EEPROM Desactivada"));
    }
    if (comando == 3){
      FLAG_posicion_servo_open = NOT FLAG_posicion_servo_open;
      if (FLAG_posicion_servo_open == true){
        abrir_paracaidas();      
        Serial.println(F("PARACAIDAS Abierto"));
      }
      else{
        cerrar_paracaidas();      
        Serial.println(F("PARACAIDAS Cerrado"));
      }
    }
    if (comando == 4){
      listar_datos();
    }
    if (comando == 5){
      FLAG_monitor_cota_cero = NOT FLAG_monitor_cota_cero;
      Serial.print(F("Monitor Cota Cero "));Serial.println(FLAG_monitor_cota_cero?"activo":"off");
      if(FLAG_monitor_cota_cero == true){
        presion_lanzamiento = calcular_presion_cota_cero();
        momento_inicio = 0; 
      }
    }
    if(FLAG_monitor_cota_cero==true){
      if(millis()>momento_inicio){
        momento_inicio = millis()+2000;
        altura_actual = bmp.readAltitude(presion_lanzamiento);
        Serial.print(F("Cota Actual: "));Serial.println(altura_actual);
      }
    }
  }

  /* Advertencia si la eeprom esta desactivada */
  if(FLAG_uso_eeprom == false){Serial.println(F("Recuerda, EEPROM desactivada"));}

  /* ======= Obtener presion de referencia para 0 metros de altura ======= */
  presion_lanzamiento = calcular_presion_cota_cero();
  
  Serial.println(F("Sistema OK"));
  
  /* ======= Bucle para no grabar datos mientras se permanezca en la plataforma de lanzamiento =======*/
  while(altura_actual < UMBRAL_LANZAMIENTO){  
    altura_actual = bmp.readAltitude(presion_lanzamiento);
  }
  
  
  /* ======= Inicializamos la marca para temporizar la frecuencia de toma de datos =======*/
  momento_inicio = millis();
}



//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm 
//***************************************************************************************************
//  BUCLE PRINCIPAL DEL PROGRAMA   (SISTEMA VEGETATIVO)
//***************************************************************************************************
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm 


void loop() 
{

  /* ======= 'Reloj' para toma de datos =======*/
 
  unsigned long ahora = millis();
  if(ahora >= momento_inicio){
    momento_inicio = ahora + INTERVALO_MUESTRAS;
    
    /* Toma de datos de temperatura */
    float Temperatura = bmp.readTemperature();
    /* temperatura con dos decimales, como un entero */
    int temperatura_int = int(Temperatura*100);  

    /* Toma de datos de presion atmosferica */
    //float Presion = bmp.readPressure();      //sin uso
    
    /* Toma de datos de altura */
    altura_actual = bmp.readAltitude(presion_lanzamiento);
    /* altura con dos decimales, como un entero */
    int altura_int = int(altura_actual*100);

    /* empaquetado de los datos de interes en un 'struct' */
    struct_type_datos datos_actuales = { temperatura_int, altura_int, };  // 32.15 ºC = 3215, 24.34 m = 2434
    
    //if(FLAG_uso_eeprom==false){ puntero_eeprom+=4;}     //incrementado aqui para debug

    /* Salvado de datos (si procede)*/
    if(FLAG_uso_eeprom==true){
      saveData(puntero_eeprom, datos_actuales);         //enviamos un dato del tipo 'struct_type_datos'
      puntero_eeprom+=4;                                //incrementamos el puntero para escritura
      /* Si llenamos la eeprom, dejamos de grabar y desactivamos los permisos de acceso*/
      if(puntero_eeprom > 1020 OR puntero_eeprom < 4){ 
        FLAG_uso_eeprom = false; // bloqueo de acceso para evitar sobreescribir
      }
    }
    
    /* ======= Actualizamos el dato de altura maxima alcanzada, si procede ======= */
    if (altura_actual > altura_max){
      altura_max = altura_actual;
    }
    
    /* ======= Control de caida para apertura del paracaidas ======= */
    if (FLAG_paracaidas_auto == true AND altura_actual < (altura_max - UMBRAL_DESCENSO)){
      //iniciar movimiento del servo para abrir el paracaidas
      abrir_paracaidas();
      
      //aprovechamos la condicion de descenso para grabar en eeprom la altura maxima
      EEPROM.write(2, int(altura_max));                       //en la posicion 2, la parte entera
      EEPROM.write(3, int((altura_max-int(altura_max))*100)); //en la posicion 3, la parte decimal
      
      servo_paracaidas.detach();     //desactivarlo si queremos evitar que el servo se quede consumiendo
      FLAG_paracaidas_auto = false;  //para evitar nuevos accesos al servo
    }
    
    /*
    // --------> DEBUG, para calcular lo que tarda un ciclo, ver muestras de datos, etc...
    if(puntero_eeprom==8){
      //Serial.print(F("tiempo de ciclo = "));Serial.println(millis()-ahora);
      //Serial.print(F("presion "));Serial.println(presion_lanzamiento);
      //Serial.print(F("altura max "));Serial.println(altura_max);
      Serial.print(F("Temperatura "));Serial.println(float(temperatura_int)/100.0);
      Serial.print(F("Altura "));Serial.println(float(altura_int)/100.0);
    }
    // <-------- DEBUG
    */
  }

  /* ======= Tras entrar en bajada, si se agota la eeprom comenzamos a enviar por BT la altura maxima ======= */
  if(FLAG_paracaidas_auto == false AND FLAG_uso_eeprom == false){
    Serial.flush();
    Serial.print(F("ALTURA MAX: "));Serial.println(altura_max);
    digitalWrite(PIN_LED_OnBoard, HIGH);
    delay(500);
    digitalWrite(PIN_LED_OnBoard, LOW);
    delay(4500); 
  } 
}



/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
   ###################################################################################################### 
        BLOQUE DE FUNCIONES: LECTURAS DE SENSORES, COMUNICACION SERIE, CONTROL LCD...
   ###################################################################################################### 
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/


/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//    COMUNICACIONES (PUERTO SERIE) 
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

//========================================================
// FUNCION PARA LECTURA DE CARACTERES POR PUERTO SERIE
//========================================================


int comandosPuertoSerie() 
{
  char orden_recibida = ' ';
   while(Serial.available()) {
    orden_recibida = Serial.read();
    int valor_tecla = int(orden_recibida-48);
    if(valor_tecla >=0 AND valor_tecla <10){
      if(valor_tecla==0){
        return 1000;            //un valor excesivo lo interpretaremso como cero
      }
      else{
        return valor_tecla*10;  //valor multiplicado por 10 para no pisar otros codigos ya en uso
      }
    }
    if(orden_recibida == 'e' OR orden_recibida == 'E'){  //desbloqueamos acceso a la eeprom
      return 1;
    }
    if(orden_recibida == 'd' OR orden_recibida == 'D'){  // bloquear acceso eeprom (solo DEBUG)
      return 2;
    }
    if(orden_recibida == 'p' OR orden_recibida == 'P'){  // abrir/cerrar paracaidas (solo DEBUG)
      return 3;
    }

    if(orden_recibida == 'l' OR orden_recibida == 'L'){  // listar datos de eeprom
      return 4;
    }
    if(orden_recibida == 'm' OR orden_recibida == 'M'){  // activa/desactiva monitor cota cero
      return 5;
    }
    if(orden_recibida == 'a' OR orden_recibida == 'A'){  //activamos el lanzamiento 
      return -1;
    }
  }
  return 0;
}



/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//   PARACAIDAS
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

//========================================================
//  ABRIR PARACAIDAS
//========================================================

void abrir_paracaidas()
{
  for (int i=0; i < 180; i++){
    servo_paracaidas.write(i);
    delay(4);   
  } 
}


//========================================================
//  CERRAR PARACAIDAS
//========================================================

void cerrar_paracaidas()
{
  for (int i=180; i >= 0; i--){
    servo_paracaidas.write(i);
    delay(4);   
  }
}



/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//   CALCULO DE COTA CERO
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

//========================================================
//  CALCULAR PRESION INICIAL PARA REFERENCIA DE COTA CERO
//========================================================

float calcular_presion_cota_cero()
{
  Serial.println(F("Calculando cota Cero..."));
  boolean FLAG_blink_led = true;
  float presion_cota_cero = 0;
  for(int s=1; s<=10; s++){
    presion_cota_cero += bmp.readPressure();
    digitalWrite(PIN_LED_OnBoard, FLAG_blink_led);
    FLAG_blink_led = NOT FLAG_blink_led;  //parpadeo del led onboard mientras se calibran los 0 metros
    delay(500);
  }
  return presion_cota_cero/1000.0;
}
/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//   EEPROM
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

//========================================================
//  LEER DATOS ALMACENADOS EN EPPROM
//========================================================

struct_type_datos loadData(int posicion)
{
  struct_type_datos muestra;
  EEPROM.get(posicion, muestra);
  return muestra;
}


//========================================================
//  SALVAR DATOS EN LA EPPROM
//========================================================

void saveData(int posicion, struct_type_datos muestra)
{
  EEPROM.put(posicion, muestra);
}


//========================================================
//  LISTAR EL CONTENIDO DE LA EPPROM
//========================================================

void listar_datos()
{
  int puntero_lectura = 2;
  float altura_max;
  byte altura_max_int = EEPROM.read(puntero_lectura); //puntero_lectura = 2
  puntero_lectura++;
  byte altura_max_dec = EEPROM.read(puntero_lectura); //puntero_lectura = 3
  altura_max = (altura_max_int * 100 + altura_max_dec)/100.0;
  Serial.print(F("Altura max: "));Serial.print(altura_max);Serial.println(F(" m\n"));
  
  puntero_lectura++;  //puntero ahora vale 4, primera posicion de las muestras de vuelo

  int tiempo = 0;
  Serial.println(F("Tiempo (ms),\tTemperatura (C),\tAltura (m)"));
  
  while(puntero_lectura < 1020){
    struct_type_datos dato_leido;
    tiempo += INTERVALO_MUESTRAS;
    EEPROM.get(puntero_lectura, dato_leido);
    float temperatura_float = float(dato_leido.temperatura)/100.0;
    float altura_float = float(dato_leido.altura)/100.0;
    
    Serial.print(tiempo); Serial.print(F(",\t\t"));
    Serial.print(temperatura_float); Serial.print(F(",\t\t\t"));
    Serial.println(altura_float);
    puntero_lectura +=4;
  }
}



//*******************************************************
//                    FIN DE PROGRAMA
//*******************************************************
