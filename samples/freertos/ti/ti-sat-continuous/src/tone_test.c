/*
 * Simple CW tone test for verifying TI frequency step size
 * Sends tones at base frequency + (step * step_size)
 * Use with spectrum analyzer to verify actual step size
 */

#include <stdint.h>
#include <pthread.h>
#include <FreeRTOS.h>
#include <task.h>

#include <ti/drivers/Board.h>
#include <ti/drivers/rcl/RCL.h>
#include <ti/drivers/rcl/commands/generic.h>

#include "ti_drivers_config.h"
#include "ti_radio_config.h"
#include "pa.h"

#define BASE_FREQ       2482500000U  /* 2482.5 MHz */
#define STEP_SIZE   	28.17

/* Use the SysConfig-generated command structure */
#define cmd rclPacketTxCmdGenericTxTest_ble_gen_0

static RCL_Client rcl_client;
static RCL_Handle rcl_handle;

static void send_tone(uint32_t frequency_hz)
{
	cmd.rfFrequency = frequency_hz;
	cmd.common.timing.relHardStopTime =
		RCL_SCHEDULER_SYSTIM_US(TONE_DURATION_MS * 1000);

	RCL_Command_submit(rcl_handle, &cmd);
	RCL_Command_pend(&cmd);
}

void *mainThread(void *arg0)
{
	/* Initialize RCL */
	RCL_init();
	rcl_handle = RCL_open(&rcl_client, &LRF_config_ble_gen_0);
	if (rcl_handle == NULL) {
		while (1) {}
	}

	/* Configure for CW transmission */
	cmd.txPower.dBm = TI_PA_MAX_DBM;
	cmd.common.scheduling = RCL_Schedule_Now;
	cmd.common.status = RCL_CommandStatus_Idle;
	cmd.config.txWord = 0U;
	cmd.config.fsOff = 1U;
	cmd.config.sendCw = 1U;

	for (;;) {
		send_tone(BASE_FREQ + (STEP_SIZE * 200));
	}

	return NULL;
}

#define THREADSTACKSIZE 2048

int main(void)
{
	pthread_t thread;
	pthread_attr_t attrs;
	struct sched_param priParam;
	int retc;

	Board_init();

	pthread_attr_init(&attrs);

	priParam.sched_priority = 1;
	retc = pthread_attr_setschedparam(&attrs, &priParam);
	retc |= pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
	retc |= pthread_attr_setstacksize(&attrs, THREADSTACKSIZE);
	if (retc != 0) {
		while (1) {}
	}

	retc = pthread_create(&thread, &attrs, mainThread, NULL);
	if (retc != 0) {
		while (1) {}
	}

	vTaskStartScheduler();

	return 0;
}
