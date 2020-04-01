/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "fsl_gpio.h"
#include "fsl_iomuxc.h"
#include "fsl_adc.h"

#include <sntp.h>

#include "HAL.h"
#include "OSAL_Api.h"
#include "LOG_Api.h"

#include "wisafe_main.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define BATTERY_ADC_BASE                      ADC1
#define BATTERY_ADC_CHANNEL_GROUP             0U
#define BATTERY_ADC_USER_CHANNEL              13U



#define BATTERY_CHARGE_DISCHARGE_THRESHOLD_MV 3840         // 3 cells @ 1.28V per cell
#define BATTERY_CHARGE_LOWER_THRESHOLD_MV     3900         // 3 cells @ 1.3V per cell
#define BATTERY_CHARGE_UPPER_THRESHOLD_MV     4650         // 3 cells @ 1.55V per cell
#define CHARGING_DURATION_4HR_IN_SECONDS      4 * 60 * 60  // 4 hours charging duration
#define CHARGING_DURATION_16HR_IN_SECONDS     16 * 60 * 60 // 16 hours charging duration

#define SWITCH_OPEN_THRESHOLD                 3000

/* ADC to millivolt conversion factor obtained via experimentation */
#define ADC_TO_MV_CONVERSION_FACTOR           1233

#define BATTERY_CHARGE_GPIO                   GPIO2
#define BATTERY_CHARGE_GPIO_PIN               (30U)

#define MAINS_DETECT_GPIO                     GPIO3
#define MAINS_DETECT_GPIO_PIN                 (12U)


/*******************************************************************************
 * Variables
 ******************************************************************************/

static bool chargingEnabled = false;
static bool batterySwitchOpen = false;



/*******************************************************************************
 * Prorotypes
 ******************************************************************************/

extern bool isMainsConnected(void);


/*******************************************************************************
 * Code
 ******************************************************************************/


/*!
 * @brief Function to set the battery enable output to the requested state
 */
/* Battery charging thresholds and duration as advised by Panasonic */
static void setBatteryChargeEnable(bool enable)
{
    if (enable)
    {
        //LOG_Info("Activate battery charge enable pin");
        GPIO_WritePinOutput(BATTERY_CHARGE_GPIO, BATTERY_CHARGE_GPIO_PIN, 0);
        chargingEnabled = true;
    }
    else
    {
        //LOG_Info("Deactivate battery charge enable pin");
        GPIO_WritePinOutput(BATTERY_CHARGE_GPIO, BATTERY_CHARGE_GPIO_PIN, 1);
        chargingEnabled = false;
    }
}


/*!
 * @brief Function to read battery voltage via adc
 */
static uint32_t read_battery_voltage(void)
{
    uint32_t adcVal;
    adc_channel_config_t adcChannelConfigStruct;
    /* When in software trigger mode, each conversion would be launched once calling the "ADC_ChannelConfigure()" function,
     * which works like writing a conversion command and executing it. For another channel's conversion, just to change the
     * "channelNumber" field in channel's configuration structure, and call the "ADC_ChannelConfigure() again. */
    /* Configure the user channel and interrupt. */
    adcChannelConfigStruct.channelNumber = BATTERY_ADC_USER_CHANNEL;
    adcChannelConfigStruct.enableInterruptOnConversionCompleted = false;

    ADC_SetChannelConfig(BATTERY_ADC_BASE, BATTERY_ADC_CHANNEL_GROUP, &adcChannelConfigStruct);
    while (0U == ADC_GetChannelStatusFlags(BATTERY_ADC_BASE, BATTERY_ADC_CHANNEL_GROUP))
    {
        OSAL_sleep_ms(10);
    }
    adcVal = ADC_GetChannelConversionValue(BATTERY_ADC_BASE, BATTERY_ADC_CHANNEL_GROUP);
    //LOG_Info("ADC Value: %4d", adcVal);

    /* calculate battery voltage adjusting the value to account for voltage divider on the battery board */
    return (adcVal * 1000 * 2) / ADC_TO_MV_CONVERSION_FACTOR;
}


/*!
 * @brief Function to manage battery charging
 */
/* Battery charging thresholds and duration as advised by Panasonic */
static void battery_manager(void *arg)
{
    uint32_t batteryMilliVoltage;
    uint32_t actualBatteryMilliVoltage;
    uint32_t milliVoltageDiff;
    uint32_t secondsSincePowerup;
    uint8_t* serialNum2;
    // Note that battery charging times are calculated relative to time from unit power up and are not related to calendar time
    bool mainsConnected;
    bool chargeRequest;
    uint32_t chargeTimeEnd = 0;
    bool switchOpen;
    uint32_t switchDetectCount = 0;

    while (1)
    {
        // note that true battery voltage is measured when power is not applied to the battery
        batteryMilliVoltage = read_battery_voltage();
        actualBatteryMilliVoltage = batteryMilliVoltage;

        chargeRequest = false;
        mainsConnected = isMainsConnected();
        if (mainsConnected)
        {
            // LOG_Info("Mains Connected");

            secondsSincePowerup = xTaskGetTickCount() / configTICK_RATE_HZ;

            if (chargingEnabled)
            {
                //LOG_Info("Charging enabled");
                setBatteryChargeEnable(false);
                OSAL_sleep_ms(100);
                actualBatteryMilliVoltage = read_battery_voltage();
                setBatteryChargeEnable(true);
            }
            else
            {
                //LOG_Info("Charging not enabled");
                setBatteryChargeEnable(true);
                OSAL_sleep_ms(100);
                batteryMilliVoltage = read_battery_voltage();
                setBatteryChargeEnable(false);
            }

            if ( batteryMilliVoltage >= actualBatteryMilliVoltage)
            {
                milliVoltageDiff = batteryMilliVoltage - actualBatteryMilliVoltage;
            }
            else
            {
                milliVoltageDiff = actualBatteryMilliVoltage - batteryMilliVoltage;
            }
            //LOG_Info("Battery voltage = %d.%02dV, Actual battery voltage = %d.%02dV, Voltage difference = %dmv", batteryMilliVoltage/1000, batteryMilliVoltage % 1000 / 10, actualBatteryMilliVoltage/1000, actualBatteryMilliVoltage % 1000 / 10, milliVoltageDiff);

            /* check for battery switch open */
            if ( milliVoltageDiff > SWITCH_OPEN_THRESHOLD )
            {
                switchOpen = true;
                Gateway_Status_request.Switch_Position=0;
            }
            else
            {
                //LOG_Info("Battery switch closed");
                switchOpen = false;
                Gateway_Status_request.Switch_Position=1;
            }

            // debounce battery switch open
            if (batterySwitchOpen != switchOpen)
            {
                switchDetectCount++;
                if (switchDetectCount >= 2)
                {
                    // LOG_Info("Battery switch state threshold exceeded, changing debounced switch state to %d",switchOpen);
                    batterySwitchOpen = switchOpen;
                    switchDetectCount = 0;
                }
            }
            else
            {
                switchDetectCount = 0;
            }

            // if battery switch is closed
            if ( (batterySwitchOpen == switchOpen) && !batterySwitchOpen )
            {
                //LOG_Info("Battery switch is closed");

                /* if voltage below discharge threshold and we are not charging then initiate charge */
                if ((actualBatteryMilliVoltage <= BATTERY_CHARGE_DISCHARGE_THRESHOLD_MV) && (chargeTimeEnd < secondsSincePowerup))
                {
                    /* set charging end time */
                    chargeTimeEnd = secondsSincePowerup + CHARGING_DURATION_16HR_IN_SECONDS;
                    LOG_Info("Battery voltage is below discharge threshold and not charging so set charge time end = %d", chargeTimeEnd);
                    Gateway_Status_request.Charging_Fault=0x1;
                }
                else
                {
                    /* if voltage below lower threshold and we are not charging then initiate charge */
                    if ((actualBatteryMilliVoltage <= BATTERY_CHARGE_LOWER_THRESHOLD_MV) && (chargeTimeEnd < secondsSincePowerup))
                    {
                        /* set charging end time */
                        chargeTimeEnd = secondsSincePowerup + CHARGING_DURATION_4HR_IN_SECONDS;
                        LOG_Info("Battery voltage is below lower threshold and not charging so set charge time end = %d", chargeTimeEnd);
                        Gateway_Status_request.Charging_Fault=0x2;
                    }
                }

                /* if within a charging period */
                if (chargeTimeEnd > secondsSincePowerup)
                {
                    if (actualBatteryMilliVoltage < BATTERY_CHARGE_UPPER_THRESHOLD_MV)
                    {
                        uint32_t remainingChargeTime = chargeTimeEnd - secondsSincePowerup;
                        LOG_Info("Charging time remaining: %dhr %02dmin %02dsec", remainingChargeTime / 3600, remainingChargeTime / 60 % 60, remainingChargeTime % 60);
                        chargeRequest = true;
                    }
                    else
                    {
                        /* reset charge end time */
                        chargeTimeEnd = 0;
                        LOG_Info("Battery voltage has reached upper threshold so stop charging");
                        Gateway_Status_request.Charging_Fault=0x4;
                    }
                }
            }
            else
            {
                LOG_Warning("Battery switch open");
                chargeTimeEnd = 0;
                Gateway_Status_request.Switch_Position=0;
                
            }
        }
        else
        {
            LOG_Warning("Mains Disconnected");
            chargeTimeEnd = 0;
        }

        LOG_Info("Battery voltage = %d.%03dV", actualBatteryMilliVoltage/1000 , actualBatteryMilliVoltage % 1000);
       
     
        Gateway_Status_request.Battery_voltage[0]=actualBatteryMilliVoltage/1000;
        Gateway_Status_request.Battery_voltage[1]=actualBatteryMilliVoltage % 1000;
        
        setBatteryChargeEnable(chargeRequest);

        if (mainsConnected)
        {
            OSAL_sleep_ms(10000);
        }
        else
        {
            // report battery voltage less frequqntly when running from battery
            OSAL_sleep_ms(60000);
        }
    }
}

/*!
 * @brief Battery manager
 */
int initBatteryManager(void)
{
    adc_config_t adcConfigStruct;
    gpio_pin_config_t gpio_config_output = {kGPIO_DigitalOutput, 0, kGPIO_NoIntmode};
    gpio_pin_config_t gpio_config_input = {kGPIO_DigitalInput, 0, kGPIO_NoIntmode};

    // Initialise BATTERY CHARGE enable output
    GPIO_PinInit(BATTERY_CHARGE_GPIO, BATTERY_CHARGE_GPIO_PIN, &gpio_config_output);
    setBatteryChargeEnable(false);

    // Initialise ADC for battery voltage monitoring
    ADC_GetDefaultConfig(&adcConfigStruct);
    ADC_Init(BATTERY_ADC_BASE, &adcConfigStruct);

    /* Do auto hardware calibration. */
    if (kStatus_Success == ADC_DoAutoCalibration(BATTERY_ADC_BASE))
    {
        //LOG_Info("ADC_DoAntoCalibration() Done.");
    }
    else
    {
        LOG_Error("ADC_DoAutoCalibration() Failed.");
        Gateway_Status_request.Charging_Fault=0x3;
    }

    // create thread to manage battery charging
    if(xTaskCreate(battery_manager, ((const char*)"battery_charging"), configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL) != pdPASS)
    {
        LOG_Error("Battery manager task creation failed!.");
        return -1;
    }

    return 0;
}
