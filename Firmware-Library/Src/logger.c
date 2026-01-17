// Includes
#include "logger.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "usart.h"
#include <string.h>

/**
 * @brief Delay between queue checks in milliseconds
 */
#define DELAY pdMS_TO_TICKS(5)

// Global Variables


//Stores the current state of the logger initialization
bool LOGGING_INITIALIZED = false;

QueueHandle_t xLogQueue;
osThreadId_t loggerTaskHandle;

//Holds all the sensor values
float data_sensors[NUM_OF_SENSORS] = {0.0f/0.0f};

//Holds the VCUs message sent over CAN (Used by the ACB to decompile the VCUs message)
char VCU_msg[LOG_MSG_LEN];
int VCU_msgLen = 0;

static int VCU_loggingReady = 0;


/*
 * logInitialize()
 *
 * @brief Initializes the logging system
 *
 * Performs the following initialization steps
 * 1. Creates a queue for log entries
 * 2. Creates and starts the USART logger task
 * 3. Sets the initialization flag.
 *
 * @return true - Logging system successfully initialized
 * @return false - Initialization failed
 *
 */
bool logInitialize() {
	//Ensures it isn't already initialized, and initializes the SD and BT
	if(!LOGGING_INITIALIZED) {
		//On success, set the bool flag and return true

		// Create message queue
		xLogQueue = xQueueCreate(LOG_QUEUE_LENGTH, sizeof(log_message_t));
		if(xLogQueue == NULL){
			HAL_USART_Transmit(&husart2, (uint8_t *) "Logger queue failed to initialize\r\n", strlen("Logger queue failed to initialize\r\n"), 10);
			return false;
		}

		// Create logger task
        osThreadAttr_t loggerTaskAttr = {
            .name = "USARTLogger",
            .stack_size = 512,  // Adjust as needed
            .priority = osPriorityHigh,
        };

        loggerTaskHandle = osThreadNew(vUSARTLoggerTask, NULL, &loggerTaskAttr);
        if (loggerTaskHandle == NULL) {
            vQueueDelete(xLogQueue);
            xLogQueue = NULL;
            return false;
        }

		LOGGING_INITIALIZED = true;

		HAL_USART_Transmit(&husart2, (uint8_t *) "Logger queue initialized.\r\n", strlen("Logger queue initialized.\r\n"), 10);


		return true;
	}


	//Return false on any init failures
	return false;
}

/*
 * logTerminate()
 * @brief Shuts down the logging system
 *
 * Performs the following:
 * 1. Terminate the logger task
 * 2. Empties and deletes the message queue.
 * 3. Sets initialization to false.
 *
 * @return true - all the clean up was successful
 * @return false - one or more of the clean up operations failed
 *
 */
bool logTerminate() {
    // Only proceed if logging is initialized
    if(!LOGGING_INITIALIZED) {
        return false;
    }

    bool success = true;

    // Delete the logger task
    if(loggerTaskHandle != NULL) {
        osThreadTerminate(loggerTaskHandle);
    } else {
        success = false;
    }

    // Delete the queue
    if(xLogQueue != NULL) {
        // First empty the queue if needed
        while(uxQueueMessagesWaiting(xLogQueue) > 0) {
            log_message_t dummy;
            xQueueReceive(xLogQueue, &dummy, 0);
        }
        vQueueDelete(xLogQueue);
        xLogQueue = NULL;
    } else {
        success = false;
    }

    // Reset the initialization flag only if everything succeeded
    if(success) {
        LOGGING_INITIALIZED = false;
        HAL_USART_Transmit(&husart2, (uint8_t *) "Logger terminated successfully.\r\n",strlen("Logger terminated successfully.\r\n"), 10);
    } else {
        HAL_USART_Transmit(&husart2, (uint8_t *) "Logger termination had errors.\r\n",strlen("Logger termination had errors.\r\n"), 10);
    }

    return success;
}
/**
 * Enables the VCU to start the logging process
 */
void enableVCULogging() {
	VCU_loggingReady = 1;
}

void nullTerminate(char *str) {
    size_t length = strlen(str);
    uint8_t isNullTerminated = str[length - 1] != '\0';
    if (isNullTerminated == 0) {
        str[length] = '\0';
    }
}


/*
 * logMessage(char *data, bool critical)
 *
 * Log a diagnostics message, by sending it to the ACB in 8 byte chunks
 *
 * data = Char array (String) that contains the message
 * critical = Boolean flag on if the message is critical, bypassing the log buffer
 */
void logMessage(char *data, bool critical) {
	if (VCU_loggingReady && LOGGING_INITIALIZED) {
		nullTerminate(data);
        HAL_USART_Transmit(&husart2, (uint8_t *)data, strlen(data), 10);
	}
}

/**
 * @brief Formats and enqueues log messages with severity levels
 *
 * @param Log_Level String representing message severity ("ERROR","WARN", etc.)
 * @param format printf-style format string for the message content
 * @param ... Variable arguments for the format string
 */
void vFormattedLog(const char *Log_Level, const char *format, ...) {
    if (LOGGING_INITIALIZED) {
        log_message_t msg;

        // All the additional arguments used in the log will be handled by this block of code.
        va_list args;
        va_start(args, format);
        // Print the log level first
        int prefix_length = snprintf(msg.message,sizeof(msg.message),"[%s] ",Log_Level);
        // Print the desired message into the string
        vsnprintf(msg.message + prefix_length, sizeof(msg.message)- prefix_length, format, args);
        va_end(args);

        // Null terminate
        msg.message[sizeof(msg.message) - 1] = '\0';

        // Send to the queue
        if (xQueueSend(xLogQueue, &msg, DELAY) != pdPASS) {
            HAL_USART_Transmit(&husart2, (uint8_t *)"[WARN] Log queue full! Message Dropped\r\n", strlen("[WARN] Log queue full! Message Dropped\r\n"), 100);
        }
    }
}

/**
 * @brief USART logger task. Transmits queued messages.
 * This Task continuously checks to:
 * 1. Check the queue for a new message.
 * 2. transmits via USART
 *
 * @param pvParameters task parameters
 */
void vUSARTLoggerTask(void *pvParameters) {
    log_message_t msg;
    LOGINFO("USART message system booted!");
    for (;;) {
        if (xQueueReceive(xLogQueue, &msg, DELAY) == pdPASS) {
            // An additional 3 bytes are needed to be allocated here to allow for the buffer to have enough space for \r\n\0
            char buffer[LOG_MSG_LEN+3];
            snprintf(buffer, sizeof(buffer), "%s\r\n", msg.message);
            HAL_USART_Transmit(&husart2, (uint8_t *)buffer, strlen(buffer), 25);
        }else{
        	osThreadYield();
        }
        osDelay(DELAY);
    }
}

