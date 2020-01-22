# Datalogger en EEPROM interna del uC 328p
 Usando Arduino NANO
  
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

  4.- Para que se active el protocolo de grabacion mediante el control de (UMBRAL_LANZAMIENTO) 
      y se desencadenen el resto de   acciones, ha de darse la orden explicita mediante la utilizacion del pulsador.
      
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

                    
    Este Sketch usa 14482 bytes, el (47%) del espacio de ROM
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



 ---  COLORES DE REFERENCIA PARA LA TIRA / LEDS DIRECCIONABLES --- 

  (255, 000, 000);    // rojo
  (000, 255, 000);    // verde
  (000, 000, 255);    // azul
  (100, 040, 000);    // naranja suave
  (100, 000, 060);    // rosa suave
  

