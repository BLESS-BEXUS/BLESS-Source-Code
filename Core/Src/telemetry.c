/*
 * telemetry.c
 *
 *  Created on: 17 mar 2026
 *      Author: Mariio
 */

#include <stdio.h>
#include <string.h>

#include "main.h"
#include "telemetry.h"

extern UART_HandleTypeDef huart2;

int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

#include <stdio.h>

void printTelemetry(const TelemetryPacket *packet) {
    uint8_t gps_packed = (uint8_t)packet->gps_fix_sats;
    uint8_t gps_fix = gps_packed >> 6;
    uint8_t gps_sats = gps_packed & 0x3FU;
    const char *gps_fix_text;

    switch (gps_fix) {
    case 0:
        gps_fix_text = "SIN FIX";
        break;
    case 1:
        gps_fix_text = "FIX GNSS";
        break;
    case 2:
        gps_fix_text = "FIX DIFERENCIAL";
        break;
    default:
        gps_fix_text = "FIX AVANZADO";
        break;
    }

    // Limpiamos un poco la salida visual
    printf("\r\n==================================================\r\n");
    printf("   PAQUETE DE TELEMETRÍA [#%lu] - T: %lu ms\r\n", packet->count, packet->timestamp);
    printf("==================================================\r\n");

    // 1. BLOQUE TÉRMICO Y DE POTENCIA (Tu prioridad ahora mismo)
    printf(">> PERFIL TÉRMICO (TEST -68 C) <<\r\n");
    printf(" - Temp Interna : %6.2f C\r\n", packet->int_temp / 100.0f);
    printf(" - Temp Externa : %6.2f C\r\n", packet->ext_temp / 100.0f);
    printf(" - Temp Chip    : %d C\r\n", packet->temp_chip);
    printf(" - Potencia PID : %6.2f %%\r\n", packet->heater_power / 100.0f);
    printf(" - Corriente HW : %6u mA\r\n", packet->halow_curr); // Asumiendo mA
    printf("--------------------------------------------------\r\n");

    // 2. ENTORNO
    printf(">> ENTORNO <<\r\n");
    // Se asume que la humedad y la presión también necesitan un factor de escala,
    // ajusta el "/ 100.0f" si las envías en formato bruto.
    printf(" - Presion      : %lu Pa\r\n", packet->env_press);
    printf(" - Humedad      : %.2f %%\r\n", packet->ext_hum / 100.0f);
    printf(" - Radiacion    : %u cps\r\n", packet->rad_cps);
    printf("--------------------------------------------------\r\n");

    // 3. RF PERFORMANCE
    printf(">> RADIO (HaLow) <<\r\n");
    printf(" - RSSI / SNR   : %d dBm  /  %d dB\r\n", packet->rf_rssi, packet->rf_snr);
    printf(" - Freq Dev     : %d Hz\r\n", packet->rf_freq_dev);
    printf(" - Exito (Tx)   : %u %%\r\n", packet->rf_success_rate);
    printf(" - Noise Floor  : %d dBm\r\n", packet->rf_noise_floor);
    printf(" - MCS / Volt   : %u  /  %u mV\r\n", packet->rf_tx_mcs, packet->rf_volt);
    printf("--------------------------------------------------\r\n");

    // 4. GNSS
    printf(">> GNSS <<\r\n");
    // Los GPS suelen enviar los grados multiplicados por 10^7
    printf(" - Lat / Lon    : %.6f / %.6f\r\n", packet->gps_lat / 10000000.0f, packet->gps_lon / 10000000.0f);
    printf(" - Altitud      : %u m\r\n", packet->gps_alt);
    printf(" - Calidad Fix  : %u (%s)\r\n", gps_fix, gps_fix_text);
    printf(" - Satelites    : %u\r\n", gps_sats);
    printf(" - Fix/Sats RAW : 0x%02X\r\n", gps_packed);
    if (gps_fix == 0U) {
        printf(" - Estado GNSS  : Buscando solucion; posicion no valida\r\n");
    } else if ((packet->gps_lat == 0) && (packet->gps_lon == 0)) {
        printf(" - Estado GNSS  : AVISO: fix valido pero coordenadas a cero\r\n");
    } else {
        printf(" - Estado GNSS  : Posicion valida\r\n");
    }
    printf("--------------------------------------------------\r\n");

    // 5. ESTADO DEL SISTEMA
    printf(">> SISTEMA <<\r\n");
    printf(" - Sync         : 0x%04X\r\n", packet->sync);
    printf(" - Error Flags  : 0x%02X\r\n", packet->error_flags);
    printf(" - Checksum     : 0x%04X\r\n", packet->checksum);
    printf("==================================================\r\n\r\n");
}
