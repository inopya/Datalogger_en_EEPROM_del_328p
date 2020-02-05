
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
  # *       Versión v2.0      Fecha: 07/09/2019          * #
  # ****************************************************** #
  ##########################################################
*/

#define __VERSION__ "Datalogger EEPROM v2.0 ESCUDO VUELO"

/*
  
 ===== NOTAS DE LA VERSION =====
 
 
  0.- Datalogger simple para cohete de agua.
      Utilizando Arduino NANO y prototipo de escudo para facilitar el conexionado y evitar cables.
      Al utilizarse un Nano tradicional no disponemos de opcion de ordenes por BT, auqneu se mantiene 
      todo el codigo relativo al puerto serie para tareas de debug y para poder listar los datos recopilados.
      Pera desencadenar el proceso de lanzamiento se utilizara un pulsador que activara el modo de espera 
      hasta que se sobrepase el UMBRAL_LANZAMIENTO. Debemos por tanto pulsarlo cuando el cohete ya este
      correctamente colocado en su base de lanzamiento.
      En esta version se usa la libreria i2c_BMP280 perteneciente al paquete iLib
      Se incluye aqui dicha libreria ya que esta ligeramente retocada por comodidad al llamar a las funciones.
      
  
  1.- Grabacion (en la EEPROM del 328p) de datos de tiempo y altura (no altitud), 
      tomando cota cero el punto de lanzamiento.
      Para ahorrar memoria de grabacion, pasamos los datos de altura a enteros tras multiplicarlos por x100
      asi disponemos de dos decimales de resolucion y de este modo solo necesitamos 2 byte por variable y muestra,
      disponiendo en nuestro caso de 255 muestras, ya que reservamos las 4 primeras posiciones para otros usos.
      El tiempo disponible de grabacion vendra determinado por la frecuencia de toma de dichas muestras 
      que estara determinado mediante (INTERVALO_MUESTRAS), valor en milisegundos.
      La variable tiempo se tomará de millis().

  2.- La grabacion de datos no se inicia hasta un cierto valor de altura (UMBRAL_LANZAMIENTO), valor en metros 
      para evitar quedarnos sin memoria, dadas las limitaciones que tenemos (maximo 1 kb)

  3.- Disponemos de un servo que activara la apertura del compartimento del paracaidas 
      cuando detecte que el cohete ha entrado en descenso mediante (UMBRAL_DESCENSO), valor en metros.
      En este momento en las posiciones 2 y 3 de la eeprom se guardara la altura maxima a efectos de respaldo.

  4.- Para que se active el protocolo de grabacion y se desencadenen el resto de acciones, 
      ha de darse la orden explicita mediante la utilizacion del pulsador.
      
  5.- Los comando disponibles por puerto serie y a efectos de DEBUG son los siguientes,
      indistintamente mayusculas o minusculas:
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
      - (Z) Elimina la condicion de superar el UMBRAL_LANZAMIENTO. Inicia el calibrado a cota CERO
            y espera la pulsacion de (A) para que se inicie la grabacion de datos.
            
      Como nota indicar que no deberiamos dejar pasar mas de unos minutos antes del lanzamiento
      ya que la altura se calcula en relacion a la presion atmosferica y esta varia de forma continua 
      debido a cambios en el clima o simplemente por efecto de las 'mareas barometricas'.
      Dichos efectos los despreciamos durante la toma de muestras ya que seran periodos de pocos segundos.
      Un largo periodo entre la pulsacion y el lanzamiento podria afectar a la cota Cero de referencia.
      y por tanto producirse errores en las mediciones o peor aun, iniciarse la grabacion 
      si UMBRAL_LANZAMIENTO es demasiado pequeño.

  6.- Una vez terminado el proceso de grabacion (eeprom completa), se desactiva el acceso a la misma para evitar 
      sobreescrituras y perdida de datos. Y se comienza a emitir por el puerto serie la altura maxima alcanzada
      durante el trayecto.

  7.- Si tras una grabacion de datos de interes se reinica el sistema, la eeprom queda a salvo 
      pues se necesita la intervencion expresa del usuario mediante el uso del pulsador para que se 
      desencadene de nuevo el proceso de calibrado y se active de nuevo la grabacion.
      Esto permite reiniciar el sistema conectado a un Pc despues de usarlo y listar por puerto serie
      los datos de la sesion anterior medinte el comando (L).
      No se necesita borrar la eeprom tras su uso, esta se sobreescribe completamente en la siguiente sesion.
      Ademas no es aconsejable borrarla ya que la memoria eeprom de los 328p tiene un numero limitado 
      (aunque elevado, aprox 100.000) de ciclos de escritura.
      
  8.- Version que graba tiempo referenciado a  "millis()" y altura
      De modo que podamos reconstruir la trayectoria parabolica lo mejor posible 

  9.- Se mantienen todos los comandos serie por una cuestion meramente nostalgica,
      (aunque siguen siendo utiles para debug)
      Perro está preparada para arduino NANO sin ningun tipo de conexion inalambrica
      Se ha implementado un pequeño escudo para facilitar las conexiones y evitar cables, 
      de forma que sea un montaje mas robustos y soporte mejor los devenires de los vuelos.
      Dispone de un pulsador que será el pistoletazo de salida para empezar a calibrarse 
      y esperar a detectar movimiento para comenzar a registrar datos.
      Dispone tambien de un led rgb con el que nos indicará los estados
      - En reposo, previo a la pulsacion, parpadeo (NARANJA) con intervalos de medio segundo.
        En este modo podemos interactuar por el puerto serie para debug y listar datos de vuelo.
      - Tras la pulsacion se activa el proceso de calibrado de cota cero y grabacion de vuelo
      - Preparacion y activacion de la eeprom (MORADO), luz fija durante 2'5 segundos
      - Calibracion de cota cero, parpadeo (ROJO) mientras dura la calibracion. 
      - Espera para deteccion de movimiento (VERDE), color fijo mientras no se alcance el umbral de lanzamiento.
      - Sobrepasado dicho umbral, comienza la Grabacion (AZUL) fijo.
      - Una vez se agote la eprom, parpadeos azules hasta el proximo reinicio.

                    
    Este Sketch usa 14490 bytes, el (47%) del espacio de ROM
    Las variables Globales usan 579 bytes, el (28%)  del espacio de RAM
 
      
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



 =======================
  ARDUINO     LED RGB
 =======================
   D2   --->   DATA  
   GND  --->   GND
   5v   --->   Vcc


 =======================
  ARDUINO     PULSADOR
 =======================
  5v    --->   PIN1  
  D2    --->   PIN2


 =======================
     RESISTENCIA 1MH
 =======================
  D2 <--- R  --->   GND  

*/ 

/*
 ---  COLORES DE REFERENCIA PARA LA TIRA / LEDS DIRECCIONABLES --- 

  (255, 000, 000);    // rojo
  (000, 255, 000);    // verde
  (000, 000, 255);    // azul
  (100, 040, 000);    // naranja suave
  (100, 000, 060);    // rosa suave
  
*/


/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//        IMPORTACION DE LIBRERIAS 
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

#include <Wire.h>                 // Utilidades para comunicaciones I2C
#include <EEPROM.h>               // Funciones para lectura/escritura en eeprom
//#include <Adafruit_BMP280.h>      // Biblioteca de funciones para el barometro
#include <i2c_BMP280.h>           // Biblioteca de funciones para el barometro

#include <Adafruit_NeoPixel.h>    // Incluir biblioteca Adafruit NeoPixel

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
//Versiones actuales de Arduino IDE ya contemplan 'and', 'or'...  pero arrastro la costumbre de cuando no era asi

      
/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//        SECCION DE DECLARACION DE CONSTANTES  Y  VARIABLES GLOBALES
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

//------------------------------------------------------
//Otras definiciones para pines y variables
//------------------------------------------------------
#define PIN_LED_OnBoard         13    // Led on Board
#define PIN_PULSADOR             2
#define PIN_SERVO_PARACAIDAS    10    // Servo para apertura del paracaidas
#define ALTITUD_BASE           640    // Altitud de uleila del campo (sin uso)
#define INTERVALO_MUESTRAS      60    // tiempo en milisegundos entre cada muestra
                                      // Recordad que disponemos de memoria para 255 muestras,
                                      // dependiendo del intervalo, tendremos mayor o menor tiempo de grabacion 

int UMBRAL_LANZAMIENTO = 2;           // Valor en metros que se ha de elevar para empezar la toma de muestras
float UMBRAL_DESCENSO = 2;            // Valor en metros que ha de descender desde la altura_max para 
                                      // que se active la apertura del paracaidas
                                      
/* creacion de un nuevo tipo de datos para contener las muestras */
struct struct_type_datos { uint16_t tiempo;  int16_t altura;  };


float altura_actual = 0;                  // Altura actual del cohete
float altura_max = 0;                     // Registro de altura maxima alcanzada. Dato de maximo interes, 
                                          // que tambien es usado para el despliegue del paracaidas
                                       
unsigned long momento_inicio = 0;         // Control de tiempos para la frecuencia de muestreo

boolean FLAG_estado_pulsador = false;     //bandera de control apra la interrupcion del pulsador
boolean FLAG_paracaidas_enable = true;    // Habilita el uso de paracaidas.
boolean FLAG_paracaidas_cerrado = true;   // Para iniciar el despliegue del paracaidas en funcion de la altura
boolean FLAG_grabacion_retrasada = true;  // True = espera a que comience a elevarse ara registrar datos
boolean FLAG_uso_eeprom = false;          // Preserva la eeprom de usos innecesarios
int puntero_eeprom = 4;                   // Reservamos las cuatro primeras posiciones:   0,1,2 y 3

float presion_lanzamiento;                // Usada como referencia para el calculos de la cota Cero
unsigned int tiempo_cero;                 // Referencia del momento de inicio de toma de muestras
unsigned long momento_blink = 0;          // Control de tiempos para parpadeo del led en espera
unsigned long momento_monitor = 0;        // Control de tiempos mostrar datos en modo monitor de cota cero

//------------------------------------------------------
// Creamos las instancia de los objetos:
//------------------------------------------------------

/* creaccion del objeto barometro/altimetro */
//Adafruit_BMP280 bmp; // sensor barometrico conectado por I2C
BMP280 bmp280;

/* creaccion del objeto servo */
Servo servo_paracaidas; 

/* Crear el 'objeto' para controlar los led */
#define PIN_TIRA_LED      12                   // patilla para la tira
#define TIPO_LED         NEO_GRB + NEO_KHZ800  //tipo controlador de los led
byte LONGITUD_TIRA = 1;                        // total de leds
Adafruit_NeoPixel tiraLEDS = Adafruit_NeoPixel(LONGITUD_TIRA, PIN_TIRA_LED, TIPO_LED);



//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm 
//***************************************************************************************************
//         FUNCION DE CONFIGURACION
//***************************************************************************************************
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm 

void setup() 
{
  Serial.begin(115200); 
  Serial.println(F(__VERSION__));   //  MostarVersion por puerto serie 

  pinMode(PIN_LED_OnBoard, OUTPUT);
  digitalWrite(PIN_LED_OnBoard, LOW);   

  pinMode(PIN_PULSADOR, INPUT);  

  tiraLEDS.begin();
  tiraLEDS.setBrightness(20);
  mostrarColor(000, 000, 000);   //NEGRO, led apagado

  pinMode(PIN_SERVO_PARACAIDAS, OUTPUT);
  servo_paracaidas.attach(PIN_SERVO_PARACAIDAS);
  servo_paracaidas.write(0);
        
  if (bmp280.initialize()) Serial.println(F("Sensor ok"));
  else
  {
      Serial.println(F("Fallo de Sensor"));
      while (true) {}
  }

  /* Configuracion de calibracion */
  bmp280.setPressureOversampleRatio(8);     //2
  bmp280.setTemperatureOversampleRatio(1);
  bmp280.setFilterRatio(4);                //4
  bmp280.setStandby(0);                     // 0=desactivado, por tanto el sensor esta activo.


  /* Medidas bajo demanda */
  bmp280.setEnabled(0);                     //0=Desactivamos el modo automatico. Solo obtendremos respuesta
                                            //del sensor  tras una peticion con 'triggerMeasurement()'

  int contador_medidas = 0;
  /* esperando el pistoletazo de salida que hara que cuando empiece a elevarse, empieze a grabar */
  Serial.println(F("Esperando orden:"));
  int comando = 0;
  boolean FLAG_posicion_servo_open = false; //para alternar la posicion del servo con cada pulsacion de 'p'
  boolean FLAG_monitor_cota_cero = false;   //para activar/desactivar el monitor de cota cero con 'z'

  momento_inicio = millis();      // 'Reloj' para control de la toma de datos
  boolean FLAG_blink_led = false;
  
  attachInterrupt(digitalPinToInterrupt(PIN_PULSADOR), atenderInterrupcion, RISING); //  FALLING
  
  while(comando >= 0){
    unsigned long ahora = millis();
    if(ahora >= momento_blink){
      momento_blink = ahora + 500;
      FLAG_blink_led = NOT FLAG_blink_led;
    }
    FLAG_blink_led ? mostrarColor(100, 040, 000) : mostrarColor(0,0,0); //parpadeo NARANJA
    
    comando = comandosPuertoSerie();
    
    if(FLAG_estado_pulsador==true){  ///digitalRead(PIN_PULSADOR)==true){
      delay(20);
      FLAG_uso_eeprom = true;
      mostrarColor(100, 0, 60);    // rosa suave, eeprom activada
      delay(2500);
      comando = -1;  //anulamos cualquier otra orden de puerto serie;
      break; // mejor interrumpimos??
    }

    /* modificar el unmbral de lanzamiento */
    if (comando > 9){
      UMBRAL_LANZAMIENTO = comando/10;
      if(UMBRAL_LANZAMIENTO>10){
        UMBRAL_LANZAMIENTO = 0;  //util para pruebas
      }
      Serial.print(F("UMBRAL_LANZAMIENTO = "));Serial.println(UMBRAL_LANZAMIENTO);
    }
    
    /* activar el uso de la eeprom */    
    if (comando == 1){
      FLAG_uso_eeprom = true;
      Serial.println(F("EEPROM Activada"));
    }
    
    /* desactivar el uso de la eeprom */ 
    if (comando == 2){
      FLAG_uso_eeprom = false;
      Serial.println(F("EEPROM Desactivada"));
    }

    /* probar el funcionamiento del servo */ 
    if (comando == 3){
      FLAG_posicion_servo_open = NOT FLAG_posicion_servo_open;
      if (FLAG_posicion_servo_open == true){
        abrir_paracaidas();  //servo_paracaidas.write(90);     
        Serial.println(F("PARACAIDAS Abierto"));
      }
      else{
        cerrar_paracaidas(); //servo_paracaidas.write(0);    
        Serial.println(F("PARACAIDAS Cerrado"));
      }
    }

    /* listar datos de vuelo almacenados en eeprom */ 
    if (comando == 4){
      listar_datos();
    }

    /* activar / desactivar el visor de cota actual en tiempo real */ 
    if (comando == 5){
      FLAG_monitor_cota_cero = NOT FLAG_monitor_cota_cero;
      Serial.print(F("Monitor Cota Cero "));Serial.println(FLAG_monitor_cota_cero?"activo":"off");
      if(FLAG_monitor_cota_cero == true){
        presion_lanzamiento = calcular_presion_cota_cero();
        momento_inicio = 0; 
      }
    }
    if(FLAG_monitor_cota_cero==true){
      if(millis()>momento_monitor){
        momento_monitor = millis()+2000;
        bmp280.triggerMeasurement(); //peticion de nueva medicion
        bmp280.awaitMeasurement();   //esperar a que el sensor termine la medicion
        float temperatura = bmp280.getTemperature();
        altura_actual = bmp280.getAltura(presion_lanzamiento);
        Serial.print(F("Cota Actual: "));Serial.print(altura_actual);
        Serial.print(F("m,  T: "));Serial.println(contador_medidas++);
      }
    }
    /* activar la grabacion instantanea sin esperar a que inicie el movimiento */ 
    if (comando == 6){                                      // habilita comenzar a grabar de forma instantanea
      presion_lanzamiento = calcular_presion_cota_cero();   // ojo, puede dejarnos sin memoria
      FLAG_grabacion_retrasada = false;                     //  si nos retrasamos en el lanzamiento
    }                                    
  }

  /* Advertencia si la eeprom esta desactivada */
  if(FLAG_uso_eeprom == false){Serial.println(F("Recuerda, EEPROM desactivada"));}

  Serial.println(F("Sistema OK"));

  if(FLAG_grabacion_retrasada == true){
    /* ======= Obtener presion de referencia para 0 metros de altura ======= */
    mostrarColor(255, 000, 000);    // rojo
    presion_lanzamiento = calcular_presion_cota_cero();
  
    /* ======= Bucle para no grabar datos mientras se permanezca en la plataforma de lanzamiento =======*/
    mostrarColor(000, 255, 000);    // verde
    altura_actual = 0; //reseteamos la cota actual
    while(altura_actual < UMBRAL_LANZAMIENTO){
      //Serial.print(F("altura_Actual: "));Serial.println(altura_actual);  //DEBUG
      bmp280.triggerMeasurement(); //peticion de nueva medicion
      bmp280.awaitMeasurement();   //esperar a que el sensor termine la medicion
      float temperatura = bmp280.getTemperature();
      altura_actual = bmp280.getAltura(presion_lanzamiento); //calculo de cota actual
    }
    mostrarColor(000, 000, 255);    // azul
  }
  
  /* ======= Inicializamos la marcas para temporizar  =======*/ 
  momento_inicio = millis();      // 'Reloj' para control de la toma de datos
  tiempo_cero = momento_inicio;   // Referencia de tiempo cero para muestras
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

    /* Toma de datos de altura */
    bmp280.triggerMeasurement(); //peticion de nueva medicion
    bmp280.awaitMeasurement();   //esperar a que el sensor termine la medicion
    altura_actual = bmp280.getAltura(presion_lanzamiento);
    /* altura con dos decimales, como un entero */
    int altura_int = int(altura_actual*100);
    
    /* Calculo del tiempo de la muestra */
    unsigned long tiempo_muestra = ahora-tiempo_cero; //operacion como 'long' para evitar perdida de informacion
    uint16_t tiempo_int = tiempo_muestra;     //milisegundos (como entero) desde que se inica la grabacion 
                                              //para tener una referencia de tiempo real en la que apoyar los
                                              //calculos de velocidades, aceleraciones... etc
    
    /* empaquetado de los datos de interes en un 'struct' */
    struct_type_datos datos_actuales = { tiempo_int, altura_int, };  // 3215 ms = 3215, 24.34 m = 2434
    
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
    if (FLAG_paracaidas_cerrado == true AND altura_actual < (altura_max - UMBRAL_DESCENSO)){
      //iniciar movimiento del servo para abrir el paracaidas
      if(FLAG_paracaidas_enable == true){       // podremos deshabilitarlo y evitar la perdia de muestras
        abrir_paracaidas();                     // que se origina mientras se accede al servo
      }
      FLAG_paracaidas_cerrado = false;          // para evitar nuevos accesos al servo
      
      /* aprovechamos la condicion de descenso para grabar en eeprom un registro con la altura maxima */
      if(FLAG_uso_eeprom == true){
        //asi evitamos que la altura maxima de sobreescriba.
        //pues en versiones anteriores, si simulamos el proceso de lanzamiento 
        //aun con la eeprom desactivada, el registro de altura maxima si se realizaba
        //lo que llevaba a perder dicho registro, aunque se mantenian los datos del vuelo.
        EEPROM.write(2, int(altura_max));                       //en la posicion 2, la parte entera
        EEPROM.write(3, int((altura_max-int(altura_max))*100)); //en la posicion 3, la parte decimal
      }      
      servo_paracaidas.detach();    //desactivarlo si queremos evitar que el servo se quede consumiendo
    }
  }

  /* ======= Tras entrar en bajada, si se agota la eeprom comenzamos a mostrar por Serie la altura maxima ======= */
  if(FLAG_paracaidas_cerrado == false AND FLAG_uso_eeprom == false){
    Serial.flush();
    Serial.print(F("ALTURA MAX: "));Serial.println(altura_max);
    digitalWrite(PIN_LED_OnBoard, HIGH);
    mostrarColor(000, 000, 100);
    delay(1000);
    digitalWrite(PIN_LED_OnBoard, LOW);
    mostrarColor(0,0,0);
    delay(2500); 
  } 
}



/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
   ###################################################################################################### 
        BLOQUE DE FUNCIONES: LECTURAS DE SENSORES, COMUNICACION SERIE, CONTROL LCD...
   ###################################################################################################### 
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/


/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//  INTERRUPCIONES
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

//========================================================
//  FUNCION ATENDER INTERRUPCIONES DEL PUSADOR
//========================================================
void atenderInterrupcion()
{
  FLAG_estado_pulsador = true;
} 



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
        return 1000;            //un valor excesivo lo interpretaremos como cero
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
    if(orden_recibida == 'z' OR orden_recibida == 'Z'){  //actualizamos cota cero 
      return 6;
    }
    if(orden_recibida == 'a' OR orden_recibida == 'A'){  //luz verde para el 'lanzamiento' 
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
  servo_paracaidas.write(90);
  return;
}


//========================================================
//  CERRAR PARACAIDAS
//========================================================

void cerrar_paracaidas()
{
  servo_paracaidas.write(0);
  return;
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
    bmp280.triggerMeasurement(); //peticion de nueva medicion
    bmp280.awaitMeasurement();   //esperar a que el sensor termine la medicion
    float temperatura = bmp280.getTemperature();
    presion_cota_cero += bmp280.getPressure();
    digitalWrite(PIN_LED_OnBoard, FLAG_blink_led);
    FLAG_blink_led ? mostrarColor(100, 000, 000) : mostrarColor(0,0,0);
    FLAG_blink_led = NOT FLAG_blink_led;  //parpadeo del led onboard mientras se calibran los 0 metros
    delay(250);
    
  }
  return presion_cota_cero/10.0;
}



/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//    CONTROL DE LA TIRA DE LEDS (ireccionable) PARA CREAR LOS EFECTOS DE COLOR
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

//========================================================
//  MOSTRAR UN COLOR (tira completa)
//========================================================

void mostrarColor(byte red, byte green, byte blue)
{
  for (int pixel=0; pixel< tiraLEDS.numPixels(); pixel++){
    tiraLEDS.setPixelColor(pixel, tiraLEDS.Color(red, green, blue)); //color unico 
  } 
  tiraLEDS.show();
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

  Serial.println(F("Tiempo (ms),\t\tAltura (m)"));
  
  while(puntero_lectura < 1020){
    struct_type_datos dato_leido;
    EEPROM.get(puntero_lectura, dato_leido);
    float altura_float = float(dato_leido.altura)/100.0;
    
    Serial.print(dato_leido.tiempo); Serial.print(F(",\t\t"));
    Serial.println(altura_float);
    puntero_lectura +=4;
  }
}



//*******************************************************
//                    FIN DE PROGRAMA
//*******************************************************
