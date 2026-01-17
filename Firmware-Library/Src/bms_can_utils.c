//
// Created by caleb on 2024-05-07.
//

#include "bms_can_utils.h"

#include <sys/types.h>

#include "FreeRTOS.h"
#include "logger.h"
#include "semphr.h"

/** Interface for the 2022 EMUS G1 BMS.
 * 	Contains information on voltages, temperature,
 * 	statuses, and diagnostic messages
 *
 * 	Documentation: https://emusbms.com/wp-content/uploads/2018/12/EMUS-G1-BMS-CAN-Protocol-v2.0.12.pdf
 *
 * 	Author: Brett Stevens
 */

/**
 * What do we need?
 * 	-Specific diagnostic codes?
 * 	-Individual cell temps / module temps / voltages
 * 		-If so, how many "groups" do we have
 * 	-balancing rate (individual / average)?
 * 	-State of change parameters?
 * 	-Statistics (Data collection)?
 *
 * 	New stuff:
 * 	-Check if voltage values are outside range on new packet (range?)
 * 	-Sense overcurrent blown or tripped (?)
 * 	-Check if temperature values are outside range on new packet (range?)
 * 	-Check for missing or interrupted V or T measurements
 * 	-Check for BMS faults
 *
 * 	-On Fault:
 * 		-Open shutdown circuit
 * 		-Turn on AMS indicator light.
 */

// Overall status readings
uint8_t BMS_INPUT_SIGNALS = 0;
uint8_t BMS_OUTPUT_SIGNALS = 0;
uint16_t BMS_NUM_LIVE_CELLS = 0;
uint8_t BMS_CHARGING_STAGE = 0;
uint16_t BMS_CHARGING_STAGE_TIME = 0;
uint8_t BMS_LAST_CHARGING_ERROR = 0;

// Diagnostic codes
uint8_t PROTECTION_FLAGS = 0;
uint8_t WARNING_FLAGS = 0;
uint8_t BATTERY_STATUS_FLAGS = 0;

// Battery Voltage Overall Parameters readings
uint8_t	BMS_HIGH_VOLTAGE = 0;
uint8_t	BMS_LOW_VOLTAGE = 0;
uint8_t	BMS_AVERAGE_VOLTAGE = 0;

// Cell Module Temperature Overall Parameters readings
uint8_t BMS_MIN_CELL_MODULE_TEMP = 0;
uint8_t BMS_MAX_CELL_MODULE_TEMP = 0;
uint8_t BMS_AVERAGE_CELL_MODULE_TEMP = 0;

// Cell Temperature Overall Parameters readings
uint8_t	BMS_HIGH_TEMP = 0;
uint8_t	BMS_LOW_TEMP = 0;
uint8_t	BMS_AVERAGE_TEMP = 0;

// Cell Balancing Rate Overall Parameters readings
uint8_t BMS_MIN_CELL_BALANCING_RATE = 0;
uint8_t BMS_MAX_CELL_BALANCING_RATE = 0;
uint8_t BMS_AVERAGE_CELL_BALANCING_RATE = 0;

// State of Charge Parameters readings
uint16_t BMS_CURRENT = 0;
uint16_t BMS_ESTIMATED_CHARGE = 0;
uint8_t BMS_ESTIMATED_STATE_OF_CHARGE = 0;

// Contactor Control readings
uint8_t BMS_CONTACTOR_STATE = 0;

// Energy Parameter readings
uint16_t BMS_ESTIMATED_CONSUMPTION = 0;
uint16_t BMS_ESTIMATED_ENERGY = 0;
uint16_t BMS_ESTIMATED_DISTANCE_LEFT = 0;
uint16_t BMS_DISTANCE_TRAVELED = 0;

// Statistics readings
uint8_t BMS_STAT_ID = 0;
uint8_t BMS_STAT_DATA_TYPE = 0;
// need more here for collecting the data with the 3 statistic data types

// Event readings
uint8_t BMS_EVENT_ENTRY_NUMBER = 0;
uint8_t BMS_EVENT_DATA_TYPE = 0;
uint8_t	BMS_EVENT_ID = 0;
uint32_t BMS_EVENT_TIMESTAMP_DATA = 0;

// Individual Cell Voltage readings
uint8_t cellVoltages[CELLS_PER_CAN_FRAME];

// Individual Cell Module Temp readings
uint8_t cellModuleTemps[CELLS_PER_CAN_FRAME];

// Individual Cell Temp readings
uint8_t cellTemps[CELLS_PER_CAN_FRAME];

// Individual Cell Balancing Rate readings
uint8_t cellBalancingRate[CELLS_PER_CAN_FRAME];

// Maximum number of cell groups (total num. of cells / 8)
uint8_t MAX_NUM_CELL_GROUPS = (NUM_EMUS_CELLS / CELLS_PER_CAN_FRAME) + 1;
//uint8_t MAX_NUM_CELL_GROUPS = 34;

SemaphoreHandle_t xBatTempMutex = NULL;			// Temperature mutex
SemaphoreHandle_t xBatVoltMutex = NULL;			// Voltage


// Inits the data structures for the BMS
void init_bms(void){

	xBatTempMutex = xSemaphoreCreateMutex();		// Create a mutex to grab the read temperature data
	xBatVoltMutex = xSemaphoreCreateMutex();		// Create a mutex to grab the read temperature data

}

_Bool isBmsCanId(uint32_t canID)
{
	return ((canID == CAN_BMS_OVERALL_ID) || (canID == CAN_BMS_DIAGNOSTIC_ID) || (canID == CAN_BMS_VOLTAGE_ID)
	|| (canID == CAN_BMS_MODULE_TEMPERATURE) || (canID == CAN_BMS_CELL_TEMPERATURE) || (canID == CAN_BMS_CELL_BALANCING_RATE)
	|| (canID == CAN_BMS_STATE_OF_CHARGE) || (canID == CAN_BMS_CONTACTOR_CONTROL) || (canID == CAN_BMS_ENERGY_PARAM)
	|| (canID == CAN_BMS_STATS) || (canID == CAN_BMS_EVENTS)
	|| ((CAN_BMS_BASE_ID + 32) <= canID) && (canID <= (CAN_BMS_BASE_ID + 32 + MAX_NUM_CELL_GROUPS))
	|| (canID == CAN_BMS_INDIVIDUAL_CELL_VOLTAGES_TYPE_B)
	|| ((CAN_BMS_BASE_ID + 64) <= canID) && (canID <= (CAN_BMS_BASE_ID + 64 + MAX_NUM_CELL_GROUPS))
	|| (canID == CAN_BMS_INDIVIDUAL_CELL_MODULE_TEMPS_TYPE_B)
	|| ((CAN_BMS_BASE_ID + 256) <= canID) && (canID <= (CAN_BMS_BASE_ID + 256 + MAX_NUM_CELL_GROUPS))
	|| (canID == CAN_BMS_INDIVIDUAL_CELL_TEMPS_TYPE_B)
	|| ((CAN_BMS_BASE_ID + 96) <= canID) && (canID <= (CAN_BMS_BASE_ID + 96 + MAX_NUM_CELL_GROUPS))
	|| (canID == CAN_BMS_INDIVIDUAL_CELL_BALANCING_RATE_TYPE_B));
}

// This function processes Type A and B CAN frames
void process_typeA_and_typeB_can_packets(CAN_RxPacketTypeDef * packetToProcess) {
	uint32_t canID = packetToProcess->rxPacketHeader.StdId; // Standard ID from the CAN frame
	uint8_t* canData = packetToProcess->rxPacketData; // 8 byte array from the CAN frame
	uint8_t dataLength = packetToProcess->rxPacketHeader.DLC; // if the data from the can frame isn't full, use this to check num of cells
	uint8_t cellGroupNum; // will be initialized to a group value if the CAN ID is in a valid range to have a group number

	// Individual Cell Voltages - Type A
	// range from 48 (CAN_BMS_BASE_ID + 32) to the max number of cell groups (CAN_BMS_BASE_ID + 32 + MAX_NUM_CELL_GROUPS) (we have 34 cell groups for ePBR24)
	if (((CAN_BMS_BASE_ID + 32) <= canID) && (canID <= (CAN_BMS_BASE_ID + 32 + MAX_NUM_CELL_GROUPS))) {
		cellGroupNum = canID - CAN_BMS_BASE_ID - 32;

		clear_8_byte_array(cellVoltages);
		fill_8_byte_array_typeA(cellVoltages, canData, dataLength);

	}

	// Individual Cell Voltages - Type B
	if (canID == CAN_BMS_INDIVIDUAL_CELL_VOLTAGES_TYPE_B) {
		cellGroupNum = canData[0]; // the group number is the first byte of the CAN frame data
		// dataLength ranges from 2-8 (because data0, or dataLength of 1, contains the group number)

		clear_8_byte_array(cellVoltages);
		fill_8_byte_array_typeB(cellVoltages, canData, dataLength);
	}

	// Individual Cell Module Temps - Type A
	if (((CAN_BMS_BASE_ID + 64) <= canID) && (canID <= (CAN_BMS_BASE_ID + 64 + MAX_NUM_CELL_GROUPS))) {
		cellGroupNum = canID - CAN_BMS_BASE_ID - 64;

		clear_8_byte_array(cellModuleTemps);
		fill_8_byte_array_typeA(cellModuleTemps, canData, dataLength);
	}
	// Individual Cell Module Temps - Type B
	if (canID == CAN_BMS_INDIVIDUAL_CELL_MODULE_TEMPS_TYPE_B) {
		cellGroupNum = canData[0];

		clear_8_byte_array(cellModuleTemps);
		fill_8_byte_array_typeB(cellModuleTemps, canData, dataLength);

	}
	// Individual Cell Temps - Type A
	if (((CAN_BMS_BASE_ID + 256) <= canID) && (canID <= (CAN_BMS_BASE_ID + 256 + MAX_NUM_CELL_GROUPS))) {
		cellGroupNum = canID - CAN_BMS_BASE_ID - 256;

		clear_8_byte_array(cellTemps);
		fill_8_byte_array_typeA(cellTemps, canData, dataLength);
	}
	// Individual Cell Temps - Type B
	if (canID == CAN_BMS_INDIVIDUAL_CELL_TEMPS_TYPE_B) {
		cellGroupNum = canData[0];

		clear_8_byte_array(cellTemps);
		fill_8_byte_array_typeB(cellTemps, canData, dataLength);
	}
	// Individual Cell Balancing Rate - Type A
	if (((CAN_BMS_BASE_ID + 96) <= canID) && (canID <= (CAN_BMS_BASE_ID + 96 + MAX_NUM_CELL_GROUPS))) {
		cellGroupNum = canID - CAN_BMS_BASE_ID - 96;

		clear_8_byte_array(cellBalancingRate);
		fill_8_byte_array_typeA(cellBalancingRate, canData, dataLength);
	}
	// Individual Cell Balancing Rate - Type B
	if (canID == CAN_BMS_INDIVIDUAL_CELL_BALANCING_RATE_TYPE_B) {
		cellGroupNum = canData[0];

		clear_8_byte_array(cellBalancingRate);
		fill_8_byte_array_typeB(cellBalancingRate, canData, dataLength);
	}
}

/**
 * PURPOSE:
 * @param Data The data from the CAN message
 */
void process_bms_overall_packet(uint8_t * Data) {
	BMS_INPUT_SIGNALS = Data[0];
	BMS_OUTPUT_SIGNALS = Data[1];
	BMS_CHARGING_STAGE = Data[3];
	BMS_LAST_CHARGING_ERROR = Data[6];

	//Adding MSByte and LSByte to NUM_LIVE_CELLS
	BMS_NUM_LIVE_CELLS = ((Data[2]) << 8) | Data[7];

	//Adding MSByte and LSByte to BMS_CHARGING_STAGE_TIME
	BMS_CHARGING_STAGE_TIME = ((Data[4]) << 8) | Data[5];

	//Charging error check: 0 means no error
	if(Data[6] != 0){
		//Handle error
		bms_handleChargingError(Data[6]);
	}
}// process_bms_overall_packet

void process_bms_diagnostic_packet(uint8_t * Data) {

	if(xBatTempMutex != NULL){
		// throw error that we need to init the BMS data structures first
		if(xSemaphoreTake( xBatTempMutex, ( TickType_t ) 10 ) == pdTRUE ){

			PROTECTION_FLAGS = Data[0] | (Data[2] << 8);
			WARNING_FLAGS = Data[1];
			BATTERY_STATUS_FLAGS = Data[3];

			if(PROTECTION_FLAGS != 0){
				bms_handleProtectionFlags(PROTECTION_FLAGS);
			}

            xSemaphoreGive( xBatTempMutex );		// Give back mutex

		}
	}

}


void process_bms_voltage_packet(uint8_t * Data) {

	if(xBatVoltMutex != NULL){
		// throw error that we need to init the BMS data structures first
		if(xSemaphoreTake( xBatVoltMutex, ( TickType_t ) 10 ) == pdTRUE ){


			BMS_LOW_VOLTAGE = Data[0];
			BMS_HIGH_VOLTAGE = Data[1];
			BMS_AVERAGE_VOLTAGE = Data[2];

			// need to add total voltage bytes (Data3 -> Data6)

			xSemaphoreGive( xBatVoltMutex );		// Give back mutex
		}//
	}//

}

void process_bms_temp_packet(uint8_t * Data) {

	if(xBatTempMutex != NULL){
		// throw error that we need to init the BMS data structures first
		if(xSemaphoreTake( xBatTempMutex, ( TickType_t ) 10 ) == pdTRUE ){

			BMS_LOW_TEMP = Data[0];
			BMS_HIGH_TEMP = Data[1];
			BMS_AVERAGE_TEMP = Data[2];
			xSemaphoreGive( xBatVoltMutex );		// Give back mutex

		}
	}
	else{
		//error
	}
}

void process_bms_module_temp_can(uint8_t * Data){
	//TODO: BMS_FUNCTION module temp
	BMS_MIN_CELL_MODULE_TEMP = Data[0];
	BMS_MAX_CELL_MODULE_TEMP = Data[1];
	BMS_AVERAGE_CELL_MODULE_TEMP = Data[2];
}

void process_bms_cell_temp_balancing_rate_can(uint8_t * Data) {
	//TODO: BMS_FUNCTION cell temp balancing rate
	BMS_MIN_CELL_BALANCING_RATE = Data[0];
	BMS_MAX_CELL_BALANCING_RATE = Data[1];
	BMS_AVERAGE_CELL_BALANCING_RATE = Data[2];
}

// type A and B functions are in one big function

void process_bms_state_of_charge_can(uint8_t * Data) {
	//TODO: BMS_FUNCTION state of charge
	BMS_CURRENT = (Data[0] << 8) | Data[1];
	BMS_ESTIMATED_CHARGE = (Data[2] << 8) | Data[3];
	BMS_ESTIMATED_STATE_OF_CHARGE = Data[6];
}

/**
 * BMS_CONTACTOR_STATE - Has possible values of 0x00 and 0x01:
 *	- 0x00: CONTACTOR OPEN
 *	- 0x01: CONTACTOR CLOSED
 *
 */
void process_bms_contactor_control_can(uint8_t * Data) {
	//TODO: BMS_FUNCTION contactor control
	BMS_CONTACTOR_STATE = Data[0];
}

void process_bms_energy_param_can(uint8_t * Data) {
	//TODO: BMS_FUNCTION energy parameter
	BMS_ESTIMATED_CONSUMPTION = (Data[0] << 8) | Data[1];
	BMS_ESTIMATED_ENERGY = (Data[2] << 8) | Data[3];
	BMS_ESTIMATED_DISTANCE_LEFT = (Data[4] << 8) | Data[5];
	BMS_DISTANCE_TRAVELED = (Data[6] << 8) | Data[7];
}

void process_bms_stats_can(uint8_t * Data) {
	//TODO: BMS_FUNCTION statistics
	BMS_STAT_ID = Data[0];
	BMS_STAT_DATA_TYPE = Data[1];

	switch(BMS_STAT_ID)
	{
	//TODO: add functions to handle the 3 diff types of statistic data
	case TOTAL_DISCHARGE:
		//add case for 3 diff types of data here
		break;
	case TOTAL_CHARGE:
		//add case for 3 diff types of data here
		break;
	case TOTAL_DISCHARGE_ENERGY:
		//add case for 3 diff types of data here
		break;
	case TOTAL_CHARGE_ENERGY:
		//add case for 3 diff types of data here
		break;
	case TOTAL_DISCHARGE_TIME:
		//add case for 3 diff types of data here
		break;
	case TOTAL_CHARGE_TIME:
		//add case for 3 diff types of data here
		break;
	case TOTAL_DISTANCE:
		//add case for 3 diff types of data here
		break;
	case MASTER_CLEAR_COUNT:
		//add case for 3 diff types of data here
		break;
	case MAX_DISCHARGE_CURRENT:
		//add case for 3 diff types of data here
		break;
	case MAX_CHARGE_CURRENT:
		//add case for 3 diff types of data here
		break;
	case MIN_CELL_VOLTAGE:
		//add case for 3 diff types of data here
		break;
	case MAX_CELL_VOLTAGE:
		//add case for 3 diff types of data here
		break;
	case MAX_CELL_VOLTAGE_DIFFERENCE:
		//add case for 3 diff types of data here
		break;
	case MIN_PACK_VOLTAGE:
		//add case for 3 diff types of data here
		break;
	case MAX_PACK_VOLTAGE:
		//add case for 3 diff types of data here
		break;
	case MIN_CELL_MODULE_TEMPERATURE:
		//add case for 3 diff types of data here
		break;
	case MAX_CELL_MODULE_TEMPERATURE:
		//add case for 3 diff types of data here
		break;
	case MAX_CELL_MODULE_TEMPERATURE_DIFFERENCE:
		//add case for 3 diff types of data here
		break;
	case BMS_STARTS_COUNT:
		//add case for 3 diff types of data here
		break;
	case UNDER_VOLTAGE_PROTECTION_COUNT:
		//add case for 3 diff types of data here
		break;
	case OVER_VOLTAGE_PROTECTION_COUNT:
		//add case for 3 diff types of data here
		break;
	case DISCHARGE_OVER_CURRENT_PROTECTION_COUNT:
		//add case for 3 diff types of data here
		break;
	case CHARGE_OVER_CURRENT_PROTECTION_COUNT:
		//add case for 3 diff types of data here
		break;
	case CELL_MODULE_OVERHEAT_PROTECTION_COUNT:
		//add case for 3 diff types of data here
		break;
	case LEAKAGE_PROTECTION_COUNT:
		//add case for 3 diff types of data here
		break;
	case NO_CELL_COMMUNICATION_PROTECTION_COUNT:
		//add case for 3 diff types of data here
		break;
	case LOW_VOLTAGE_POWER_REDUCTION_COUNT:
		//add case for 3 diff types of data here
		break;
	case HIGH_CURRENT_POWER_REDUCTION_COUNT:
		//add case for 3 diff types of data here
		break;
	case HIGH_CELL_MODULE_TEMPERATURE_POWER_REDUCTION_COUNT:
		//add case for 3 diff types of data here
		break;
	case CHARGER_CONNECT_COUNT:
		//add case for 3 diff types of data here
		break;
	case CHARGER_DISCONNECT_COUNT:
		//add case for 3 diff types of data here
		break;
	case PREHEAT_STAGE_COUNT:
		//add case for 3 diff types of data here
		break;
	case PRECHARGE_STAGE_COUNT:
		//add case for 3 diff types of data here
		break;
	case MAIN_CHARGE_STAGE_COUNT:
		//add case for 3 diff types of data here
		break;
	case BALANCING_STAGE_COUNT:
		//add case for 3 diff types of data here
		break;
	case CHARGING_FINISHED_COUNT:
		//add case for 3 diff types of data here
		break;
	case CHARGING_ERROR_COUNT:
		//add case for 3 diff types of data here
		break;
	case CHARGING_RETRY_COUNT:
		//add case for 3 diff types of data here
		break;
	case TRIPS_COUNT:
		//add case for 3 diff types of data here
		break;
	case CHARGE_RESTARTS_COUNT:
		//add case for 3 diff types of data here
		break;
	case CELL_OVERHEAT_PROTECTION_COUNT:
		//add case for 3 diff types of data here
		break;
	case HIGH_CELL_TEMPERATURE_POWER_REDUCTION_COUNT:
		//add case for 3 diff types of data here
		break;
	case MIN_CELL_TEMPERATURE:
		//add case for 3 diff types of data here
		break;
	case MAX_CELL_TEMPERATURE:
		//add case for 3 diff types of data here
		break;
	case MAX_CELL_TEMPERATURE_DIFFERENCE:
		//add case for 3 diff types of data here
		break;
	default:
		break;
	}
}

void process_bms_events_can(uint8_t * Data) {
	//TODO: BMS_FUNCTION events
	BMS_EVENT_ENTRY_NUMBER = Data[0];
	BMS_EVENT_DATA_TYPE = Data[1];

	if (BMS_EVENT_DATA_TYPE == EVENT_INFO)
	{
		BMS_EVENT_ID = Data[2]; // If Data[2] is zero then No Event
		switch(BMS_EVENT_ID)
		{
		case NO_EVENT:
			// No event, so no action needed
			break;
		case BMS_STARTED:
			logMessage("The BMS has started.", true);
			break;
		case LOST_COMM_TO_CELLS:
			logMessage("Lost communication to the cells.", true);
			break;
		case ESTABLISH_COMM_TO_CELLS:
			logMessage("", true);
			break;
		case CELL_VOLT_CRIT_LOW:
			logMessage("Cell voltage is CRITICALLY LOW.", true);
			break;
		case CRIT_LOW_VOLTAGE_RECOVERED:
			logMessage("Critically low voltage recovered.", true);
			break;
		case CELL_VOLT_CRIT_HIGH:
			logMessage("Cell voltage is CRITICALLY HIGH.", true);
			break;
		case CRIT_HIGH_VOLTAGE_RECOVERED:
			logMessage("Critically high voltage recovered.", true);
			break;
		case DISCHARGE_CURR_CRIT_HIGH:
			logMessage("Discharge current is CRITICALLY HIGH.", true);
			break;
		case DISCHARGE_CRIT_HIGH_CURR_RECOVERED:
			logMessage("Critically high discharge current recovered.", true);
			break;
		case CHARGE_CURR_CRIT_HIGH:
			logMessage("Charge current CRITICALLY HIGH.", true);
			break;
		case CHARGE_CRIT_HIGH_CURR_RECOVERED:
			logMessage("Critically high charge current recovered.", true);
			break;
		case CELL_MODULE_TEMP_CRIT_HIGH:
			logMessage("Cell module temperature CRITICALLY HIGH.", true);
			break;
		case CRIT_HIGH_CELL_MODULE_TEMP_RECOVERED:
			logMessage("Critically high cell module temperature recovered.", true);
			break;
		case LEAKAGE_DETECTED:
			logMessage("Leakage detected.", true);
			break;
		case LEAKAGE_RECOVERED:
			logMessage("Leakage recovered.", true);
			break;
		case LOW_VOLT_REDUCING_POWER_WARNING:
			logMessage("Low voltage reducing power WARNING.", true);
			break;
		case POWER_REDUCTION_FROM_LOW_VOLT_RECOVERED:
			logMessage("Power reduction from low voltage recovered.", true);
			break;
		case HIGH_CURR_REDUCING_POWER_WARNING:
			logMessage("High current reducing power WARNING.", true);
			break;
		case POWER_REDUCTION_FROM_HIGH_CURR_RECOVERED:
			logMessage("Power reduction from high current recovered.", true);
			break;
		case HIGH_CELL_MODULE_TEMP_REDUCING_POWER_WARNING:
			logMessage("High cell module temperature reducting power WARNING.", true);
			break;
		case POWER_REDUCTION_FROM_HIGH_CELL_MODULE_TEMP_RECOVERED:
			logMessage("Power reduction from high cell module temperature recovered.", true);
			break;
		case CHARGER_CONNECTED:
			logMessage("Charger connected.", true);
			break;
		case CHARGER_DISCONNECTED:
			logMessage("Charger disconnected.", true);
			break;
		case STARTED_PREHEAT_STAGE:
			logMessage("Started the preheat stage.", true);
			break;
		case STARTED_PRECHARGE_STAGE:
			logMessage("Started teh precharge stage.", true);
			break;
		case STARTED_MAIN_CHARGING_STAGE:
			logMessage("Started main charging stage.", true);
			break;
		case STARTED_BALANCING_STAGE:
			logMessage("Started balancing stage.", true);
			break;
		case CHARGING_FINISHED:
			logMessage("Charging finished.", true);
			break;
		case CHARGING_ERROR_OCCURRED:
			logMessage("Charging error occurred", true);
			break;
		case RETRYING_CHARGING:
			logMessage("Retrying charging.", true);
			break;
		case RESTARTING_CHARGING:
			logMessage("Restarting charging.", true);
			break;
		case CELL_TEMP_CRIT_HIGH:
			logMessage("Cell temperature CRITICALLY HIGH.", true);
			break;
		case CRIT_HIGH_CELL_TEMP_RECOVERED:
			logMessage("Critically high cell temperature recovered.", true);
			break;
		case HIGH_CELL_TEMP_REDUCING_POWER_WARNING:
			logMessage("High cell temperature reducing power WARNING.", true);
			break;
		case POWER_REDUCTION_FROM_HIGH_CELL_TEMP_RECOVERED:
			logMessage("Power reduction from high cell temperature recovered.", true);
			break;
		default:
			logMessage("Default case reached.", true);
			break;
		}
	}
	else // Else we have an event timestamp
	{
		// check if bitshifting is correct
		BMS_EVENT_TIMESTAMP_DATA = (Data[2] << 24) | (Data[3] << 16) | (Data[4] << 8) | Data[5];
	}


}

/*
 * Interface to log the data message into the sd card
 *
 * @Brief: The following functions is just the interface used in file sensorLogging.c
 *
 */

//Temperature logging
int bms_getHighTemp() {
	int tempInC = -273;

	if(xBatTempMutex != NULL){
		// throw error that we need to init the BMS data structures first
		if(xSemaphoreTake( xBatTempMutex, ( TickType_t ) 10 ) == pdTRUE ){
			tempInC = -100 + BMS_HIGH_TEMP;

			xSemaphoreGive( xBatTempMutex );		// Give back mutex

		}
	}

	return tempInC;

}

int bms_getLowTemp() {
	int tempInC = -273;

	if(xBatTempMutex != NULL){
		// throw error that we need to init the BMS data structures first
		if(xSemaphoreTake( xBatTempMutex, ( TickType_t ) 10 ) == pdTRUE ){
			tempInC = -100 + BMS_LOW_TEMP;
			xSemaphoreGive( xBatTempMutex );		// Give back mutex
		}
	}
	return tempInC;
}

int bms_getAverageTemp() {
	int tempInC = -273;

	if(xBatTempMutex != NULL){
		// throw error that we need to init the BMS data structures first
		if(xSemaphoreTake( xBatTempMutex, ( TickType_t ) 10 ) == pdTRUE ){
			tempInC = -100 + BMS_AVERAGE_TEMP;		// calc
			xSemaphoreGive( xBatTempMutex );		// Give back mutex
		}
	}
	return tempInC;
}


//Voltage logging
float bms_getHighVoltage() {
	float v = 2 + BMS_HIGH_VOLTAGE * 0.01;
	return v;
}

float bms_getLowVoltage() {
	float v = 2 + BMS_LOW_VOLTAGE * 0.01;
	return v;
}

float bms_getAverageVoltage() {
	float v = 2 + BMS_AVERAGE_VOLTAGE * 0.01;
	return v;
}

/**
 * Error handling functions
 */

void bms_handleChargingError(uint8_t cErrorByte){
	switch(cErrorByte){
	case 1:
		logMessage("Cell communication lost at start of charging", true);
		break;
	case 2:
		logMessage("No cell communication (non-can charging)", true);
		break;
	case 3:
		logMessage("Charging stage timeout", true);
		break;
	case 4:
		logMessage("No cell communication (non-can charging)", true);
		break;
	case 5:
		logMessage("Cannot set cell balancing threshold", true);
		break;
	case 6:
		logMessage("Cell or cell module temperature too high", true);
		break;
	case 7:
		logMessage("Cell communication lost during pre-heating stage", true);
		break;
	case 8:
		logMessage("Number of cells mismatch", true);
		break;
	case 9:
		logMessage("Cell over-voltage", true);
		break;
	case 10:
		logMessage("Cell protection event occurred, check diagnostic codes", true);
		//Need to check diagnostic codes or handled already?
		break;
	default:
		break;
	}
}

void bms_handleProtectionFlags(uint16_t Flags){
	if (bms_checkBit16(Flags, 0)){
		logMessage("Under-voltage – some cell is below critical minimum voltage.", true);
	}
	if (bms_checkBit16(Flags, 1)){
		logMessage("Over-voltage – some cell is above critical maximum voltage.", true);
	}
	if (bms_checkBit16(Flags, 2)){
		logMessage("Discharge Over-current – discharge current (negative current) exceeds the critical discharge current setting.", true);
	}
	if (bms_checkBit16(Flags, 3)){
		logMessage("Charge Over-current – charge current (positive current) exceeds the critical charge current setting.", true);
	}
	if (bms_checkBit16(Flags, 4)){
		logMessage("Cell Module Overheat – cell module temperature exceeds maximum critical temperature setting.", true);
	}
	if (bms_checkBit16(Flags, 5)){
		logMessage("Leakage – leakage signal was detected on leakage input pin.", true);
	}
	if (bms_checkBit16(Flags, 6)){
		logMessage("No Cell Communication – loss of communication to cells.", true);
	}
	if (bms_checkBit16(Flags, 11)){
		logMessage("Cell Overheat – cell temperature exceeds maximum cell temperature threshold", true);
	}
	if (bms_checkBit16(Flags, 12)){
		logMessage("No Current Sensor", true);
	}
	if (bms_checkBit16(Flags, 13)){
		logMessage("Pack Under-Voltage", true);
	}
}

int bms_checkBit16(uint16_t bytes, int n){
	if(n >= 16){
		return 0;
	}
	else{
		return (bytes & (1<<n)) != 0;
	}
}

// new functions

// Overwrite previous can message data with zeros to prevent the partially filled cell group at the end
void clear_8_byte_array(uint8_t * array) {
	for (int i = 0; i < CELLS_PER_CAN_FRAME; i++) {
		array[i] = 0;
	}
}

// if the dataLength is 0: NOTICE! If cell communication is lost and request cannot be processed for more than 5 seconds, Control Unit will send an empty
// response message (same identifier as normal response message, but data length zero bytes), and unregister the request.
// Fills an array with the data from the Type-A CAN message
void fill_8_byte_array_typeA(uint8_t * array, uint8_t * arrayOfCANData, uint8_t dataLength) {
	if (dataLength != 0) {
		for (int i = 0; i < dataLength; i++) {
			array[i] = arrayOfCANData[i];
		}
	}
	else { // If dataLength is 0, the cell communication was lost (look at above comment)
		// TODO: Write error message function to handle when cell communication is lost
	}
}

// if the dataLength is 0: NOTICE! If cell communication is lost and request cannot be processed for more than 5 seconds, Control Unit will send an empty
// response message (same identifier as normal response message, but data length zero bytes), and unregister the request.
// Fills an array with the data from the Type-B CAN message
void fill_8_byte_array_typeB(uint8_t * array, uint8_t * arrayOfCANData, uint8_t dataLength) {
    if (dataLength >= 2) { // data length will range from 2-8 bytes of data
        int canDataIndex = 1;
        for (int i = 0; i < dataLength - 1; i++) {
            array[i] = arrayOfCANData[canDataIndex];
            canDataIndex++;
        }
    }
    else { // If dataLength is 0, the cell communication was lost (look at above comment)
        // TODO: Write error message function to handle when cell communication is lost
    }
}

// Requests (using standard IDs)
void bms_request_overall_parameters()
{
	uint8_t emptyDataArray[] = {0, 0, 0, 0 ,0, 0, 0, 0};
	sendCan(&hcan1, emptyDataArray, 0, CAN_BMS_OVERALL_ID, 0, 0);
}

void bms_request_diagnostic_codes()
{
	uint8_t emptyDataArray[] = {0, 0, 0, 0 ,0, 0, 0, 0};
	sendCan(&hcan1, emptyDataArray, 0, CAN_BMS_DIAGNOSTIC_ID, 0, 0);
}

void bms_request_overall_battery_voltages()
{
	uint8_t emptyDataArray[] = {0, 0, 0, 0 ,0, 0, 0, 0};
	sendCan(&hcan1, emptyDataArray, 0, CAN_BMS_VOLTAGE_ID, 0, 0);
}

void bms_request_overall_cell_module_temp()
{
	uint8_t emptyDataArray[] = {0, 0, 0, 0 ,0, 0, 0, 0};
	sendCan(&hcan1, emptyDataArray, 0, CAN_BMS_MODULE_TEMPERATURE, 0, 0);
}

void bms_request_overall_cell_temp()
{
	uint8_t emptyDataArray[] = {0, 0, 0, 0 ,0, 0, 0, 0};
	sendCan(&hcan1, emptyDataArray, 0, CAN_BMS_CELL_TEMPERATURE, 0, 0);
}

void bms_request_overall_cell_balancing_rate()
{
	uint8_t emptyDataArray[] = {0, 0, 0, 0 ,0, 0, 0, 0};
	sendCan(&hcan1, emptyDataArray, 0, CAN_BMS_CELL_BALANCING_RATE, 0, 0);
}

void bms_request_individual_cell_voltages_type_A(uint8_t groupNumber, uint8_t cellStringNumber)
{
	uint8_t requestDataArray[] = {cellStringNumber};
	sendCan(&hcan1, requestDataArray, 1, CAN_BMS_BASE_ID + 32 + groupNumber, 0, 0);
}

void bms_request_individual_cell_voltages_type_B(uint8_t groupNumber, uint8_t cellStringNumber)
{
	uint8_t requestDataArray[] = {groupNumber, cellStringNumber};
	sendCan(&hcan1, requestDataArray, 2, CAN_BMS_INDIVIDUAL_CELL_VOLTAGES_TYPE_B, 0, 0);
}

void bms_request_individual_cell_module_temps_type_A(uint8_t groupNumber, uint8_t cellStringNumber)
{
	uint8_t requestDataArray[] = {cellStringNumber};
	sendCan(&hcan1, requestDataArray, 1, CAN_BMS_BASE_ID + 64 + groupNumber, 0, 0);
}

void bms_request_individual_cell_module_temps_type_B(uint8_t groupNumber, uint8_t cellStringNumber)
{
	uint8_t requestDataArray[] = {groupNumber, cellStringNumber};
	sendCan(&hcan1, requestDataArray, 2, CAN_BMS_INDIVIDUAL_CELL_MODULE_TEMPS_TYPE_B, 0, 0);
}

void bms_request_individual_cell_temps_type_A(uint8_t groupNumber, uint8_t cellStringNumber)
{
	uint8_t requestDataArray[] = {cellStringNumber};
	sendCan(&hcan1, requestDataArray, 1, CAN_BMS_BASE_ID + 256 + groupNumber, 0, 0);
}

void bms_request_individual_cell_temps_type_B(uint8_t groupNumber, uint8_t cellStringNumber)
{
	uint8_t requestDataArray[] = {groupNumber, cellStringNumber};
	sendCan(&hcan1, requestDataArray, 2, CAN_BMS_INDIVIDUAL_CELL_TEMPS_TYPE_B, 0, 0);
}

void bms_request_individual_cell_balancing_rate_type_A(uint8_t groupNumber, uint8_t cellStringNumber)
{
	uint8_t requestDataArray[] = {cellStringNumber};
	sendCan(&hcan1, requestDataArray, 1, CAN_BMS_BASE_ID + 96 + groupNumber, 0, 0);
}

void bms_request_individual_cell_balancing_rate_type_B(uint8_t groupNumber, uint8_t cellStringNumber)
{
	uint8_t requestDataArray[] = {groupNumber, cellStringNumber};
	sendCan(&hcan1, requestDataArray, 2, CAN_BMS_INDIVIDUAL_CELL_BALANCING_RATE_TYPE_B, 0, 0);
}

void bms_request_state_of_charge_parameters()
{
	uint8_t emptyDataArray[] = {0, 0, 0, 0 ,0, 0, 0, 0};
	sendCan(&hcan1, emptyDataArray, 0, CAN_BMS_STATE_OF_CHARGE, 0, 0);
}

void bms_request_set_state_of_charge(uint8_t newStateOfCharge)
{
	uint8_t requestDataArray[] = {0, 0, 0, 0 ,0, 0, newStateOfCharge, 0};
	sendCan(&hcan1, requestDataArray, 8, CAN_BMS_STATE_OF_CHARGE, 0, 0);
}

void bms_request_contactor_control(uint8_t contactorState)
{
	uint8_t requestDataArray[] = {contactorState};
	sendCan(&hcan1, requestDataArray, 1, CAN_BMS_CONTACTOR_CONTROL, 0, 0);
}

void bms_request_energy_parameters()
{
	uint8_t emptyDataArray[] = {0, 0, 0, 0 ,0, 0, 0, 0};
	sendCan(&hcan1, emptyDataArray, 0, CAN_BMS_ENERGY_PARAM, 0, 0);
}

void bms_request_all_statistics()
{
	uint8_t emptyDataArray[] = {0, 0, 0, 0 ,0, 0, 0, 0};
	sendCan(&hcan1, emptyDataArray, 0, CAN_BMS_STATS, 0, 0);
}

void bms_request_inidividual_statistic(uint8_t statisticID)
{
	uint8_t requestDataArray[] = {statisticID};
	sendCan(&hcan1, requestDataArray, 1, CAN_BMS_STATS, 0, 0);
}

void bms_request_clear_all_statistics()
{
	uint8_t clearStats = 0xFF;
	uint8_t requestDataArray[] = {clearStats};
	sendCan(&hcan1, requestDataArray, 1, CAN_BMS_STATS, 0, 0);
}

void bms_request_all_events()
{
	uint8_t emptyDataArray[] = {0, 0, 0, 0 ,0, 0, 0, 0};
	sendCan(&hcan1, emptyDataArray, 0, CAN_BMS_EVENTS, 0, 0);
}

void bms_request_clear_all_events()
{
	uint8_t clearStats = 0xFF;
	uint8_t reqeustDataArray[] = {clearStats};
	sendCan(&hcan1, reqeustDataArray, 1, CAN_BMS_EVENTS, 0, 0);
}

// Getters
// Overall status getters
uint8_t get_bms_input_signals()
{
	return BMS_INPUT_SIGNALS;
}

uint8_t get_bms_output_signals()
{
	return BMS_OUTPUT_SIGNALS;
}

uint16_t get_bms_num_live_cells()
{
	return BMS_NUM_LIVE_CELLS;
}

uint8_t get_bms_charging_stage()
{
	return BMS_CHARGING_STAGE;
}

uint16_t get_bms_charging_stage_time()
{
	return BMS_CHARGING_STAGE_TIME;
}

uint8_t get_bms_last_charging_error()
{
	return BMS_LAST_CHARGING_ERROR;
}

// Diagnostic code getters
uint8_t get_bms_protection_flags()
{
	return PROTECTION_FLAGS;
}

uint8_t get_bms_warning_flags()
{
	return WARNING_FLAGS;
}

uint8_t get_bms_battery_status_flags()
{
	return BATTERY_STATUS_FLAGS;
}

// Battery voltage overall parameter getters
uint8_t get_bms_high_voltage()
{
	return BMS_HIGH_VOLTAGE;
}

uint8_t	get_bms_low_voltage()
{
	return BMS_LOW_VOLTAGE;
}

uint8_t get_bms_average_voltage()
{
	return BMS_AVERAGE_VOLTAGE;
}

// Cell module temperature overall parameter getters
uint8_t get_bms_min_cell_module_temp()
{
	return BMS_MIN_CELL_MODULE_TEMP;
}

uint8_t get_bms_max_cell_module_temp()
{
	return BMS_MAX_CELL_MODULE_TEMP;
}
uint8_t get_bms_average_cell_module_temp()
{
	return BMS_AVERAGE_CELL_MODULE_TEMP;
}

// Cell temperature overall parameter getters
uint8_t get_bms_high_temp()
{
	return BMS_HIGH_TEMP;
}

uint8_t get_bms_low_temp()
{
	return BMS_LOW_TEMP;
}

uint8_t get_bms_average_temp()
{
	return BMS_AVERAGE_TEMP;
}

// Cell balancing rate overall parameter getters
uint8_t get_bms_min_cell_balancing_rate()
{
	return BMS_MIN_CELL_BALANCING_RATE;
}

uint8_t get_bms_max_cell_balancing_rate()
{
	return BMS_MAX_CELL_BALANCING_RATE;
}

uint8_t get_bms_average_cell_balancing_rate()
{
	return BMS_AVERAGE_CELL_BALANCING_RATE;
}

// Stage of chare parameter getters
uint16_t get_bms_current()
{
	return BMS_CURRENT;
}

uint16_t get_bms_estimated_charge()
{
	return BMS_ESTIMATED_CHARGE;
}

uint8_t get_bms_estimated_state_of_charge()
{
	return BMS_ESTIMATED_STATE_OF_CHARGE;
}

// bms contactor state getter
uint8_t get_bms_contactor_state()
{
	return BMS_CONTACTOR_STATE;
}

// Energy parameter getters
uint16_t get_bms_estimated_consumption()
{
	return BMS_ESTIMATED_CONSUMPTION;
}

uint16_t get_bms_estimated_energy()
{
	return BMS_ESTIMATED_ENERGY;
}

uint16_t get_bms_estimated_distance_left()
{
	return BMS_ESTIMATED_DISTANCE_LEFT;
}

uint16_t get_bms_distance_traveled()
{
	return BMS_DISTANCE_TRAVELED;
}

// Statistic getters
uint8_t get_bms_stat_id()
{
	return BMS_STAT_ID;
}

uint8_t get_bms_stat_data_type()
{
	return BMS_STAT_DATA_TYPE;
}
// need more here for collecting the data with the 3 statistic data types

// Event getters
uint8_t get_bms_event_entry_number()
{
	return BMS_EVENT_ENTRY_NUMBER;
}

uint8_t get_bms_event_data_type()
{
	return BMS_EVENT_DATA_TYPE;
}

uint8_t get_bms_event_id()
{
	return BMS_EVENT_ID;
}

uint32_t get_bms_event_timestamp_data()
{
	return BMS_EVENT_TIMESTAMP_DATA;
}