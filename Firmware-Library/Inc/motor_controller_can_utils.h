

#ifndef _MOTOR_CONTROLLER_CAN_H
#define _MOTOR_CONTROLLER_CAN_H

#include "stdint.h"

//Defines

#define MC_GEN5             0

#define TORQUE_MODE			0
#define SPEED_MODE			1

#define BUS_DISCHARGED 		0
#define BUS_CHARGED    		1

#define MC_COMMAND_READ 	0
#define MC_COMMAND_WRITE 	1

#define MC_COMMAND_MSG			 0x0C0
#define MC_PARAM_COMMAND_MSG	 0x0C1

#define MC_ENABLE_BYTE_4 		 0b11100101
#define MC_ENABLE_BYTE_5		 0b00000000

#define CAN_MC_RX_TEMP1_ID					0x0A0
#define CAN_MC_RX_TEMP2_ID					0x0A1
#define CAN_MC_RX_TEMP3_ID					0x0A2
#define CAN_MC_RX_ANALOG_INPUTS_VOLTAGE		0x0A3
#define CAN_MC_RX_DIGITAL_INPUT_STATUS		0x0A4
#define CAN_MC_RX_MOTOR_ID					0x0A5
#define CAN_MC_RX_CURRENT_ID				0x0A6
#define CAN_MC_RX_VOLT_ID					0x0A7
#define CAN_MC_RX_FAULT_ID					0x0AB
#define CAN_MC_RX_INTERNAL_VOLTAGES 		0x0A9
#define CAN_MC_RX_INTERNAL_STATES			0x0AA
#define CAN_MC_RX_TORQUE_TIMER_INFO			0x0AC
#define CAN_MC_RX_MODULATION_INDEX			0x0AD
#define CAN_MC_RX_FIRMWARE_INFO				0x0AE
#define CAN_MC_RX_DIAGNOSTIC_DATA			0x0AF
#define CAN_MC_RX_HIGHSPEED					0x0B0
#define CAN_MC_RX_TORQUE_CAPABILITY			0x0B1

#define PEAK_TORQUE			230	//Peak torque for EMRAX 228 motor

_Bool isMcCanId(uint16_t canId);

//Getters
float mc_getIGBTACurrent();
float mc_getIGBTBCurrent();
float mc_getIGBTCCurrent();
float mc_getAverageIGBTTemp();

float mc_get_mc_control_board_temp();
float mc_get_RTD_temp_1();
float mc_get_RTD_temp_2();
float mc_get_RTD_temp_3();

float mc_get_coolant_temp();
float mc_get_hot_spot_temp();
float mc_getMotorTemp();
float mc_get_torque_shudder();

//These are boolean values.
int8_t mc_get_forward_switch();
int8_t mc_get_reverse_switch();
int8_t mc_get_brake_switch();
int8_t mc_get_REGEN_disable();
int8_t mc_get_ignition_switch();
int8_t mc_get_start_switch();
int8_t mc_get_valet_mode();
int8_t mc_digital_input_8();

float mc_get_motor_angle();
int mc_get_motor_RPM();
float mc_get_electrical_output_freq();
float mc_get_delta_resolver_filtered();

float mc_get_current_A();
float mc_get_current_B();
float mc_get_current_C();
float mc_getBusCurrent();

float mc_getBusVoltage();
float mc_getOutputVoltage();
float mc_get_vd();
float mc_get_vq();

float mc_get_modulation_index();
float mc_get_flux_weakening_point();
float mc_get_id_command();
float mc_get_iq_command();

float mc_get_onehalf_volt_ref();
float mc_get_twohalf_volt_ref();
float mc_get_five_volt_ref();
float mc_get_twelve_volt_ref();

int mc_get_VSM_state();
int mc_get_PWM_freq();
int mc_get_inverter_state();
int mc_get_relay_state();
int mc_get_inverter_run_mode();
int mc_get_self_sensing_assist_enable();
int mc_get_inverter_active_discharge_state();
int mc_get_inverter_command_mode();
int mc_get_rolling_counter_value();
int mc_get_inverter_enable_state();
int mc_get_burst_model_mode();
int mc_get_start_mode_active();
int mc_get_inverter_enable_lockout();
int mc_get_direction_command();
int mc_get_BMS_Active();
int mc_get_BMS_limiting_torque();
int mc_get_limit_max_speed();
int mc_get_limit_max_hot_spot();
int mc_get_low_speed_limit();
int mc_get_coolant_temperature_limit();
int mc_get_limit_stall_burst_model();

float mc_getCommandedTorque();
float mc_getFeedbackTorque();


float mc_getOutputVoltage();



//Setters
void mc_set_torque_limit(int setTorque);
void mc_set_torque(int setTorque);
void mc_set_speed(int setSpeed);
void mc_set_direction(uint8_t setDirection);
void mc_set_inverter_enable(uint8_t setEnable);
void mc_set_inverter_discharge(uint8_t setEnable);
uint8_t isMCBusCharged();

//Can messaging processing
void mc_process_temp1_can(uint8_t * data);
void mc_process_temp2_can(uint8_t * data);
void mc_process_temp3_can(uint8_t * data);
void mc_process_analog_inputs_voltage_can(uint8_t * data);
void mc_process_digital_input_status_can(uint8_t * data);
void mc_process_motor_can(uint8_t * data);
void mc_process_current_can(uint8_t * data);
void mc_process_volt_can(uint8_t * data);
void mc_process_fault_can(uint8_t * data);
void mc_process_internal_volt_can(uint8_t * data);
void mc_process_internal_states_can(uint8_t * data);
void mc_process_torque_timer_info_can(uint8_t * data);
void mc_process_modulation_index_can(uint8_t * data);
void mc_process_firmware_info_can(uint8_t * data);
void mc_process_diagnostic_data_can(uint8_t * data);
void mc_process_fast_can(uint8_t * data);
void mc_process_torque_capability_can(uint8_t * data);

void sendTorque(int16_t);

// Motor Controller send message
void mc_send_command_msg(uint8_t mode);
void mc_send_param_command_message(uint8_t param_address, uint8_t RW, uint8_t * Data);

// Motor controller broadcasting messaging
void mc_enable_broadcast_msgs();
void mc_disable_broadcast_msgs();

//Motor controller modifying states
void UpdateMCState(int16_t mc_trottle_val);
void EnableMC();
void DisableMC();

//Clear faults in motor controller
void fixFaults();

//Enumerated Values for changing motor controller state
typedef enum{
    MC_DISABLED = 0,
    MC_ENABLED = 1
}mc_state_t;
#endif
