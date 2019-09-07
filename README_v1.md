# Datalogger en EEPROM interna del uC 328p
 Usando Arduino NANO BLE
  
 ===== NOTAS DE LA VERSION =====
 
  0.- Datalogger simple para cohete de agua que graba los datos directamente en la 
      memoria eeprom del microcontrolador ATmega328p
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
