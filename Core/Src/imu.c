//***************************
//
//	@file 		IMU.c
//	@author 	Matthew Mora
//	@created	August 6th, 2023
//	@brief		Processes data transmitted by the IMU via CAN
//
//***************************

#include "imu.h"
#include "cmsis_os.h"

#define BUFFER_SIZE 128

//define buffer struct
typedef struct BUFFER{
    uint64_t queue[BUFFER_SIZE];
    uint8_t buf_start_pos;
    uint8_t buf_end_pos;
    bool	buf_wrap;
}buffer;

// circular buffer for holding received unprocessed packets
buffer acceleration_buffer;
buffer angular_rate_buffer;

// function prototypes
void bufferInit();
uint64_t dequeueAccelerationPacket();
uint64_t dequeueAngularRatePacket();
void processAccelerationPacket(uint64_t packet);
void processAngularRatePacket(uint64_t packet);
void printPacket(char *data_name, float x, float y, float z);

void bufferInit()
{
    // initialize acceleration buffer
    acceleration_buffer.buf_start_pos	= 0;
    acceleration_buffer.buf_end_pos 	= 0;
    acceleration_buffer.buf_wrap		= false;

    //initialize angular rate buffer
    angular_rate_buffer.buf_start_pos	= 0;
    angular_rate_buffer.buf_end_pos		= 0;
    angular_rate_buffer.buf_wrap		= false;
}


int queueAccelerationPacket(uint8_t data[])
{
    // check if space in buffer
    if ((acceleration_buffer.buf_wrap && acceleration_buffer.buf_end_pos < acceleration_buffer.buf_start_pos) ||
        (!acceleration_buffer.buf_wrap && acceleration_buffer.buf_end_pos >= acceleration_buffer.buf_start_pos))
    {
        // add packet **NOTE a packet is 8 bytes so we can just grab the 8 bytes at once (arrays are contiguous in memory)**
        acceleration_buffer.queue[acceleration_buffer.buf_end_pos] = ((uint64_t*)data)[0];

        // increment end position
        acceleration_buffer.buf_end_pos++;

        // check for buffer wrap
        if(acceleration_buffer.buf_end_pos % BUFFER_SIZE < acceleration_buffer.buf_end_pos)
        {
            acceleration_buffer.buf_end_pos = acceleration_buffer.buf_end_pos % BUFFER_SIZE;
            acceleration_buffer.buf_wrap = true;
        }

        return 0;
    }

    return -1;
}

int queueAngularRatePacket(uint8_t data[])
{
    // check if space in buffer
    if ((angular_rate_buffer.buf_wrap && angular_rate_buffer.buf_end_pos < angular_rate_buffer.buf_start_pos) ||
        (!angular_rate_buffer.buf_wrap && angular_rate_buffer.buf_end_pos >= angular_rate_buffer.buf_start_pos))
    {
        // add packet **NOTE a packet is 8 bytes so we can just grab the 8 bytes at once (arrays are contiguous in memory)**
        angular_rate_buffer.queue[angular_rate_buffer.buf_end_pos] = ((uint64_t*)data)[0];

        // increment end position
        angular_rate_buffer.buf_end_pos++;

        // check for buffer wrap
        if(angular_rate_buffer.buf_end_pos % BUFFER_SIZE < angular_rate_buffer.buf_end_pos)
        {
            angular_rate_buffer.buf_end_pos = angular_rate_buffer.buf_end_pos % BUFFER_SIZE;
            angular_rate_buffer.buf_wrap = true;
        }

        return 0;
    }

    return -1;
}

uint64_t dequeueAccelerationPacket()
{
    uint64_t temp;

    // check if buffer empty
    if (!acceleration_buffer.buf_wrap && acceleration_buffer.buf_start_pos == acceleration_buffer.buf_end_pos)
    {
        return 0; // check and make sure to return a number that should not be possible!!!
    }

    // get data value
    temp = acceleration_buffer.queue[acceleration_buffer.buf_start_pos];

    acceleration_buffer.buf_start_pos++;

    // check for buffer wrap
    if (acceleration_buffer.buf_start_pos % BUFFER_SIZE < acceleration_buffer.buf_start_pos)
    {
        acceleration_buffer.buf_start_pos = acceleration_buffer.buf_start_pos % BUFFER_SIZE;
        acceleration_buffer.buf_wrap = false;
    }

    return temp;
}

uint64_t dequeueAngularRatePacket()
{
    uint64_t temp;

    // check if buffer empty
    if (!angular_rate_buffer.buf_wrap && angular_rate_buffer.buf_start_pos == angular_rate_buffer.buf_end_pos)
    {
        return 0; // check and make sure to return a number that should not be possible!!!
    }

    // get data value
    temp = angular_rate_buffer.queue[angular_rate_buffer.buf_start_pos];

    angular_rate_buffer.buf_start_pos++;

    // check for buffer wrap
    if (angular_rate_buffer.buf_start_pos % BUFFER_SIZE < angular_rate_buffer.buf_start_pos)
    {
        angular_rate_buffer.buf_start_pos = angular_rate_buffer.buf_start_pos % BUFFER_SIZE;
        angular_rate_buffer.buf_wrap = false;
    }

    return temp;
}

void processAccelerationPacket(uint64_t packet)
{
    uint16_t *data = NULL;
    float x_acceleration;
    float y_acceleration;
    float z_acceleration;

    data = (uint16_t*) &packet;

    y_acceleration = ((float)data[0] / 100) - 320;
    x_acceleration = ((float)data[1] / 100) - 320;
    z_acceleration = ((float)data[2] / 100) - 320;

    /* TODO SCU#35 */
    /* Logging Starts */
    printPacket("Acceleration", x_acceleration, y_acceleration, z_acceleration);
    /* Logging Ends */
}

void processAngularRatePacket(uint64_t packet)
{
    uint16_t *data = NULL;
    float x_angular_rate;
    float y_angular_rate;
    float z_angular_rate;

    data = (uint16_t*) &packet;

    y_angular_rate = ((float)data[0] / 128) - 250;
    x_angular_rate = ((float)data[1] / 128) - 250;
    z_angular_rate = ((float)data[2] / 128) - 250;

    /* TODO SCU#35 */
    /* Logging Starts */
    printPacket("Angular Rate", x_angular_rate, y_angular_rate, z_angular_rate);
    /* Logging Ends */
}

void printPacket(char *data_name, float x, float y, float z)
{
    char tempMsg[512];

    sprintf(tempMsg, "%s X: %f\r\n%s Y: %f\r\n%s Z: %f\r\n", data_name, x, data_name, y, data_name, z);

    HAL_USART_Transmit(&husart2, (uint8_t *) tempMsg, strlen(tempMsg), 20);
}

void StartImuCanProcTask(void *argument)
{
    uint64_t packet;
    for (;;)
    {
        // should probably check to make sure queues are in valid state
        packet = dequeueAccelerationPacket();
        // need to check if packet valid
        processAccelerationPacket(packet);

        packet = dequeueAngularRatePacket();
        // need to check if packet valid
        processAngularRatePacket(packet);

        osThreadYield();
    }
}
