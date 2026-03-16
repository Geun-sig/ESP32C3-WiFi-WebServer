#include "adc_reader.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"

#define ADC_UNIT     ADC_UNIT_1
#define ADC_CHANNEL  ADC_CHANNEL_2   // GPIO2

static adc_oneshot_unit_handle_t adc1_handle;

void adc_reader_init(void)
{
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc1_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten    = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &chan_cfg));
}

int adc_reader_get(void)
{
    int raw = 0;
    adc_oneshot_read(adc1_handle, ADC_CHANNEL, &raw);
    return raw;
}
