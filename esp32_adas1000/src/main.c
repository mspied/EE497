#include "Communication.h"
#include "ADAS1000.h"
#include <stdio.h>
#include <rom/ets_sys.h>

#define FRAME_SIZE ((ADAS1000_16KHZ_FRAME_SIZE * ADAS1000_16KHZ_WORD_SIZE)/8) //48 bytes/frame

// Buffers to hold ECG frame data
unsigned char primary_buf[FRAME_SIZE];    // Define FRAME_SIZE as needed
unsigned char secondary_buf[FRAME_SIZE];

void process_ecg_data(unsigned char*, unsigned char*);

void app_main(void)
{
    if(!(ADAS1000_Init(16000)))
        printf("ADAS INIT FAILURE");

    current_target = ADAS_PRIMARY;
    ADAS1000_SetRegisterValue(0x01, 0xF800BE);
    ADAS1000_SetRegisterValue(0x05, 0xE0000A);
    ADAS1000_SetRegisterValue(0x21, 0x000F54);
    ADAS1000_SetRegisterValue(0x22, 0x000F26);
    ADAS1000_SetRegisterValue(0x23, 0x000EFC);
    ADAS1000_SetRegisterValue(0x24, 0x000F5A);
    ADAS1000_SetRegisterValue(0x25, 0x000F3E);

    current_target = ADAS_SECONDARY;
    ADAS1000_SetRegisterValue(0x01, 0xF000DE);
    ADAS1000_SetRegisterValue(0x05, 0x000004);
    ADAS1000_SetRegisterValue(0x21, 0x000F7E);
    ADAS1000_SetRegisterValue(0x22, 0x000EFA);
    ADAS1000_SetRegisterValue(0x23, 0x000F4D);
    ADAS1000_SetRegisterValue(0x24, 0x000F7E);
    ADAS1000_SetRegisterValue(0x25, 0x000F66);



    while (1) {
        // Wait until BOTH DRDY signals go low
        while (!(is_data_ready(ADAS_PRIMARY) /*&& is_data_ready(ADAS_SECONDARY)*/)) {
            // Optional: add short delay to avoid busy waiting
            ets_delay_us(10); // 10 µs delay (ESP-IDF SDK delay)
        }

        // Read both devices back-to-back (order doesn't matter if synced)
        current_target = ADAS_PRIMARY;
        ADAS1000_ReadData(primary_buf, 1, 1, 1, 0, 0);

        current_target = ADAS_SECONDARY;
        ADAS1000_ReadData(secondary_buf, 1, 1, 1, 0, 0);

        // Process the two frames together here
        process_ecg_data(primary_buf, secondary_buf);
    }
}

void process_ecg_data(unsigned char primary[], unsigned char secondary[])
{
    //uint32_t word1 = (uint32_t)(primary[1]<<16) +
    //                 (uint32_t)(primary[2]<<8) +
    //                 (uint32_t)(primary[3]<<0);

    uint32_t L1 = (uint32_t)(primary[5]<<16) +
                     (uint32_t)(primary[6]<<8) +
                     (uint32_t)(primary[7]<<0);

    uint32_t L2 = (uint32_t)(primary[9]<<16) +
                     (uint32_t)(primary[10]<<8) +
                     (uint32_t)(primary[11]<<0);

    uint32_t L3 = (uint32_t)(primary[13]<<16) +
                     (uint32_t)(primary[14]<<8) +
                     (uint32_t)(primary[15]<<0);

    uint32_t V1 = (uint32_t)(primary[17]<<16) +
                     (uint32_t)(primary[18]<<8) +
                     (uint32_t)(primary[19]<<0);

    uint32_t V2 = (uint32_t)(primary[21]<<16) +
                     (uint32_t)(primary[22]<<8) +
                     (uint32_t)(primary[23]<<0);

    uint32_t aVR = (-0.5 * (L1+L2));

    uint32_t aVL = (0.5 * (L1-L3));

    uint32_t aVF = (0.5 * (L2+L3));

    int32_t V3 = (int32_t)(secondary[5]<<16) +
                    (int32_t)(secondary[6]<<8) +
                    (int32_t)(secondary[7]<<0);

    int32_t V4 = (int32_t)(secondary[9]<<16) +
                    (int32_t)(secondary[10]<<8) +
                    (int32_t)(secondary[11]<<0);

    int32_t V5 = (int32_t)(secondary[13]<<16) +
                    (int32_t)(secondary[14]<<8) +
                    (int32_t)(secondary[15]<<0);

    int32_t V6 = (int32_t)(secondary[17]<<16) +
                    (int32_t)(secondary[18]<<8) +
                    (int32_t)(secondary[19]<<0);

    printf("%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%ld,%ld,%ld,%ld", 
            L1, L2, L3, aVR, aVL, aVF, V1, V2, V3, V4, V5, V6);
    
    return;
}