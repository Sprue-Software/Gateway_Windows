/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "fsl_gpio.h"
#include "fsl_phy.h"

#include "board.h"
#include <sntp.h>

#include "HAL.h"
#include "OSAL_Api.h"
#include "LOG_Api.h"

#include "pin_mux.h"
#include "wisafe_main.h"

#include "fsl_common.h"
#include "fsl_iomuxc.h"

//nishi
#define PRIORITIE_OFFSET			( 4 )
/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define MAINS_RESTORATION_DELAY               3               // delay applied before mains restoration activities are actioned
#define FALLBACK_USB_ID_PIN_TOGGLE_DELAY      6 * 60 * 60     // fallback usb id pin toggle delay to ensure tablet charges
#define SYSTEM_INIT_USB_ID_PIN_TOGGLE_DELAY   60              // usb id pin toggle delay to ensure system acquires valid calendar time
#define INITIAL_USB_ID_PIN_TOGGLE_DELAY       5               // initial (after power on) usb id pin toggle delay to ensure tablet charges    

#define MAINS_DETECT_GPIO                     GPIO3
#define MAINS_DETECT_GPIO_PIN                 (12U)

#define USB_OTG1_ID_GPIO                      GPIO1
#define USB_OTG1_ID_GPIO_PIN                  (1U)


/*******************************************************************************
 * Extern functions
 ******************************************************************************/

extern bool getSystemRunStatus(void);

/*******************************************************************************
 * Variables
 ******************************************************************************/

static uint8_t seqNum = 0;

static bool mainsConnected = true;
#if 0
static time_t lastUsbIdPinToggleTimeSecs = 0;   // note the time when the USB ID pin was last toggled
#endif

/*******************************************************************************
 * Code
 ******************************************************************************/


/*!
 * @brief Report mains connected status
 */
bool isMainsConnected(void)
{
    return mainsConnected;
}


/*!
 * @brief Toggles the tablet usb id pin to initiate tablet charging
 */
void toggleTabletUsbIdPin(void)
{
    // apply a 100ms pulse to the USB OTG ID pin to enable tablet charging
    LOG_Info("Toggling USB ID pin");
    GPIO_WritePinOutput(USB_OTG1_ID_GPIO, USB_OTG1_ID_GPIO_PIN, 0);
    OSAL_sleep_ms(100);
    GPIO_WritePinOutput(USB_OTG1_ID_GPIO, USB_OTG1_ID_GPIO_PIN, 1);
    OSAL_sleep_ms(100);
    GPIO_WritePinOutput(USB_OTG1_ID_GPIO, USB_OTG1_ID_GPIO_PIN, 0);
   // lastUsbIdPinToggleTimeSecs = OSAL_time_ms() / 1000; //nishi
}


/*!
 * @ABR added
 * @brief Toggles the tablet usb id pin to initiate tablet charging
 */
static void toggleTabletUsbIdPinToReconnectToWg(void)
{    
    // apply a 100ms pulse to the USB OTG ID pin to enable tablet charging
    LOG_Info("Toggling USB ID pin for resuming tethering");
    GPIO_WritePinOutput(USB_OTG1_ID_GPIO, USB_OTG1_ID_GPIO_PIN, 0);
    OSAL_sleep_ms(100);
    GPIO_WritePinOutput(USB_OTG1_ID_GPIO, USB_OTG1_ID_GPIO_PIN, 1);
    OSAL_sleep_ms(100);
    // Set Drive Strength to HI-Z 
    IOMUXC_SetPinConfig(
          IOMUXC_GPIO_AD_B0_01_GPIO1_IO01,        
          0x0000u);
    OSAL_sleep_ms(1000);
    // Set Drive Strength back
    IOMUXC_SetPinConfig(
          IOMUXC_GPIO_AD_B0_01_GPIO1_IO01,        
          0x1030u);
    OSAL_sleep_ms(100);
    GPIO_WritePinOutput(USB_OTG1_ID_GPIO, USB_OTG1_ID_GPIO_PIN, 0);
   // lastUsbIdPinToggleTimeSecs = OSAL_time_ms() / 1000;
}


/*!
 * @brief Main thread function to monitor mains power
 */
static void power_monitor(void *arg)
{
    static bool firstMainsConnection = true;
    bool lastMainsConnected = false;

    // Disable wired ethernet if mains disconnected
    mainsConnected = GPIO_PinRead(MAINS_DETECT_GPIO, MAINS_DETECT_GPIO_PIN);
    if (!mainsConnected)
    {
        // disable wired Ethernet
        enable_wired_ethernet(false);
    }

    while (1)
    {
        mainsConnected = GPIO_PinRead(MAINS_DETECT_GPIO, MAINS_DETECT_GPIO_PIN);
        if (mainsConnected)
        {
            //LOG_Info("Mains Connected");
        }
        else
        {
            LOG_Warning("Mains Disconnected");
        }

        // generate event if mains status has changed
        if (mainsConnected != lastMainsConnected)
        {
            if (mainsConnected)
            {
                bool mainsOn = true;

                // wait for mains to stabilise
                for (int i=0; i < MAINS_RESTORATION_DELAY; i++)
                {
                    if ( GPIO_PinRead(MAINS_DETECT_GPIO, MAINS_DETECT_GPIO_PIN) == 0)
                    {
                        mainsOn = false;
                    }
                    OSAL_sleep_ms(1000);
                }

                if (mainsOn)
                {
                    // enable wired Ethernet
                    enable_wired_ethernet(true);
                    //Nishi
        //            PHY_Init(BOARD_ENET_BASEADDR, BOARD_ENET0_PHY_ADDRESS, CLOCK_GetFreq(kCLOCK_CoreSysClk));

                    // send event to ensoAgent
                    uint8_t mainsConnectedEventMsg[] = {0x71,0xFF,0xFF,0xFF,0x1C,0x04,0x05,0x00,0x00,0x7E};
                    mainsConnectedEventMsg[8] = seqNum;
                    seqNum = (seqNum +1) & 0x0F;
                    LOG_Info("Sending Mains Connected event message to cloud");
                    writeWGqueue(mainsConnectedEventMsg);

                    toggleTabletUsbIdPin();
                    lastMainsConnected = mainsConnected;
                }
            }
            else
            {
                // disable wired Ethernet
                enable_wired_ethernet(false);

                // send event to ensoAgent
                uint8_t mainsDisconnectedEventMsg[] = {0x71,0xFF,0xFF,0xFF,0x1C,0x04,0x15,0x00,0x00,0x7E};
                mainsDisconnectedEventMsg[8] = seqNum;
                seqNum = (seqNum +1) & 0x0F;
                LOG_Info("Sending Mains Disconnected event message to cloud");
                writeWGqueue(mainsDisconnectedEventMsg);
                lastMainsConnected = mainsConnected;
            }
        }
        else
        {
            // check for system operational
            if (getSystemRunStatus())
            {
                if (mainsConnected)
                {
                    // ensure that the tablet is aware that mains power is available by periodically toggling usb id pin
                    // covering the situation where the tablet may fail to initiate charging following mains power restoration
                    if (firstMainsConnection)
                    {
                      //Nishi // if (((OSAL_time_ms()/1000) - lastUsbIdPinToggleTimeSecs) >= INITIAL_USB_ID_PIN_TOGGLE_DELAY)
                        {
                            firstMainsConnection = false;
                            toggleTabletUsbIdPinToReconnectToWg();
                        }
                    }
                    else
                    {
                     //Nishi   if (((OSAL_time_ms()/1000) - lastUsbIdPinToggleTimeSecs) >= FALLBACK_USB_ID_PIN_TOGGLE_DELAY)
                        {                            
                            toggleTabletUsbIdPin();
                        }
                    }                    
                }
            }
            else
            {
                // re-init usb link if we do not have network connectivity and/or calendar time to allow for situation where
                // tablet initialises after the gateway starts up
            //Nishi//    if (((OSAL_time_ms()/1000) - lastUsbIdPinToggleTimeSecs) >= SYSTEM_INIT_USB_ID_PIN_TOGGLE_DELAY)
                {
                    toggleTabletUsbIdPin();
                }
            }
        }
        OSAL_sleep_ms(1000);
    }
}



/*!
 * @brief Initialise power monitor
 */
int initPowerMonitor(void)
{
    gpio_pin_config_t gpio_config_input = {kGPIO_DigitalInput, 0, kGPIO_NoIntmode};
    gpio_pin_config_t gpio_config_output = {kGPIO_DigitalOutput, 0, kGPIO_NoIntmode};

    // Initialize DC 5V IN SENSE pin
    GPIO_PinInit(MAINS_DETECT_GPIO, MAINS_DETECT_GPIO_PIN, &gpio_config_input);

    // Initialize USB_OTG1_ID pin
    GPIO_PinInit(USB_OTG1_ID_GPIO, USB_OTG1_ID_GPIO_PIN, &gpio_config_output);
    GPIO_WritePinOutput(USB_OTG1_ID_GPIO, USB_OTG1_ID_GPIO_PIN, 0);

    // create thread to monitor mains power and to 
    if(xTaskCreate(power_monitor, ((const char*)"power monitor"), configMINIMAL_STACK_SIZE, NULL,tskIDLE_PRIORITY + 1 + PRIORITIE_OFFSET, NULL) != pdPASS)
    {
        LOG_Error("Power monitor task creation failed!.");
        return -1;
    }

    return 0;
}
