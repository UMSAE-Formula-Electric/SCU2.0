//
// Created by caleb on 2024-05-05.
//

#ifndef BMS_CAN_UTILS_H
#define BMS_CAN_UTILS_H

#include "can.h"

// The number of cells in the accumulator (270 cells for ePBR24)
#define NUM_CELLS 270
#define NUM_CELL_PER_EMUS_CELL 3
#define NUM_EMUS_CELLS (NUM_CELLS / NUM_CELL_PER_EMUS_CELL)
#define CELLS_PER_CAN_FRAME	8

#define CAN_BMS_BASE_ID         0x100

#define CAN_BMS_OVERALL_ID		CAN_BMS_BASE_ID + 0
#define CAN_BMS_DIAGNOSTIC_ID	CAN_BMS_BASE_ID + 7
#define CAN_BMS_VOLTAGE_ID		CAN_BMS_BASE_ID + 1

// starting at "individual cell voltages" in the document we are using the option B version of the CAN ID
#define CAN_BMS_MODULE_TEMPERATURE				CAN_BMS_BASE_ID + 2
#define CAN_BMS_CELL_TEMPERATURE				CAN_BMS_BASE_ID + 8
#define CAN_BMS_CELL_BALANCING_RATE				CAN_BMS_BASE_ID + 3

/* TYPE-A CAN IDs are a range of values found dynamically instead of being defined here (look at the function
 * "process_typeA_and_typeB_can_packets" to see the different ranges of CAN IDs.
 *
 * Below are the respective TYPE-B CAN IDs
 */
#define CAN_BMS_INDIVIDUAL_CELL_VOLTAGES_TYPE_B			CAN_BMS_BASE_ID + 11
#define CAN_BMS_INDIVIDUAL_CELL_MODULE_TEMPS_TYPE_B		CAN_BMS_BASE_ID + 12
#define CAN_BMS_INDIVIDUAL_CELL_TEMPS_TYPE_B			CAN_BMS_BASE_ID + 14
#define CAN_BMS_INDIVIDUAL_CELL_BALANCING_RATE_TYPE_B	CAN_BMS_BASE_ID + 13

#define CAN_BMS_STATE_OF_CHARGE					CAN_BMS_BASE_ID + 5

// skipped config params, Log in/Log out, set a new password (all password stuff) in the EMUS document (add later if needed)
// might not need the next 3 defines
#define CAN_BMS_CONTACTOR_CONTROL 				CAN_BMS_BASE_ID + 129
#define CAN_BMS_ENERGY_PARAM					CAN_BMS_BASE_ID + 6
#define CAN_BMS_STATS							CAN_BMS_BASE_ID + 132

#define CAN_BMS_EVENTS							CAN_BMS_BASE_ID + 133

/**
 * Function defs
 */

void init_bms();		// init the bms data structures and mutex

_Bool isBmsCanId(uint32_t canID);

int bms_getHighTemp();
int bms_getLowTemp();
int bms_getAverageTemp();

float bms_getHighVoltage();
float bms_getLowVoltage();
float bms_getAverageVoltage();

void process_bms_overall_packet(uint8_t * Data);
void process_bms_voltage_packet(uint8_t * Data);
void process_bms_temp_packet(uint8_t * Data);
void process_bms_diagnostic_packet(uint8_t * Data);

// new function headers
void process_typeA_and_typeB_can_packets(CAN_RxPacketTypeDef * packetToProcess);
void process_bms_module_temp_can(uint8_t * Data);
// void process_bms_cell_temp_can(uint8_t * Data);
void process_bms_cell_temp_balancing_rate_can(uint8_t * Data);
void process_bms_state_of_charge_can(uint8_t * Data);
void process_bms_contactor_control_can(uint8_t * Data);
void process_bms_energy_param_can(uint8_t * Data);
void process_bms_stats_can(uint8_t * Data);
void process_bms_events_can(uint8_t * Data);

void clear_8_byte_array(uint8_t * array);
void fill_8_byte_array_typeA(uint8_t * array, uint8_t * arrayOfCANData, uint8_t dataLength);
void fill_8_byte_array_typeB(uint8_t * array, uint8_t * arrayOfCANData, uint8_t dataLength);

void bms_handleProtectionFlags(uint16_t Flags);
int bms_checkBit16(uint16_t bytes, int n);
void bms_handleChargingError(uint8_t errorByte);

// Requests (using standard IDs)
void bms_request_overall_parameters();
void bms_request_diagnostic_codes();
void bms_request_overall_battery_voltages();
void bms_request_overall_cell_module_temp();
void bms_request_overall_cell_temp();
void bms_request_overall_cell_balancing_rate();
void bms_request_individual_cell_voltages_type_A(uint8_t groupNumber, uint8_t cellStringNumber);
void bms_request_individual_cell_voltages_type_B(uint8_t groupNumber, uint8_t cellStringNumber);
void bms_request_individual_cell_module_temps_type_A(uint8_t groupNumber, uint8_t cellStringNumber);
void bms_request_individual_cell_module_temps_type_B(uint8_t groupNumber, uint8_t cellStringNumber);
void bms_request_individual_cell_temps_type_A(uint8_t groupNumber, uint8_t cellStringNumber);
void bms_request_individual_cell_temps_type_B(uint8_t groupNumber, uint8_t cellStringNumber);
void bms_request_individual_cell_balancing_rate_type_A(uint8_t groupNumber, uint8_t cellStringNumber);
void bms_request_individual_cell_balancing_rate_type_B(uint8_t groupNumber, uint8_t cellStringNumber);
void bms_request_state_of_charge_parameters();
void bms_request_set_state_of_charge(uint8_t newStateOfCharge);
void bms_request_contactor_control(uint8_t contactorState);
void bms_request_energy_parameters();
void bms_request_all_statistics();
void bms_request_inidividual_statistic(uint8_t statisticID);
void bms_request_clear_all_statistics();
void bms_request_all_events();
void bms_request_clear_all_events();

// Getters
// Overall status getters
uint8_t get_bms_input_signals();
uint8_t get_bms_output_signals();
uint16_t get_bms_num_live_cells();
uint8_t get_bms_charging_stage();
uint16_t get_bms_charging_stage_time();
uint8_t get_bms_last_charging_error();

// Diagnostic code getters
uint8_t get_bms_protection_flags();
uint8_t get_bms_warning_flags();
uint8_t get_bms_battery_status_flags();

// Battery voltage overall parameter getters
uint8_t get_bms_high_voltage();
uint8_t	get_bms_low_voltage();
uint8_t get_bms_average_voltage();

// Cell module temperature overall parameter getters
uint8_t get_bms_min_cell_module_temp();
uint8_t get_bms_max_cell_module_temp();
uint8_t get_bms_average_cell_module_temp();

// Cell temperature overall parameter getters
uint8_t get_bms_high_temp();
uint8_t get_bms_low_temp();
uint8_t get_bms_average_temp();

// Cell balancing rate overall parameter getters
uint8_t get_bms_min_cell_balancing_rate();
uint8_t get_bms_max_cell_balancing_rate();
uint8_t get_bms_average_cell_balancing_rate();

// Stage of chare parameter getters
uint16_t get_bms_current();
uint16_t get_bms_estimated_charge();
uint8_t get_bms_estimated_state_of_charge();

// bms contactor state getter
uint8_t get_bms_contactor_state();

// Energy parameter getters
uint16_t get_bms_estimated_consumption();
uint16_t get_bms_estimated_energy();
uint16_t get_bms_estimated_distance_left();
uint16_t get_bms_distance_traveled();
// Statistic getters
uint8_t get_bms_stat_id();
uint8_t get_bms_stat_data_type();
// need more here for collecting the data with the 3 statistic data types

// Event getters
uint8_t get_bms_event_entry_number();
uint8_t get_bms_event_data_type();
uint8_t get_bms_event_id();
uint32_t get_bms_event_timestamp_data();

// BMS Statistic Identifiers
enum BMS_STAT_ID {
	TOTAL_DISCHARGE,
	TOTAL_CHARGE,
	TOTAL_DISCHARGE_ENERGY,
	TOTAL_CHARGE_ENERGY,
	TOTAL_DISCHARGE_TIME,
	TOTAL_CHARGE_TIME,
	TOTAL_DISTANCE,
	MASTER_CLEAR_COUNT,
	MAX_DISCHARGE_CURRENT,
	MAX_CHARGE_CURRENT,
	MIN_CELL_VOLTAGE,
	MAX_CELL_VOLTAGE,
	MAX_CELL_VOLTAGE_DIFFERENCE,
	MIN_PACK_VOLTAGE,
	MAX_PACK_VOLTAGE,
	MIN_CELL_MODULE_TEMPERATURE,
	MAX_CELL_MODULE_TEMPERATURE,
	MAX_CELL_MODULE_TEMPERATURE_DIFFERENCE,
	BMS_STARTS_COUNT,
	UNDER_VOLTAGE_PROTECTION_COUNT,
	OVER_VOLTAGE_PROTECTION_COUNT,
	DISCHARGE_OVER_CURRENT_PROTECTION_COUNT,
	CHARGE_OVER_CURRENT_PROTECTION_COUNT,
	CELL_MODULE_OVERHEAT_PROTECTION_COUNT,
	LEAKAGE_PROTECTION_COUNT,
	NO_CELL_COMMUNICATION_PROTECTION_COUNT,
	LOW_VOLTAGE_POWER_REDUCTION_COUNT,
	HIGH_CURRENT_POWER_REDUCTION_COUNT,
	HIGH_CELL_MODULE_TEMPERATURE_POWER_REDUCTION_COUNT,
	CHARGER_CONNECT_COUNT,
	CHARGER_DISCONNECT_COUNT,
	PREHEAT_STAGE_COUNT,
	PRECHARGE_STAGE_COUNT,
	MAIN_CHARGE_STAGE_COUNT,
	BALANCING_STAGE_COUNT,
	CHARGING_FINISHED_COUNT,
	CHARGING_ERROR_COUNT,
	CHARGING_RETRY_COUNT,
	TRIPS_COUNT,
	CHARGE_RESTARTS_COUNT,
	CELL_OVERHEAT_PROTECTION_COUNT = 45,
	HIGH_CELL_TEMPERATURE_POWER_REDUCTION_COUNT = 46,
	MIN_CELL_TEMPERATURE = 47,
	MAX_CELL_TEMPERATURE = 48,
	MAX_CELL_TEMPERATURE_DIFFERENCE = 49,

};

// Types of statistic data sent as a response message from the BMS
enum STAT_DATA_TYPE {
	STATISTIC_VALUE,
	ADDITIONAL_VALUE,
	STATISTIC_TIMESTAMP,
};

// BMS CAN Events
enum BMS_EVENT {
	NO_EVENT,
	BMS_STARTED,
	LOST_COMM_TO_CELLS,
	ESTABLISH_COMM_TO_CELLS,
	CELL_VOLT_CRIT_LOW,
	CRIT_LOW_VOLTAGE_RECOVERED,
	CELL_VOLT_CRIT_HIGH,
	CRIT_HIGH_VOLTAGE_RECOVERED,
	DISCHARGE_CURR_CRIT_HIGH,
	DISCHARGE_CRIT_HIGH_CURR_RECOVERED,
	CHARGE_CURR_CRIT_HIGH,
	CHARGE_CRIT_HIGH_CURR_RECOVERED,
	CELL_MODULE_TEMP_CRIT_HIGH,
	CRIT_HIGH_CELL_MODULE_TEMP_RECOVERED,
	LEAKAGE_DETECTED,
	LEAKAGE_RECOVERED,
	LOW_VOLT_REDUCING_POWER_WARNING,
	POWER_REDUCTION_FROM_LOW_VOLT_RECOVERED,
	HIGH_CURR_REDUCING_POWER_WARNING,
	POWER_REDUCTION_FROM_HIGH_CURR_RECOVERED,
	HIGH_CELL_MODULE_TEMP_REDUCING_POWER_WARNING,
	POWER_REDUCTION_FROM_HIGH_CELL_MODULE_TEMP_RECOVERED,
	CHARGER_CONNECTED,
	CHARGER_DISCONNECTED,
	STARTED_PREHEAT_STAGE,
	STARTED_PRECHARGE_STAGE,
	STARTED_MAIN_CHARGING_STAGE,
	STARTED_BALANCING_STAGE,
	CHARGING_FINISHED,
	CHARGING_ERROR_OCCURRED,
	RETRYING_CHARGING,
	RESTARTING_CHARGING,
	CELL_TEMP_CRIT_HIGH = 42,
	CRIT_HIGH_CELL_TEMP_RECOVERED = 43,
	HIGH_CELL_TEMP_REDUCING_POWER_WARNING = 44,
	POWER_REDUCTION_FROM_HIGH_CELL_TEMP_RECOVERED = 45

};

// Types of event data sent as a response message from the BMS
enum EVENT_DATA_TYPE {
	EVENT_INFO,
	EVENT_TIMESTAMP
};

#endif //BMS_CAN_UTILS_H
