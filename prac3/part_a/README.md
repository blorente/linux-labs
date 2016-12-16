# Práctica 3, parte A - Scripts

Junto con el código del módulo del kernel se distribuyen diferentes scripts para comprobar diferentes grados de correción:

- **Testing estático:** Los scripts `mount.sh` y `unmount.sh` contienen instrucciones para _compilar y montar_ y _desmontar y limpiar_ (respectivamente) el módulo del kernel. Para hacer una comprobación rápida y detectar errores superficiales, basta con ejecutar `testmount.sh`, el cual ejecuta los scripts anteriores en orden e imprime el log del kernel para detectar cualquier fallo en la inicialización.

- **Identificar regresiones:** Para asegurarnos de que no causamos errores nuevos en el código original al añadir nueva funcionalidad, se incluye un test de acceso secuencial a la lista, en el script `testsimple.sh`. Este script es muy parecido al utilizado en la práctica 1.

- **Test de concurrencia:** Para poder comprobar el funcionamiento correcto de la concurrencia, se incluyen varios scripts que realizan repetidamente operaciones típicas de acceso a secciones críticas. Cada uno de estos scripts debería poder ser ejecutado desde una termimal diferente, en una combinación de tipo y número arbitrarias. Por ejemplo, podrían abrirse 4 terminales y ejecutar un proceso monitor, un servidor, y dos clientes.
	- `serve.sh`: Un proceso productor que inserta elementos en la lista cada cierto intervalo.
	- `consume.sh`: Un proceso consumidor que _vacía la lista_ e imprime sus contenidos cada cierto intervalo.
	- `monitor.sh`: Un preso _no-modificante_ que simplemente imprime los contenidos de la lista cada poco tiempo.
