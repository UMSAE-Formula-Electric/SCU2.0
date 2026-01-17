/*
 * Created on Jan 14 2019
 * Created by Martin Rickey
 *
 */

#include "motor_controller_can_utils.h"
#include "can_utils.h"
#include "logger.h"
#include "error_handler.h"
#include "FreeRTOS.h"
#include "task.h"

//busvoltage at 333V, 90% of full bus voltage is ~300
//meausred using bench top power supply powered to 270V, then added 100 margin
#define BUS_VOLTAGE_90_PER_LIMIT 2700 //[11000] reduced, was 12600
#define TR_MAX_RPM 6000


//Global variables
int16_t bus_voltage = 0;
int16_t bus_current = 0;
int16_t mc_currentA = 0;
int16_t mc_currentB = 0;
int16_t mc_currentC = 0;

//Temp 1
//Insulated, Gate, Bipolar, Transistor
int16_t mc_igbtA_temp = 0;
int16_t mc_igbtB_temp = 0;
int16_t mc_igbtC_temp = 0;

//Temp 2
// Resistance Temprature Detector
int16_t mc_controlboard_temp = 0;
int16_t mc_RTD_temp_1 = 0;
int16_t mc_RTD_temp_2 = 0;
int16_t mc_RTD_temp_3 = 0;

//Temp 3
int16_t mc_coolant_temp = 0;
int16_t mc_hot_spot_temp = 0;
int16_t mc_motor_temp = 0;
int16_t mc_torque_shudder = 0;

//Digital input status
int8_t mc_forward_switch = 0;
int8_t mc_reverse_switch = 0;
int8_t mc_brake_switch = 0;
int8_t mc_REGEN_disable = 0;
int8_t mc_ignition_switch = 0;
int8_t mc_Start_switch = 0;
int8_t mc_valet_mode = 0;
int8_t mc_digital_input8 = 0;

//internal voltages
int16_t mc_onehalf_volt_ref = 0;
int16_t mc_twohalf_volt_ref = 0;
int16_t mc_five_volt_ref = 0;
int16_t mc_twelve_volt_ref = 0;

//internal states
int8_t mc_VSM_state = 0;
int8_t mc_PWM_freq = 0;
int8_t mc_inverter_state = 0;
int8_t mc_relay_state = 0;
int8_t mc_inverter_run_mode = 0;
int8_t mc_self_sensing_assist_enable = 0;
int8_t mc_inverter_active_discharge_state = 0;
int8_t mc_inverter_command_mode = 0;
int8_t mc_rolling_counter_value = 0;
int8_t mc_inverter_enable_state = 0;
int8_t mc_burst_model_mode = 0;
int8_t mc_start_mode_active = 0;
int8_t mc_inverter_enable_lockout = 0;
int8_t mc_direction_command = 0;
int8_t mc_BMS_active = 0;
int8_t mc_BMS_limiting_torque = 0;
int8_t mc_limit_max_speed = 0;
int8_t mc_limit_hot_spot = 0;
int8_t mc_low_speed_limiting = 0;
int8_t mc_coolant_temperature_limiting = 0;
int8_t mc_limit_stall_burst_model = 0;

//torque timer info
int16_t mc_torque_command = 0;
int16_t mc_torque_feedback = 0;
int32_t mc_power_on_timer = 0;

//modulation index & flux
int16_t mc_modulation_index = 0;
int16_t mc_flux_weakening_output = 0;
int16_t mc_id_command = 0;
int16_t mc_iq_command = 0;

//Motor position info
int16_t mc_angle = 0;
int16_t mc_rpm = 0;
int16_t mc_Electrical_output_freq = 0;
int16_t mc_delta_resolver_filtered = 0;

int16_t mc_output_voltage = 0;

int16_t mc_vd = 0;
int16_t mc_vq = 0;

int8_t  mc_direction = 1;
int8_t  mc_enable_inverter = 0;
int8_t  mc_enable_discharge = 0;
int16_t mc_torque_limit = 0;
int16_t mc_torque = 0;
int16_t mc_speed = 0;

// analog inputs voltages
int16_t mc_analog_input1 = 0;
int16_t mc_analog_input2 = 0;
int16_t mc_analog_input3 = 0;
int16_t mc_analog_input4 = 0;

//Firmware information
int16_t mc_EEPROM_ver = 0;
int16_t mc_software_ver = 0;
int16_t mc_datecode_mmdd = 0;
int16_t mc_datecode_yyyy = 0;

//torque capability
int16_t mc_torque_capability = 0;


// state machine for motor controller
static mc_state_t motor_controller_state = MC_DISABLED;

static void enableRegReading(uint8_t reg, uint8_t freq);

//only VCU keeps track of mc state machine
/*
 * @brief  Disabled the motor controller when there is not throttle input
 *		  re-enables when there is throttle input. Saves power, the documentation tells us to (E-DS-NDrive.pdf pg23 freecoasting on)
 *		  and it reduces spaceship noises
 * @param  mc_trottle_val: the current throittle value to send to the MC (adter processing!!!!)
 * @retval none
 */
void UpdateMCState(int16_t mc_trottle_val) {
	switch (motor_controller_state) {
	case MC_DISABLED:
		if (mc_trottle_val < 0) {
			//neg means go
			EnableMC();
			motor_controller_state = MC_ENABLED;
			logMessage("MC: Enable MC\n", false);
		}
		break;
	case MC_ENABLED:
		if (mc_trottle_val == 0) {
			//soft disable MC to coast
			DisableMC();
			motor_controller_state = MC_DISABLED;
			logMessage("MC: Disable MC\n", false);
		}
	}
}

_Bool isMcCanId(uint16_t canId){
    return (canId == CAN_MC_RX_HIGHSPEED) || (canId == CAN_MC_RX_TEMP1_ID) || (canId == CAN_MC_RX_TEMP2_ID) ||
        (canId == CAN_MC_RX_ANALOG_INPUTS_VOLTAGE) || (canId == CAN_MC_RX_DIGITAL_INPUT_STATUS )||
        (canId == CAN_MC_RX_MOTOR_ID) || (canId == CAN_MC_RX_CURRENT_ID) || (canId == CAN_MC_RX_VOLT_ID) ||
        (canId == CAN_MC_RX_FAULT_ID) || (canId == CAN_MC_RX_INTERNAL_VOLTAGES) || (canId == CAN_MC_RX_INTERNAL_STATES) ||
        (canId == CAN_MC_RX_TORQUE_TIMER_INFO) || (canId == CAN_MC_RX_MODULATION_INDEX) || (canId == CAN_MC_RX_FIRMWARE_INFO) ||
        (canId == CAN_MC_RX_DIAGNOSTIC_DATA) || (canId == CAN_MC_RX_TORQUE_CAPABILITY) || (canId == CAN_MC_RX_TEMP3_ID);
}

/*
 * GETTERS
 */

float mc_getAverageIGBTTemp() {
	return (float)(mc_igbtA_temp + mc_igbtB_temp + mc_igbtC_temp) / 30;
}

float mc_getIGBTATemp() {
	return (float)mc_igbtA_temp / 10;
}

float mc_getIGBTBTemp() {
	return (float)mc_igbtB_temp / 10;
}

float mc_getIGBTCTemp() {
	return (float)mc_igbtC_temp / 10;
}

float mc_get_mc_control_board_temp(){
    return (float)mc_controlboard_temp * 10;
}

float mc_get_RTD_temp_1(){
    return (float)mc_RTD_temp_1 / 10;
}

float mc_get_RTD_temp_2(){
    return (float)mc_RTD_temp_2 / 10;
}

float mc_get_RTD_temp_3(){
    return (float)mc_RTD_temp_3 / 10;
}

float mc_get_coolant_temp(){
    return (float)mc_coolant_temp / 10;
}

float mc_get_hot_spot_temp(){
    return (float)mc_hot_spot_temp /10;
}

float mc_getMotorTemp() {
	return (float)mc_motor_temp / 10;
}

float mc_get_torque_shudder(){
    return (float)mc_torque_shudder / 10;
}

int8_t mc_get_forward_switch(){
    return mc_forward_switch;
}

int8_t mc_get_reverse_switch(){
    return mc_reverse_switch;
}

int8_t mc_get_brake_switch(){
    return mc_reverse_switch;
}

int8_t mc_get_REGEN_disable(){
    return mc_REGEN_disable;
}

int8_t mc_get_ignition_switch(){
    return mc_ignition_switch;
}

int8_t mc_get_start_switch(){
    return mc_Start_switch;
}

int8_t mc_get_valet_mode(){
    return mc_valet_mode;
}

int8_t mc_digital_input_8(){
    return mc_digital_input8;
}

float mc_get_current_A(){
    return (float)mc_currentA / 10;
}

float mc_get_current_B(){
    return (float)mc_currentB / 10;
}

float mc_get_current_C(){
    return  (float)mc_currentC / 10;
}

float mc_getBusCurrent() {
	return (float)bus_current * 10;
}

float mc_getBusVoltage() {
    return (float)bus_voltage / 10;
}

float mc_getOutputVoltage(){
    return (float) mc_output_voltage / 10;
}

float mc_get_vd() {
    return mc_vd / 10;
}

float mc_get_vq() {
    return mc_vq / 10;
}

float mc_get_modulation_index(){
    return (float)mc_modulation_index / 100;
}

float mc_get_flux_weakening_point(){
    return (float)mc_id_command / 10;
}

float mc_get_id_command(){
    return (float) mc_id_command / 10;
}

float mc_get_iq_command(){
    return (float) mc_iq_command / 10;
}

float mc_get_onehalf_volt_ref(){
    return (float) mc_onehalf_volt_ref / 100;
}

float mc_get_twohalf_volt_ref(){
    return (float) mc_twohalf_volt_ref / 100;
}

float mc_get_five_volt_ref(){
    return (float) mc_five_volt_ref / 100;
}

float mc_get_twelve_volt_ref(){
    return (float) mc_twelve_volt_ref / 100;
}

int mc_get_VSM_state(){
    return mc_VSM_state;
}

int mc_get_PWM_freq(){
    return mc_PWM_freq;
}

int mc_get_inverter_state(){
    return mc_inverter_enable_state;
}

int mc_get_relay_state(){
    return mc_relay_state;
}

int mc_get_inverter_run_mode(){
    return mc_inverter_run_mode;
}

int mc_get_self_sensing_assist_enable(){
    return mc_self_sensing_assist_enable;
}

int mc_get_inverter_active_discharge_state(){
    return mc_inverter_active_discharge_state;
}

int mc_get_inverter_command_mode(){
    return mc_inverter_command_mode;
}

int mc_get_rolling_counter_value(){
    return mc_rolling_counter_value;
}

int mc_get_inverter_enable_state(){
    return mc_inverter_enable_state;
}

int mc_get_burst_model_mode(){
    return mc_burst_model_mode;
}

int mc_get_start_mode_active(){
    return mc_start_mode_active;
}

int mc_get_inverter_enable_lockout(){
    return mc_inverter_enable_lockout;
}

int mc_get_direction_command(){
    return mc_direction_command;
}

int mc_get_BMS_Active(){
    return mc_BMS_limiting_torque;
}

int mc_get_BMS_limiting_torque(){
    return mc_BMS_limiting_torque;
}

int mc_get_limit_max_speed(){
    return mc_limit_max_speed;
}

int mc_get_limit_max_hot_spot(){
    return mc_limit_hot_spot;
}

int mc_get_low_speed_limit(){
    return mc_low_speed_limiting;
}

int mc_get_coolant_temperature_limit(){
    return mc_coolant_temperature_limiting;
}

int mc_get_limit_stall_burst_model(){
    return mc_limit_stall_burst_model;
}

float mc_getIGBTACurrent() {
	return (float)mc_currentA / 10;
}

float mc_getIGBTBCurrent() {
	return (float)mc_currentB / 10;
}

float mc_getIGBTCCurrent() {
	return (float)mc_currentC / 10;
}

float mc_get_motor_angle(){
    return (float)mc_angle / 10;
}

int mc_get_motor_RPM() {
	return mc_rpm;
}

float mc_get_electrical_output_freq(){
    return (float)mc_Electrical_output_freq / 10;
}

float mc_get_delta_resolver_filtered(){
    return (float) mc_delta_resolver_filtered / 10;
}

float mc_getCommandedTorque() {
	return (float)mc_torque_command / 10;
}

float mc_getFeedbackTorque() {
	return mc_torque_feedback / 10;
}



/**
 * SETTERS
 */


/**
 * @brief Function to set torque limit.
 * @param setTorque: Value to set torque limit to in N-m, must be between -3276.8 to +3276.7, set to 0 to use EEPROM setting
 * Send value times 10 per documentation
 */
void mc_set_torque_limit(int setTorque) {
	mc_torque_limit = (int16_t) (setTorque * 10);
	mc_send_command_msg(TORQUE_MODE);
}

/**
 * @brief Function to set torque.
 * @param setTorque: Value to set torque to in N-m, must be between -3276.8 to +3276.7 (Send value times 10)
 * Send value times 10 per documentation
 */
void mc_set_torque(int setTorque) {
	mc_torque = (int16_t) (setTorque * 10);
	mc_send_command_msg(TORQUE_MODE);
}

/**
 * @brief Function to set speed.
 * @param setTorque: Value to set torque to in RPM, must be between -32768 to +32767
 */
void mc_set_speed(int setSpeed) {
	mc_speed = (int16_t) (setSpeed);
}

/**
 * @brief Function to set direction.
 * @param setDirection: 0x00 is "reverse", 0x01 is "forward"
 */
void mc_set_direction(uint8_t setDirection) {
	mc_direction = setDirection;
}

/**
 * @brief Function to enable or disable inverter.
 * @param setEnable: 0x00 is disabled, 0x01 is enabled
 */
void mc_set_inverter_enable(uint8_t setEnable) {
	mc_enable_inverter = setEnable;
}

/**
 * @brief Function to enable or disable inverter discharge.
 * @param setEnable: 0x00 is disabled, 0x01 is enabled
 */
void mc_set_inverter_discharge(uint8_t setEnable) {
	mc_enable_discharge = setEnable;
}


/**
 * isMCBusCharged
 *
 * @Brief: This function checks if the MC dc bus is charged. It is intended
 * to be used as a check before closing the AIRs
 *
 * @Return BUS_CHARGED or BUS_DISCHARGED
 */
uint8_t isMCBusCharged() {
	uint8_t isCharged = BUS_DISCHARGED;
	if (bus_voltage > BUS_VOLTAGE_90_PER_LIMIT) {
		isCharged = BUS_CHARGED;
	}
	return isCharged;
}

//TODO: write logs to the process functions
/**
 * PACKET PROCESSING
 */


/**
 * @brief Temperature of Power Module phases A,B,C
 * @param data can message
 */
void mc_process_temp1_can(uint8_t * data) {
    /*
     * Byte
     * 0,1 Module A Temp
     * 2,3 Module B Temp
     * 4,5 Module C Temp
     *
     * 6,7 Gate Drive Board Temp
     */
	mc_igbtA_temp = (data[1] << 8) | data[0];
	mc_igbtB_temp = (data[3] << 8) | data[2];
	mc_igbtC_temp = (data[5] << 8) | data[4];
}

/**
 * @breif Temperature of Control Board
 * @param data can message
 */
void mc_process_temp2_can(uint8_t * data){
    /*
     * Byte
     * 0,1 Control Board Temp
     * 2,3 RTD #1 Temp
     * 4,5 RTD #2 Temp
     * 6,7 RTD #3 Temp
     */
    mc_controlboard_temp = (data[1] << 8) | data[0];
    mc_RTD_temp_1 = (int16_t )(data[3]<<8) | data[2];
    mc_RTD_temp_2 = (int16_t )(data[5] << 8) | data[4];
    mc_RTD_temp_3 = (int16_t )(data[7] << 8) | data[6];
}

/**
 * @breif Temperature of Torque Shudder
 * @param data can message
 */
void mc_process_temp3_can(uint8_t * data) {
    /*
     *Byte
     * 0,1 Coolant Temp/ RTD #5 Temp
     * 2,3 Hot Spot Temp/ RTD Temp
     * 4,5 Motor Temp
     * 6,7 Torque Shudder
     */
    mc_coolant_temp = (data[1]<<8) | data[0];
    mc_hot_spot_temp = (data[3] << 8) |data[2];
	mc_motor_temp = (data[5] << 8) | data[4];
    mc_torque_shudder = (data[7]<<8)|data[6];
}


/**
 * @brief Process analog inputs depending on the firmware version
 * @param data
 */
void mc_process_analog_inputs_voltage_can(uint8_t * data){
    // Firmware version before 1995
    /*
     *Byte  Name    Format  Des
     *0,1   input 1 lowVolt Voltage on input 1
     *2,3   input 2 lowVolt Voltage on input 2
     *4,5   input 3 lowVolt Voltage on input 3
     *6,7   input 4 lowVolt Voltage on input 4
     */

    // Firmware version 1995 and after
    /*
     * Bit#     Format      Desc
     * 0-9      lowVolt     input1
     * 10-19    lowVolt     input2
     * 20-29    lowVolt     input3
     * 32-41    lowVolt     input4
     * 42-51    lowVolt     input5
     * 52-61    lowVolt     input6
     */

    //Firmware where iM-225 motor type
    /*
     * Bit#     Name        format      Desc
     * 0-15     Oil Temp    Temp        Oil temperature of iM-225
     * 16-31    Oil pres    Pressure    Oil Pressure of iM-225
     * 32-41    input 4     lowVolt     Voltage of input 4
     * 42-51    input 5     lowVolt     Voltage of input 5
     * 52-61    input 6     lowVolt     Voltage of input 6
     */

    //Assuming oldest case
    mc_analog_input1 = (data[1] << 8) | data[0];
    mc_analog_input2 = (data[3] << 8) | data[2];
    mc_analog_input3 = (data[5] << 8) | data[4];
    mc_analog_input4 = (data[7] << 8) | data[6];

}

/**
 * @breif Process digital
 * @param data can message
 */
void mc_process_digital_input_status_can(uint8_t * data){
    /*
     * Byte
     * 0 Status of Digital input #1 forward switch
     * 1 Status of Digital input #2 Reverse switch
     * 2 Status of Digital input #3 Brake switch
     * 3 Status of Digital input #4 REGEN disable switch
     * 4 Status of Digital input #5 ignition switch
     * 5 Status of Digital input #6 Start switch
     * 6 Status of Digital input #7 Valet Mode
     * 7 Status of Digital input #8,
     */
    mc_forward_switch = (int8_t )data[0];
    mc_reverse_switch = data[1];
    mc_brake_switch = data[2];
    mc_REGEN_disable = data[3];
    mc_ignition_switch = data[4];
    mc_Start_switch = data[5];
    mc_valet_mode = data[6];
    mc_digital_input8 = data[7];
}

void mc_process_fault_can(uint8_t * inData) {
    if((inData[7] & 64) || 1) {
        //resolver fault
        DisableMC();
        sendTorque(0);

        fixFaults();

        sendTorque(0);
        vTaskDelay(pdMS_TO_TICKS(100));
        EnableMC();
        //sendTorque(0);
        return;

    }
}

void mc_process_internal_volt_can(uint8_t * data){
    /*
     * Byte#    Name    Format      Des
     * 0,1      1.5v    low volt    on of the low voltage references
     * 2,3      2.5v    low volt    one of the low voltage references
     * 4,5      5.0v    low volt    one of the low voltage references
     * 6,7      12.0v   low voltage one of the low voltage references
     */
    mc_onehalf_volt_ref = (int16_t)((data[1] << 8)|data[0]);
    mc_twohalf_volt_ref = (int16_t)((data[3] << 8)|data[2]);
    mc_five_volt_ref = (int16_t)((data[5] << 8) | data[4]);
    mc_twelve_volt_ref = (int16_t )((data[7]<<8)|data[6]);
}

void mc_process_internal_states_can(uint8_t * data){
    /*
     * byte
     * 0
     * 1
     * 2
     * 3
     * 4-[0]
     * 4-[1]
     * 4-[5-7]
     * 5-[0]
     * 5-[4-7]
     * 6-[0]
     * 6-[1]
     * 6-[6]
     * 6-[7]
     * 7-0
     * 7-1
     * 7-2
     * 7-3
     * 7-4
     * 7-5
     * 7-6
     * 7-7
     */

    //masking bits
    uint8_t bit_zero_mask = 0x1;
    uint8_t bit_one_mask = 0x2;
    uint8_t bit_two_mask = 0x4;
    uint8_t bit_three_mask = 0x8;
    uint8_t bit_four_mask = 0x10;
    uint8_t bit_five_mask = 0x20;
    uint8_t bit_six_mask = 0x40;
    uint8_t bit_seven_mask = 0x80;
    uint8_t byte4_bit_five_seven = 0xE0;
    uint8_t byte5_bit_four_seven = 0xF0;

    mc_VSM_state = (int8_t )data[0];
    mc_PWM_freq = (int8_t )data[1];
    mc_inverter_state = (int8_t )data[2];
    mc_relay_state = (int8_t )data[3];
    mc_inverter_run_mode = (int8_t )(data[4] & bit_zero_mask);
    mc_self_sensing_assist_enable = (int8_t )((data[4] & bit_one_mask) >> 1);
    mc_inverter_active_discharge_state = (int8_t )((data[4] & byte4_bit_five_seven) >> 5);
    mc_inverter_command_mode = (int8_t )(data[5] & bit_zero_mask);
    mc_rolling_counter_value = (int8_t )((data[5] & byte5_bit_four_seven) >> 4);
    mc_inverter_enable_state = (int8_t )(data[6] & bit_zero_mask);
    mc_burst_model_mode = (int8_t )((data[6] & bit_one_mask) >> 1);
    mc_Start_switch = (int8_t )((data[6] & bit_six_mask) >> 6);
    mc_inverter_enable_lockout = (int8_t )((data[6] & bit_seven_mask) >> 7);
    mc_direction_command = (int8_t )(data[7] & bit_zero_mask);
    mc_BMS_active = (int8_t )((data[7] & bit_one_mask) >> 1);
    mc_BMS_limiting_torque = (int8_t )((data[7] & bit_two_mask) >> 2);
    mc_limit_max_speed = (int8_t )((data[7] & bit_three_mask) >> 3);
    mc_limit_hot_spot = (int8_t )((data[7] & bit_four_mask) >> 4);
    mc_low_speed_limiting = (int8_t )((data[7] & bit_five_mask) >> 5);
    mc_coolant_temperature_limiting = (int8_t )((data[7] & bit_six_mask) >> 6);
    mc_limit_stall_burst_model = (int8_t )((data[7] & bit_seven_mask) >> 7);

}

void mc_process_torque_timer_info_can(uint8_t * data){
    /*
     * Byte     Name            Format  Description
     * 0,1      Command-Torque  Torque  The command torque
     * 2,3      Torque Feedback Torque  The estimated motor torque based on motor parameters and feedback
     * 4,5,6,7  Power on Timer  (Counts x .003)sec  this timer is updated every 3 msec. This timer will roll-over in approximately 5 months. The timer will reset to 0 to show when a reset of the processor has occurred.
     */
    mc_torque_command = (int16_t )((data[1] << 8) | data[0]);
    mc_torque_feedback = (int16_t )((data[3] << 8)| data[4]);
    mc_power_on_timer = (int32_t)((data[7] << 24) | (data[6] << 16) | (data[5] << 8) | data[4]);
}

void mc_process_modulation_index_can(uint8_t * data){
    /*
     * Byte     Name        Format      Desc
     * 0,1      Mod index   per-unit val
     * 2,3      Flux weak   current
     * 4,5      id command  current
     * 6,7      iq command  current
     */
    mc_modulation_index = (int16_t )((data[1] << 8) | data[0]);
    mc_flux_weakening_output = (int16_t )((data[3] << 8) | data[2]);
    mc_id_command = (int16_t) ((data[5]<<8)|data[4]);
    mc_iq_command = (int16_t )((data[7]<<8)|data[6]);
}


void mc_process_firmware_info_can(uint8_t * data){
    /*
     * Byte Name         Format  Description
     * 0,1  EEPROM Ver   Na      EEPROM Version
     * 2,3  Software Ver Na      Software Version
     * 4,5  Date Code    Na      mmdd
     * 6,7  Date Code    Na      yyyy
     */
    mc_EEPROM_ver = (int16_t) ((data[1] << 8) | data[0]);
    mc_software_ver = (int16_t)((data[3] << 8) | data[2]);
    mc_datecode_mmdd = (int16_t)((data[5] << 8) | data[4]);
    mc_datecode_yyyy = (int16_t)((data[7] << 8) | data[6]);
}

void mc_process_diagnostic_data_can(uint8_t * data){
    //TODO: MC_FUNCTION diagnostic data
    // Documentation states additional documents are needed
}

/**
 * @brief Log the current bus voltage and rpm of the motor controller
 * @param data: data from the can bus
 */
void mc_process_fast_can(uint8_t * data) {

    mc_torque_command = (int16_t )((data[1] << 8) | data[0]);
    mc_torque_feedback = (int16_t )((data[3] << 8) | data[2]);
    mc_rpm = (int16_t )((data[5] << 8) | data[4]);
    bus_voltage = (int16_t )((data[7] << 8) | data[6]);
}

/**
 * @brief Process the torque capability
 * @param data
 */
void mc_process_torque_capability_can(uint8_t * data){
    /*
     * Byte
     * 0,1  Torque Capability
     * 2,3  NA
     * 4,5  NA
     * 6,7  NA
     */
    mc_torque_capability = (int16_t )((data[1] << 8) | data[0]);

}


/**
 * @breif Process Dc bus voltage and output voltage
 * @param data
 */
void mc_process_volt_can(uint8_t * data) {
    /*
     * Byte
     * 0,1  DC Bus Voltage  Actual measured value of the DC bus voltage.
     * 2,3  Output Voltage  The calculated value of the output voltage, in peak line-neutral volts.
     * 4,5  VAB_Vd_Voltage  Measured value of the voltage between Phase A and Phase B (VAB) when the inverter is disabled. Vd voltage when the inverter is enabled
     * 6,7  VBC_Vq_Voltage  Measured value of the voltage between Phase B and Phase C (VBC) when the inverter is disabled. Vq voltage when the inverter is enabled
     */
	bus_voltage = (int16_t )((data[1] << 8) | data[0]);
	mc_output_voltage = (int16_t )((data[3] << 8) | data[2]);
	mc_vd = (int16_t )((data[5] << 8) | data[4]);
	mc_vq = (int16_t )((data[7] << 8) | data[6]);
}

/**
 * @brief Motor Position information
 * @param data can message data
 */
void mc_process_motor_can(uint8_t * data) {
    /*
     * Byte
     * 0,1 Motor angle
     * 2,3 Motor Speed
     * 4,5 Electrical Output Frequency
     * 6,7 Delta Resolver Filtered
     */
    mc_angle = (int16_t )((data[1] << 8) | data[0]);
	mc_speed = (int16_t )((data[3] << 8) | data[2]);
    mc_Electrical_output_freq =(int16_t )((data[5] << 8) | data[4]);
    mc_delta_resolver_filtered = (int16_t )((data[7] << 8) | data[6]);
}

/**
 * @brief Current Information
 * @param data can message
 */
void mc_process_current_can(uint8_t * data) {
	mc_currentA = (int16_t )((data[1] << 8) | data[0]);
	mc_currentB = (int16_t )((data[3] << 8) | data[2]);
	mc_currentC = (int16_t )((data[5] << 8) | data[4]);
	bus_current = (int16_t )((data[7] << 8) | data[6]);
}

/*
 *	Sent whenever a torque request is read by the APPS
 */
void mc_send_command_msg(uint8_t mode) {
	uint8_t len = 8; // DLC MUST be 8 for command message, this is the sendCan bug
	uint8_t data[len];
	uint8_t dest = MC_COMMAND_MSG;

	uint8_t ret = 0;

	//Torque Mode
	if (mode == TORQUE_MODE) {
		data[0] = mc_torque & 0xFF;
		data[1] = (mc_torque >> 8) & 0xFF;
		data[2] = 0x00;
		data[3] = 0x00;
		data[4] = mc_direction;
		data[5] = (mc_enable_inverter);
		data[6] = 0x00;
		data[7] = 0x00;
	}
	//Speed Mode, Set 0x04 bit in data[5] to denote.
	//Probably not used in epbr22
	else if (mode == SPEED_MODE) {
		data[0] = 0x00;
		data[1] = 0x00;
		data[2] = mc_speed & 0xFF;
		data[3] = (mc_speed >> 8) & 0xFF;
		data[4] = mc_direction;
		data[5] = (mc_enable_inverter) | (mc_enable_discharge << 1) | 0x04;
		data[6] = mc_torque_limit & 0xFF;
		data[7] = (mc_torque_limit >> 8) & 0xFF;
	}

	ret = sendCan(&hcan1, data, len, dest, CAN_RTR_DATA, CAN_NO_EXT);
	if (ret != 0) {
		//can error, log it
		log_and_handle_error(ERROR_CAN_ONE_TX_FAIL, NULL);
		logMessage("MC: Failed to send MC command CAN packet\n", false); //should be critical??
	}
}

void sendTorque(int16_t torque) {
	uint8_t len = 8; // DLC MUST be 8 for command message, this is the sendCan bug
    uint8_t data[len];
    uint8_t dest = 0xC0;

    uint8_t ret = 0;

    data[0] = torque & 0xFF;
    data[1] = (torque >> 8) & 0xFF;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = mc_direction;
    data[5] = mc_enable_inverter;
    data[6] = 0x00;
    data[7] = 0x00;

    ret = sendCan(&hcan1, data, len, dest, CAN_RTR_DATA, CAN_NO_EXT);
    if (ret != 0) {
        //can error, log it
        log_and_handle_error(ERROR_CAN_ONE_TX_FAIL, NULL);
        logMessage("MC: Failed to send MC command CAN packet\n", false); //should be critical??
    }
}

/**
 * @brief A method for sending a parameter command message to the motor controller
 *
 * @param param_address: the address for the specific kind of command message (Page 39)
 * @param RW: Read/Write. 0x1 -> write, 0x0 -> read.
 * @param Data: data to be sent, length must be 2 bytes. Data[0] goes to byte 4, Data[1] goes to byte 5.
 */
void mc_send_param_command_message(uint8_t param_address, uint8_t RW, uint8_t * Data) {
	uint8_t len = 8;
	uint8_t data[8];
	uint8_t dest = MC_PARAM_COMMAND_MSG;
	uint8_t ret = 0;

	//Referencing page 19, disabling the periodic can messages we want
	data[0] = param_address;
	data[1] = 0x00;
	data[2] = RW;
	data[3] = 0x00; 	//Reserved
	data[4] = Data[1];
	data[5] = Data[2];
	data[6] = 0x00;		//Reserved
	data[7] = 0x00;		//Reserved

	//ret = sendCan(&hcan1, data, len, dest, CAN_NO_RTR, CAN_NO_EXT);
	//if (ret != 0) {
		//can error, log it

		//log_and_handle_error(ERROR_CAN_ONE_TX_FAIL, NULL);
		//logMessage("MC: Failed to send parameter MC CAN packet\n", false);
	//}
}

/*
 * PARAM COMMAND MESSAGES
 */

/**
 * @brief Set torque to zero
 */
void sendZeroTorque() {
	mc_set_torque(0);
}

void mc_enable_broadcast_msgs() {
	uint8_t data[2];
	data[0] = MC_ENABLE_BYTE_4;
	data[1] = MC_ENABLE_BYTE_5;
	mc_send_param_command_message(CAN_MC_ACTIVE_MESSAGES, MC_COMMAND_WRITE,
			data);
}

void mc_disable_broadcast_msgs() {
	uint8_t data[2];

	data[0] = 0x0;
	data[1] = 0x0;

	mc_send_param_command_message(CAN_MC_ACTIVE_MESSAGES, MC_COMMAND_WRITE,
			data);
}

/**
 * @brief Clears errors in the motor controller
 *
 */
void fixFaults() {
    uint8_t len = 8;
    uint8_t data[len];
    uint8_t dest = MC_PARAM_COMMAND_MSG;

    uint8_t ret = 0;

    data[0] = 20;
    data[1] = 0;
    data[2] = 1;
    data[3] = 0;
    data[4] = 0;
    data[5] = 0;
    data[6] = 0;
    data[7] = 0;

    ret = sendCan(&hcan1, data, len, dest, CAN_NO_RTR, CAN_NO_EXT);
    if (ret != 0) {
        //can error, log it
        log_and_handle_error(ERROR_CAN_ONE_TX_FAIL, NULL);
        logMessage("MC: Failed to send MC command CAN packet\n", false); //should be critical??
    }
}

void EnableMC() {
	mc_enable_inverter = 1;
}

void DisableMC() {
	mc_enable_inverter = 0;
}

/**
 * Change the speed from the motor to wheel speed
 * @return Wheel speed rounded to nearest int
 */
int wheel_speed_MC(){
    int speed_MC = mc_get_motor_RPM();
    float gear_constant = 3.571 ; //Gear and differential ratio of the motor to the wheel axial
    int speed_Wheel = 0;
    float wheel_radius = 0.23;

    speed_Wheel = speed_MC / gear_constant * wheel_radius;

    return speed_Wheel;
}


