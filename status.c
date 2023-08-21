#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

unsigned int inicial1[10], inicial2[10];
unsigned int actual1[10], actual2[10];
unsigned int wifiIS, wifiAS, wifiIB, wifiAB;

int cpu1, cpu2;

//Funciones
char *getstatus(char command[]){
	FILE *fp;
	static char salida[100];
	fp = popen(command, "r");
	fgets(salida, sizeof(salida), fp);
	pclose(fp);
	salida[strcspn(salida, "\n")] = '\0';
	return salida;
}

void getproc(char core[], int type){
	char comando[200];
	for (int i = 0; i != 10; i++){
		sprintf(comando, "cat /proc/stat | grep '%s' | awk '{print $%d}'", core, i + 2);
		if (type == 0) inicial1[i] = atoi(getstatus(comando));
		else if (type == 1) inicial2[i] = atoi(getstatus(comando));
		else if (type == 2) actual1[i] = atoi(getstatus(comando));
		else if (type == 3) actual2[i] = atoi(getstatus(comando));
	}
}

unsigned int get_t(unsigned int type[]){
	return type[0] + type[1] + type[2] + type[3] + type[4] + type[5] + type[6] + type[7] + type[8] + type[9];
}

void compare(){
	wifiIB = atoi(getstatus("cat /proc/net/dev | grep 'wlan0:' | awk '{print $2}'"));
	wifiIS = atoi(getstatus("cat /proc/net/dev | grep 'wlan0:' | awk '{print $10}'"));
	getproc("cpu0", 0);
	getproc("cpu1", 1);
	usleep(1000000);
	wifiAB = atoi(getstatus("cat /proc/net/dev | grep 'wlan0:' | awk '{print $2}'"));
	wifiAS = atoi(getstatus("cat /proc/net/dev | grep 'wlan0:' | awk '{print $10}'"));
	getproc("cpu0", 2);
	getproc("cpu1", 3);
	
	//Idles
	unsigned int idlet_1 = actual1[3] - inicial1[3];
	unsigned int idlet_2 = actual2[3] - inicial2[3];
	
	//Tiempo
	unsigned int totalt1 = get_t(actual1) - get_t(inicial1);
	unsigned int totalt2 = get_t(actual2) - get_t(inicial2);
	
	cpu1 = 100.0 * (1 - ((double) idlet_1 / (double)totalt1));
	cpu2 = 100.0 * (1 - ((double) idlet_2 / (double)totalt2));
}

void colores(){
	start_color(); //Iniciar colores
	init_pair(1, COLOR_YELLOW, COLOR_BLACK);
	init_pair(2, COLOR_WHITE, COLOR_BLACK);
	init_pair(3, COLOR_CYAN, COLOR_BLACK);
}

//Programa
int main(){
	initscr(); //Iniciar ncurses
	colores(); //Iniciar colores
	curs_set(FALSE); //Ocultar cursor
	noecho(); //Desactivar impresión automática
	
	//Bucle
	while (1){
		clear();
		attron(COLOR_PAIR(1));
		mvprintw(0, 26, "Estado del sistema");
		attroff(COLOR_PAIR(1));
		
		//Tiempo
		attron(COLOR_PAIR(2));
		mvprintw(2, 5, "Tiempo");
		attroff(COLOR_PAIR(2));
		attron(COLOR_PAIR(3));
		mvprintw(4, 0, "Fecha: %s", getstatus("date +%F"));
		mvprintw(5, 1, "Hora: %s", getstatus("date +%T"));
		attroff(COLOR_PAIR(3));
		
		//Energia
		attron(COLOR_PAIR(2));
		mvprintw(7, 7, "Batería");
		attroff(COLOR_PAIR(2));
		attron(COLOR_PAIR(3));
		if (strcmp(getstatus("cat /sys/class/power_supply/BAT1/status"), "Charging") == 0) mvprintw(9, 0, "Estado: Cargando.");
		else if (strcmp(getstatus("cat /sys/class/power_supply/BAT1/status"), "Discharging") == 0) mvprintw(9, 0, "Estado: Descargando.");
		else mvprintw(9, 0, "Estado: Desconocido.");
		mvprintw(10, 2, "Porcentaje: %s%s", getstatus("cat /sys/class/power_supply/BAT1/capacity"), "%");
		attroff(COLOR_PAIR(3));
		
		//CPUs
		attron(COLOR_PAIR(2));
		mvprintw(2, 33, "CPUs");
		attroff(COLOR_PAIR(2));
		attron(COLOR_PAIR(3));
		mvprintw(4, 27, "CPU1: %d%s|%.1f°C", cpu1, "%", atof(getstatus("cat /sys/class/hwmon/hwmon3/temp2_input")) / 1000);
		mvprintw(5, 27, "CPU2: %d%s|%.1f°C", cpu2, "%", atof(getstatus("cat /sys/class/hwmon/hwmon3/temp3_input")) / 1000);
		mvprintw(6, 30, "Tareas: %s", getstatus("ps aux | wc -l"));
		attroff(COLOR_PAIR(3));
		
		//Wifi
		attron(COLOR_PAIR(2));
		mvprintw(8, 33, "Wifi");
		attroff(COLOR_PAIR(2));
		attron(COLOR_PAIR(3));
		mvprintw(10, 27, "Subida: %dB/s", wifiAS - wifiIS);
		mvprintw(11, 27, "Bajada: %dB/s", wifiAB - wifiIB);
		mvprintw(12, 27, "IPv4: %.12s", getstatus("ip addr show wlan0 | grep 'inet ' | awk '{print $2}'"));
		attroff(COLOR_PAIR(3));
		
		//RAM
		attron(COLOR_PAIR(2));
		mvprintw(2, 55, "Recursos RAM");
		attroff(COLOR_PAIR(2));
		attron(COLOR_PAIR(3));
		mvprintw(4, 53, "RAM: %.2fG|%.3fG", atof(getstatus("cat /proc/meminfo | grep 'MemTotal:' | awk '{print $2}'")) / 1024 / 1024, 
				(atof(getstatus("cat /proc/meminfo | grep 'MemTotal:' | awk '{print $2}'")) -
				atof(getstatus("cat /proc/meminfo | grep 'MemFree:' | awk '{print $2}'")) - 
				atof(getstatus("cat /proc/meminfo | grep 'Cached:' | awk '{print $2}'")) - 
				atof(getstatus("cat /proc/meminfo | grep 'Buffers:' | awk '{print $2}'"))) / 1024 / 1024);
		mvprintw(5, 53, "Swap: %.2fG|%.3fG", atof(getstatus("cat /proc/meminfo | grep 'SwapTotal:' | awk '{print $2}'")) / 1024 / 1024,
				atof(getstatus("cat /proc/meminfo | grep 'SwapCached:' | awk '{print $2}'")) / 1024 / 1024);
		attroff(COLOR_PAIR(3));
		
		//Espacio
		attron(COLOR_PAIR(2));
		mvprintw(7, 58, "Espacio");
		attroff(COLOR_PAIR(2));
		attron(COLOR_PAIR(3));
		mvprintw(9, 56, "Usado: %dG", atoi(getstatus("df | grep '/dev/sda2' | awk '{print $3}'")) / 1024 / 1024);
		mvprintw(10, 53, "Disponible: %dG", atoi(getstatus("df | grep '/dev/sda2' | awk '{print $4}'")) / 1024 / 1024);
		attroff(COLOR_PAIR(3));
		refresh();
		compare();
	}
	endwin();
}
